/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2002.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
static const char cvsid[] = "$Id: panel.c,v 1.60 2002/04/09 17:20:58 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <time.h>
#include <pwd.h>
#include <elvin/elvin.h>
#include "panel.h"

#include <Xm/XmAll.h>
#include <X11/Xmu/Editres.h>
#include "History.h"

/* Make sure ELVIN_SHA1DIGESTLEN is defined */
#if ! defined(ELVIN_VERSION_AT_LEAST)
#define ELVIN_SHA1DIGESTLEN SHA1DIGESTLEN
#endif

#define BUFFER_SIZE 1024
#define ATTACHMENT_FMT \
    "MIME-Version: 1.0\n" \
    "Content-Type: %s; charset=us-ascii\n" \
    "\n" \
    "%s\n"


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
char *timeouts[] = { "1", "5", "10", "30", "60", NULL };

/* The default message timeout */
#define DEFAULT_TIMEOUT "5"

/* The format of the default user field */
#define USER_FMT "%s@%s"

/* The characters to use when converting a hex digit to ASCII */
static char hex_chars[] = "0123456789abcdef";

/* Used in callbacks to indicate both a control_panel_t and a callback */
typedef struct menu_item_tuple *menu_item_tuple_t;
struct menu_item_tuple
{
    /* The tuple's control panel */
    control_panel_t control_panel;

    /* The tuple's push-button widget */
    Widget widget;

    /* The tuple's symbolic tag */
    char *tag;

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

    /* The receiver's history form */
    Widget history_form;

    /* Non-zero if the history widget is threaded */
    Boolean is_threaded;

    /* Non-zero if the history widget is displaying timestamps */
    Boolean show_timestamps;

    /* The receiver's history list widget */
    Widget history;

    /* The status line widget */
    Widget status_line;

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

    /* The current status message string */
    char *status;

    /* The overriding status message string */
    char *status_override;

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

    /* Non-zero indicates that the the control panel should be closed
     * after a notification is sent */
    int close_on_send;
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

static void options_threaded(Widget widget, XtPointer rock, XtPointer data);
static void options_show_time(Widget widget, XtPointer rock, XtPointer data);
static void options_close_policy(Widget widget, XtPointer rock, XtPointer data);
static void create_options_menu(control_panel_t self, Widget parent);

static void configure_about_box(Widget shell, XtPointer rock, XConfigureEvent *event);
static Widget create_about_box(control_panel_t self, Widget parent);

static Cardinal order_group_menu(Widget item);
static Widget create_group_menu(control_panel_t self, Widget parent);
static Widget create_timeout_menu(control_panel_t self, Widget parent);

static void history_selection_callback(Widget widget, XtPointer closure, XtPointer call_data);
static void history_attachment_callback(Widget widget, XtPointer closure, XtPointer call_data);
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
static char *get_mime_type(control_panel_t self);
static char *get_mime_args(control_panel_t self);

static char *create_uuid(control_panel_t self);
static message_t construct_message(control_panel_t self);

static void action_send(Widget button, control_panel_t self, XtPointer ignored);
static void action_clear(Widget button, control_panel_t self, XtPointer ignored);
static void action_cancel(Widget button, control_panel_t self, XtPointer ignored);
static void action_return(Widget textField, control_panel_t self, XmAnyCallbackStruct *cbs);
static void action_dismiss(Widget button, control_panel_t self, XtPointer ignored);

static void prepare_reply(control_panel_t self, message_t message);

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

/* This is called when the `threaded' toggle button is changed */
static void options_threaded(Widget widget, XtPointer rock, XtPointer data)
{
    control_panel_t self = (control_panel_t)rock;
    XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *)data;

    HistorySetThreaded(self -> history, info -> set);
}

/* This is called when the `show time' toggle button is changed */
static void options_show_time(Widget widget, XtPointer rock, XtPointer data)
{
    control_panel_t self = (control_panel_t)rock;
    XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *)data;

    HistorySetShowTimestamps(self -> history, info -> set);
}

