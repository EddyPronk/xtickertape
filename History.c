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
static const char cvsid[] = "$Id: History.c,v 1.25 2001/07/21 01:12:35 phelps Exp $";
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
    },

    /* Dimension margin_width */
    {
	XtNmarginWidth, XtCMarginWidth, XtRDimension, sizeof(Dimension),
	offset(history.margin_width), XtRImmediate, (XtPointer)5
    },

    /* Dimension margin_height */
    {
	XtNmarginHeight, XtCMarginHeight, XtRDimension, sizeof(Dimension),
	offset(history.margin_height), XtRImmediate, (XtPointer)5
    },

    /* unsigned int message_capacity */
    {
	XtNmessageCapacity, XtCMessageCapacity, XtRDimension, sizeof(unsigned int),
	offset(history.message_capacity), XtRImmediate, (XtPointer)32
    },

    /* Pixel selection_pixel */
    {
	XtNselectionPixel, XtCSelectionPixel, XtRPixel, sizeof(Pixel),
	offset(history.selection_pixel), XtRString, XtDefaultForeground
    },

    /* int drag_delay */
    {
	XtNdragDelay, XtCDragDelay, XtRInt, sizeof(int),
	offset(history.drag_delay), XtRImmediate, (XtPointer)100
    }
};
#undef offset


/* Action declarations */
static void drag(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void drag_done(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void do_select(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void toggle_selection(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void show_attachment(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void select_previous(Widget widget, XEvent *event, String *params, Cardinal *nparams);
static void select_next(Widget widget, XEvent *event, String *params, Cardinal *nparams);

static XtActionsRec actions[] =
{
    { "drag", drag },
    { "drag-done", drag_done },
    { "select", do_select },
    { "toggle-selection", toggle_selection },
    { "show-attachment", show_attachment },
    { "select-previous", select_previous },
    { "select-next", select_next }
};


/* We move the History window around by calling XCopyArea to move the
 * visible portion of the screen and rely on GraphicsExpose events to
 * tell us what to repaint.  It occasionally happens that we want to
 * move the window again before the X server has processed our
 * XCopyArea request.  When this happens we need to tweak the
 * coordinates of the Expose events generated by the first XCopyArea
 * because the exposed areas will have moved by the time the packets
 * which redraw them are processed by the server.  Since there's no
 * upper limit on the number of XCopyAreas that haven't yet been
 * processed we end up using a linked list to represent all them */
struct delta_queue
{
    /* The next item in the queue */
    delta_queue_t next;

    /* The sequence number of the XCopyArea request */
    unsigned long request_id;

    /* The displacement in the X direction */
    long dx;

    /* The displacement in the Y direction */
    long dy;
};

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
	actions, /* actions */
	XtNumber(actions), /* num_actions */
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
static void set_origin(HistoryWidget self, long x, long y, int update_scrollbars)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self -> history.gc;
    XGCValues values;
    delta_queue_t item;

    /* Skip this part if we're not visible */
    if (gc != None)
    {
	/* Allocate a new queue item to represent this movement */
	item = malloc(sizeof(struct delta_queue));
	item -> next = self -> history.dqueue;
	item -> request_id = NextRequest(display);
	item -> dx = x - self -> history.x;
	item -> dy = y - self -> history.y;
	self -> history.dqueue = item;

	/* Remove our clip mask */
	values.clip_mask = None;
	XChangeGC(display, gc, GCClipMask, &values);

	/* Copy to the new location */
	XCopyArea(
	    display, window, window, gc,
	    item -> dx, item -> dy,
	    self -> core.width, self -> core.height,
	    0, 0);
    }

    /* Update the scrollbars */
    if (update_scrollbars)
    {
	/* Have we moved horizontally? */
	if (self -> history.x != x)
	{
	    XtVaSetValues(
		self -> history.hscrollbar,
		XmNvalue, x,
		NULL);
	}

	/* Have we moved vertically? */
	if (self -> history.y != y)
	{
	    XtVaSetValues(
		self -> history.vscrollbar,
		XmNvalue, y,
		NULL);
	}
    }

    /* Record our new location */
    self -> history.x = x;
    self -> history.y = y;

}

/* Callback for the vertical scrollbar */
static void vert_drag_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, self -> history.x, cbs -> value, 0);
}

/* Callback for the vertical scrollbar */
static void vert_dec_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Move up by one pixel for now */
    set_origin(self, self -> history.x, self -> history.y - 1, 0);
}

/* Callback for the vertical scrollbar */
static void vert_inc_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Move down by one pixel for now */
    set_origin(self, self -> history.x, self -> history.y + 1, 0);
}

