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
static const char cvsid[] = "$Id: panel.c,v 1.9 1999/10/07 04:58:59 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <crypt.h>
#include <time.h>
#include <pwd.h>
#include "history.h"
#include "panel.h"

#include <Xm/XmAll.h>
#include <X11/Xmu/Editres.h>


/*
 *
 * Layout (range: [0-3])
 *
 */

/* The position of the "Send" button */
#define SEND_LEFT 0
#define SEND_RIGHT 1

/* The position of the "Clear" button */
#define CLEAR_LEFT 1
#define CLEAR_RIGHT 2

/* The position of the "Cancel" button */
#define CANCEL_LEFT 2
#define CANCEL_RIGHT 3


/* The list of timeouts for the timeout menu */
char *timeouts[] =
{
    "1",
    "5",
    "10",
    "30",
    "60",
    NULL
};

/* The default message timeout */
#define DEFAULT_TIMEOUT "5"

/* The number of milliseconds to wait before showing a tool-tip */
#define TOOL_TIP_DELAY 1000

/* The format of the default user field */
#define USER_FMT "%s@%s"


/* Used in callbacks to indicate both a control_panel_t and a callback */
typedef struct menu_item_tuple *menu_item_tuple_t;
struct menu_item_tuple
{
    /* The tuple's control panel */
    control_panel_t control_panel;

    /* The tuple's push-button widget */
    Widget widget;

    /* The title of the tuple's push-button widget */
    char *title;

    /* The position of the widget in the pull-down menu */
    int index;

    /* The tuple's callback for button-press */
    control_panel_callback_t callback;

    /* The user data for the tuple's callback */
    void *rock;
};

/* The structure of a control panel */
struct control_panel
{
    /* The receiver's tickertape */
    tickertape_t tickertape;

    /* The receiver's top-level widget */
    Widget top;

    /* The receiver's user text widget */
    Widget user;

    /* The receiver's group menu button */
    Widget group;

    /* The receiver's group menu */
    Widget group_menu;

    /* The number of entries in the group menu */
    int group_count;

    /* The receiver's timeout menu button */
    Widget timeout;

    /* The receiver's default timeout menu button */
    Widget default_timeout;

    /* The receiver's mime menu button */
    Widget mime_type;

    /* The receiver's mime args text widget */
    Widget mime_args;

    /* The receiver's text text widget */
    Widget text;

    /* The receiver's "Send" button widget */
    Widget send;

    /* The receiver's history list widget */
    Widget history;

    /* The tool-tip timer */
    XtIntervalId timer;

    /* The tool-tip's shell widget */
    Widget tool_tip;

    /* The tool-tip's label widget */
    Widget tool_tip_label;

    /* The x position of the pointer when the timer was last set */
    Position x;

    /* The y position of the pointer when the timer was last set */
    Position y;

    /* The uuid counter */
    int uuid_count;

    /* The receiver's about box widget */
    Widget about_box;

    /* The currently selected subscription (group) */
    menu_item_tuple_t selection;

    /* The list of timeouts to appear in the timeout menu */
    char **timeouts;

    /* The message to which we are replying (0 if none) */
    char *message_id;
};


/*
 *
 * Static function prototypes
 *
 */
static void file_groups(Widget widget, control_panel_t self, XtPointer unused);
static void file_usenet(Widget widget, control_panel_t self, XtPointer unused);
static void file_exit(Widget widget, control_panel_t self, XtPointer unused);
static void create_file_menu(control_panel_t self, Widget parent);

static void help_about(Widget widget, control_panel_t self, XtPointer unused);
static void create_help_menu(control_panel_t self, Widget parent);

static void options_threaded(Widget w, control_panel_t self, XmToggleButtonCallbackStruct *i);
static void create_options_menu(control_panel_t self, Widget parent);

static void configure_about_box(Widget shell, XtPointer rock, XConfigureEvent *event);
static Widget create_about_box(control_panel_t self, Widget parent);

static Cardinal order_group_menu(Widget item);
static Widget create_group_menu(control_panel_t self, Widget parent);
static Widget create_timeout_menu(control_panel_t self, Widget parent);

void history_selection_callback(Widget widget, control_panel_t self, XmListCallbackStruct *info);
void history_action_callback(Widget widget, control_panel_t self, XmListCallbackStruct *info);
static void create_history_box(control_panel_t self, Widget parent);

static void create_top_box(control_panel_t self, Widget parent);
static void create_text_box(control_panel_t self, Widget parent);
static Widget create_mime_type_menu(control_panel_t self, Widget parent);
static void create_mime_box(control_panel_t self, Widget parent);
static void create_bottom_box(control_panel_t self, Widget parent);

static void configure_control_panel(Widget top, XtPointer rock, XConfigureEvent *event);
static void init_ui(control_panel_t self, Widget parent);

static void select_group(Widget widget, menu_item_tuple_t tuple, XtPointer ignored);
static void set_group_selection(control_panel_t self, menu_item_tuple_t tuple);
static void set_user(control_panel_t self, char *user);
static char *get_user(control_panel_t self);
static void set_text(control_panel_t self, char *text);
static char *get_text(control_panel_t self);
static void reset_timeout(control_panel_t self);
static int get_timeout(control_panel_t self);
static void set_mime_args(control_panel_t self, char *args);
static char *get_mime_args(control_panel_t self);
static char *get_mime_type(control_panel_t self);

static char *crypt_id(time_t now);
char *create_uuid(control_panel_t self);
message_t contruct_message(control_panel_t self);

static void action_send(Widget button, control_panel_t self, XtPointer ignored);
static void action_clear(Widget button, control_panel_t self, XtPointer ignored);
static void action_cancel(Widget button, control_panel_t self, XtPointer ignored);
static void action_return(Widget textField, control_panel_t self, XmAnyCallbackStruct *cbs);
static void action_dismiss(Widget button, control_panel_t self, XtPointer ignored);

