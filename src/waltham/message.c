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

#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>
#include <sys/uio.h>
#include <inttypes.h>
#include <assert.h>

#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <netdb.h>

#include "message.h"
#include "demarshaller.h"
#include "waltham-private.h"

ClientReader *
new_reader (void)
{
  ClientReader *r = calloc(1, sizeof(ClientReader));

  r->ringsize = (MESSAGE_MAX_SIZE + sizeof(hdr_t)) * 2 + 1;
  r->ringbuffer = malloc (r->ringsize);

  r->rp = r->wp = r->ringbuffer;
  r->messages = calloc(1, 64 * sizeof(ReaderMessage));
  r->m_total = 64;

  return r;
}

void
free_reader (ClientReader *reader)
{
  free (reader->ringbuffer);
  free (reader->messages);
  free (reader->tail);
  free (reader->bounce);
  free (reader);
}

static inline ssize_t
bytes_left (ClientReader *reader, uint8_t *rp)
{
  ssize_t left;
  if (rp <= reader->wp)
    {
      left = reader->wp - rp;
    }
  else
    {
      /* reader till end */
      left = reader->ringbuffer + reader->ringsize - rp;
      /* start till wp */
      left += reader->wp - reader->ringbuffer;
    }

  return left;
}

static inline uint8_t *
move_forward (ClientReader *reader, uint8_t *rp, int offset)
{
  ssize_t o = ((rp + offset) - reader->ringbuffer) % reader->ringsize;
  return reader->ringbuffer + o;
}

static inline uint8_t
get_uint8 (ClientReader *reader, uint8_t *rp, int offset)
{
  return * move_forward (reader, rp, offset);
}

static inline uint16_t
get_uint16 (ClientReader *reader, uint8_t *rp, int offset)
{
  uint16_t r = get_uint8 (reader, rp, offset + 1);
  r <<= 8;
  r +=  get_uint8 (reader, rp, offset);

  return r;
}

static bool
get_one_message (ClientReader *reader)
{
  size_t size;
  size_t left = bytes_left (reader, reader->rp);

  if (left < sizeof(hdr_t))
    return false;

  size = get_uint16 (reader, reader->rp, M_OFFSET_SIZE);

  if (left < size)
    return false;

  if (size == 0)
    {
      wth_error ("Invalid message size (0)");
      return false;
    }

  reader->messages[reader->m_complete].start = reader->rp;
  reader->messages[reader->m_complete].length = size;


  if (reader->rp + (READER_MESSAGE_FIELDS * sizeof (uint16_t)) >
      reader->ringbuffer + reader->ringsize)
    {
      size_t left = reader->ringsize + reader->ringbuffer - reader->rp;
      uint8_t *base = (uint8_t*)&reader->messages[reader->m_complete].id;

      memcpy (base, reader->rp, left);
      memcpy (base + left, reader->ringbuffer,
              (READER_MESSAGE_FIELDS * sizeof (uint16_t)) - left);
    }
  else
    {
      memcpy (&reader->messages[reader->m_complete].id,
        reader->rp, READER_MESSAGE_FIELDS * sizeof (uint16_t));
    }

  reader->m_complete++;

  reader->rp = move_forward (reader, reader->rp, size);

  return true;
}

static bool
reader_fill_ring_buffer (ClientReader *reader, int fd)
{
  struct iovec vecs[2];
  int iocnt = 1;
  ssize_t ret;

  /* Assuming all messages are sent out before push */
  assert (reader->m_complete == 0);

  vecs[0].iov_base = reader->wp;
  if (reader->rp > reader->wp)
    {
      vecs[0].iov_len = reader->rp - reader->wp;
    }
  else
    {
      vecs[0].iov_len = reader->ringbuffer + reader->ringsize - reader->wp;
      if (reader->rp >  reader->ringbuffer)
        {
          vecs[1].iov_base = reader->ringbuffer;
          vecs[1].iov_len = reader->rp - reader->ringbuffer;
          iocnt++;
        }
    }
  /* Never let the write pointer  completely catch up with the read pointer */
  vecs[iocnt - 1].iov_len -= 1;

  ret = readv (fd, vecs, iocnt);
  if (ret <= 0) {
    wth_error ("Error while filling buffer: %m");
    return false;
  }

  reader->total_read += ret;

  if (iocnt == 1 || ret < (ssize_t) vecs[0].iov_len)
    reader->wp += ret;
  else
    reader->wp = reader->ringbuffer + ret - vecs[0].iov_len;

  assert (reader->wp != reader->rp);
  return true;
}


