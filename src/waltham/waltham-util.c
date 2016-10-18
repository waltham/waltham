/*
 * Copyright © 2008-2011 Kristian Høgsberg
 * Copyright © 2011 Intel Corporation
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

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "waltham-util.h"
#include "waltham-connection.h"
#include "waltham-private.h"

WTH_EXPORT void
wth_array_init(struct wth_array *array)
{
	memset(array, 0, sizeof *array);
}

WTH_EXPORT void
wth_array_release(struct wth_array *array)
{
	free(array->data);
}

WTH_EXPORT void *
wth_array_add(struct wth_array *array, size_t size)
{
	size_t alloc;
	void *data, *p;

	if (array->alloc > 0)
		alloc = array->alloc;
	else
		alloc = 16;

	while (alloc < array->size + size)
		alloc *= 2;

	if (array->alloc < alloc) {
		if (array->alloc > 0)
			data = realloc(array->data, alloc);
		else
			data = malloc(alloc);

		if (data == NULL)
			return NULL;
		array->data = data;
		array->alloc = alloc;
	}

	p = array->data + array->size;
	array->size += size;

	return p;
}

WTH_EXPORT int
wth_array_copy(struct wth_array *array, struct wth_array *source)
{
	if (array->size < source->size) {
		if (!wth_array_add(array, source->size - array->size))
			return -1;
	} else {
		array->size = source->size;
	}

	memcpy(array->data, source->data, source->size);
	return 0;
}

union map_entry {
	uintptr_t next;
	void *data;
};

#define map_entry_is_free(entry) ((entry).next & 0x1)
#define map_entry_get_data(entry) ((void *)((entry).next & ~(uintptr_t)0x3))
#define map_entry_get_flags(entry) (((entry).next >> 1) & 0x1)

WTH_EXPORT void
wth_map_init(struct wth_map *map, uint32_t side)
{
	memset(map, 0, sizeof *map);
	map->side = side;
}

WTH_EXPORT void
wth_map_release(struct wth_map *map)
{
	wth_array_release(&map->client_entries);
	wth_array_release(&map->server_entries);
}

WTH_EXPORT uint32_t
wth_map_insert_new(struct wth_map *map, uint32_t flags, void *data)
{
	union map_entry *start, *entry;
	struct wth_array *entries;
	uint32_t base;

	if (map->side == WTH_CONNECTION_SIDE_CLIENT) {
		entries = &map->client_entries;
		base = 0;
	} else {
		entries = &map->server_entries;
		base = WTH_SERVER_ID_START;
	}

	if (map->free_list) {
		start = entries->data;
		entry = &start[map->free_list >> 1];
		map->free_list = entry->next;
	} else {
		entry = wth_array_add(entries, sizeof *entry);
		if (!entry)
			return 0;
		start = entries->data;
	}

	entry->data = data;
	entry->next |= (flags & 0x1) << 1;

	return (entry - start) + base;
}

WTH_EXPORT int
wth_map_insert_at(struct wth_map *map, uint32_t flags, uint32_t i, void *data)
{
	union map_entry *start;
	uint32_t count;
	struct wth_array *entries;

	if (i < WTH_SERVER_ID_START) {
		entries = &map->client_entries;
	} else {
		entries = &map->server_entries;
		i -= WTH_SERVER_ID_START;
	}

	count = entries->size / sizeof *start;
	if (count < i)
		return -1;

	if (count == i)
		wth_array_add(entries, sizeof *start);

	start = entries->data;
	start[i].data = data;
	start[i].next |= (flags & 0x1) << 1;

	return 0;
}

WTH_EXPORT int
wth_map_reserve_new(struct wth_map *map, uint32_t i)
{
	union map_entry *start;
	uint32_t count;
	struct wth_array *entries;

	if (i < WTH_SERVER_ID_START) {
		if (map->side == WTH_CONNECTION_SIDE_CLIENT)
			return -1;

		entries = &map->client_entries;
	} else {
		if (map->side == WTH_CONNECTION_SIDE_SERVER)
			return -1;

		entries = &map->server_entries;
		i -= WTH_SERVER_ID_START;
	}

	count = entries->size / sizeof *start;

	if (count < i)
		return -1;

	if (count == i) {
		wth_array_add(entries, sizeof *start);
		start = entries->data;
		start[i].data = NULL;
	} else {
		start = entries->data;
		if (start[i].data != NULL) {
			return -1;
		}
	}

	return 0;
}

WTH_EXPORT void
wth_map_remove(struct wth_map *map, uint32_t i)
{
	union map_entry *start;
	struct wth_array *entries;

	if (i < WTH_SERVER_ID_START) {
		if (map->side == WTH_CONNECTION_SIDE_SERVER)
			return;

		entries = &map->client_entries;
	} else {
		if (map->side == WTH_CONNECTION_SIDE_CLIENT)
			return;

		entries = &map->server_entries;
		i -= WTH_SERVER_ID_START;
	}

	start = entries->data;
	start[i].next = map->free_list;
	map->free_list = (i << 1) | 1;
}

WTH_EXPORT void *
wth_map_lookup(struct wth_map *map, uint32_t i)
{
	union map_entry *start;
	uint32_t count;
	struct wth_array *entries;

	if (i < WTH_SERVER_ID_START) {
		entries = &map->client_entries;
	} else {
		entries = &map->server_entries;
		i -= WTH_SERVER_ID_START;
	}

	start = entries->data;
	count = entries->size / sizeof *start;

	if (i < count && !map_entry_is_free(start[i]))
		return map_entry_get_data(start[i]);

	return NULL;
}

WTH_EXPORT uint32_t
wth_map_lookup_flags(struct wth_map *map, uint32_t i)
{
	union map_entry *start;
	uint32_t count;
	struct wth_array *entries;

	if (i < WTH_SERVER_ID_START) {
		entries = &map->client_entries;
	} else {
		entries = &map->server_entries;
		i -= WTH_SERVER_ID_START;
	}

	start = entries->data;
	count = entries->size / sizeof *start;

	if (i < count && !map_entry_is_free(start[i]))
		return map_entry_get_flags(start[i]);

	return 0;
}

static enum wth_iterator_result
for_each_helper(struct wth_array *entries, wth_iterator_func_t func, void *data)
{
	union map_entry *start, *end, *p;
	enum wth_iterator_result ret = WTH_ITERATOR_CONTINUE;

	start = entries->data;
	end = (union map_entry *) ((char *) entries->data + entries->size);

	for (p = start; p < end; p++)
		if (p->data && !map_entry_is_free(*p)) {
			ret = func(map_entry_get_data(*p), data);
			if (ret != WTH_ITERATOR_CONTINUE)
				break;
		}

	return ret;
}

WTH_EXPORT void
wth_map_for_each(struct wth_map *map, wth_iterator_func_t func, void *data)
{
	enum wth_iterator_result ret;

	ret = for_each_helper(&map->client_entries, func, data);
	if (ret == WTH_ITERATOR_CONTINUE)
		for_each_helper(&map->server_entries, func, data);
}
