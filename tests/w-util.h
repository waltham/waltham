/*
 * Copyright © 2008 Kristian Høgsberg
 * Copyright © 2016 DENSO CORPORATION
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

#ifndef W_UTIL_H
#define W_UTIL_H

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

/** \class wl_list
 *
 * \brief doubly-linked list
 *
 * The list head is of "struct wl_list" type, and must be initialized
 * using wl_list_init().  All entries in the list must be of the same
 * type.  The item type must have a "struct wl_list" member. This
 * member will be initialized by wl_list_insert(). There is no need to
 * call wl_list_init() on the individual item. To query if the list is
 * empty in O(1), use wl_list_empty().
 *
 * Let's call the list reference "struct wl_list foo_list", the item type as
 * "item_t", and the item member as "struct wl_list link".
 *
 * The following code will initialize a list:
 * \code
 * struct wl_list foo_list;
 *
 * struct item_t {
 * 	int foo;
 * 	struct wl_list link;
 * };
 * struct item_t item1, item2, item3;
 *
 * wl_list_init(&foo_list);
 * wl_list_insert(&foo_list, &item1.link);	// Pushes item1 at the head
 * wl_list_insert(&foo_list, &item2.link);	// Pushes item2 at the head
 * wl_list_insert(&item2.link, &item3.link);	// Pushes item3 after item2
 * \endcode
 *
 * The list now looks like [item2, item3, item1]
 *
 * Iterate the list in ascending order:
 * \code
 * item_t *item;
 * wl_list_for_each(item, foo_list, link) {
 * 	Do_something_with_item(item);
 * }
 * \endcode
 */
struct wl_list {
	struct wl_list *prev;
	struct wl_list *next;
};

void
wl_list_init(struct wl_list *list);

void
wl_list_insert(struct wl_list *list, struct wl_list *elm);

void
wl_list_remove(struct wl_list *elm);

int
wl_list_length(const struct wl_list *list);

int
wl_list_empty(const struct wl_list *list);

void
wl_list_insert_list(struct wl_list *list, struct wl_list *other);

/**
 * Retrieves a pointer to the containing struct of a given member item.
 *
 * This macro allows conversion from a pointer to a item to its containing
 * struct. This is useful if you have a contained item like a wl_list,
 * wl_listener, or wl_signal, provided via a callback or other means and would
 * like to retrieve the struct that contains it.
 *
 * To demonstrate, the following example retrieves a pointer to
 * `example_container` given only its `destroy_listener` member:
 *
 * \code
 * struct example_container {
 *     struct wl_listener destroy_listener;
 *     // other members...
 * };
 *
 * void example_container_destroy(struct wl_listener *listener, void *data)
 * {
 *     struct example_container *ctr;
 *
 *     ctr = wl_container_of(listener, ctr, destroy_listener);
 *     // destroy ctr...
 * }
 * \endcode
 *
 * \param ptr A valid pointer to the contained item.
 *
 * \param sample A pointer to the type of content that the list item
 * stores. Sample does not need be a valid pointer; a null or
 * an uninitialised pointer will suffice.
 *
 * \param member The named location of ptr within the sample type.
 *
 * \return The container for the specified pointer.
 */
#define wl_container_of(ptr, sample, member)				\
	(__typeof__(sample))((char *)(ptr) -				\
			     offsetof(__typeof__(*sample), member))
/* If the above macro causes problems on your compiler you might be
 * able to find an alternative name for the non-standard __typeof__
 * operator and add a special case here */

#define wl_list_for_each(pos, head, member)				\
	for (pos = wl_container_of((head)->next, pos, member);	\
	     &pos->member != (head);					\
	     pos = wl_container_of(pos->member.next, pos, member))

#define wl_list_for_each_safe(pos, tmp, head, member)			\
	for (pos = wl_container_of((head)->next, pos, member),		\
	     tmp = wl_container_of((pos)->member.next, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = wl_container_of(pos->member.next, tmp, member))

#define wl_list_for_each_reverse(pos, head, member)			\
	for (pos = wl_container_of((head)->prev, pos, member);	\
	     &pos->member != (head);					\
	     pos = wl_container_of(pos->member.prev, pos, member))

#define wl_list_for_each_reverse_safe(pos, tmp, head, member)		\
	for (pos = wl_container_of((head)->prev, pos, member),	\
	     tmp = wl_container_of((pos)->member.prev, tmp, member);	\
	     &pos->member != (head);					\
	     pos = tmp,							\
	     tmp = wl_container_of(pos->member.prev, tmp, member))

#define wl_list_first_until_empty(pos, head, member)			\
	while (!wl_list_empty(head) &&					\
		(pos = wl_container_of((head)->next, pos, member), 1))

#define wl_list_last_until_empty(pos, head, member)			\
	while (!wl_list_empty(head) &&					\
		(pos = wl_container_of((head)->prev, pos, member), 1))

static inline void *
zalloc(size_t size)
{
	return calloc(1, size);
}

#ifdef  __cplusplus
}
#endif

#endif
