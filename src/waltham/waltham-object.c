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

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "waltham-connection.h"

#include "waltham-object.h"

GHashTable *hash;

struct wth_object *
wth_object_new_with_id (struct wth_connection *connection, uint32_t id)
{
	struct wth_object *proxy = NULL;

	if (hash == NULL)
		hash = g_hash_table_new (g_direct_hash, g_direct_equal);

	proxy = malloc(sizeof *proxy);
	memset (proxy, 0, sizeof *proxy);

	if (proxy == NULL)
		return NULL;

	g_hash_table_insert (hash, GUINT_TO_POINTER (id), proxy);
	proxy->id = id;
	proxy->connection = connection;

	return proxy;
}

struct wth_object *
wth_object_new (struct wth_connection *connection)
{
	int id = wth_connection_get_next_object_id (connection);

	return wth_object_new_with_id (connection, id);
}

void
wth_object_delete (struct wth_object *object)
{
	g_hash_table_remove (hash, GUINT_TO_POINTER (object->id));

	free (object);
}

void
wth_object_set_listener(struct wth_object *obj,
			void (**listener)(void), void *user_data)
{
	if (obj->vfunc)
		g_warning ("vfunc table already set!");

	obj->vfunc = listener;
	obj->user_data = user_data;
}

void *
wth_object_get_user_data(struct wth_object *obj)
{
	return obj->user_data;
}
