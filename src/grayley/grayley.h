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

#ifndef __GRAYLEY_H__
#define __GRAYLEY_H__

#include <glib.h>
#include <gio/gio.h>

#include "waltham-message.h"
#include "grayley_loop.h"

#define API_CALL __attribute__ ((visibility ("default")))

/* functions */

extern void grayley_init (struct grayley_loop_t *loop) API_CALL;
extern void grayley_bind_wl_display (struct wl_display *display) API_CALL;
extern void grayley_notify_disconnect (void) API_CALL;

extern gboolean grayley_dispatch (msg_t *msg, 
                                  hdr_t *header_reply, 
                                  char *body_reply, data_t *data_reply) API_CALL;
extern gboolean grayley_client_is_compositor (unsigned short client_id) API_CALL;

void grayley_sync_upstream(void) API_CALL;

/* variables */

extern const int grayley_max_opcode;

typedef gboolean (*grayley_helper_function_t)(const hdr_t *header,
        const char *body, hdr_t *header_reply, char *body_reply);

extern const grayley_helper_function_t grayley_functions[];

#endif
