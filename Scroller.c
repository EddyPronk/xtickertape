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
static const char cvsid[] = "$Id: Scroller.c,v 1.32 1999/06/21 14:26:47 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "ScrollerP.h"
#include "glyph.h"


/* FIX THIS: should compute based on the width of some number of 'n's */
#define END_SPACING 30
#define TMP_PREFIX "/tmp/ticker"

/*
 * Resources
 */
#define offset(field) XtOffsetOf(ScrollerRec, field)

static XtResource resources[] =
{
    /* XtCallbackList callbacks */
    {
	XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(scroller.callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XFontStruct *font */
    {
	XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	offset(scroller.font), XtRString, XtDefaultFont
    },

    /* Pixel groupPixel */
    {
	XtNgroupPixel, XtCGroupPixel, XtRPixel, sizeof(Pixel),
	offset(scroller.groupPixel), XtRString, "Blue"
    },

    /* Pixel userPixel */
    {
	XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
	offset(scroller.userPixel), XtRString, "Green"
    },

    /* Pixel stringPixel */
    {
	XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
	offset(scroller.stringPixel), XtRString, "Red"
    },

    /* Pixel separatorPixel */
    {
	XtNseparatorPixel, XtCSeparatorPixel, XtRPixel, sizeof(Pixel),
	offset(scroller.separatorPixel), XtRString, XtDefaultForeground
    },

    /* Dimension fadeLevels */
    {
	XtNfadeLevels, XtCFadeLevels, XtRDimension, sizeof(Dimension),
	offset(scroller.fadeLevels), XtRImmediate, (XtPointer)5
    },

    /* Dimension frequency (in Hz) */
    {
	XtNfrequency, XtCFrequency, XtRDimension, sizeof(Dimension),
	offset(scroller.frequency), XtRImmediate, (XtPointer)24
    },

    /* Dimension step (in pixels) */
    {
	XtNstepSize, XtCStepSize, XtRPosition, sizeof(Position),
	offset(scroller.step), XtRImmediate, (XtPointer)1
    }
};
#undef offset


/*
 * Action declarations
 */
static void menu(Widget widget, XEvent *event);
static void decode_mime(Widget widget, XEvent *event);
static void expire(Widget widget, XEvent *event);
static void delete(Widget widget, XEvent *event);
static void faster(Widget widget, XEvent *event);
static void slower(Widget widget, XEvent *event);

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "menu", (XtActionProc)menu },
    { "decodeMime", (XtActionProc)decode_mime },
    { "expire", (XtActionProc)expire },
    { "delete", (XtActionProc)delete },
    { "faster", (XtActionProc)faster },
    { "slower", (XtActionProc)slower }
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: menu()\n<Btn2Down>: decodeMime()\n<Btn3Down>: expire()\n<Key>d: expire()\n<Key>x: delete()\n<Key>q: quit()\n<Key>-: slower()\n<Key>=: faster()"
};



/*
 * Method declarations
 */
static void scroll(ScrollerWidget self, int offset);

static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args);
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes);
static void Redisplay(Widget widget, XEvent *event, Region region);
static void Paint(ScrollerWidget self, int x, int y, unsigned int width, unsigned int height);
static void Destroy(Widget widget);
static void Resize(Widget widget);
static Boolean SetValues(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args);
static XtGeometryResult QueryGeometry(
    Widget widget,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred);

/*
 * Class record initialization
 */
ScrollerClassRec scrollerClassRec =
{
    /* core_class fields */
    {
	(WidgetClass) &widgetClassRec, /* superclass */
	"Scroller", /* class_name */
	sizeof(ScrollerRec), /* widget_size */
	NULL, /* class_initialize */
	NULL, /* class_part_initialize */
	False, /* class_inited */
	Initialize, /* initialize */
	NULL, /* initialize_hook */
	Realize, /* realize */
	actions, /* actions */
	XtNumber(actions), /* num_actions */
	resources, /* resources */
	XtNumber(resources), /* num_resources */
	NULLQUARK, /* xrm_class */
	True, /* compress_motion */
	True, /* compress_exposure */
	True, /* compress_enterleave */
	False, /* visible_interest */
	Destroy, /* destroy */
	Resize, /* resize */
	Redisplay, /* expose */
	SetValues, /* set_values */
	NULL, /* set_values_hook */
	XtInheritSetValuesAlmost, /* set_values_almost */
	NULL, /* get_values_hook */
	NULL, /* accept_focus */
	XtVersion, /* version */
	NULL, /* callback_private */
	defaultTranslations, /* tm_table */
	QueryGeometry, /* query_geometry */
	XtInheritDisplayAccelerator, /* display_accelerator */
	NULL /* extension */
    },

    /* Scroller class fields initialization */
    {
	0 /* ignored */
    }
};

