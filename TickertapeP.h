/* $Id: TickertapeP.h,v 1.8 1997/02/14 16:33:16 phelps Exp $ */

#ifndef TickertapeP_H
#define TickertapeP_H

/* Tickertape Widget Private Data */

#include "List.h"

#include <X11/Xaw/SimpleP.h>
#include "Tickertape.h"
#include "MessageView.h"

/* New fields for the Tickertape widget record */
typedef struct
{
    int foo;
} TickertapeClassPart;


/* Full class record declaration */
typedef struct _TickertapeClassRec
{
    CoreClassPart core_class;
    SimpleClassPart simple_class;
    TickertapeClassPart tickertape_class;
} TickertapeClassRec;


/* New fields for the Tickertape widget record */
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
    GC gc;
    Pixel *groupPixels;
    Pixel *userPixels;
    Pixel *stringPixels;
    Pixel *separatorPixels;
} TickertapePart;


/* Full instance record declaration */
typedef struct _TickertapeRec
{
    CorePart core;
    SimplePart simple;
    TickertapePart tickertape;
} TickertapeRec;



/* Semi-private methods */

/* Answers a GC to be used for displaying the group */
GC TtGCForGroup(TickertapeWidget self, int level);

/* Answers a GC to be used for displaying the user */
GC TtGCForUser(TickertapeWidget self, int level);

/* Answers a GC to be used for displaying the string */
GC TtGCForString(TickertapeWidget self, int level);

/* Answers a GC to be used for displaying the separators */
GC TtGCForSeparator(TickertapeWidget self, int level);

/* Answers a GC to be used to draw things in the background color */
GC TtGCForBackground(TickertapeWidget self);

/* Answers the font to use for displaying the group */
XFontStruct *TtFontForGroup(TickertapeWidget self);

/* Answers the font to use for displaying the user */
XFontStruct *TtFontForUser(TickertapeWidget self);

/* Answers the font to use for displaying the string */
XFontStruct *TtFontForString(TickertapeWidget self);

/* Answers the font to use for displaying the separators */
XFontStruct *TtFontForSeparator(TickertapeWidget self);


/* Answers the number of fade levels messages should go through */
Dimension TtGetFadeLevels(TickertapeWidget self);

/* Answers a new Pixmap of the given width */
Pixmap TtCreatePixmap(TickertapeWidget self, unsigned int width);

/* Sets a timer to go off in interval milliseconds */
XtIntervalId TtStartTimer(TickertapeWidget self, unsigned long interval,
			  XtTimerCallbackProc proc, XtPointer client_data);

/* Stops a timer from going off */
void TtStopTimer(TickertapeWidget self, XtIntervalId timer);

#endif /* TickertapeP_H */
