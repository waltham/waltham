/*
 * Copyright © 2016 DENSO CORPORATION
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <sys/types.h>
#include <unistd.h>

#include <waltham-object.h>
#include <waltham-client.h>
#include <waltham-connection.h>

#include "w-util.h"

#define MAX_EPOLL_WATCHES 2

struct display;

/* epoll structure */
struct watch {
	struct display *display;
	int fd;
	void (*cb)(struct watch *w, uint32_t events);
};

struct display {
	struct wth_connection *connection;
	struct watch conn_watch;
	struct wth_display *display;

	bool running;
	int epoll_fd;

	struct wthp_registry *registry;

	struct wthp_callback *bling;

	struct wthp_compositor *compositor;
	struct wtimer *fiddle_timer;
};

/* a timerfd based timer */
struct wtimer {
	struct watch watch;
	void (*func)(struct wtimer *, void *);
	void *data;
};

/* Have some fun with a wthp_region, just to excercise the protocol. */
static void
fiddle_with_region(struct display *dpy)
{
	struct wthp_region *region;

	region = wthp_compositor_create_region(dpy->compositor);

	wthp_region_add(region, 2, 2, 10, 10);
	wthp_region_add(region, 12, 2, 100, 50);
	wthp_region_subtract(region, 50, 50, 5, 5);

	wthp_region_destroy(region);
}

/* The server advertises a global interface.
 * We can store the ad for later and/or bind to it immediately
 * if we want to.
 * We also need to keep track of the globals we bind to, so that
 * global_remove can be handled properly (not implemented).
 */
static void
registry_handle_global(struct wthp_registry *registry,
		       uint32_t name,
		       const char *interface,
		       uint32_t version)
{
	struct display *dpy = wth_object_get_user_data((struct wth_object *)registry);

	printf("got global %d: '%s' version '%d'\n",
	       name, interface, version);

	if (strcmp(interface, "wthp_compositor") == 0) {
		assert(!dpy->compositor);
		dpy->compositor = (struct wthp_compositor *)wthp_registry_bind(registry, name, interface, 1);
		/* has no events to handle */
	}
}

/* The server removed a global.
 * We should destroy everything we created through that global,
 * and destroy the objects we created by binding to it.
 * The identification happens by global's name, so we need to keep
 * track what names we bound.
 * (not implemented)
 */
static void
registry_handle_global_remove(struct wthp_registry *wthp_registry,
			      uint32_t name)
{
	printf("global %d removed\n", name);
}

static const struct wthp_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

static int
watch_ctl(struct watch *w, int op, uint32_t events)
{
	struct epoll_event ee;

	ee.events = events;
	ee.data.ptr = w;
	return epoll_ctl(w->display->epoll_fd, op, w->fd, &ee);
}

static void
wtimer_handle_data(struct watch *w, uint32_t events)
{
	struct wtimer *t = container_of(w, struct wtimer, watch);
	uint64_t expires;
	int len;

	assert(events & EPOLLIN);

	len = read(t->watch.fd, &expires, sizeof expires);
	if (!(len == -1 && errno == EAGAIN) && len != sizeof expires) {
		perror("timer read");
		exit(1);
	}

	return t->func(t, t->data);
}

static struct wtimer *
wtimer_create(struct display *dpy, int clock_id,
	      void (*func)(struct wtimer *, void *), void *data)
{
	struct wtimer *t;

	t = zalloc(sizeof *t);
	if (!t)
		return NULL;

	t->watch.display = dpy;
	t->watch.fd = timerfd_create(clock_id, TFD_CLOEXEC);
	t->watch.cb = wtimer_handle_data;
	t->func = func;
	t->data = data;

	if (t->watch.fd == -1) {
		perror("wtimer_create: timerfd_create");
		goto out_free;
	}

	if (watch_ctl(&t->watch, EPOLL_CTL_ADD, EPOLLIN) < 0) {
		perror("adding timer watch");
		goto out_close;
	}

	return t;

out_close:
	close(t->watch.fd);

out_free:
	free(t);
	return NULL;
}

static void
wtimer_destroy(struct wtimer *t)
{
	watch_ctl(&t->watch, EPOLL_CTL_DEL, 0);
	close(t->watch.fd);
	free(t);
}

static void
wtimer_arm_once(struct wtimer *t, int ms_delay)
{
	struct itimerspec its;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;
	its.it_value.tv_sec = ms_delay / 1000;
	its.it_value.tv_nsec = (ms_delay % 1000) * 1000 * 1000;
	if (timerfd_settime(t->watch.fd, 0, &its, NULL) < 0) {
		perror("arming timer");
		exit(1);
	}
}