WidgetClass scrollerWidgetClass = (WidgetClass)&scrollerClassRec;


/*
 * Private Methods
 */

static void CreateGC(ScrollerWidget self);
static Pixel *CreateFadedColors(Display *display, Colormap colormap,
				XColor *first, XColor *last, unsigned int levels);
static void StartClock(ScrollerWidget self);
static void StopClock(ScrollerWidget self);
static void SetClock(ScrollerWidget self);
static void Tick(XtPointer widget, XtIntervalId *interval);



/* Answers a GC with the right background color and font */
static void CreateGC(ScrollerWidget self)
{
    XGCValues values;

    values.font = self -> scroller.font -> fid;
    values.background = self -> core.background_pixel;
    values.foreground = self -> core.background_pixel;
    self -> scroller.backgroundGC = XCreateGC(
	XtDisplay(self), XtWindow(self), GCFont | GCBackground | GCForeground, &values);
    self -> scroller.gc = XCreateGC(
	XtDisplay(self), XtWindow(self), GCFont | GCBackground, &values);
}


/* Answers an array of colors fading from first to last */
static Pixel *CreateFadedColors(
    Display *display, Colormap colormap,
    XColor *first, XColor *last, unsigned int levels)
{
    Pixel *result = calloc(levels, sizeof(Pixel));
    long redNumerator = (long)last -> red - first -> red;
    long greenNumerator = (long)last -> green - first -> green;
    long blueNumerator = (long)last -> blue - first -> blue;
    long denominator = levels;
    long index;

    for (index = 0; index < levels; index++)
    {
	XColor color;

	color.red = first -> red + (redNumerator * index / denominator);
	color.green = first -> green + (greenNumerator * index / denominator);
	color.blue = first -> blue + (blueNumerator * index / denominator);
	color.flags = DoRed | DoGreen | DoBlue;
	XAllocColor(display, colormap, &color);
	result[index] = color.pixel;
    }

    return result;
}





/* Starts the clock if it isn't already going */
static void StartClock(ScrollerWidget self)
{
    if (self -> scroller.isStopped)
    {
#ifdef DEBUG
	fprintf(stderr, "restarting\n");
#endif /* DEBUG */
	fflush(stderr);
	self -> scroller.isStopped = False;
	SetClock(self);
    }
}

/* Stops the clock */
static void StopClock(ScrollerWidget self)
{
#ifdef DEBUG
    fprintf(stderr, "stalling\n");
#endif /* DEBUG */
    fflush(stderr);
    self -> scroller.isStopped = True;
}

/* Sets the timer if the clock isn't stopped */
static void SetClock(ScrollerWidget self)
{
    if (! self -> scroller.isStopped)
    {
	ScStartTimer(self, 1000L / self -> scroller.frequency, Tick, (XtPointer) self);
    }
}


/* One interval has passed */
static void Tick(XtPointer widget, XtIntervalId *interval)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    scroll(self, self -> scroller.step);
    Redisplay((Widget)self, NULL, 0);
    SetClock(self);
}



/*
 * Answers the width of the gap for the given scroller width and sum
 * of the widths of all glyphs
 */
static int gap_width(unsigned int scroller_width, unsigned int last_width)
{
    return (scroller_width < last_width + END_SPACING) ?
	END_SPACING : scroller_width - last_width;
}


/* Answers True if the queue contains only the gap */
static int queue_is_empty(glyph_t head)
{
    /* If the last element of the queue is also the first, then the
     * queue is empty */
    return head == head -> previous;
}

/* Adds an item to a circular queue of glyphs */
static void queue_add(glyph_t head, glyph_t glyph)
{
    glyph_t tail = head -> previous;

    glyph -> previous = tail;
    glyph -> next = head;
    tail -> next = glyph;
    head -> previous = glyph;
}

