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
static const char cvsid[] = "$Id: Scroller.c,v 1.24 1999/06/21 01:12:55 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "ScrollerP.h"
#include "glyph.h"


/* FIX THIS: should compute based on the width of some number of 'n's */
#define END_SPACING 30

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
static void Menu(Widget widget, XEvent *event);
static void DecodeMime(Widget widget, XEvent *event);
static void Expire(Widget widget, XEvent *event);
static void Delete(Widget widget, XEvent *event);
static void Faster(Widget widget, XEvent *event);
static void Slower(Widget widget, XEvent *event);

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "menu", (XtActionProc)Menu },
    { "decodeMime", (XtActionProc)DecodeMime },
    { "expire", (XtActionProc)Expire },
    { "delete", (XtActionProc)Delete },
    { "faster", (XtActionProc)Faster },
    { "slower", (XtActionProc)Slower }
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: menu()\n<Btn2Down>: decodeMime()\n<Btn3Down>: delete()\n<Key>d: expire()\n<Key>x: delete()\n<Key>q: quit()\n<Key>-: slower()\n<Key>=: faster()"
};



/*
 * Method declarations
 */
static void scroll(ScrollerWidget self, int offset);

static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args);
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes);
/*static void Rotate(ScrollerWidget self);*/
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
static int gap_width(unsigned int scroller_width, unsigned int glyphs_width)
{
    return (scroller_width < glyphs_width + END_SPACING) ?
	END_SPACING : scroller_width - glyphs_width;
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
void ScRepaintMessageView(ScrollerWidget self, MessageView view)
{
    printf("ScRepaintMessageView\n");
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
    self -> scroller.glyphs_width = 0;
    self -> scroller.next_glyphs_width = 0;
    self -> scroller.pending = gap_alloc(self);
    self -> scroller.pending_width = 0;
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

/* Update the left_glyph pointer when appropriate */
static void adjust_left(ScrollerWidget self, glyph_t right)
{
    /* Keep looping until we're down to the gap with nothing else to add */
    while (! (queue_is_empty(self -> scroller.glyphs) && queue_is_empty(self -> scroller.pending)))
    {
	glyph_t left = self -> scroller.left_glyph;
	int width = left -> get_width(left);

	/* If the left_glyph is too far to the left, then we must find 
	 * a new left_glyph */
	if (width <= self -> scroller.left_offset)
	{
/*	    printf("L is too far left!\n");*/

	    self -> scroller.left_glyph = left -> next;
	    self -> scroller.left_offset -= width;

	    /* If left_glyph was the gap and the glyphs_width needs
	     * adjusting, then now is a good time */
	    if ((left == self -> scroller.glyphs) &&
		(self -> scroller.glyphs_width != self -> scroller.next_glyphs_width))
	    {
		/* If the right_glyph is not the gap, then the gap is
		 * not visible and we can adjust the glyphs_width all
		 * we want */
		if (right != self -> scroller.glyphs)
		{
		    self -> scroller.glyphs_width = self -> scroller.next_glyphs_width;
		}
		/* Otherwise we can adjust it somewhat but we'll have
		 * to fiddle with the right_offset to keep things happy */
		else
		{
		    int current_width = gap_width(self -> core.width, self -> scroller.glyphs_width);
		    int next_width = gap_width(self -> core.width, self -> scroller.next_glyphs_width);

		    /* We can chop off up to right_offset pixels */
		    if (current_width - self -> scroller.right_offset <= next_width)
		    {
			self -> scroller.right_offset += next_width - current_width;
			self -> scroller.glyphs_width = self -> scroller.next_glyphs_width;
		    }
		    else
		    {
			printf("voonR\n");
		    }
		}
	    }
	}

	/* If the left_glyph is too far to the right, then we must
	 * find a new left_glyph */
	else if (self -> scroller.left_offset < 0)
	{
/*	    printf("L is too far right!\n");*/

	    /* If the left_glyph was the gap, then add any pending
	     * glyphs to the circular queue */
	    if (left == self -> scroller.glyphs)
	    {
		queue_append_queue(self -> scroller.glyphs, self -> scroller.pending);
		self -> scroller.next_glyphs_width += self -> scroller.pending_width;
		self -> scroller.pending_width = 0;
	    }

	    /* Locate the new leftmost glyph */
	    self -> scroller.left_glyph = left -> previous;
	    self -> scroller.left_offset += left -> previous -> get_width(left -> previous);
	}
	/* Otherwise, we're done */
	else
	{
	    return;
	}
    }
}

/* Update the right glyph pointer if appropriate and attempt to
 * resolve some constraints */
static void adjust_right(ScrollerWidget self, glyph_t left)
{
    /* Keep looping until we're down to the gap with nothing else to add */
    while (! (queue_is_empty(self -> scroller.glyphs) && queue_is_empty(self -> scroller.pending)))
    {
	glyph_t right = self -> scroller.right_glyph;
	int width = right -> get_width(right);

	/* If the right_glyph is too far to the right then we must
	 * find ourselves a new right_glyph */
	if (width <= self -> scroller.right_offset)
	{
/*	    printf("R is too far right!\n");*/

	    self -> scroller.right_glyph = right -> previous;
	    self -> scroller.right_offset -= width;

	    /* If the right_glyph was the gap and the glyphs_width
	     * needs adjusting, then now is a good time */
	    if ((right == self -> scroller.glyphs) &&
		(self -> scroller.glyphs_width != self -> scroller.next_glyphs_width))
	    {
		/* If the left_glyph is not the gap, then the gap is
		 * not visible and we can adjust the glyphs_width all
		 * we want */
		if (left != self -> scroller.glyphs)
		{
		    self -> scroller.glyphs_width = self -> scroller.next_glyphs_width;
		}
		/* Otherwise we can adjust it somewhat but we'll have
		 * to fiddle with the left_offset to keep things happy */
		else
		{
		    int current_width = gap_width(self -> core.width, self -> scroller.glyphs_width);
		    int next_width = gap_width(self -> core.width, self -> scroller.next_glyphs_width);

		    printf("current_width = %d, next_width = %d, min_width = %d\n",
			   current_width, next_width, current_width - self -> scroller.left_offset);

		    /* We can chop off up to left_offset pixels */
		    if (current_width - self -> scroller.left_offset <= next_width)
		    {
			self -> scroller.left_offset += next_width - current_width;
			self -> scroller.glyphs_width = self -> scroller.next_glyphs_width;
		    }
		    else
		    {
			printf("voonL\n");
		    }
		}
	    }
	}
	/* If the right_glyph is too far to the left then we must find
	 * ourselves a new right_glyph */
	else if (self -> scroller.right_offset < 0)
	{
/*	    printf("R is too far left!\n");*/

	    /* If the right_glyph was the last glyph in the queue
	     * then now is a good time to add the pending glyphs to
	     * the queue */
	    if (right == self -> scroller.glyphs -> previous)
	    {
		queue_append_queue(self -> scroller.glyphs, self -> scroller.pending);
		self -> scroller.next_glyphs_width += self -> scroller.pending_width;
		self -> scroller.pending_width = 0;
	    }

	    /* Locate the new rightmost glyph */
	    self -> scroller.right_glyph = right -> next;
	    self -> scroller.right_offset += right -> next -> get_width(right -> next);
	}
	/* Otherwise we're done */
	else
	{
	    return;
	}
    }
}

static void scroll_left(ScrollerWidget self, int offset)
{
    glyph_t left = self -> scroller.left_glyph;
    glyph_t right = self -> scroller.right_glyph;

    self -> scroller.left_offset += offset;
    self -> scroller.right_offset -= offset;

    /* Update the left_glyph and right_glyph pointers */
    adjust_left(self, right);
    adjust_right(self, left);

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
    glyph_t left = self -> scroller.left_glyph;
    glyph_t right = self -> scroller.right_glyph;

    self -> scroller.left_offset += offset;
    self -> scroller.right_offset -= offset;

    /* Update the left_glyph and right_glyph pointers */
    adjust_right(self, left);
    adjust_left(self, right);

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


#if 0
static void OutWithTheOld(ScrollerWidget self)
{
    /* Keep looping until we're down to just the gap */
    while (! queue_is_empty(self -> scroller.glyphs))
    {
	glyph_t left = self -> scroller.left_glyph;
	unsigned int width = left -> get_width(left);

	/* FIX THIS: assumes right-to-left scrolling */
	/* If we haven't scrolled anything off the left edge, then we're done */
	if (self -> scroller.offset < width)
	{
	    return;
	}

	/* If we've just dropped the gap off the left edge, then
	 * update the total width. */
	if (left == self -> scroller.glyphs)
	{
	    self -> scroller.glyphs_width = self -> scroller.next_glyphs_width;
	}

	/* Move the left_glyph pointer on to the next glyph and update 
	 * the offset. */
	self -> scroller.left_glyph = left -> next;
	self -> scroller.offset -= width;

	/* If the glyph has expired, then remove it from the queue,
	 * update the next glyphs_width and free it */
	if (left -> is_expired(left))
	{
	    self -> scroller.next_glyphs_width -= width;
	    queue_remove(left);
	    left -> free(left);
	}
    }
}

/* Enqueue a view_holder_t if there's space for one */
static void InWithTheNew(ScrollerWidget self)
{
    while (self -> scroller.visibleWidth - self -> scroller.offset < self -> core.width)
    {
	MessageView next;

	/* Find a message that hasn't expired */
	next = List_get(self -> scroller.messages, self -> scroller.nextVisible);
	while ((next != NULL) && (MessageView_isExpired(next)))
	{
	    RemoveMessageView(self, next);
	    next = List_get(self -> scroller.messages, self -> scroller.nextVisible);
	}
	self -> scroller.nextVisible++;

	/* If at end of list, add a spacer */
	if (next == NULL)
	{
	    view_holder_t last = self -> scroller.last_holder;
	    view_holder_t holder;
	    unsigned long lastWidth;
	    unsigned long width;

	    if (last == NULL)
	    {
		width = self -> core.width;
		lastWidth = 0;
	    }
	    else
	    {
		lastWidth = last -> width;

		/* This is (self -> core.width - lastWidth < END_SPACING)
		 * written so that we don't have negative numbers */
		if (self -> core.width < lastWidth + END_SPACING)
		{
		    width = END_SPACING;
		}
		else
		{
		    width = self -> core.width - lastWidth;
		}
	    }

	    self -> scroller.nextVisible = 0;
	    holder = view_holder_alloc(NULL, width);
	    holder -> previous_width = lastWidth;
	    EnqueueViewHolder(self, holder);
	    self -> scroller.spacer = holder;
	}
	else
	{
	    /* otherwise add message */
	    EnqueueViewHolder(self, view_holder_alloc(next, MessageView_getWidth(next)));
	    self -> scroller.realCount++;
	}
    }
}

/* Move the messages along */
static void Rotate(ScrollerWidget self)
{
    self -> scroller.left_offset += self -> scroller.step;
    self -> scroller.right_offset -= self -> scroller.step;

    /* Scroll the pixmap */
    XCopyArea(
	XtDisplay(self), self -> scroller.pixmap,
	self -> scroller.pixmap, self -> scroller.backgroundGC,
	self -> scroller.step, 0,
	self -> core.width - self -> scroller.step, self -> core.height,
	0, 0);

    /* Update any leftover bits */
    OutWithTheOld(self);
    InWithTheNew(self);

    /* If no messages showing then pause */
    if (queue_is_empty(self -> scroller.glyphs))
    {
	view_holder_t holder;
	StopClock(self);

	/* Remove all of the blank view_holders */
	while ((holder = DequeueViewHolder(self)) != NULL)
	{
	    view_holder_free(holder);
	}

	/* Insert a new blank one */
	holder = view_holder_alloc(NULL, self -> core.width);
	EnqueueViewHolder(self, holder);
	self -> scroller.spacer = holder;
	
	/* Reset the offset to 0, in case we're suffering from round-off error */
	self -> scroller.offset = 0;
	return;
    }

    /* Otherwise, update the ones on the rightmost edge */
    Paint(
	self, self -> core.width - self -> scroller.step, 0, 
	self -> scroller.step, self -> core.height);
}
#endif /* 0 */


/* Repaints the view of each view_holder_t */
static void Paint(ScrollerWidget self, int x, int y, unsigned int width, unsigned int height)
{
    Widget widget = (Widget)self;
    Display *display = XtDisplay(widget);
    glyph_t glyph;
    long end = x + width;
    long offset;
    long next;

    /* Reset this portion of the pixmap to the background color */
    XFillRectangle(
	display, self -> scroller.pixmap, self -> scroller.backgroundGC,
	x, y, width, height);

    /* Draw each visible glyph */
    glyph = self -> scroller.left_glyph;
    offset = -self -> scroller.left_offset;
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
#if 0
    ScrollerWidget self = (ScrollerWidget) widget;
    view_holder_t spacer;

#if DEBUG
    fprintf(stderr, "Resize %p\n", widget);
#endif /* DEBUG */

    /* If we haven't had a chance to realize the widget yet, then just replace the spacer */
    if (! XtIsRealized(widget))
    {
	/* Throw away our original spacer and replace it with a new one */
	view_holder_free(DequeueViewHolder(self));
	EnqueueViewHolder(self, view_holder_alloc(NULL, self -> core.width));
	self -> scroller.spacer = self -> scroller.last_holder;
	return;
    }

    /* Resize our offscreen pixmap */
    XFreePixmap(XtDisplay(widget), self -> scroller.pixmap);
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(widget), XtWindow(widget),
	self -> core.width, self -> core.height,
	self -> core.depth);

    /* Update the size of the spacer (and our visible width) */
    if ((spacer = self -> scroller.spacer) != NULL)
    {
	long width = self -> core.width - spacer -> previous_width;

	self -> scroller.visibleWidth -= spacer -> width;
	spacer -> width = (width < END_SPACING) ? END_SPACING : width;
	self -> scroller.visibleWidth += spacer -> width;
    }

    /* Make sure we have the right number of things visible */
    OutWithTheOld(self);
    InWithTheNew(self);

    /* Repaint our offscreen pixmap and copy it to the display*/
    Paint(self, 0, 0, self -> core.width, self -> core.height);
    Redisplay(widget, NULL, 0);
#endif /* 0 */
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
void Menu(Widget widget, XEvent *event)
{
    printf("menu\n");
#if 0
    ScrollerWidget self = (ScrollerWidget) widget;
    MessageView view = MessageAtEvent(self, event);
    Message message = view ? MessageView_getMessage(view) : NULL;
    XtCallCallbackList((Widget)self, self -> scroller.callbacks, (XtPointer) message);
#endif /* 0 */
}

/* Spawn metamail to decode the Message's MIME attachment */
static void DecodeMime(Widget widget, XEvent *event)
{
    printf("decode mime\n");
#if 0
    ScrollerWidget self = (ScrollerWidget) widget;
    MessageView view = MessageAtEvent(self, event);
    
    if (view)
    {
	MessageView_decodeMime(view);
    }
    else
    {
#ifdef DEBUG
	fprintf(stderr, "none\n");
#endif /* DEBUG */
    }
#endif /* 0 */
}


/* Expires a Message in short order */
static void Expire(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph;

    glyph = glyph_at_event(self, event);
    glyph -> expire(glyph);
}

/* Simple remove the message from the scroller NOW */
static void Delete(Widget widget, XEvent *event)
{
    printf("delete\n");
#if 0
    ScrollerWidget self = (ScrollerWidget) widget;
    view_holder_t holder;
    MessageView view;

    if ((holder = ViewHolderAtEvent(self, event)) == NULL)
    {
	return;
    }

    if ((view = holder -> view) != NULL)
    {
	MessageView_expireNow(view);
	view_holder_free(RemoveViewHolder(self, holder));
	Resize(widget);
    }
#endif /* 0 */
}

/* Scroll more quickly */
static void Faster(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    int step = self -> scroller.step + 1;
    self -> scroller.step = step;
}

/* Scroll more slowly */
static void Slower(Widget widget, XEvent *event)
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
    self -> scroller.pending_width += glyph -> get_width(glyph);

    /* Make sure the clock is running */
    StartClock(self);
}

/* Answers the width of the gap glyph */
int ScGapWidth(ScrollerWidget self)
{
    return gap_width(self -> core.width, self -> scroller.glyphs_width);
}

