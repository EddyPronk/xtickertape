/* $Id: FontInfo.c,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "FontInfo.h"

struct FontInfo_t
{
    XFontStruct *fontStruct;
    unsigned int first;
    unsigned int last;
    unsigned int defaultWidth;
};


/* Initializes a newly created FontInfo */
void FontInfo_initialize(FontInfo self)
{
    unsigned int ch = self -> fontStruct -> default_char;

    self -> first = self -> fontStruct -> min_char_or_byte2;
    self -> last = self -> fontStruct -> max_char_or_byte2;

    /* Check to see if default_char is valid */
    if ((self -> first <= ch) && (ch <= self -> last))
    {
	self -> defaultWidth = self -> fontStruct -> per_char[ch].width;
    }
    /* If defaultChar is bogus, try space */
    else if ((self -> first <= ' ') && (' ' <= self -> last))
    {
	self -> defaultWidth = self -> fontStruct -> per_char[' '].width;
    }
    /* Otherwise use the max width for default */
    else
    {
	self -> defaultWidth = self -> fontStruct -> max_bounds.width;
    }
}


/* Answers a new FontInfo */
FontInfo FontInfo_alloc(XFontStruct *fontStruct)
{
    FontInfo info = (FontInfo) malloc(sizeof(struct FontInfo_t));

    info -> fontStruct = fontStruct;
    FontInfo_initialize(info);
    return info;
}


/* Answers the underlying font */
Font FontInfo_getFont(FontInfo self)
{
    return self -> fontStruct -> fid;
}

/* Answers the ascent of the font */
unsigned int FontInfo_getAscent(FontInfo self)
{
    return self -> fontStruct -> ascent;
}

/* Answers the descent of the font */
unsigned int FontInfo_getDescent(FontInfo self)
{
    return self -> fontStruct -> descent;
}

/* Answers the height of the font */
unsigned int FontInfo_getHeight(FontInfo self)
{
    return FontInfo_getAscent(self) + FontInfo_getDescent(self);
}

/* Measures the width of a string displayed in this font */
unsigned long FontInfo_stringWidth(FontInfo self, char *string)
{
    unsigned long width = 0;
    char *pointer;

    for (pointer = string; *pointer != '\0'; pointer++)
    {
	unsigned char ch = (unsigned char)(*pointer);
	if ((self -> first <= ch) && (ch <= self -> last))
	{
	    width += self -> fontStruct -> per_char[ch - self -> first].width;
	}
	else
	{
	    width += self -> defaultWidth;
	}
    }

    return width;
}
