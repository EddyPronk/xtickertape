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
static const char cvsid[] = "$Id: message_view.c,v 2.1 2001/07/04 08:38:07 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "message.h"
#include "message_view.h"

#define SEPARATOR ":"
#if ! defined(MIN)
# define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
#if ! defined(MAX)
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif

static XCharStruct empty_char = { 0, 0, 0, 0, 0, 0 };

/* Answers the statistics to use for a given character in the font */
static XCharStruct *per_char(XFontStruct *font, int ch)
{
    unsigned int first = font -> min_char_or_byte2;
    unsigned int last = font -> max_char_or_byte2;
    XCharStruct *info;

    /* Is the character present? */
    if (first <= ch && ch <= last)
    {
	info = font -> per_char + ch - first;

	/* If the bounding box is non-zero then return the info */
	if (info -> width != 0)
	{
	    return info;
	}
    }

    /* Is the default character defined? */
    if (first <= font -> default_char && font -> default_char <= last)
    {
	/* Then use it */
	return font -> per_char + font -> default_char - first;
    }

    /* Fall back on a 0-width character */
    return &empty_char;
}


/* Measures all of the characters in a string */
static void measure_string(
    XFontStruct *font,
    char *string,
    string_sizes_t sizes)
{
    XCharStruct *info;
    unsigned char *point = (unsigned char *)string;
    long lbearing;
    long rbearing;
    long width;

    /* Watch out for empty strings */
    if (*point == '\0')
    {
	memset(sizes, 0, sizeof(struct string_sizes));
	return;
    }

    /* Start with the measurements of the first character */
    info = per_char(font, *point);
    lbearing = info -> lbearing;
    rbearing = info -> rbearing;
    width = info -> width;
    point++;

    while (*point != '\0')
    {
	XCharStruct *info = per_char(font, *point);
	lbearing = MIN(lbearing, width + (long)info -> lbearing);
	rbearing = MAX(rbearing, width + (long)info -> rbearing);
	width += info -> width;
	point++;
    }

    /* Record our findings */
    sizes -> lbearing = lbearing;
    sizes -> rbearing = rbearing;
    sizes -> width = width;
    sizes -> ascent = font -> ascent;
    sizes -> descent = font -> descent;
}

/* Draw a string within the bounding box, measuring the characters so
 * as to minimize bandwidth requirements */
static void draw_string(
    Display *display,
    Drawable drawable,
    GC gc,
    XFontStruct *font,
    int x, int y,
    XRectangle *bbox,
    char *string)
{
    XCharStruct *info;
    char *first, *last;
    long left, right;
    int ch;

    /* Find the first visible character */
    first = string;
    left = x;
    info = per_char(font, *(unsigned char *)first);
    while (left + info -> rbearing < bbox -> x)
    {
	left += info -> width;

	/* Bail if the we reach the end of the string without finding
	 * any visible characters */
	if ((ch = (unsigned char)*(++first)) == 0)
	{
	    return;
	}

	info = per_char(font, ch);
    }

    /* Find the character after the last visible one */
    last = first;
    right = left;
    while (*last != '\0' && right + info -> lbearing < bbox -> x + bbox -> width)
    {
	right += info -> width;
	last++;
	info = per_char(font, (unsigned char)*last);
    }

    /* Draw the visible characters */
    XDrawString(display, drawable, gc, left, y, first, last - first);
}



/* The structure of a message view */
struct message_view
{
    /* The message to be displayed */
    message_t message;

    /* The font to use when displaying the message */
    XFontStruct *font;

    /* Dimensions of the group string */
    struct string_sizes group_sizes;

    /* Dimensions of the user string */
    struct string_sizes user_sizes;

    /* Dimensions of the message string */
    struct string_sizes message_sizes;

    /* Dimensions of the separator string */
    struct string_sizes separator_sizes;
};



/* Answers non-zero if the XRectangle overlaps with the rectangular
 * region described by left, top, right and bottom */
static int rect_overlaps(XRectangle *rect, long left, long top, long right, long bottom)
{
    /* Check the vertical */
    if (bottom < rect -> y || rect -> y + (long)rect -> height < top)
    {
	return 0;
    }

    /* Check the horizontal */
    if (right < rect -> x || rect -> x + (long)rect -> width < left)
    {
	return 0;
    }

    return 1;
}

/* Draws a string in the appropriate color with optional underline */
static void paint_string(
    Display *display,
    Drawable drawable,
    GC gc,
    unsigned long pixel,
    long x, long y,
    XRectangle *bbox,
    string_sizes_t sizes,
    XFontStruct *font,
    char *string,
    int do_underline)
{
    XGCValues values;

    /* Is the string visible? */
    if (rect_overlaps(
	    bbox,
	    x + sizes -> lbearing, y - sizes -> ascent,
	    x + sizes -> rbearing, y + sizes -> descent))
    {
	/* Set the foreground color */
	/* FIX THIS: do we just assume that the font is set? */
	values.foreground = pixel;
	XChangeGC(display, gc, GCForeground, &values);

	/* Draw the string */
	draw_string(display, drawable, gc, font, x, y, bbox, string);
    }

    /* Bail out if we're not underlining */
    if (! do_underline)
    {
	return;
    }

    /* Is the underline visible? */
    if (rect_overlaps(bbox, x, y, x + sizes -> width, y))
    {
	/* Set the foreground color */
	values.foreground = pixel;
	XChangeGC(display, gc, GCForeground, &values);

	/* Draw the underline */
	XDrawLine(
	    display, drawable, gc,
	    MIN(x, bbox -> x), y + 1,
	    MAX(x + sizes -> width, (long)bbox -> x + (long)bbox -> width), y + 1);
    }
}


