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
#include <netinet/in.h>
#include <string.h>

#include <waltham-object.h>
#include <waltham-server.h>
#include <waltham-connection.h>
//#include <waltham-server-protocol.h>

#include "zalloc.h"
#include "w-util.h"

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])
#endif

#ifndef container_of
#define container_of(ptr, type, member) ({                              \
	const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
	(type *)( (char *)__mptr - offsetof(type,member) );})
#endif

#define MAX_EPOLL_WATCHES 2

struct server;

struct watch {
	struct server *server;
	int fd;
	void (*cb)(struct watch *w, uint32_t events);
};

struct client {
	struct wl_list link; /* struct server::client_list */
	struct server *server;
	struct wth_connection *connection;
	struct watch conn_watch;

	/* TODO: a list of client objects, for clean-up */
};

struct server {
	int listen_fd;
	struct watch listen_watch;

	bool running;
	int epoll_fd;

	struct wl_list client_list;
};

static int
watch_ctl(struct watch *w, int op, uint32_t events)
{
	struct epoll_event ee;

	ee.events = events;
	ee.data.ptr = w;
	return epoll_ctl(w->server->epoll_fd, op, w->fd, &ee);
}

static void
client_destroy(struct client *c)
{
	fprintf(stderr, "Client %p disconnected.\n", c);

	wth_connection_flush(c->connection);
	wl_list_remove(&c->link);

	watch_ctl(&c->conn_watch, EPOLL_CTL_DEL, 0);
	wth_connection_destroy(c->connection);

	free(c);
}

static void
object_post_error(struct wth_object *obj,
		  uint32_t code,
		  const char *fmt, ...)
{
	struct client *c;
	struct wth_connection *conn;
	struct wth_display *disp;
	char str[256];
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(str, sizeof str, fmt, ap);
	va_end(ap);

	conn = obj->connection; /* XXX: use a getter? */
	disp = wth_connection_get_display(conn);
	wth_display_send_error(disp, obj, code, str);

	c = wth_object_get_user_data((struct wth_object *)disp);
	fprintf(stderr, "client %p: sending fatal error '%s'\n", c, str);

	/* XXX: AARGH!
	 * However, getting the error message actually transmitted
	 * while we want to close the connection without blocking
	 * on it is a pretty nasty problem.
	 */
	sleep(1);

	/* XXX: if we are in the middle of dispatching, which is very
	 * likely, we should mark the wth_connection as errored.
	 * Destroying it here means we probably crash.
	 */
	client_destroy(c);

	/* XXX: http://stackoverflow.com/questions/3757289/tcp-option-so-linger-zero-when-its-required
	 * So we should design this so that the client will be closing
	 * the connection on a protocol error. Fair enough, so we should
	 * just mark the connection as failed but not close it.
	 * While it is failed, we should just ignore all incoming
	 * messages and wait for it to close. We might also need a
	 * timer to trigger a forced close if the client never closes it.
	 * Would the TCP stack have anything to do that automatically?
	 */
}

static void
region_destroy(struct wthp_region *wthp_region)
{
	fprintf(stderr, "region %p destroy\n", wthp_region);
	wth_object_delete((struct wth_object *)wthp_region);
}

static void
region_add(struct wthp_region *wthp_region,
	   int32_t x, int32_t y, int32_t width, int32_t height)
{
	fprintf(stderr, "region %p add(%d, %d, %d, %d)\n",
		wthp_region, x, y, width, height);
}

static void
region_subtract(struct wthp_region *wthp_region,
		int32_t x, int32_t y, int32_t width, int32_t height)
{
	fprintf(stderr, "region %p subtract(%d, %d, %d, %d)\n",
		wthp_region, x, y, width, height);
}

static const struct wthp_region_interface region_implementation = {
	region_destroy,
	region_add,
	region_subtract
};

static void
compositor_create_surface(struct wthp_compositor *compositor,
			  struct wthp_surface *id)
{
	object_post_error((struct wth_object *)compositor, 0, "unimplemented: %s", __func__);
}

static void
compositor_create_region(struct wthp_compositor *compositor,
			 struct wthp_region *id)
{
	struct client *c = wth_object_get_user_data((struct wth_object *)compositor);

