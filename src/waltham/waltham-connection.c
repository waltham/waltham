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
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>

#include "message.h"
#include "marshaller.h"
#include "waltham-client.h"
#include "waltham-connection.h"
#include "waltham-object.h"
#include "waltham-private.h"
#include "waltham-util.h"

// FIXME
struct wth_display {

};

struct wth_connection {
	int fd;
	enum wth_connection_side side;

	ClientReader *reader;
	int error;
	struct {
		uint32_t code;
		uint32_t id;
		const char *interface;
	} protocol_error;

	struct wth_display *display;
	struct wth_map map;
};


WTH_EXPORT struct wth_connection *
wth_connect_to_server(const char *host, const char *port)
{
	struct wth_connection *conn = NULL;
	int fd;

	fd = connect_to_host(host, port);

	if (fd >= 0)
		conn = wth_connection_from_fd(fd, WTH_CONNECTION_SIDE_CLIENT);

	return conn;
}

WTH_EXPORT struct wth_connection *
wth_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	struct wth_connection *conn = NULL;
	int fd;

	fd = accept(sockfd, addr, addrlen);

	if (fd >= 0)
		conn = wth_connection_from_fd(fd, WTH_CONNECTION_SIDE_SERVER);

	return conn;
}

WTH_EXPORT struct wth_connection *
wth_connection_from_fd(int fd, enum wth_connection_side side)
{
	struct wth_connection *conn;

	conn = calloc(1, sizeof *conn);

	if (conn == NULL)
		return NULL;

	conn->fd = fd;
	conn->side = side;

	conn->reader = new_reader();
	wth_map_init(&conn->map, side);

	/* id 0 should be NULL, id 1 the display */
	if (side == WTH_CONNECTION_SIDE_CLIENT) {
		wth_map_insert_at(&conn->map, 0, 0, NULL);
		conn->display = (struct wth_display *) wth_object_new(conn);
	} else {
		wth_map_reserve_new(&conn->map, 0);
		wth_map_insert_at(&conn->map, 0, 0, NULL);
		conn->display = (struct wth_display *) wth_object_new_with_id(conn, 1);
	}

	if (conn->display == NULL) {
		free(conn);
		conn = NULL;
	}

	return conn;
}

WTH_EXPORT int
wth_connection_get_fd(struct wth_connection *conn)
{
	return conn->fd;
}

WTH_EXPORT struct wth_display *
wth_connection_get_display(struct wth_connection *conn)
{
	return conn->display;
}

void
wth_connection_insert_new_object(struct wth_connection *conn,
		struct wth_object *obj)
{
	obj->id = wth_map_insert_new(&conn->map, 0, obj);

	wth_debug("%s: new object id: %d", __func__, obj->id);
}

void
wth_connection_insert_object_with_id(struct wth_connection *conn,
		struct wth_object *obj)
{
	wth_debug("%s: %d", __func__, obj->id);

	wth_map_reserve_new(&conn->map, obj->id);
	wth_map_insert_at(&conn->map, 0, obj->id, obj);
}

void
wth_connection_remove_object(struct wth_connection *conn,
		struct wth_object *obj)
{
	/* XXX use _remove when we are ready to reuse ids */
	//wth_map_remove(&conn->map, obj->id);
	wth_map_insert_at(&conn->map, 0, obj->id, NULL);
}

struct wth_object *
wth_connection_get_object(struct wth_connection *conn, uint32_t id)
{
	return wth_map_lookup(&conn->map, id);
}

WTH_EXPORT void
wth_connection_destroy(struct wth_connection *conn)
{
	close(conn->fd);

	wth_object_delete((struct wth_object *) conn->display);
	wth_map_release(&conn->map);
	free_reader(conn->reader);

	free(conn);
}

WTH_EXPORT int
wth_connection_flush(struct wth_connection *conn)
{
	/* FIXME currently we don't use the ringbuffer to send messages,
	 * they are just written to the fd in write_all() in
	 * src/marshaller/marshaller.h. */

	return 0;
}

WTH_EXPORT int
wth_connection_read(struct wth_connection *conn)
{
	/* If the connection is set to EPROTO, we still want to empty the kernel
	 * buffers. We just discard the messages without dispatching them. */
	if (conn->error && conn->error != EPROTO) {
		errno = conn->error;
		return -1;
	}

	if (!reader_pull_new_messages(conn->reader, conn->fd, true)) {
		/* Don't set the connection to error state in case of EAGAIN.
		 * We still return -1, but the user should handle errno == EAGAIN. */
		if (errno != EAGAIN)
			wth_connection_set_error(conn, errno);

		return -1;
	}

	/* Discard read messages without dispatching them if the connection
	 * was set to EPROTO. */
	if (conn->error == EPROTO)
		reader_flush(conn->reader);

	return 0;
}

