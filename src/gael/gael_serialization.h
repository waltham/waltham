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

#ifndef __GAEL_SERIALIZATION_H__
#define __GAEL_SERIALIZATION_H__

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <assert.h>
#include <dlfcn.h>
#include <poll.h>

#include "waltham-message.h"
#include "gael_log.h"

/* All shared libraries in a process wanting to serialize data
 * to the AD side need to LOCK the common mutex first */

/* This variable should only be declared in one shared library */
extern pthread_mutex_t gael_mutex;

/* These functions should only be implemented in one shared library */
int gael_get_client_id (void) __attribute__ ((visibility ("default")));
int gael_get_request_id (void) __attribute__ ((visibility ("default")));
int gael_get_new_request_id (void) __attribute__ ((visibility ("default")));
int gael_get_fd (void) __attribute__ ((visibility ("default")));
void gael_set_fd (int fd) __attribute__ ((visibility ("default")));
void gael_connect_client (void) __attribute__ ((visibility ("default")));

static inline int send_all (int sock, const struct iovec *iov, int iovcnt)
{
   int ret;

   do {
      ret = writev (sock, iov, iovcnt);
   } while (ret == -1 && errno == EINTR);

   return ret;
}

static inline int recv_all (int sock, struct iovec *iov, int iovcnt)
{
   ssize_t ret;

   do {
      ret = readv (sock, iov, iovcnt);

      while (ret > 0) {
         if ((size_t) ret < iov[0].iov_len) {
            iov[0].iov_base = ((uint8_t *) iov[0].iov_base) + ret;
            iov[0].iov_len -= ret;
            break;
         }
         ret -= iov[0].iov_len;
         iov++;
         iovcnt--;
      }

   } while ((ret == -1 && errno == EINTR) || iovcnt > 0);

   return ret;
}

#define ABORT_CALL_NR() { \
   ABORT_TIMING(""); \
   return; \
}

#define ABORT_CALL(ret, _fmt, ...) { \
   ABORT_TIMING(_fmt, ## __VA_ARGS__); \
   return ret; \
}


#define LOCK()                          \
   pthread_mutex_lock(&gael_mutex)
#define UNLOCK()                        \
   pthread_mutex_unlock(&gael_mutex)

#define PADDED(sz) \
   (((sz) + 3) & ~3)

#define START_MESSAGE(name, sz, api, opcode) \
   const char *msg_name __attribute__((unused)) = name; \
   hdr_t hdr = { gael_get_client_id(), gael_get_new_request_id(), sz, api, opcode, 0 }; \
   int send_ret; \
   struct iovec gael_params[16]; \
   int gael_paramid = 1; \
   int param_padding __attribute__((unused)) = 0; \
   gael_params[0].iov_base = (void *) &hdr; \
   gael_params[0].iov_len = sizeof(hdr_t); \
   DEBUG_STAMP (); \
   STREAM_DEBUG ((unsigned char *) &hdr, sizeof (hdr), "header -> ");

#define END_MESSAGE() \
   send_ret = send_all (gael_get_fd (), gael_params, gael_paramid); \
   if (send_ret == -1) \
      exit (errno); \
   DEBUG_TYPE(msg_name);

#define ADD_PADDING(sz) \
   param_padding = (4 - ((sz) & 3)) & 3; \
   if (param_padding > 0) { \
      gael_params[gael_paramid].iov_base = (void *) &gael_paramid; \
      gael_params[gael_paramid].iov_len = param_padding; \
      gael_paramid++; \
   }

#define SERIALIZE_PARAM(param, sz) \
   gael_params[gael_paramid].iov_base = (void *) param; \
   gael_params[gael_paramid].iov_len = sz; \
   gael_paramid++; \
   ADD_PADDING (sz); \
   STREAM_DEBUG ((unsigned char *) param, sz, "param  -> ");

#define SERIALIZE_PARAM_SILENT(param, sz) \
   gael_params[gael_paramid].iov_base = param; \
   gael_params[gael_paramid].iov_len = sz; \
   gael_paramid++; \
   ADD_PADDING (sz); \
   STREAM_DEBUG_DATA (param, sz);

