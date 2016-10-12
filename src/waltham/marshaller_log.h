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

#ifndef __MARSHALLER_LOG_H__
#define __MARSHALLER_LOG_H__

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* Comment/uncomment to disable/enable debugging log */
#define DEBUG
//#define PROFILE

#ifdef DEBUG
static inline void DEBUG_STAMP (void) {
   char str[200];
   time_t t;
   struct tm tm;

   t = time (NULL);
   localtime_r (&t, &tm);
   strftime (str, sizeof str, "%FT%TZ", &tm);
   printf ("%s ", str);
}

static inline void DEBUG_TYPE (const char *type) {
   printf (" %s\n", type);
}

static inline void STREAM_DEBUG( unsigned char *data, int sz, char *preamble ){
   int itr;
   unsigned char *p = data;
   for( itr = 0; itr < sz; itr++ ){
       printf( "%02x", p[itr] );
   }
}

static inline void STREAM_DEBUG_DATA (unsigned char *data, int sz) {
   printf (" [%i bytes] ", sz);
}
#else
#define STREAM_DEBUG(a, b, c)
#define STREAM_DEBUG_DATA(a, b)
#define DEBUG_STAMP()
#define DEBUG_TYPE(a)
#endif

#ifdef PROFILE

extern int timing_level;

static inline double diff(struct timespec *start, struct timespec *end)
{
   struct timespec temp;
   double ret;

   if ((end->tv_nsec - start->tv_nsec) < 0) {
      temp.tv_sec = end->tv_sec - start->tv_sec - 1;
      temp.tv_nsec = 1000000000 + end->tv_nsec - start->tv_nsec;
   } else {
      temp.tv_sec = end->tv_sec - start->tv_sec;
      temp.tv_nsec = end->tv_nsec - start->tv_nsec;
   }

   ret = (temp.tv_sec * 1000.0) + ((double) temp.tv_nsec) / 1000000.0;
   return ret;
}

#define START_TIMING(call, _fmt, ...) \
   struct timespec time1, time2; \
   double temp; \
   const char *_name = call; \
   timing_level++; \
   printf( "%s%s(" _fmt "): ", timing_level > 1 ? " " : "", _name, ##__VA_ARGS__); \
   clock_gettime(CLOCK_MONOTONIC, &time1);

#define END_TIMING(_fmt, ...) \
   clock_gettime(CLOCK_MONOTONIC, &time2); \
   temp = diff(&time1, &time2); \
   timing_level--; \
   printf(_fmt, ##__VA_ARGS__); \
   printf( "TOTAL (%f ms)\n", temp);

#define ABORT_TIMING(_fmt, ...) \
   clock_gettime(CLOCK_MONOTONIC, &time2); \
   temp = diff(&time1, &time2); \
   timing_level--; \
   printf(_fmt, ##__VA_ARGS__); \
   printf( "NOOP (%f ms)\n", temp);

#else

#define START_TIMING(a, fmt, ...)
#define END_TIMING(fmt, ...)
#define ABORT_TIMING(fmt, ...)

#endif

#endif
