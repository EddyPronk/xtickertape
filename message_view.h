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

#ifndef MESSAGE_VIEW_H
#define MESSAGE_VIEW_H

#ifndef lint
static const char cvs_MESSAGE_VIEW_H[] = "$Id: message_view.h,v 2.7 2003/01/10 11:57:24 phelps Exp $";
#endif /* lint */

/* String measurements */
typedef struct string_sizes *string_sizes_t;

/* The code_set_info type */
typedef struct code_set_info *code_set_info_t;

/* The message_view type */
typedef struct message_view *message_view_t;

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

/* Returns an iconv conversion descriptor for converting characters to
 * be displayed in a given font from a given code set.  If tocode is
 * non-NULL then it will be tried first. */
code_set_info_t code_set_info_alloc(
    Display *display,
    XFontStruct *font,
    const char *tocode);

/* Releases the resources allocated by a code_set_info_t */
void code_set_info_free(code_set_info_t self);

/* Allocates and initializes a new message_view_t */
message_view_t message_view_alloc(
    message_t message,
    long indent,
    code_set_info_t cs_info);

/* Frees a message_view_t */
void message_view_free(message_view_t self);

/* Returns the message view's message */
message_t message_view_get_message(message_view_t self);

/* Returns the sizes of the message view */
void message_view_get_sizes(
    message_view_t self,
    int show_timestamp,
    string_sizes_t sizes_out);

/* Draws the message_view */
void message_view_paint(
    message_view_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    int show_timestamp,
    unsigned long timestamp_pixel,
    unsigned long group_pixel,
    unsigned long user_pixel,
    unsigned long message_pixel,
    unsigned long separator_pixel,
    long x, long y,
    XRectangle *bbox);
#endif /* MESSAGE_VIEW_H */