#define SERIALIZE_DATA(data, sz) \
   gael_params[gael_paramid].iov_base = (void *) &sz; \
   gael_params[gael_paramid].iov_len = sizeof (int); \
   gael_paramid++; \
   STREAM_DEBUG ((unsigned char *) &sz, sizeof (int), "data sz   -> "); \
   gael_params[gael_paramid].iov_base = data; \
   gael_params[gael_paramid].iov_len = sz; \
   gael_paramid++; \
   ADD_PADDING (sz); \
   STREAM_DEBUG ((unsigned char *) data, sz, "data   -> ");

#define SERIALIZE_ARRAY(array) \
   gael_params[gael_paramid].iov_base = (void *) &array->size; \
   gael_params[gael_paramid].iov_len = sizeof (size_t); \
   gael_paramid++; \
   STREAM_DEBUG ((unsigned char *) &sz, sizeof (int), "array sz  -> "); \
   gael_params[gael_paramid].iov_base = array->data; \
   gael_params[gael_paramid].iov_len = array->size; \
   gael_paramid++; \
   ADD_PADDING (array->size); \
   STREAM_DEBUG ((unsigned char *) array->data, array->size, "array  -> ");

#define FLUSH_RECV() \
   if (gael_paramid > 0) { \
      int recv_ret; \
      recv_ret = recv_all (gael_get_fd (), gael_params, gael_paramid); \
      if (recv_ret == -1) \
         exit (errno); \
      gael_paramid = 0; \
   }

//STREAM_DEBUG ((unsigned char *) param, sz, "       <- ");
//STREAM_DEBUG_DATA (param, sz);

#define START_REPLY() \
   int data_sz __attribute__((unused)); \
   char recv_buffer[10] __attribute__((unused)); \
   char padding_buffer[4] __attribute__((unused)); \
   gael_paramid = 1; \
   gael_params[0].iov_base = (void *) &hdr; \
   gael_params[0].iov_len = sizeof(hdr_t); \
   DEBUG_STAMP(); \

#define END_REPLY(type) \
   FLUSH_RECV() \
   STREAM_DEBUG ((unsigned char *) &hdr, sizeof (hdr), "header <- "); \
   if (hdr.opcode != OPCODE_REPLY) { \
      printf ("\nInvalid message received in %s:\n", msg_name); \
      printf ("id=%d sz=%d (incl. header %zu) api=%d opcode=%d\n", \
              hdr.id, hdr.sz, sizeof (hdr), hdr.api, hdr.opcode); \
   } \
   assert(hdr.opcode == OPCODE_REPLY); \
   DEBUG_TYPE(type);

#define WAIT_REPLY()

#define SKIP_PADDING(sz) \
   param_padding = (4 - ((sz) & 3)) & 3; \
   if (param_padding > 0) { \
      gael_params[gael_paramid].iov_base = (void *) &padding_buffer; \
      gael_params[gael_paramid].iov_len = param_padding; \
      gael_paramid++; \
   }

#define DESERIALIZE_PARAM(param, sz) \
   gael_params[gael_paramid].iov_base = (void *) param; \
   gael_params[gael_paramid].iov_len = sz; \
   gael_paramid++; \
   SKIP_PADDING(sz);

#define DESERIALIZE_PARAM_SILENT(param, sz) \
   gael_params[gael_paramid].iov_base = (void *) param; \
   gael_params[gael_paramid].iov_len = sz; \
   gael_paramid++; \
   SKIP_PADDING(sz);

#define DESERIALIZE_DATA(data) \
   gael_params[gael_paramid].iov_base = (void *) &data_sz; \
   gael_params[gael_paramid].iov_len = sizeof (data_sz); \
   gael_paramid++; \
   FLUSH_RECV(); \
   if (data_sz > 0) { \
      DESERIALIZE_PARAM (data, data_sz) \
   }

#define DESERIALIZE_OPTIONAL_PARAM(param, sz) \
   if (param) { \
      DESERIALIZE_PARAM (param, sz); \
   } else { \
      DESERIALIZE_PARAM (recv_buffer, sz); \
   }

#endif
