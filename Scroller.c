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
static const char cvsid[] = "$Id: Scroller.c,v 1.14 1999/05/17 14:29:48 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "ScrollerP.h"
#include "MessageView.h"


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
	XtNstepSize, XtCStepSize, XtRDimension, sizeof(Dimension),
	offset(scroller.step), XtRImmediate, (XtPointer)1
    }
};
#undef offset


/*
 * Action declarations
 */
static void Menu(Widget widget, XEvent *event);
static void DecodeMime(Widget widget, XEvent *event);
static void Delete(Widget widget, XEvent *event);
static void Kill(Widget widget, XEvent *event);
static void Faster(Widget widget, XEvent *event);
static void Slower(Widget widget, XEvent *event);

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "menu", (XtActionProc)Menu },
    { "decodeMime", (XtActionProc)DecodeMime },
    { "delete", (XtActionProc)Delete },
    { "kill", (XtActionProc)Kill },
    { "faster", (XtActionProc)Faster },
    { "slower", (XtActionProc)Slower }
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: menu()\n<Btn2Down>: decodeMime()\n<Btn3Down>: delete()\n<Key>d: delete()\n<Key>x: kill()\n<Key>q: quit()\n<Key>-: slower()\n<Key>=: faster()"
};



/*
 * Method declarations
 */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args);
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes);
static void Rotate(ScrollerWidget self);
static void Redisplay(Widget widget, XEvent *event, Region region);
static void Paint(ScrollerWidget self);
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

/* Holder for MessageViews and Spacers */
struct view_holder
{
    view_holder_t next;
    MessageView view;
    unsigned int width;
    unsigned int previous_width;
};

static view_holder_t view_holder_alloc(MessageView view, unsigned int width);
static void view_holder_free(view_holder_t self);

static void CreateGC(ScrollerWidget self);
static Pixel *CreateFadedColors(Display *display, Colormap colormap,
				XColor *first, XColor *last, unsigned int levels);
static void StartClock(ScrollerWidget self);
static void StopClock(ScrollerWidget self);
static void SetClock(ScrollerWidget self);
static void Tick(XtPointer widget, XtIntervalId *interval);

static void AddMessageView(ScrollerWidget self, MessageView view);
static MessageView RemoveMessageView(ScrollerWidget self, MessageView view);
static void EnqueueViewHolder(ScrollerWidget self, view_holder_t view_holder);


/* Allocates and initializes new view_holder_t.  Returns a valid
 * view_holder_t if successful, NULL otherwise */
static view_holder_t view_holder_alloc(MessageView view, unsigned int width)
{
    view_holder_t self;

    if ((self = (view_holder_t) malloc(sizeof(struct view_holder))) == NULL)
    {
	return NULL;
    }

    self -> next = NULL;
    self -> width = width;

    if (view != NULL)
    {
	self -> view = MessageView_allocReference(view);
    }
    else
    {
	self -> view = NULL;
    }

    return self;
}

/* Frees a view_holder_t */
static void view_holder_free(view_holder_t self)
{
    if (self -> view)
    {
	MessageView_freeReference(self -> view);
    }

    free(self);
}


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
	ScStartTimer(self, 1000 / self -> scroller.frequency, Tick, (XtPointer) self);
    }
}


/* One interval has passed */
static void Tick(XtPointer widget, XtIntervalId *interval)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    Rotate(self);
    Paint(self);
    Redisplay((Widget)self, NULL, 0);
    SetClock(self);
}





/* Adds a MessageView to the receiver */
static void AddMessageView(ScrollerWidget self, MessageView view)
{
    List_addLast(self -> scroller.messages, view);
    MessageView_allocReference(view);
#ifdef DEBUG
    fprintf(stderr, "Added message view %p\n", view);
#endif /* DEBUG */

    /* Make sure the clock is running */
    StartClock(self);
}

/* Removes a MessageView from the receiver */
static MessageView RemoveMessageView(ScrollerWidget self, MessageView view)
{
    List_remove(self -> scroller.messages, view);
    MessageView_freeReference(view);

    return view;
}


