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
static const char cvsid[] = "$Id: History.c,v 1.5 2001/07/06 04:55:19 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include <Xm/XmAll.h>
#include <Xm/PrimitiveP.h>
#include "message.h"
#include "message_view.h"
#include "History.h"
#include "HistoryP.h"


#if 1
#define dprintf(x) printf x
#else
#define dprintf(x) ;
#endif

#if !defined(MIN)
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#if !defined(MAX)
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

/*
 *
 * Resources
 *
 */

#define offset(field) XtOffsetOf(HistoryRec, field)
static XtResource resources[] =
{
    /* XtCallbackProc callback */
    {
	XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(history.callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XtCallbackList attachment_callbacks */
    {
	XtNattachmentCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
	offset(history.attachment_callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XFontStruct *font */
    {
	XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
	offset(history.font), XtRString, XtDefaultFont
    },

    /* Pixel group_pixel */
    {
	XtNgroupPixel, XtCGroupPixel, XtRPixel, sizeof(Pixel),
	offset(history.group_pixel), XtRString, "Blue"
    },

    /* Pixel user_pixel */
    {
	XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
	offset(history.user_pixel), XtRString, "Green"
    },

    /* Pixel string_pixel */
    {
	XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
	offset(history.string_pixel), XtRString, "Red"
    },

    /* Pixel separator_pixel */
    {
	XtNseparatorPixel, XtCSeparatorPixel, XtRPixel, sizeof(Pixel),
	offset(history.separator_pixel), XtRString, XtDefaultForeground
    }
};
#undef offset




/*
 *
 * Method declarations
 *
 */

static void init(Widget request, Widget widget, ArgList args, Cardinal *num_args);
static void realize(
    Widget widget,
    XtValueMask *value_mask,
    XSetWindowAttributes *attributes);
static void redisplay(Widget self, XEvent *event, Region region);
static void gexpose(Widget widget, XtPointer rock, XEvent *event, Boolean *ignored);
static void destroy(Widget self);
static void resize(Widget self);
static Boolean set_values(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args);
static XtGeometryResult query_geometry(
    Widget self,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred);


/*
 *
 * Class record initializations
 *
 */

HistoryClassRec historyClassRec =
{
    /* core_class fields */
    {
	(WidgetClass)&xmPrimitiveClassRec, /* superclass */
	"History", /* class_name */
	sizeof(HistoryRec), /* widget_size */
	NULL, /* class_initialize */
	NULL, /* class_part_initialize */
	False, /* class_inited */
	init, /* initialize */
	NULL, /* initialize_hook */
	realize, /* realize */
	NULL, /* actions */
	0, /* num_actions */
	resources, /* resources */
	XtNumber(resources), /* num_resources */
	NULLQUARK, /* xrm_class */
	True, /* compress_motion */
	True, /* compress_exposure */
	True, /* compress_enterleave */
	False, /* visible_interest */
	destroy, /* destroy */
	resize, /* resize */
	redisplay, /* expose */
	set_values, /* set_values */
	NULL, /* set_values_hook */
	XtInheritSetValuesAlmost, /* set_values_almost */
	NULL, /* get_values_hook */
	NULL, /* accept_focus */
	XtVersion, /* version */
	NULL, /* callback_private */
	NULL, /* tm_table */
	query_geometry, /* query_geometry */
	XtInheritDisplayAccelerator, /* display_accelerator */
	NULL /* extension */
    },

    /* Primitive class fields initialization */
    {
	NULL, /* border_highlight */
	NULL, /* border_unhighlight */
	NULL, /* translations */
	NULL, /* arm_and_activate_proc */
	NULL, /* synthetic resources */
	0, /* num syn res */
	NULL, /* extension */
    },

    /* History class fields initialization */
    {
	0 /* foo */
    }
};

WidgetClass historyWidgetClass = (WidgetClass) &historyClassRec;


/*
 *
 * Private static variables
 *
 */

/*
 *
 * Private Methods
 *
 */

/* Sets the origin of the visible portion of the widget */
static void set_origin(HistoryWidget self, Position x, Position y)
{
    Display *display = XtDisplay((Widget)self);
    GC gc = self -> history.gc;
    XGCValues values;

    /* Remove our clip mask */
    values.clip_mask = None;
    XChangeGC(display, gc, GCClipMask, &values);

    /* Copy to the new location */
    XCopyArea(
	display, XtWindow((Widget)self), XtWindow((Widget)self), gc,
	x - self -> history.x, y - self -> history.y,
	self -> core.width, self -> core.height,
	0, 0);

    self -> history.x = x;
    self -> history.y = y;
}


/*
 *
 * Method definitions
 *
 */

/* Set the widget's display to be threaded or not */
void HistorySetThreaded(Widget widget, Boolean is_threaded)
{
    dprintf(("HistorySetThreaded(): not yet implemented\n"));
}

/* Returns whether or not the history is threaded */
Boolean HistoryIsThreaded(Widget widget)
{
    dprintf(("HistoryIsThreaded(): not yet implemented\n"));
    return False;
}

/* Set the widget's display to show timestamps or not */
void HistorySetShowTimestamps(Widget widget, Boolean show_timestamps)
{
    dprintf(("HistorySetShowTimestamps(): not yet implemented\n"));
}

/* Returns whether or not the timestamps are visible */
Boolean HistoryIsShowingTimestamps(Widget widget)
{
    dprintf(("HistoryIsShowingTimestamps(): not yet implemented\n"));
    return False;
}


/* Adds a new message to the History */
void HistoryAddMessage(Widget widget, message_t message)
{
    dprintf(("HistoryAddMessage(): not yet implemented\n"));
}

/* Kills the thread of the given message */
void HistoryKillThread(Widget widget, message_t message)
{
    dprintf(("HistoryKillThread(): not yet implemented\n"));
}

/* Returns whether or not a given message is in a killed thread */
Boolean HistoryIsMessageKilled(Widget widget, message_t message)
{
    dprintf(("HistoryIsMessageKilled(): not yet implemented\n"));
    return False;
}

/* Selects a message in the history */
void HistorySelect(Widget widget, message_t message)
{
    dprintf(("HistorySelect(): not yet implemented\n"));
}

/* Returns the selected message or NULL if none is selected */
message_t HistoryGetSelection(Widget widget)
{
    dprintf(("HistoryGetSelection(): not yet implemented\n"));
    return NULL;
}



/* Callback for the vertical scrollbar */
static void vert_dec_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_dec_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_drag_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, self -> history.x, cbs -> value);
}

/* Callback for the vertical scrollbar */
static void vert_inc_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_inc_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_page_dec_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_page_dec_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_page_inc_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_page_inc_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_to_top_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_to_top_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_to_bottom_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_to_bottom_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_val_changed_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_val_changed_cb(): not yet implemented\n"));
}



/* Callback for the horizontal scrollbar */
static void horiz_dec_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_dec_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_drag_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, cbs -> value, self -> history.y);
}

