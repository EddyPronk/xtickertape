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
static const char cvsid[] = "$Id: History.c,v 1.1 2001/06/15 12:20:14 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include <Xm/XmAll.h>
#include <Xm/PrimitiveP.h>
#include "message.h"
#include "History.h"
#include "HistoryP.h"


#if 1
#define dprintf(x) printf x
#else
#define dprintf(x) ;
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
static void redisplay(Widget self, XEvent *event, Region region);
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
	XtInheritRealize, /* realize */
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
	redisplay , /* expose */
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
static void vert_drag_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("vert_drag_cb(): not yet implemented\n"));
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
static void horiz_drag_cb(Widget widget, XtPointer client_data, XtPointer call_dta)
{
    dprintf(("horiz_drag_cb(): not yet implemented\n"));
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

    printf("History: init()\n");
}

/* Draws the highlights and shadows */
static void draw_highlights(Display *display,
		       Drawable drawable,
		       GC highlight_gc,
		       GC shadow_gc,
		       int x, int y,
		       int width, int height,
		       Dimension thickness)
{
    XPoint points[5];

    /* Draw the left shadow */
    points[0].x = x;
    points[0].y = y;

    points[1].x = x;
    points[1].y = y + height;

    points[2].x = x + thickness;
    points[2].y = y + height - thickness;

    points[3].x = x + thickness;
    points[3].y = y;

    points[4].x = x;
    points[4].y = y;

    XFillPolygon(
	display, drawable,
	shadow_gc, points, 5,
	Convex, CoordModeOrigin);

    /* Draw the top shadow */
    points[0].x = x;
    points[0].y = y;

    points[1].x = x + width;
    points[1].y = y;

    points[2].x = x + width - thickness;
    points[2].y = y + thickness;

    points[3].x = x;
    points[3].y = y + thickness;

    points[4].x = x;
    points[4].y = y;

    XFillPolygon(
	display, drawable,
	shadow_gc, points, 5,
	Convex, CoordModeOrigin);

    /* Draw the right highlight */
    points[0].x = x + width;
    points[0].y = y + height;

    points[1].x = x + width;
    points[1].y = y;

    points[2].x = x + width - thickness;
    points[2].y = y + thickness;

    points[3].x = x + width - thickness;
    points[3].y = y + height;

    points[4].x = x + width;
    points[4].y = y + height;

    XFillPolygon(
	display, drawable,
	highlight_gc, points, 5,
	Convex, CoordModeOrigin);

    /* Draw the bottom highlight */
    points[0].x = x + width;
    points[0].y = y + height;

    points[1].x = x;
    points[1].y = y + height;

    points[2].x = x + thickness;
    points[2].y = y + height - thickness;

    points[3].x = x + width;
    points[3].y = y + height - thickness;

    points[4].x = x + width;
    points[4].y = y + height;

    XFillPolygon(
	display, drawable,
	highlight_gc, points, 5,
	Convex, CoordModeOrigin);
}

/* Repaint the widget */
static void redisplay(Widget widget, XEvent *event, Region region)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(widget);
    Window window = XtWindow(widget);

    /* Draw the highlights and shadows */
    draw_highlights(display, window,
		    self -> primitive.top_shadow_GC,
		    self -> primitive.bottom_shadow_GC,
		    0, 0, self -> core.width, self -> core.height,
		    self -> primitive.shadow_thickness);

    dprintf(("History: redisplay()\n"));
}


/* Destroy the widget */
static void destroy(Widget self)
{
    dprintf(("History: destroy()\n"));
}


/* Resize the widget */
static void resize(Widget self)
{
    dprintf(("History: resize (width=%d, height=%d, x=%d, y=%d)\n",
	    self -> core.width, self -> core.height,
	    self -> core.x, self -> core.y));
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


/* What should this do? */
static XtGeometryResult query_geometry(
    Widget self,
    XtWidgetGeometry *intended,
    XtWidgetGeometry *preferred)
{
    dprintf(("History: query_geometry()\n"));
    return XtGeometryYes;
}
