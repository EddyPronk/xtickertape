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
static const char cvsid[] = "$Id: message_glyph.c,v 1.38 2001/06/17 00:48:56 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include "glyph.h"
#include "ScrollerP.h"

#define SEPARATOR ":"
#define MIN(x, y) ((x < y) ? x : y)
#define MAX(x, y) ((x > y) ? x : y)


/* Various debugging flags */
#if 0
#define DEBUG_PER_CHAR 1
#define DEBUG_GLYPH 1
#endif

static XCharStruct empty_char = { 0, 0, 0, 0, 0, 0 };

typedef struct message_glyph *message_glyph_t;
struct message_glyph
{
    GLYPH_PREFIX

    /* The ScrollerWidget in which the receiver is displayed */
    ScrollerWidget widget;

    /* The message which the receiver represents */
    message_t message;

    /* The receiver's replacement */
    glyph_t replacement;

    /* The receiver's reference count */
    int ref_count;

    /* True if the receiver has [been] expired */
    int has_expired;

    /* The level of fading exhibited by the receiver */
    int fade_level;

    /* The largest descent of any character in the font */
    int ascent;

    /* The width of the receiver's separator string */
    int separator_width;

    /* The separator's left bearing */
    int separator_lbearing;

    /* The separator's right bearing */
    int separator_rbearing;


    /* The width of the receiver's group string */
    int group_width;

    /* The group string's left bearing */
    int group_lbearing;

    /* The group string's right bearing */
    int group_rbearing;


    /* The width of the receiver's user string */
    int user_width;

    /* The user string's left bearing */
    int user_lbearing;

    /* The user string's right bearing */
    int user_rbearing;


    /* The width of the receiver's message string */
    int string_width;

    /* The message string's left bearing */
    int string_lbearing;

    /* The message string's right bearing */
    int string_rbearing;


    /* The width of the whitepspace after the message */
    int spacing;

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

	/* Tell the scroller to repaint this glyph */
	ScRepaintGlyph(self -> widget, (glyph_t) self);
    }
    else
    {
	if (! self -> has_expired)
	{
	    self -> has_expired = True;
	    ScGlyphExpired(self -> widget, (glyph_t) self);
	}
    }
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

    /* Free our replacement if we have one */
    if (self -> replacement != NULL)
    {
	self -> replacement -> free(self -> replacement);
    }

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
	MIN(0, -self -> group_lbearing) +
	self -> group_width +
	self -> separator_width +
	self -> user_width +
	self -> separator_width +
	self -> string_rbearing +
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
	XCharStruct *per_char = font -> per_char + ch - first;

	/* If the bounding box is non-zero then return this character */
	if (per_char -> width != 0 ||
	    per_char -> ascent != 0 || per_char -> descent != 0 ||
	    per_char -> lbearing != 0 || per_char -> rbearing != 0) {
	    return font -> per_char + ch - first;
	}
    }

    /* If the character isn't in the given range, try the default char */
    if ((first <= font -> default_char) && (font -> default_char <= last))
    {
	return font -> per_char + font -> default_char - first;
    }

    /* Assume zero-width characters for this pathological case */
    return &empty_char;
}

/* Measure all of the characters in a string */
static unsigned int measure_string(XFontStruct *font, char *string, int *lbearing, int *rbearing)
{
    unsigned char *pointer;
    XCharStruct *char_info;
    long width;
    int left;
    int right;

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
    left = char_info -> lbearing;
    right = char_info -> rbearing;

    /* Measure the rest of the characters */
    while (*pointer != '\0')
    {
	char_info = per_char(font, *(pointer++));
	left = (left < (width + char_info -> lbearing)) ? left : (width + char_info -> lbearing);
	right = (right > (width + char_info -> rbearing)) ? right : (width + char_info -> rbearing);
	width += char_info -> width;
    }

    /* Update the lbearing and rbearing */
    if (lbearing != NULL)
    {
	*lbearing = left;
    }

    if (rbearing != NULL)
    {
	*rbearing = right;
    }

    return (unsigned int)width;
}

#if 0
/* Draws a String with an optional underline */
static void paint_string(
    message_glyph_t self,
    Display *display, Drawable drawable, GC gc, XFontStruct *font,
    int x, int string_width, int y, char *string, int do_underline,
    XRectangle *bbox)
{
    XCharStruct *char_info;
    char *first;
    char *last;
    int left, right;

    /* Find the first visible character in the string */
    left = x;
    first = string;
    char_info = per_char(font, (unsigned char)*first);

    while ((left + char_info -> rbearing < bbox -> x) && (*first != '\0'))
    {
	left += char_info -> width;
	first++;
	char_info = per_char(font, (unsigned char)*first);
    }

    /* Find the character *after* the last visible character */
    last = first;
    right = left;

    while ((right + char_info -> lbearing < bbox -> x + bbox -> width) && (*last != '\0'))
    {
	right += char_info -> width;
	last++;
	char_info = per_char(font, *last);
    }

#if 0
    {
	XGCValues values;

	XFillRectangle(display, drawable, gc, x, 0, string_width, bbox -> height);
	values.foreground = random();
	XChangeGC(display, gc, GCForeground, &values);
    }
#endif

    /* Draw all of the characters of the string if there are any */
    if (*first != '\0')
    {
	XDrawString(display, drawable, gc, left, y, first, last - first);
    }

    /* Draw the underline based on the string's width to avoid gaps */
    if (do_underline)
    {
	XDrawLine(display, drawable, gc,
		  MAX(x, bbox -> x), y + 1,
		  MIN(x + string_width, bbox -> x + bbox -> width), y + 1);
    }
}
#endif