/* Callback for the horizontal scrollbar */
static void horiz_inc_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_inc_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_page_dec_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_page_dec_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_page_inc_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_page_inc_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_to_top_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_to_top_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_to_bottom_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_to_bottom_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_val_changed_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_val_changed_cb(): not yet implemented\n"));
}


/* Initializes the History-related stuff */
/* ARGSUSED */
static void init(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    HistoryWidget self = (HistoryWidget)widget;
    Widget scrollbar;

    /* Set the initial width/height if none are supplied */
    self -> core.width = 400;
    self -> core.height = 80;

    /* Create the vertical scrollbar */
    scrollbar = XtVaCreateManagedWidget(
	"VertScrollBar", xmScrollBarWidgetClass,
	XtParent(self),
	XmNorientation, XmVERTICAL,
	NULL);
    XtAddCallback(scrollbar, XmNdecrementCallback, vert_dec_cb, self);
    XtAddCallback(scrollbar, XmNdragCallback, vert_drag_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, vert_inc_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback, vert_page_dec_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback, vert_page_inc_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, vert_to_top_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, vert_to_bottom_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback, vert_val_changed_cb, self);
    self -> history.vscrollbar = scrollbar;

    /* Create the horizontal scrollbar */
    scrollbar = XtVaCreateManagedWidget(
	"HorizScrollBar", xmScrollBarWidgetClass,
	XtParent(self),
	XmNorientation, XmHORIZONTAL,
	NULL);
    XtAddCallback(scrollbar, XmNdecrementCallback, horiz_dec_cb, self);
    XtAddCallback(scrollbar, XmNdragCallback, horiz_drag_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, horiz_inc_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback, horiz_page_dec_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback, horiz_page_inc_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, horiz_to_top_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, horiz_to_bottom_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback, horiz_val_changed_cb, self);
    self -> history.hscrollbar = scrollbar;

    printf("History: init()\n");
}

/* Realize the widget by creating a window in which to display it */
static void realize(
    Widget widget,
    XtValueMask *value_mask,
    XSetWindowAttributes *attributes)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(self);
    XGCValues values;
    message_t message;
    struct string_sizes sizes;

    printf("History: realize() w=%d, h=%d\n", self -> core.width, self -> core.height);

    /* Create our window */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask, attributes);

    /* Create a GC for our own nefarious purposes */
    values.background = self -> core.background_pixel;
    values.font = self -> history.font -> fid;
    self -> history.gc = XCreateGC(display, XtWindow(widget), GCBackground | GCFont, &values);

    /* Register to receive GraphicsExpose events */
    XtAddEventHandler(widget, 0, True, gexpose, NULL);


    /* Create a message */
    message = message_alloc(
	NULL,
	"ÆgroupÆ", "ÆuserÆ", "ÆstringÆ", 10,
	"x-elvin/url", "http://www.woolfit.com/",
	strlen("http://www.woolfit.com/"), "tag",
	"id", "reply_id");

    /* Create a mesage view */
    self -> history.mv = message_view_alloc(message, self -> history.font);
    message_view_get_sizes(self -> history.mv, &sizes);

    /* Record our sizes */
    self -> history.width = sizes.width + 2 * 10;
    self -> history.height = sizes.ascent + sizes.descent + 2 * 10;
    printf("height=%u\n", self -> history.height);

    XtVaSetValues(
	self -> history.hscrollbar,
	XmNminimum, 0,
	XmNmaximum, sizes.width + 20,
	NULL);

    /* Update the vertical scrollbar */
    XtVaSetValues(
	self -> history.vscrollbar,
	XmNminimum, 0,
	XmNmaximum, sizes.ascent + sizes.descent,
	NULL);
}

