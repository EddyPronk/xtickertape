/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
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

#ifndef GLYPH_H
#define GLYPH_H

#ifndef lint
static const char cvs_GLYPH_H[] = "$Id: glyph.h,v 1.3 1999/08/17 17:59:49 phelps Exp $";
#endif /* lint */

typedef struct glyph *glyph_t;

#include "Scroller.h"
#include "Message.h"

/*
 * The signature of the function which allocates another reference to
 * the glyph. 
 */
typedef glyph_t (*alloc_method_t)(glyph_t glyph);

/*
 * The signature of the function which frees the resources consumed by
 * the glyph.
 */
typedef void (*free_method_t)(glyph_t glyph);

/*
 * The signature of a function which returns the Message represented
 * by the glyph.
 */
typedef Message (*message_method_t)(glyph_t glyph);

/*
 * The signature of a function which computes and returns the glyph's
 * width.  This will be called frequently and should be reasonably
 * efficient.
 */
typedef unsigned int (*width_method_t)(glyph_t glyph);

/*
 * The signature of a function which paints the glyph with width w
 * onto the given drawable at the given offset.  Any portion of the
 * glyph outside the bounding box of [x, y, width, height] need not be
 * drawn 
 */
typedef void (*paint_method_t)(
    glyph_t glyph, Display *display, Drawable drawable,
    int offset, int w, int x, int y, unsigned int width, unsigned int height);

/*
 * The signature of a function which answers True if the glyph has
 * expired.
 */
typedef int (*is_expired_method_t)(glyph_t glyph);

/*
 * The signature of a function which causes the given glyph to expire
 * in a graceful manner, preferably without causing a discontinuity in 
 * the scrolling
 */
typedef void (*expire_method_t)(glyph_t glyph);

/* The common fields shared by all glyphs */
#define GLYPH_PREFIX \
    glyph_t previous; \
    glyph_t next; \
    alloc_method_t alloc; \
    free_method_t free; \
    message_method_t get_message; \
    width_method_t get_width; \
    paint_method_t paint; \
    is_expired_method_t is_expired; \
    expire_method_t expire;

/* The basic glyph structure */
struct glyph
{
    GLYPH_PREFIX
};


/* Allocates and initializes a new gap glyph */
glyph_t gap_alloc(ScrollerWidget widget);

/* Allocates and initializes a new message_view glyph */
glyph_t message_glyph_alloc(ScrollerWidget widget, Message message);

#endif /* GLYPH_H */