/* Adds a view_holder_t to the receiver */
static void EnqueueViewHolder(ScrollerWidget self, view_holder_t holder)
{
    holder -> next = NULL;
    self -> scroller.visibleWidth += holder -> width;

    /* If the list is empty, then simply add this view_holder_t */
    if (self -> scroller.last_holder == NULL)
    {
	self -> scroller.holders = holder;
	self -> scroller.last_holder = holder;
	return;
    }

    /* Otherwise, append the view_holder_t to the end of the list */
    self -> scroller.last_holder -> next = holder;
    self -> scroller.last_holder = holder;
}

/* Removes a view_holder_t from the receiver */
static view_holder_t RemoveViewHolder(ScrollerWidget self, view_holder_t holder)
{
    view_holder_t probe;

    /* Short cut for removing the first view_holder_t */
    if (self -> scroller.holders == holder)
    {
	self -> scroller.holders = holder -> next;

	/* Watch for removal of the last view_holder_t */
	if (self -> scroller.last_holder == holder)
	{
	    self -> scroller.last_holder = NULL;
	}

	/* Update the spacer */
	if (holder == self -> scroller.spacer)
	{
	    self -> scroller.spacer = NULL;
	}

	/* Update the visible width */
	self -> scroller.visibleWidth -= holder -> width;
	return holder;
    }

    /* Otherwise we spin through the list looking for the holder */
    for (probe = self -> scroller.holders; probe != NULL; probe = probe -> next)
    {
	if (probe -> next == holder)
	{
	    probe -> next = holder -> next;

	    /* Watch for removal of the last view_holder_t */
	    if (self -> scroller.last_holder == holder)
	    {
		self -> scroller.last_holder = probe;
	    }

	    /* Update the spacer */
	    if (holder == self -> scroller.spacer)
	    {
		self -> scroller.spacer = NULL;
	    }

	    /* Update the visible width */
	    self -> scroller.visibleWidth -= holder -> width;
	    return holder;
	}
    }

    /* Didn't find the view_holder_t in the list.  Return NULL */
    return NULL;
}



/*
 * Semi-private methods
 */

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


/*
 * Method Definitions
 */

