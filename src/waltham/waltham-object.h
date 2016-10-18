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

#ifndef WALTHAM_OBJECT_H
#define WALTHAM_OBJECT_H

#include <stdint.h>

#ifdef  __cplusplus
extern "C" {
#endif

struct wth_object;

void
wth_object_delete (struct wth_object *object);

void
wth_object_set_listener(struct wth_object *obj,
			void (**listener)(void), void *user_data);

void *
wth_object_get_user_data(struct wth_object *obj);

/** Post a fatal protocol error to a client
 *
 * \param obj The object that specifies the error code.
 * \param code The error code, specific to the object's interface.
 * \param fmt A freeform description of the error for human readers;
 * printf-style format string.
 *
 * This call sends the protocol error event to the client, and sets
 * the associated wth_connection into protocol error state. As a result,
 * no further events will reach the client, and no requests from the
 * client will be dispatched.
 *
 * After calling this, the server is expected to keep the wth_connection
 * open until the client disconnects. This ensures the error event
 * reaches the client.
 *
 * Clients cannot use this function.
 */
void
wth_object_post_error(struct wth_object *obj,
		      uint32_t code,
		      const char *fmt, ...);

#ifdef  __cplusplus
}
#endif

#endif