/* Callback for the vertical scrollbar */
static void vert_page_dec_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("vert_page_dec_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_page_inc_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("vert_page_inc_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_to_top_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("vert_to_top_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_to_bottom_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("vert_to_bottom_cb(): not yet implemented\n"));
}

/* Callback for the vertical scrollbar */
static void vert_val_changed_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("vert_val_changed_cb(): not yet implemented\n"));
}



/* Callback for the horizontal scrollbar */
static void horiz_drag_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, cbs -> value, self -> history.y, 0);
}

/* Callback for the horizontal scrollbar */
static void horiz_dec_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Move left by one pixel for now */
    set_origin(self, self -> history.x - 1, self -> history.y, 0);
}

/* Callback for the horizontal scrollbar */
static void horiz_inc_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Move right by one pixel for now */
    set_origin(self, self -> history.x + 1, self -> history.y, 0);
}

/* Callback for the horizontal scrollbar */
static void horiz_page_dec_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("horiz_page_dec_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_page_inc_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("horiz_page_inc_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_to_top_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("horiz_to_top_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_to_bottom_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    dprintf(("horiz_to_bottom_cb(): not yet implemented\n"));
}

/* Callback for the horizontal scrollbar */
static void horiz_val_changed_cb(Widget widget, XtPointer client_data, XtPointer call_data)
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

    /* Set our initial dimensions to be the margin width */
    self -> history.width = (long)self -> history.margin_width * 2;
    self -> history.height = (long)self -> history.margin_height * 2;

    /* Compute the line height */
    self -> history.line_height =
	(long)self -> history.font -> ascent +
	(long)self -> history.font -> descent + 1;

    /* Start with an empty delta queue */
    self -> history.dqueue = NULL;

    /* Allocate enough room for all of our message views */
    self -> history.message_count = 0;
    self -> history.message_views =
	calloc(self -> history.message_capacity, sizeof(message_view_t));

    /* Nothing is selected yet */
    self -> history.selection = NULL;
    self -> history.selection_index = (unsigned int)-1;
    self -> history.drag_timeout = None;

    /* Create the horizontal scrollbar */
    scrollbar = XtVaCreateManagedWidget(
	"HorizScrollBar", xmScrollBarWidgetClass,
	XtParent(self),
	XmNorientation, XmHORIZONTAL,
	XmNminimum, 0,
	XmNmaximum, self -> history.width,
	XmNsliderSize, self -> history.width,
	NULL);

    /* Add a bunch of callbacks */
    XtAddCallback(scrollbar, XmNdecrementCallback, horiz_dec_cb, self);
    XtAddCallback(scrollbar, XmNdragCallback, horiz_drag_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, horiz_inc_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback, horiz_page_dec_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback, horiz_page_inc_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, horiz_to_top_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, horiz_to_bottom_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback, horiz_val_changed_cb, self);
    self -> history.hscrollbar = scrollbar;
    
    /* Create the vertical scrollbar */
    scrollbar = XtVaCreateManagedWidget(
	"VertScrollBar", xmScrollBarWidgetClass,
	XtParent(self),
	XmNorientation, XmVERTICAL,
	XmNminimum, 0,
	XmNmaximum, self -> history.height,
	XmNsliderSize, self -> history.height,
	NULL);

    /* Add a bunch of callbacks */
    XtAddCallback(scrollbar, XmNdecrementCallback, vert_dec_cb, self);
    XtAddCallback(scrollbar, XmNdragCallback, vert_drag_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, vert_inc_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback, vert_page_dec_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback, vert_page_inc_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, vert_to_top_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, vert_to_bottom_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback, vert_val_changed_cb, self);
    self -> history.vscrollbar = scrollbar;

    printf("History: init()\n");
}