/* ARGSUSED */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    ScrollerWidget self = (ScrollerWidget) widget;

    self -> scroller.isStopped = True;
    self -> scroller.messages = List_alloc();
    self -> scroller.holders = NULL;
    self -> scroller.last_holder = NULL;
    self -> scroller.spacer = NULL;
    self -> scroller.offset = 0;
    self -> scroller.visibleWidth = 0;
    self -> scroller.nextVisible = 0;
    self -> scroller.realCount = 0;

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

    /* Ensure that we always have at least a blank view holder around */
    EnqueueViewHolder(self, view_holder_alloc(NULL, self -> core.width));
    self -> scroller.spacer = self -> scroller.last_holder;
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
    Paint(self);

    /* Allocate colors */
    self -> scroller.groupPixels = CreateFadedColors(
	display, colormap, &colors[1], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.userPixels = CreateFadedColors(
	display, colormap, &colors[2], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.stringPixels = CreateFadedColors(
	display, colormap, &colors[3], &colors[0], self -> scroller.fadeLevels);
    self -> scroller.separatorPixels = CreateFadedColors(
	display, colormap, &colors[4], &colors[0], self -> scroller.fadeLevels);
}


/* Dequeue a view_holder_t if it is no longer visible */
static void OutWithTheOld(ScrollerWidget self)
{
    view_holder_t holder;

    while ((holder = self -> scroller.holders) != NULL)
    {
	/* See if the first holder has scrolled off the left edge */
	if (self -> scroller.offset < holder -> width)
	{
	    return;
	}

	/* It has -- remove it from the queue */
	RemoveViewHolder(self, holder);

	/* Update the count of real views */
	if (holder -> view != NULL)
	{
	    self -> scroller.realCount--;
	}

	/* Free the view_holder_t */
	view_holder_free(holder);

	/* Reset the scroller offset to zero */
	self -> scroller.offset = 0;
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
	while ((next != NULL) && (MessageView_isTimedOut(next)))
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
		if (self -> core.width < last -> width + END_SPACING)
		{
		    width = END_SPACING;
		}
		else
		{
		    width = self -> core.width - last -> width;
		}
	    }

	    self -> scroller.nextVisible = 0;
	    holder = view_holder_alloc(NULL, width);
	    holder -> previous_width = lastWidth;
	    EnqueueViewHolder(self, holder);
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
    self -> scroller.offset += self -> scroller.step;

    OutWithTheOld(self);
    InWithTheNew(self);

    /* If no messages around then stop spinning */
    if ((self -> scroller.realCount == 0) && List_isEmpty(self -> scroller.messages))
    {
	StopClock(self);
    }
}


/* Repaints the view of each view_holder_t */
static void Paint(ScrollerWidget self)
{
    Widget widget = (Widget)self;
    view_holder_t holder;
    long x;

    /* Reset the pixmap to the background color */
    XFillRectangle(
	XtDisplay(widget), self -> scroller.pixmap, self -> scroller.backgroundGC,
	0, 0, self -> core.width, self -> core.height);

    /* Draw each visible view_holder_t */
    x = -self -> scroller.offset;
    for (holder = self -> scroller.holders; holder != NULL; holder = holder -> next)
    {
	if (holder -> view != NULL)
	{
	    MessageView_redisplay(holder -> view, self -> scroller.pixmap, x, 0);
	}

	x += holder -> width;
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
    view_holder_t spacer;

#if DEBUG
    fprintf(stderr, "Resize %p\n", widget);
#endif /* DEBUG */

    /* Resize our offscreen pixmap */
    XFreePixmap(XtDisplay(widget), self -> scroller.pixmap);
    self -> scroller.pixmap = XCreatePixmap(
	XtDisplay(widget), XtWindow(widget),
	self -> core.width, self -> core.height,
	self -> core.depth);

    /* Update the size of the spacer (and our visible width) */
    if ((self -> scroller.spacer) != NULL)
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
    Paint(self);
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


/* Locates the view_holder_t at the given offset (NULL if none) */
static view_holder_t ViewHolderAtOffset(ScrollerWidget self, int offset)
{
    long x = offset + self -> scroller.offset;
    view_holder_t probe;

    /* Watch out for negative offsets (these can happen with
     * click-to-focus and actions for keys) */
    if (x < 0)
    {
	return NULL;
    }

    /* Locate the matching view_holder_t */
    for (probe = self -> scroller.holders; probe != NULL; probe = probe -> next)
    {
	x -= probe -> width;
	if (x < 0)
	{
	    return probe;
	}
    }

    return NULL;
}

/* Answers the view_holder_t corresponding to the location of the last event */
static view_holder_t ViewHolderAtEvent(ScrollerWidget self, XEvent *event)
{
    switch (event -> type)
    {
	case KeyPress:
	case KeyRelease:
	{
	    XKeyEvent *keyEvent = (XKeyEvent *) event;
	    return ViewHolderAtOffset(self, keyEvent -> x);
	}

	case ButtonPress:
	case ButtonRelease:
	{
	    XButtonEvent *buttonEvent = (XButtonEvent *) event;
	    return ViewHolderAtOffset(self, buttonEvent -> x);
	}

	case MotionNotify:
	{
	    XMotionEvent *motionEvent = (XMotionEvent *) event;
	    return ViewHolderAtOffset(self, motionEvent -> x);
	}

	default:
	{
	    return NULL;
	}
    }
}

/* Answers the message under the mouse in the given event */
static MessageView MessageAtEvent(ScrollerWidget self, XEvent *event)
{
    view_holder_t holder;

    if ((holder = ViewHolderAtEvent(self, event)) == NULL)
    {
	return NULL;
    }

    return holder -> view;
}


/*
 * Action definitions
 */

/* Called when the button is pressed */
void Menu(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    MessageView view = MessageAtEvent(self, event);
    Message message = view ? MessageView_getMessage(view) : NULL;
    XtCallCallbackList((Widget)self, self -> scroller.callbacks, (XtPointer) message);
}

/* Spawn metamail to decode the Message's MIME attachment */
static void DecodeMime(Widget widget, XEvent *event)
{
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
}


/* Expires a Message in short order */
static void Delete(Widget widget, XEvent *event)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    MessageView view = MessageAtEvent(self, event);

    if (view != NULL)
    {
	MessageView_expire(view);
    }
}

/* Simple remove the message from the scroller NOW */
static void Kill(Widget widget, XEvent *event)
{
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
	RemoveViewHolder(self, holder);
	Resize(widget);
    }
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
    if (step >= 0)
    {
	self -> scroller.step = step;
    }
}





/*
 *Public methods
 */

/* Adds a Message to the receiver */
void ScAddMessage(ScrollerWidget self, Message message)
{
    AddMessageView(self, MessageView_alloc(self, message));
}


