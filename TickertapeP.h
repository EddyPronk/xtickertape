/* $Id: TickertapeP.h,v 1.2 1997/02/09 13:55:44 phelps Exp $ */

#ifndef TickertapeP_H
#define TickertapeP_H

/* Tickertape Widget Private Data */

#include <X11/Xaw/SimpleP.h>
#include "Tickertape.h"

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
    Dimension fadeLevels;

    /* Private state */
    GC gc;
    Pixel *groupPixels;
    Pixel *userPixels;
    Pixel *stringPixels;
} TickertapePart;


/* Full instance record declaration */
typedef struct _TickertapeRec
{
    CorePart core;
    SimplePart simple;
    TickertapePart tickertape;
} TickertapeRec;

#endif /* TickertapeP_H */
