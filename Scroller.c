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
static const char cvsid[] = "$Id: Scroller.c,v 1.11 1999/04/25 02:04:13 phelps Exp $";
#endif /* lint */

#include <config.h>
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
typedef struct ViewHolder_t
{
    MessageView view;
    unsigned int width;
    unsigned int previous_width;
} *ViewHolder;

static ViewHolder ViewHolder_alloc(MessageView view, unsigned int width);
static void ViewHolder_free(ViewHolder self);
static MessageView ViewHolder_view(ViewHolder self);
static void ViewHolder_redisplay(ViewHolder self, void *context[]);

static void CreateGC(ScrollerWidget self);
static Pixel *CreateFadedColors(Display *display, Colormap colormap,
				XColor *first, XColor *last, unsigned int levels);
static void StartClock(ScrollerWidget self);
static void StopClock(ScrollerWidget self);
static void SetClock(ScrollerWidget self);
static void Tick(XtPointer widget, XtIntervalId *interval);

static void AddMessageView(ScrollerWidget self, MessageView view);
static MessageView RemoveMessageView(ScrollerWidget self, MessageView view);
static void EnqueueViewHolder(ScrollerWidget self, ViewHolder holder);
static ViewHolder DequeueViewHolder(ScrollerWidget self);


/* Allocates a new ViewHolder */
static ViewHolder ViewHolder_alloc(MessageView view, unsigned int width)
{
    ViewHolder self = (ViewHolder) malloc(sizeof(struct ViewHolder_t));
    self -> view = view;
    self -> width = width;

    if (view)
    {
	MessageView_allocReference(view);
    }

    return self;
}

/* Frees a ViewHolder */
static void ViewHolder_free(ViewHolder self)
{
    if (self -> view)
    {
	MessageView_freeReference(self -> view);
    }

    free(self);
}


/* Answers the receiver's MessageView */
static MessageView ViewHolder_view(ViewHolder self)
{
    return self -> view;
}

/* Displays a ViewHolder */
static void ViewHolder_redisplay(ViewHolder self, void *context[])
{
    ScrollerWidget widget = (ScrollerWidget) context[0];
    Drawable drawable = (Drawable) context[1];
    int x = (int) context[2];
    int y = (int) context[3];

    if (self -> view != NULL)
    {
	MessageView_redisplay(self -> view, drawable, x, y);
    }
    else /* Fill the background color */
    {
	XFillRectangle(
	    XtDisplay(widget), drawable, widget -> scroller.backgroundGC,
	    x, y, self -> width, widget -> core.height);
    }

    context[2] = (void *)(x + self -> width);
    XFlush(XtDisplay(widget));
}