/* Draw a string, measuring the individual characters to ensure that
 * we don't draw anything outside the bounding box */
static void draw_string(
    Display *display, Drawable drawable,
    GC gc, XFontStruct *font,
    int x, int y, XRectangle *bbox,
    char *string)
{
    XCharStruct *char_info;
    char *first;
    char *last;
    int left, right;

    /* Find the first visible character in the string */
    left = x;
    first = string;
    /* FIX THIS: what about multibyte characters? */
    char_info = per_char(font, (unsigned char)*first);

    while (left + char_info -> rbearing < bbox -> x && *first != '\0')
    {
	left += char_info -> width;
	first++;
	char_info = per_char(font, (unsigned char)*first);
    }

    /* Find the character after the last visible character */
    last = first;
    right = left;

    while (right + char_info -> lbearing < bbox -> x + bbox -> width && *last != '\0')
    {
	right += char_info -> width;
	last++;
	char_info = per_char(font, (unsigned char)*last);
    }

#ifdef DEBUG_DRAW_STRING
    {
	XGCValues values;

	values.foreground = random();
	XChangeGC(display, gc, GCForeground, &values);
    }
#endif

    /* Draw the visible characters (if any) */
    if (*first != '\0')
    {
	XDrawString(display, drawable, gc, left, y, first, last - first);
    }
}

