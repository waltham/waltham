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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>

#include "waltham-message.h"

#include "log.h"
#include "util.h"

static gboolean logging_enabled = FALSE;
static int log_fd = -1;
static const gchar *log_file;

#if 0
void
log_message (TransportMsg *msg)
{
  GTimeVal now;
  gchar *time_str;

  gchar buffer[2 * MESSAGE_MAX_SIZE + 100];
  unsigned int i;
  gchar *p;
  const gchar *api = NULL;
  const gchar *name = NULL;

  if (!logging_enabled)
    return;

  switch (msg->header.api) {
    case API_CONTROL:
      api = "control";
      break;
    case API_TRANSFERS:
      api = "transfers";
      break;
    default:
      api = NULL;
  }

  p = buffer;

  g_get_current_time (&now);
  time_str = g_time_val_to_iso8601 (&now);
  p += sprintf (p, "%s ", time_str);
  g_free (time_str);

  for (i = 0; i < sizeof (hdr_t); i++)
    p += sprintf (p, "%02hhx", ((gchar *)&msg->header)[i]);

  for (i = 0; i < msg->header.sz - sizeof (hdr_t); i++)
    p += sprintf (p, "%02hhx", msg->body[i]);

  if (api && name) {
    p += sprintf (p, " %s%s\n", api, name);
  }

  p[0] = '\n';
  p++;

  i = write (log_fd, buffer, p - buffer);
}
#endif

static void
logging_set_enabled (gboolean enabled)
{
  if (logging_enabled == enabled)
    return;

  logging_enabled = enabled;

  if (logging_enabled)
    {
      const gchar *file;

      g_debug ("Enable message logging");

      file = g_getenv ("COLLABORA_DEBUG_FILE");

      if (!file)
        file = log_file;

      log_fd = open (file, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR);
    }
  else
    {
      g_debug ("Disable message logging");

      if (log_fd != -1)
        {
          close (log_fd);
          log_fd = -1;
        }
    }
}

static void
signal_handler (int signo)
{
  /* Only handle SIGUSR1 */
  if (signo != SIGUSR1)
    return;

  logging_set_enabled (!logging_enabled);
}

void
log_set_up (const gchar *default_log_file)
{
  const gchar *env;

  log_file = default_log_file;

  env = g_getenv ("COLLABORA_DEBUG");
  logging_set_enabled (env && !g_strcmp0 (env, "1"));

  if (signal (SIGUSR1, signal_handler) == SIG_ERR)
    g_error ("cannot install signal handler");
}
