/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

/*
 * This file contains macros and functions which either aren't
 * associated with a specific data structure.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <X11/Intrinsic.h>
#include "message.h"

/* Useful macros */
#if !defined(MIN)
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#if !defined(MAX)
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/* Transform a time_t into a broken-down time in the local time zone,
 * like localtime.  Also compute the determine the number of seconds
 * east of (before) UTC that time zone is/was/will be at that time. */
struct tm *
localtime_offset(time_t *when, int* utc_off);


/* Handle the conversion of a selected message to various clipboard
 * types. */
void
message_convert(Widget widget, XtPointer *call_data,
                message_t message, message_part_t part);



/* Print debug messages if so configured. */
#if defined(DEBUG)
# define DPRINTF(x) xdprintf x
# define VDPRINTF(x) vxdprintf x

/* The level of debug message verbosity. */
extern int verbosity;

/* Print debug messages */
void
xdprintf(int level, const char *format, ...);

/* Print debug messages */
void
vxdprintf(int level, const char *format, va_list args);
#else /* !DEBUG */
# define DPRINTF(x) do {} while (0)
# define VDPRINTF(x) do {} while (0)
#endif /* DEBUG */

/* Simple version of basename without all of the baggage of the original. */
const char *
xbasename(const char *path);

#endif /* UTILS_H */
