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

//#include <waltham-object.h>
//#include <waltham-client.h>
//#include <waltham-client-protocol.h>

#include "waltham-connection.h"

#include "util/message.h"
#include "marshaller.h"

// FIXME
struct wth_display {

};

struct wth_connection {
  int fd;
  enum wth_connection_side side;

  ClientReader *reader;

  struct wth_display *display;

  GHashTable *hash;

  int next_message_id;
  int next_object_id;
};


struct wth_connection *
wth_connect_to_server(const char *host, const char *port)
{
  struct wth_connection *conn = NULL;
  int fd;

  fd = connect_to_host(host, port);

  if (fd >= 0)
    conn = wth_connection_from_fd(fd, WTH_CONNECTION_SIDE_CLIENT);

  return conn;
}

struct wth_connection *
wth_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  struct wth_connection *conn = NULL;
  int fd;

  fd = accept(sockfd, addr, addrlen);

  if (fd >= 0)
    conn = wth_connection_from_fd(fd, WTH_CONNECTION_SIDE_SERVER);

  return conn;
}

struct wth_connection *
wth_connection_from_fd(int fd, enum wth_connection_side side)
{
  struct wth_connection *conn;

  conn = calloc(1, sizeof *conn);

  if (conn == NULL)
    return NULL;

  conn->fd = fd;
  conn->side = side;

  conn->reader = new_reader ();
  conn->hash = g_hash_table_new (g_direct_hash, g_direct_equal);

  /* Since this is a new connection, the display will always have id 1. */
  conn->display = (struct wth_display *) wth_object_new (conn);

  if (conn->display == NULL)
    {
      free(conn);
      conn = NULL;
    }

  return conn;
}

int
wth_connection_get_fd(struct wth_connection *conn)
{
  return conn->fd;
}

struct wth_display *
wth_connection_get_display(struct wth_connection *conn)
{
  return conn->display;
}

int
wth_connection_get_next_message_id(struct wth_connection *conn)
{
  return conn->next_message_id++;
}

int
wth_connection_get_next_object_id(struct wth_connection *conn)
{
  return ++conn->next_object_id;
}

GHashTable *
wth_connection_get_hash(struct wth_connection *conn)
{
  return conn->hash;
}

void
wth_connection_destroy(struct wth_connection *conn)
{
  close(conn->fd);

  wth_object_delete((struct wth_object *) conn->display);
  g_hash_table_destroy(conn->hash);

  free(conn);
}

int
wth_connection_flush(struct wth_connection *conn)
{
  /* FIXME currently we don't use the ringbuffer to send messages,
   * they are just written to the fd in write_all() in
   * src/marshaller/marshaller.h. */

  return 0;
}

int
wth_connection_read(struct wth_connection *conn)
{
  if (!reader_pull_new_messages(conn->reader, conn->fd, TRUE))
    return -1;
  else
    return 0;
}

int
wth_connection_dispatch(struct wth_connection *conn)
{
  int i, complete;

  for (i = 0 ; i < conn->reader->m_complete; i++)
    {
      msg_t msg;

      reader_map_message (conn->reader, i, &msg);
      g_debug ("Message received on conn %p: (%d, %d) %d bytes",
               conn, msg.hdr->api, msg.hdr->opcode, msg.hdr->sz);
      msg_dispatch (conn, &msg, NULL, NULL, NULL);
      reader_unmap_message (conn->reader, i, &msg);
    }

  complete = conn->reader->m_complete;

  /* Remove processed messages */
  reader_flush (conn->reader);

  return complete;
}

int
wth_roundtrip(struct wth_connection *conn)
{
  // FIXME
  return -1;
}
