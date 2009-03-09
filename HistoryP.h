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

#ifndef HistoryP_H
#define HistoryP_H

#ifndef lint
static const char cvs_HISTORYP_H[] = "$Id: HistoryP.h,v 1.30 2009/03/09 05:26:26 phelps Exp $";
#endif /* lint */

#include <X11/CoreP.h>

#include "message_view.h"
#include "History.h"


/* New fields for the History widget record */
typedef struct
{
    int foo; /* Need to have something here */
} HistoryClassPart;

/* Full class record declaration */
typedef struct _HistoryClassRec
{
    CoreClassPart core_class;
    XmPrimitiveClassPart primitive_class;
    HistoryClassPart history_class;
} HistoryClassRec;


/* The type of outstanding movement events */
typedef struct translation_queue *translation_queue_t;

/* The history is stored as nodes in a tree and list */
typedef struct node *node_t;

/* Which way are we dragging? */
typedef enum
{
    DRAG_NONE,
    DRAG_UP,
    DRAG_DOWN
} drag_direction_t;

/* New fields for the History widget record */
typedef struct
{
    /* Resources */

    /* When are these called? */
    XtCallbackList callbacks;

    /* The functions to call when an attachment is to be displayed */
    XtCallbackList attachment_callbacks;

    /* The functions to call in response to a motion event */
    XtCallbackList motion_callbacks;

    /* The font to use when displaying the history messages */
    XFontStruct *font;

    /* The code set used by the font */
    char *code_set;

    /* The color to use when drawing the timestamp */
    Pixel timestamp_pixel;

    /* The color to use when drawing group strings */
    Pixel group_pixel;

    /* The color to use when drawing user strings */
    Pixel user_pixel;

    /* The color to use when drawing message strings */
    Pixel string_pixel;

    /* The color to use when drawing the separator */
    Pixel separator_pixel;

    /* The width of the margin */
    Dimension margin_width;

    /* The height of the margin */
    Dimension margin_height;

    /* The maximum number of messages to display in the history */
    Dimension message_capacity;

    /* The color to use when drawing the selection box */
    Pixel selection_pixel;

    /* The amount of time to wait before scrolling the list when
     * dragging the selection around */
    int drag_delay;
    
    /* Private state */

    /* Our graphics context */
    GC gc;

    /* The code set info used to display UTF-8 strings */
    utf8_renderer_t renderer;

    /* The vertical scrollbar */
    Widget vscrollbar;

    /* The horizontal scrollbar */
    Widget hscrollbar;

    /* The width of the longest string in the history widget */
    unsigned long width;

    /* The height of all of the strings in the history widget */
    unsigned long height;

    /* The height of a line in the history widget */
    long line_height;

    /* The x coordinate of the origin of the visible region */
    long x;

    /* The y coordinate of the origin of the visible region */
    long y;

    /* The x coordinate of the pointer during a drag operation */
    short pointer_x;

    /* The y coordinate of the pointer during a drag operation */
    short pointer_y;

    /* The queue of outstanding movements */
    translation_queue_t tqueue;

    /* The end of the queue */
    translation_queue_t tqueue_end;

    /* Non-zero if the history should display threads */
    Boolean is_threaded;

    /* The history as both tree and list */
    node_t nodes;

    /* The messages in order of receipt */
    message_t *messages;

    /* The number of message_views in the history */
    unsigned int message_count;

    /* The first index messages circular array */
    unsigned int message_index;

    /* An array of message_views in display order */
    message_view_t *message_views;

    /* The currently selected message_t (NULL if none) */
    message_t selection;

    /* The index of the selection ((unsigned int)-1 if none) */
    unsigned int selection_index;

    /* The dragging timeout */
    XtIntervalId drag_timeout;

    /* The direction in which to drag */
    drag_direction_t drag_direction;

    /* Non-zero if the timestamps should be displayed */
    int show_timestamps;
} HistoryPart;

/* Full instance record declaration */
typedef struct _HistoryRec
{
    CorePart core;
    XmPrimitivePart primitive;
    HistoryPart history;
} HistoryRec;


/* Semi-private methods */

#endif /* HISTORYP_H */
