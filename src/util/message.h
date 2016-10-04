/*
 * Copyright © 2013-2014 Collabora, Ltd.
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

#ifndef __UTIL_MESSAGE_H__
#define __UTIL_MESSAGE_H__

#include "waltham-message.h"

#include <glib.h>
//#include <gio/gio.h>

#define API_CALL __attribute__ ((visibility ("default")))

#define MSG_FORMAT "id: %hu, sz: %hu, api: %hu, opcode: %hu"
#define MSG_MEM(msg) msg->hdr->id, msg->hdr->sz, msg->hdr->api, msg->hdr->opcode
#define MSG_MEMS(msg) msg->header.id, msg->header.sz, msg->header.api, msg->header.opcode


gboolean
is_valid_command (unsigned short api, unsigned short opcode) API_CALL;

gboolean
forward_raw_msg (int out, msg_t *msg) API_CALL;

msg_t *
copy_msg (msg_t *msg) API_CALL;

void
free_msg (msg_t *msg) API_CALL;

gboolean
msg_dispatch (msg_t *msg,
    hdr_t *header_reply, char *body_reply, data_t *data_reply) API_CALL;

/**** Ringbuffer based network reader & handler */
typedef struct {
  guint8 *start;
  gssize length;
  guint16 id;
  guint16 sz;
  guint16 api;
  guint16 opcode;
} ReaderMessage;

typedef struct {
  guint8 *ringbuffer;
  gssize ringsize;
  guint8 *rp; /* read pointer */
  guint8 *wp; /* write pointer */
  ReaderMessage *messages; /* complete messages in the  ring */
  int m_complete;
  int m_total; /* total number of message slots */

  /* potential data tail */
  guint8 *tail;
  gssize tailsize;
  gssize allocated_tailsize;

  /* mapping bounce buffer */
  guint8 *bounce;
  gssize allocated_bouncesize;

  /* Stats */
  gsize total_read;

} ClientReader;

ClientReader *new_reader (void) API_CALL;
void free_reader (ClientReader *reader) API_CALL;

gboolean reader_pull_new_messages (ClientReader *reader, int fd,
  gboolean from_client) API_CALL;

void reader_map_message (ClientReader *reader, int m, msg_t *msg) API_CALL;
void reader_unmap_message (ClientReader *reader, int m, msg_t *msg) API_CALL;

/* Forward complete messages */
gboolean reader_forward_message_range (ClientReader *reader, int fd,
  int s, int e) API_CALL;
gboolean reader_forward_all_messages (ClientReader *reader, int fd) API_CALL;
void reader_flush (ClientReader *reader) API_CALL;

/** Network helpers */
int connect_to_host (const char *host, const char *port) API_CALL;
int connect_to_unix_socket (const char *path) API_CALL;

/* Trivial epoll based loop implementation, fds can only be added once! */
typedef struct _EPollLoop EPollLoop;

struct _EPollLoop {
  int epoll_fd;
  int n_clients;
  int n_events;
  struct epoll_event *events;
  GHashTable *callbacks;
  int priority_fd;
};


typedef gboolean (*epoll_callback) (EPollLoop *loop, int fd,
    GIOCondition cond, gpointer data);

void epoll_loop_init (EPollLoop *loop) API_CALL;
void epoll_loop_deinit (EPollLoop *loop) API_CALL;
void epoll_loop_iterate (EPollLoop *loop) API_CALL;
void epoll_loop_add_fd (EPollLoop *loop, int fd,
  GIOCondition cond, epoll_callback cb, gpointer data) API_CALL;
void epoll_loop_remove_fd (EPollLoop *loop, int fd) API_CALL;
void epoll_loop_set_priority(EPollLoop *loop, int fd) API_CALL;


#endif