	fprintf(stderr, "client %p create region %p\n", c, id);
	wthp_region_set_interface(id, &region_implementation, NULL);
}

static const struct wthp_compositor_interface compositor_implementation = {
	compositor_create_surface,
	compositor_create_region
	/* XXX: protocol is missing destructor */
};

static void
registry_destroy(struct wthp_registry *registry)
{
	struct client *c = wth_object_get_user_data((struct wth_object *)registry);

	fprintf(stderr, "Client %p wthp_registry.destroy\n", c);

	/* XXX: wthp_registry_free(registry); */
	wth_object_delete((struct wth_object *)registry);
}

static void
registry_bind(struct wthp_registry *registry,
	     uint32_t name,
	     struct wth_object *id)
{
	struct client *c = wth_object_get_user_data((struct wth_object *)registry);

	/* XXX: we could use a database of globals instead of hardcoding them */

	switch (name) {
	case 1:
		wthp_compositor_set_interface(
			(struct wthp_compositor *)id,
			&compositor_implementation, c);
		fprintf(stderr, "client %p bound %s version %d\n",
			c, "(wthp_compositor?)", -1);
		break;
	default:
		object_post_error((struct wth_object *)registry, 0, "%s: unknown name %u", __func__, name);
	}
}

const struct wthp_registry_interface registry_implementation = {
	registry_destroy,
	registry_bind
};

static void
display_client_version(struct wth_display * wth_display, uint32_t client_version)
{
	object_post_error((struct wth_object *)wth_display, 0, "unimplemented: %s", __func__);
}

static void
display_sync(struct wth_display * wth_display, struct wthp_callback * callback)
{
	struct client *c = wth_object_get_user_data((struct wth_object *)wth_display);

	fprintf(stderr, "Client %p requested wth_display.sync\n", c);
	wthp_callback_send_done(callback, 0);

	/* XXX: wthp_callback_XXX_destroy(callback); */
}

static void
display_get_registry(struct wth_display *wth_display,
		     struct wthp_registry *registry)
{
	struct client *c = wth_object_get_user_data((struct wth_object *)wth_display);

	wthp_registry_set_interface(registry, &registry_implementation, c);

	/* XXX: advertise our globals */
	wthp_registry_send_global(registry, 1, "wthp_compositor", 4);
}

static const struct wth_display_interface display_implementation = {
	display_client_version,
	display_sync,
	display_get_registry
};

static void
connection_handle_data(struct watch *w, uint32_t events)
{
	struct client *c = container_of(w, struct client, conn_watch);
	int ret;

	if (events & EPOLLERR) {
		fprintf(stderr, "Client %p errored out.\n", c);
		client_destroy(c);

		return;
	}

	if (events & EPOLLHUP) {
		fprintf(stderr, "Client %p hung up.\n", c);
		client_destroy(c);

		return;
	}

	if (events & EPOLLOUT) {
		ret = wth_connection_flush(c->connection);
		if (ret == 0)
			watch_ctl(&c->conn_watch, EPOLL_CTL_MOD, EPOLLIN);
		else if (ret < 0 && errno != EAGAIN){
			fprintf(stderr, "Client %p flush error.\n", c);
			client_destroy(c);

			return;
		}
	}

	if (events & EPOLLIN) {
		ret = wth_connection_read(c->connection);
		if (ret < 0) {
			fprintf(stderr, "Client %p read error.\n", c);
			client_destroy(c);

			return;
		}

		ret = wth_connection_dispatch(c->connection);
		if (ret < 0) {
			fprintf(stderr, "Client %p dispatch error.\n", c);
			client_destroy(c);

			return;
		}
	}
}

static struct client *
client_create(struct server *srv, struct wth_connection *conn)
{
	struct client *c;
	struct wth_display *disp;

	c = zalloc(sizeof *c);
	if (!c)
		return NULL;

	c->server = srv;
	c->connection = conn;

	c->conn_watch.server = srv;
	c->conn_watch.fd = wth_connection_get_fd(conn);
	c->conn_watch.cb = connection_handle_data;
	if (watch_ctl(&c->conn_watch, EPOLL_CTL_ADD, EPOLLIN) < 0) {
		free(c);
		return NULL;
	}

	fprintf(stderr, "Client %p connected.\n", c);

	wl_list_insert(&srv->client_list, &c->link);

	/* XXX: this should be inside Waltham */
	disp = wth_connection_get_display(c->connection);
	wth_display_set_interface(disp, &display_implementation, c);

	return c;
}