/* Removes an item from a circular queue of glyphs */
static void queue_remove(glyph_t glyph)
{
    glyph -> previous -> next = glyph -> next;
    glyph -> next -> previous = glyph -> previous;

    glyph -> previous = NULL;
    glyph -> next = NULL;
}

/* Adds all glyphs on the pending queue to the visible queue */
static void queue_append_queue(glyph_t head, glyph_t queue)
{
    glyph_t tail;

    /* If queue is empty, then we're done */
    if (queue_is_empty(queue))
    {
	return;
    }

    /* Locate our tail */
    tail = head -> previous;

    /* Tell head and the tail of the queue where to go */
    queue -> next -> previous = tail;
    queue -> previous -> next = head;

    /* Update our queue to point to the new elements */
    head -> previous = queue -> previous;
    tail -> next = queue -> next;

    /* Update the old queue's gap to point to itself */
    queue -> previous = queue;
    queue -> next = queue;
}


/*
 * Semi-private methods
 */

/* Answers the GC for the background */
GC ScGCForBackground(ScrollerWidget self)
{
    return self -> scroller.backgroundGC;
}

/* Answers a GC for displaying the Group field of a message at the given fade level */
GC ScGCForGroup(ScrollerWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> scroller.groupPixels[level];
    XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground, &values);
    return self -> scroller.gc;
}

/* Answers a GC for displaying the User field of a message at the given fade level */
GC ScGCForUser(ScrollerWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> scroller.userPixels[level];
    XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground, &values);
    return self -> scroller.gc;
}

/* Answers a GC for displaying the String field of a message at the given fade level */
GC ScGCForString(ScrollerWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> scroller.stringPixels[level];
    XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground, &values);
    return self -> scroller.gc;
}

/* Answers a GC for displaying the field separators at the given fade level */
GC ScGCForSeparator(ScrollerWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> scroller.separatorPixels[level];
    XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground, &values);
    return self -> scroller.gc;
}

