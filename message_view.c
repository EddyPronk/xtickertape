/***********************************************************************

  Copyright (C) 2001-2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: message_view.c,v 2.29 2004/08/03 12:29:16 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* perror, snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* exit, free, malloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memset */
#endif
#ifdef HAVE_TIME_H
#include <time.h> /* localtime */
#endif
#if defined(HAVE_SYS_TIME_H) && defined(TM_IN_SYS_TIME)
#include <sys/time.h> /* struct tm */
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "replace.h"
#include "message.h"
#include "utf8.h"
#include "message_view.h"

#define SEPARATOR ":"
#define NOON_TIMESTAMP "12:00pm"
#define INDENT "  "
#define TIMESTAMP_FORMAT "%2d:%02d%s"
#define TIMESTAMP_SIZE 8

#if ! defined(MIN)
# define MIN(x, y) (((x) < (y)) ? (x) : (y))
#endif
#if ! defined(MAX)
# define MAX(x, y) (((x) > (y)) ? (x) : (y))
#endif


/* The structure of a message view */
struct message_view
{
    /* The message to be displayed */
    message_t message;

    /* The number of levels of indentation */
    long indent;

    /* The width of a single logical indent */
    long indent_width;

    /* Code set information */
    utf8_renderer_t renderer;

    /* Should the message be underlined? */
    Bool has_underline;

    /* The width of the longest timestamp (12:00pm) */
    long noon_width;

    /* The timestamp as a string */
    char timestamp[TIMESTAMP_SIZE];

    /* Dimensions of the time string */
    struct string_sizes timestamp_sizes;

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
    if (bottom < rect -> y || rect -> y + (long)rect -> height <= top)
    {
	return 0;
    }

    /* Check the horizontal */
    if (right < rect -> x || rect -> x + (long)rect -> width <= left)
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
    utf8_renderer_t renderer,
    char *string,
    Bool has_underline)
{
    XGCValues values;

    /* Is the string visible? */
    if (rect_overlaps(
	    bbox,
	    x + sizes -> lbearing, y - sizes -> ascent,
	    x + sizes -> rbearing, y + sizes -> descent) ||
	(has_underline && rect_overlaps(
	    bbox,
	    x, y - sizes -> ascent,
	    x + sizes -> width, y + sizes -> descent)))
    {
	/* Set the foreground color */
	/* FIX THIS: do we just assume that the font is set? */
	values.foreground = pixel;
	XChangeGC(display, gc, GCForeground, &values);

	/* Draw the string */
	utf8_renderer_draw_string(display, drawable, gc, renderer, x, y, bbox, string);

	/* Draw the underline */
	if (has_underline)
	{
	    utf8_renderer_draw_underline(
		renderer, display, drawable, gc,
		x, y, bbox, sizes -> width);
	}
    }
}


