/* $Id: Tickertape.c,v 1.1 1997/02/09 06:55:16 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include "TickertapeP.h"

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
#if 0
    /* Pixel groupPixel */
    {
	XtNgroupPixel,
	XtCGroupPixel
	XtRPixel,
	sizeof(Pixel),
	offset(tickertape.groupPixel),
	XtRString,
	XtDefaultForeground /* FIX THIS: should be blue, somehow */
    },

    /* Pixel userPixel */
    {
	XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.userPixel), XtRString, XtDefaultForeground 
    },

    /* Pixel stringPixel */
    {
	XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
	offset(tickertape.stringPixel), XtRString, XtDefaultForeground
    },

    /* Dimension fadeLevels */
    {
	XtNfadeLevels, XtCFadeLevels, XtRDimension, sizeof(Dimension),
	offset(tickertape.fadeLevels), XtRImmediate, (XtPointer)5
    }
#endif /* 0 */
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
 * Method Definitions
 */

/* ARGSUSED */
static void Initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    TickertapeWidget self = (TickertapeWidget) widget;

    fprintf(stderr, "initializing tickertape 0x%p\n", self);
    self -> core.height = 40;
    self -> core.width = 200;
}

/* ARGSUSED */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    fprintf(stderr, "Redisplay 0x%p\n", widget);
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