/* Answers the XFontStruct to be use for displaying the group */
XFontStruct *ScFontForGroup(ScrollerWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> scroller.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *ScFontForUser(ScrollerWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> scroller.font;
}


/* Answers the XFontStruct to be use for displaying the string */
XFontStruct *ScFontForString(ScrollerWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> scroller.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *ScFontForSeparator(ScrollerWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> scroller.font;
}


/* Answers the number of fade levels */
Dimension ScGetFadeLevels(ScrollerWidget self)
{
    return self -> scroller.fadeLevels;
}

/* Sets a timer to go off in interval milliseconds */
XtIntervalId ScStartTimer(ScrollerWidget self, unsigned long interval,
			  XtTimerCallbackProc proc, XtPointer client_data)
{
    return XtAppAddTimeOut(
	XtWidgetToApplicationContext((Widget)self),
	interval, proc, client_data);
}

/* Stops a timer from going off */
void ScStopTimer(ScrollerWidget self, XtIntervalId timer)
{
     XtRemoveTimeOut(timer);
}

/* Repaints the given MessageView (if visible) */
void ScRepaintGlyph(ScrollerWidget self, glyph_t glyph)
{
    Display *display = XtDisplay((Widget) self);
    glyph_t probe = self -> scroller.left_glyph;
    int end = self -> core.width;
    int offset = -self -> scroller.left_offset;
    int next;

    /* Go through the visible glyphs and paint any which match glyph */
    while (offset < end)
    {
	next = offset + probe -> get_width(probe);
	if (probe == glyph)
	{
	    glyph -> paint(
		glyph, display, self -> scroller.pixmap,
		offset, offset, 0, next - offset, self -> core.height);
	}

	probe = probe -> next;
	offset = next;
    }
}



/*
 * Method Definitions
 */

/* ARGSUSED */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    self -> scroller.isStopped = True;

    /* Initialize the queue to only contain the gap with a 0 offset */
    self -> scroller.glyphs = gap_alloc(self);
    self -> scroller.left_glyph = self -> scroller.glyphs;
    self -> scroller.right_glyph = self -> scroller.glyphs;
    self -> scroller.last_width = 0;
    self -> scroller.pending = gap_alloc(self);
    self -> scroller.left_offset = 0;
    self -> scroller.right_offset = 0;

    /* Make sure we have a width */
    if (self -> core.width == 0)
    {
	self -> core.width = 400;
    }

    /* Make sure we have a height */
    if (self -> core.height == 0)
    {
	self -> core.height = self -> scroller.font -> ascent + self -> scroller.font -> descent;
    }
}

/* Realize the widget by creating a window in which to display it */
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    Display *display = XtDisplay(self);
    Colormap colormap = XDefaultColormapOfScreen(XtScreen(self));
    XColor colors[5];

    /* Initialize colors */
    colors[0].pixel = self -> core.background_pixel;
    colors[1].pixel = self -> scroller.groupPixel;
    colors[2].pixel = self -> scroller.userPixel;
    colors[3].pixel = self -> scroller.stringPixel;
    colors[4].pixel = self -> scroller.separatorPixel;
    XQueryColors(display, colormap, colors, 5);

    /* Create a window and couple graphics contexts */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask, attributes);
    CreateGC(self);

    /* Create an offscreen pixmap */
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(self), XtWindow(self),
	self -> core.width, self -> core.height,
	self -> core.depth);

    /* Clear the offscreen pixmap to the background color */
    Paint(self, 0, 0, self -> core.width, self -> core.height);

    /* Allocate colors */
    self -> scroller.separatorPixels = CreateFadedColors(
	display, colormap, &colors[4], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.groupPixels = CreateFadedColors(
	display, colormap, &colors[1], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.userPixels = CreateFadedColors(
	display, colormap, &colors[2], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.stringPixels = CreateFadedColors(
	display, colormap, &colors[3], &colors[0], self -> scroller.fadeLevels);
}



/*
 * The Scroller widget operates as follows:
 *
 * 1. messages are kept in a circular queue in the order in which they
 *    are received.
 *
 * 2. a message may appear on the scroller in two places, but only no
 *    overlapping portions may appear.  i.e. if 75% a message appears
 *    on the left edge the scroller then no more than 25% of that
 *    message may appear on the right edge of the scroller 
 *
 * 3. there is a gap glyph which is always in the circular queue.  The 
 *    width of this gap may be adjusted in order to accomodate rule 2, 
 *    but must have at least some minimum width.
 *
 * 4. during normal scrolling, there must be no `jumps.'  This means
 *     that expired messages (but not deleted ones) may not be removed 
 *     from the circular queue until they are no longer visible.  It
 *     also means that the width gap may not be adjusted if it is
 *     visible unless (a) it is either the leftmost or rightmost (or
 *     both) glyph on the scroller and (b) the adjustment in size can
 *     be compensated for by adjusting the appropriate offset.
 *
 * 5. New messages are not added in the middle of the scroller, but
 *     are instead scrolled into place between the last message and
 *     the gap.  Messages which cannot be immediately added to the
 *     circular queue because of this constraint are kept in a
 *     separate `pending' queue until they can be added.
 */

/* Answers the width of the last element on the queue or 0 if the queue is empty */
static int get_last_width(ScrollerWidget self)
{
    glyph_t last = self -> scroller.glyphs -> previous;

    /* Locate last unexpired glyph */
    while (last -> is_expired(last))
    {
	last = last -> previous;
    }

    return (last == self -> scroller.glyphs) ? 0 : last -> get_width(last);
}

/* Update the state of the scroller's left edge after a shift of zero
 * or more pixels to the left has been performed */
static void adjust_left_left(ScrollerWidget self)
{
    /* Keep looping until we either get an explicit return or we run
     * out of glyphs to play with */
    while (! queue_is_empty(self -> scroller.glyphs))
    {
	glyph_t left = self -> scroller.left_glyph;
	int width = left -> get_width(left);
	int last_width;

	/* If the left_glyph is still visible on the left side, then
	 * we're done */
	if (self -> scroller.left_offset < width)
	{
	    return;
	}

/*	printf("L is too far left\n");*/
	self -> scroller.left_glyph = left -> next;
	self -> scroller.left_offset -= width;

	/* If the left_glyph was the gap and the gap needs to grow
	 * then do so now */
	last_width = get_last_width(self);
	if ((left == self -> scroller.glyphs) && (self -> scroller.last_width < last_width))
	{
	    /* If the gap is still visible then it must be the
	     * right_glyph.  If it is then we'll need to adjust the
	     * right_offset in order to compensate for the shrinkage
	     * in the gap width */
	    if (self -> scroller.right_glyph == self -> scroller.glyphs)
	    {
		int next_width = gap_width(self -> core.width, last_width);
		self -> scroller.right_offset -= width - next_width;
	    }

	    self -> scroller.last_width = last_width;
	}
    }
}

/* Update the state of the scroller's right edge after a shift of zero
 * or more pixels to the left has been performed */
static void adjust_left_right(ScrollerWidget self)
{
    /* Keep looping until we either get an explicit return or we run
     * out of glyphs to play with */
    while (! (queue_is_empty(self -> scroller.glyphs) && queue_is_empty(self -> scroller.pending)))
    {
	glyph_t right = self -> scroller.right_glyph;
	int width = right -> get_width(right);
	int last_width;

	/* If the right_glyph is not too far left then we're done */
	if (0 <= self -> scroller.right_offset)
	{
	    return;
	}

/*	printf("R is too far left\n");*/

	/* If the right_glyph was the last glyph in the queue then add
	 * the pending glyphs */
	if (right == self -> scroller.glyphs -> previous)
	{
	    queue_append_queue(self -> scroller.glyphs, self -> scroller.pending);
	}

	/* If the right_glyph is the gap and the gap needs to grow
	 * then do so now */
	last_width = get_last_width(self);
	if ((right == self -> scroller.glyphs) && (self -> scroller.last_width > last_width))
	{
	    int next_width = gap_width(self -> core.width, last_width);

	    self -> scroller.right_offset += next_width - width;
	    self -> scroller.last_width = last_width;

	    if (self -> scroller.left_glyph == self -> scroller.glyphs)
	    {
		printf("LR: how did we get here?\n");
	    }
	}
	else
	{
	    glyph_t glyph;

	    /* Locate a new (unexpired!) right_glyph */
	    glyph = right -> next;
	    while (glyph -> is_expired(glyph))
	    {
		glyph_t next = glyph -> next;

		printf("deleting glyph %p\n", glyph);
		queue_remove(glyph);
		glyph -> free(glyph);
		glyph = next;
	    }

	    /* If the right_glyph is the gap then insert any pending glyphs now */
	    if (glyph == self -> scroller.glyphs)
	    {
		queue_append_queue(self -> scroller.glyphs, self -> scroller.pending);
	    }

	    glyph = right -> next;
	    self -> scroller.right_glyph = glyph;
	    self -> scroller.right_offset += glyph -> get_width(glyph);
	}
    }

    self -> scroller.left_offset = 0;
    self -> scroller.right_offset = 0;
    self -> scroller.last_width = get_last_width(self);
    StopClock(self);
}

/* Update the state of the scroller's right edge after a shift of zero
 * or more pixels to the right has been performed */
static void adjust_right_right(ScrollerWidget self)
{
    /* Keep looping until we either get an explicit return or we run
     * out of glyphs to play with */
    while (! queue_is_empty(self -> scroller.glyphs))
    {
	glyph_t right = self -> scroller.right_glyph;
	int width = right -> get_width(right);
	int last_width;

	/* If the right_glyph is still visible on the right side then
	 * we're done */
	if (self -> scroller.right_offset < width)
	{
	    return;
	}

/*	printf("R is too far right\n");*/
	self -> scroller.right_glyph = right -> previous;
	self -> scroller.right_offset -= width;

	/* If the right_glyph was the gap and the gap needs to shrink
	 * then do so now */
	last_width = get_last_width(self);
	if ((right == self -> scroller.glyphs) && (self -> scroller.last_width < last_width))
	{
	    /* If the gap is still visible then it must be the
	     * left_glyph.  If it is then we'll need to adjust the
	     * left_offset in order to compensate for the shrinkage in 
	     * the gap width */
	    if (self -> scroller.left_glyph == self -> scroller.glyphs)
	    {
		int next_width = gap_width(self -> core.width, last_width);
		self -> scroller.left_offset -= width - next_width;
	    }

	    self -> scroller.last_width = last_width;
	}
    }
}

/* Update the state of the scroller's left edge after a shift of zero
 * or more pixels to the right has been performed */
static void adjust_right_left(ScrollerWidget self)
{
    /* Keep looping until we either get an explicit return or we run
     * out of glyphs to play with */
    while (! (queue_is_empty(self -> scroller.glyphs) && queue_is_empty(self -> scroller.pending)))
    {
	glyph_t left = self -> scroller.left_glyph;
	int width = left -> get_width(left);
	int last_width;

	/* If the left_glyph is not too far right then we're done */
	if (0 <= self -> scroller.left_offset)
	{
	    return;
	}

	/* If the left_glyph is the gap then now is a good time to
	 * append any pending glyphs to the queue */
	if (left == self -> scroller.glyphs)
	{
	    queue_append_queue(self -> scroller.glyphs, self -> scroller.pending);
	}

	/* If the left_glyph is the gap and the gap needs to grow then do it now */
	last_width = get_last_width(self);
	if ((left == self -> scroller.glyphs) && (self -> scroller.last_width > last_width))
	{
	    int next_width = gap_width(self -> core.width, last_width);

	    self -> scroller.left_offset += next_width - width;
	    self -> scroller.last_width = last_width;

	    if (self -> scroller.right_glyph == self -> scroller.glyphs)
	    {
		printf("RL: how did we get here?\n");
	    }
	}
	else
	{
	    glyph_t glyph;

	    /* Locate a new (unexpired!) left_glyph */
	    glyph = left -> previous;
	    while ((glyph != self -> scroller.right_glyph) && (glyph -> is_expired(glyph)))
	    {
		glyph_t previous = glyph -> previous;

		printf("deleting glyph %p\n", glyph);
		queue_remove(glyph);
		glyph -> free(glyph);
		glyph = previous;
	    }

	    self -> scroller.left_glyph = glyph;
	    self -> scroller.left_offset += glyph -> get_width(glyph);
	}
    }

    self -> scroller.left_offset = 0;
    self -> scroller.right_offset = 0;
    self -> scroller.last_width = get_last_width(self);
    StopClock(self);
}



static void scroll_left(ScrollerWidget self, int offset)
{
    self -> scroller.left_offset += offset;
    self -> scroller.right_offset -= offset;

    /* Update the left_glyph and right_glyph pointers */
    adjust_left_left(self);
    adjust_left_right(self);

    /* Scroll the pixmap */
    XCopyArea(
	XtDisplay(self), self -> scroller.pixmap, self -> scroller.pixmap,
	self -> scroller.backgroundGC,
	0, 0, self -> core.width, self -> core.height, -offset, 0);

    /* Scroll the display and paint in the missing bits */
    Paint(self, self -> core.width - offset, 0, offset, self -> core.height);
}

/* Scrolls the glyphs in the widget to the right */
static void scroll_right(ScrollerWidget self, int offset)
{
    self -> scroller.left_offset += offset;
    self -> scroller.right_offset -= offset;

    /* Update the left_glyph and right_glyph pointers */
    adjust_right_right(self);
    adjust_right_left(self);

    /* Scroll the pixmap */
    XCopyArea(
	XtDisplay(self), self -> scroller.pixmap, self -> scroller.pixmap,
	self -> scroller.backgroundGC,
	0, 0, self -> core.width, self -> core.height, -offset, 0);

    /* Scroll the display and paint in the missing bits */
    Paint(self, 0, 0, -offset, self -> core.height);
}

/* Scrolls the glyphs in the widget by offset pixels */
static void scroll(ScrollerWidget self, int offset)
{
    if (offset < 0)
    {
	scroll_right(self, offset);
    }
    else
    {
	scroll_left(self, offset);
    }
}



/* Repaints the view of each view_holder_t */
static void Paint(ScrollerWidget self, int x, int y, unsigned int width, unsigned int height)
{
    Widget widget = (Widget)self;
    Display *display = XtDisplay(widget);
    glyph_t glyph = self -> scroller.left_glyph;
/*    int end = x + width;*/
    int end = self -> core.width;
    int offset = - self -> scroller.left_offset;
    int next;

    /* Reset this portion of the pixmap to the background color */
    XFillRectangle(
	display, self -> scroller.pixmap, self -> scroller.backgroundGC,
	x, y, width, height);

    /* Draw each visible glyph */
    while (offset < end)
    {
	next = offset + glyph -> get_width(glyph);
	if (x <= next)
	{
	    glyph -> paint(glyph, display, self -> scroller.pixmap, offset, x, y, width, height);
	}

	glyph = glyph -> next;
	offset = next;
    }

    if (offset - self -> core.width != self -> scroller.right_offset)
    {
	printf("offset=%d, right_offset=%d\n", offset, self -> scroller.right_offset);
    }

    if (glyph -> previous != self -> scroller.right_glyph)
    {
	printf("glyph=%p, right_glyph=%p\n", glyph, self -> scroller.right_glyph);
    }
}

/* Repaints the scroller */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    /* Copy the pixmap to the window */
    XCopyArea(
	XtDisplay(self), self -> scroller.pixmap,
	XtWindow(self), self -> scroller.backgroundGC,
	0, 0, self -> core.width, self -> core.height, 0, 0);
    XFlush(XtDisplay(widget));
}

/* FIX THIS: should actually do something? */
static void Destroy(Widget widget)
{
#ifdef DEBUG
    fprintf(stderr, "Destroy %p\n", widget);
#endif /* DEBUG */
}


/* Find the empty view and update its width */
static void Resize(Widget widget)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    /* Nothing to do if we're not yet realized */
    if (! XtIsRealized(widget))
    {
	return;
    }

    /* Otherwise we need a new pixmap */
    XFreePixmap(XtDisplay(widget), self -> scroller.pixmap);
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(widget), XtWindow(widget),
	self -> core.width, self -> core.height,
	self -> core.depth);

    /* Update the left_glyph and right_glyph pointers */
    adjust_left_left(self);
    adjust_left_right(self);

    /* Update the display on that pixmap */
    Paint(self, 0, 0, self -> core.width, self -> core.height);
    Redisplay(widget, NULL, 0);
}

