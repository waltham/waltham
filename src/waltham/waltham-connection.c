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

//#include <waltham-object.h>
//#include <waltham-client.h>
//#include <waltham-client-protocol.h>

#include "waltham-connection.h"

#include "util/message.h"
#include "gael_serialization.h"
#include "grayley.h"

// FIXME
struct wth_display {

};

struct wth_connection {
  int fd;
  enum wth_connection_side side;

  ClientReader *reader;

  struct wth_display *display;
};


struct wth_connection *
wth_connect_to_server(const char *host, const char *port)
{
  struct wth_connection *conn = NULL;
  int fd;

  fd = connect_to_host(host, port);

  if (fd >= 0)
    {
      conn = wth_connection_from_fd(fd, WTH_CONNECTION_SIDE_CLIENT);
      gael_set_fd(fd);
    }

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

  conn->reader = new_reader (0);

  conn->display = wth_object_new ();

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

void
wth_connection_destroy(struct wth_connection *conn)
{
  close(conn->fd);

  free(conn->display);
  free(conn);
}

int
wth_connection_flush(struct wth_connection *conn)
{
  reader_flush(conn->reader);

  // FIXME return value?

  return 0;
}

int
wth_connection_read(struct wth_connection *conn)
{
  return reader_pull_new_messages(conn->reader, conn->fd, TRUE);
}

static void
dispatch_msg (msg_t *msg)
{
  gboolean ret;

  g_debug ("Message received: client %d (%d, %d) %d bytes",
           msg->hdr->client_id, msg->hdr->api, msg->hdr->opcode, msg->hdr->sz);

  ret = grayley_dispatch (msg, NULL, NULL, NULL);
}

int
wth_connection_dispatch(struct wth_connection *conn)
{
  int i;

  for (i = 0 ; i < conn->reader->m_complete; i++)
    {
      msg_t msg;

      reader_map_message (conn->reader, i, &msg);
      dispatch_msg (&msg);
      reader_unmap_message (conn->reader, i, &msg);
    }

  return conn->reader->m_complete;
}

int
wth_roundtrip(struct wth_connection *conn)
{
  // FIXME
  return -1;
}
