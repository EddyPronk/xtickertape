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

   Description: 
             Private Scroller widget definitions

****************************************************************/

#ifndef SCROLLERP_H
#define SCROLLERP_H

#ifndef lint
static const char cvs_SCROLLERP_H[] = "$Id: ScrollerP.h,v 1.42 2001/07/04 08:50:14 phelps Exp $";
#endif /* lint */

#include <X11/CoreP.h>

#include "Scroller.h"


/* New fields for the Scroller widget record */
typedef struct
{
    int foo;
} ScrollerClassPart;


/* Full class record declaration */
typedef struct _ScrollerClassRec
{
    CoreClassPart core_class;
    ScrollerClassPart scroller_class;
} ScrollerClassRec;


typedef struct glyph *glyph_t;
typedef struct glyph_holder *glyph_holder_t;

/* New fields for the Scroller widget record */
typedef struct
{
    /* Resources */
    XtCallbackList callbacks;
    XtCallbackList attachment_callbacks;
    XtCallbackList kill_callbacks;
    XFontStruct *font;
    Pixel group_pixel;
    Pixel user_pixel;
    Pixel string_pixel;
    Pixel separator_pixel;
    Dimension fade_levels;
    Boolean use_pixmap;
    Position drag_delta;
    Dimension frequency;
    Position step;

    /* Private state */

    /* The timer used to do the scrolling */
    XtIntervalId timer;

    /* True if there are no messages to scroll */
    Bool is_stopped;

    /* True if the scroller is visible */
    Bool is_visible;

    /* Are we dragging? */
    Bool is_dragging;

    /* The leftmost glyph holder */
    glyph_holder_t left_holder;

    /* The rightmost glyph holder */
    glyph_holder_t right_holder;

    /* The number of pixels of the leftmost glyph beyond the left edge of the scroller */
    int left_offset;

    /* The number of pixels of the rightmost glyph beyond the edge of the scroller */
    int right_offset;

    /* The gap in the scroller's circular queue of glyphs */
    glyph_t gap;

    /* The minimum width for the gap */
    int min_gap_width;

    /* The width of the unexpired glyphs in the circular queue the
     * last time that a gap was added */
    int last_width;

    /* The height of the scrolling text */
    int height;


    /* The sequence number of our last CopyArea request */
    unsigned long request_id;

    /* The difference in position between the left_offset and
     * right_offset view of our position and our knowledge of the X
     * server's state.  Positive values indicate the we've performed a
     * CopyArea to the right that hasn't yet been acknowledged by the
     * server.  If this is zero then the server should be in sync. */
    int local_delta;

    /* The difference between the current scroller position and the
     * desired scroller position.  Positive values indicate that we
     * would like to see the scroller to the right of its current
     * position. */
    int target_delta;


    /* The initial position of the drag */
    int start_drag_x;

    /* The position of the pointer the last time we noticed a drag */
    int last_x;

    /* The width of the clip mask */
    int clip_width;


    /* The off-screen pixmap */
    Pixmap pixmap;

    /* The GC used to draw the Scroller's background */
    GC backgroundGC;

    /* The GC used to draw various glyphs */
    GC gc;

    /* The array of Pixels used to display the group portion of a
     * message at varying degrees of fading */
    Pixel *group_pixels;

    /* The array of Pixels used to display the user portion of a
     * message at varying degrees of fading */
    Pixel *user_pixels;

    /* The array of Pixels used to display the string portion of a
     * message at varying degrees of fading */
    Pixel *string_pixels;

    /* The array of Pixels used to display the separator portion of a
     * message at varying degrees of fading */
    Pixel *separator_pixels;
} ScrollerPart;


/* Full instance record declaration */
typedef struct _ScrollerRec
{
    CorePart core;
    ScrollerPart scroller;
} ScrollerRec;



/* Semi-private methods */

/* Repaints the given glyph (if visible) */
void ScRepaintGlyph(ScrollerWidget self, glyph_t glyph);

/* Callback for expiring glyphs */
void ScGlyphExpired(ScrollerWidget self, glyph_t glyph);

/* Answers the width of the gap glyph */
int ScGapWidth(ScrollerWidget self);

/* Delete a message from the receiver */
void ScDeleteMessage(ScrollerWidget self, message_t message);

#endif /* SCROLLERP_H */
