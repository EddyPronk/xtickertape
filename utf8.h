/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2001.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef UTF8_H
#define UTF8_H

#include <Xm/XmAll.h>

#ifndef lint
static const char cvs_UTF8_H[] = "$Id: utf8.h,v 1.2 2003/01/14 17:01:34 phelps Exp $";
#endif /* lint */

/* String measurements */
typedef struct string_sizes *string_sizes_t;

/* Use for string measurement results */
struct string_sizes
{
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
utf8_renderer_t utf8_renderer_alloc(
    Display *display,
    XFontStruct *font,
    const char *code_set);

/* Releases the resources allocated by a utf8_renderer_t */
void utf8_renderer_free(utf8_renderer_t self);

/* Measures all of the characters in a string */
void utf8_renderer_measure_string(
    utf8_renderer_t self,
    char *string,
    string_sizes_t sizes);

/* Draw a string within the bounding box, measuring the characters so
 * as to minimize bandwidth requirements */
void utf8_renderer_draw_string(
    Display *display,
    Drawable drawable,
    GC gc,
    utf8_renderer_t renderer,
    int x, int y,
    XRectangle *bbox,
    char *string);

/* Draw an underline under a string */
void utf8_renderer_draw_underline(
    utf8_renderer_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    int x, int y,
    XRectangle *bbox,
    long width);

typedef struct utf8_encoder *utf8_encoder_t;

/* Returns a utf8_encoder */
utf8_encoder_t utf8_encoder_alloc(
    Display *display,
    XmFontList font_list,
    char *code_set);

/* Releases the resources allocated by a utf8_encoder_t */
void utf8_encoder_free(utf8_encoder_t self);

/* Encodes a string */
char *utf8_encoder_encode(utf8_encoder_t self, char *input);

#endif /* MESSAGE_VIEW_H */
