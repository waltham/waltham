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

#ifndef MESSAGE_H
#define MESSAGE_H

#include "waltham-message.h"

#include <glib.h>

struct wth_connection;

gboolean
forward_raw_msg (int out, msg_t *msg);

msg_t *
copy_msg (msg_t *msg);

void
free_msg (msg_t *msg);

void
msg_dispatch (struct wth_connection *conn, msg_t *msg);

/**** Ringbuffer based network reader & handler */
typedef struct {
  guint8 *start;
  gssize length;
  guint16 id;
  guint16 sz;
  guint16 opcode;
  guint16 pad;
} ReaderMessage;

/* number of guint16 fields, starting from id, in ReaderMessage */
#define READER_MESSAGE_FIELDS 4

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

ClientReader *new_reader (void);
void free_reader (ClientReader *reader);

gboolean reader_pull_new_messages (ClientReader *reader, int fd,
  gboolean from_client);

void reader_map_message (ClientReader *reader, int m, msg_t *msg);
void reader_unmap_message (ClientReader *reader, int m, msg_t *msg);

/* Forward complete messages */
gboolean reader_forward_message_range (ClientReader *reader, int fd,
  int s, int e);
gboolean reader_forward_all_messages (ClientReader *reader, int fd);
void reader_flush (ClientReader *reader);

/** Network helpers */
int connect_to_host (const char *host, const char *port);
int connect_to_unix_socket (const char *path);

#endif