/* Updates the scrollbars after either the widget or the data it is
 * displaying is resized */
static void update_scrollbars(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;
    Dimension width, height;
    Position x, y;

    /* Figure out how big the widget is now */
    XtVaGetValues(widget, XmNwidth, &width, XmNheight, &height, NULL);

    /* The slider can't be wider than the total */
    if (self -> history.width < width)
    {
	width = self -> history.width;
    }

    /* Keep the right edge sane */
    x = self -> history.x;
    if (self -> history.width - width < x)
    {
	x = self -> history.width - width;
    }


    /* The slider can't be taller than the total */
    if (self -> history.height < height)
    {
	height = self -> history.height;
    }

    /* Keep the bottom edge sane */
    y = self -> history.y;
    if (self -> history.height - height < y)
    {
	y = self -> history.height - height;
    }

    /* Move the origin */
    set_origin(self, x, y, 0);

    /* Update the horizontal scrollbar */
    XtVaSetValues(
	self -> history.hscrollbar,
	XmNvalue, x,
	XmNsliderSize, width,
	XmNmaximum, self -> history.width,
	NULL);

    /* Update the vertical scrollbar */
    XtVaSetValues(
	self -> history.vscrollbar,
	XmNvalue, y,
	XmNsliderSize, height,
	XmNmaximum, self -> history.height,
	NULL);
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

    printf("History: realize() w=%d, h=%d\n", self -> core.width, self -> core.height);

    /* Create our window */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask, attributes);

    /* Create a GC for our own nefarious purposes */
    values.background = self -> core.background_pixel;
    values.font = self -> history.font -> fid;
    self -> history.gc = XCreateGC(display, XtWindow(widget), GCBackground | GCFont, &values);

    /* Register to receive GraphicsExpose events */
    XtAddEventHandler(widget, 0, True, gexpose, NULL);
}

/* Translate the bounding box to compensate for unprocessed CopyArea
 * requests */
static void compensate_bbox(
    HistoryWidget self,
    unsigned long request_id,
    XRectangle *bbox)
{
    delta_queue_t previous = NULL;
    delta_queue_t item = self -> history.dqueue;

    /* Go through each outstanding delta item */
    while (item != NULL)
    {
	/* Has the request for this item been processed? */
	if (item -> request_id < request_id)
	{
	    /* Yes.  Throw it away. */
	    if (previous == NULL)
	    {
		self -> history.dqueue = item -> next;
		free(item);
		item = self -> history.dqueue;
	    }
	    else
	    {
		previous -> next = item -> next;
		free(item);
		item = previous -> next;
	    }
	}
	else
	{
	    /* No.  Translate the bounding box accordingly. */
	    bbox -> x -= item -> dx;
	    bbox -> y -= item -> dy;
	    previous = item;
	    item = item -> next;
	}
    }
}

/* Repaint the widget */
static void paint(HistoryWidget self, XRectangle *bbox)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self -> history.gc;
    long xmargin = (long)self -> history.margin_width;
    long ymargin = (long)self -> history.margin_height;
    message_view_t view;
    XGCValues values;
    unsigned int index;
    long x, y;

    /* Set that as our bounding box */
    XSetClipRectangles(display, gc, 0, 0, bbox, 1, YXSorted);

    /* Compute our X coordinate */
    x = xmargin - self -> history.x;

    /* Is the top margin visible? */
    if (self -> history.y < self -> history.margin_height)
    {
	/* Yes.  Start drawing at index 0 */
	index = 0;
	y = ymargin - self -> history.y;
    }
    else
    {
	/* No.  Compute the first visible index */
	index = (self -> history.y - self -> history.margin_height) / self -> history.line_height;
	y = (ymargin - self -> history.y) % self -> history.line_height;
    }

    /* Draw all visible message views */
    while (index < self -> history.message_count)
    {
	/* Skip NULL message views */
	if ((view = self -> history.message_views[index++]) == NULL)
	{
	    return;
	}

	/* Is this the selected message? */
	if (message_view_get_message(view) == self -> history.selection)
	{
	    /* Yes, draw a background for it */
	    values.foreground = self -> history.selection_pixel;
	    XChangeGC(display, gc, GCForeground, &values);

	    /* FIX THIS: should we respect the margin? */
	    XFillRectangle(
		display, window, gc,
		0, y, self -> core.width,
		self -> history.line_height);
	}

	/* Draw the view */
	message_view_paint(
	    view, display, window, gc,
	    self -> history.group_pixel, self -> history.user_pixel,
	    self -> history.string_pixel, self -> history.separator_pixel,
	    x, y + self -> history.font -> ascent, bbox);

	/* Get ready to draw the next one */
	y += self -> history.line_height;

	/* Bail out if the next line is past the end of the screen */
	if (! (y < self -> core.height)) {
	    return;
	}
    }
}

