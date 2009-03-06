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

#ifndef UTF8_H
#define UTF8_H

#include <Xm/XmAll.h>

/* Make sure that ICONV_CONST is defined */
#ifndef ICONV_CONST
# define ICONV_CONST
#endif

/* String measurements */
typedef struct string_sizes *string_sizes_t;

/* Use for string measurement results */
struct string_sizes {
    /* The distance from the origin to the left edge of the string */
    long lbearing;

    /* The distance from the origin to the right edge of the string */
    long rbearing;

    /* The distance from the origin to the next string's origin */
    long width;

    /* The distance from the baseline to the top of the string */
    short ascent;

    /* The distance from the baseline to the bottom of the string */
    short descent;
};

/* The utf8_renderer type */
typedef struct utf8_renderer *utf8_renderer_t;

/* Returns a utf8_renderer which can be used to render UTF-8
 * characters in the given font.  If code_set is NULL then the
 * renderer will attempt to guess it from the font's properties. */
utf8_renderer_t
utf8_renderer_alloc(Display *display, XFontStruct *font, const char *code_set);


/* Releases the resources allocated by a utf8_renderer_t */
void
utf8_renderer_free(utf8_renderer_t self);


/* Measures all of the characters in a string */
void
utf8_renderer_measure_string(utf8_renderer_t self,
                             const char *string,
                             string_sizes_t sizes);


/* Draw a string within the bounding box, measuring the characters so
 * as to minimize bandwidth requirements */
void
utf8_renderer_draw_string(Display *display,
                          Drawable drawable,
                          GC gc,
                          utf8_renderer_t renderer,
                          int x,
                          int y,
                          XRectangle *bbox,
                          const char *string);


/* Draw an underline under a string */
void
utf8_renderer_draw_underline(utf8_renderer_t self,
                             Display *display,
                             Drawable drawable,
                             GC gc,
                             int x,
                             int y,
                             XRectangle *bbox,
                             long width);


typedef struct utf8_encoder *utf8_encoder_t;

/* Returns a utf8_encoder */
utf8_encoder_t
utf8_encoder_alloc(Display *display, XmFontList font_list,
                   const char *code_set);


/* Releases the resources allocated by a utf8_encoder_t */
void
utf8_encoder_free(utf8_encoder_t self);


/* Encodes a string */
char *
utf8_encoder_encode(utf8_encoder_t self, const char *input);


/* Decodes a string */
char *
utf8_encoder_decode(utf8_encoder_t self, const char *input);


#endif /* MESSAGE_VIEW_H */