static void
server_flush_clients(struct server *srv)
{
	struct client *c, *tmp;
	int ret;

	wl_list_for_each_safe(c, tmp, &srv->client_list, link) {
		/* Flush out buffered requests. If the Waltham socket is
		 * full, poll it for writable too.
		 */
		ret = wth_connection_flush(c->connection);
		if (ret < 0 && errno == EAGAIN) {
			watch_ctl(&c->conn_watch, EPOLL_CTL_MOD, EPOLLIN | EPOLLOUT);
		} else if (ret < 0) {
			perror("Connection flush failed");
			client_destroy(c);
		}
	}
}

static void
server_accept_client(struct server *srv)
{
	struct client *client;
	struct wth_connection *conn;
	struct sockaddr_in addr;
	socklen_t len;

	len = sizeof addr;
	conn = wth_accept(srv->listen_fd, (struct sockaddr *)&addr, &len);
	if (!conn) {
		fprintf(stderr, "Failed to accept a connection.\n");

		return;
	}

	client = client_create(srv, conn);
	if (!client) {
		fprintf(stderr, "Failed client_create().\n");

		return;
	}
}

static void
listen_socket_handle_data(struct watch *w, uint32_t events)
{
	struct server *srv = container_of(w, struct server, listen_watch);
	int ret;

	if (events & EPOLLERR) {
		fprintf(stderr, "Listening socket errored out.\n");
		srv->running = false;

		return;
	}

	if (events & EPOLLHUP) {
		fprintf(stderr, "Listening socket hung up.\n");
		srv->running = false;

		return;
	}

	if (events & EPOLLIN)
		server_accept_client(srv);
}

static void
mainloop(struct server *srv)
{
	struct epoll_event ee[MAX_EPOLL_WATCHES];
	struct watch *w;
	int count;
	int i;
	int ret;

	srv->running = true;

	while (srv->running) {
		/* Run any idle tasks at this point. */

		server_flush_clients(srv);

		/* Wait for events or signals */
		count = epoll_wait(srv->epoll_fd,
				   ee, ARRAY_LENGTH(ee), -1);
		if (count < 0 && errno != EINTR) {
			perror("Error with epoll_wait");
			break;
		}

		/* Handle all fds, both the listening socket
		 * (see listen_handle_data()) and clients
		 * (see connection_handle_data()).
		 */
		for (i = 0; i < count; i++) {
			w = ee[i].data.ptr;
			w->cb(w, ee[i].events);
		}
	}
}

static int
server_listen(uint16_t tcp_port)
{
	int fd;
	int reuse = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(tcp_port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

	if (bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
		fprintf(stderr, "Failed to bind to port %d", tcp_port);
		close(fd);
		return -1;
	}

	if (listen(fd, 1024) < 0) {
		fprintf(stderr, "Failed to listen to port %d", tcp_port);
		close (fd);
		return -1;
	}

	return fd;
}

int
main(int arcg, char *argv[])
{
	struct server srv = { 0 };
	int fd;

	wl_list_init(&srv.client_list);

	srv.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
	if (srv.epoll_fd == -1) {
		perror("Error on epoll_create1");
		exit(1);
	}

	srv.listen_fd = server_listen(34400);
	if (srv.listen_fd < 0) {
		perror("Error setting up listening socket");
		exit(1);
	}

	srv.listen_watch.server = &srv;
	srv.listen_watch.cb = listen_socket_handle_data;
	srv.listen_watch.fd = srv.listen_fd;
	if (watch_ctl(&srv.listen_watch, EPOLL_CTL_ADD, EPOLLIN) < 0) {
		perror("Error setting up listen polling");
		exit(1);
	}

	mainloop(&srv);

	/* destroy all things */

	close(srv.listen_fd);
	close(srv.epoll_fd);

	return 0;
}
