/***********************************************************************

  Copyright (C) 1998-2004 by Mantara Software (ABN 17 105 665 594).
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

#ifndef SCROLLERP_H
#define SCROLLERP_H

#ifndef lint
static const char cvs_SCROLLERP_H[] = "$Id: ScrollerP.h,v 1.49 2004/08/03 12:29:16 phelps Exp $";
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
    char *code_set;
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

    /* The code set info used to display UTF-8 strings */
    utf8_renderer_t renderer;

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

#endif /* SCROLLERP_H */
