/* $Id: Scroller.h,v 1.4 1998/12/17 01:24:50 phelps Exp $ */

#ifndef SCROLLER_H
#define SCROLLER_H

/*
 A Scroller scroller widget

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
 stepSize		StepSize		Dimension	1

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

#include "Message.h"

/* Adds a Message to the receiver */
void ScAddMessage(ScrollerWidget self, Message message);


#endif /* SCROLLER_H */