/* This is called when the `close policy' toggle button is changed */
static void options_close_policy(Widget widget, XtPointer rock, XtPointer data)
{
    control_panel_t self = (control_panel_t)rock;
    XmToggleButtonCallbackStruct *info = (XmToggleButtonCallbackStruct *)data;

    self -> close_on_send = info -> set;
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
    XtAddCallback(item, XmNvalueChangedCallback, options_threaded, (XtPointer)self);
    self -> is_threaded = XmToggleButtonGetState(item);

    /* Create the `show time' menu item */
    item = XtVaCreateManagedWidget("showTime", xmToggleButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNvalueChangedCallback, options_show_time, (XtPointer)self);
    self -> show_timestamps = XmToggleButtonGetState(item);

    /* Create a `close policy' menu item */
    item = XtVaCreateManagedWidget("closePolicy", xmToggleButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNvalueChangedCallback, options_close_policy, (XtPointer)self);
    self -> close_on_send = XmToggleButtonGetState(item);

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
    Widget info_form;
    Widget title;
    Widget copyright;
    Widget button_form;
    Widget button;
    Atom wm_delete_window;
    XmString string;

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
    info_form = XtVaCreateWidget(
	"infoForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);

    /* Create the title label */
    string = XmStringCreateSimple(PACKAGE " " VERSION);
    title = XtVaCreateManagedWidget(
	"titleLabel", xmLabelWidgetClass, info_form,
	XmNlabelString, string,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	NULL);
    XmStringFree(string);

    /* Create the copyright label */
    copyright = XtVaCreateManagedWidget(
	"copyrightLabel", xmLabelWidgetClass, info_form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, title,
	NULL);

    /* Create the author label */
    XtVaCreateManagedWidget(
	"authorLabel", xmLabelWidgetClass, info_form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, copyright,
	NULL);

    XtManageChild(info_form);

    /* Create a Form widget to manage the button */
    button_form = XtVaCreateWidget(
	"buttonForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopWidget, info_form,
	NULL);

    /* Create the dismiss button */
    button = XtVaCreateManagedWidget(
	"dismiss", xmPushButtonWidgetClass, button_form,
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

    XtManageChild(button_form);

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
static void history_selection_callback(
    Widget widget,
    XtPointer closure,
    XtPointer call_data)
{
    control_panel_t self = (control_panel_t)closure;
    message_t message = (message_t)call_data;
    
    /* Reconfigure to reply to the selected message (or lack thereof) */
    prepare_reply(self, message);
}

/* This is called when an item in the history is double-clicked */
static void history_attachment_callback(
    Widget widget,
    XtPointer closure,
    XtPointer call_data)
{
    control_panel_t self = (control_panel_t)closure;
    message_t message = (message_t)call_data;

    /* Display the attachment */
    tickertape_show_attachment(self -> tickertape, message);
}

/* Copies from the attachment into the buffer */
static void decode_attachment(
    char *attachment, size_t length,
    char *buffer, size_t buflen)
{
    char *end = attachment + length;
    char *point;
    char *out = buffer;
    int state = 0;
    int ch;

    /* Find the start of the body */
    for (point = attachment; point < end; point++)
    {
	ch = *point;

	/* Decide how to treat the character depending on our state */
	switch (state)
	{
	    /* Skipping the MIME header */
	    case 0:
	    {
		if (ch == '\n')
		{
		    state = 1;
		    break;
		}

		break;
	    }

	    /* Looking at the ch after a linefeed in the header */
	    case 1:
	    {
		if (ch == '\n')
		{
		    state = 2;
		    break;
		}

		state = 0;
		break;
	    }

	    /* Looking at a character in the body */
	    case 2:
	    {
		if (isprint(ch))
		{
		    if (out < buffer + buflen)
		    {
			*(out++) = ch;
		    }
		    else
		    {
			*(buffer + buflen - 1) = '\0';
			return;
		    }
		}

		break;
	    }

	    default:
	    {
		abort();
	    }
	}
    }

    *out = '\0';
}

/* This is called when the mouse has moved in the history widget */
static void history_motion_callback(
    Widget widget,
    XtPointer closure,
    XtPointer call_data)
{
    control_panel_t self = (control_panel_t)closure;
    message_t message = (message_t)call_data;
    char buffer[BUFFER_SIZE];
    char *string = NULL;
    size_t length;

    /* If the message has an URL then show it in the status bar */
    if (message && message_has_attachment(message))
    {
	/* Get the attachment */
	length = message_get_attachment(message, &string);

	/* Decode it */
	decode_attachment(string, length, buffer, BUFFER_SIZE);
	string = buffer;
    }

    control_panel_show_status(self, string);
}


/* Constructs the history list */
static void create_history_box(control_panel_t self, Widget parent)
{
    Widget scroll_window;

    /* Create a scrolled window to enclose History widget */
    scroll_window = XtVaCreateWidget(
	"historySW", xmScrolledWindowWidgetClass, parent,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_WIDGET,
	XmNbottomWidget, XtParent(self -> status_line),
	XmNscrollingPolicy, XmAPPLICATION_DEFINED,
	XmNvisualPolicy, XmVARIABLE,
	XmNscrollBarDisplayPolicy, XmSTATIC,
	XmNshadowThickness, 2,
	NULL);

    /* Create the History widget itself */
    self -> history = XtVaCreateManagedWidget(
	"history", historyWidgetClass, scroll_window,
	NULL);

    /* Indicate whether or not to show timestamps and do threading */
    HistorySetThreaded(self -> history, self -> is_threaded);
    HistorySetShowTimestamps(self -> history, self -> show_timestamps);

    /* Tell the scroller what its work window is */
    XtVaSetValues(scroll_window, XmNworkWindow, self -> history, NULL);
    XtManageChild(scroll_window);

    /* Add a callback for selection changes */
    XtAddCallback(
	self -> history, XtNcallback,
	history_selection_callback, (XtPointer)self);

    /* Add a callback for showing attachments */
    XtAddCallback(
	self -> history, XtNattachmentCallback,
	history_attachment_callback, (XtPointer)self);

    /* Add a callback for showing message attachments */
    XtAddCallback(
	self -> history, XtNmotionCallback,
	history_motion_callback, (XtPointer)self);
}

/* Constructs the status line */
static void create_status_line(control_panel_t self, Widget parent)
{
    XmString string;
    Widget frame;

    /* Create a frame for the status line */
    frame = XtVaCreateWidget(
	"statusFrame", xmFrameWidgetClass, parent,
	XmNshadowType, XmSHADOW_IN,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* Create an empty string for the status line */
    string = XmStringCreateSimple(PACKAGE " version " VERSION);
    self -> status_line = XtVaCreateManagedWidget(
	"statusLabel", xmLabelWidgetClass, frame,
	XmNalignment, XmALIGNMENT_BEGINNING,
	XmNlabelString, string,
	NULL);
    XmStringFree(string);

    /* Manage the frame now that its child has been created */
    XtManageChild(frame);
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
	XtNsensitive, False,
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
	self -> history_form,
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
    Widget top_form;
    Widget text_form;
    Widget mime_form;
    Widget button_form;
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
    top_form = XtVaCreateWidget(
	"topForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, menubar,
	NULL);

    create_top_box(self, top_form);
    XtManageChild(top_form);

    /* Create the text box */
    text_form = XtVaCreateWidget(
	"textForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, top_form,
	NULL);
    create_text_box(self, text_form);
    XtManageChild(text_form);

    /* Create the mime box */
    mime_form = XtVaCreateWidget(
	"mimeForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, text_form,
	NULL);
    create_mime_box(self, mime_form);
    XtManageChild(mime_form);

    /* Create the buttons across the bottom */
    button_form = XtVaCreateWidget(
	"buttonForm", xmFormWidgetClass, form,
	XmNfractionBase, 3,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, mime_form,
	NULL);
    create_bottom_box(self, button_form);
    XtManageChild(button_form);

    /* Create the history list */
    self -> history_form = XtVaCreateWidget(
	"historyForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopWidget, button_form,
	NULL);

    /* Create the status line */
    create_status_line(self, self -> history_form);

    /* And the history box */
    create_history_box(self, self -> history_form);

    XtManageChild(self -> history_form);

    /* Manage the form widget */
    XtManageChild(form);

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
	HistorySelect(self -> history, NULL);
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


/* Answers the receiver's user.  Note: this string will need to be
 * released with XtFree() */
static char *get_user(control_panel_t self)
{
    return XmTextGetString(self -> user);
}


/* Sets the receiver's text */
static void set_text(control_panel_t self, char *text)
{
    XmTextSetString(self -> text, text);
}

/* Answers the receiver's text.  Note: this string will need to be
 * released with XtFree() */
static char *get_text(control_panel_t self)
{
    return XmTextGetString(self -> text);
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
    if (XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &timeout))
    {
	result = atoi(timeout);
	XtFree(timeout);
    }
    else
    {
	result = atoi(DEFAULT_TIMEOUT);
    }

    /* Clean up */
    XmStringFree(string);
    return result;
}

/* Answers the receiver's MIME type.  Note: this string will need to
 * be released with XtFree() */
static char *get_mime_type(control_panel_t self)
{
    XmString string;
    char *mime_type;

    XtVaGetValues(XmOptionButtonGadget(self -> mime_type), XmNlabelString, &string, NULL);
    if (XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &mime_type))
    {
	XmStringFree(string);
	return mime_type;
    }

    return NULL;
}

/* Sets the receiver's MIME args */
static void set_mime_args(control_panel_t self, char *args)
{
    XmTextSetString(self -> mime_args, args);
}

/* Answers the receiver's MIME args.  Note: if non-null, this string
 * will need to be released with XtFree() */
static char *get_mime_args(control_panel_t self)
{
    char *args = XmTextGetString(self -> mime_args);

    if (*args == '\0')
    {
	XtFree(args);
	return NULL;
    }

    return args;
}

/* Generates a universally unique identifier for a message */
char *create_uuid(control_panel_t self)
{
    char buffer[32];
    time_t now;
    struct tm *tm_gmt;
    char digest[ELVIN_SHA1DIGESTLEN];
    char result[ELVIN_SHA1DIGESTLEN * 2 + 1];
    int index;

    /* Get the time */
    if (time(&now) == (time_t)-1)
    {
	perror("unable to determine the current time");
	exit(1);
    }

    /* Figure out what that means in GMT */
    tm_gmt = gmtime(&now);

    /* Construct the UUID */
    sprintf(buffer, "%04x%02x%02x%02x%02x%02x-%08lx-%04x-%02x",
	    tm_gmt -> tm_year + 1900, tm_gmt -> tm_mon + 1, tm_gmt -> tm_mday,
	    tm_gmt -> tm_hour, tm_gmt -> tm_min, tm_gmt -> tm_sec,
	    (long)gethostid(), (int)getpid(), self -> uuid_count);

    /* Increment our little counter */
    self -> uuid_count = (self -> uuid_count + 1) % 256;

    /* Construct a SHA1 digest of the UUID, which should be just as
     * unique but should make it much harder to track down the sender */
#if ! defined(ELVIN_VERSION_AT_LEAST)
    if (! elvin_sha1digest(buffer, 31, digest))
    {
	abort();
    }
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
    if (! elvin_sha1digest(buffer, 31, digest, NULL))
    {
	return NULL;
    }
#endif

    /* Convert those digits into bytes */
    for (index = 0; index < ELVIN_SHA1DIGESTLEN; index++)
    {
	int ch = (uchar)digest[index];
	result[index * 2] = hex_chars[ch >> 4];
	result[index * 2 + 1] = hex_chars[ch & 0xF];
    }
    result[ELVIN_SHA1DIGESTLEN * 2] = '\0';

    return strdup(result);
}


/* Answers a message based on the receiver's current state */
static message_t construct_message(control_panel_t self)
{
    char *user;
    char *text;
    char *mime_type;
    char *mime_args;
    char *uuid;
    size_t length = 0;
    char *attachment = NULL;
    message_t message;

    /* Determine our user and text */
    user = get_user(self);
    text = get_text(self);

    /* Determine our MIME args */
    mime_type = get_mime_type(self);
    mime_args = get_mime_args(self);
    uuid = create_uuid(self);

    /* Construct an RFC2045-compliant MIME message */
    if (mime_type != NULL || mime_args != NULL)
    {
	/* Measure how long the attachment will need to be */
	length = sizeof(ATTACHMENT_FMT) - 5 + strlen(mime_type) + strlen(mime_args);
	if ((attachment = malloc(length + 1)) == NULL)
	{
	    length = 0;
	}
	else
	{
	    snprintf(attachment, length + 1, ATTACHMENT_FMT, mime_type, mime_args);
	}
    }

    /* Construct a message */
    message = message_alloc(
	self -> selection -> tag,
	self -> selection -> title, user, text, get_timeout(self),
	attachment, length,
	NULL, uuid, self -> message_id);

    /* Clean up */
    if (mime_args != NULL)
    {
	XtFree(mime_args);
    }

    if (mime_type != NULL)
    {
	XtFree(mime_type);
    }

    if (attachment)
    {
	free(attachment);
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
    /* Make sure a group is selected */
    if (self -> selection != NULL)
    {
	message_t message = construct_message(self);
	self -> selection -> callback(self -> selection -> rock, message);
	message_free(message);
    }

    /* Clear the message field */
    set_text(self, "");
    set_mime_args(self, "");

    /* Close the box if appropriate */
    if (self -> close_on_send)
    {
	XtPopdown(self -> top);
    }

    /* Shift focus back to the text field */
    XmProcessTraversal(self -> text, XmTRAVERSE_CURRENT);
}

/* Callback for `Clear' button */
static void action_clear(Widget button, control_panel_t self, XtPointer ignored)
{
    char *user;
    char *domain;
    char *buffer;

    /* Set the user to be `user@domain' unless domain is the empty
     * string, in which case we just use `user' */
    user = tickertape_user_name(self -> tickertape);
    domain = tickertape_domain_name(self -> tickertape);
    if (*domain == '\0' ||
	(buffer = (char *)malloc(sizeof(USER_FMT) + strlen(user) + strlen(domain) - 4)) == NULL)
    {
	set_user(self, user);
    }
    else
    {
	sprintf(buffer, USER_FMT, user, domain);
	set_user(self, buffer);
	free(buffer);
    }

    /* Clear the message and mime areas */
    set_text(self, "");
    set_mime_args(self, "");

    /* Reset the timeout */
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
    /* If the `Send' button is enabled then tickle it */
    if (XtIsSensitive(self -> send))
    {
	XtCallActionProc(
	    self -> send, "ArmAndActivate", cbs -> event, 
	    NULL, 0);
    }
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
    memset(self, 0, sizeof(struct control_panel));
    self -> tickertape = tickertape;
    self -> timeouts = timeouts;

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


/* Show a status message in the status bar */
static void show_status(control_panel_t self, char *message)
{
    XmString string;

    /* Create an XmString version of the string */
    string = XmStringCreateSimple(message ? message : " ");
    XtVaSetValues(self -> status_line, XmNlabelString, string, NULL);
    XmStringFree(string);
}

/* Sets the control panel status message */
void control_panel_set_status(
    control_panel_t self,
    char *message)
{
    /* Record the status message */
    if (message != NULL)
    {
	self -> status = strdup(message);
    }

    /* If there's no overriding status message then show this one */
    if (self -> status_override == NULL)
    {
	show_status(self, message);
    }
}

/* Displays a message in the status line.  If `message' is NULL then
 * the message will revert to the message set from
 * control_set_status(). */
void control_panel_show_status(
    control_panel_t self,
    char *message)
{
    /* Bail if we're already displaying that message */
    if (self -> status_override == message)
    {
	return;
    }

    /* Record the status */
    self -> status_override = message;

    /* Display the `official' status if this one is NULL */
    if (self -> status_override == NULL)
    {
	show_status(self, self -> status);
    }
    else
    {
	show_status(self, message);
    }
}

/* This is called when the elvin connection status changes */
void control_panel_set_connected(
    control_panel_t self,
    int is_connected)
{
    /* Enable/disable the `Send' button as appropriate */
    XtSetSensitive(self -> send, is_connected);
}

/* Adds a subscription to the receiver at the end of the groups list */
void *control_panel_add_subscription(
    control_panel_t self, char *tag, char *title,
    control_panel_callback_t callback, void *rock)
{
    XmString string;
    menu_item_tuple_t tuple;

    /* Create a tuple to hold callback information */
    if ((tuple = (menu_item_tuple_t)malloc(sizeof(struct menu_item_tuple))) == NULL)
    {
	fprintf(stderr, PACKAGE ": out of memory\n");
	exit(1);
    }

    /* Fill in the values of the tuple */
    tuple -> control_panel = self;
    tuple -> tag = strdup(tag);
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
    Cardinal num_children;
    Widget *children;
    Widget *child;

    /* Make sure we haven't already remove it */
    if (tuple -> widget == NULL)
    {
	return;
    }

    /* Remove the widget's client data since Motif won't really
     * destroy the widget until much later */
    XtVaSetValues(tuple -> widget, XmNuserData, NULL, NULL);

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
	if (child_tuple != NULL)
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

    /* Clean up */
    free(tuple -> title);
    free(tuple);
}

/* Adds a message to the control panel's history */
void control_panel_add_message(control_panel_t self, message_t message)
{
    /* Add the message to the history */
    HistoryAddMessage(self -> history, message);
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
	if (child_tuple != NULL)
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

/* Locates the tuple with the given tag */
static menu_item_tuple_t get_tuple_from_tag(control_panel_t self, char *tag)
{
    Cardinal num_children;
    Widget *children;
    Widget *child;

    /* Sanity check */
    if (tag == NULL)
    {
	return NULL;
    }

    /* Get the list of children */
    XtVaGetValues(
	self -> group_menu,
	XmNnumChildren, &num_children,
	XmNchildren, &children,
	NULL);

    /* Find the matching one */
    for (child = children; child < children + num_children; child++)
    {
	menu_item_tuple_t tuple;

	XtVaGetValues(*child, XmNuserData, &tuple, NULL);
	if (tuple != NULL && strcmp(tuple -> tag, tag) == 0)
	{
	    return tuple;
	}
    }

    return NULL;
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

	if ((tuple = get_tuple_from_tag(self, message_get_info(message))) != NULL)
	{
	    self -> selection = tuple;
	    set_group_selection(self, tuple);

	    id = message_get_id(message);
	    if (id != NULL)
	    {
		self -> message_id = strdup(id);
	    }
	}
    }
}

/* Makes the control panel window visible */
void control_panel_select(control_panel_t self, message_t message)
{
    /* Set up to reply */
    prepare_reply(self, message);

    /* Select the item in the history */
    HistorySelect(self -> history, message);
}

/* Makes the control panel window visible */
void control_panel_show(control_panel_t self)
{
    /* Pop up the control panel */
    XtPopup(self -> top, XtGrabNone);

    /* Even if it's iconified */
    XMapWindow(XtDisplay(self -> top), XtWindow(self -> top));
}


/* Handle notifications */
void control_panel_handle_notify(control_panel_t self, Widget widget)
{
    /* Press the magic OK button */
    action_send(widget, self, NULL);
}