/* Locates the Message at the given offset */
static int ViewHolder_locate(ViewHolder self, long *x)
{
    if (*x < 0)
    {
	return 0;
    }

    *x -= self -> width;

    return (*x < 0);
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


/* Adds a ViewHolder to the receiver */
static void EnqueueViewHolder(ScrollerWidget self, ViewHolder holder)
{
    List_addLast(self -> scroller.holders, holder);
    self -> scroller.visibleWidth += holder -> width;
}

/* Removes a ViewHolder from the receiver */
static ViewHolder RemoveViewHolder(ScrollerWidget self, ViewHolder holder)
{
    /* Remove the holder from the list (and make sure it was there) */
    if (List_remove(self -> scroller.holders, holder) == NULL)
    {
	return NULL;
    }

    self -> scroller.visibleWidth -= holder -> width;
    return holder;
}

/* Removes a ViewHolder from the receiver */
static ViewHolder DequeueViewHolder(ScrollerWidget self)
{
    return RemoveViewHolder(self, List_first(self -> scroller.holders));
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

/* Answers a GC to be used to draw things in the background color */
GC ScGCForBackground(ScrollerWidget self)
{
    return self -> scroller.backgroundGC;
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

/* Answers a Pixmap of the given width */
Pixmap ScCreatePixmap(ScrollerWidget self, unsigned int width, unsigned int height)
{
    Pixmap pixmap;
    pixmap = XCreatePixmap(XtDisplay(self), XtWindow(self), width, height, self -> core.depth);
    XFillRectangle(XtDisplay(self), pixmap, ScGCForBackground(self), 0, 0, width, height);
    return pixmap;
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
    self -> scroller.holders = List_alloc();
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
    EnqueueViewHolder(self, ViewHolder_alloc(NULL, self -> core.width));
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


/* Dequeue a ViewHolder if it is no longer visible */
static void OutWithTheOld(ScrollerWidget self)
{
    ViewHolder holder;

    while ((holder = List_first(self -> scroller.holders)) != NULL)
    {
	/* See if the first holder has scrolled off the left edge */
	if (self -> scroller.offset < holder -> width)
	{
	    return;
	}

	/* It has -- remove it from the queue */
	holder = DequeueViewHolder(self);
	    
	if (holder -> view)
	{
	    self -> scroller.realCount--;
	}

	self -> scroller.offset = 0;
	ViewHolder_free(holder);
    }
}

/* Enqueue a ViewHolder if there's space for one */
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
	    ViewHolder last = List_last(self -> scroller.holders);
	    ViewHolder holder;
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
	    holder = ViewHolder_alloc(NULL, width);
	    holder -> previous_width = lastWidth;
	    EnqueueViewHolder(self, holder);
	}
	/* otherwise add message */
	else
	{
	    EnqueueViewHolder(self, ViewHolder_alloc(next, MessageView_getWidth(next)));
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


/* ARGSUSED */
/* Repaints all of the ViewHolders' views */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    void *context[4];

#ifdef DEBUG
    /* Fill in any areas that aren't redrawn so we can spot redisplay bugs... */
    XFillRectangle(
	XtDisplay(widget), XtWindow(self), self -> scroller.gc,
	0, 0, self -> core.width, self -> core.height);
#endif /* 0 */
    context[0] = (void *)widget;
    context[1] = (void *)XtWindow(self);
    context[2] = (void *)(0 - self -> scroller.offset);
    context[3] = (void *)0;
    List_doWith(self -> scroller.holders, ViewHolder_redisplay, context);
}

/* FIX THIS: should actually do something? */
static void Destroy(Widget widget)
{
#ifdef DEBUG
    fprintf(stderr, "Destroy %p\n", widget);
#endif /* DEBUG */
}



/* Update the widget of the blank view */
static void ViewHolder_Resize(ViewHolder self, ScrollerWidget widget, long *x)
{
    long width;

    /* If x < 0 then we don't want to resize (but we do want to update x) */
    if (*x < 0)
    {
	*x += self -> width;
	return;
    }

    /* Skip any ViewHolders with actual text */
    if (self -> view != NULL)
    {
	return;
    }

    /* Ok, we've got the blank one.  Update its size (and make sure
       the visibleWidth is properly updated */
    widget -> scroller.visibleWidth -= self -> width;
    width = widget -> core.width - self -> previous_width;
    self -> width = (width < END_SPACING) ? END_SPACING : width;
    widget -> scroller.visibleWidth += self -> width;
}


/* Find the empty view and update its width */
static void Resize(Widget widget)
{
    ScrollerWidget self = (ScrollerWidget) widget;
    long x = 0 - self -> scroller.offset;

#if DEBUG
    fprintf(stderr, "Resize %p\n", widget);
#endif /* DEBUG */

    List_doWithWith(self -> scroller.holders, ViewHolder_Resize, self, &x);

    /* Make sure we have the right number of things visible */
    OutWithTheOld(self);
    InWithTheNew(self);
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


/* Locates the ViewHolder at the given offset (NULL if none) */
static ViewHolder ViewHolderAtOffset(ScrollerWidget self, int offset)
{
    long x = offset + self -> scroller.offset;

    return List_findFirstWith(self -> scroller.holders, (ListFindWithFunc)ViewHolder_locate, &x);
}

/* Answers the ViewHolder corresponding to the location of the last event */
static ViewHolder ViewHolderAtEvent(ScrollerWidget self, XEvent *event)
{
    if ((event -> type == KeyPress) || (event -> type == KeyRelease))
    {
	XKeyEvent *keyEvent = (XKeyEvent *) event;
	return ViewHolderAtOffset(self, keyEvent -> x);
    }

    if ((event -> type == ButtonPress) || (event -> type == ButtonRelease))
    {
	XButtonEvent *buttonEvent = (XButtonEvent *) event;
	return ViewHolderAtOffset(self, buttonEvent -> x);
    }

    if (event -> type == MotionNotify)
    {
	XMotionEvent *motionEvent = (XMotionEvent *) event;
	return ViewHolderAtOffset(self, motionEvent -> x);
    }

    return NULL;
}

/* Answers the message under the mouse in the given event */
static MessageView MessageAtEvent(ScrollerWidget self, XEvent *event)
{
    return ViewHolder_view(ViewHolderAtEvent(self, event));
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
    ViewHolder holder = ViewHolderAtEvent(self, event);
    MessageView view = ViewHolder_view(holder);

    if (view != NULL)
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


