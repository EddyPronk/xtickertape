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
             A Widget which scrolls glyphs

****************************************************************/

#ifndef SCROLLER_H
#define SCROLLER_H

#ifndef lint
static const char cvs_SCROLLER_H[] = "$Id: Scroller.h,v 1.9 1999/09/09 14:29:48 phelps Exp $";
#endif /* lint */

/*
 A tickertape Scroller widget

 Class:		scrollerWidgetClass
 Class Name:	Scroller
 Superclass:	Core

 Resources added by the Scroller widget:

 Name		Class		RepType		Default Value
 ----		-----		-------	-----------
 font		Font		XFontStruct *	XtDefaultFont
 groupPixel	GroupPixel	Pixel		Blue
 userPixel	UerPixel		Pixel		Green
 stringPixel	StringPixel	Pixel		Red
 separatorPixel	SeparatorPixel	Pixel		Black
 fadeLevels	FadeLevels	Dimension	5
 frequency	Frequency	Dimension	24
 stepSize		StepSize		Position	1

 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 cursor		     Cursor		Cursor		None
 cursorName	     Cursor		String		NULL
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	0
 insensitiveBorder   Insensitive	Pixmap		Gray
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 pointerColor        Foreground         Pixel           XtDefaultForeground
 pointerColorBackground Background      Pixel           XtDefaultBackground
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0

 */

#define XtNattachmentCallback "attachmentCallback"
#define XtNkillCallback "killCallback"
#define XtNgroupPixel "groupPixel"
#define XtCGroupPixel "GroupPixel"
#define XtNuserPixel "userPixel"
#define XtCUserPixel "UserPixel"
#define XtNstringPixel "stringPixel"
#define XtCStringPixel "StringPixel"
#define XtNseparatorPixel "separatorPixel"
#define XtCSeparatorPixel "SeparatorPixel"
#define XtNfadeLevels "fadeLevels"
#define XtCFadeLevels "FadeLevels"
#define XtNfrequency "frequency"
#define XtCFrequency "Frequency"
#define XtNstepSize "stepSize"
#define XtCStepSize "StepSize"

typedef struct _ScrollerClassRec *ScrollerWidgetClass;
typedef struct _ScrollerRec *ScrollerWidget;

extern WidgetClass scrollerWidgetClass;


/*
 *Public methods
 */

#include "message.h"

/* Adds a Message to the receiver */
void ScAddMessage(ScrollerWidget self, message_t message);


#endif /* SCROLLER_H */
