/* $Id: Tickertape.c,v 1.4 1997/02/10 08:07:35 phelps Exp $ */

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

/*
 * Resources
 */
#define offset(field) XtOffsetOf(TickertapeRec, field)
static XtResource resources[] =
{
    /* XFontStruct *font */
    {
	XtNfont, /* String resource_name */
	XtCFont, /* String resource_class */
	XtRFontStruct, /*String resource_type */
	sizeof(XFontStruct *), /* Cardinal resource_size */
	offset(tickertape.font), /* Cardinal resource_offset */
	XtRString, /* String default_type */
	XtDefaultFont /* XtPointer default_address */
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
	offset(tickertape.fadeLevels), XtRImmediate, (XtPointer)8
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
static void CreateGC(TickertapeWidget self)
{
    XGCValues values;

    values.font = self -> tickertape.font -> fid;
    values.background = self -> core.background_pixel;
    self -> tickertape.gc = XCreateGC(
	XtDisplay(self), RootWindowOfScreen(XtScreen(self)),
	GCFont | GCBackground, &values);
}

static Pixel *CreateFadedColors(
    Display *display, Colormap colormap,
    XColor *first, XColor *last, unsigned int levels)
{
    Pixel *result = calloc(levels, sizeof(Pixel));
    long redNumerator = (long)last -> red - first -> red;
    long greenNumerator = (long)last -> green - first -> green;
    long blueNumerator = (long)last -> blue - first -> blue;
    long denominator = levels + 1;
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

/* Answers the XFontStruct to be use for displaying the group */
XFontStruct *TtFontForGroup(TickertapeWidget self)
{
    /* Fix this: should allow user to specify various fonts */
    return self -> tickertape.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *TtFontForUser(TickertapeWidget self)
{
    /* Fix this: should allow user to specify various fonts */
    return self -> tickertape.font;
}


/* Answers the XFontStruct to be use for displaying the string */
XFontStruct *TtFontForString(TickertapeWidget self)
{
    /* Fix this: should allow user to specify various fonts */
    return self -> tickertape.font;
}

/* Answers the XFontStruct to be use for displaying the user */
XFontStruct *TtFontForSeparator(TickertapeWidget self)
{
    /* Fix this: should allow separator to specify various fonts */
    return self -> tickertape.font;
}


/* Answers a Pixmap of the given width */
Pixmap TtCreatePixmap(TickertapeWidget self, unsigned int width)
{
    return XCreatePixmap(
	XtDisplay(self), RootWindowOfScreen(XtScreen(self)),
	width, self -> core.height,
	DefaultDepthOfScreen(XtScreen(self)));
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

    CreateGC(self);
    self -> core.height = self -> tickertape.font -> ascent + self -> tickertape.font -> descent;
    self -> core.width = 200; /* FIX THIS: should be what? */

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

    /* give us a message to play with */
    {
	Message message = Message_alloc("Tickertape", "internal", "startup", 60);
	self -> tickertape.view = MessageView_alloc(self, message);
	MessageView_debug(self -> tickertape.view);
    }
}

/* ARGSUSED */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    TickertapeWidget self = (TickertapeWidget)widget;
    Display *display = event -> xany.display;
    Window window = event -> xany.window;
    unsigned int index;
    unsigned width = self -> core.width / self -> tickertape.fadeLevels;

    fprintf(stderr, "Redisplay 0x%p\n", widget);
    for (index = 0; index < self -> tickertape.fadeLevels; index++)
    {
	GC gc = TtGCForSeparator(self, index);
	XFillRectangle(display, window, gc,
		       width * index, 0,
		       self -> core.width, self -> core.height);
    }

    /* Paint the message view */
    MessageView_redisplay(self -> tickertape.view, self, XtWindow(self),
			  0, self -> tickertape.font -> ascent);
}

static void Destroy(Widget widget)
{
    fprintf(stderr, "Destroy 0x%p\n", widget);
}

static void Resize(Widget widget)
{
    fprintf(stderr, "Resize 0x%p\n", widget);
}

static Boolean SetValues(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args)
{
    return False;
}

static XtGeometryResult QueryGeometry(
    Widget widget,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred)
{
    return XtGeometryYes;
}


/* Action definitions */
void Click(Widget widget, XEvent event)
{
    fprintf(stderr, "Click 0x%p\n", widget);
}