/* Repaint the widget */
static void paint(HistoryWidget self, XRectangle *bbox)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);

    /* Set that as our bounding box */
    XSetClipRectangles(display, self -> history.gc, 0, 0, bbox, 1, YXSorted);

#if 1
    /* Fill the background */
    {
	XGCValues values;

	values.foreground = 0xc0c0c0;
	XChangeGC(display, self -> history.gc, GCForeground, &values);
	XFillRectangle(
	    display, window, self -> history.gc,
	    10 - self -> history.x, 10 - self -> history.y,
	    self -> history.width - 20, self -> history.height - 20);

	values.foreground = 0x000000;
	XChangeGC(display, self -> history.gc, GCForeground, &values);
	XDrawRectangle(
	    display, window, self -> history.gc,
	    10 - self -> history.x, 10 - self -> history.y,
	    self -> history.width - 20, self -> history.height - 20);
    }
#endif

    /* Draw something.  I don't know */
    message_view_paint(
	self -> history.mv, display, window, self -> history.gc,
	self -> history.group_pixel, self -> history.user_pixel,
	self -> history.string_pixel, self -> history.separator_pixel,
	10 - self -> history.x, 10 + self -> history.font -> ascent - self -> history.y, bbox);
}

/* Redisplay the given region */
static void redisplay(Widget widget, XEvent *event, Region region)
{
    HistoryWidget self = (HistoryWidget)widget;
    XRectangle bbox;

    /* Find the smallest rectangle which contains the region */
    XClipBox(region, &bbox);

    /* Repaint the region */
    paint(self, &bbox);
}

/* Repaint the bits of the widget that didn't get copied */
static void gexpose(Widget widget, XtPointer rock, XEvent *event, Boolean *ignored)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(widget);
    XEvent event_buffer;
    XRectangle bbox;

    /* Process all of the GraphicsExpose events in one hit */
    while (1)
    {
	XGraphicsExposeEvent *g_event;

	/* See if the server has synced with our local state */
	/* FIX THIS: this is still necessary */

	/* Stop drawing stuff if the widget is obscured */
	if (event -> type == NoExpose)
	{
	    /* FIX THIS: set a flag to stop doing stuff? */
	    return;
	}

	/* Sanity check */
	assert(event -> type == GraphicsExpose || event -> type == NoExpose);

	/* Coerce the event */
	g_event = (XGraphicsExposeEvent *)event;

	/* Get the bounding box of the event */
	bbox.x = g_event -> x;
	bbox.y = g_event -> y;
	bbox.width = g_event -> width;
	bbox.height = g_event -> height;

	/* Update this portion of the history */
	paint(self, &bbox);

	/* Bail out if this is the last GraphicsExpose event */
	if (g_event -> count < 1)
	{
	    return;
	}

	/* Otherwise grab the next one */
	XNextEvent(display, &event_buffer);
	event = &event_buffer;
    }
}


/* Destroy the widget */
static void destroy(Widget self)
{
    dprintf(("History: destroy()\n"));
}


/* Resize the widget */
static void resize(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;
    Dimension width, height;

    /* Figure out how big the widget is now */
    XtVaGetValues(widget, XmNwidth, &width, XmNheight, &height, NULL);

    /* Update the horizontal scrollbar */
    XtVaSetValues(
	self -> history.hscrollbar,
	XmNsliderSize, MIN(width, self -> history.width) + 1,
	XmNmaximum, self -> history.width + 1,
	NULL);

    /* Update the vertical scrollbar */
    XtVaSetValues(
	self -> history.vscrollbar,
	XmNsliderSize, MIN(height, self -> history.height) + 1,
	XmNmaximum, self -> history.height + 1,
	NULL);
}


/* What should this do? */
static Boolean set_values(
    Widget current,
    Widget request,
    Widget new,
    ArgList args,
    Cardinal *num_args)
{
    dprintf(("History: set_values()\n"));
    return False;
}


/* We're always happy */
static XtGeometryResult query_geometry(
    Widget widget,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred)
{
    dprintf(("History: query_geometry()\n"));
    return XtGeometryYes;
}
