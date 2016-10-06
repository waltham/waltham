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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <poll.h>
#include <sys/epoll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <assert.h>

#include <waltham-object.h>
//#include <waltham-client.h> /* XXX: misses include guards */
#include <waltham-connection.h>
//#include <waltham-client-protocol.h>


#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                              \
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define MAX_EPOLL_WATCHES 2

struct display;

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
		ret = wth_connection_flush(dpy->connection);
		if (ret == 0)
			watch_ctl(&dpy->conn_watch, EPOLL_CTL_MOD, EPOLLIN);
		else if (ret < 0 && errno != EAGAIN)
			dpy->running = false;
	}

	if (events & EPOLLIN) {
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

	/* XXX: wthp_callback_free(cb); */
	wth_object_delete((struct wth_object *)cb);

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
		wth_connection_dispatch(dpy->connection);

		if (!dpy->running)
			break;

		/* Run any application idle tasks at this point. */

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

/* The server advertises a global interface.
 * We can store the ad for later and/or bind to it immediately
 * if we want to.
 * We also need to keep track of the globals we bind to, so that
 * global_remove can be handled properly.
 */
static void
registry_handle_global(struct wthp_registry *wthp_registry,
		       uint32_t name,
		       const char *interface,
		       uint32_t version)
{
	printf("got global %d: '%s' version '%d'\n",
	       name, interface, version);
}

/* The server removed a global.
 * We should destroy everything we created through that global,
 * and destroy the objects we created by binding to it.
 * The identification happens by global's name, so we need to keep
 * track what names we bound.
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

static void
bling_done(struct wthp_callback *cb, uint32_t arg)
{
	struct display *dpy = wth_object_get_user_data((struct wth_object *)cb);

	fprintf(stderr, "...sync done.\n");
	dpy->running = false;

	/* This needs to be safe! */
	/* XXX: wthp_callback_free(cb); */
	wth_object_delete((struct wth_object *)cb);
}

static const struct wthp_callback_listener bling_listener = {
	bling_done
};

int
main(int arcg, char *argv[])
{
	struct display dpy = { 0 };
	int fd;

	dpy.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (dpy.epoll_fd == -1) {
		perror("Error on epoll_create1");
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

	fprintf(stderr, "sending wth_display.sync...\n");
	dpy.bling = wth_display_sync(dpy.display);
	wthp_callback_set_listener(dpy.bling, &bling_listener, &dpy);

	/* Create surfaces, draw initial content, etc. if you want. */

	mainloop(&dpy);

	/* destroy all things */

	/* If no destructor protocol, there would be an auto-generated
	 * wthp_registry_free() to just free() the wth_object.
	 */
	wthp_registry_destroy(dpy.registry);

	/* Disconnect, automatically destroys wth_display, free
	 * the connection.
	 */
	wth_connection_destroy(dpy.connection);

	close(dpy.epoll_fd);

	return 0;
}
