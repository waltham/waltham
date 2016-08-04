/*
 * Copyright © 2013-2015 Collabora, Ltd.
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

#include "config.h"

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <poll.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include <glib.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>

#include <waltham-message.h>
#include <grayley.h>
#include <grayley_platform.h>
#include <log.h>
#include <message.h>
#include <profiling.h>
#include <util.h>
#include <zalloc.h>

static gint server_port = 1234;

/* Comment/uncomment to enable or disable server profiling */
//#define PROFILE 1
/* Comment/uncomment to enable or disable server debug messages */
#define DEBUGGING 1

static int client_fd = -1;
static ClientReader *client_reader = NULL;

/* hash table: client_id -> last request id */
GHashTable *last_request_ids;

/* Scratch pre-allocated reply message */
msg_t scratch_reply;

struct server_msg_handler {
	GThread *thread;
	EPollLoop loop;
	ClientReader *reader;
	int grayley_fd;
};

#define MSG_ID_TO_POINTER(cid, id) GUINT_TO_POINTER ((cid << 16) + id)
#define MSG_TO_POINTER(msg) MSG_ID_TO_POINTER(msg->hdr->client_id, msg->hdr->id)

static void
error_cleanup (void)
{
	if (client_fd != -1) {
		g_debug ("Closing connection to client");
	}

	//grayley_notify_disconnect ();

	client_fd = -1;
	free_reader (client_reader);
	client_reader = NULL;
}

static void
exec_msg (msg_t *msg)
{
	unsigned short api = msg->hdr->api;
	unsigned short opcode = msg->hdr->opcode;
	gboolean ret;
	msg_t *reply = &scratch_reply;
	data_t data_reply = {0,};

	/* Disconnecting client - Dispatch */
	if (api == API_CONTROL && opcode == API_CONTROL_DISCONNECT_CLIENT) {
		ret = grayley_dispatch (msg, NULL, NULL, NULL);
		return;
	}

#ifdef DEBUGGING
	g_debug ("Message from client: client %d (%d, %d) %d bytes",
		msg->hdr->client_id, msg->hdr->api, msg->hdr->opcode, msg->hdr->sz);
#endif

	START_PROFILER ();
	memcpy (reply->hdr, msg->hdr, sizeof (hdr_t));
	reply->hdr->opcode = OPCODE_REPLY;
	memset (reply->chunks, 0, sizeof(reply->chunks));

	ret = grayley_dispatch (msg, reply->hdr, reply->body, &data_reply);

	RESET_PROFILER ("dispatch message");

	/* No need to reply */
	if (ret) {
		DESTROY_PROFILER ();
		return;
	}

#ifdef DEBUGGING
	g_debug ("Send reply to client: client %d (%d, %d) %d bytes %d data",
	         reply->hdr->client_id, msg->hdr->api, msg->hdr->opcode,
	         reply->hdr->sz, data_reply.sz);
#endif

	if (data_reply.sz > 0) {
		reply->chunks[0].data = (void *)&data_reply.sz;
		reply->chunks[0].size = sizeof (data_reply.sz);
		reply->chunks[1].data = data_reply.data;
		reply->chunks[1].size = data_reply.sz;
	}

	/* Send the reply to client */
	forward_raw_msg (client_fd, reply);

	END_PROFILER ("forward reply");
	return;
}

#define GREATER(req_id, wait_req_id) (req_id >= wait_req_id)
#define OVERFLOWED(req_id, wait_req_id) ((wait_req_id - req_id) >= USHRT_MAX/2)
#define SYNCED(A, B) (GREATER(A, B) || OVERFLOWED(A, B))

static void
exec_or_queue_msg (msg_t *msg)
{
	uint16_t client_id = msg->hdr->client_id;
	uint16_t request_id = msg->hdr->id;
	uint16_t wait_request_id;
	gboolean found;
	gboolean disconnected = (msg->hdr->api == API_CONTROL &&
	                         msg->hdr->opcode == API_CONTROL_DISCONNECT_CLIENT);

#ifdef DEBUGGING
	g_debug ("Execing immediately:");
#endif
	exec_msg (msg);

	// Insert (or remove) last received request id in hash table
	if (disconnected) {
		g_hash_table_remove (last_request_ids, GUINT_TO_POINTER (client_id));
	} else {
		g_hash_table_insert (last_request_ids, GUINT_TO_POINTER (client_id),
		                     GUINT_TO_POINTER (request_id));
	}
}

