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

#ifndef __GRAYLEY_LOOP_H__
#define __GRAYLEY_LOOP_H__

#include <glib.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>

#define API_CALL __attribute__ ((visibility ("default")))

struct grayley_loop_t;
struct grayley_loop_source_t;
typedef struct _EPollLoop EPollLoop;

typedef gboolean (*grayley_loop_cb_t)(gpointer data);

struct grayley_loop_t* grayley_loop_new_from_wayland (struct wl_event_loop *loop);
struct grayley_loop_t* grayley_loop_new_from_glib (GMainLoop *loop) API_CALL;
struct grayley_loop_t* grayley_loop_new_from_epoll (EPollLoop *loop) API_CALL;

struct grayley_loop_source_t* grayley_loop_add_fd (struct grayley_loop_t *loop,
   int fd, GIOCondition cond, grayley_loop_cb_t callback, gpointer data);

#endif
