/* $Id: Tickertape.c,v 1.11 1997/02/14 10:52:34 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include "TickertapeP.h"
#include "MessageView.h"

#define END_SPACING 30

/*
 * Resources
 */
#define offset(field) XtOffsetOf(TickertapeRec, field)
static XtResource resources[] =
{
    /* XtCallbackProc callback */
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
    }
};
#undef offset

/*
 * Action declarations
 */
static void Click();


/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    {"Click", Click}
};


/*
 * Default translation table
 */
static char defaultTranslations[] =
{
    "<Btn1Down>: Click()"
};



/*
 * Method declarations
 */
static void Initialize();
static void Rotate();
static void Redisplay();
static void Destroy();
static void Resize();
static Boolean SetValues();
static XtGeometryResult QueryGeometry();


/*
 * Class record initialization
 */
TickertapeClassRec tickertapeClassRec =
{
    /* core_class fields */
    {
	(WidgetClass) &simpleClassRec, /* superclass */
	"Tickertape", /* class_name */
	sizeof(TickertapeRec), /* widget_size */
	NULL, /* class_initialize */
	NULL, /* class_part_initialize */
	FALSE, /* class_inited */
	Initialize, /* Initialize */
	NULL, /* Initialize_hook */
	XtInheritRealize, /* realize */
	actions, /* actions */
	XtNumber(actions), /* num_actions */
	resources, /* resources */
	XtNumber(resources), /* num_resources */
	NULLQUARK, /* xrm_class */
	TRUE, /* compress_motion */
	TRUE, /* compress_exposure */
	TRUE, /* compress_enterleave */
	FALSE, /* visible_interest */
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

    /* FIX THIS: should be capable of being a Motif widget as well */
    /* Simple class fields initialization */
    {
	XtInheritChangeSensitive /* change_sensitive */
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
    Drawable drawable = (Drawable)context[0];
    int x = (int)context[1];
    int y = (int)context[2];

    if (self -> view)
    {
	MessageView_redisplay(self -> view, drawable, x, y);
    }

    context[1] = (void *)(x + self -> width);
}




/* Answers a GC with the right background color and font */
static void CreateGC(TickertapeWidget self)
{
    XGCValues values;

    values.font = self -> tickertape.font -> fid;
    values.background = self -> core.background_pixel;
    self -> tickertape.gc = XCreateGC(
	XtDisplay(self), RootWindowOfScreen(XtScreen(self)),
	GCFont | GCBackground, &values);
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
	fprintf(stderr, "restarting\n");
	fflush(stderr);
	self -> tickertape.isStopped = FALSE;
	SetClock(self);
    }
}

/* Stops the clock */
static void StopClock(TickertapeWidget self)
{
    fprintf(stderr, "stalling\n");
    fflush(stderr);
    self -> tickertape.isStopped = TRUE;
}

/* Sets the timer if the clock isn't stopped */
static void SetClock(TickertapeWidget self)
{
    if (! self -> tickertape.isStopped)
    {
	TtStartTimer(self, 41, Tick, (XtPointer) self);
    }
}


/* One interval has passed */
static void Tick(XtPointer widget, XtIntervalId *interval)
{
    TickertapeWidget self = (TickertapeWidget) widget;

    Rotate(self);
    Redisplay(self, NULL, 0);
    SetClock(self);
}





/* Adds a MessageView to the receiver */
static void AddMessageView(TickertapeWidget self, MessageView view)
{
    List_addLast(self -> tickertape.messages, view);
    MessageView_allocReference(view);
    printf("Added message view 0x%p\n", view);

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
    XGCValues values;

    values.foreground = self -> core.background_pixel;
    XChangeGC(XtDisplay(self), self -> tickertape.gc, GCForeground, &values);
    return self -> tickertape.gc;
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
Pixmap TtCreatePixmap(TickertapeWidget self, unsigned int width)
{
    Pixmap pixmap;
    pixmap = XCreatePixmap(
	XtDisplay(self), RootWindowOfScreen(XtScreen(self)),
	width, self -> core.height,
	DefaultDepthOfScreen(XtScreen(self)));
    XFillRectangle(XtDisplay(self), pixmap, TtGCForBackground(self), 0, 0, width, self -> core.height);
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
    Display *display = XtDisplay(self);
    Colormap colormap = XDefaultColormapOfScreen(XtScreen(self));
    XColor colors[5];

    self -> tickertape.isStopped = TRUE;
    self -> tickertape.messages = List_alloc();
    self -> tickertape.holders = List_alloc();
    self -> tickertape.offset = 0;
    self -> tickertape.visibleWidth = 0;
    self -> tickertape.nextVisible = 0;
    self -> tickertape.realCount = 0;

    CreateGC(self);
    self -> core.height = self -> tickertape.font -> ascent + self -> tickertape.font -> descent;
    self -> core.width = 400; /* FIX THIS: should be what? */

    /* Initialize colors */
    colors[0].pixel = self -> core.background_pixel;
    colors[1].pixel = self -> tickertape.groupPixel;
    colors[2].pixel = self -> tickertape.userPixel;
    colors[3].pixel = self -> tickertape.stringPixel;
    colors[4].pixel = self -> tickertape.separatorPixel;
    XQueryColors(display, colormap, colors, 5);

    self -> tickertape.groupPixels = CreateFadedColors(
	display, colormap, &colors[1], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.userPixels = CreateFadedColors(
	display, colormap, &colors[2], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.stringPixels = CreateFadedColors(
	display, colormap, &colors[3], &colors[0], self -> tickertape.fadeLevels);
    self -> tickertape.separatorPixels = CreateFadedColors(
	display, colormap, &colors[4], &colors[0], self -> tickertape.fadeLevels);

    EnqueueViewHolder(self, ViewHolder_alloc(NULL, self -> core.width));

    {
	Message message = Message_alloc("tickertape", "internal", "nevermind", 30);
	MessageView view = MessageView_alloc(self, message);

	AddMessageView(self, view);
    }

    StartClock(self);
}


/* Dequeue a ViewHolder if it is no longer visible */
static void OutWithTheOld(TickertapeWidget self)
{
    ViewHolder holder = List_first(self -> tickertape.holders);

    if (holder -> width <= self -> tickertape.offset)
    {
	ViewHolder holder = DequeueViewHolder(self);
	
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
	    unsigned long width;

	    if (self -> core.width < last -> width + END_SPACING)
	    {
		width = END_SPACING;
	    }
	    else
	    {
		width = self -> core.width - last -> width;
	    }
	    self -> tickertape.nextVisible = 0;
	    EnqueueViewHolder(self, ViewHolder_alloc(NULL, width));
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
    self -> tickertape.offset++;

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
    void *context[3];

    context[0] = (void *)XtWindow(self);
    context[1] = (void *)(0 - self -> tickertape.offset);
    context[2] = (void *)0;
    List_doWith(self -> tickertape.holders, ViewHolder_redisplay, context);
}

/* FIX THIS: should actually do something? */
static void Destroy(Widget widget)
{
    fprintf(stderr, "Destroy 0x%p\n", widget);
}

/* Nothing to do */
static void Resize(Widget widget)
{
    fprintf(stderr, "Resize 0x%p\n", widget);
}

/* What should this do? */
static Boolean SetValues(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args)
{
    return False;
}

/* What should this do? */
static XtGeometryResult QueryGeometry(
    Widget widget,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred)
{
    return XtGeometryYes;
}


/*
 * Action definitions
 */

/* Called when the button is pressed */
void Click(Widget widget, XEvent event)
{
    TickertapeWidget self = (TickertapeWidget) widget;
    XtCallCallbackList((Widget)self, self -> tickertape.callbacks, (XtPointer)NULL);
}

/*
 *Public methods
 */

/* Adds a Message to the receiver */
void TtAddMessage(TickertapeWidget self, Message message)
{
    AddMessageView(self, MessageView_alloc(self, message));
}


