/* $Id: Tickertape.h,v 1.3 1997/02/10 08:07:35 phelps Exp $ */

#ifndef TICKERTAPE_H
#define TICKERTAPE_H

#include <X11/Xaw/Simple.h>

/*
 A Tickertape scroller widget

 Class:		tickertapeWidgetClass
 Class Name:	Tickertape
 Superclass:	Simple

 Resources added by the Tickertape widget:

 Name		Class		RepType		Default Value
 ----		-----		-------	-----------
 font		Font		XFontStruct *	XtDefaultFont
 groupPixel	GroupPixel	Pixel		Blue
 userPixel	UerPixel		Pixel		Green
 stringPixel	StringPixel	Pixel		Red
 separatorPixel	SeparatorPixel	Pixel		Black
 fadeLevels	FadeLevels	Dimension	5

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

typedef struct _TickertapeClassRec *TickertapeWidgetClass;
typedef struct _TickertapeRec *TickertapeWidget;

extern WidgetClass tickertapeWidgetClass;




#endif /* TICKERTAPE_H */
