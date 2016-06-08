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

/*
 * This file contains the request handlers for a Waltham server.
 *
 * They are confusingly named because the functions are called
 * like this in the client side, and Waltham message dispatcher just
 * calls functions of the same name on the server side.
 *
 * The naming will be fixed once incoming message handlers are
 * dynamically set with listener structs (vfunc tables).
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <wayland-client.h>

void
wth_display_client_version (struct wth_display * wth_display, uint32_t client_version)
{
  printf ("%s\n", __func__);
}

struct wthp_callback *
wth_display_sync (struct wth_display * wth_display)
{
  printf ("%s\n", __func__);
}

struct wthp_registry *
wth_display_get_registry (struct wth_display * wth_display)
{
  unsigned char keymap[] = { 1, 2, 3, 4, 5, 6, };
  //struct wl_array keys = { , , };

  printf ("%s\n", __func__);

  wthp_registry_send_global_remove (wth_display, 5);
  wthp_registry_send_global_remove (wth_display, 5);

  wthp_registry_send_global (wth_display, 5, "wthp_seat", 8);
  wthp_registry_send_global (wth_display, 5, "wthp_seat", 8);

  //wthp_keyboard_send_enter (wth_display, 7, surface, &array);

  wthp_keyboard_send_keymap (wth_display, 9, sizeof (keymap), keymap);
  wthp_keyboard_send_keymap (wth_display, 9, sizeof (keymap), keymap);
}

struct wthp_registry *
wthp_registry_bind (struct wthp_registry * wthp_registry, uint32_t name)
{
  printf ("%s\n", __func__);
}

struct wthp_surface *
wthp_compositor_create_surface (struct wthp_compositor * wthp_compositor)
{
  printf ("%s\n", __func__);
}

struct wthp_region *
wthp_compositor_create_region (struct wthp_compositor * wthp_compositor)
{
  printf ("%s\n", __func__);
}

void
wthp_blob_factory_create_buffer (struct wthp_blob_factory * wthp_blob_factory, struct wthp_buffer * new_buffer, uint32_t data_sz, void * data, int32_t width, int32_t height, int32_t stride, uint32_t format)
{
  printf ("%s\n", __func__);

  unsigned char *buf = data;

  assert (wthp_blob_factory != NULL);
  assert (new_buffer != NULL);
  assert (data_sz == 10);
  assert (buf[0] == 0x10 && buf[1] == 0x11 && buf[2] == 0x12 && buf[3] == 0x13 && buf[4] == 0x14 && buf[5] == 0x15 && buf[6] == 0x16 && buf[7] == 0x17 && buf[8] == 0x18 && buf[9] == 0x19);
  assert (width == 30);
  assert (height == 40);
  assert (stride == 30);
  assert (format == 10);
}

void
wthp_buffer_destroy (struct wthp_buffer * wthp_buffer)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_destroy (struct wthp_surface * wthp_surface)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_attach (struct wthp_surface * wthp_surface, struct wthp_buffer * buffer, int32_t x, int32_t y)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_damage (struct wthp_surface * wthp_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
  printf ("%s\n", __func__);
}

struct wthp_callback *
wthp_surface_frame (struct wthp_surface * wthp_surface)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_set_opaque_region (struct wthp_surface * wthp_surface, struct wthp_region * region)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_set_input_region (struct wthp_surface * wthp_surface, struct wthp_region * region)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_commit (struct wthp_surface * wthp_surface)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_set_buffer_transform (struct wthp_surface * wthp_surface, int32_t transform)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_set_buffer_scale (struct wthp_surface * wthp_surface, int32_t scale)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_damage_buffer (struct wthp_surface * wthp_surface, int32_t x, int32_t y, int32_t width, int32_t height)
{
  printf ("%s\n", __func__);
}

struct wthp_pointer *
wthp_seat_get_pointer (struct wthp_seat * wthp_seat)
{
  printf ("%s\n", __func__);
}

struct wthp_keyboard *
wthp_seat_get_keyboard (struct wthp_seat * wthp_seat)
{
  printf ("%s\n", __func__);
}

struct wthp_touch *
wthp_seat_get_touch (struct wthp_seat * wthp_seat)
{
  printf ("%s\n", __func__);
}

void
wthp_seat_release (struct wthp_seat * wthp_seat)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_set_cursor (struct wthp_pointer * wthp_pointer, uint32_t serial, struct wthp_surface * surface, int32_t hotspot_x, int32_t hotspot_y)
{
  printf ("%s\n", __func__);

  assert (wthp_pointer != NULL);
  assert (serial == 1);
  assert (surface != NULL);
  assert (hotspot_x == 3);
  assert (hotspot_y == 4);
}

void
wthp_pointer_release (struct wthp_pointer * wthp_pointer)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_release (struct wthp_keyboard * wthp_keyboard)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_release (struct wthp_touch * wthp_touch)
{
  printf ("%s\n", __func__);
}

void
wthp_region_destroy (struct wthp_region * wthp_region)
{
  printf ("%s\n", __func__);
}

void
wthp_region_add (struct wthp_region * wthp_region, int32_t x, int32_t y, int32_t width, int32_t height)
{
  printf ("%s\n", __func__);
}

void
wthp_region_subtract (struct wthp_region * wthp_region, int32_t x, int32_t y, int32_t width, int32_t height)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_destroy (struct wthp_farstream * wthp_farstream)
{
  printf ("%s\n", __func__);
}

struct wthp_farstream_remote *
wthp_farstream_connect (struct wthp_farstream * wthp_farstream, uint32_t port)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_remote_destroy (struct wthp_farstream_remote * wthp_farstream_remote)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_remote_codec_offer (struct wthp_farstream_remote * wthp_farstream_remote, char * list)
{
  printf ("%s\n", __func__);
}

struct wthp_buffer *
wthp_farstream_remote_create_buffer (struct wthp_farstream_remote * wthp_farstream_remote, uint32_t frame_id)
{
  printf ("%s\n", __func__);
}
