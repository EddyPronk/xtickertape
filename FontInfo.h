/* $Id: FontInfo.h,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#ifndef FONTINFO_H
#define FONTINFO_H

#include <X11/Xlib.h>

typedef struct FontInfo_t *FontInfo;

/* Answers a new FontInfo */
FontInfo FontInfo_alloc(XFontStruct *fontStruct);

/* Answers the underlying font */
Font FontInfo_getFont(FontInfo self);

/* Answers the ascent of the font */
unsigned int FontInfo_getAscent(FontInfo self);

/* Answers the descent of the font */
unsigned int FontInfo_getDescent(FontInfo self);

/* Answers the height of the font */
unsigned int FontInfo_getHeight(FontInfo self);

/* Measures the width of a string displayed in this font */
unsigned long FontInfo_stringWidth(FontInfo self, char *string);


#endif /* FONTINFO_H */