/* What should this do? */
static Boolean SetValues(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args)
{
#ifdef DEBUG
    fprintf(stderr, "SetValues\n");
#endif /* DEBUG */
    return False;
}

/* What should this do? */
static XtGeometryResult QueryGeometry(
    Widget widget,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred)
{
#ifdef DEBUG
    fprintf(stderr, "QueryGeometry\n");
#endif /* DEBUG */

    return XtGeometryYes;
}


/* Answers the glyph corresponding to the given point */
static glyph_t glyph_at_point(ScrollerWidget self, int x, int y)
{
    glyph_t pointer;
    int left;
    int right;

    /* Points outside the scroller bounds return the gap glyph */
    if ((x < 0) || (self -> core.width <= x) || (y < 0) || (self -> core.height <= y))
    {
	printf("out of bounds! (x=%d, y=%d)\n", x, y);
	return self -> scroller.glyphs;
    }

    /* Work from left-to-right looking for the glyph */
    left = - self -> scroller.left_offset;
    for (pointer = self -> scroller.left_glyph;
	 (right = left + pointer -> get_width(pointer)) <= x;
	 pointer = pointer -> next)
    {
	left = right;
    }

    /* Found something.  Return it */
    return pointer;
}

/* Answers the glyph corresponding to the location of the last event */
static glyph_t glyph_at_event(ScrollerWidget self, XEvent *event)
{
    switch (event -> type)
    {
	case KeyPress:
	case KeyRelease:
	{
	    XKeyEvent *key_event = (XKeyEvent *) event;
	    return glyph_at_point(self, key_event -> x, key_event -> y);
	}

	case ButtonPress:
	case ButtonRelease:
	{
	    XButtonEvent *button_event = (XButtonEvent *) event;
	    return glyph_at_point(self, button_event -> x, button_event -> y);
	}

	case MotionNotify:
	{
	    XMotionEvent *motion_event = (XMotionEvent *) event;
	    return glyph_at_point(self, motion_event -> x, motion_event -> y);
	}

	default:
	{
	    return NULL;
	}
    }
}