static void
connection_handle_data(struct watch *w, uint32_t events)
{
	struct display *dpy = container_of(w, struct display, conn_watch);
	int ret;

	if (events & EPOLLERR) {
		fprintf(stderr, "Connection errored out.\n");
		dpy->running = false;

		return;
	}

	if (events & EPOLLOUT) {
		/* Flush out again. If the flush completes, stop
		 * polling for writable as everything has been written.
		 */
		ret = wth_connection_flush(dpy->connection);
		if (ret == 0)
			watch_ctl(&dpy->conn_watch, EPOLL_CTL_MOD, EPOLLIN);
		else if (ret < 0 && errno != EAGAIN)
			dpy->running = false;
	}

	if (events & EPOLLIN) {
		/* Do not ignore EPROTO */
		ret = wth_connection_read(dpy->connection);
		if (ret < 0) {
			perror("Connection read error\n");
			dpy->running = false;

			return;
		}
	}

	if (events & EPOLLHUP) {
		fprintf(stderr, "Connection hung up.\n");
		dpy->running = false;

		return;
	}
}

static void
sync_listener_handle_done(struct wthp_callback *cb, uint32_t arg)
{
	bool *flag = wth_object_get_user_data((struct wth_object *)cb);

	*flag = true;
}

static const struct wthp_callback_listener sync_listener = {
	sync_listener_handle_done
};

/* A blocking roundtrip to the Waltham server
 *
 * This function sends wth_display.sync request and blocks waiting
 * for the reply. While waiting, it processes only the Waltham
 * connection and will dispatch incoming Waltham messages.
 *
 * Return value is 0 if the roundtrip completed, and -1 otherwise
 * due to some error.
 */
static int
roundtrip(struct display *dpy)
{
	struct wthp_callback *cb;
	bool flag = false;
	int ret;
	struct pollfd pfd;

	assert(!dpy->running);

	cb = wth_display_sync(dpy->display);
	wthp_callback_set_listener(cb, &sync_listener, &flag);

	pfd.fd = wth_connection_get_fd(dpy->connection);
	pfd.events = POLLIN;

	while (!flag) {
		/* Do not ignore EPROTO */
		if (wth_connection_dispatch(dpy->connection) < 0)
			break;

		if (flag)
			break;

		ret = wth_connection_flush(dpy->connection);
		if (ret < 0 && errno == EAGAIN) {
			pfd.events = POLLIN | POLLOUT;
		} else if (ret < 0) {
			perror("Roundtrip connection flush failed");
			break;
		}

		do {
			ret = poll(&pfd, 1, -1);
		} while (ret == -1 && errno == EINTR);
		if (ret == -1) {
			perror("Roundtrip error with poll");
			break;
		}

		if (pfd.revents & (POLLERR | POLLNVAL)) {
			fprintf(stderr,
				"Roundtrip connection errored out.\n");
			break;
		}

		if (pfd.revents & POLLOUT) {
			ret = wth_connection_flush(dpy->connection);
			if (ret == 0)
				pfd.events = POLLIN;
			else if (ret < 0 && errno != EAGAIN) {
				perror("Roundtrip connection re-flush failed");
				break;
			}
		}

		if (pfd.revents & POLLIN) {
			ret = wth_connection_read(dpy->connection);
			if (ret < 0) {
				perror("Roundtrip connection read error");
				break;
			}
		}

		if (pfd.revents & POLLHUP) {
			/* If there was also unread data when HUP
			 * happened, assume POLLIN was also set and
			 * we just read everything already, so the
			 * only thing left is to dispatch it.
			 */
			wth_connection_dispatch(dpy->connection);
			break;
		}
	}

	wthp_callback_free(cb);

	return flag ? 0 : -1;
}

