/* $Id: TickertapeP.h,v 1.3 1997/02/10 08:07:36 phelps Exp $ */

#ifndef TickertapeP_H
#define TickertapeP_H

/* Tickertape Widget Private Data */

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
    XFontStruct *font;
    Pixel groupPixel;
    Pixel userPixel;
    Pixel stringPixel;
    Pixel separatorPixel;
    Dimension fadeLevels;

    /* Private state */
    GC gc;
    Pixel *groupPixels;
    Pixel *userPixels;
    Pixel *stringPixels;
    Pixel *separatorPixels;

    /* Broken */
    MessageView view;
} TickertapePart;


/* Full instance record declaration */
typedef struct _TickertapeRec
{
    CorePart core;
    SimplePart simple;
    TickertapePart tickertape;
} TickertapeRec;



/* Private methods */
GC TtGCForGroup(TickertapeWidget self, int level);
XFontStruct *TtFontForGroup(TickertapeWidget self);

GC TtGCForUser(TickertapeWidget self, int level);
XFontStruct *TtFontForUser(TickertapeWidget self);

GC TtGCForString(TickertapeWidget self, int level);
XFontStruct *TtFontForString(TickertapeWidget self);

GC TtGCForSeparator(TickertapeWidget self, int level);
XFontStruct *TtFontForSeparator(TickertapeWidget self);

Pixmap TtCreatePixmap(TickertapeWidget self, unsigned int width);


#endif /* TickertapeP_H */
