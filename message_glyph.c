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
static const char cvsid[] = "$Id: message_glyph.c,v 1.7 1999/08/19 05:04:59 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include "glyph.h"
#include "ScrollerP.h"

#define SPACING 10
#define SEPARATOR ":"
#define MAX(x, y) ((x > y) ? x : y)

typedef struct message_glyph *message_glyph_t;
struct message_glyph
{
    GLYPH_PREFIX

    /* The ScrollerWidget in which the receiver is displayed */
    ScrollerWidget widget;

    /* The Message which the receiver represents */
    Message message;

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

    /* The width of the receiver's group string */
    unsigned int group_width;

    /* The width of the receiver's user string */
    unsigned int user_width;

    /* The width of the receiver's message string */
    unsigned int string_width;

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
	duration = 60 * 1000 * Message_getTimeout(self -> message) / 
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
    Message_free(self -> message);

#ifdef DEBUG
    printf("message_glyph freed!\n");
#endif /* DEBUG */

    free(self);
}

/* Answers the receiver's Message */
static Message get_message(message_glyph_t self)
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
	SPACING;
}

/* Draws a String with an optional underline */
static void paint_string(
    message_glyph_t self,
    Display *display, Drawable drawable, GC gc,
    int left, int right, int y, char *string, int do_underline)
{
    /* Draw all of the characters of the string */
    XDrawString(display, drawable, gc, left, y, string, strlen(string));

    /* Draw the underline if appropriate */
    if (do_underline)
    {
	XDrawLine(display, drawable, gc, left, y + 1, right, y + 1);
    }
}

/* Draw the receiver */
static void do_paint(
    message_glyph_t self, Display *display, Drawable drawable,
    int offset, int w, int x, int y, unsigned int width, unsigned int height)
{
    int do_underline = Message_hasAttachment(self -> message);
    int max = x + width;
    int level = self -> fade_level;
    int bottom = y + self -> ascent;
    int left;
    int right;

    /* If the group string fits, draw it */
    left = offset;
    right = left + self -> group_width;
    if ((left <= max) && (x < right))
    {
	paint_string(
	    self, display, drawable, ScGCForGroup(self -> widget, level),
	    left, right, bottom, Message_getGroup(self -> message), do_underline);
    }

    /* If the separator fits, draw it */
    left = right;
    right = left + self -> separator_width;
    if ((left <= max) && (x < right))
    {
	paint_string(
	    self, display, drawable, ScGCForSeparator(self -> widget, level),
	    left, right, bottom, SEPARATOR, do_underline);
    }

    /* If the user string fits, draw it */
    left = right;
    right = left + self -> user_width;
    if ((left <= max) && (x < right))
    {
	paint_string(
	    self, display, drawable, ScGCForUser(self -> widget, level),
	    left, right, bottom, Message_getUser(self -> message), do_underline);
    }

    /* If the separator fits, draw it */
    left = right;
    right = left + self -> separator_width;
    if ((left <= max) && (x < right))
    {
	paint_string(
	    self, display, drawable, ScGCForSeparator(self -> widget, level),
	    left, right, bottom, SEPARATOR, do_underline);
    }

    /* If the message string fits, draw it */
    left = right;
    right = left + self -> string_width;
    if ((left <= max) && (x < right))
    {
	paint_string(
	    self, display, drawable, ScGCForString(self -> widget, level),
	    left, right, bottom, Message_getString(self -> message), do_underline);
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


/* Measures the width of a string in the given font */
static unsigned int measure_string(XFontStruct *font, char *string)
{
    unsigned int first = font -> min_char_or_byte2;
    unsigned int last = font -> max_char_or_byte2;
    unsigned int default_width;
    unsigned int width = 0;
    unsigned char *pointer;

    /* Make sure the font's default_char is valid */
    if ((first <= font -> default_char) && (font -> default_char <= last))
    {
	default_width = font -> per_char[font -> default_char].width;
    }
    /* Otherwise see if a space will work */
    else if ((first <= ' ') && (' ' <= last))
    {
	default_width = font -> per_char[' '].width;
    }
    /* abandon all hope and use max_width */
    else
    {
	default_width = font -> max_bounds.width;
    }

    /* Add up the character widths */
    for (pointer = (unsigned char *)string; *pointer != '\0'; pointer++)
    {
	unsigned char ch = *pointer;
	width += ((first <= ch) && (ch <= last)) ? font -> per_char[ch - first].width : default_width;
    }

    return width;
}



/* Allocates and initializes a new message_glyph glyph */
glyph_t message_glyph_alloc(ScrollerWidget widget, Message message)
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
    self -> message = Message_allocReference(message);
    self -> ref_count = 1;
    self -> has_expired = False;
    self -> fade_level = 0;

    /* Compute sizes of various strings */
    font = ScFontForSeparator(self -> widget);
    self -> separator_width = measure_string(font, SEPARATOR);
    self -> ascent = font -> ascent;

    font = ScFontForGroup(self -> widget);
    self -> group_width = measure_string(font, Message_getGroup(self -> message));
    self -> ascent = MAX(self -> ascent, font -> ascent);

    font = ScFontForString(self -> widget);
    self -> user_width = measure_string(font, Message_getUser(self -> message));
    self -> ascent = MAX(self -> ascent, font -> ascent);

    font = ScFontForString(self -> widget);
    self -> string_width = measure_string(font, Message_getString(self -> message));
    self -> ascent = MAX(self -> ascent, font -> ascent);

    set_clock(self);

    return (glyph_t) self;
}
