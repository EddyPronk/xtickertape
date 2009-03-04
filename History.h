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

#ifndef HISTORY_H
#define HISTORY_H

#ifndef lint
static const char cvs_HISTORY_H[] =
    "$Id: History.h,v 1.16 2009/03/09 05:26:26 phelps Exp $";
#endif /* lint */


/*
  A threaded mesage History widget

 Class:                historyWidgetClass
 Class Name:        History
 Superclass:        Core

 Resources added by the History widget:

 Name                     Class                RepType                Default Value
 ----                     -----                -------                -------------
 font                     Font                XFontStruct *        XtDefaultFont
 fontCodeSet             String                String                NULL
 timestampPixel             TimestampPixel        Pixel                Black
 groupPixel             GroupPixel                Pixel                Blue
 userPixel             UserPixel                Pixel                Green
 stringPixel             StringPixel        Pixel                Red
 separatorPixel             SeparatorPixel        Pixel                Black
 marginWidth             MarginWidth        Dimension        5
 marginHeight             MarginHeight        Dimension        5
 messageCount             MessageCount        Dimension        32
 selectionPixel             SelectionPixel        Pixel                Gray
 dragDelay             DragDelay                int                100

 background             Background                Pixel                XtDefaultBackground
 border                     BorderColor        Pixel                XtDefaultForeground
 borderWidth             BorderWidth        Dimension        1
 cursor                     Cursor                Cursor                None
 cursorName             Cursor                String                NULL
 destroyCallback     Callback                Pointer                NULL
 height                     Height                Dimension        0
 insensitiveBorder   Insensitive        Pixmap                Gray
 mappedWhenManaged   MappedWhenManaged        Boolean                True
 pointerColor        Foreground         Pixel           XtDefaultForeground
 pointerColorBackground Background      Pixel           XtDefaultBackground
 sensitive             Sensitive                Boolean                True
 width                     Width                Dimension        0
 x                     Position                Position        0
 y                     Position                Position        0

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
void
HistorySetThreaded(Widget widget, Boolean is_threaded);


/* Returns whether or not the history is threaded */
Boolean
HistoryIsThreaded(Widget widget);


/* Set the widget's display to show timestamps or not */
void
HistorySetShowTimestamps(Widget widget, Boolean show_timestamps);


/* Returns whether or not the timestamps are visible */
Boolean
HistoryIsShowingTimestamps(Widget widget);


/* Adds a new message to the History */
void
HistoryAddMessage(Widget widget, message_t message);


/* Kills the thread of the given message */
void
HistoryKillThread(Widget widget, message_t message);


/* Selects a message in the history */
void
HistorySelect(Widget widget, message_t message);


/* Selects the parent of the message in the history */
void
HistorySelectId(Widget widget, char *message_id);


/* Returns the selected message or NULL if none is selected */
message_t
HistoryGetSelection(Widget widget);


#endif /* HISTORY_H */