/* Allocates and initializes a message_view */
message_view_t message_view_alloc(
    message_t message,
    long indent,
    utf8_renderer_t renderer)
{
    message_view_t self;
    struct tm *timestamp;
    struct string_sizes sizes;

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

    /* Record the indentation and code set info */
    self -> indent = indent;
    self -> renderer = renderer;

    /* If the message has an attachment then compute the underline info */
    self -> has_underline = message_has_attachment(message);

    /* Get the message's timestamp */
    if ((timestamp = localtime(message_get_creation_time(message))) == NULL)
    {
	perror("localtime(): failed");
	exit(1);
    }

    /* Convert that into a string */
    snprintf(self -> timestamp, TIMESTAMP_SIZE,
	     TIMESTAMP_FORMAT,
	     ((timestamp -> tm_hour + 11) % 12) + 1,
	     timestamp -> tm_min,
	    timestamp -> tm_hour / 12 != 1 ? "am" : "pm");

    /* Measure the width of the string to use for noon */
    utf8_renderer_measure_string(renderer, NOON_TIMESTAMP, &sizes);
    self -> noon_width = sizes.width;

    /* Figure out how much to indent the message */
    utf8_renderer_measure_string(renderer, INDENT, &sizes);
    self -> indent_width = sizes.width;

    /* Measure the message's strings */
    utf8_renderer_measure_string(
	renderer, self -> timestamp, &self -> timestamp_sizes);
    utf8_renderer_measure_string(
	renderer, message_get_group(message), &self -> group_sizes);
    utf8_renderer_measure_string(
	renderer, message_get_user(message), &self -> user_sizes);
    utf8_renderer_measure_string(
	renderer, message_get_string(message), &self -> message_sizes);
    utf8_renderer_measure_string(
	renderer, SEPARATOR, &self -> separator_sizes);
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
void message_view_get_sizes(
    message_view_t self,
    int show_timestamp,
    string_sizes_t sizes_out)
{
    long lbearing = 0;
    long rbearing = 0;
    long width = 0;

    /* See if we're including the timestamp */
    if (show_timestamp)
    {
	/* Skip to the end of the timestamp string */
	width += self -> noon_width - self -> timestamp_sizes.width;

	/* And right-justify the timestamp */
	lbearing = MIN(lbearing, width + self -> timestamp_sizes.lbearing);
	rbearing = MAX(rbearing, width + self -> timestamp_sizes.rbearing);
	width += self -> timestamp_sizes.width + self -> indent_width;
    }

    /* Include the width of the indent */
    width += self -> indent * self -> indent_width;

    /* Start with the sizes of the group string */
    lbearing = MIN(lbearing, width + self -> group_sizes.lbearing);
    rbearing = MAX(rbearing, width + self -> group_sizes.rbearing);
    width += self -> group_sizes.width;

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
    sizes_out -> lbearing = lbearing;
    sizes_out -> rbearing = rbearing;
    sizes_out -> width = width;
    sizes_out -> ascent = self -> separator_sizes.ascent;
    sizes_out -> descent = self -> separator_sizes.descent;
}

/* Draws the message_view */
void message_view_paint(
    message_view_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    int show_timestamp,
    unsigned long timestamp_pixel,
    unsigned long group_pixel,
    unsigned long user_pixel,
    unsigned long message_pixel,
    unsigned long separator_pixel,
    long x, long y,
    XRectangle *bbox)
{
#if (DEBUG_PER_CHAR - 1) == 0
    XGCValues values;
    char *string;
    long px;
#endif /* DEBUG_PER_CHAR */

#if (DEBUG_PER_CHAR - 1) == 0
    /* Draw the message the slow way so that we can tell where we're
     * diverging from the server's interpretation of the font */ 
    values.foreground = separator_pixel;
    XChangeGC(display, gc, GCForeground, &values);
    px = x + 1;

    /* Draw the group string */
    string = message_get_group(self -> message);
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> group_sizes.width;

    /* Then the separator */
    string = SEPARATOR;
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> separator_sizes.width;

    /* Then the user string */
    string = message_get_user(self -> message);
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> user_sizes.width;

    /* Then the separator again */
    string = SEPARATOR;
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> separator_sizes.width;

    /* Then the message itself */
    string = message_get_string(self -> message);
    XDrawString(display, drawable, gc, px, y, string, strlen(string));
    px += self -> message_sizes.width;

    /* And finally draw the underline */
    if (underline_height)
    {
	XFillRectangle(display, drawable, gc, x, y + 2, px - x, underline_height);
    }
#endif /* DEBUG_PER_CHAR */

    /* Paint the timestamp */
    if (show_timestamp)
    {
	/* Go to the end of the timestamp */
	x += self -> noon_width;

	/* Draw the timestamp right justified */
	paint_string(
	    display, drawable, gc, timestamp_pixel,
	    x - self -> timestamp_sizes.width , y,
	    bbox, &self -> timestamp_sizes,
	    self -> renderer, self -> timestamp, False);

	/* Indent the next bit */
	x += self -> indent_width;
    }

    /* Indent */
    x += self -> indent * self -> indent_width;

    /* Paint the group string */
    paint_string(
	display, drawable, gc, group_pixel,
	x, y, bbox, &self -> group_sizes,
	self -> renderer, message_get_group(self -> message),
	self -> has_underline);
    x += self -> group_sizes.width;

    /* Paint the first separator */
    paint_string(
	display, drawable, gc, separator_pixel,
	x, y, bbox, &self -> separator_sizes,
	self -> renderer, SEPARATOR,
	self -> has_underline);
    x += self -> separator_sizes.width;

    /* Paint the user string */
    paint_string(
	display, drawable, gc, user_pixel,
	x, y, bbox, &self -> user_sizes,
	self -> renderer, message_get_user(self -> message),
	self -> has_underline);
    x += self -> user_sizes.width;

    /* Paint the second separator */
    paint_string(
	display, drawable, gc, separator_pixel,
	x, y, bbox, &self -> separator_sizes,
	self -> renderer, SEPARATOR,
	self -> has_underline);
    x += self -> separator_sizes.width;

    /* Paint the message string */
    paint_string(
	display, drawable, gc, message_pixel,
	x, y, bbox, &self -> message_sizes,
	self -> renderer, message_get_string(self -> message),
	self -> has_underline);
    x += self -> message_sizes.width;
}