/* Redisplay the given region */
static void redisplay(Widget widget, XEvent *event, Region region)
{
    HistoryWidget self = (HistoryWidget)widget;
    XExposeEvent *x_event = (XExposeEvent *)event;
    XRectangle bbox;

    /* Find the smallest rectangle which contains the region */
    XClipBox(region, &bbox);

    /* Compensate for unprocessed CopyArea requests */
    compensate_bbox(self, x_event -> serial, &bbox);

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

	/* Stop drawing stuff if the widget is obscured */
	if (event -> type == NoExpose)
	{
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

	/* Compensate for unprocessed CopyArea requests */
	compensate_bbox(self, g_event -> serial, &bbox);

	/* Update this portion of the history */
	paint(self,  &bbox);

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

/* Insert a message before the given index */
static void insert_message(HistoryWidget self, unsigned int index, message_t message)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    message_view_t view;
    struct string_sizes sizes;
    long y;
    long width = 0;
    long height = 0;
    XRectangle bbox;
    unsigned int i;
    XGCValues values;
    GC gc = self -> history.gc;

    /* Sanity check */
    assert(index <= self -> history.message_count);

    /* Cancel our clip mask */
    if (gc != None)
    {
	values.clip_mask = None;
 	values.foreground = self -> core.background_pixel;
	XChangeGC(display, gc, GCClipMask | GCForeground, &values);
    }

    /* Are we out of room? */
    if (! (self -> history.message_count < self -> history.message_capacity))
    {
	/* Get rid of the first message and scroll the others upwards */
	view = self -> history.message_views[0];
	message_view_get_sizes(view, &sizes);
	message_view_free(view);

	/* Figure out where to start copying */
	y = self -> history.line_height;

	/* Move the other views towards the beginning of the array */
	for (i = 1; i < index; i++)
	{
	    view = self -> history.message_views[i];

	    /* Measure the view for the new width/height of the widget */
	    message_view_get_sizes(view, &sizes);
	    width = MAX(width, sizes.width);
	    height += self -> history.line_height;
	    self -> history.message_views[i - 1] = view;
	}

	/* Update the selection index if we've just moved it */
	if (self -> history.selection_index <= index)
	{
	    self -> history.selection_index--;
	}

	/* Move the messages upwards as a block */
	if (gc != None)
	{
	    XCopyArea(
		display, window, window, gc,
		0, self -> history.margin_height - self -> history.y + y,
		self -> core.width, height,
		0, self -> history.margin_height - self -> history.y);
	}

	/* The index gets decremented */
	index--;
    }
    else
    {
	/* Otherwise simply measure the messages */
	for (i = 0; i < index; i++)
	{
	    message_view_get_sizes(self -> history.message_views[i], &sizes);
	    width = MAX(width, sizes.width);
	    height += self -> history.line_height;
	}

	/* Update the counter */
	/* FIX THIS: perhaps do this at the end? */
	self -> history.message_count++;
    }

    /* Create a new message_view */
    view = message_view_alloc(message, self -> history.font);
    self -> history.message_views[index] = view;

    /* Measure it */
    message_view_get_sizes(view, &sizes);

    /* Draw it if we have a graphics context */
    if (gc != None)
    {
	/* Erase the space needed by the message */
	XFillRectangle(
	    display, window, gc,
	    0, self -> history.margin_height + height - self -> history.y,
	    self -> core.width, self -> history.line_height);

	/* Paint it */
	bbox.x = 0;
	bbox.y = self -> history.margin_height + height - self -> history.y;
	bbox.width = self -> core.width;
	bbox.height = self -> history.line_height;

	message_view_paint(
	    view, display, window, gc,
	    self -> history.group_pixel, self -> history.user_pixel,
	    self -> history.string_pixel, self -> history.separator_pixel,
	    self -> history.margin_width - self -> history.x,
	    self -> history.margin_height - self -> history.y + height +
	    self -> history.font -> ascent,
	    &bbox);
    }

    /* Update the width and height */
    width = MAX(width, sizes.width);
    height += self -> history.line_height;

    /* FIX THIS: copy the rest down! */

    /* Measure the remaining items */
    for (i = index + 1; i < self -> history.message_count; i++)
    {
	message_view_get_sizes(self -> history.message_views[i], &sizes);
	width = MAX(width, sizes.width);
	height += self -> history.line_height;
    }

    /* Update the widget's dimensions */
    self -> history.width = width + (long)self -> history.margin_width * 2;
    self -> history.height = height + (long)self -> history.margin_height * 2;

    /* Update the scrollbars */
    update_scrollbars((Widget)self);
}


/* Make sure the given index is visible */
static void make_index_visible(HistoryWidget self, unsigned int index)
{
    long y;

    /* Sanity check */
    assert(index < self -> history.message_count);

    /* Figure out where the index would appear */
    y = self -> history.selection_index * self -> history.line_height;

    /* If it's above then scroll up to it */
    if (y + self -> history.margin_height < self -> history.y)
    {
	set_origin(self, self -> history.x, y + self -> history.margin_height, 1);
	return;
    }

    /* If it's below then scroll down to it */
    if (self -> history.y + self -> core.height - self -> history.margin_height <
	y + self -> history.line_height)
    {
	set_origin(
	    self, self -> history.x,
	    y + self -> history.line_height - self -> core.height +
	    self -> history.margin_height, 1);
	return;
    }
}

/* Selects the given message at the given index */
static void set_selection(HistoryWidget self, unsigned int index, message_t message)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self -> history.gc;
    XGCValues values;
    XRectangle bbox;
    long y;

    /* Bail if the selection is unchanged */
    if (self -> history.selection == message)
    {
	return;
    }

    /* Set up a bounding box */
    bbox.x = 0;
    bbox.y = 0;
    bbox.width = self -> core.width;
    bbox.height = self -> core.height;

    /* Drop our extra reference to the old selection */
    if (self -> history.selection != NULL)
    {
	message_free(self -> history.selection);
    }

    /* Redraw the old selection if appropriate */
    if (gc != None && self -> history.selection_index != (unsigned int)-1)
    {
	/* Determine the location of the old selection */
	y = self -> history.selection_index * self -> history.line_height -
	    self -> history.y + self -> history.margin_height;

	/* Is it visible? */
	if (0 <= y + self -> history.line_height && y < (long)self -> history.height)
	{
	    /* Clear the clip mask */
	    values.clip_mask = None;
	    values.foreground = self -> core.background_pixel;
	    XChangeGC(display, gc, GCClipMask | GCForeground, &values);

	    /* Erase the rectangle previously occupied by the message */
	    XFillRectangle(
		display, window, gc,
		0, y, self -> core.width, self -> history.line_height);

	    /* And then draw it again */
	    message_view_paint(
		self -> history.message_views[self -> history.selection_index],
		display, window, gc,
		self -> history.group_pixel, self -> history.user_pixel,
		self -> history.string_pixel, self -> history.separator_pixel,
		self -> history.margin_width - self -> history.x,
		y + self -> history.font -> ascent,
		&bbox);
	}
    }

    /* Record the new selection */
    if (message == NULL) {
	self -> history.selection = NULL;
    } else {
	self -> history.selection = message_alloc_reference(message);
    }

    /* And record its index */
    self -> history.selection_index = index;

    /* Draw the new selection if appropriate */
    if (gc != None && self -> history.selection_index != (unsigned int)-1)
    {
	/* Determine its location */
	y = self -> history.selection_index * self -> history.line_height -
	    self -> history.y + self -> history.margin_height;

	/* Is it visible? */
	if (0 <= y + self -> history.line_height && y < (long)self -> history.height)
	{
	    /* Clear the clip mask */
	    values.clip_mask = None;
	    values.foreground = self -> history.selection_pixel;
	    XChangeGC(display, gc, GCClipMask | GCForeground, &values);

	    /* Draw the selection rectangle */
	    XFillRectangle(
		display, window, gc,
		0, y, self -> core.width, self -> history.line_height);

	    /* And then draw the message view on top of it */
	    message_view_paint(
		self -> history.message_views[self -> history.selection_index],
		display, window, gc,
		self -> history.group_pixel, self -> history.user_pixel,
		self -> history.string_pixel, self -> history.separator_pixel,
		self -> history.margin_width - self -> history.x,
		y + self -> history.font -> ascent,
		&bbox);
	}

	/* Try to make the entire selection is visible */
	make_index_visible(self, index);
    }

    /* Call the callback with our new selection */
    XtCallCallbackList((Widget)self, self -> history.callbacks, (XtPointer)message);
}

