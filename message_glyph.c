/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: message_glyph.c,v 1.20 1999/09/13 12:51:23 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include "glyph.h"
#include "ScrollerP.h"

#define SEPARATOR ":"
#define MAX(x, y) ((x > y) ? x : y)

typedef struct message_glyph *message_glyph_t;
struct message_glyph
{
    GLYPH_PREFIX

    /* The ScrollerWidget in which the receiver is displayed */
    ScrollerWidget widget;

    /* The message which the receiver represents */
    message_t message;

    /* The receiver's reference count */
    int ref_count;

    /* True if the receiver has [been] expired */
    int has_expired;

    /* The level of fading exhibited by the receiver */
    int fade_level;

    /* The ascent of the tallest font used by the receiver */
    int ascent;

    /* The width of the receiver's separator string */
    unsigned int separator_width;

    /* The separator's left bearing */
    int separator_lbearing;

    /* The separator's right bearing */
    int separator_rbearing;


    /* The width of the receiver's group string */
    unsigned int group_width;

    /* The group string's left bearing */
    int group_lbearing;

    /* The group string's right bearing */
    int group_rbearing;


    /* The width of the receiver's user string */
    unsigned int user_width;

    /* The user string's left bearing */
    int user_lbearing;

    /* The user string's right bearing */
    int user_rbearing;


    /* The width of the receiver's message string */
    unsigned int string_width;

    /* The message string's left bearing */
    int string_lbearing;

    /* The message string's right bearing */
    int string_rbearing;


    /* The width of the whitepspace after the message */
    unsigned int spacing;

    /* The receiver's timer id */
    XtIntervalId timer;
};


/* Static function headers */
static void set_clock(message_glyph_t self);


/* This is called when the colors should fade */
static void tick(message_glyph_t self, XtIntervalId *ignored)
{
    unsigned int max_levels;

    self -> timer = 0;
    max_levels = ScGetFadeLevels(self -> widget);

    /* Don't get older than we have to */
    if (self -> fade_level + 1 < max_levels)
    {
	self -> fade_level++;
	set_clock(self);
    }
    else
    {
	if (! self -> has_expired)
	{
	    self -> has_expired = True;
	    ScGlyphExpired(self -> widget, (glyph_t) self);
	}
    }

    /* Tell the scroller to repaint this glyph */
    ScRepaintGlyph(self -> widget, (glyph_t) self);
}

/* Sets the timer to call tick() when the colors should fade */
static void set_clock(message_glyph_t self)
{
    int duration;

    /* Default to 1/20th of a second delay on expired messages */
    if (self -> has_expired)
    {
	duration = 50;
    }
    else
    {
	duration = 60 * 1000 * message_get_timeout(self -> message) / 
	    ScGetFadeLevels(self -> widget);
    }

    self -> timer = ScStartTimer(self -> widget, duration, (XtTimerCallbackProc)tick, (XtPointer)self);
}

/* Clears the timer */
static void clear_clock(message_glyph_t self)
{
    if ((self -> timer) != 0)
    {
	ScStopTimer(self -> widget, self -> timer);
	self -> timer = 0;
    }
}

/* Allocates another reference to the gap */
static message_glyph_t do_alloc(message_glyph_t self)
{
    self -> ref_count++;
    return self;
}

/* Free everything except the message */
static void do_free(message_glyph_t self)
{
    /* Decrement the reference count */
    if (--self -> ref_count > 0)
    {
	return;
    }

    clear_clock(self);

    /* Free the message */
    message_free(self -> message);

    free(self);
}

/* Answers the receiver's message */
static message_t get_message(message_glyph_t self)
{
    return self -> message;
}

/* Answers the receiver's width */
static unsigned int get_width(message_glyph_t self)
{
    return
	self -> group_width +
	self -> separator_width +
	self -> user_width +
	self -> separator_width +
	self -> string_width +
	self -> spacing;
}