static void prepare_reply(control_panel_t self, message_t message);
static void make_index_visible(Widget list, int index);


/* This gets called when the user selects the "reloadGroups" menu item */
static void file_groups(Widget widget, control_panel_t self, XtPointer unused)
{
    tickertape_reload_groups(self -> tickertape);
}

/* This gets called when the user selects the "reloadUsenet" menu item */
static void file_usenet(Widget widget, control_panel_t self, XtPointer unused)
{
    tickertape_reload_usenet(self -> tickertape);
}


/* This gets called when the user selects the "exit" menu item from the file menu */
static void file_exit(Widget widget, control_panel_t self, XtPointer unused)
{
    tickertape_quit(self -> tickertape);
}

/* Construct the File menu widget */
static void create_file_menu(control_panel_t self, Widget parent)
{
    Widget menu;
    Widget item;

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the `reload groups' menu item */
    item = XtVaCreateManagedWidget("reloadGroups", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)file_groups, (XtPointer)self);

    /* Create the `reload usenet' menu item */
    item = XtVaCreateManagedWidget("reloadUsenet", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)file_usenet, (XtPointer)self);

    /* Create a separator */
    XtVaCreateManagedWidget("separator", xmSeparatorGadgetClass, menu, NULL);

    /* Create the "exit" menu item */
    item = XtVaCreateManagedWidget("exit", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)file_exit, (XtPointer)self);

    /* Create the menu's cascade button */
    XtVaCreateManagedWidget(
	"fileMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);
}

/* This is called when the `threaded' toggle button is selected */
static void options_threaded(
    Widget widget,
    control_panel_t self,
    XmToggleButtonCallbackStruct *info)
{
    history_set_threaded(tickertape_history(self -> tickertape), info -> set);
}

/* Creates the `Options' menu */
static void create_options_menu(control_panel_t self, Widget parent)
{
    Widget menu;
    Widget item;

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the `threaded' menu item */
    item = XtVaCreateManagedWidget("threaded", xmToggleButtonGadgetClass, menu, NULL);
    XtAddCallback(
	item, XmNvalueChangedCallback,
	(XtCallbackProc)options_threaded, (XtPointer)self);
    history_set_threaded(tickertape_history(self -> tickertape), XmToggleButtonGetState(item));

    /* Create the menu's cascade button */
    XtVaCreateManagedWidget(
	"optionsMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);
}


/* Sets the minimum size of the `About Box' to its initial dimensions */
static void configure_about_box(Widget shell, XtPointer rock, XConfigureEvent *event)
{
    /* Ignore events that aren't about the structure */
    if (event -> type != ConfigureNotify)
    {
	return;
    }

    /* We won't need to be called again */
    XtRemoveEventHandler(
	shell, StructureNotifyMask, False,
	(XtEventHandler)configure_about_box, rock);

    /* Set the minimum width/height to be the current width/height */
    XtVaSetValues(
	shell,
	XmNminWidth, event -> width,
	XmNminHeight, event -> height,
	NULL);
}

/* Creates the `About Box' */
static Widget create_about_box(control_panel_t self, Widget parent)
{
    Widget top;
    Widget form;
    Widget infoForm;
    Widget title;
    Widget copyright;
    Widget buttonForm;
    Widget button;
    Atom wm_delete_window;
    XmString string;
    char buffer[sizeof(PACKAGE) + sizeof(VERSION)];

    /* Create the shell widget */
    /* FIX THIS: we should be able to use XmUNMAP it breaks lesstif */
    top = XtVaCreatePopupShell(
	"aboutBox", topLevelShellWidgetClass, parent,
	XmNdeleteResponse, XmDO_NOTHING,
	NULL);

    /* Add a handler for the WM_DELETE_WINDOW protocol */
    wm_delete_window = XmInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(
	top, wm_delete_window,
	(XtCallbackProc)action_dismiss, (XtPointer)self);

    /* Add an event handler so that we can set the minimum size to be the initial size */
    XtAddEventHandler(
	top, StructureNotifyMask, False, 
	(XtEventHandler)configure_about_box, (XtPointer)NULL);

    /* Create a Form widget to manage the dialog box's children */
    form = XtVaCreateWidget(
	"aboutBoxForm", xmFormWidgetClass, top,
	NULL);

    /* Create a Form widget to manage the Labels */
    infoForm = XtVaCreateWidget(
	"infoForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);

    /* Create the title label */
    sprintf(buffer, "%s %s", PACKAGE, VERSION);
    string = XmStringCreateSimple(buffer);
    title = XtVaCreateManagedWidget(
	"titleLabel", xmLabelWidgetClass, infoForm,
	XmNlabelString, string,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);
    XmStringFree(string);

    /* Create the copyright label */
    copyright = XtVaCreateManagedWidget(
	"copyrightLabel", xmLabelWidgetClass, infoForm,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, title,
	NULL);

    /* Create the author label */
    XtVaCreateManagedWidget(
	"authorLabel", xmLabelWidgetClass, infoForm,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, copyright,
	NULL);

    XtManageChild(infoForm);

    /* Create a Form widget to manage the button */
    buttonForm = XtVaCreateWidget(
	"buttonForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopWidget, infoForm,
	NULL);

    /* Create the dismiss button */
    button = XtVaCreateManagedWidget(
	"dismiss", xmPushButtonWidgetClass, buttonForm,
	XmNsensitive, True,
	XmNshowAsDefault, True,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_POSITION,
	XmNrightAttachment, XmATTACH_POSITION,
	XmNleftPosition, 40,
	XmNrightPosition, 60,
	NULL);
    XtAddCallback(
	button, XmNactivateCallback,
	(XtCallbackProc)action_dismiss, (XtPointer)self);

    XtManageChild(buttonForm);

    /* Manage all of the widgets */
    XtManageChild(form);
    return top;
}


/* Pop up an about box */
static void help_about(Widget widget, control_panel_t self, XtPointer unused)
{
    if (self -> about_box == NULL)
    {
	self -> about_box = create_about_box(self, XtParent(self -> top));
    }

    XtPopup(self -> about_box, XtGrabNone);
}

/* Construct the Help menu widget */
static void create_help_menu(control_panel_t self, Widget parent)
{
    Widget menu;
    Widget item;
    Widget cascade;

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the "about" menu item */
    item = XtVaCreateManagedWidget("about", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)help_about, (XtPointer)self);

    /* Create the menu's cascade button */
    cascade = XtVaCreateManagedWidget(
	"helpMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);

    /* Tell the menubar that this menu is the help menu */
    XtVaSetValues(parent, XmNmenuHelpWidget, cascade, NULL);
}



/* This function is used to order the elements of the group menu */
static Cardinal order_group_menu(Widget item)
{
    control_panel_t self;
    menu_item_tuple_t tuple;
    XtVaGetValues(item, XmNuserData, &tuple, NULL);

    /* I don't know what this is */
    if (tuple == NULL)
    {
	return 0;
    }

    self = tuple -> control_panel;

    /* Locate the menu item tuple corresponding to the given item */
    return tuple -> index;
}

/* Construct the menu for the Group list */
static Widget create_group_menu(control_panel_t self, Widget parent)
{
    Widget widget;
    Arg args[2];

    /* Create a pulldown menu */
    XtSetArg(args[0], XmNinsertPosition, order_group_menu);
    self -> group_menu = XmCreatePulldownMenu(parent, "_pulldown", args, 1);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, self -> group_menu);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "group", args, 2);

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* Creates the popup menu for timeout selection */
static Widget create_timeout_menu(control_panel_t self, Widget parent)
{
    Widget menu, widget;
    Arg args[2];
    char **pointer;

    /* Create a pulldown menu */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, menu);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "timeout", args, 2);

    /* Fill in the recommended timeout values */
    for (pointer = timeouts; *pointer != NULL; pointer++)
    {
	Widget item;

	item = XtVaCreateManagedWidget(
	    *pointer, xmPushButtonWidgetClass, menu,
	    NULL);

	/* Watch for the default */
	if (strcmp(*pointer, DEFAULT_TIMEOUT) == 0)
	{
	    self -> default_timeout = item;
	}
    }

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* This is called when an item in the history list is selected */
void history_selection_callback(Widget widget, control_panel_t self, XmListCallbackStruct *info)
{
    message_t message;

    /* Reconfigure to reply to the selected message (or lack thereof) */
    message = history_get(tickertape_history(self -> tickertape), info -> item_position);
    prepare_reply(self, message);
}

