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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include "gael_serialization.h"

pthread_mutex_t gael_mutex __attribute__ ((visibility ("default"))) = PTHREAD_MUTEX_INITIALIZER;
int timing_level __attribute__ ((visibility ("default"))) = 0;

int request_id = 0;
int client_id = 0;

static int gael_fd = -1;

void
gael_connect_client (void)
{
   struct iovec gael_params[2];
   int fd = gael_get_fd ();
   int ret;
   int version;
   hdr_t hdr = { 0, 0, sizeof(hdr), API_CONTROL, API_CONTROL_CONNECT_CLIENT, 0 };

   g_print ("connect_client\n");

   do {
      ret = write(fd, (unsigned char *) &hdr, sizeof(hdr_t));
   } while (ret == -1 && errno == EINTR);

   gael_params[0].iov_base = &hdr;
   gael_params[0].iov_len = sizeof(hdr_t);
   gael_params[1].iov_base = &version;
   gael_params[1].iov_len = sizeof(int);

   ret = recv_all(fd, gael_params, 2);
   assert(hdr.opcode == OPCODE_REPLY); // && hdr.client_id != 0);

   client_id = hdr.client_id;

   printf("Connected client (id=%i)\n", client_id);
}

int
gael_get_client_id (void)
{
   return client_id;
}

int
gael_get_new_request_id (void)
{
   return request_id++;
}

int
gael_get_request_id (void)
{
   return request_id;
}

void
gael_set_fd (int fd)
{
   gael_fd = fd;
}

int
gael_get_fd (void)
{
   return gael_fd;
}