/* Selects the message at the given index */
static void set_selection_index(HistoryWidget self, unsigned int index)
{
    message_view_t view;

    /* Locate the message view at that index */
    if (index < self -> history.message_count)
    {
	view = self -> history.message_views[index];
    }
    else
    {
	index = (unsigned int)-1;
	view = NULL;
    }

    /* Select it */
    set_selection(self, index, view ? message_view_get_message(view) : NULL);
}

/* Destroy the widget */
static void destroy(Widget self)
{
    dprintf(("History: destroy()\n"));
}


/* Resize the widget */
static void resize(Widget widget)
{
    /* Update the scrollbar sizes */
    update_scrollbars(widget);
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


/*
 *
 *  Action definitions
 *
 */

/* Scroll up or down, wait and repeat */
static void drag_timeout_cb(XtPointer closure, XtIntervalId *id)
{
    HistoryWidget self = (HistoryWidget)closure;

    /* Scroll in the appropriate direction */
    switch (self -> history.drag_direction)
    {
	/* We're dragging upwards */
	case DRAG_UP:
	{
	    select_previous((Widget)self, NULL, NULL, NULL);
	    break;
	}

	/* We're dragging downwards */
	case DRAG_DOWN:
	{
	    select_next((Widget)self, NULL, NULL, NULL);
	    break;
	}

	/* We're in trouble */
	default:
	{
	    printf("unlikely drag direction: %d\n", self -> history.drag_direction);
	    abort();
	}
    }

    /* Arrange to scroll again after a judicious pause */
    /* FIX THIS: use a resource to specify the delay */
    self -> history.drag_timeout = XtAppAddTimeOut(
	XtWidgetToApplicationContext((Widget)self), self -> history.drag_delay,
	drag_timeout_cb, (XtPointer)self);
}

/* Dragging selects */
static void drag(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;
    unsigned int index;


    /* Is the pointer above the widget? */
    if (button_event -> y < 0)
    {
	/* Is a timeout already registered? */
	if (self -> history.drag_timeout != None)
	{
	    /* Bail if we're already scrolling up */
	    if (self -> history.drag_direction == DRAG_UP)
	    {
		return;
	    }

	    /* Otherwise we're scrolling the wrong way!  Cancel the
	     * timeout! */
	    printf("*\n");
	    XtRemoveTimeOut(self -> history.drag_timeout);
	    self -> history.drag_timeout = None;
	}

	/* Call the timeout callback to scroll up and set a timeout */
	self -> history.drag_direction = DRAG_UP;
	drag_timeout_cb((XtPointer)self, &self -> history.drag_timeout);
	return;
    }

    /* Is the pointer below the widget? */
    if (self -> core.height < button_event -> y)
    {
	/* Is a timeout already registered? */
	if (self -> history.drag_timeout != None)
	{
	    /* Bail if we're already scrolling down */
	    if (self -> history.drag_direction == DRAG_DOWN)
	    {
		return;
	    }

	    /* Otherwise we're scrolling the wrong way!  Cancel the
	     * timeout. */
	    printf("*\n");
	    XtRemoveTimeOut(self -> history.drag_timeout);
	    self -> history.drag_timeout = None;
	}

	/* Call the timeout callback to scroll down and set a timeout */
	self -> history.drag_direction = DRAG_DOWN;
	drag_timeout_cb((XtPointer)self, &self -> history.drag_timeout);
	return;
    }

    /* The y coordinate is within the bounds of the widget.  Cancel
     * any outstanding callbacks */
    if (self -> history.drag_timeout)
    {
	XtRemoveTimeOut(self -> history.drag_timeout);
	self -> history.drag_timeout = None;
    }

    /* Find the index of the message to select */
    index = (self -> history.y + button_event -> y - self -> history.margin_height) /
	self -> history.line_height;

    /* Do we have that many messages? */
    if (! (index < self -> history.message_count))
    {
	index = self -> history.message_count - 1;
    }

    /* Select it */
    set_selection_index(self, index);
}

/* End of the dragging */
static void drag_done(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;

    /* Cancel any pending timeout */
    if (self -> history.drag_timeout != None)
    {
	XtRemoveTimeOut(self -> history.drag_timeout);
	self -> history.drag_timeout = None;
    }
}

/* Select the item under the mouse */
static void do_select(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;
    unsigned int index;

    /* Convert the y coordinate into an index */
    index = (self -> history.y + button_event -> y - self -> history.margin_height) /
	self -> history.line_height;

    /* Are we selecting past the end of the list? */
    if (! (index < self -> history.message_count))
    {
	index = self -> history.message_count - 1;
    }

    /* Set the selection */
    set_selection_index(self, index);
}

/* Select or deselect the item under the pointer */
static void toggle_selection(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;
    unsigned int index;

    /* Convert the y coordinate into an index */
    index = (self -> history.y + button_event -> y - self -> history.margin_height) /
	self -> history.line_height;

    /* Are we selecting past the end of the list? */
    if (! (index < self -> history.message_count))
    {
	index = self -> history.message_count - 1;
    }

    /* Is this our current selection? */
    if (self -> history.selection_index == index)
    {
	/* Yes.  Toggle the selection off */
	index = (unsigned int)-1;
    }

    /* Set the selection */
    set_selection_index(self, index);
}


/* Call the attachment callbacks */
static void show_attachment(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;

    /* Finish the dragging */
    if (self -> history.drag_timeout != None)
    {
	XtRemoveTimeOut(self -> history.drag_timeout);
	self -> history.drag_timeout = None;
    }

    /* Call the attachment callbacks */
    if (self -> history.selection)
    {
	XtCallCallbackList(
	    widget,
	    self -> history.attachment_callbacks,
	    (XtPointer)self -> history.selection);
    }
}


/* Select the previous item in the history */
static void select_previous(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int index;

    /* Is anything selected? */
    if (self -> history.selection_index == (unsigned int)-1)
    {
	/* No.  Prepare to select the last item in the list */
	index = self -> history.message_count;
    }
    else
    {
	/* Yes.  Prepare to select the one before it */
	index = self -> history.selection_index;
    }

    /* Bail if there is no previous item */
    if (index == 0)
    {
	return;
    }

    /* Select the previous item */
    set_selection_index(self, index - 1);
}

/* Select the next item in the history */
static void select_next(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int index;

    /* Prepare to select the next item */
    index = self -> history.selection_index + 1;

    /* Bail if there is no next item */
    if (! (index < self -> history.message_count))
    {
	return;
    }

    /* Select the next item */
    set_selection_index(self, index);
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
    HistoryWidget self = (HistoryWidget)widget;

    /* For now just insert at the end */
    insert_message(self, self -> history.message_count, message);
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
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int i;

    /* Find the index of the message */
    for (i = 0; i < self -> history.message_count; i++)
    {
	if (message_view_get_message(self -> history.message_views[i]) == message)
	{
	    set_selection(self, i, message);
	    return;
	}
    }

    /* Not found -- select at -1 */
    set_selection(self, (unsigned int)-1, message);
}

/* Returns the selected message or NULL if none is selected */
message_t HistoryGetSelection(Widget widget)
{
    dprintf(("HistoryGetSelection(): not yet implemented\n"));
    return NULL;
}