/* This is called when an item in the history is double-clicked */
void history_action_callback(Widget widget, control_panel_t self, XmListCallbackStruct *info)
{
    message_t message;

    /* Figure out which message was selected */
    if ((message = history_get(
	tickertape_history(self -> tickertape),
	info -> item_position)) == NULL)
    {
	return;
    }

    /* Display its attachment */
    tickertape_show_attachment(self -> tickertape, message);
}



/* Show a tool-tip window */
static void show_tool_tip(control_panel_t self, Widget widget, char *tip, Position x, Position y)
{
    Position absolute_x, absolute_y;
    XmString string;
    int screen_width;
    Dimension width;
    Dimension height;

    /* Set the tool-tip label's text */
    string = XmStringCreateSimple(tip);
    XtVaSetValues(self -> tool_tip_label, XmNlabelString, string, NULL);
    XmStringFree(string);

    /* Figure out how big the tool-tip window is */
    XtVaGetValues(self -> tool_tip_label, XmNwidth, &width, XmNheight, &height, NULL);

    /* Figure out where the pointer is */
    XtTranslateCoords(widget, x, y, &absolute_x, &absolute_y);

    /* Find a good x position for the tool-tip */
    screen_width = WidthOfScreen(XtScreen(self -> top));

    /* Try to the right of the pointer */
    if (absolute_x + width + 10 < screen_width)
    {
	absolute_x += 10;
    }
    /* Then try to the right of the pointer */
    else if (width + 10 < absolute_x)
    {
	absolute_x -= width + 10;
    }
    /* Then try with the right edge flush with the right edge of the screen */
    else if (width < screen_width)
    {
	absolute_x = screen_width - width;
    }
    /* Otherwise give up an go with 0 */
    else
    {
	absolute_x = 0;
    }

    /* Find a convenient y position */
    if (absolute_y + height + 10 < HeightOfScreen(XtScreen(self -> top)))
    {
	absolute_y += 10;
    }
    else
    {
	absolute_y -= (10 + height);
    }

    XtVaSetValues(self -> tool_tip, XmNx, absolute_x, XmNy, absolute_y, NULL);

    /* Show the tool-tip */
    XtPopup(self -> tool_tip, XtGrabNone);
}

/* Hide the tool-tip window */
static void hide_tool_tip(control_panel_t self)
{
    XtPopdown(self -> tool_tip);
}


/* Show a tool-tip for the selected list item */
static void history_timer_callback(control_panel_t self, XtIntervalId *ignored)
{
    message_t message;
    char *mime_args;

    /* Clear the timer */
    self -> timer = 0;

    /* Locate the message under the pointer */
    message = history_get_at_point(tickertape_history(self -> tickertape), self -> x, self -> y);
    if (message == NULL)
    {
	return;
    }

    /* Get its mime args to use for the tool tip */
    if ((mime_args = message_get_mime_args(message)) == NULL)
    {
	return;
    }

    /* Show them in a tool-tip */
    show_tool_tip(self, self -> history, mime_args, self -> x, self -> y);
}

    
/* This is called when the mouse enters or leaves or moves around
 * inside the history widget */
