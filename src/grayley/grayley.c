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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <string.h>
#include <stdio.h>

#include "waltham-message.h"
#include "util/log.h"

#include "grayley.h"
#include "grayley_platform.h"

//#define ENABLE_PROFILING 1

#ifdef ENABLE_PROFILING
#define START_TIMING() \
  gdouble msec; \
  GTimer *timer = g_timer_new ();
#define END_TIMING(label) \
  msec = g_timer_elapsed (timer, NULL) * 1000; \
  g_timer_destroy (timer); \
  log_debug ("%s (%f ms)", label, msec);
#else
#define START_TIMING()
#define END_TIMING(a)
#endif

#define PADDED(sz) \
   (((sz) + 3) & ~3)

void
grayley_init (struct grayley_loop_t *loop)
{
   if (getenv("COLLABORA_TEST") == NULL) {
      if (!grayley_platform_init(loop)) {
          g_critical("Grayley platform init failed");
          exit(1);
      }
   }

   grayley_platform_init_complete();
}

void
grayley_sync_upstream (void)
{
    grayley_platform_sync();
}

gboolean
grayley_dispatch (msg_t *msg,
                  hdr_t *header_reply, char *body_reply, data_t *data_reply)
{
   gboolean ret;

   switch (msg->hdr->api) {
      case API_CONTROL:
         if (msg->hdr->opcode == API_CONTROL_DISCONNECT_CLIENT) {
         } else {
            g_warning ("Invalid control message (opcode=%d), discard message",
                       msg->hdr->opcode);
         }
         return TRUE;
      case API_WAYLAND:
         if (msg->hdr->opcode > grayley_max_opcode
             || grayley_functions[msg->hdr->opcode] == NULL) {
            g_warning ("Invalid wayland opcode %d, discard message", msg->hdr->opcode);
            return TRUE;
         }
         ret = grayley_functions[msg->hdr->opcode](msg->hdr, msg->body,
                   header_reply, body_reply);
         if (!ret)
            return TRUE;
         break;
      default:
         g_warning ("Invalid api %d, discard message", msg->hdr->api);
         return TRUE;
   }

   return FALSE;
}
