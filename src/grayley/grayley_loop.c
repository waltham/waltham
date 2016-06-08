#include "grayley_loop.h"

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <glib-object.h>

#include "util/message.h"

enum {
   GLIB_BACKEND,
   WAYLAND_BACKEND,
   EPOLL_BACKEND
};

struct grayley_loop_source_t {
   int type;
   gpointer data;
};

struct grayley_loop_t {
   int type;
   gpointer data;
};

struct cb_data_t {
   grayley_loop_cb_t callback;
   gpointer data;
   uint32_t mask;
};

/* TODO Add support to remove sources and freeing callback data */

struct grayley_loop_t*
grayley_loop_new_from_glib (GMainLoop *loop)
{
   struct grayley_loop_t *ret;

   ret = calloc (1, sizeof (struct grayley_loop_t));
   if (!ret)
      return NULL;

   ret->type = GLIB_BACKEND;
   ret->data = loop;
   return ret;
}

struct grayley_loop_t*
grayley_loop_new_from_wayland (struct wl_event_loop *loop)
{
   struct grayley_loop_t *ret;

   ret = calloc (1, sizeof (struct grayley_loop_t));
   if (!ret)
      return NULL;

   ret->type = WAYLAND_BACKEND;
   ret->data = loop;
   return ret;
}

struct grayley_loop_t*
grayley_loop_new_from_epoll (EPollLoop  *loop)
{
   struct grayley_loop_t *ret;

   ret = calloc (1, sizeof (struct grayley_loop_t));
   if (!ret)
      return NULL;

   ret->type = EPOLL_BACKEND;
   ret->data = loop;
   return ret;
}

static gboolean
glib_fd_callback (GIOChannel *source, GIOCondition condition, gpointer data)
{
    struct cb_data_t *cb_data = (struct cb_data_t *) data;

    return cb_data->callback (cb_data->data);
}

static struct grayley_loop_source_t *
add_glib_fd (GMainLoop *loop, int fd, GIOCondition cond,
             grayley_loop_cb_t callback, gpointer data)
{
   struct grayley_loop_source_t *ret;
   struct cb_data_t *cb_data;
   GIOChannel *channel;
   GSource *source;

   cb_data = malloc (sizeof (struct cb_data_t));
   cb_data->callback = callback;
   cb_data->data = data;

   channel = g_io_channel_unix_new (fd);
   source = g_io_create_watch (channel, cond);
   g_source_set_callback (source, (GSourceFunc) glib_fd_callback, cb_data, NULL);
   g_source_attach (source, g_main_loop_get_context (loop));

   ret = calloc (1, sizeof (struct grayley_loop_source_t));
   if (!ret)
      return NULL;

   ret->type = GLIB_BACKEND;
   ret->data = source;

   return ret;
}

static int
wayland_fd_callback (int fd, uint32_t mask, void *data)
{
   struct cb_data_t *cb_data = (struct cb_data_t *) data;

   if ((mask & cb_data->mask) && cb_data->callback)
      return cb_data->callback (cb_data->data);

   return 1;
}

static struct grayley_loop_source_t *
add_wayland_fd (struct wl_event_loop *loop, int fd, GIOCondition cond,
                grayley_loop_cb_t callback, gpointer data)
{
   struct grayley_loop_source_t *ret;
   struct cb_data_t *cb_data;
   struct wl_event_source *source;
   uint32_t mask = 0;

   if (cond & G_IO_IN)
      mask |= WL_EVENT_READABLE;
   if (cond & G_IO_OUT)
      mask |= WL_EVENT_WRITABLE;

   cb_data = malloc (sizeof (struct cb_data_t));
   cb_data->callback = callback;
   cb_data->data = data;
   cb_data->mask = mask;

   source = wl_event_loop_add_fd (loop, fd, mask, wayland_fd_callback, cb_data);

   ret = calloc (1, sizeof (struct grayley_loop_source_t));
   if (!ret)
      return NULL;
   ret->type = WAYLAND_BACKEND;
   ret->data = source;

   return ret;
}

static gboolean
epoll_fd_callback (EPollLoop *loop, int fd, GIOCondition condition, gpointer data)
{
    struct cb_data_t *cb_data = (struct cb_data_t *) data;

    return cb_data->callback (cb_data->data);
}

static struct grayley_loop_source_t *
add_epoll_fd (EPollLoop *loop, int fd, GIOCondition cond,
                grayley_loop_cb_t callback, gpointer data)
{
   struct grayley_loop_source_t *ret;
   struct cb_data_t *cb_data;

   cb_data = malloc (sizeof (struct cb_data_t));
   cb_data->callback = callback;
   cb_data->data = data;

   epoll_loop_add_fd (loop, fd, cond, epoll_fd_callback, cb_data);

   ret = calloc (1, sizeof (struct grayley_loop_source_t));
   if (!ret)
      return NULL;
   ret->type = EPOLL_BACKEND;
   ret->data = GUINT_TO_POINTER (fd);

   return ret;
}

struct grayley_loop_source_t *
grayley_loop_add_fd (struct grayley_loop_t *loop, int fd, GIOCondition cond,
                     grayley_loop_cb_t callback, gpointer data)
{
   if (loop->type == GLIB_BACKEND) {
      return add_glib_fd ((GMainLoop *) loop->data, fd, cond, callback, data);
   } else if (loop->type == WAYLAND_BACKEND) {
      return add_wayland_fd ((struct wl_event_loop *) loop->data, fd, cond, callback, data);
   } else if (loop->type == EPOLL_BACKEND) {
      return add_epoll_fd ((EPollLoop *) loop->data, fd, cond, callback, data);
   } else {
      g_warning ("Invalid type %i for loop %p", loop->type, loop);
      return NULL;
   }
}