static void
mainloop(struct display *dpy)
{
	struct epoll_event ee[MAX_EPOLL_WATCHES];
	struct watch *w;
	int count;
	int i;
	int ret;

	dpy->running = true;

	while (1) {
		/* Dispatch queued events. */
		ret = wth_connection_dispatch(dpy->connection);
		if (ret < 0)
			dpy->running = false;

		if (!dpy->running)
			break;

		/* Run any application idle tasks at this point. */
		/* (nothing to run so far) */

		/* Flush out buffered requests. If the Waltham socket is
		 * full, poll it for writable too, and continue flushing then.
		 */
		ret = wth_connection_flush(dpy->connection);
		if (ret < 0 && errno == EAGAIN) {
			watch_ctl(&dpy->conn_watch, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
		} else if (ret < 0) {
			perror("Connection flush failed");
			break;
		}

		/* Wait for events or signals */
		count = epoll_wait(dpy->epoll_fd,
				   ee, ARRAY_LENGTH(ee), -1);
		if (count < 0 && errno != EINTR) {
			perror("Error with epoll_wait");
			break;
		}

		/* Waltham events only read in the callback, not dispatched,
		 * if the Waltham socket signalled readable. If it signalled
		 * writable, flush more. See connection_handle_data().
		 */
		for (i = 0; i < count; i++) {
			w = ee[i].data.ptr;
			w->cb(w, ee[i].events);
		}
	}
}

/* A one-off asynchronous open-coded roundtrip handler. */
static void
bling_done(struct wthp_callback *cb, uint32_t arg)
{
	struct display *dpy = wth_object_get_user_data((struct wth_object *)cb);

	fprintf(stderr, "...sync done.\n");

	wthp_callback_free(cb);
}

static const struct wthp_callback_listener bling_listener = {
	bling_done
};

static void
fiddle_timer_cb(struct wtimer *t, void *data)
{
	struct display *dpy = data;
	static int count;

	fiddle_with_region(dpy);

	count++;
	if (count > 4)
		dpy->running = false;
	else
		wtimer_arm_once(t, 1000);
}

static void
check_connection_errors(struct wth_connection *conn)
{
	int err;
	uint32_t id;
	uint32_t code;
	const char *intf;

	err = wth_connection_get_error(conn);
	if (err) {
		fprintf(stderr, "Connection error '%s'.\n", strerror(err));
		if (err == EPROTO) {
			code = wth_connection_get_protocol_error(conn,
				&intf, &id);
			fprintf(stderr,
				"  protocol error was %s@%#x: %d\n",
				intf, id, code);
		}
	} else {
		fprintf(stderr, "No errors recorded.\n");
	}
}

/* XXX: these three handlers should not be here */

static void
not_here_error(struct wth_display *d, struct wth_object *obj,
	       uint32_t code, const char *msg)
{
	struct wth_connection *conn;

	conn = wth_object_get_user_data((struct wth_object *)d);
	fprintf(stderr, "fatal protocol error %d: %s\n", code, msg);
	wth_connection_set_protocol_error(conn, 0xf000dead /* XXX obj->id */,
					  "unknown", code);
}

static void
not_here_delete_id(struct wth_display *d, uint32_t id)
{
	fprintf(stderr, "wth_display.delete_id(%d)\n", id);
}

static void
not_here_server_version(struct wth_display *d, uint32_t ver)
{
	fprintf(stderr, "wth_display.server_version(%d)\n", ver);
}

static const struct wth_display_listener not_here_listener = {
	not_here_error,
	not_here_delete_id,
	not_here_server_version
};

int
main(int arcg, char *argv[])
{
	struct display dpy = { 0 };
	int fd;
	int err;

	dpy.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (dpy.epoll_fd == -1) {
		perror("Error on epoll_create1");
		exit(1);
	}

	dpy.fiddle_timer = wtimer_create(&dpy, CLOCK_MONOTONIC,
					 fiddle_timer_cb, &dpy);
	if (!dpy.fiddle_timer) {
		perror("creating fiddle_timer");
		exit(1);
	}

	dpy.connection = wth_connect_to_server("localhost", "34400");
	if (!dpy.connection) {
		perror("Error connecting");
		exit(1);
	}

	dpy.conn_watch.display = &dpy;
	dpy.conn_watch.cb = connection_handle_data;
	dpy.conn_watch.fd = wth_connection_get_fd(dpy.connection);
	if (watch_ctl(&dpy.conn_watch, EPOLL_CTL_ADD, EPOLLIN) < 0) {
		perror("Error setting up connection polling");
		exit(1);
	}

	dpy.display = wth_connection_get_display(dpy.connection);
	 /* wth_display_set_listener() is already done by waltham, as
	  * all the events are just control messaging.
	  */

	/* ..except Waltham does not yet, so let's plug in something
	 * to print out stuff at least.
	 */
	wth_display_set_listener(dpy.display, &not_here_listener, dpy.connection);

	/* Create a registry so that we will get advertisements of the
	 * interfaces implemented by the server.
	 */
	dpy.registry = wth_display_get_registry(dpy.display);
	wthp_registry_set_listener(dpy.registry, &registry_listener, &dpy);

	/* Roundtrip ensures all globals' ads have been received. */
	if (roundtrip(&dpy) < 0) {
		fprintf(stderr, "Roundtrip failed.\n");
		exit(1);
	}

	if (!dpy.compositor) {
		fprintf(stderr, "Did not find wthp_compositor, quitting.\n");
		exit(1);
	}

	fiddle_with_region(&dpy);

#if 0
	/* Trigger a protocol error, since the test server does not
	 * implement this.
	 */
	wthp_compositor_create_surface(dpy.compositor);
#endif

	/* A one-off asynchronous roundtrip, just for fun. */
	fprintf(stderr, "sending wth_display.sync...\n");
	dpy.bling = wth_display_sync(dpy.display);
	wthp_callback_set_listener(dpy.bling, &bling_listener, &dpy);

	/* Create surfaces, draw initial content, etc. if you want. */
	wtimer_arm_once(dpy.fiddle_timer, 1000);

	mainloop(&dpy);

	/* destroy all things */
	wtimer_destroy(dpy.fiddle_timer);

	/* This sends the request, unlike wthp_registry_free() */
	wthp_registry_destroy(dpy.registry);

	/* Do the final roundtrip to guarantee the server has received
	 * all our tear-down requests.
	 */
	roundtrip(&dpy);

	/* Check and report any errors */
	check_connection_errors(dpy.connection);

	/* Disconnect, automatically destroys wth_display, free
	 * the connection.
	 */
	wth_connection_destroy(dpy.connection);

	close(dpy.epoll_fd);

	return 0;
}
