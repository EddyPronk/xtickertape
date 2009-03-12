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

#define YYMMDD(tm) \
    (((tm)->tm_year << 16) | ((tm)->tm_mon << 8) | (tm)->tm_mday)

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
    assert(off == tm->tm_gmtoff);
# endif /* HAVE_STRUCT_TM_TM_GMTOFF */

#else /* !DEBUG && HAVE_STRUCT_TM_TM_GMTOFF */
    *utc_off = tm->tm_gmtoff;
#endif /* DEBUG || !HAVE_STRUCT_TM_TM_GMTOFF */
    return tm;
}

/**********************************************************************/
