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
static const char cvs_HISTORYP_H[] = "$Id: HistoryP.h,v 1.4 2001/07/04 14:27:55 phelps Exp $";
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


/* New fields for the History widget record */
typedef struct
{
    /* Resources */
    XtCallbackList callbacks;
    XtCallbackList attachment_callbacks;
    XFontStruct *font;
    Pixel group_pixel;
    Pixel user_pixel;
    Pixel string_pixel;
    Pixel separator_pixel;
    
    /* Private state */

    /* Our graphics context */
    GC gc;

    /* The x coordinate of the origin of the visible region */
    Position x;

    /* The y coordinate of the origin of the visible region */
    Position y;

    /* A message view for playing with */
    message_view_t mv;
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
