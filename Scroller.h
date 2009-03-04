/* -*- mode: c; c-file-style: "elvin" -*- */
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

#ifndef SCROLLER_H
#define SCROLLER_H

/*
 A tickertape Scroller widget

 Class:                scrollerWidgetClass
 Class Name:        Scroller
 Superclass:        Core

 Resources added by the Scroller widget:

 Name                     Class                RepType                Default Value
 ----                     -----                -------                -----------
 font                     Font                XFontStruct *        XtDefaultFont
 fontCodeSet         String             String          NULL
 groupPixel             GroupPixel                Pixel                Blue
 userPixel             UserPixel                Pixel                Green
 stringPixel             StringPixel        Pixel                Red
 separatorPixel             SeparatorPixel        Pixel                Black
 fadeLevels             FadeLevels                Dimension        5

 usePixmap           UsePixmap                Boolean                False
 dragDelta             DragDelta                Dimension        3
 frequency             Frequency                Dimension        24
 stepSize             StepSize                Position        1

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
#ifndef XtNkillCallback
# define XtNkillCallback "killCallback"
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
#ifndef XtNfadeLevels
# define XtNfadeLevels "fadeLevels"
#endif
#ifndef XtCFadeLevels
# define XtCFadeLevels "FadeLevels"
#endif
#ifndef XtNusePixmap
# define XtNusePixmap "usePixmap"
#endif
#ifndef XtCUsePixmap
# define XtCUsePixmap "UsePixmap"
#endif
#ifndef XtNdragDelta
# define XtNdragDelta "dragDelta"
#endif
#ifndef XtCDragDelta
# define XtCDragDelta "DragDelta"
#endif
#ifndef XtNfrequency
# define XtNfrequency "frequency"
#endif
#ifndef XtCFrequency
# define XtCFrequency "Frequency"
#endif
#ifndef XtNstepSize
# define XtNstepSize "stepSize"
#endif
#ifndef XtCStepSize
# define XtCStepSize "StepSize"
#endif

typedef struct _ScrollerClassRec *ScrollerWidgetClass;
typedef struct _ScrollerRec *ScrollerWidget;

extern WidgetClass scrollerWidgetClass;


/*
 *Public methods
 */

#include "message.h"

/* Adds a Message to the receiver */
void
ScAddMessage(Widget self, message_t message);


/* Purge any killed messages */
void
ScPurgeKilled(Widget self);


#endif /* SCROLLER_H */
