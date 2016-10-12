/*
 * Copyright © 2008 Kristian Høgsberg
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

/** \file wayland-util.h
 *
 * \brief Utility classes, functions, and macros.
 */

#ifndef WAYLAND_UTIL_H
#define WAYLAND_UTIL_H

#include <math.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_EXPORT __attribute__ ((visibility("default")))
#else
#define WL_EXPORT
#endif

/* Deprecated attribute */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_DEPRECATED __attribute__ ((deprecated))
#else
#define WL_DEPRECATED
#endif

/* Printf annotation */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WL_PRINTF(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define WL_PRINTF(x, y)
#endif

struct wth_array {
	size_t size;
	size_t alloc;
	void *data;
};

#define wl_array_for_each(pos, array)					\
	for (pos = (array)->data;					\
	     (const char *) pos < ((const char *) (array)->data + (array)->size); \
	     (pos)++)

void
wl_array_init(struct wl_array *array);

void
wl_array_release(struct wl_array *array);

void *
wl_array_add(struct wl_array *array, size_t size);

int
wl_array_copy(struct wl_array *array, struct wl_array *source);

typedef int32_t wl_fixed_t;

static inline double
wl_fixed_to_double (wl_fixed_t f)
{
	union {
		double d;
		int64_t i;
	} u;

	u.i = ((1023LL + 44LL) << 52) + (1LL << 51) + f;

	return u.d - (3LL << 43);
}

static inline wl_fixed_t
wl_fixed_from_double(double d)
{
	union {
		double d;
		int64_t i;
	} u;

	u.d = d + (3LL << (51 - 8));

	return u.i;
}

static inline int
wl_fixed_to_int(wl_fixed_t f)
{
	return f / 256;
}

static inline wl_fixed_t
wl_fixed_from_int(int i)
{
	return i * 256;
}

/** \enum wl_iterator_result
 *
 * This enum represents the return value of an iterator function.
 */
enum wl_iterator_result {
	/** Stop the iteration */
	WL_ITERATOR_STOP,
	/** Continue the iteration */
	WL_ITERATOR_CONTINUE
};

#ifdef  __cplusplus
}
#endif

#endif
