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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_STDARG_H
# include <stdarg.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_TIME_H
# include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#include <Xm/XmAll.h>
#include <Xm/TransferT.h>
#include "globals.h"
#include "utf8.h"
#include "utils.h"

#define YYMMDD(tm) \
    (((tm)->tm_year << 16) | ((tm)->tm_mon << 8) | (tm)->tm_mday)

#if defined(DEBUG)
int verbosity = 1;
#endif /* DEBUG */

struct tm *
localtime_offset(time_t *when, int* utc_off)
{
#if DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF
    struct tm utc;
#endif /* DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF */
    struct tm *tm;
    int off;

    /* Many operating systems return utc_off in the struct tm, so
     * don't waste time computing that on those systems unless we're
     * in debug mode, in which case we want to check that our
     * calculations are correct. */
#if DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF
    /* Get the broken-down time in UTC. */
    tm = gmtime(when);
    memcpy(&utc, tm, sizeof(utc));
#endif /* DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF */

    /* Get the broken-down time in the local time zone. */
    tm = localtime(when);

#if DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF
    /* Local time can range from 14 hours ahead of GMT to 10 hours
     * after, so if the year, month or day differ, we'll just assume
     * it's the next or previous day.  We combine the bits of the
     * year, month and day so that we can do a single comparison. */
    off = (YYMMDD(&utc) < YYMMDD(tm)) ? 86400 :
        (YYMMDD(&utc) > YYMMDD(tm)) ? -86400 : 0;

    /* Adjust by the difference in hours. */
    off += (tm->tm_hour - utc.tm_hour) * 3600 +
        (tm->tm_min - utc.tm_min) * 60 +
        tm->tm_sec - utc.tm_sec;

    /* Record the offset to UTC. */
    *utc_off = off;
# if HAVE_STRUCT_TM_TM_GMTOFF
    ASSERT(off == tm->tm_gmtoff);
# endif /* HAVE_STRUCT_TM_TM_GMTOFF */

#else /* !DEBUG && HAVE_STRUCT_TM_TM_GMTOFF */
    *utc_off = tm->tm_gmtoff;
#endif /* DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF */
    return tm;
}

#if defined(DEBUG)
void
xdprintf(int level, const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vxdprintf(level, format, args);
    va_end(args);
}

void
vxdprintf(int level, const char *format, va_list args)
{
    char buffer[128];
    struct timeval tv;
    struct tm* tm;
    size_t len;

    /* Get the current time. */
    if (gettimeofday(&tv, NULL) < 0) {
        perror("gettimeofday failed");
        return;
    }

    /* Break it down. */
    tm = localtime(&tv.tv_sec);
    len = snprintf(buffer, sizeof(buffer),
                   "%s: %04d-%02d-%02dT%02d:%02d:%02d.%06ld: ", progname,
                   tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                   tm->tm_hour, tm->tm_min, tm->tm_sec, (long)tv.tv_usec);
    ASSERT(len < sizeof(buffer));

    if (level <= verbosity) {
        fwrite(buffer, len, 1, stderr);
        vfprintf(stderr, format, args);
    }
}
#endif /* DEBUG */

static int
do_convert(Widget widget, XmConvertCallbackStruct *data,
           message_t message, message_part_t part,
           XtPointer *value_out, Atom *type_out, size_t *length_out,
           int *format_out)
{
    Atom *targets;
    char *utf8;
    size_t len;
    int i;

    /* If the destination has requested the list of targets then we
     * return this as the value. */
    if (data->target == atoms[AN_TARGETS]) {
        /* We support all of the standard Motif targets. */
        targets = XmeStandardTargets(widget, 2, &i);
        if (targets == NULL) {
            perror("XmeStandardTargets failed");
            return -1;
        }

        /* We also support some textual types. */
        targets[i++] = XA_STRING;
        targets[i++] = atoms[AN_UTF8_STRING];
        *value_out = targets;
        *type_out = XA_ATOM;
        *length_out = i;
        *format_out = 32;
        return 0;
    }

    /* If the destination has requested a set of Motif clipboard
     * targets, then provide the supported string types. */
    if (data->target == atoms[AN__MOTIF_CLIPBOARD_TARGETS]) {
        targets = (Atom *)XtMalloc(2 * sizeof(Atom));
        if (targets == NULL) {
            perror("XtMalloc failed");
            return -1;
        }

        i = 0;
        targets[i++] = XA_STRING;
        targets[i++] = atoms[AN_UTF8_STRING];
        *value_out = targets;
        *type_out = XA_ATOM;
        *length_out = i;
        *format_out = 32;
        return 0;
    }

    /* String conversions. */
    if (data->target == XA_STRING || data->target == atoms[AN_UTF8_STRING]) {
        /* Get the UTF-8 version of the message string. */
        len = message_part_size(message, part);
        utf8 = XtMalloc(len + 1);
        if (utf8 == NULL) {
            perror("XtMalloc failed");
            return -1;
        }
        message_get_part(message, part, utf8, len);
        utf8[len] = '\0';

        /* Convert if necessary. */
        if (data->target == atoms[AN_UTF8_STRING]) {
            *value_out = utf8;
        } else {
            *value_out = utf8_to_target(utf8, data->target, &len);
            XtFree(utf8);
        }
        *type_out = data->target;
        *length_out = len;
        *format_out = 8;
        return 0;
    }

    /* Standard conversions. */
    XmeStandardConvert(widget, NULL, data);
    return 0;
}

void
message_convert(Widget widget, XtPointer *call_data,
                message_t message, message_part_t part)
{
    XmConvertCallbackStruct *data = (XmConvertCallbackStruct *)call_data;
    XtPointer value;
    Atom type;
    size_t len;
    int format;

    /* Make sure there's a message and part to copy. */
    ASSERT(message != NULL);
    ASSERT(part != MSGPART_NONE);

    /* Do the widget-specific conversion. */
    if (do_convert(widget, data, message, part,
                   &value, &type, &len, &format) < 0) {
        data->status = XmCONVERT_REFUSE;
        return;
    }

    /* If the application has supplied some data already then merge
     * with that. */
    if (data->status == XmCONVERT_MERGE) {
        XmeConvertMerge(value, type, format, len, data);
        return;
    }

    /* Otherwise use the conversion information. */
    data->status = XmCONVERT_DONE;
    data->value = value;
    data->type = type;
    data->format = format;
    data->length = len;
}

/**********************************************************************/
