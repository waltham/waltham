/*
 * Copyright © 2013-2014 Collabora, Ltd.
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

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <gio/gunixsocketaddress.h>
#include <gio/gfiledescriptorbased.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <waltham-client.h>

#include "util/log.h"
#include "util/message.h"
#include "util/profiling.h"
#include "waltham-message.h"

#define DEBUGGING 1

static gchar *server_address = "localhost";
static gchar *server_port = "1234";

static GOptionEntry entries[] =
{
  { "connect", 'c', 0, G_OPTION_ARG_STRING, &server_address,
    "server's address to connect to [default: localhost]", NULL },
  { "port", 'p', 0, G_OPTION_ARG_STRING, &server_port,
    "server's port to connect to [default: 1234]", NULL },
  { NULL }
};

int server_fd = -1;

/* Scratch pre-allocated reply message */
/* FIXME: we shouldn't need this as the protocol is fully async and
 * we never send replies back. */
msg_t scratch_reply;

#define MSG_ID_TO_POINTER(cid, id) GUINT_TO_POINTER ((cid << 16) + id)
#define MSG_TO_POINTER(msg) MSG_ID_TO_POINTER(msg->header.client_id, msg->header.id)

static void
server_parse_msg (msg_t *msg)
{
  msg_t *reply = &scratch_reply;
  data_t data_reply = {0,};
  gboolean ret;

  g_debug ("Message received: client %d (%d, %d) %d bytes",
           msg->hdr->client_id, msg->hdr->api, msg->hdr->opcode, msg->hdr->sz);

  memcpy (reply->hdr, msg->hdr, sizeof (hdr_t));
  reply->hdr->opcode = OPCODE_REPLY;
  memset (reply->chunks, 0, sizeof(reply->chunks));

  ret = grayley_dispatch (msg, reply->hdr, reply->body, &data_reply);
}

static gboolean
server_readable (EPollLoop *loop, int fd, GIOCondition cond, gpointer data)
{
  gboolean ret;
  int i;
  ClientReader *reader = (ClientReader  *)data;

  ret = reader_pull_new_messages (reader, server_fd, TRUE);
  if (!ret)
    exit(0);

  for (i = 0 ; i < reader->m_complete; i++)
    {
      msg_t msg;

      reader_map_message (reader, i, &msg);
      server_parse_msg (&msg);
      reader_unmap_message (reader, i, &msg);
    }

  /* flush */
  reader_flush (reader);

  return TRUE;
}


static void
run_main_loop (EPollLoop *loop) {

  for (;;)
    {
      epoll_loop_iterate (loop);
    }
}

static gboolean
connect_to_server (EPollLoop *loop)
{
  server_fd = connect_to_host (server_address, server_port);
  ClientReader *reader;

  if (server_fd == -1)
    {
      g_critical ("Could not connect to server at %s:%s",
                  server_address, server_port);
      return FALSE;
    }

  gael_set_fd (server_fd);
  gael_connect_client ();

  reader = new_reader (0);
  epoll_loop_add_fd (loop, server_fd, G_IO_IN, server_readable, reader);
  return TRUE;
}

int
main(int argc, char **argv)
{
  GError *error = NULL;
  GOptionContext *context;
  int ret = -1;
  EPollLoop loop;

#if !GLIB_CHECK_VERSION(2,36,0)
  g_type_init ();
#endif

  memset (&scratch_reply, 0, sizeof (msg_t));
  scratch_reply.hdr = g_malloc0 (sizeof (hdr_t));
  scratch_reply.body = g_malloc0 (MESSAGE_MAX_SIZE);

  log_set_up ("/var/log/collabora_id_daemon.log");

  context = g_option_context_new ("- test client");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_error ("option parsing failed: %s", error->message);
      g_clear_error (&error);
      return 1;
    }

  epoll_loop_init (&loop);

  /* First, connect to server */
  ret = connect_to_server (&loop);
  if (!ret)
    goto out;

  struct wth_display { void *display; int id } display;
  struct wthp_registry *registry;
  struct wthp_compositor *compositor;
  struct wthp_seat *seat;
  struct wthp_touch *touch;
  struct wthp_pointer *pointer;
  struct wthp_surface *surface;
  struct wthp_blob_factory *blob_factory;
  struct wthp_buffer *buffer;
  unsigned char data[10] = { 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19 };

  //display = wth_display_connect (NULL);

  registry = wth_display_get_registry (&display);

  seat = wthp_registry_bind (registry, 5);

  touch = wthp_seat_get_touch (seat);
  wthp_touch_release (touch);

  compositor = wthp_registry_bind (registry, 8);
  pointer = wthp_seat_get_pointer (seat);
  surface = wthp_compositor_create_surface (compositor);
  wthp_pointer_set_cursor (pointer, 1, surface, 3, 4);

  blob_factory = wthp_registry_bind (registry, 8);
  buffer = wthp_blob_factory_create_buffer (blob_factory, sizeof (data), data,
      /* width */ 30, /* height */ 40, /* stride */ 30, /* format */ 10);

  run_main_loop (&loop);

out:
  return ret;
}