/* Answers the message under the mouse in the given event */
#if 0
static MessageView MessageAtEvent(ScrollerWidget self, XEvent *event)
{
    view_holder_t holder;

    if ((holder = ViewHolderAtEvent(self, event)) == NULL)
    {
	return NULL;
    }

    return holder -> view;
}
#endif /* 0 */


/*
 * Action definitions
 */

/* Called when the button is pressed */
void menu(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph = glyph_at_event(self, event);

    XtCallCallbackList(widget, self -> scroller.callbacks, (XtPointer)glyph -> get_message(glyph));
}

/* Spawn metamail to decode the Message's MIME attachment */
static void decode_mime(Widget widget, XEvent *event)
{
#ifdef METAMAIL
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph = glyph_at_event(self, event);
    Message message = glyph -> get_message(glyph);
    char filename[sizeof(TMP_PREFIX) + 16];
    char *mime_type;
    char *mime_args;
    char *buffer;
    FILE *file;

    /* If there's no message, then we can't decode any mime */
    if (message == NULL)
    {
#ifdef DEBUG
	printf("missed!\n");
#endif /* DEBUG */
	return;
    }

    /* Make sure there's a mime type and args */
    if (((mime_type = Message_getMimeType(message)) == NULL) ||
	((mime_args = Message_getMimeArgs(message)) == NULL))
    {
#ifdef DEBUG
	printf("no mime\n");
#endif /* DEBUG */
	return;
    }

#ifdef DEBUG
    printf("MIME: %s %s\n", mime_type, mime_args);
#endif /* DEBUG */

    /* Write the mime_args to a file (assumes 16-digit pids) */
    sprintf(filename, "%s%d", TMP_PREFIX, getpid());

    /* If we can't open the file then print an error and give up */
    if ((file = fopen(filename,"wb")) == NULL)
    {
	fprintf(stderr, "*** unable to open temporary file %s\n", filename);
	return;
    }

    fputs(mime_args, file);
    fclose(file);

    /* Invoke metamail to display the message */
    buffer = (char *) malloc(
	sizeof(METAMAIL) + sizeof(" -B -q -b -c   > /dev/null 2>&1")
	+ strlen(mime_type) + strlen(filename));

    sprintf(buffer, "%s -B -q -b -c %s %s > /dev/null 2>&1",
	    METAMAIL, mime_type, filename);
    system(buffer);

    /* Remove the temporary file */
    unlink(filename);
    free(buffer);
#endif /* METAMAIL */
}