/* Draw the receiver */
static void do_paint(
    message_glyph_t self,
    Display *display, Drawable drawable,
    int x, int y, XRectangle *bbox)
{
    int do_underline = message_has_attachment(self -> message);
    int level = self -> fade_level;
    int baseline = y + self -> ascent;
    int left = x - MIN(self -> group_lbearing, 0);

#ifdef DEBUG_PER_CHAR
    /* Draw the message the slow way, offset by one pixel so we
     * can tell when we're misinterpreting the per_char information */
    {
	GC gc = ScGCForSeparator(self -> widget, level, bbox);
	char *string;
	int px = left;
	int py = baseline;

	/* Draw the strings the slow but easy way to make sure we
	 * measure the characters properly */
	string = message_get_group(self -> message);
	XDrawString(display, drawable, gc, px, py, string, strlen(string));
	px += self -> group_width;

	string = ":";
	XDrawString(display, drawable, gc, px, py, string, strlen(string));
	px += self -> separator_width;

	string = message_get_user(self -> message);
	XDrawString(display, drawable, gc, px, py, string, strlen(string));
	px += self -> user_width;

	string = ":";
	XDrawString(display, drawable, gc, px, py, string, strlen(string));
	px += self -> separator_width;

	string = message_get_string(self -> message);
	XDrawString(display, drawable, gc, px, py, string, strlen(string));

	/* Draw an underline */
	if (do_underline) {
	    XDrawLine(
		display, drawable, gc,
		left, baseline + 1,
		left + self -> group_width + self -> separator_width +
		self -> user_width + self -> separator_width + self -> string_width,
		baseline + 1);
	}
    }
#endif /* DEBUG_PER_CHAR */

#ifdef DEBUG_GLYPH
    {
	/* Draw a rectangle around the message */
	GC gc = ScGCForSeparator(self -> widget, level, bbox);
	XDrawRectangle(display, drawable, gc, x, y, get_width(self) - 1, bbox -> height - 1);
    }
#endif /* DEBUG_GLYPH */

    /* Draw the group string if it's visible */
    if ((bbox -> x <= left + self -> group_rbearing) &&
	(left + self -> group_lbearing <= bbox -> x + bbox -> width))
    {
	draw_string(
	    display, drawable,
	    ScGCForGroup(self -> widget, level, bbox),
	    ScFontForGroup(self -> widget),
	    left, baseline, bbox,
	    message_get_group(self -> message));
    }

    /* Underline it now to take advantage of GC caching */
    if (do_underline &&
	(bbox -> x <= left + self -> group_width) &&
	(left <= bbox -> x + bbox -> width))
    {
	XDrawLine(
	    display, drawable, 
	    ScGCForGroup(self -> widget, level, bbox),
	    MAX(left, bbox -> x), baseline + 1,
	    MIN(left + self -> group_width, bbox -> x + bbox -> width), baseline + 1);
    } 

    left += self -> group_width;

    /* Draw the separator string if it's visible */
    if ((bbox -> x <= left + self -> separator_rbearing) &&
	(left + self -> separator_lbearing <= bbox -> x + bbox -> width))
    {
	draw_string(
	    display, drawable,
	    ScGCForSeparator(self -> widget, level, bbox),
	    ScFontForSeparator(self -> widget),
	    left, baseline, bbox,
	    SEPARATOR);
    }

    /* Underline it now to take advantage of GC caching */
    if (do_underline &&
	(bbox -> x <= left + self -> separator_width) &&
	(left <= bbox -> x + bbox -> width))
    {
	XDrawLine(
	    display, drawable,
	    ScGCForSeparator(self -> widget, level, bbox),
	    MAX(left, bbox -> x), baseline + 1,
	    MIN(left + self -> separator_width, bbox -> x + bbox -> width), baseline + 1);
    }

    left += self -> separator_width;

    /* Draw the user string if's visible */
    if ((bbox -> x <= left + self -> user_rbearing) &&
	(left + self -> user_lbearing <= bbox -> x + bbox -> width))
    {
	draw_string(
	    display, drawable,
	    ScGCForUser(self -> widget, level, bbox),
	    ScFontForUser(self -> widget),
	    left, baseline, bbox,
	    message_get_user(self -> message));
    }

    /* Underline it now to take advantage of GC caching */
    if (do_underline &&
	(bbox -> x <= left + self -> user_width) &&
	(left <= bbox -> x + bbox -> width))
    {
	XDrawLine(
	    display, drawable,
	    ScGCForUser(self -> widget, level, bbox),
	    MAX(left, bbox -> x), baseline + 1,
	    MIN(left + self -> user_width, bbox -> x + bbox -> width), baseline + 1);
    }

    left += self -> user_width;

    /* Draw the separator string if it's visible */
    if ((bbox -> x <= left + self -> separator_rbearing) &&
	(left + self -> separator_lbearing <= bbox -> x + bbox -> width))
    {
	draw_string(
	    display, drawable,
	    ScGCForSeparator(self -> widget, level, bbox),
	    ScFontForSeparator(self -> widget),
	    left, baseline, bbox,
	    SEPARATOR);
    }

    /* Underline it now to take advantage of GC caching */
    if (do_underline &&
	(bbox -> x <= left + self -> separator_width) &&
	(left <= bbox -> x + bbox -> width))
    {
	XDrawLine(
	    display, drawable,
	    ScGCForSeparator(self -> widget, level, bbox),
	    MAX(left, bbox -> x), baseline + 1,
	    MIN(left + self -> separator_width, bbox -> x + bbox -> width), baseline + 1);

    }

    left += self -> separator_width;

    /* Draw the message string if appropriate */
    if ((bbox -> x <= left + self -> string_rbearing) &&
	(left + self -> separator_lbearing <= bbox -> x + bbox -> width))
    {
	draw_string(
	    display, drawable,
	    ScGCForString(self -> widget, level, bbox),
	    ScFontForString(self -> widget),
	    left, baseline, bbox,
	    message_get_string(self -> message));
    }

    /* Underline it now to take advantage of GC caching */
    if (do_underline &&
	(bbox -> x <= left + self -> string_width) &&
	(left <= bbox -> x + bbox -> width))
    {
	XDrawLine(
	    display, drawable,
	    ScGCForString(self -> widget, level, bbox),
	    MAX(left, bbox -> x), baseline + 1,
	    MIN(left + self -> string_width, bbox -> x + bbox -> width), baseline + 1);
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

/* Sets the receiver's replacement */
static void do_set_replacement(message_glyph_t self, glyph_t replacement)
{
    self -> replacement = replacement;
    replacement -> alloc(replacement);
}

/* Returns the receiver's (possibly nested) replacement */
static glyph_t do_get_replacement(message_glyph_t self)
{
    /* If we haven't been replaced then return ourself */
    if (self -> replacement == NULL)
    {
	return (glyph_t)self;
    }

    /* Otherwise return our replacement's replacement */
    return self -> replacement -> get_replacement(self -> replacement);
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
    self -> visible_count = 0;
    self -> alloc = (alloc_method_t)do_alloc;
    self -> free = (free_method_t)do_free;
    self -> get_message = (message_method_t)get_message;
    self -> get_width = (width_method_t)get_width;
    self -> paint = (paint_method_t)do_paint;
    self -> is_expired = (is_expired_method_t)get_is_expired;
    self -> expire = (expire_method_t)do_expire;
    self -> set_replacement = (set_replacement_method_t)do_set_replacement;
    self -> get_replacement = (get_replacement_method_t)do_get_replacement;

    self -> widget = widget;
    self -> message = message_alloc_reference(message);
    self -> replacement = NULL;
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
