/***************************************************************

  Copyright (C) 2001-2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

  This software is the property of the Mantara Software. All use
  of this software is permitted only under the terms of a
  license agreement with Mantara Software. If you do not have
  such a license, then you have no rights to use this software
  in any manner and should contact Mantara at the address below
  to determine an appropriate licensing arrangement.
  
     Mantara Software
     PO Box 1820
     Toowong QLD 4066
     Australia
     Tel: +61 7 3876 8844
     Fax: +61 7 3876 8843
     Email: licensing@mantara.com
  
     Web: http://www.mantara.com
  
  This software is being provided "AS IS" without warranty of
  any kind. In no event shall Mantara Software be liable for
  damage of any kind arising out of or in connection with the
  use or performance of this software.
****************************************************************/

#ifndef HISTORY_H
#define HISTORY_H

#ifndef lint
static const char cvs_HISTORY_H[] = "$Id: History.h,v 1.13 2004/08/02 22:24:16 phelps Exp $";
#endif /* lint */


/*
  A threaded mesage History widget

 Class:		historyWidgetClass
 Class Name:	History
 Superclass:	Core

 Resources added by the History widget:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 font		     Font		XFontStruct *	XtDefaultFont
 fontCodeSet	     String		String		NULL
 timestampPixel	     TimestampPixel	Pixel		Black
 groupPixel	     GroupPixel		Pixel		Blue
 userPixel	     UserPixel		Pixel		Green
 stringPixel	     StringPixel	Pixel		Red
 separatorPixel	     SeparatorPixel	Pixel		Black
 marginWidth	     MarginWidth	Dimension	5
 marginHeight	     MarginHeight	Dimension	5
 messageCount	     MessageCount	Dimension	32
 selectionPixel	     SelectionPixel	Pixel		Gray
 dragDelay	     DragDelay		int		100

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

#ifndef XtNfontCodeSet
# define XtNfontCodeSet "fontCodeSet"
#endif
#ifndef XtNattachmentCallback
# define XtNattachmentCallback "attachmentCallback"
#endif
#ifndef XtNmotionCallback
# define XtNmotionCallback "motionCallback"
#endif
#ifndef XtNtimestampPixel
# define XtNtimestampPixel "timestampPixel"
#endif
#ifndef XtCTimestampPixel
# define XtCTimestampPixel "TimestampPixel"
#endif
#ifndef XtNgroupPixel
# define XtNgroupPixel "groupPixel"
#endif
#ifndef XtCGroupPixel
# define XtCGroupPixel "GroupPixel"
#endif
#ifndef XtNuserPixel
# define XtNuserPixel "userPixel"
#endif
#ifndef XtCUserPixel
# define XtCUserPixel "UserPixel"
#endif
#ifndef XtNstringPixel
# define XtNstringPixel "stringPixel"
#endif
#ifndef XtCStringPixel
# define XtCStringPixel "StringPixel"
#endif
#ifndef XtNseparatorPixel
# define XtNseparatorPixel "separatorPixel"
#endif
#ifndef XtCSeparatorPixel
# define XtCSeparatorPixel "SeparatorPixel"
#endif
#ifndef XtNmarginWidth
# define XtNmarginWidth "marginWidth"
#endif
#ifndef XtCMarginWidth
# define XtCMarginWidth "MarginWidth"
#endif
#ifndef XtNmarginHeight
# define XtNmarginHeight "marginHeight"
#endif
#ifndef XtCMarginHeight
# define XtCMarginHeight "MarginHeight"
#endif
#ifndef XtNmessageCapacity
# define XtNmessageCapacity "messageCapacity"
#endif
#ifndef XtCMessageCapacity
# define XtCMessageCapacity "MessageCapacity"
#endif
#ifndef XtNselectionPixel
# define XtNselectionPixel "selectionPixel"
#endif
#ifndef XtCSelectionPixel
# define XtCSelectionPixel "SelectionPixel"
#endif
#ifndef XtNdragDelay
# define XtNdragDelay "dragDelay"
#endif
#ifndef XtCDragDelay
# define XtCDragDelay "DragDelay"
#endif


typedef struct _HistoryClassRec *HistoryWidgetClass;
typedef struct _HistoryRec *HistoryWidget;

extern WidgetClass historyWidgetClass;

/*
 *
 * Public methods
 *
 */

/* Set the widget's display to be threaded or not */
void HistorySetThreaded(Widget widget, Boolean is_threaded);

/* Returns whether or not the history is threaded */
Boolean HistoryIsThreaded(Widget widget);

/* Set the widget's display to show timestamps or not */
void HistorySetShowTimestamps(Widget widget, Boolean show_timestamps);

/* Returns whether or not the timestamps are visible */
Boolean HistoryIsShowingTimestamps(Widget widget);


/* Adds a new message to the History */
void HistoryAddMessage(Widget widget, message_t message);

/* Kills the thread of the given message */
void HistoryKillThread(Widget widget, message_t message);

/* Selects a message in the history */
void HistorySelect(Widget widget, message_t message);

/* Selects the parent of the message in the history */
void HistorySelectId(Widget widget, char *message_id);

/* Returns the selected message or NULL if none is selected */
message_t HistoryGetSelection(Widget widget);


#endif /* HISTORY_H */
