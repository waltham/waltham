/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
 * Copyright © 2013 Jason Ekstrand
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WALTHAM_PRIVATE_H
#define WALTHAM_PRIVATE_H

#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>

#include "waltham-util.h"

#define ARRAY_LENGTH(a) (sizeof (a) / sizeof (a)[0])

#define WTH_MAP_SERVER_SIDE 0
#define WTH_MAP_CLIENT_SIDE 1
#define WTH_SERVER_ID_START 0xff000000

/* Flags for wth_map_insert_new and wth_map_insert_at.  Flags can be queried with
 * wth_map_lookup_flags.  The current implementation has room for 1 bit worth of
 * flags.  If more flags are ever added, the implementation of wth_map will have
 * to change to allow for new flags */
enum wth_map_entry_flags {
	WTH_MAP_ENTRY_LEGACY = (1 << 0)
};

struct wth_map {
	struct wth_array client_entries;
	struct wth_array server_entries;
	uint32_t side;
	uint32_t free_list;
};

typedef enum wth_iterator_result (*wth_iterator_func_t)(void *element,
							void *data);

void
wth_map_init(struct wth_map *map, uint32_t side);

void
wth_map_release(struct wth_map *map);

uint32_t
wth_map_insert_new(struct wth_map *map, uint32_t flags, void *data);

int
wth_map_insert_at(struct wth_map *map, uint32_t flags, uint32_t i, void *data);

int
wth_map_reserve_new(struct wth_map *map, uint32_t i);

void
wth_map_remove(struct wth_map *map, uint32_t i);

void *
wth_map_lookup(struct wth_map *map, uint32_t i);

uint32_t
wth_map_lookup_flags(struct wth_map *map, uint32_t i);

void
wth_map_for_each(struct wth_map *map, wth_iterator_func_t func, void *data);

#endif
