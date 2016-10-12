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

/** \file waltham-util.h
 *
 * \brief Utility classes, functions, and macros.
 */

#ifndef WALTHAM_UTIL_H
#define WALTHAM_UTIL_H

#include <math.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdarg.h>

#ifdef  __cplusplus
extern "C" {
#endif

/* GCC visibility */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WTH_EXPORT __attribute__ ((visibility("default")))
#else
#define WTH_EXPORT
#endif

/* Deprecated attribute */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WTH_DEPRECATED __attribute__ ((deprecated))
#else
#define WTH_DEPRECATED
#endif

/* Printf annotation */
#if defined(__GNUC__) && __GNUC__ >= 4
#define WTH_PRINTF(x, y) __attribute__((__format__(__printf__, x, y)))
#else
#define WTH_PRINTF(x, y)
#endif

struct wth_array {
	size_t size;
	size_t alloc;
	void *data;
};

#define wth_array_for_each(pos, array)					\
	for (pos = (array)->data;					\
	     (const char *) pos < ((const char *) (array)->data + (array)->size); \
	     (pos)++)

void
wth_array_init(struct wth_array *array);

void
wth_array_release(struct wth_array *array);

void *
wth_array_add(struct wth_array *array, size_t size);

int
wth_array_copy(struct wth_array *array, struct wth_array *source);

typedef int32_t wth_fixed_t;

static inline double
wth_fixed_to_double (wth_fixed_t f)
{
	union {
		double d;
		int64_t i;
	} u;

	u.i = ((1023LL + 44LL) << 52) + (1LL << 51) + f;

	return u.d - (3LL << 43);
}

static inline wth_fixed_t
wth_fixed_from_double(double d)
{
	union {
		double d;
		int64_t i;
	} u;

	u.d = d + (3LL << (51 - 8));

	return u.i;
}

static inline int
wth_fixed_to_int(wth_fixed_t f)
{
	return f / 256;
}

static inline wth_fixed_t
wth_fixed_from_int(int i)
{
	return i * 256;
}

/** \enum wth_iterator_result
 *
 * This enum represents the return value of an iterator function.
 */
enum wth_iterator_result {
	/** Stop the iteration */
	WTH_ITERATOR_STOP,
	/** Continue the iteration */
	WTH_ITERATOR_CONTINUE
};

#ifdef  __cplusplus
}
#endif

#endif