bool
reader_pull_new_messages (ClientReader *reader, int fd, bool from_client)
{
  if (!reader_fill_ring_buffer (reader, fd))
    return false;

  /* Setup message headers */
  while (get_one_message (reader))
    {
      if (reader->m_complete == reader->m_total)
        {
          reader->messages = realloc (reader->messages,
            reader->m_total * 2 * sizeof(ReaderMessage));
          reader->m_total *= 2;
          wth_debug ("Updated client to %d messages", reader->m_total);
        }
    }
  return true;
}

/* File one message buffer */
void
reader_map_message (ClientReader *reader, int m, msg_t *msg)
{
  ReaderMessage *rm;
  int nc = 0;
  void *start;

  assert (m >= 0 && m < reader->m_complete);
  memset (msg, 0, sizeof (msg_t));

  rm = &reader->messages[m];

  start = rm->start;
  if ((rm->start + rm->length)
      > (reader->ringbuffer + reader->ringsize))
    {
      size_t l;
      while (reader->allocated_bouncesize < rm->length)
        {
          free (reader->bounce);
          if (reader->allocated_bouncesize == 0)
            reader->allocated_bouncesize = 2048;
          else
            reader->allocated_bouncesize *= 2;
          reader->bounce = malloc (reader->allocated_bouncesize);
        }
      /* split message, simply copy the whole message to a bounce buffer  */
      l = (reader->ringbuffer + reader->ringsize) - rm->start;
      memcpy (reader->bounce, rm->start, l);
      memcpy (reader->bounce + l, reader->ringbuffer, rm->length - l);
      start = reader->bounce;
    }

  /* Whole message is now linearly in memory from start */
  msg->hdr = start;
  msg->body = start + sizeof(hdr_t);
  if (rm->length > msg->hdr->sz)
    {
      msg->chunks[nc].data = start + msg->hdr->sz;
      msg->chunks[nc].size = rm->length - msg->hdr->sz;
      nc++;
    }

  /* Grab data from the readers tail if applicable */
  if (m + 1 == reader->m_complete && reader->tailsize)
    {
      msg->chunks[nc].data = (void *)reader->tail;
      msg->chunks[nc].size = reader->tailsize;
    }
}

void
reader_unmap_message (ClientReader *reader, int m, msg_t *msg)
{
}

bool
reader_forward_message_range (ClientReader *reader, int fd, int s, int e)
{
  struct iovec vecs[3];
  int iocnt = 1;
  ssize_t ret;
  uint8_t *start;
  uint8_t *end;

  assert (s <= e && e < reader->m_complete);

  start = reader->messages[s].start;
  end = move_forward (reader, reader->messages[e].start,
    reader->messages[e].length);

  assert (reader->m_complete > 0);

  memset (vecs, 0, 3 * sizeof(struct iovec));
  vecs[0].iov_base = start;
  if (end > start)
    {
      vecs[0].iov_len = end - start;
    }
  else
    {
      vecs[0].iov_len = reader->ringbuffer + reader->ringsize - start;
      vecs[1].iov_base = reader->ringbuffer;
      vecs[1].iov_len = end - reader->ringbuffer;
      if (vecs[1].iov_len > 0)
        iocnt++;
    }

  if (e == (reader->m_complete -1) && reader->tailsize != 0)
    {
      vecs[iocnt].iov_base = reader->tail;
      vecs[iocnt].iov_len = reader->tailsize;
      iocnt++;
    }

  ret = writev (fd, vecs, iocnt);

  return ret == (ssize_t)(vecs[0].iov_len + vecs[1].iov_len + vecs[2].iov_len);

}

void
reader_flush (ClientReader *reader)
{
  /* Micro-optimisation.. In most cases the message handling will have
   * handled the full ringbuffer (e.g. the readv call read in upto the message
   * boundaries). In that case reset the read & write pointers to the
   * ringbuffer start, such that the next read won't wrap around trigger a need
   * for using the bounce buffer (and hence an extra memcpy) */
  if (reader->wp == reader->rp)
    reader->wp = reader->rp = reader->ringbuffer;
  reader->m_complete = 0;
  reader->tailsize = 0;
}

bool
reader_forward_all_messages (ClientReader *reader, int fd)
{
  bool ret;

  ret = reader_forward_message_range (reader, fd, 0, reader->m_complete - 1);

  reader_flush (reader);

  return ret;
}

