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

#ifndef WALTHAM_MESSAGE_H
#define WALTHAM_MESSAGE_H

#include <stdint.h>

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

#define OPCODE_REPLY 0xffff

#endif
