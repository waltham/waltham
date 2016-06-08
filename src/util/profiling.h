/*
 * Copyright © 2013-2014 Collabora, Ltd.
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

#ifndef __UTIL_PROFILING_H__
#define __UTIL_PROFILING_H__

/* Comment/uncomment this line to disable/enable profiling */
//#define PROFILE 1

#ifdef PROFILE

#define START_PROFILER() \
  GTimer *timer = g_timer_new (); \
  gdouble msec;

#define RESET_PROFILER(label) \
  msec = g_timer_elapsed (timer, NULL) * 1000; \
  g_debug ("%f milliseconds to " label, msec); \
  g_timer_reset (timer); \

#define STOP_PROFILER(label) \
  g_timer_stop (timer); \
  msec = g_timer_elapsed (timer, NULL) * 1000; \
  g_debug ("%f milliseconds to " label, msec);

#define CONTINUE_PROFILER() \
  g_timer_continue (timer);

#define DESTROY_PROFILER() \
  g_timer_destroy (timer); \
  timer = NULL;

#define END_PROFILER(label) \
  STOP_PROFILER (label); \
  DESTROY_PROFILER ();

#else

#define START_PROFILER()
#define STOP_PROFILER(label)
#define RESET_PROFILER(label)
#define CONTINUE_PROFILER()
#define DESTROY_PROFILER()
#define END_PROFILER(label)

#endif

#endif