/* Allocates and initializes a message_view */
message_view_t message_view_alloc(
    message_t message,
    XFontStruct *font)
{
    message_view_t self;

    /* Allocate enough memory for the new message view */
    if ((self = (message_view_t)malloc(sizeof(struct message_view))) == NULL)
    {
	return NULL;
    }

    /* Initialize its fields to sane values */
    memset(self, 0, sizeof(struct message_view));

    /* Allocate a reference to the message */
    if ((self -> message = message_alloc_reference(message)) == NULL)
    {
	message_view_free(self);
	return NULL;
    }

    /* Record the font */
    self -> font = font;

    /* Measure the message's strings */
    measure_string(font, message_get_group(message), &self -> group_sizes);
    measure_string(font, message_get_user(message), &self -> user_sizes);
    measure_string(font, message_get_string(message), &self -> message_sizes);
    measure_string(font, SEPARATOR, &self -> separator_sizes);
    return self;
}

/* Frees a message_view */
void message_view_free(message_view_t self)
{
    /* Free our reference to the message */
    message_free(self -> message);

    /* Free the message_view itself */
    free(self);
}

/* Returns the message view's message */
message_t message_view_get_message(message_view_t self)
{
    return self -> message;
}

/* Returns the sizes of the message view */
void message_view_get_sizes(message_view_t self, string_sizes_t sizes)
{
    long lbearing;
    long rbearing;
    long width;

    /* Start with the sizes of the group string */
    lbearing = self -> group_sizes.lbearing;
    rbearing = self -> group_sizes.rbearing;
    width = self -> group_sizes.width;

    /* Adjust for the size of the first separator string */
    lbearing = MIN(lbearing, width + self -> separator_sizes.lbearing);
    rbearing = MAX(rbearing, width + self -> separator_sizes.rbearing);
    width += self -> separator_sizes.width;

    /* Adjust for the size of the user string */
    lbearing = MIN(lbearing, width + self -> user_sizes.lbearing);
    rbearing = MAX(rbearing, width + self -> user_sizes.rbearing);
    width += self -> user_sizes.width;

    /* Adjust for the size of the second separator again */
    lbearing = MIN(lbearing, width + self -> separator_sizes.lbearing);
    rbearing = MAX(rbearing, width + self -> separator_sizes.rbearing);
    width += self -> separator_sizes.width;

    /* Adjust for the size of the message string */
    lbearing = MIN(lbearing, width + self -> message_sizes.lbearing);
    rbearing = MAX(rbearing, width + self -> message_sizes.rbearing);
    width += self -> message_sizes.width;

    /* Export our results */
    sizes -> lbearing = lbearing;
    sizes -> rbearing = rbearing;
    sizes -> width = width;
    sizes -> ascent = self -> separator_sizes.ascent;
    sizes -> descent = self -> separator_sizes.ascent;
}

/* Draws the message_view */
void message_view_paint(
    message_view_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    unsigned long group_pixel,
    unsigned long user_pixel,
    unsigned long message_pixel,
    unsigned long separator_pixel,
    int x, int y,
    XRectangle *bbox)
{
    int do_underline = message_has_attachment(self -> message);
#ifdef DEBUG_PER_CHAR
    char *string;
    long px;

    /* Draw the message the slow way so that we can tell where we're
     * diverging from the server's interpretation of the font */ 
    values.foreground = separator_pixel;
    XChangeGC(display, gc, GCForeground, &values);
    px = x;

    /* Draw the group string */
    string = message_get_group(self -> message);
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> group_sizes.width;

    /* Then the separator */
    string = SEPARATOR;
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> separator_sizes.width;

    /* Then the user string */
    string = message_get_group(self -> message);
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> user_sizes.width;

    /* Then the separator again */
    string = SEPARATOR;
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> separator_sizes.width;

    /* And finally draw the underline */
    if (do_underline)
    {
	XDrawLine(display, drawable, gc, x, y + 1, px, y + 1);
    }
#endif /* DEBUG_PER_CHAR */

    /* Paint the group string */
    paint_string(
	display, drawable, gc, group_pixel,
	x, y, bbox, &self -> group_sizes,
	self -> font, message_get_group(self -> message), do_underline);
    x += self -> group_sizes.width;

    /* Paint the first separator */
    paint_string(
	display, drawable, gc, separator_pixel,
	x, y, bbox, &self -> separator_sizes,
	self -> font, SEPARATOR, do_underline);
    x += self -> separator_sizes.width;

    /* Paint the user string */
    paint_string(
	display, drawable, gc, user_pixel,
	x, y, bbox, &self -> user_sizes,
	self -> font, message_get_user(self -> message), do_underline);
    x += self -> user_sizes.width;

    /* Paint the second separator */
    paint_string(
	display, drawable, gc, separator_pixel,
	x, y, bbox, &self -> separator_sizes,
	self -> font, SEPARATOR, do_underline);
    x += self -> separator_sizes.width;

    /* Paint the message string */
    paint_string(
	display, drawable, gc, message_pixel,
	x, y, bbox, &self -> message_sizes,
	self -> font, message_get_string(self -> message), do_underline);
    x += self -> message_sizes.width;
}