/* Answers statistics to use for a given character in the font */
static XCharStruct *per_char(XFontStruct *font, int ch)
{
    unsigned int first = font -> min_char_or_byte2;
    unsigned int last = font -> max_char_or_byte2;

    /* Look for the most common case first */
    if ((first <= ch) && (ch <= last))
    {
	return font -> per_char + ch - first;
    }

    /* If the font's default char is valid then use it */
    if ((first <= font -> default_char) && (font -> default_char <= last))
    {
	return font -> per_char + font -> default_char - first;
    }

    /* Ok, try using a space */
    if ((first <= ' ') && (' ' <= last))
    {
	return font -> per_char + ' ' - first;
    }

    /* If all else fails, default to the first character in the font */
    return font -> per_char;
}

/* Measure all of the characters in a string */
static unsigned int measure_string(XFontStruct *font, char *string, int *lbearing, int *rbearing)
{
    unsigned char *pointer;
    XCharStruct *char_info;
    unsigned int width;


    /* Shortcut for empty strings that makes sure everything gets set */
    if (*string == '\0')
    {
	if (lbearing != NULL)
	{
	    *lbearing = 0;
	}

	if (rbearing != NULL)
	{
	    *rbearing = 0;
	}

	return 0;
    }

    /* Get the first character's information for the string's lbearing */
    pointer = (unsigned char *)string;
    char_info = per_char(font, *(pointer++));
    width = char_info -> width;
    if (lbearing != NULL)
    {
	*lbearing = char_info -> lbearing;
    }

    /* Measure the rest of the characters */
    while (*pointer != '\0')
    {
	char_info = per_char(font, *(pointer++));
	width += char_info -> width;
    }

    /* Adjust the width by the last character's rbearing */
    if (rbearing != NULL)
    {
	*rbearing = width + char_info -> rbearing - char_info -> width;
    }

    return width;
}

/* Draws a String with an optional underline */
static void paint_string(
    message_glyph_t self,
    Display *display, Drawable drawable, GC gc, XFontStruct *font,
    int offset, int string_width, int baseline, char *string, int do_underline,
    int x, int y, int width, int height)
{
    XCharStruct *char_info;
    unsigned char *first;
    unsigned char *last;
    int left, right;

    /* Find the first visible character in the string */
    left = offset;
    first = (unsigned char *)string;
    char_info = per_char(font, *first);

    while (left + char_info -> rbearing < x)
    {
	left += char_info -> width;
	first++;
	char_info = per_char(font, *first);
    }

    /* Find the character *after* the last visible character */
    last = first;
    right = left;

    while ((right + char_info -> lbearing < x + width) && (*last != '\0'))
    {
	right += char_info -> width;
	last++;
	char_info = per_char(font, *last);
    }


#ifdef DEBUG_GLYPH
    {
	XGCValues values;
	values.foreground = random();
	XChangeGC(display, gc, GCForeground, &values);
    }
#endif /* DEBUG */

    /* Draw all of the characters of the string */
    XDrawString(display, drawable, gc, left, baseline, first, last - first);

    /* Draw the underline based on the string's width to avoid gaps */
    if (do_underline)
    {
	XDrawLine(
	    display, drawable, gc,
	    x < offset ? offset : x, baseline + 1,
	    offset + string_width < x + width ? offset + string_width : x + width, baseline + 1);
    }
}