/* Expires a Message in short order */
static void expire(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph = glyph_at_event(self, event);

    glyph -> expire(glyph);
}

/* Simple remove the message from the scroller NOW */
static void delete(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph = glyph_at_event(self, event);

    /* If the glyph is the gap then don't bother */
    if (glyph == self -> scroller.glyphs)
    {
	printf("missed\n");
	return;
    }

    /* Figure out which way we're scrolling to determine what to do */
    if (self -> scroller.step < 0)
    {
	printf("not yet implemented\n");
    }
    else
    {
	/* Scrolling right-to-left */
	glyph_t left = self -> scroller.left_glyph;
	glyph_t right = self -> scroller.right_glyph;

	/* If we've deleted the left_glyph, then update */
	if (glyph == left)
	{
	    self -> scroller.left_glyph = left -> next;
	}

	/* If we've deleted the right_glyph then roll back */
	if (glyph == right)
	{
	    self -> scroller.right_glyph = right -> previous;
	}

	/* Roll the right_offset back the width of the deleted glyph */
	self -> scroller.right_offset -= glyph -> get_width(glyph);

	/* Remove the deleted glyph from the queue */
	queue_remove(glyph);
	glyph -> free(glyph);
	self -> scroller.last_width = get_last_width(self);

	/* Attempt to recover */
	adjust_left_left(self);
	adjust_left_right(self);
    }

    /* Repaint everything */
    Paint(self, 0, 0, self -> core.width, self -> core.height);
    Redisplay(widget, NULL, 0);
}

/* Scroll more quickly */
static void faster(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    int step = self -> scroller.step + 1;
    self -> scroller.step = step;
}

/* Scroll more slowly */
static void slower(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    int step = self -> scroller.step -1;
    self -> scroller.step = step;
}





/*
 *Public methods
 */

/* Adds a Message to the receiver */
void ScAddMessage(ScrollerWidget self, Message message)
{
    glyph_t glyph;

    /* Create a glyph for the message */
    if ((glyph = message_glyph_alloc(self, message)) == NULL)
    {
	return;
    }

    /* Add it to the list of pending glyphs */
    queue_add(self -> scroller.pending, glyph);

    /* Make sure the clock is running */
    StartClock(self);
}

/* Answers the width of the gap glyph */
int ScGapWidth(ScrollerWidget self)
{
    return gap_width(self -> core.width, self -> scroller.last_width);
}