static void history_motion_callback(
    Widget widget,
    control_panel_t self,
    XEvent *event)
{
    switch (event -> type)
    {
	case MotionNotify:
	{
	    /* If we have a timer then reset it */
	    if (self -> timer != 0)
	    {
		XMotionEvent *motion_event = (XMotionEvent *)event;

		XtRemoveTimeOut(self -> timer);
		self -> timer = XtAppAddTimeOut(
		    XtWidgetToApplicationContext(widget),
		    TOOL_TIP_DELAY,
		    (XtTimerCallbackProc)history_timer_callback,
		    (XtPointer)self);
		self -> x = motion_event -> x;
		self -> y = motion_event -> y;
		return;
	    }

	    hide_tool_tip(self);
	}

	/* When the mouse enters the window we set the timer */
	case EnterNotify:
	{
	    XEnterWindowEvent *enter_event = (XEnterWindowEvent *)event;

	    /* Set the timer for a short pause before we show the tool-tip */
	    self -> timer = XtAppAddTimeOut(
		XtWidgetToApplicationContext(widget),
		TOOL_TIP_DELAY,
		(XtTimerCallbackProc)history_timer_callback,
		(XtPointer)self);
	    self -> x = enter_event -> x;
	    self -> y = enter_event -> y;
	    return;
	}

	/* Treat a button release as an enter event */
	case ButtonRelease:
	{
	    XButtonEvent *button_event = (XButtonEvent *)event;

	    /* Set the timer for a short pause before we show the tool-tip */
	    self -> timer = XtAppAddTimeOut(
		XtWidgetToApplicationContext(widget),
		TOOL_TIP_DELAY,
		(XtTimerCallbackProc)history_timer_callback,
		(XtPointer)self);
	    self -> x = button_event -> x;
	    self -> y = button_event -> y;
	    return;
	}

	case ButtonPress:
	case LeaveNotify:
	{
	    /* If we have a timer cancel it.  Otherwise just hide the tool-tip */
	    if (self -> timer != 0)
	    {
		XtRemoveTimeOut(self -> timer);
		self -> timer = 0;
		return;
	    }

	    hide_tool_tip(self);
	    return;
	}
}

/* Constructs the history list */
static void create_history_box(control_panel_t self, Widget parent)
{
    Arg args[9];

    XtSetArg(args[0], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNselectionPolicy, XmBROWSE_SELECT);
    XtSetArg(args[5], XmNitemCount, 0);
    XtSetArg(args[6], XmNvisibleItemCount, 3);
    XtSetArg(args[7], XmNlistSizePolicy, XmCONSTANT);
    XtSetArg(args[8], XmNscrollBarDisplayPolicy, XmSTATIC);
    self -> history = XmCreateScrolledList(parent, "history", args, 9);

    /* Add callbacks for interesting things */
    XtAddCallback(
	self -> history, XmNbrowseSelectionCallback,
	(XtCallbackProc)history_selection_callback, (XtPointer)self);
    XtAddCallback(
	self -> history, XmNdefaultActionCallback,
	(XtCallbackProc)history_action_callback, (XtPointer)self);
    XtAddEventHandler(
 	self -> history,
	EnterWindowMask | LeaveWindowMask | PointerMotionMask | 
	ButtonPressMask | ButtonReleaseMask,
	False,
	(XtEventHandler)history_motion_callback,
	(XtPointer)self);

    XtManageChild(self -> history);

    /* Tell the tickertape's history to use this XmList widget */
    history_set_list(tickertape_history(self -> tickertape), self -> history);
}

/* Constructs the top box of the Control Panel */
static void create_top_box(control_panel_t self, Widget parent)
{
    Widget form, label;

    /* Create a layout manager for the User Label and TextField */
    form = XtVaCreateWidget(
	"userForm", xmFormWidgetClass, parent,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* The "User" label */
    label = XtVaCreateManagedWidget(
	"userLabel", xmLabelWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* The "User" text field */
    self -> user = XtVaCreateManagedWidget(
	"user", xmTextFieldWidgetClass, form,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNleftWidget, label,
	NULL);

    /* Manage the form widget now that all of its children are created */
    XtManageChild(form);

    /* The "Timeout" label */
    self -> timeout = create_timeout_menu(self, parent);
    XtVaSetValues(
	self -> timeout,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* The "Group" menu button */
    self -> group = create_group_menu(self, parent);
    XtVaSetValues(
	self -> group,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNrightAttachment, XmATTACH_WIDGET,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftWidget, form,
	XmNrightWidget, self -> timeout,
	NULL);
}


/* Constructs the Text box */
static void create_text_box(control_panel_t self, Widget parent)
{
    Widget label;

    /* Create the label for the text field */
    label = XtVaCreateManagedWidget(
	"textLabel", xmLabelWidgetClass, parent,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* Create the text field itself */
    self -> text = XtVaCreateManagedWidget(
	"text", xmTextFieldWidgetClass, parent,
	XmNtraversalOn, True,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNrightAttachment, XmATTACH_FORM,
	XmNleftWidget, label,
	NULL);

    /* Add a callback to the text field so that we can send the
     * notification if the user hits Return */
    XtAddCallback(
	self -> text, XmNactivateCallback,
	(XtCallbackProc)action_return, (XtPointer)self);
}

/* Constructs the Mime type menu */
static Widget create_mime_type_menu(control_panel_t self, Widget parent)
{
    Widget pulldown, widget;
    Arg args[2];

    /* Create a pulldown menu */
    pulldown = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, pulldown);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "mime_type", args, 2);

    /* Add the old URL mime type to it */
    XtVaCreateManagedWidget(
	"x-elvin/url", xmPushButtonGadgetClass, pulldown,
	NULL);

    /* Add the accepted URL mime type to it */
    XtVaCreateManagedWidget(
	"text/uri-list", xmPushButtonGadgetClass, pulldown,
	NULL);

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* Constructs the Mime box */
static void create_mime_box(control_panel_t self, Widget parent)
{
    self -> mime_type = create_mime_type_menu(self, parent);
    XtVaSetValues(
	self -> mime_type,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    self -> mime_args = XtVaCreateManagedWidget(
	"mime_args", xmTextFieldWidgetClass, parent,
	XmNtraversalOn, True,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNrightAttachment, XmATTACH_FORM,
	XmNleftWidget, self -> mime_type,
	NULL);

    /* Add a callback to the text field so that we can send the
     * notification if the user hits Return */
    XtAddCallback(
	self -> mime_args, XmNactivateCallback,
	(XtCallbackProc)action_return, (XtPointer)self);
}

/* Creates the bottom box (where the buttons live) */
static void create_bottom_box(control_panel_t self, Widget parent)
{
    Widget button;

    /* Create the "Send" button */
    self -> send = XtVaCreateManagedWidget(
	"send", xmPushButtonWidgetClass, parent,
	XtNsensitive, True,
	XmNshowAsDefault, True,
	XmNleftAttachment, XmATTACH_POSITION,
	XmNrightAttachment, XmATTACH_POSITION,
	XmNleftPosition, SEND_LEFT,
	XmNrightPosition, SEND_RIGHT,
	NULL);

    /* Have it call action_send when it gets pressed */
    XtAddCallback(
	self -> send, XmNactivateCallback,
	(XtCallbackProc)action_send, (XtPointer)self);


    /* Create the "Clear" button */
    button = XtVaCreateManagedWidget(
	"clear", xmPushButtonWidgetClass, parent,
	XtNsensitive, True,
	XmNtraversalOn, False,
	XmNdefaultButtonShadowThickness, 1,
	XmNleftAttachment, XmATTACH_POSITION,
	XmNrightAttachment, XmATTACH_POSITION,
	XmNleftPosition, CLEAR_LEFT,
	XmNrightPosition, CLEAR_RIGHT,
	NULL);

    /* Have it call action_clear when it gets pressed */
    XtAddCallback(
	button, XmNactivateCallback,
	(XtCallbackProc)action_clear, (XtPointer)self);


    /* Create the "Cancel" button */
    button = XtVaCreateManagedWidget(
	"cancel", xmPushButtonWidgetClass, parent,
	XtNsensitive, True,
	XmNtraversalOn, False,
	XmNdefaultButtonShadowThickness, 1,
	XmNleftAttachment, XmATTACH_POSITION,
	XmNrightAttachment, XmATTACH_POSITION,
	XmNleftPosition, CANCEL_LEFT,
	XmNrightPosition, CANCEL_RIGHT,
	NULL);

    /* Have it call action_cancel when it gets pressed */
    XtAddCallback(
	button, XmNactivateCallback,
	(XtCallbackProc)action_cancel, (XtPointer)self);
}

/* Configures the control panel's minimum dimensions */
static void configure_control_panel(Widget top, XtPointer rock, XConfigureEvent *event)
{
    control_panel_t self = (control_panel_t)rock;
    Dimension history_height;

    /* Make sure we've got a ConfigureNotify event */
    if (event -> type != ConfigureNotify)
    {
	return;
    }

    /* We're no longer interested in configure events */
    XtRemoveEventHandler(
	top, StructureNotifyMask, False,
	(XtEventHandler)configure_control_panel, rock);

    /* Figure out how tall the history list is */
    XtVaGetValues(
	self -> history,
	XmNheight, &history_height,
	NULL);

    /* Set the minimum width and height */
    XtVaSetValues(
	top,
	XmNminWidth, event -> width,
	XmNminHeight, event -> height - history_height,
	NULL);
}

/* Constructs the entire control panel */
static void init_ui(control_panel_t self, Widget parent)
{
    Widget form;
    Widget menubar;
    Widget historyForm;
    Widget topForm;
    Widget textForm;
    Widget mimeForm;
    Widget buttonForm;
    Atom wm_delete_window;

    /* Set some variables to sane values */
    self -> about_box = NULL;

    /* Create a popup shell for the receiver */
    self -> top = XtVaCreatePopupShell(
	"controlPanel", topLevelShellWidgetClass, parent,
	XmNdeleteResponse, XmDO_NOTHING,
	NULL);

    /* Add a handler for the WM_DELETE_WINDOW protocol */
    wm_delete_window = XmInternAtom(XtDisplay(self -> top), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(self -> top, wm_delete_window, (XtCallbackProc)action_cancel, (XtPointer)self);

    /* Register an event handler for the first structure event */
    XtAddEventHandler(
	self -> top, StructureNotifyMask, False,
	(XtEventHandler)configure_control_panel, (XtPointer)self);

    /* Create a form widget to manage the dialog box's children */
    form = XtVaCreateWidget(
	"controlPanelForm", xmFormWidgetClass, self -> top,
	NULL);

    /* Create a menubar for the receiver */
    menubar = XmVaCreateSimpleMenuBar(
	form, "menuBar",
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);

    /* Create the `file' menu */
    create_file_menu(self, menubar);

    /* Create the `options' menu */
    create_options_menu(self, menubar);

    /* Create the `help' menu and tell Motif that it's the Help menu */
    create_help_menu(self, menubar);

    /* Manage the menubar */
    XtManageChild(menubar);

    /* Create the top box */
    topForm = XtVaCreateWidget(
	"topForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, menubar,
	NULL);

    create_top_box(self, topForm);
    XtManageChild(topForm);

    /* Create the text box */
    textForm = XtVaCreateWidget(
	"textForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, topForm,
	NULL);
    create_text_box(self, textForm);
    XtManageChild(textForm);

    /* Create the mime box */
    mimeForm = XtVaCreateWidget(
	"mimeForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, textForm,
	NULL);
    create_mime_box(self, mimeForm);
    XtManageChild(mimeForm);

    /* Create the buttons across the bottom */
    buttonForm = XtVaCreateWidget(
	"buttonForm", xmFormWidgetClass, form,
	XmNfractionBase, 3,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, mimeForm,
	NULL);
    create_bottom_box(self, buttonForm);
    XtManageChild(buttonForm);

    /* Create the history list */
    historyForm = XtVaCreateWidget(
	"historyForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopWidget, buttonForm,
	NULL);

    create_history_box(self, historyForm);
    XtManageChild(historyForm);

    /* Manage the form widget */
    XtManageChild(form);


    /* Create the tool-tip widget */
    self -> tool_tip = XtVaCreatePopupShell(
	"toolTipShell", transientShellWidgetClass, self -> top,
	XmNoverrideRedirect, True,
	XmNallowShellResize, True,
	NULL);

    /* And set its child widget to be a label */
    self -> tool_tip_label = XtVaCreateManagedWidget(
	"toolTip", xmLabelWidgetClass, self -> tool_tip,
	NULL);

    /* Make sure that we can use Editres */
    XtAddEventHandler(self -> top, (EventMask)0, True, _XEditResCheckMessages, NULL);
}


/* Sets the Option menu's selection */

/* This gets called when the user selects a group from the UI */
static void select_group(Widget widget, menu_item_tuple_t tuple, XtPointer ignored)
{
    control_panel_t self = tuple -> control_panel;

    if (self -> selection != tuple)
    {
	self -> selection = tuple;
	XmListDeselectAllItems(self -> history);
    }

    /* Changing the group prevents a reply */
    if (self -> message_id != NULL)
    {
	free(self -> message_id);
	self -> message_id = NULL;
    }
}

/* Programmatically sets the selection in the Group option */
static void set_group_selection(control_panel_t self, menu_item_tuple_t tuple)
{
    if (tuple == NULL)
    {
	self -> selection = NULL;
    }
    else
    {
	/* Tell Motif that the menu item was selected, which will call select_group for us */
	XtCallActionProc(tuple -> widget, "ArmAndActivate", NULL, NULL, 0);
    }
}


/* Sets the receiver's user */
static void set_user(control_panel_t self, char *user)
{
    XmTextSetString(self -> user, user);
}


/* Answers the receiver's user */
static char *get_user(control_panel_t self)
{
    char *user;

    XtVaGetValues(self -> user, XmNvalue, &user, NULL);
    return user;
}


/* Sets the receiver's text */
static void set_text(control_panel_t self, char *text)
{
    XmTextSetString(self -> text, text);
}

/* Answers the receiver's text */
static char *get_text(control_panel_t self)
{
    char *text;

    XtVaGetValues(self -> text, XmNvalue, &text, NULL);
    return text;
}


/* Sets the receiver's timeout */
static void reset_timeout(control_panel_t self)
{
    /* Pretend that the user selected the default timeout item */
    XtCallActionProc(
	self -> default_timeout,
	"ArmAndActivate", NULL,
	NULL, 0);
}

/* Answers the receiver's timeout */
static int get_timeout(control_panel_t self)
{
    XmString string;
    char *timeout;
    int result;

    XtVaGetValues(XmOptionButtonGadget(self -> timeout), XmNlabelString, &string, NULL);
    XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &timeout);
    XmStringFree(string);

    result = atoi(timeout);
    XtFree(timeout);
    return result;
}

/* Sets the receiver's MIME args */
static void set_mime_args(control_panel_t self, char *args)
{
    XmTextSetString(self -> mime_args, args);
}

/* Answers the receiver's MIME args */
static char *get_mime_args(control_panel_t self)
{
    char *args;

    XtVaGetValues(self -> mime_args, XmNvalue, &args, NULL);

    if (*args != '\0')
    {
	return args;
    }
    else
    {
	return NULL;
    }
}

/* Answers the receiver's MIME type */
static char *get_mime_type(control_panel_t self)
{
    XmString string;
    char *mime_type;

    XtVaGetValues(XmOptionButtonGadget(self -> mime_type), XmNlabelString, &string, NULL);
    XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &mime_type);
    XmStringFree(string);

    return mime_type;
}


/* Transforms an integer into a digit for use in a crypt salt */
static char *crypt_id(time_t now)
{
    static char chars[] = 
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./";
    char hostid[9];
    char salt[3];

    /* Construct a salt out of the current time */
    salt[0] = chars[now & 0x3f];
    salt[1] = chars[(now >> 6) & 0x3f];
    salt[2] = '\0';

    /* Print the hostid into a buffer */
    sprintf(hostid, "%08lx", (long)gethostid());

    /* Return the encrypted result */
    return crypt(hostid, salt);
}

/* Generates a universally unique identifier for a message */
char *create_uuid(control_panel_t self)
{
    char *cryptid;
    char *buffer;
    time_t now;
    struct tm *tm_gmt;

    /* Get the time -- we'll fix this if time() ever fails */
    if (time(&now) == (time_t) -1)
    {
	perror("unable to determine current time");
	exit(1);
    }

    /* Look up what that means in GMT */
    tm_gmt = gmtime(&now);

    /* Construct an encrypted hostid using the time as the salt */
    cryptid = crypt_id(now);

    /* Construct the UUID */
    buffer = (char *)malloc(sizeof("YYYYMMDDHHMMSS--1234-12") + strlen(cryptid));
    sprintf(buffer, "%04x%02x%02x%02x%02x%02x-%s-%04x-%02x",
	    tm_gmt -> tm_year + 1900, tm_gmt -> tm_mon + 1, tm_gmt -> tm_mday,
	    tm_gmt -> tm_hour, tm_gmt -> tm_min, tm_gmt -> tm_sec,
	    cryptid, (int)getpid(), self -> uuid_count);

    self -> uuid_count = (self -> uuid_count + 1) % 256;
    return buffer;
}

/* Answers a message based on the receiver's current state */
message_t contruct_message(control_panel_t self)
{
    char *mime_type;
    char *mime_args;
    char *uuid;
    message_t message;

    /* Determine our MIME args */
    mime_type = get_mime_type(self);
    mime_args = get_mime_args(self);
    uuid = create_uuid(self);

    /* Construct a message */
    message = message_alloc(
	self -> selection,
	self -> selection -> title,
	get_user(self),
	get_text(self),
	get_timeout(self),
	(mime_args == NULL) ? NULL : mime_type,
	mime_args,
	uuid,
	self -> message_id);

    if (mime_type != NULL)
    {
	XtFree(mime_type);
    }

    free(uuid);
    return message;
}

/*
 *
 * Actions
 *
 */

/* Callback for `Send' button */
static void action_send(Widget button, control_panel_t self, XtPointer ignored)
{
    if (self -> selection != NULL)
    {
	message_t message = contruct_message(self);
	(*self -> selection -> callback)(self -> selection -> rock, message);
	message_free(message);
    }

    XtPopdown(self -> top);
}

/* Callback for `Clear' button */
static void action_clear(Widget button, control_panel_t self, XtPointer ignored)
{
    char *user;
    char *domain;
    char *buffer;

    /* Set the user to be ``user@domain'' */
    user = tickertape_user_name(self -> tickertape);
    domain = tickertape_domain_name(self -> tickertape);
    if ((buffer = (char *)malloc(strlen(USER_FMT) + strlen(user) + strlen(domain) - 3)) != NULL)
    {
	sprintf(buffer, USER_FMT, user, domain);
	set_user(self, buffer);
	free(buffer);
    }
    else
    {
	set_user(self, user);
    }

    set_text(self, "");
    set_mime_args(self, "");
    reset_timeout(self);
}

/* Callback for `Cancel' button */
static void action_cancel(Widget button, control_panel_t self, XtPointer ignored)
{
    XtPopdown(self -> top);
}


/* Call back for the Return key in the text and mime_args fields */
static void action_return(Widget textField, control_panel_t self, XmAnyCallbackStruct *cbs)
{
    XtCallActionProc(
	self -> send, "ArmAndActivate", cbs -> event, 
	NULL, 0);
}

/* Callback for the `Dismiss' button on the about box */
static void action_dismiss(Widget button, control_panel_t self, XtPointer ignored)
{
    /* This check is probably a bit paranoid, but it doesn't hurt... */
    if (self -> about_box != NULL)
    {
	XtPopdown(self -> about_box);
    }
}


/*
 *
 * Exported functions
 *
 */

/* Constructs the Tickertape Control Panel */
control_panel_t control_panel_alloc(tickertape_t tickertape, Widget parent)
{
    control_panel_t self = (control_panel_t)malloc(sizeof(struct control_panel));

    /* Set the receiver's contents to something sane */
    self -> tickertape = tickertape;
    self -> top = NULL;
    self -> user = NULL;
    self -> group = NULL;
    self -> group_menu = NULL;
    self -> group_count = 0;
    self -> timeout = NULL;
    self -> default_timeout = NULL;
    self -> mime_type = NULL;
    self -> mime_args = NULL;
    self -> text = NULL;
    self -> send = NULL;
    self -> history = NULL;
    self -> timer = 0;
    self -> x = 0;
    self -> y = 0;
    self -> uuid_count = 0;
    self -> selection = NULL;
    self -> timeouts = timeouts;
    self -> message_id = NULL;

    /* Initialize the UI */
    init_ui(self, parent);

    /* Set the various fields to their default values */
    action_clear(self -> top, self, NULL);
    return self;
}

/* Releases the resources used by the receiver */
void control_panel_free(control_panel_t self)
{
    free(self);
}



/* Adds a subscription to the receiver at the end of the groups list */
void *control_panel_add_subscription(
    control_panel_t self, char *title,
    control_panel_callback_t callback, void *rock)
{
    XmString string;
    menu_item_tuple_t tuple;

    /* Create a tuple to hold callback information */
    if ((tuple = (menu_item_tuple_t)malloc(sizeof(struct menu_item_tuple))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

    /* Fill in the values of the tuple */
    tuple -> control_panel = self;
    tuple -> title = strdup(title);
    tuple -> index = self -> group_count++;
    tuple -> callback = callback;
    tuple -> rock = rock;
    
    /* Create a menu item for the subscription.
     * Use the user data field to point to the tuple so that we can
     * find our way back to the control panel when we want to order the 
     * menu items */
    string = XmStringCreateSimple(title);
    tuple -> widget = XtVaCreateManagedWidget(
	title, xmPushButtonWidgetClass, self -> group_menu,
	XmNlabelString, string,
	XmNuserData, tuple,
	NULL);
    XmStringFree(string);

    /* Add a callback to update the selection when the item is selected */
    XtAddCallback(
	tuple -> widget, XmNactivateCallback,
	(XtCallbackProc)select_group, (XtPointer)tuple);

    /* If this is our first menu item, then select it by default */
    if (self -> selection == NULL)
    {
	set_group_selection(self, tuple);
    }

    return tuple;
}


/* Removes a subscription from the receiver (tuple was returned by addSubscription) */
void control_panel_remove_subscription(control_panel_t self, void *info)
{
    menu_item_tuple_t tuple = (menu_item_tuple_t)info;
    menu_item_tuple_t first_tuple = NULL;
    Widget *children;
    Widget *child;
    int num_children;

    /* Make sure we haven't already remove it */
    if (tuple -> widget == NULL)
    {
	return;
    }

    /* Destroy it's widget so it isn't in the menu anymore */
    XtDestroyWidget(tuple -> widget);
    tuple -> widget = NULL;
    self -> group_count--;

    /* Update the indices of the other tuples */
    XtVaGetValues(
	self -> group_menu,
	XmNnumChildren, &num_children,
	XmNchildren, &children,
	NULL);

    for (child = children; child < children + num_children; child++)
    {
	menu_item_tuple_t child_tuple;

	XtVaGetValues(*child, XmNuserData, &child_tuple, NULL);
	if (child != NULL)
	{
	    /* Pull the following children up */
	    if (tuple -> index < child_tuple -> index)
	    {
		child_tuple -> index--;
	    }

	    /* Remember the first tuple just in case */
	    if ((child_tuple -> index == 0) && (child_tuple -> widget != NULL))
	    {
		first_tuple = child_tuple;
	    }
	}
    }

    /* If it's the selected item, then change the selection to something else */
    if (self -> selection == tuple)
    {
	set_group_selection(self, first_tuple);
    }

    /* Free up some memory */
    free(tuple -> title);
    free(tuple);
}

/* Changes the location of the subscription within the control panel */
void control_panel_set_index(control_panel_t self, void *info, int index)
{
    menu_item_tuple_t tuple = (menu_item_tuple_t)info;
    XmString string;
    Cardinal num_children;
    Widget *children;
    Widget *child;

    /* Bail now if the tuple isn't registered */
    if (tuple -> widget == NULL)
    {
	return;
    }

    /* Remove the item from the menu... */
    XtDestroyWidget(tuple -> widget);
    tuple -> widget = NULL;

    /* Update the indices of the other tuples */
    XtVaGetValues(
	self -> group_menu,
	XmNnumChildren, &num_children,
	XmNchildren, &children,
	NULL);
    for (child = children; child < children + num_children; child++)
    {
	menu_item_tuple_t child_tuple;

	XtVaGetValues(*child, XmNuserData, &child_tuple, NULL);
	if (child != NULL)
	{
	    /* Push the following children along */
	    if (index <= child_tuple -> index)
	    {
		child_tuple -> index++;
	    }
	}
    }

    /* ... and put it back in the proper locations */
    tuple -> index = index;
    string = XmStringCreateSimple(tuple -> title);
    tuple -> widget = XtVaCreateManagedWidget(
	tuple -> title, xmPushButtonWidgetClass, self -> group_menu,
	XmNlabelString, string,
	XmNuserData, tuple,
	NULL);
    XmStringFree(string);

    /* Add a callback to update the selection when the item is specified */
    XtAddCallback(
	tuple -> widget, XmNactivateCallback,
	(XtCallbackProc)select_group, (XtPointer)tuple);

    /* If it was the selection, then make sure it is still selected in the menu */
    if (self -> selection == tuple)
    {
	set_group_selection(self, tuple);
    }
}

/* Retitles an entry */
void control_panel_retitle_subscription(control_panel_t self, void *info, char *title)
{
    menu_item_tuple_t tuple;
    XmString string;

    /* Update the tuple's label */
    tuple = (menu_item_tuple_t)info;
    free(tuple -> title);
    tuple -> title = strdup(title);

    /* Update the menu item's label */
    string = XmStringCreateSimple(title);
    XtVaSetValues(
	tuple -> widget,
	XmNlabelString, string,
	NULL);
    XmStringFree(string);

    /* If this is the selected item, then update the MenuButton's label as well */
    if (self -> selection == tuple)
    {
	set_group_selection(self, tuple);
    }
}

/* Sets up the receiver to reply to the given message */
static void prepare_reply(control_panel_t self, message_t message)
{
    menu_item_tuple_t tuple;

    /* Free the old message id */
    free(self -> message_id);
    self -> message_id = NULL;

    /* If a message_t was provided that is in the menu, then select it
     * and set the message_id so that we can do threading */
    if (message != NULL)
    {
	char *id;

	if ((tuple = (menu_item_tuple_t)message_get_info(message)) != NULL)
	{
	    self -> selection = tuple;
	    set_group_selection(self, tuple);
	}

	id = message_get_id(message);
	if (id != NULL)
	{
	    self -> message_id = strdup(id);
	}
    }
}

/* Ensures that the given list item is visible */
static void make_index_visible(Widget list, int index)
{
    int top;
    int count;

    /* Figure out what we're looking at */
    XtVaGetValues(
	list,
	XmNtopItemPosition, &top,
	XmNvisibleItemCount, &count,
	NULL);

    /* Make the index the top if it's above the top */
    if (index < top)
    {
	XmListSetPos(list, index);
	return;
    }

    /* Make the index the bottom if it's below the bottom */
    if (! (index < top + count))
    {
	XmListSetBottomPos(list, index);
	return;
    }
}

/* Makes the control panel window visible */
void control_panel_select(control_panel_t self, message_t message)
{
    int index;

    /* Set up to reply */
    prepare_reply(self, message);

    /* Unselect everything */
    XmListDeselectAllItems(self -> history);

    /* If the message isn't visible in the history list then don't select anything */
    if ((index = history_index(tickertape_history(self -> tickertape), message)) < 0)
    {
	return;
    }

    /* Select the item and make sure it is visible */
    make_index_visible(self -> history, index);
    XmListSelectPos(self -> history, index, False);
}

/* Makes the control panel window visible */
void control_panel_show(control_panel_t self)
{
    XtPopup(self -> top, XtGrabNone);
}


/* Handle notifications */
void control_panel_handle_notify(control_panel_t self, Widget widget)
{
    /* Press the magic OK button */
    action_send(widget, self, NULL);
}
