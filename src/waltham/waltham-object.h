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

#ifndef __WTH_OBJECT__
#define __WTH_OBJECT__

#include <stdint.h>
#include <glib.h>

#define APICALL __attribute__ ((visibility ("default")))

/* FIXME: move to waltham-private.h ? */
struct wth_object {
	struct wth_display *display;
	struct wth_connection *connection;
	uint32_t id;

	void (**vfunc)(void);
	void *user_data;
};

struct wth_object *
wth_object_new_with_id (struct wth_connection *connection, uint32_t id) APICALL;

struct wth_object *
wth_object_new (struct wth_connection *connection) APICALL;

void
wth_object_delete (struct wth_object *object) APICALL;

void
wth_object_set_listener(struct wth_object *obj,
			void (**listener)(void), void *user_data) APICALL;

void *
wth_object_get_user_data(struct wth_object *obj) APICALL;

#endif /* __WTH_OBJECT__ */