static gboolean
client_readable_cb (EPollLoop *loop, int fd, GIOCondition cond, gpointer data)
{
	ClientReader *reader = data;
	msg_t msg;
	int i;

	START_PROFILER ();
	if (!reader_pull_new_messages (reader, fd, TRUE)) {
		error_cleanup ();
		return FALSE;
	}
	END_PROFILER ("read messages");

	for (i = 0 ; i < reader->m_complete; i++) {
		reader_map_message (reader, i, &msg);
		exec_or_queue_msg (&msg);
		reader_unmap_message (reader, i, &msg);
	}

	reader_flush (reader);

	return TRUE;
}

static inline gboolean
forward_range_to_client (ClientReader *reader, int s, int e)
{
	return reader_forward_message_range (reader, client_fd, s, e);
}

static gboolean
new_client_connection_cb (EPollLoop *loop,
                          int fd,
                          GIOCondition cond,
                          gpointer data)
{
	int new_fd;
	int flag = 1;

	new_fd = accept (fd, NULL, NULL);
	if (new_fd < 0) {
		g_critical ("Failed to accept client connection");
		return FALSE;
	}

	if (client_fd != -1) {
		/* server only accepts one connection: Unref this connection */
		g_warning ("New connection but client is already connected. "
		           "Discarding the new connection.");
		close (new_fd);
		return FALSE;
	}

	g_debug ("New connection: %d", new_fd);
	client_fd = new_fd;
	gael_set_fd (client_fd);
	client_reader = new_reader (0);
	setsockopt(new_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));
	epoll_loop_add_fd (loop, new_fd, G_IO_IN, client_readable_cb, client_reader);

	return TRUE;
}

static gboolean
initialize_service (EPollLoop *loop)
{
	int fd;
	int reuse = 1;
	struct sockaddr_in addr;

	fd = socket (AF_INET, SOCK_STREAM, 0);

	memset (&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(server_port);
	addr.sin_addr.s_addr = htonl (INADDR_ANY);

	setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof (reuse));

	if (bind (fd, (struct sockaddr *)&addr, sizeof (struct sockaddr_in)) < 0) {
		g_critical ("Failed to bind to port %d", server_port);
		close (fd);
		return FALSE;
	}

	if (listen (fd, 1024) < 0) {
		g_critical ("Failed to listen to port %d", server_port);
		close (fd);
		return FALSE;
	}

	epoll_loop_add_fd (loop, fd, G_IO_IN, new_client_connection_cb, NULL);

	g_debug("server listening on TCP port %d", server_port);

	return TRUE;
}

static gpointer
server_msg_handler_run_loop_thread (gpointer data)
{
	struct server_msg_handler *handler = (struct server_msg_handler *) data;
	struct wl_event_loop *wl_loop;
	struct grayley_loop_t *loop;
	char s[32];
	int ret;

	g_debug ("server message handler loop starting");

#if !GLIB_CHECK_VERSION(2,36,0)
	g_type_init ();
#endif

	memset (&scratch_reply, 0, sizeof (msg_t));
	scratch_reply.hdr = g_malloc0 (sizeof (hdr_t));
	scratch_reply.body = g_malloc0 (MESSAGE_MAX_SIZE);

	loop = grayley_loop_new_from_epoll (&handler->loop);
	if (!loop) {
		g_error ("Could not create Grayley event loop");
		return 0;
	}

	snprintf(s, sizeof(s), "%d", handler->grayley_fd);
	setenv("WAYLAND_SOCKET", s, 1);
	g_debug("Starting Grayley thread with display fd %d",
	        handler->grayley_fd);
	grayley_init (loop);

	last_request_ids = g_hash_table_new (g_direct_hash, g_direct_equal);

	/* Initialize message service */
	ret = initialize_service (&handler->loop);
	if (!ret) {
		g_error ("server service initialisation failed!");
		return 0;
	}

	g_debug ("server message handler loop running");

	for (;;) epoll_loop_iterate (&handler->loop);

	return NULL;
}

void
server_msg_handler_run (struct server_msg_handler *handler)
{
	handler->thread = g_thread_new ("msg handler main loop",
				        server_msg_handler_run_loop_thread, handler);
}

struct server_msg_handler*
server_msg_handler_create (struct wl_display *wl_display)
{
	struct server_msg_handler *handler;
	int sv[2];

	g_debug ("Initializing server message handler");

	handler = zalloc (sizeof *handler);
	//handler->wl_dpy = wl_display;
	epoll_loop_init (&handler->loop);

	log_set_up ("/var/log/collabora_ad_daemon.log");

	return handler;
}

int
main (int argc, char **argv)
{
	struct server_msg_handler *handler;

	handler = server_msg_handler_create (NULL);
	server_msg_handler_run_loop_thread (handler);

	return 0;
}
