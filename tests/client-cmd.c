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
 * This file contains the event handlers for a Waltham client.
 *
 * They are confusingly named as "send" because the functions are called
 * like this in the server side, and Waltham message dispatcher just
 * calls functions of the same name on the client side.
 *
 * The naming will be fixed once incoming message handlers are
 * dynamically set with listener structs (vfunc tables).
 */

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

#include <wayland-client.h>

void
wth_display_send_error (struct wthp_resource * resource_, struct wthp_resource * object_id, uint32_t code, char * message)
{
  printf ("%s\n", __func__);
}

void
wth_display_send_delete_id (struct wthp_resource * resource_, uint32_t id)
{
  printf ("%s\n", __func__);
}

void
wth_display_send_server_version (struct wthp_resource * resource_, uint32_t server_version)
{
  printf ("%s\n", __func__);
}

void
wthp_registry_send_global (struct wthp_resource * resource_, uint32_t name, char * interface, uint32_t version)
{
  printf ("%s\n", __func__);

  assert (name == 5);
  assert (strcmp (interface, "wthp_seat") == 0);
  assert (version == 8);
}

void
wthp_registry_send_global_remove (struct wthp_resource * resource_, uint32_t name)
{
  printf ("%s\n", __func__);
}

void
wthp_callback_send_done (struct wthp_resource * resource_, uint32_t callback_data)
{
  printf ("%s\n", __func__);
}

void
wthp_blob_factory_send_format (struct wthp_resource * resource_, uint32_t format)
{
  printf ("%s\n", __func__);
}

void
wthp_buffer_send_complete (struct wthp_resource * resource_, uint32_t serial)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_send_enter (struct wthp_resource * resource_, struct wthp_output * output)
{
  printf ("%s\n", __func__);
}

void
wthp_surface_send_leave (struct wthp_resource * resource_, struct wthp_output * output)
{
  printf ("%s\n", __func__);
}

void
wthp_seat_send_capabilities (struct wthp_resource * resource_, uint32_t capabilities)
{
  printf ("%s\n", __func__);
}

void
wthp_seat_send_name (struct wthp_resource * resource_, char * name)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_enter (struct wthp_resource * resource_, uint32_t serial, struct wthp_surface * surface, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_leave (struct wthp_resource * resource_, uint32_t serial, struct wthp_surface * surface)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_motion (struct wthp_resource * resource_, uint32_t time, wl_fixed_t surface_x, wl_fixed_t surface_y)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_button (struct wthp_resource * resource_, uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_axis (struct wthp_resource * resource_, uint32_t time, uint32_t axis, wl_fixed_t value)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_frame (struct wthp_resource * resource_)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_axis_source (struct wthp_resource * resource_, uint32_t axis_source)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_axis_stop (struct wthp_resource * resource_, uint32_t time, uint32_t axis)
{
  printf ("%s\n", __func__);
}

void
wthp_pointer_send_axis_discrete (struct wthp_resource * resource_, uint32_t axis, int32_t discrete)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_send_keymap (struct wthp_resource * resource_, uint32_t format, uint32_t keymap_sz, void * keymap)
{
  printf ("%s\n", __func__);

  unsigned char *buf = keymap;

  assert (format == 9);
  assert (keymap_sz == 6);
  assert (buf[0] == 1 && buf[1] == 2 && buf[2] == 3 && buf[3] == 4 && buf[4] == 5 && buf[5] == 6);
}

void
wthp_keyboard_send_enter (struct wthp_resource * resource_, uint32_t serial, struct wthp_surface * surface, struct wl_array * keys)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_send_leave (struct wthp_resource * resource_, uint32_t serial, struct wthp_surface * surface)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_send_key (struct wthp_resource * resource_, uint32_t serial, uint32_t time, uint32_t key, uint32_t state)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_send_modifiers (struct wthp_resource * resource_, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group)
{
  printf ("%s\n", __func__);
}

void
wthp_keyboard_send_repeat_info (struct wthp_resource * resource_, int32_t rate, int32_t delay)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_send_down (struct wthp_resource * resource_, uint32_t serial, uint32_t time, struct wthp_surface * surface, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_send_up (struct wthp_resource * resource_, uint32_t serial, uint32_t time, int32_t id)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_send_motion (struct wthp_resource * resource_, uint32_t time, int32_t id, wl_fixed_t x, wl_fixed_t y)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_send_frame (struct wthp_resource * resource_)
{
  printf ("%s\n", __func__);
}

void
wthp_touch_send_cancel (struct wthp_resource * resource_)
{
  printf ("%s\n", __func__);
}

void
wthp_output_send_geometry (struct wthp_resource * resource_, int32_t x, int32_t y, int32_t physical_width, int32_t physical_height, int32_t subpixel, char * make, char * model, int32_t transform)
{
  printf ("%s\n", __func__);
}

void
wthp_output_send_mode (struct wthp_resource * resource_, uint32_t flags, int32_t width, int32_t height, int32_t refresh)
{
  printf ("%s\n", __func__);
}

void
wthp_output_send_done (struct wthp_resource * resource_)
{
  printf ("%s\n", __func__);
}

void
wthp_output_send_scale (struct wthp_resource * resource_, int32_t factor)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_remote_send_port (struct wthp_resource * resource_, uint32_t port)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_remote_send_codec_answer (struct wthp_resource * resource_, char * list)
{
  printf ("%s\n", __func__);
}

void
wthp_farstream_remote_send_error (struct wthp_resource * resource_, uint32_t code, char * message)
{
  printf ("%s\n", __func__);
}
