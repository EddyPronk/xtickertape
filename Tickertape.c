/* $Id: Tickertape.c,v 1.29 1998/11/05 01:54:54 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include "TickertapeP.h"
#include "MessageView.h"


#define END_SPACING 30

/*
 * Resources
 */
#define offset(field) XtOffsetOf(TickertapeRec, field)

static XtResource resources[] =
{
    /* XtCallbackList callbacks */
    {
	XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(tickertape.callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XFontStruct *font */
    {
	XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	offset(tickertape.font), XtRString, XtDefaultFont
    },

    /* Pixel groupPixel */
    {
	XtNgroupPixel, XtCGroupPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.groupPixel), XtRString, "Blue"
    },

    /* Pixel userPixel */
    {
	XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.userPixel), XtRString, "Green"
    },

    /* Pixel stringPixel */
    {
	XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.stringPixel), XtRString, "Red"
    },

    /* Pixel separatorPixel */
    {
	XtNseparatorPixel, XtCSeparatorPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.separatorPixel), XtRString, XtDefaultForeground
    },

    /* Dimension fadeLevels */
    {
	XtNfadeLevels, XtCFadeLevels, XtRDimension, sizeof(Dimension),
	offset(tickertape.fadeLevels), XtRImmediate, (XtPointer)5
    },

    /* Dimension frequency (in Hz) */
    {
	XtNfrequency, XtCFrequency, XtRDimension, sizeof(Dimension),
	offset(tickertape.frequency), XtRImmediate, (XtPointer)24
    },

    /* Dimension step (in pixels) */
    {
	XtNstepSize, XtCStepSize, XtRDimension, sizeof(Dimension),
	offset(tickertape.step), XtRImmediate, (XtPointer)1
    }
};
#undef offset


/*
 * Action declarations
 */
static void Menu();
static void DecodeMime();
static void Delete();

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "menu", Menu },
    { "decodeMime", DecodeMime },
    { "delete", Delete }
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: menu()\n<Btn2Down>: decodeMime()\n<Btn3Down>: delete()\n<Key>d: delete()\n<Key>q: quit()"
};



