/*
 * $Id: ScrollerP.h,v 1.1 1998/12/17 01:24:51 phelps Exp $
 * COPYRIGHT!
 * 
 * Scroller Widget Private Data
 */

#ifndef ScrollerP_H
#define ScrollerP_H

#include <X11/CoreP.h>

#include "Scroller.h"
#include "MessageView.h"
#include "List.h"

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


/* New fields for the Scroller widget record */
typedef struct
{
    /* Resources */
    XtCallbackList callbacks;
    XFontStruct *font;
    Pixel groupPixel;
    Pixel userPixel;
    Pixel stringPixel;
    Pixel separatorPixel;
    Dimension fadeLevels;
    Dimension frequency;
    Dimension step;

    /* Private state */
    int isStopped;
    List messages;
    List holders;
    long offset;
    long visibleWidth;
    unsigned int nextVisible;
    unsigned int realCount;
    GC backgroundGC;
    GC gc;
    Pixel *groupPixels;
    Pixel *userPixels;
    Pixel *stringPixels;
    Pixel *separatorPixels;
} ScrollerPart;


/* Full instance record declaration */
typedef struct _ScrollerRec
{
    CorePart core;
    ScrollerPart scroller;
} ScrollerRec;



/* Semi-private methods */

/* Answers a GC to be used for displaying the group */
GC ScGCForGroup(ScrollerWidget self, int level);

/* Answers a GC to be used for displaying the user */
GC ScGCForUser(ScrollerWidget self, int level);

/* Answers a GC to be used for displaying the string */
GC ScGCForString(ScrollerWidget self, int level);

/* Answers a GC to be used for displaying the separators */
GC ScGCForSeparator(ScrollerWidget self, int level);

/* Answers a GC to be used to draw things in the background color */
GC ScGCForBackground(ScrollerWidget self);

/* Answers the font to use for displaying the group */
XFontStruct *ScFontForGroup(ScrollerWidget self);

/* Answers the font to use for displaying the user */
XFontStruct *ScFontForUser(ScrollerWidget self);

/* Answers the font to use for displaying the string */
XFontStruct *ScFontForString(ScrollerWidget self);

/* Answers the font to use for displaying the separators */
XFontStruct *ScFontForSeparator(ScrollerWidget self);


/* Answers the number of fade levels messages should go through */
Dimension ScGetFadeLevels(ScrollerWidget self);

/* Answers a new Pixmap of the given width */
Pixmap ScCreatePixmap(ScrollerWidget self, unsigned int width, unsigned int height);

/* Sets a timer to go off in interval milliseconds */
XtIntervalId ScStartTimer(ScrollerWidget self, unsigned long interval,
			  XtTimerCallbackProc proc, XtPointer client_data);

/* Stops a timer from going off */
void ScStopTimer(ScrollerWidget self, XtIntervalId timer);

#endif /* ScrollerP_H */