/* Draw the receiver */
static void do_paint(
    message_glyph_t self, Display *display, Drawable drawable,
    int offset, int w, int x, int y, int width, int height)
{
    int do_underline = message_has_attachment(self -> message);
    int level = self -> fade_level;
    int baseline = y + self -> ascent;
    int left = offset;

    /* Draw the group string if appropriate */
    if ((x <= left + self -> group_rbearing) && (left + self -> group_lbearing <= x + width))
    {
	paint_string(
	    self, display, drawable, ScGCForGroup(self -> widget, level, x, y, width, height),
	    ScFontForGroup(self -> widget),
	    left, self -> group_width, baseline,
	    message_get_group(self -> message), do_underline,
	    x, y, width, height);
    }

    left += self -> group_width;

    /* Draw the separator if appropriate */
    if ((x <= left + self -> separator_rbearing) && (left + self -> separator_lbearing <= x + width))
    {
	paint_string(
	    self, display, drawable, ScGCForSeparator(self -> widget, level, x, y, width, height),
	    ScFontForSeparator(self -> widget),
	    left, self -> separator_width, baseline,
	    SEPARATOR, do_underline,
	    x, y, width, height);
    }

    left += self -> separator_width;

    /* Draw the user string if appropriate */
    if ((x <= left + self -> user_rbearing) && (left + self -> user_lbearing <= x + width))
    {
	paint_string(
	    self, display, drawable, ScGCForUser(self -> widget, level, x, y, width, height),
	    ScFontForUser(self -> widget),
	    left, self -> user_width, baseline,
	    message_get_user(self -> message), do_underline,
	    x, y, width, height);
    }

    left += self -> user_width;

    /* Draw the separator if appropriate */
    if ((x <= left + self -> separator_rbearing) && (left + self -> separator_lbearing <= x + width))
    {
	paint_string(
	    self, display, drawable, ScGCForSeparator(self -> widget, level, x, y, width, height),
	    ScFontForSeparator(self -> widget),
	    left, self -> separator_width, baseline,
	    SEPARATOR, do_underline,
	    x, y, width, height);
    }

    left += self -> separator_width;

    /* Draw the message string if appropriate */
    if ((x <= left + self -> string_rbearing) && (left + self -> string_lbearing <= x + width))
    {
	paint_string(
	    self, display, drawable, ScGCForString(self -> widget, level, x, y, width, height),
	    ScFontForString(self -> widget),
	    left, self -> string_width, baseline,
	    message_get_string(self -> message), do_underline,
	    x, y, width, height);
    }
}

/* Answers True if the receiver has [been] expired */
static int get_is_expired(message_glyph_t self)
{
    return self -> has_expired;
}

/* Expires the receiver now */
static void do_expire(message_glyph_t self)
{
    if (! self -> has_expired)
    {
	self -> has_expired = True;
	ScRepaintGlyph(self -> widget, (glyph_t) self);
	ScGlyphExpired(self -> widget, (glyph_t) self);

	/* Restart the timer so that we quickly fade */
	clear_clock(self);
	set_clock(self);
    }
}


/* Allocates and initializes a new message_glyph glyph */
glyph_t message_glyph_alloc(ScrollerWidget widget, message_t message)
{
    XFontStruct *font;
    message_glyph_t self;

    /* Allocate memory for a new message_glyph glyph */
    if ((self = (message_glyph_t) malloc(sizeof(struct message_glyph))) == NULL)
    {
	return NULL;
    }

    /* Initialize its fields to sane values */
    self -> previous = NULL;
    self -> next = NULL;
    self -> alloc = (alloc_method_t)do_alloc;
    self -> free = (free_method_t)do_free;
    self -> get_message = (message_method_t)get_message;
    self -> get_width = (width_method_t)get_width;
    self -> paint = (paint_method_t)do_paint;
    self -> is_expired = (is_expired_method_t)get_is_expired;
    self -> expire = (expire_method_t)do_expire;

    self -> widget = widget;
    self -> message = message_alloc_reference(message);
    self -> ref_count = 1;
    self -> has_expired = False;
    self -> fade_level = 0;

    /* Compute sizes of various strings */
    font = ScFontForSeparator(self -> widget);
    self -> separator_width = measure_string(
	font, SEPARATOR,
	&self -> separator_lbearing,
	&self -> separator_rbearing);
    self -> ascent = font -> ascent;

    font = ScFontForGroup(self -> widget);
    self -> group_width = measure_string(
	font, message_get_group(self -> message),
	&self -> group_lbearing,
	&self -> group_rbearing);
    self -> ascent = MAX(self -> ascent, font -> ascent);

    font = ScFontForString(self -> widget);
    self -> user_width = measure_string(
	font, message_get_user(self -> message),
	&self -> user_lbearing,
	&self -> user_rbearing);
    self -> ascent = MAX(self -> ascent, font -> ascent);

    font = ScFontForString(self -> widget);
    self -> string_width = measure_string(
	font, message_get_string(self -> message),
	&self -> string_lbearing,
	&self -> string_rbearing);
    self -> ascent = MAX(self -> ascent, font -> ascent);

    self -> spacing = measure_string(font, "n", NULL, NULL);

    set_clock(self);

    return (glyph_t) self;
}