WTH_EXPORT int
wth_connection_dispatch(struct wth_connection *conn)
{
	int i, complete;

	/* If there is an error, we still want to empty the ringbuffer.
	 * Messages won't be dispatched though, so this should be safe. */

	for (i = 0 ; i < conn->reader->m_complete; i++) {
		msg_t msg;

		reader_map_message(conn->reader, i, &msg);
		wth_debug("Message received on conn %p: (%d) %d bytes",
			  conn, msg.hdr->opcode, msg.hdr->sz);

		/* Don't dispatch more messages after the connection is set
		 * to EPROTO. */
		if (conn->error != EPROTO)
			msg_dispatch(conn, &msg);

		reader_unmap_message(conn->reader, i, &msg);
	}

	complete = conn->reader->m_complete;

	/* Remove processed messages */
	reader_flush(conn->reader);

	/* The connection has been set to error in this call. */
	if (conn->error) {
		errno = conn->error;
		return -1;
	}

	return complete;
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

WTH_EXPORT int
wth_connection_roundtrip(struct wth_connection *conn)
{
	struct wthp_callback *cb;
	bool flag = false;
	int ret;
	struct pollfd pfd;

	ASSERT_CLIENT_SIDE(conn);

	cb = wth_display_sync(conn->display);
	wthp_callback_set_listener(cb, &sync_listener, &flag);

	pfd.fd = conn->fd;
	pfd.events = POLLIN;

	while (!flag) {
		/* Do not ignore EPROTO */
		if (wth_connection_dispatch(conn) < 0)
			break;

		if (flag)
			break;

		ret = wth_connection_flush(conn);
		if (ret < 0 && errno == EAGAIN) {
			pfd.events = POLLIN | POLLOUT;
		} else if (ret < 0) {
			wth_debug("Roundtrip connection flush failed: %s",
				  strerror(errno));
			break;
		}

		do {
			ret = poll(&pfd, 1, -1);
		} while (ret == -1 && errno == EINTR);
		if (ret == -1) {
			wth_debug("Roundtrip error with poll: %s",
				  strerror(errno));
			break;
		}

		if (pfd.revents & (POLLERR | POLLNVAL)) {
			wth_debug("Roundtrip connection errored out.");
			break;
		}

		if (pfd.revents & POLLOUT) {
			ret = wth_connection_flush(conn);
			if (ret == 0) {
				pfd.events = POLLIN;
			} else if (ret < 0 && errno != EAGAIN) {
				wth_debug("Roundtrip connection re-flush failed: %s",
					strerror(errno));
				break;
			}
		}

		if (pfd.revents & POLLIN) {
			ret = wth_connection_read(conn);
			if (ret < 0) {
				wth_debug("Roundtrip connection read error: %s",
					  strerror(errno));
				break;
			}
		}

		if (pfd.revents & POLLHUP) {
			/* If there was also unread data when HUP
			 * happened, assume POLLIN was also set and
			 * we just read everything already, so the
			 * only thing left is to dispatch it.
			 */
			ret = wth_connection_dispatch(conn);
			if (ret < 0) {
				wth_debug("Roundtrip dispatch error: %s",
					  strerror(errno));
				break;
			}
			break;
		}
	}

	wthp_callback_free(cb);

	return flag ? 0 : -1;
}

WTH_EXPORT void
wth_connection_set_error(struct wth_connection *conn, int err)
{
	if (!conn->error || err == EPROTO)
		conn->error = err;
}

WTH_EXPORT void
wth_connection_set_protocol_error(struct wth_connection *conn,
				  uint32_t object_id,
				  const char *interface,
				  uint32_t error_code)
{
	wth_connection_set_error(conn, EPROTO);
	conn->protocol_error.interface = interface;
	conn->protocol_error.id = object_id;
	conn->protocol_error.code = error_code;
}

WTH_EXPORT int
wth_connection_get_error(struct wth_connection *conn)
{
	return conn->error;
}

WTH_EXPORT uint32_t
wth_connection_get_protocol_error(struct wth_connection *conn,
				  const char **interface,
				  uint32_t *object_id)
{
	*interface = conn->protocol_error.interface;
	*object_id = conn->protocol_error.id;

	return conn->protocol_error.code;
}

static const char *
side_to_str(enum wth_connection_side side)
{
	switch (side) {
	case WTH_CONNECTION_SIDE_CLIENT:
		return "client";
	case WTH_CONNECTION_SIDE_SERVER:
		return "server";
	default:
		return "(invalid)";
	}
}

void
wth_connection_assert_side(struct wth_connection *conn,
			   const char *func,
			   enum wth_connection_side expected)
{
	const char *conn_side;
	const char *expected_side;

	if (conn->side == expected)
		return;

	conn_side = side_to_str(conn->side);
	expected_side = side_to_str(expected);

	wth_abort("function %s is %s-only but it was called on a %s side object",
		  func, expected_side, conn_side);
}