bool
forward_raw_msg (int fd, msg_t *msg)
{
  struct iovec vecs[4];
  int iocnt = 1;
  size_t ret;

  memset (vecs, 0, 4 * sizeof(struct iovec));

  vecs[0].iov_base = msg->hdr;
  vecs[0].iov_len = sizeof(hdr_t);
  if ((msg->hdr->sz - sizeof(hdr_t)) > 0)
    {
      vecs[iocnt].iov_base = msg->body;
      vecs[iocnt].iov_len = msg->hdr->sz - sizeof(hdr_t);
      iocnt++;
    }

  if (msg->chunks[0].size)
    {
      vecs[iocnt].iov_base = msg->chunks[0].data;
      vecs[iocnt].iov_len = msg->chunks[0].size;
      iocnt++;
    }

  if (msg->chunks[1].size)
    {
      vecs[iocnt].iov_base = msg->chunks[1].data;
      vecs[iocnt].iov_len = msg->chunks[1].size;
      iocnt++;
    }

  ret = writev (fd, vecs, iocnt);
  return ret == (vecs[0].iov_len + vecs[1].iov_len
                  + vecs[2].iov_len + vecs[3].iov_len);
}

msg_t *
copy_msg (msg_t *msg)
{
  size_t dsize;
  msg_t *m = calloc (1, sizeof (msg_t));

  m->hdr = malloc (msg->hdr->sz);
  memcpy (m->hdr, msg->hdr, sizeof(hdr_t));

  m->body = (char *)m->hdr + sizeof(hdr_t);
  memcpy (m->body, msg->body, msg->hdr->sz - sizeof (hdr_t));

  dsize = m->chunks[0].size + m->chunks[1].size;
  if (dsize > 0)
    {
      m->chunks[0].data = malloc (dsize);
      m->chunks[0].size = dsize;
      memcpy (m->chunks[0].data, msg->chunks[0].data, msg->chunks[0].size);
      memcpy (m->chunks[0].data + msg->chunks[0].size, msg->chunks[1].data,
        msg->chunks[1].size);
    }

  return m;
}

void
free_msg (msg_t *msg)
{
  free (msg->hdr);
  free (msg->chunks[0].data);
  free (msg);
}

void
msg_dispatch (struct wth_connection *conn, msg_t *msg)
{
  if (msg->hdr->opcode > demarshaller_max_opcode
      || (request_demarshaller_functions[msg->hdr->opcode] == NULL &&
          event_demarshaller_functions[msg->hdr->opcode] == NULL)) {
    wth_error ("Invalid opcode %d, discard message", msg->hdr->opcode);
    return;
  }

  if (request_demarshaller_functions[msg->hdr->opcode])
    request_demarshaller_functions[msg->hdr->opcode](conn, msg->hdr, msg->body);
  else
    event_demarshaller_functions[msg->hdr->opcode](conn, msg->hdr, msg->body);
}

/* Network helpers */
int
connect_to_unix_socket (const char *path)
{
  int fd;
  fd = socket (AF_UNIX, SOCK_STREAM, 0);
  struct sockaddr_un addr;

  if (fd < 0)
    {
      wth_debug ("Failed to open socket: %s", strerror (errno));
      return fd;
    }

  memset (&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  memcpy (addr.sun_path + 1, path, strlen(path) + 1);

  if (connect (fd, (struct sockaddr *) &addr,
      sizeof(addr.sun_family) + 1 + strlen (path)) < 0)
    {
      wth_debug ("Failed to connect to %s: %s", path, strerror (errno));
      close (fd);
      return -1;
    }

  return fd;
}

int
connect_to_host (const char *host, const char *port)
{
  int flag = 1;
  int fd;
  int ret;
  struct addrinfo hints = { 0, };
  struct addrinfo *res, *r;

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  ret = getaddrinfo (host, port, &hints, &res);
  if (ret < 0)
    {
      wth_error ("Connect to %s port %s failed (getaddrinfo: %s)",
        host, port,
        strerror(errno));
      return -1;
    }

  for (r = res; r != NULL ; r = r->ai_next)
    {
      fd = socket (r->ai_family, r->ai_socktype, r->ai_protocol);
      if (fd == -1)
        continue;

      if (connect (fd, r->ai_addr, r->ai_addrlen) != -1)
        break; /* success */

      close (fd);
      fd = -1;
    }


  if (fd != -1)
    setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, (char *) &flag, sizeof(int));

  return fd;
}