/*
 * Method declarations
 */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args);
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes);
static void Rotate(TickertapeWidget self);
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
TickertapeClassRec tickertapeClassRec =
{
    /* core_class fields */
    {
	(WidgetClass) &widgetClassRec, /* superclass */
	"Tickertape", /* class_name */
	sizeof(TickertapeRec), /* widget_size */
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

    /* Tickertape class fields initialization */
    {
	0 /* ignored */
    }
};

WidgetClass tickertapeWidgetClass = (WidgetClass)&tickertapeClassRec;


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
static void ViewHolder_redisplay(ViewHolder self, void *context[]);

static void CreateGC(TickertapeWidget self);
static Pixel *CreateFadedColors(Display *display, Colormap colormap,
				XColor *first, XColor *last, unsigned int levels);
static void StartClock(TickertapeWidget self);
static void StopClock(TickertapeWidget self);
static void SetClock(TickertapeWidget self);
static void Tick(XtPointer widget, XtIntervalId *interval);

static void AddMessageView(TickertapeWidget self, MessageView view);
static MessageView RemoveMessageView(TickertapeWidget self, MessageView view);
static void EnqueueViewHolder(TickertapeWidget self, ViewHolder holder);
static ViewHolder DequeueViewHolder(TickertapeWidget self);


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

/* Displays a ViewHolder */
static void ViewHolder_redisplay(ViewHolder self, void *context[])
{
    TickertapeWidget widget = (TickertapeWidget) context[0];
    Drawable drawable = (Drawable) context[1];
    int x = (int) context[2];
    int y = (int) context[3];

    if (self -> view)
    {
	MessageView_redisplay(self -> view, drawable, x, y);
    }
    else /* Fill the background color */
    {
	XFillRectangle(
	    XtDisplay(widget), drawable, widget -> tickertape.backgroundGC,
	    x, y, self -> width, widget -> core.height);
    }

    context[2] = (void *)(x + self -> width);
    XFlush(XtDisplay(widget));
}

/* Locates the Message at the given offset */
static void ViewHolder_locate(ViewHolder self, long *x, MessageView *view_return)
{
    if (*x < 0)
    {
	return;
    }

    *x -= self -> width;

    if (*x < 0)
    {
	*view_return = self -> view;
    }
}




/* Answers a GC with the right background color and font */
static void CreateGC(TickertapeWidget self)
{
    XGCValues values;

    values.font = self -> tickertape.font -> fid;
    values.background = self -> core.background_pixel;
    values.foreground = self -> core.background_pixel;
    self -> tickertape.backgroundGC = XCreateGC(
	XtDisplay(self), XtWindow(self), GCFont | GCBackground | GCForeground, &values);
    self -> tickertape.gc = XCreateGC(
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
static void StartClock(TickertapeWidget self)
{
    if (self -> tickertape.isStopped)
    {
#ifdef DEBUG
	fprintf(stderr, "restarting\n");
#endif /* DEBUG */
	fflush(stderr);
	self -> tickertape.isStopped = False;
	SetClock(self);
    }
}

/* Stops the clock */
static void StopClock(TickertapeWidget self)
{
#ifdef DEBUG
    fprintf(stderr, "stalling\n");
#endif /* DEBUG */
    fflush(stderr);
    self -> tickertape.isStopped = True;
}

/* Sets the timer if the clock isn't stopped */
static void SetClock(TickertapeWidget self)
{
    if (! self -> tickertape.isStopped)
    {
	TtStartTimer(self, 1000 / self -> tickertape.frequency, Tick, (XtPointer) self);
    }
}


/* One interval has passed */
static void Tick(XtPointer widget, XtIntervalId *interval)
{
    TickertapeWidget self = (TickertapeWidget) widget;

    Rotate(self);
    Redisplay((Widget)self, NULL, 0);
    SetClock(self);
}





/* Adds a MessageView to the receiver */
static void AddMessageView(TickertapeWidget self, MessageView view)
{
    List_addLast(self -> tickertape.messages, view);
    MessageView_allocReference(view);
#ifdef DEBUG
    fprintf(stderr, "Added message view %p\n", view);
#endif /* DEBUG */

    /* Make sure the clock is running */
    StartClock(self);
}

/* Removes a MessageView from the receiver */
static MessageView RemoveMessageView(TickertapeWidget self, MessageView view)
{
    List_remove(self -> tickertape.messages, view);
    MessageView_freeReference(view);

    return view;
}


/* Adds a ViewHolder to the receiver */
static void EnqueueViewHolder(TickertapeWidget self, ViewHolder holder)
{
    List_addLast(self -> tickertape.holders, holder);
    self -> tickertape.visibleWidth += holder -> width;
}

/* Removes a ViewHolder from the receiver */
static ViewHolder DequeueViewHolder(TickertapeWidget self)
{
    ViewHolder holder = List_dequeue(self -> tickertape.holders);
    self -> tickertape.visibleWidth -= holder -> width;
    return holder;
}



/*
 * Semi-private methods
 */

/* Answers a GC for displaying the Group field of a message at the given fade level */
GC TtGCForGroup(TickertapeWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> tickertape.groupPixels[level];
    XChangeGC(XtDisplay(self), self -> tickertape.gc, GCForeground, &values);
    return self -> tickertape.gc;
}

/* Answers a GC for displaying the User field of a message at the given fade level */
GC TtGCForUser(TickertapeWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> tickertape.userPixels[level];
    XChangeGC(XtDisplay(self), self -> tickertape.gc, GCForeground, &values);
    return self -> tickertape.gc;
}

/* Answers a GC for displaying the String field of a message at the given fade level */
GC TtGCForString(TickertapeWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> tickertape.stringPixels[level];
    XChangeGC(XtDisplay(self), self -> tickertape.gc, GCForeground, &values);
    return self -> tickertape.gc;
}

/* Answers a GC for displaying the field separators at the given fade level */
GC TtGCForSeparator(TickertapeWidget self, int level)
{
    XGCValues values;

    values.foreground = self -> tickertape.separatorPixels[level];
    XChangeGC(XtDisplay(self), self -> tickertape.gc, GCForeground, &values);
    return self -> tickertape.gc;
}

/* Answers a GC to be used to draw things in the background color */
GC TtGCForBackground(TickertapeWidget self)
{
    return self -> tickertape.backgroundGC;
}

/* Answers the XFontStruct to be use for displaying the group */
XFontStruct *TtFontForGroup(TickertapeWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> tickertape.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *TtFontForUser(TickertapeWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> tickertape.font;
}


/* Answers the XFontStruct to be use for displaying the string */
XFontStruct *TtFontForString(TickertapeWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> tickertape.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *TtFontForSeparator(TickertapeWidget self)
{
    /* FIX THIS: should allow user to specify different fonts for each part*/
    return self -> tickertape.font;
}


/* Answers the number of fade levels */
Dimension TtGetFadeLevels(TickertapeWidget self)
{
    return self -> tickertape.fadeLevels;
}

/* Answers a Pixmap of the given width */
Pixmap TtCreatePixmap(TickertapeWidget self, unsigned int width, unsigned int height)
{
    Pixmap pixmap;
    pixmap = XCreatePixmap(XtDisplay(self), XtWindow(self), width, height, self -> core.depth);
    XFillRectangle(XtDisplay(self), pixmap, TtGCForBackground(self), 0, 0, width, height);
    return pixmap;
}

/* Sets a timer to go off in interval milliseconds */
XtIntervalId TtStartTimer(TickertapeWidget self, unsigned long interval,
			  XtTimerCallbackProc proc, XtPointer client_data)
{
    return XtAppAddTimeOut(
	XtWidgetToApplicationContext((Widget)self),
	interval, proc, client_data);
}

/* Stops a timer from going off */
void TtStopTimer(TickertapeWidget self, XtIntervalId timer)
{
     XtRemoveTimeOut(timer);
}


/*
 * Method Definitions
 */

/* ARGSUSED */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    TickertapeWidget self = (TickertapeWidget) widget;

    self -> tickertape.isStopped = True;
    self -> tickertape.messages = List_alloc();
    self -> tickertape.holders = List_alloc();
    self -> tickertape.offset = 0;
    self -> tickertape.visibleWidth = 0;
    self -> tickertape.nextVisible = 0;
    self -> tickertape.realCount = 0;

    /* Make sure we have a width */
    if (self -> core.width == 0)
    {
	self -> core.width = 400;
    }

    /* Make sure we have a height */
    if (self -> core.height == 0)
    {
	self -> core.height = self -> tickertape.font -> ascent + self -> tickertape.font -> descent;
    }

    /* Ensure that we always have at least a blank view holder around */
    EnqueueViewHolder(self, ViewHolder_alloc(NULL, self -> core.width));
}

/* Realize the widget by creating a window in which to display it */
static void Realize(Widget widget, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    Display *display = XtDisplay(self);
    Colormap colormap = XDefaultColormapOfScreen(XtScreen(self));
    XColor colors[5];

    /* Initialize colors */
    colors[0].pixel = self -> core.background_pixel;
    colors[1].pixel = self -> tickertape.groupPixel;
    colors[2].pixel = self -> tickertape.userPixel;
    colors[3].pixel = self -> tickertape.stringPixel;
    colors[4].pixel = self -> tickertape.separatorPixel;
    XQueryColors(display, colormap, colors, 5);

    /* Create a window and couple graphics contexts */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask, attributes);
    CreateGC(self);

    /* Allocate colors */
    self -> tickertape.groupPixels = CreateFadedColors(
	display, colormap, &colors[1], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.userPixels = CreateFadedColors(
	display, colormap, &colors[2], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.stringPixels = CreateFadedColors(
	display, colormap, &colors[3], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.separatorPixels = CreateFadedColors(
	display, colormap, &colors[4], &colors[0], self -> tickertape.fadeLevels);
}


/* Dequeue a ViewHolder if it is no longer visible */
static void OutWithTheOld(TickertapeWidget self)
{
    ViewHolder holder;

    while ((holder = List_first(self -> tickertape.holders)) != NULL)
    {
	/* See if the first holder has scrolled off the left edge */
	if (self -> tickertape.offset < holder -> width)
	{
	    return;
	}

	/* It has -- remove it from the queue */
	holder = DequeueViewHolder(self);
	    
	if (holder -> view)
	{
	    self -> tickertape.realCount--;
	}

	self -> tickertape.offset = 0;
	ViewHolder_free(holder);
    }
}

/* Enqueue a ViewHolder if there's space for one */
static void InWithTheNew(TickertapeWidget self)
{
    while (self -> tickertape.visibleWidth - self -> tickertape.offset < self -> core.width)
    {
	MessageView next;

	/* Find a message that hasn't expired */
	next = List_get(self -> tickertape.messages, self -> tickertape.nextVisible);
	while ((next != NULL) && (MessageView_isTimedOut(next)))
	{
	    RemoveMessageView(self, next);
	    next = List_get(self -> tickertape.messages, self -> tickertape.nextVisible);
	}
	self -> tickertape.nextVisible++;

	/* If at end of list, add a spacer */
	if (next == NULL)
	{
	    ViewHolder last = List_last(self -> tickertape.holders);
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

	    self -> tickertape.nextVisible = 0;
	    holder = ViewHolder_alloc(NULL, width);
	    holder -> previous_width = lastWidth;
	    EnqueueViewHolder(self, holder);
	}
	/* otherwise add message */
	else
	{
	    EnqueueViewHolder(self, ViewHolder_alloc(next, MessageView_getWidth(next)));
	    self -> tickertape.realCount++;
	}
    }
}

/* Move the messages along */
static void Rotate(TickertapeWidget self)
{
    self -> tickertape.offset += self -> tickertape.step;

    OutWithTheOld(self);
    InWithTheNew(self);

    /* If no messages around then stop spinning */
    if ((self -> tickertape.realCount == 0) && List_isEmpty(self -> tickertape.messages))
    {
	StopClock(self);
    }
}


/* ARGSUSED */
/* Repaints all of the ViewHolders' views */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    TickertapeWidget self = (TickertapeWidget)widget;
    void *context[4];

#ifdef DEBUG
    /* Fill in any areas that aren't redrawn so we can spot redisplay bugs... */
    XFillRectangle(
	XtDisplay(widget), XtWindow(self), self -> tickertape.gc,
	0, 0, self -> core.width, self -> core.height);
#endif /* 0 */
    context[0] = (void *)widget;
    context[1] = (void *)XtWindow(self);
    context[2] = (void *)(0 - self -> tickertape.offset);
    context[3] = (void *)0;
    List_doWith(self -> tickertape.holders, ViewHolder_redisplay, context);
}

/* FIX THIS: should actually do something? */
static void Destroy(Widget widget)
{
#ifdef DEBUG
    fprintf(stderr, "Destroy %p\n", widget);
#endif /* DEBUG */
}



/* Update the widget of the blank view */
static void ViewHolder_Resize(ViewHolder self, TickertapeWidget widget, long *x)
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
    widget -> tickertape.visibleWidth -= self -> width;
    width = widget -> core.width - self -> previous_width;
    self -> width = (width < END_SPACING) ? END_SPACING : width;
    widget -> tickertape.visibleWidth += self -> width;
}


/* Find the empty view and update its width */
static void Resize(Widget widget)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    long x = 0 - self -> tickertape.offset;

#if DEBUG
    fprintf(stderr, "Resize %p\n", widget);
#endif /* DEBUG */

    List_doWithWith(self -> tickertape.holders, ViewHolder_Resize, self, &x);

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


/* Locates the message at the given offset (NULL if none) */
static MessageView messageAtOffset(TickertapeWidget self, int offset)
{
    long x = offset + self -> tickertape.offset;
    MessageView view = NULL;

    List_doWithWith(self -> tickertape.holders, ViewHolder_locate, &x, &view);
    return view;
}

/* Answers the message under the mouse in the given event */
static MessageView messageAtEvent(TickertapeWidget self, XEvent *event)
{
    if ((event -> type == KeyPress) || (event -> type == KeyRelease))
    {
	XKeyEvent *keyEvent = (XKeyEvent *) event;
	return messageAtOffset(self, keyEvent -> x);
    }

    if ((event -> type == ButtonPress) || (event -> type == ButtonRelease))
    {
	XButtonEvent *buttonEvent = (XButtonEvent *) event;
	return messageAtOffset(self, buttonEvent -> x);
    }

    if (event -> type == MotionNotify)
    {
	XMotionEvent *motionEvent = (XMotionEvent *) event;
	return messageAtOffset(self, motionEvent -> x);
    }

    return NULL;
}


/*
 * Action definitions
 */

/* Called when the button is pressed */
void Menu(Widget widget, XEvent *event)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    MessageView view = messageAtEvent(self, event);
    Message message = view ? MessageView_getMessage(view) : NULL;
    XtCallCallbackList((Widget)self, self -> tickertape.callbacks, (XtPointer) message);
}

static void DecodeMime(Widget widget, XEvent *event)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    MessageView view = messageAtEvent(self, event);
    
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

static void Delete(Widget widget, XEvent *event)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    MessageView view = messageAtEvent(self, event);

    if (view)
    {
	MessageView_expire(view);
    }
    else
    {
#ifdef DEBUG       
	fprintf(stderr, "missed\n");
#endif /* DEBUG */
    }
}

/*
 *Public methods
 */

/* Adds a Message to the receiver */
void TtAddMessage(TickertapeWidget self, Message message)
{
    AddMessageView(self, MessageView_alloc(self, message));
}


