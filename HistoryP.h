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

#ifndef HistoryP_H
#define HistoryP_H

#ifndef lint
static const char cvs_HISTORYP_H[] = "$Id: HistoryP.h,v 1.11 2001/07/15 23:58:39 phelps Exp $";
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
typedef struct delta_queue *delta_queue_t;

/* New fields for the History widget record */
typedef struct
{
    /* Resources */

    /* When are these called? */
    XtCallbackList callbacks;

    /* The functions to call when an attachment is to be displayed */
    XtCallbackList attachment_callbacks;

    /* The font to use when displaying the history messages */
    XFontStruct *font;

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

    
    /* Private state */

    /* Our graphics context */
    GC gc;

    /* The vertical scrollbar */
    Widget vscrollbar;

    /* The horizontal scrollbar */
    Widget hscrollbar;

    /* The width of the longest string in the history widget */
    unsigned long width;

    /* The height of the strings in the history widget */
    unsigned long height;

    /* The x coordinate of the origin of the visible region */
    long x;

    /* The y coordinate of the origin of the visible region */
    long y;

    /* The queue of outstanding movements */
    delta_queue_t dqueue;

    /* The number of message_views in the history */
    unsigned int message_count;

    /* An array of message_views in display order */
    message_view_t *message_views;
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
