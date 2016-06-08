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

#include <stdint.h>
#include <wayland-util.h>

struct wth_display;
struct wthp_blob_factory;
struct wthp_buffer;
struct wthp_compositor;
struct wthp_farstream;
struct wthp_keyboard;
struct wthp_output;
struct wthp_pointer;
struct wthp_region;
struct wthp_registry;
struct wthp_resource;
struct wthp_seat;
struct wthp_surface;
struct wthp_touch;

#define APICALL __attribute__ ((visibility ("default")))

void
wth_display_client_version (struct wth_display * wth_display, uint32_t client_version) APICALL;

struct wthp_callback *
wth_display_sync (struct wth_display * wth_display) APICALL;

struct wthp_registry *
wth_display_get_registry (struct wth_display * wth_display) APICALL;

struct wth_object *
wthp_registry_bind (struct wthp_registry * wthp_registry, uint32_t name) APICALL;

struct wthp_surface *
wthp_compositor_create_surface (struct wthp_compositor * wthp_compositor) APICALL;

struct wthp_region *
wthp_compositor_create_region (struct wthp_compositor * wthp_compositor) APICALL;

struct wthp_buffer *
wthp_blob_factory_create_buffer (struct wthp_blob_factory * wthp_blob_factory, uint32_t data_sz, void * data, int32_t width, int32_t height, int32_t stride, uint32_t format) APICALL;

void
wthp_buffer_destroy (struct wthp_buffer * wthp_buffer) APICALL;

void
wthp_surface_destroy (struct wthp_surface * wthp_surface) APICALL;

void
wthp_surface_attach (struct wthp_surface * wthp_surface, struct wthp_buffer * buffer, int32_t x, int32_t y) APICALL;

void
wthp_surface_damage (struct wthp_surface * wthp_surface, int32_t x, int32_t y, int32_t width, int32_t height) APICALL;

struct wthp_callback *
wthp_surface_frame (struct wthp_surface * wthp_surface) APICALL;

void
wthp_surface_set_opaque_region (struct wthp_surface * wthp_surface, struct wthp_region * region) APICALL;

void
wthp_surface_set_input_region (struct wthp_surface * wthp_surface, struct wthp_region * region) APICALL;

void
wthp_surface_commit (struct wthp_surface * wthp_surface) APICALL;

void
wthp_surface_set_buffer_transform (struct wthp_surface * wthp_surface, int32_t transform) APICALL;

void
wthp_surface_set_buffer_scale (struct wthp_surface * wthp_surface, int32_t scale) APICALL;

void
wthp_surface_damage_buffer (struct wthp_surface * wthp_surface, int32_t x, int32_t y, int32_t width, int32_t height) APICALL;

struct wthp_pointer *
wthp_seat_get_pointer (struct wthp_seat * wthp_seat) APICALL;

struct wthp_keyboard *
wthp_seat_get_keyboard (struct wthp_seat * wthp_seat) APICALL;

struct wthp_touch *
wthp_seat_get_touch (struct wthp_seat * wthp_seat) APICALL;

void
wthp_seat_release (struct wthp_seat * wthp_seat) APICALL;

void
wthp_pointer_set_cursor (struct wthp_pointer * wthp_pointer, uint32_t serial, struct wthp_surface * surface, int32_t hotspot_x, int32_t hotspot_y) APICALL;

void
wthp_pointer_release (struct wthp_pointer * wthp_pointer) APICALL;

void
wthp_keyboard_release (struct wthp_keyboard * wthp_keyboard) APICALL;

void
wthp_touch_release (struct wthp_touch * wthp_touch) APICALL;

void
wthp_region_destroy (struct wthp_region * wthp_region) APICALL;

void
wthp_region_add (struct wthp_region * wthp_region, int32_t x, int32_t y, int32_t width, int32_t height) APICALL;

void
wthp_region_subtract (struct wthp_region * wthp_region, int32_t x, int32_t y, int32_t width, int32_t height) APICALL;

void
wthp_farstream_destroy (struct wthp_farstream * wthp_farstream) APICALL;

struct wthp_farstream_remote *
wthp_farstream_connect (struct wthp_farstream * wthp_farstream, uint32_t port) APICALL;

void
wthp_farstream_remote_destroy (struct wthp_farstream_remote * wthp_farstream_remote) APICALL;

void
wthp_farstream_remote_codec_offer (struct wthp_farstream_remote * wthp_farstream_remote, char * list) APICALL;

struct wthp_buffer *
wthp_farstream_remote_create_buffer (struct wthp_farstream_remote * wthp_farstream_remote, uint32_t frame_id) APICALL;
