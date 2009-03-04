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
#include <stdio.h> /* snprintf, vsprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* NULL */
#endif
#ifdef HAVE_CTYPE_H
# include <ctype.h> /* toupper */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* memcpy */
#endif
#include "replace.h"

/* Replacements for standard functions */


#ifndef HAVE_MEMSET
/* A slow but correct implementation of memset */
void *
memset(void *s, int c, size_t n)
{
    char *point = (char *)s;
    char *end = point + n;

    while (point < end) {
        *(point++) = c;
    }

    return s;
}
#endif

#ifndef HAVE_SNPRINTF
/* A dodgy hack to replace a real snprintf implementation */
# include <stdarg.h>

int
snprintf(char *s, size_t n, const char *format, ...)
{
    va_list ap;
    int result;

    va_start(ap, format);
    result = vsprintf(s, format, ap);
    va_end(ap);

    return result;
}

#endif

#ifndef HAVE_STRCASECMP
/* A slow but correct implementation of strcasecmp */
int
strcasecmp(const char *s1, const char *s2)
{
    size_t i = 0;
    int c1, c2;

    while (1) {
        c1 = toupper(s1[i]);
        c2 = toupper(s2[i]);

        if (c1 == 0) {
            if (c2 == 0) {
                return 0;
            } else {
                return -1;
            }
        }

        if (c2 == 0) {
            return 1;
        }

        if (c1 < c2) {
            return -1;
        }

        if (c1 > c2) {
            return 1;
        }

        i++;
    }
}
#endif


#ifndef HAVE_STRCHR
/* A slow but correct implementation of strchr */
char *
strchr(const char *s, int c)
{
    size_t i = 0;
    int ch;

    while ((ch = s[i]) != 0) {
        if (ch == c) {
            return s + i;
        }

        i++;
    }

    return NULL;
}
#endif

#ifndef HAVE_STRDUP
/* A slow but correct implementation of strdup */
char *
strdup(const char *s)
{
    size_t length;
    char *result;

    length = strlen(s) + 1;
    result = malloc(length);
    if (result == NULL) {
        return NULL;
    }

    memcpy(result, s, length);
    return result;
}
#endif

#ifndef HAVE_STRRCHR
/* A slow but correct implementation of strrchr */
char *
strrchr(const char *s, int c)
{
    char *result = NULL;
    int ch;

    while ((ch = *s) != 0) {
        if (ch == c) {
            result = (char *)s;
        }

        s++;
    }

    return result;
}
#endif

#ifndef HAVE_STRERROR
# define BUFFER_SIZE 32
static char buffer[BUFFER_SIZE];
char *
strerror(int errnum)
{
    snprintf(buffer, BUFFER_SIZE, "errno=%d\n", errnum);
    return buffer;
}
#endif
