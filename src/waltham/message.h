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

#include <stdint.h>
#include <stdbool.h>

struct wth_connection;

#define M_OFFSET_MSG_ID 0
#define M_OFFSET_SIZE 2
#define M_OFFSET_OPCODE 4

typedef struct __attribute__((__packed__)) hdr_t {
   unsigned short id;
   unsigned short sz;
   unsigned short opcode;
   unsigned short pad;
} hdr_t;
#define MESSAGE_MAX_SIZE (0xffff - sizeof (hdr_t))

typedef struct data_t {
   unsigned int sz;
   void *data;
} data_t;

typedef struct {
  hdr_t *hdr;
  char *body;
  struct chunk {
    char *data;
    size_t size;
  } chunks[2];
} msg_t;

bool
forward_raw_msg (int out, msg_t *msg);

msg_t *
copy_msg (msg_t *msg);

void
free_msg (msg_t *msg);

void
msg_dispatch (struct wth_connection *conn, msg_t *msg);

/**** Ringbuffer based network reader & handler */
typedef struct {
  uint8_t *start;
  ssize_t length;
  uint16_t id;
  uint16_t sz;
  uint16_t opcode;
  uint16_t pad;
} ReaderMessage;

/* number of uint16_t fields, starting from id, in ReaderMessage */
#define READER_MESSAGE_FIELDS 4

typedef struct {
  uint8_t *ringbuffer;
  ssize_t ringsize;
  uint8_t *rp; /* read pointer */
  uint8_t *wp; /* write pointer */
  ReaderMessage *messages; /* complete messages in the  ring */
  int m_complete;
  int m_total; /* total number of message slots */

  /* potential data tail */
  uint8_t *tail;
  ssize_t tailsize;
  ssize_t allocated_tailsize;

  /* mapping bounce buffer */
  uint8_t *bounce;
  ssize_t allocated_bouncesize;

  /* Stats */
  size_t total_read;

} ClientReader;

ClientReader *new_reader (void);
void free_reader (ClientReader *reader);

bool reader_pull_new_messages (ClientReader *reader, int fd,
  bool from_client);

void reader_map_message (ClientReader *reader, int m, msg_t *msg);
void reader_unmap_message (ClientReader *reader, int m, msg_t *msg);

/* Forward complete messages */
bool reader_forward_message_range (ClientReader *reader, int fd,
  int s, int e);
bool reader_forward_all_messages (ClientReader *reader, int fd);
void reader_flush (ClientReader *reader);

/** Network helpers */
int connect_to_host (const char *host, const char *port);
int connect_to_unix_socket (const char *path);

#endif
