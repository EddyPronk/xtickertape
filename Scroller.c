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
static const char cvsid[] = "$Id: Scroller.c,v 1.74 1999/10/07 03:45:28 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "ScrollerP.h"
#include "glyph.h"


/* Minimum drag amount (in pixels) before we decide not `click' */
#define DRAG_DELTA 5

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

    /* XtCallbackList attachment_callbacks */
    {
	XtNattachmentCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(scroller.attachment_callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XtCallbackList kill_callbacks */
    {
	XtNkillCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(scroller.kill_callbacks), XtRCallback, (XtPointer)NULL
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
static void start_drag(Widget widget, XEvent *event);
static void drag(Widget widget, XEvent *event);
static void show_menu(Widget widget, XEvent *event);
static void show_attachment(Widget widget, XEvent *event);
static void expire(Widget widget, XEvent *event);
static void delete(Widget widget, XEvent *event);
static void kill(Widget widget, XEvent *event);
static void faster(Widget widget, XEvent *event);
static void slower(Widget widget, XEvent *event);

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "start-drag", (XtActionProc)start_drag },
    { "drag", (XtActionProc)drag },
    { "show-menu", (XtActionProc)show_menu },
    { "show-attachment", (XtActionProc)show_attachment },
    { "expire", (XtActionProc)expire },
    { "delete", (XtActionProc)delete },
    { "kill", (XtActionProc)kill },
    { "faster", (XtActionProc)faster },
    { "slower", (XtActionProc)slower }
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: start-drag()\n"
    "<Btn1Motion>: drag()\n"
    "<Btn1Up>: show-menu()\n"
    "<Btn2Down>: show-attachment()\n"
    "<Btn3Down>: expire()\n"
    "<Key>d: expire()\n"
    "<Key>x: delete()\n"
    "<Key>k: kill()\n"
    "<Key>q: quit()\n"
    "<Key>-: slower()\n"
    "<Key>+: faster()\n"
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
 * The structure of a glyph_holder
 */
struct glyph_holder
{
    /* The previous glyph_holder_t in the list */
    glyph_holder_t previous;

    /* The next glyph_holder_t in the list */
    glyph_holder_t next;

    /* The width (in pixels) of this holder's glyph */
    int width;

    /* This holder's glyph */
    glyph_t glyph;
};

/* Allocates and initializes a new glyph_holder */
glyph_holder_t glyph_holder_alloc(glyph_t glyph, int width)
{
    glyph_holder_t self;

    /* Allocate memory for the receiver */
    if ((self = (glyph_holder_t) malloc(sizeof(struct glyph_holder))) == NULL)
    {
	return NULL;
    }

    /* Initialize its contents to sane values */
    self -> previous = NULL;
    self -> next = NULL;
    self -> width = width;
    self -> glyph = glyph -> alloc(glyph);
    return self;
}

/* Frees the resources consumed by the receiver */
void glyph_holder_free(glyph_holder_t self)
{
    self -> previous = NULL;
    self -> next = NULL;
    self -> glyph -> free(self -> glyph);
    self -> glyph = NULL;
    free(self);
}

/* Answers the width of the holder's glyph */
int glyph_holder_width(glyph_holder_t self)
{
    return self -> width;
}

/* Paints the holder's glyph */
void glyph_holder_paint(
    glyph_holder_t self, Display *display, Drawable drawable,
    int offset, int x, int y, unsigned int width, unsigned int height)
{
    self -> glyph -> paint(
	self -> glyph, display, drawable,
	offset, self -> width, x, y, width, height);
}




/*
 * Private Methods
 */

static void CreateGC(ScrollerWidget self);
static Pixel *CreateFadedColors(Display *display, Colormap colormap,
				XColor *first, XColor *last, unsigned int levels);
static void EnableClock(ScrollerWidget self);
static void DisableClock(ScrollerWidget self);
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





/* Re-enables the timer */
static void EnableClock(ScrollerWidget self)
{
    if ((self -> scroller.timer == 0) && (self -> scroller.step != 0))
    {
#ifdef DEBUG
	fprintf(stderr, "clock enabled\n");
#endif /* DEBUG */	
	SetClock(self);
    }
}

/* Temporarily disables the timer */
static void DisableClock(ScrollerWidget self)
{
    if (self -> scroller.timer != 0)
    {
#ifdef DEBUG
	fprintf(stderr, "clock disabled\n");
#endif /* DEBUG */
	ScStopTimer(self, self -> scroller.timer);
	self -> scroller.timer = 0;
    }
}


/* Sets the timer if the clock isn't stopped */
static void SetClock(ScrollerWidget self)
{
    self -> scroller.timer = ScStartTimer(
	self, 1000L / self -> scroller.frequency, Tick, (XtPointer) self);
}


/* One interval has passed */
static void Tick(XtPointer widget, XtIntervalId *interval)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    /* Don't scroll if we're in the midst of a drag or if the scroller is stopped */
    self -> scroller.timer = 0;
    if (self -> scroller.step != 0)
    {
	scroll(self, self -> scroller.step);
	Redisplay((Widget)self, NULL, 0);
    }

    SetClock(self);
}



/*
 * Answers the width of the gap for the given scroller width and sum
 * of the widths of all glyphs
 */
static int gap_width(ScrollerWidget self, int last_width)
{
    return (self -> core.width < last_width + self -> scroller.min_gap_width) ?
	self -> scroller.min_gap_width : self -> core.width - last_width;
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


/*
 * Semi-private methods
 */

/* Answers the GC for the background */
GC ScGCForBackground(ScrollerWidget self)
{
    return self -> scroller.backgroundGC;
}

/* Sets up a GC with the given foreground color and clip region */
static GC SetUpGC(ScrollerWidget self, Pixel foreground, int x, int y, int width, int height)
{
    XGCValues values;

    /* Try to reuse the clip mask if at all possible */
    if (self -> scroller.clip_width == width)
    {
	values.foreground = foreground;
	values.clip_x_origin = x;
	XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground | GCClipXOrigin, &values);
    }
    else
    {
	XRectangle rectangle;

	rectangle.x = 0;
	rectangle.y = 0;
	rectangle.width = width;
	rectangle.height = y + height;

	/* Set up the clip mask */
	XSetClipRectangles(XtDisplay(self), self -> scroller.gc, x, 0, &rectangle, 1, Unsorted);
	self -> scroller.clip_width = width;

	/* Set up the foreground color */
	values.foreground = foreground;
	XChangeGC(XtDisplay(self), self -> scroller.gc, GCForeground, &values);
    }

    return self -> scroller.gc;
}

/* Answers a GC for displaying the Group field of a message at the given fade level */
GC ScGCForGroup(ScrollerWidget self, int level, int x, int y, int width, int height)
{
    return SetUpGC(self, self -> scroller.groupPixels[level], x, y, width, height);
}

/* Answers a GC for displaying the User field of a message at the given fade level */
GC ScGCForUser(ScrollerWidget self, int level, int x, int y, int width, int height)
{
    return SetUpGC(self, self -> scroller.userPixels[level], x, y, width, height);
}

/* Answers a GC for displaying the String field of a message at the given fade level */
GC ScGCForString(ScrollerWidget self, int level, int x, int y, int width, int height)
{
    return SetUpGC(self, self -> scroller.stringPixels[level], x, y, width, height);
}

/* Answers a GC for displaying the field separators at the given fade level */
GC ScGCForSeparator(ScrollerWidget self, int level, int x, int y, int width, int height)
{
    return SetUpGC(self, self -> scroller.separatorPixels[level], x, y, width, height);
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

/* Repaints the given glyph (if visible) */
void ScRepaintGlyph(ScrollerWidget self, glyph_t glyph)
{
    Display *display = XtDisplay((Widget) self);
    glyph_holder_t holder = self -> scroller.left_holder;
    int offset = 0 - self -> scroller.left_offset;

    /* Go through the visible glyphs looking for the one to paint */
    while (holder != NULL)
    {
	int width = glyph_holder_width(holder);

	if (holder -> glyph == glyph)
	{
	    glyph_holder_paint(
		holder, display, self -> scroller.pixmap,
		offset, offset, 0, width, self -> core.height);
	    Redisplay((Widget)self, NULL, 0);
	}

	holder = holder -> next;
	offset += width;
    }
}



/*
 * Method Definitions
 */
static int compute_min_gap_width(XFontStruct *font)
{
    unsigned int first = font -> min_char_or_byte2;
    unsigned int last = font -> max_char_or_byte2;

    /* Try to use the width of 8 'n' characters */
    if ((first <= 'n') && ('n' <= last))
    {
	return 3 * (font -> per_char + 'n' - first) -> width;
    }

    /* Try 8 default character widths */
    if ((first <= font -> default_char) && (font -> default_char <= last))
    {
	return 3 * (font -> per_char + font -> default_char - first) -> width;
    }

    /* Ok, how about spaces? */
    if ((first <= ' ') && (' ' <= last))
    {
	return 3 * (font -> per_char + ' ' - first) -> width;
    }

    /* All right, resort to 8 of some arbitrary character */
    return 3 * font -> per_char -> width;
}

/* ARGSUSED */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t gap;
    glyph_holder_t holder;

    /* Make sure we have a width */
    if (self -> core.width == 0)
    {
	self -> core.width = 400;
    }

    /* Record the height and width for future reference */
    self -> scroller.height = self -> scroller.font -> ascent + self -> scroller.font -> descent;

    /* Record the width of 8 'n' characters as the minimum gap width */
    self -> scroller.min_gap_width = compute_min_gap_width(self -> scroller.font);

    /* Make sure we have a height */
    if (self -> core.height == 0)
    {
	self -> core.height = self -> scroller.height;
    }

    gap = gap_alloc(self);
    holder = glyph_holder_alloc(gap, self -> core.width);

    /* Initialize the queue to only contain the gap with 0 offsets */
    self -> scroller.timer = 0;
    self -> scroller.is_stopped = True;
    self -> scroller.is_dragging = False;
    self -> scroller.left_holder = holder;
    self -> scroller.right_holder = holder;
    self -> scroller.left_offset = 0;
    self -> scroller.right_offset = 0;
    self -> scroller.gap = gap;
    self -> scroller.left_glyph = gap;
    self -> scroller.right_glyph = gap;
    self -> scroller.last_width = 0;
    self -> scroller.start_drag_x = 0;
    self -> scroller.last_x = 0;
    self -> scroller.clip_width = 0;
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

    /* Create a window and a couple of graphics contexts */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask, attributes);
    CreateGC(self);

    /* Create an offscreen pixmap */
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(self), XtWindow(self),
	self -> core.width, self -> scroller.height,
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




/* Add a glyph_holder to the left edge of the Scroller */
static void add_left_holder(ScrollerWidget self)
{
    glyph_t glyph;
    glyph_holder_t holder;
    int width;

    /* Find the next glyph to show */
    self -> scroller.left_glyph = self -> scroller.left_glyph -> previous;
    glyph = self -> scroller.left_glyph;

    /* We need to do magic for the gap */
    if (glyph == self -> scroller.gap)
    {
	glyph_holder_t left = self -> scroller.left_holder;
	glyph_t previous = glyph -> previous;

	/* Determine the width of the last glyph (or the width of
	 * the scroller if there are no glyphs to scroll on) */
	if (previous == self -> scroller.gap)
	{
	    width = gap_width(self, self -> core.width);
	}
	else
	{
	    width = gap_width(self, previous -> get_width(previous));
	}
	    
	/* If the next glyph is also the gap then just expand it */
	if (left -> glyph == self -> scroller.gap)
	{
	    left -> width += width;
	    self -> scroller.left_offset += width;
	    return;
	}

	holder = glyph_holder_alloc(glyph, width);
    }
    else
    {
	width = glyph -> get_width(glyph);
	holder = glyph_holder_alloc(glyph, width);
    }

    self -> scroller.left_holder -> previous = holder;
    holder -> next = self -> scroller.left_holder;
    self -> scroller.left_holder = holder;

    self -> scroller.left_offset += width;
}

/* Add a glyph_holder to the right edge */
static void add_right_holder(ScrollerWidget self)
{
    glyph_t glyph;
    glyph_holder_t holder;
    int width;

    /* Find the next glyph to show */
    self -> scroller.right_glyph = self -> scroller.right_glyph -> next;
    glyph = self -> scroller.right_glyph;

    /* We need to do some magic for the gap */
    if (glyph == self -> scroller.gap)
    {
	glyph_holder_t right = self -> scroller.right_holder;
	width = gap_width(self, glyph_holder_width(right));

	/* If the previous glyph is also the gap then just expand it */
	if (right -> glyph == self -> scroller.gap)
	{
	    right -> width += width;
	    self -> scroller.right_offset += width;
	    return;
	}
    }
    else
    {
	width = glyph -> get_width(glyph);
    }

    /* Create a glyph_holder and add it to the list */
    holder = glyph_holder_alloc(glyph, width);
    self -> scroller.right_holder -> next = holder;
    holder -> previous = self -> scroller.right_holder;
    self -> scroller.right_holder = holder;

    self -> scroller.right_offset += width;
}

/* Remove the glyph_holder at the left edge of the scroller */
static void remove_left_holder(ScrollerWidget self)
{
    glyph_holder_t holder = self -> scroller.left_holder;
    glyph_t glyph = holder -> glyph;

    /* Clean up the linked list */
    self -> scroller.left_holder = holder -> next;
    self -> scroller.left_holder -> previous = NULL;
    self -> scroller.left_offset -= glyph_holder_width(holder);
    glyph_holder_free(holder);

    /* Update the left_glyph if appropriate */
    if (glyph == self -> scroller.left_glyph)
    {
	glyph_holder_t probe = self -> scroller.left_holder;

	/* Find an unexpired glyph to become the new left_glyph */
	while (probe != NULL)
	{
	    if (! probe -> glyph -> is_expired(probe -> glyph))
	    {
		self -> scroller.left_glyph = probe -> glyph;
		return;
	    }

	    probe = probe -> next;
	}

	self -> scroller.left_glyph = self -> scroller.left_glyph -> next;
    }
}

/* Remove the glyph_holder at the right edge of the scroller */
static void remove_right_holder(ScrollerWidget self)
{
    glyph_holder_t holder = self -> scroller.right_holder;
    glyph_t glyph = holder -> glyph;

    /* Clean up the linked list */
    self -> scroller.right_holder = holder -> previous;
    self -> scroller.right_holder -> next = NULL;
    self -> scroller.right_offset -= glyph_holder_width(holder);
    glyph_holder_free(holder);

    /* Update the right_glyph if appropriate */
    if (glyph == self -> scroller.right_glyph)
    {
	glyph_holder_t probe = self -> scroller.right_holder;

	/* Find an unexpired glyph to become the new right_glyph */
	while (probe != NULL)
	{
	    if (! probe -> glyph -> is_expired(probe -> glyph))
	    {
		self -> scroller.right_glyph = probe -> glyph;
		return;
	    }

	    probe = probe -> previous;
	}

	self -> scroller.right_glyph = self -> scroller.right_glyph -> previous;
    }
}

/* Remove glyph_holders from the left edge if appropriate */
static void adjust_left_left(ScrollerWidget self)
{
    /* Keep going until we can do no more damage */
    while (self -> scroller.left_offset >= glyph_holder_width(self -> scroller.left_holder))
    {
	remove_left_holder(self);
    }
}

/* Add glyph_holders to the right edge if necessary */
static void adjust_left_right(ScrollerWidget self)
{
    /* Keep going until we've plugged the hole */
    while (self -> scroller.right_offset < 0)
    {
	add_right_holder(self);
    }
}

/* Add glyph_holders to the left edge if necessary */
static void adjust_right_left(ScrollerWidget self)
{
    /* Keep going until we've plugged the hole */
    while (self -> scroller.left_offset < 0)
    {
	add_left_holder(self);
    }
}

/* Remove glyph_holders from the right edge if appropriate */
static void adjust_right_right(ScrollerWidget self)
{
    /* Keep going until we can do no more damage */
    while (self -> scroller.right_offset >= glyph_holder_width(self -> scroller.right_holder))
    {
	remove_right_holder(self);
    }
}


/* Updates the state of the scroller after a shift of zero or more
 * pixels to the left. */
static void adjust_left(ScrollerWidget self)
{
    /* Add new glyph_holders if we don't have enough to get to the
     * right edge */
    adjust_left_right(self);

    /* Remove glyph holders that have scrolled off the left edge */
    adjust_left_left(self);

    /* Check for the magical stop condition */
    if ((self -> scroller.left_holder == self -> scroller.right_holder) &&
	(self -> scroller.left_holder -> glyph == self -> scroller.gap) &&
	(queue_is_empty(self -> scroller.gap)))
    {
	/* Tidy up and stop */
	self -> scroller.left_offset = 0;
	self -> scroller.right_offset = 0;
	self -> scroller.left_holder -> width = self -> core.width;
	self -> scroller.is_stopped = True;
	DisableClock(self);
    }
}

/* Updates the state of the scroller after a shift of zero or more
 * pixels to the right */
static void adjust_right(ScrollerWidget self)
{
    /* Add new glyph_holders if we don't have enough to get to the
     * left edge */
    adjust_right_left(self);

    /* Remove glyph holders that have scrolled off the right edge */
    adjust_right_right(self);

    /* Check for the magical stop condition */
    if ((self -> scroller.left_holder == self -> scroller.right_holder) &&
	(self -> scroller.right_holder -> glyph == self -> scroller.gap) &&
	(queue_is_empty(self -> scroller.gap)))
    {
	/* Tidy up and stop */
	self -> scroller.left_offset = 0;
	self -> scroller.right_offset = 0;
	self -> scroller.right_holder -> width = self -> core.width;
	self -> scroller.is_stopped = True;
	DisableClock(self);
    }
}


/* Scrolls the glyphs in the widget to the left */
static void scroll_left(ScrollerWidget self, int offset)
{
    self -> scroller.left_offset += offset;
    self -> scroller.right_offset -= offset;

    /* Update the glyph_holders list */
    adjust_left(self);

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

    /* Update the glyph_holders list */
    adjust_right(self);

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
    glyph_holder_t holder = self -> scroller.left_holder;
    int offset = 0 - self -> scroller.left_offset;
    int end = self -> core.width;

    /* Reset this portion of the pixmap to the background color */
    XFillRectangle(
	display, self -> scroller.pixmap, self -> scroller.backgroundGC,
	x, y, width, height);

    /* Draw each visible glyph */
    while (offset < end)
    {
	glyph_holder_paint(holder, display, self -> scroller.pixmap, offset, x, y, width, height);
	offset += glyph_holder_width(holder);
	holder = holder -> next;
    }

    /* If the internal state is inconsistent then let's bail right now */
    if (offset - self -> core.width != self -> scroller.right_offset)
    {
	fprintf(stderr, "*** Internal scroller state is inconsistent\n");
	fprintf(stderr, "*** Please alert Ted Phelps <phelps@pobox.com>\n");

	/* Force a core dump */
	abort();
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

    /* Nothing to do if we're not yet realized or if the width hasn't changed */
    if (! XtIsRealized(widget))
    {
	self -> scroller.left_holder -> width = self -> core.width;
	return;
    }

    /* Otherwise we need a new offscreen pixmap */
    XFreePixmap(XtDisplay(widget), self -> scroller.pixmap);
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(widget), XtWindow(widget),
	self -> core.width, self -> scroller.height,
	self -> core.depth);

    /* If the scroller is stalled, then we simply need to expand the gap */
    if (self -> scroller.is_stopped)
    {
	self -> scroller.left_holder -> width = self -> core.width;
	Paint(self, 0, 0, self -> core.width, self -> scroller.height);
	Redisplay(widget, NULL, 0);
	return;
    }

    /* Adjust the glyph_holders and offsets to compensate */
    if (self -> scroller.step < 0)
    {
	glyph_holder_t holder = self -> scroller.right_holder;
	int offset = glyph_holder_width(holder) - self -> scroller.right_offset;

	/* Look for a gap (but not the leading glyph) that we can adjust */
	holder = holder -> previous;
	while (holder != NULL)
	{
	    /* Did we find one? */
	    if (holder -> glyph == self -> scroller.gap)
	    {
		int new_width;

		/* Determine how wide the gap *should* be */
		if (holder -> previous != NULL)
		{
		    new_width = gap_width(self, glyph_holder_width(holder -> previous));
		}
		else
		{
		    glyph_t glyph = self -> scroller.left_glyph -> previous;

		    if (glyph != self -> scroller.gap)
		    {
			new_width = gap_width(self, glyph -> get_width(glyph));
		    }
		    else
		    {
			new_width = self -> core.width;
		    }
		}

		holder -> width = new_width;
	    }

	    offset += holder -> width;
	    holder = holder -> previous;
	}

	self -> scroller.left_offset = offset - self -> core.width;
	adjust_left_left(self);
	adjust_right_left(self);
    }
    else
    {
	glyph_holder_t holder = self -> scroller.left_holder;
	int offset = glyph_holder_width(holder) - self -> scroller.left_offset;

	/* Look for a gap (but not the leading glyph) that we can adjust */
	holder = holder -> next;
	while (holder != NULL)
	{
	    /* Did we find one? */
	    if (holder -> glyph == self -> scroller.gap)
	    {
		int new_width = gap_width(self, glyph_holder_width(holder -> previous));
		holder -> width = new_width;
	    }

	    offset += holder -> width;
	    holder = holder -> next;
	}

	self -> scroller.right_offset = offset - self -> core.width;
	adjust_right_right(self);
	adjust_left_right(self);
    }

    /* Update the display on that pixmap */
    Paint(self, 0, 0, self -> core.width, self -> scroller.height);
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


/* Answers the glyph_holder corresponding to the given point */
static glyph_holder_t holder_at_point(ScrollerWidget self, int x, int y)
{
    glyph_holder_t holder;
    int offset;

    /* Points outside the scroller bounds return NULL */
    if ((x < 0) || (self -> core.width <= x) || (y < 0) || (self -> core.height <= y))
    {
#ifdef DEBUG
	printf("out of bounds! (x=%d, y=%d)\n", x, y);
#endif /* DEBUG */
	return NULL;
    }

    /* Work from left-to-right looking for the glyph */
    offset = - self -> scroller.left_offset;
    for (holder = self -> scroller.left_holder; holder != NULL; holder = holder -> next)
    {
	offset += glyph_holder_width(holder);
	if (x < offset)
	{
	    return holder;
	}
    }

    /* Didn't find the point (!) so we return NULL */
    return NULL;
}

/* Answers the glyph_holder corresponding to the location of the given event */
static glyph_holder_t holder_at_event(ScrollerWidget self, XEvent *event)
{
    switch (event -> type)
    {
	case KeyPress:
	case KeyRelease:
	{
	    XKeyEvent *key_event = (XKeyEvent *) event;
	    return holder_at_point(self, key_event -> x, key_event -> y);
	}

	case ButtonPress:
	case ButtonRelease:

	{
	    XButtonEvent *button_event = (XButtonEvent *) event;
	    return holder_at_point(self, button_event -> x, button_event -> y);
	}

	case MotionNotify:
	{
	    XMotionEvent *motion_event = (XMotionEvent *) event;
	    return holder_at_point(self, motion_event -> x, motion_event -> y);
	}

	default:
	{
	    return NULL;
	}
    }
}

/* Answers the glyph corresponding to the location of the given event */
static glyph_t glyph_at_event(ScrollerWidget self, XEvent *event)
{
    glyph_holder_t holder = holder_at_event(self, event);

    /* If no holder found then return the gap */
    if (holder == NULL)
    {
	return self -> scroller.gap;
    }

    return holder -> glyph;
}



/*
 * Action definitions
 */

/* This should be called by each action function -- it makes sure that
 * dragging operates correctly.  It returns 0 if the action should
 * proceed normally, -1 if the action should be aborted (i.e.  it's
 * really the end of a drag operation */
static int pre_action(ScrollerWidget self, XEvent *event)
{
    /* Watch for a button release after a drag */
    if (event -> type == ButtonRelease)
    {
	if (self -> scroller.is_dragging)
	{
	    /* Restart the timer */
	    self -> scroller.is_dragging = False;
	    EnableClock(self);
	    return -1;
	}
    }

    /* Ignore everything else */
    return 0;
}


/* Called when the button is pressed */
void show_menu(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Pop up the menu and select the message that was clicked on */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(widget, self -> scroller.callbacks, (XtPointer)glyph -> get_message(glyph));
}

/* Spawn metamail to decode the message's attachment */
static void show_attachment(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Deliver the chosen message to the callbacks */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(
	widget, self -> scroller.attachment_callbacks,
	(XtPointer)glyph -> get_message(glyph));
}


/* Expires a message in short order */
static void expire(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Expire the glyph under the pointer */
    glyph = glyph_at_event(self, event);
    glyph -> expire(glyph);
}

/* Remove a glyph from the circular queue */
static void dequeue(ScrollerWidget self, glyph_t glyph)
{
    /* If the glyph is already dequeued, then don't do anything */
    if (glyph -> next == NULL)
    {
	return;
    }

    /* Make sure left_glyph is safe */
    if (self -> scroller.left_glyph == glyph)
    {
	self -> scroller.left_glyph = self -> scroller.left_glyph -> next;
    }

    /* Make sure that right_glyph is safe */
    if (self -> scroller.right_glyph == glyph)
    {
	self -> scroller.right_glyph = self -> scroller.right_glyph -> previous;
    }

    queue_remove(glyph);

    /* Free our reference to the glyph */
    glyph -> free(glyph);
}

/* Delete a message when scrolling left to right (backwards) */
static void delete_left_to_right(ScrollerWidget self, glyph_t glyph)
{
    glyph_holder_t holder = self -> scroller.right_holder;
    int offset = self -> core.width + self -> scroller.right_offset;
    int missing_width = 0;

    /* If we're deleting the leftmost (last) glyph then we add another
     * now so that we know what comes next */
    if (self -> scroller.left_holder -> glyph == glyph)
    {
	add_left_holder(self);
    }

    /* Go through the glyphs and compensate */
    while (holder != NULL)
    {
	/* Remember the previous glyph in case we delete the current one */
	glyph_holder_t previous = holder -> previous;

	/* If we've found the gap then insert any lost width into it */
	if ((holder -> glyph == self -> scroller.gap) && (missing_width != 0))
	{
	    holder -> width += missing_width;
	    missing_width = 0;
	}

	/* If we've found a holder for the deleted glyph then extract it now */
	if (holder -> glyph == glyph)
	{
	    missing_width += glyph_holder_width(holder);

	    /* Previous can never be NULL */
	    previous -> next = holder -> next;

	    /* Remove the holder from the list */
	    if (holder -> next == NULL)
	    {
		self -> scroller.right_holder = previous;
	    }
	    else
	    {
		holder -> next -> previous = previous;

		/* If the glyph was surrounded by gaps then join the gaps into one */
		if ((previous -> glyph == self -> scroller.gap) &&
		    (holder -> next -> glyph == self -> scroller.gap))
		{
		    offset -= previous -> width;
		    holder -> next -> width += previous -> width;
		    holder -> next -> previous = previous -> previous;
		    previous = previous -> previous;
		    glyph_holder_free(holder -> previous);

		    /* Update the previous pointer to skip the next glyph */
		    if (previous == NULL)
		    {
			self -> scroller.left_holder = holder -> next;
		    }
		    else
		    {
			previous -> next = holder -> next;
		    }
		}
	    }

	    glyph_holder_free(holder);
	}
	else
	{
	    offset -= holder -> width;
	}

	holder = previous;
    }

    self -> scroller.left_offset = -offset;
    adjust_right(self);
}

/* Delete a message when scrolling right to left (forwards) */
static void delete_right_to_left(ScrollerWidget self, glyph_t glyph)
{
    glyph_holder_t holder = self -> scroller.left_holder;
    int offset = - self -> scroller.left_offset;
    int missing_width = 0;

    /* If we're deleting the rightmost glyph, then we add another
     * now so that we know what comes next */
    if (self -> scroller.right_holder -> glyph == glyph)
    {
	add_right_holder(self);
    }

    /* Go through the glyphs and compensate */
    while (holder != NULL)
    {
	glyph_holder_t next = holder -> next;

	/* If we've found the gap then insert any lost width into it */
	if ((holder -> glyph == self -> scroller.gap) && (missing_width != 0))
	{
	    holder -> width += missing_width;
	    missing_width = 0;
	}

	/* If we've found a holder for the deleted glyph, then extract it now */
	if (holder -> glyph == glyph)
	{
	    missing_width += glyph_holder_width(holder);

	    /* We'll always have a next glyph */
	    next -> previous = holder -> previous;

	    /* Remove the holder from the list */
	    if (holder -> previous == NULL)
	    {
		self -> scroller.left_holder = next;
	    }
	    else
	    {
		holder -> previous -> next = next;

		/* If the glyph was surrounded by gaps, then join the gaps into one */
		if ((next -> glyph == self -> scroller.gap) &&
		    (holder -> previous -> glyph == self -> scroller.gap))
		{
		    offset += next -> width;
		    holder -> previous -> width += next -> width;
		    holder -> previous -> next = next -> next;
		    next = next -> next;
		    glyph_holder_free(holder -> next);

		    if (next == NULL)
		    {
			self -> scroller.right_holder = holder -> previous;
		    }
		    else
		    {
			next -> previous = holder -> previous;
		    }
		}
	    }

	    glyph_holder_free(holder);
	}
	else
	{
	    offset += holder -> width;
	}
	    
	holder = next;
    }

    self -> scroller.right_offset = offset - self -> core.width;
    adjust_left(self);
}

/* Delete the given glyph from the scroller */
static void delete_glyph(ScrollerWidget self, glyph_t glyph)
{
    /* Refuse to delete the gap */
    if (glyph == self -> scroller.gap)
    {
	return;
    }

    /* Mark the glyph as expired */
    glyph -> expire(glyph);

    /* Delete the glyph and any holders for that glyph.
     * Keep the leading edge of the text in place */
    if (self -> scroller.step < 0)
    {
	delete_left_to_right(self, glyph);
    }
    else
    {
	delete_right_to_left(self, glyph);
    }

    Paint(self, 0, 0, self -> core.width, self -> scroller.height);
    Redisplay((Widget) self, NULL, 0);
}


/* Simply remove the message from the scroller NOW */
static void delete(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    glyph_t glyph;

    /* Abort if the pre_action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }
    
    /* Figure out which glyph to delete */
    glyph = glyph_at_event(self, event);

    /* Ignore attempts to delete the gap */
    if (glyph == self -> scroller.gap)
    {
	return;
    }

    delete_glyph(self, glyph);
}

/* Kill a message and any replies */
static void kill(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    /* Abort if the pre_action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Figure out which glyph to kill */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(
	widget, self -> scroller.kill_callbacks,
	(XtPointer)glyph -> get_message(glyph));
}


/* Scroll more quickly */
static void faster(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Make sure the clock runs if we're scrolling and not if we're stationary */
    if (++self -> scroller.step != 0)
    {
	EnableClock(self);
    }
    else
    {
	DisableClock(self);
    }
}

/* Scroll more slowly */
static void slower(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Make sure the clock runs if we're scrolling and not if we're stationary */
    if (--self -> scroller.step != 0)
    {
	EnableClock(self);
    }
    else
    {
	DisableClock(self);
    }
}

/* Someone has pressed a mouse button */
static void start_drag(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    XButtonEvent *button_event = (XButtonEvent *)event;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* Record the postion of the click for future reference */
    self -> scroller.start_drag_x = button_event -> x;
    self -> scroller.last_x = button_event -> x;
}

/* Someone is dragging the mouse */
static void drag(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    XMotionEvent *motion_event = (XMotionEvent *) event;

    /* Abort if the pre-action tells us to */
    if (pre_action(self, event) < 0)
    {
	return;
    }

    /* If we're more than DELTA pixels from the position of the mouse
     * down event, then we must be dragging */
    if ((! self -> scroller.is_dragging) &&
	(self -> scroller.start_drag_x - DRAG_DELTA < motion_event -> x) &&
	(motion_event -> x < self -> scroller.start_drag_x + DRAG_DELTA))
    {
	return;
    }

    /* Ok, we're definitely dragging */
    self -> scroller.is_dragging = True;

    /* Disable the timer until the drag is done */
    DisableClock(self);

    /* Otherwise scroll by the difference */
    scroll(self, self -> scroller.last_x - motion_event -> x);
    self -> scroller.last_x = motion_event -> x;
    
    /* Repaint the widget */
    Redisplay(widget, NULL, 0);
}





/*
 *Public methods
 */

/* Adds a message to the receiver */
void ScAddMessage(ScrollerWidget self, message_t message)
{
    glyph_t glyph;
    glyph_holder_t holder;

    /* Create a glyph for the message */
    if ((glyph = message_glyph_alloc(self, message)) == NULL)
    {
	return;
    }

    /* Add it to the list of viable glyphs */
    queue_add(self -> scroller.gap, glyph);

    /* Adjust the gap width if possible and appropriate */
    holder = self -> scroller.left_holder;
    if ((self -> scroller.step < 0) && (holder -> glyph == self -> scroller.gap))
    {
	int width = gap_width(self, glyph -> get_width(glyph));

	/* If the effect is invisible, then just do it */
	if (holder -> width - self -> scroller.left_offset < width)
	{
	    self -> scroller.left_offset -= holder -> width - width;
	    holder -> width = width;
	}
	/* Otherwise we have to compromise */
	else
	{
	    holder -> width -= self -> scroller.left_offset;
	    self -> scroller.left_offset = 0;
	}
    }

    /* Make sure the clock is running */
    if (self -> scroller.is_stopped)
    {
	self -> scroller.is_stopped = False;
	EnableClock(self);
    }
}

/* Callback for expiring glyphs */
void ScGlyphExpired(ScrollerWidget self, glyph_t glyph)
{
#ifdef DEBUG
    printf("expired...\n");
#endif /* DEBUG */
    dequeue(self, glyph);
}

/* Answers the width of the gap glyph */
int ScGapWidth(ScrollerWidget self)
{
    return gap_width(self, self -> scroller.last_width);
}

/* Delete a message from the receiver */
void ScDeleteMessage(ScrollerWidget self, message_t message)
{
    glyph_t glyph;

    /* Make sure there is a message */
    if (message == NULL)
    {
	return;
    }

    /* Locate the message's glyph and delete it */
    for (glyph = self -> scroller.gap -> next; glyph != self -> scroller.gap; glyph = glyph -> next)
    {
	if (glyph -> get_message(glyph) == message)
	{
	    delete_glyph(self, glyph);
	    return;
	}
    }
}



