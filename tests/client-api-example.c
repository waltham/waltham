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
#include <sys/epoll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

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

	if (events & EPOLLHUP) {
		fprintf(stderr, "Connection hung up.\n");
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

	dpy.connection = wth_connect_to_server("localhost", "12300");
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

	dpy.registry = wth_display_get_registry(dpy.display);
	/* wthp_registry_set_listener() */

	/* Roundtrip ensures all global ads have been received. */
	wth_roundtrip(dpy.connection);

	/* Create surfaces, draw initial content, etc. if you want. */

	mainloop(&dpy);

	/* destroy all things */

	/* If no destructor protocol, this is autogenerated function to
	 * just free() the wth_object.
	 */
/* XXX:	wthp_registry_destroy(dpy.registry); */

	/* Disconnect, automatically destroys wth_display, free
	 * the connection.
	 */
	wth_connection_destroy(dpy.connection);

	close(dpy.epoll_fd);

	return 0;
}
