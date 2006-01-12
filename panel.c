/***********************************************************************

  Copyright (C) 1999-2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: panel.c,v 1.91 2006/01/12 17:52:12 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* abort, atoi, free, malloc */
#endif
#ifdef HAVE_TIME_H
#include <time.h> /* gmtime */
#endif
#if defined(HAVE_SYS_TIME_H) && defined(TM_IN_SYS_TIME)
#include <sys/time.h> /* struct tm */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memset, strcmp */
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h> /* isprint */
#endif
#include <elvin/elvin.h>
#include <Xm/XmAll.h>
#include <Xm/RowColumnP.h>
#include <X11/Xmu/Editres.h>
#include "globals.h"
#include "replace.h"
#include "utf8.h"
#include "panel.h"
#include "History.h"

/* Make sure ELVIN_SHA1_DIGESTLEN is defined */
#if ! defined(ELVIN_VERSION_AT_LEAST)
#define ELVIN_SHA1_DIGESTLEN SHA1DIGESTLEN
#endif

/* The UUIDs we generate will need this many bytes */
#define UUID_SIZE (ELVIN_SHA1_DIGESTLEN * 2 + 1)

/* The maximum size of the message-id pre-digested string */
#define PREDIGEST_ID_SIZE 32

#define BUFFER_SIZE 1024
#define ATTACHMENT_FMT \
    "MIME-Version: 1.0\n" \
    "Content-Type: %s; charset=us-ascii\n" \
    "\n" \
    "%s\n"

#define TO_CODE "UTF-8"

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
#define DEFAULT_TIMEOUT 5

/* The list of mime types */
char *mime_types[] = { "x-elvin/url", "text/uri-list", NULL };

/* The format of the default user field */
#define USER_FMT "%s@%s"

/* The characters to use when converting a hex digit to ASCII */
static char hex_chars[] = "0123456789abcdef";


/* The structure used to hold font code-set information */
typedef struct
{
    /* The widget's font list */
    XmFontList font_list;

    /* The font's code set */
    char *code_set;
} TextWidgetRec;

/* Additional resources for Motif text widgets */
#define offset(field) XtOffsetOf(TextWidgetRec, field)
static XtResource resources[] =
{
    /* XmFontList font_list */
    {
	XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
	offset(font_list), XmRFontList, (XtPointer)NULL
    },

    /* char *code_set */
    {
	XtNfontCodeSet, XtCString, XtRString, sizeof(char *),
	offset(code_set), XtRString, (XtPointer)NULL
    }
};
#undef offset

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

    /* The utf8 encoder for the user widget */
    utf8_encoder_t user_encoder;

    /* The receiver's group menu button */
    Widget group;

    /* The receiver's group menu */
    Widget group_menu;

    /* The number of entries in the group menu */
    int group_count;

    /* The receiver's timeout menu button */
    Widget timeout;

    /* The widgets representing each of the possible timeouts */
    Widget timeouts[XtNumber(timeouts)];

    /* The receiver's mime menu button */
    Widget mime_type;

    /* The widgets representing the possible mime types */
    Widget mime_types[XtNumber(mime_types)];

    /* The receiver's mime args text widget */
    Widget mime_args;

    /* The utf8 encoder for the mime args widget */
    utf8_encoder_t mime_encoder;

    /* The receiver's text text widget */
    Widget text;

    /* The utf8 encoder for the text widget */
    utf8_encoder_t text_encoder;

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

    /* The message whose URL is overriding the current status */
    message_t status_message;

    /* Non-zero if the status message is overriding the current status */
    int is_overridden;

    /* The uuid counter */
    int uuid_count;

    /* The receiver's about box widget */
    Widget about_box;

    /* The currently selected subscription (group) */
    menu_item_tuple_t selection;

    /* The message to which we are replying (NULL if none) */
    char *message_id;

    /* The thread to which we are replying (NULL if none) */
    char *thread_id;

    /* Non-zero indicates that the the control panel should be closed
     * after a notification is sent */
    int close_on_send;

    /* Non-zero indicates that the elvin connection is currently
     * capable of sending a notification. */
    int is_connected;

    /* The last few messages sent by the user */
    message_t *send_history;

    /* The capacity of the send history */
    int send_history_count;

    /* The index of the current item in the send history */
    int send_history_point;

    /* The next item in the send history */
    int send_history_next;
};


/*
 *
 * Static function prototypes
 *
 */
static void file_groups(Widget widget, control_panel_t self, XtPointer unused);
static void file_usenet(Widget widget, control_panel_t self, XtPointer unused);
static void file_keys(Widget widget, control_panel_t self, XtPointer unused);
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
static menu_item_tuple_t get_tuple_from_tag(control_panel_t self, char *tag);
static void set_user(control_panel_t self, char *user);
static char *get_user(control_panel_t self);
static void set_text(control_panel_t self, char *text);
static char *get_text(control_panel_t self);
static void reset_timeout(control_panel_t self);
static int get_timeout(control_panel_t self);
static char *get_mime_type(control_panel_t self);
static void set_mime_args(control_panel_t self, char *args);
static char *get_mime_args(control_panel_t self);

static void create_uuid(control_panel_t self, char *uuid);
static message_t construct_message(control_panel_t self);
static void deconstruct_message(control_panel_t self, message_t message);

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

static void file_keys(Widget widget, control_panel_t self, XtPointer unused)
{
    tickertape_reload_keys(self -> tickertape);
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

    /* Create the `reload keys' menu item */
    item = XtVaCreateManagedWidget("reloadKeys", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)file_keys, (XtPointer)self);

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

    /* HACK: tell the RowColumn widget what its cascade button is.
     * Normally this is set when the Option is clicked on, but we need
     * it to work before that happens, so we force it. */
    ((XmRowColumnWidget)self->group_menu)->row_column.cascadeBtn =
        XmOptionButtonGadget(widget);

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* Creates the popup menu for timeout selection */
static Widget create_timeout_menu(control_panel_t self, Widget parent)
{
    Widget menu, widget;
    Arg args[2];
    int i;

    /* Create a pulldown menu */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, menu);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "timeout", args, 2);

    /* Fill in the recommended timeout values */
    for (i = 0; timeouts[i] != NULL; i++)
    {
	self -> timeouts[i] = XtVaCreateManagedWidget(
	    timeouts[i], xmPushButtonWidgetClass, menu,
	    NULL);
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

/* This is called when the mouse has moved in the history widget */
static void history_motion_callback(
    Widget widget,
    XtPointer closure,
    XtPointer call_data)
{
    control_panel_t self = (control_panel_t)closure;
    message_t message = (message_t)call_data;

    control_panel_set_status_message(self, message);
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
    TextWidgetRec rc;

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

    /* Read the resources for the string's text widget */
    XtGetApplicationResources(
	self -> user, &rc,
	resources, XtNumber(resources),
	NULL, 0);
    self -> user_encoder = utf8_encoder_alloc(XtDisplay(parent), rc.font_list, rc.code_set);

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

/* Set the send button's sensitivity */
static void update_send_button_sensitive(control_panel_t self)
{
    char *string;

    /* Are we connected? */
    if (! self -> is_connected)
    {
	XtSetSensitive(self -> send, False);
	return;
    }

    /* Is there any message text? */
    if ((string = get_text(self)) == NULL) {
        XtSetSensitive(self -> send, False);
        return;
    }

    /* Enable the send button iff there's something to send */
    XtSetSensitive(self -> send, *string == '\0' ? False : True);
    free(string);
}

/* Callback for changes in the text box */
static void text_changed(Widget widget, XtPointer rock, XtPointer unused)
{
    control_panel_t self = (control_panel_t)rock;
    update_send_button_sensitive(self);
}

/* Constructs the Text box */
static void create_text_box(control_panel_t self, Widget parent)
{
    Widget label;
    TextWidgetRec rc;

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

    /* Read the resources for the string's text widget */
    XtGetApplicationResources(
	self -> text, &rc,
	resources, XtNumber(resources),
	NULL, 0);
    self -> text_encoder = utf8_encoder_alloc(XtDisplay(parent), rc.font_list, rc.code_set);

    /* Alert us when the message text changes */
    XtAddCallback(
	self -> text, XmNvalueChangedCallback,
	text_changed, (XtPointer)self);

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
    int i;

    /* Create a pulldown menu */
    pulldown = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, pulldown);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "mime_type", args, 2);

    /* Add each of the supported mime types to it */
    for (i = 0; mime_types[i] != NULL; i++)
    {
	/* Add the  mime type to it */
	self -> mime_types[i] = XtVaCreateManagedWidget(
	    mime_types[i], xmPushButtonWidgetClass, pulldown,
	    NULL);
    }

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* Constructs the Mime box */
static void create_mime_box(control_panel_t self, Widget parent)
{
    TextWidgetRec rc;

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

    /* Read the resources for the string's text widget */
    XtGetApplicationResources(
	self -> mime_args, &rc,
	resources, XtNumber(resources),
	NULL, 0);
    self -> mime_encoder = utf8_encoder_alloc(XtDisplay(parent), rc.font_list, rc.code_set);

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

    /* Record the tuple for later use */
    self -> selection = tuple;
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
    char *raw;

    /* Convert the string into the font's code set */
    raw = utf8_encoder_decode(self -> user_encoder, user);
    XmTextSetString(self -> user, raw);
    free(raw);
}


/* Answers the receiver's user.  Note: this string will need to be
 * released with XtFree() */
static char *get_user(control_panel_t self)
{
    char *raw;
    char *result;

    /* Get the string from the widget */
    raw = XmTextGetString(self -> user);

    /* Encode it to UTF-8 */
    result = utf8_encoder_encode(self -> user_encoder, raw);
    XtFree(raw);

    return result;
}


/* Sets the receiver's text */
static void set_text(control_panel_t self, char *text)
{
    char *raw;

    /* Convert the string to the font's code set */
    raw = utf8_encoder_decode(self -> text_encoder, text);
    XmTextSetString(self -> text, raw);
    free(raw);
}

/* Answers the receiver's text.  Note: this string will need to be
 * released with XtFree() */
static char *get_text(control_panel_t self)
{
    char *raw;
    char *result;

    /* Get the string from the widget */
    raw = XmTextGetString(self -> text);

    /* Encode it in UTF8 */
    result = utf8_encoder_encode(self -> text_encoder, raw);
    XtFree(raw);

    return result;
}


/* Sets the receiver's timeout */
static void set_timeout(control_panel_t self, int value)
{
    int i;

    /* Find the widget for the timeout */
    for (i = 0; timeouts[i] != NULL; i++)
    {
	if (atoi(timeouts[i]) == value)
	{
	    XtCallActionProc(
		self -> timeouts[i],
		"ArmAndActivate", NULL,
		NULL, 0);
	    return;
	} 
    }

    fprintf(stderr, "Timeout %d not found\n", value);
}

/* Sets the receiver's timeout to the default */
static void reset_timeout(control_panel_t self)
{
    set_timeout(self, DEFAULT_TIMEOUT);
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
	result = 60 * atoi(timeout);
	XtFree(timeout);
    }
    else
    {
	result = DEFAULT_TIMEOUT;
    }

    /* Clean up */
    XmStringFree(string);
    return result;
}

/* Sets the receiver's MIME type. */
static void set_mime_type(control_panel_t self, char *string)
{
    int i;

    /* Find the widget */
    for (i = 0; mime_types[i] != NULL; i++)
    {
	if (strcmp(mime_types[i], string) == 0)
	{
	    XtCallActionProc(
		self -> mime_types[i],
		"ArmAndActivate", NULL,
		NULL, 0);
	    return;
	}
    }

    fprintf(stderr, "Mime Type `%s' not found\n", string);
}

/* Sets the receiver's MIME type to the default */
static void reset_mime_type(control_panel_t self)
{
    set_mime_type(self, mime_types[0]);
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
    char *raw;

    /* Convert the string to the font's code set */
    raw = utf8_encoder_decode(self -> mime_encoder, args);
    XmTextSetString(self -> mime_args, raw);
    free(raw);
}

/* Answers the receiver's MIME args.  Note: if non-null, this string
 * will need to be released with XtFree() */
static char *get_mime_args(control_panel_t self)
{
    char *raw;
    char *result;

    /* Get the string from the widget */
    raw = XmTextGetString(self -> mime_args);

    if (*raw == '\0')
    {
	/* Convert empty strings into NULLs */
	result = NULL;
    }
    else
    {
	/* Convert anything else into UTF-8 */
	result = utf8_encoder_encode(self -> mime_encoder, raw);
    }

    XtFree(raw);
    return result;
}

/* Generates a universally unique identifier for a message.  Result
 * should point to a buffer of UUID_SIZE bytes. */
void create_uuid(control_panel_t self, char *result)
{
    char buffer[PREDIGEST_ID_SIZE];
    time_t now;
    struct tm *tm_gmt;
    char digest[ELVIN_SHA1_DIGESTLEN];
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
    snprintf(buffer, PREDIGEST_ID_SIZE,
	     "%04x%02x%02x%02x%02x%02x-%08lx-%04x-%02x",
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
    if (! elvin_sha1_digest(client, buffer, 31, digest, NULL))
    {
	abort();
    }
#else
#error "Unsupported Elvin library version"
#endif

    /* Convert those digits into bytes */
    for (index = 0; index < ELVIN_SHA1_DIGESTLEN; index++)
    {
	int ch = (uchar)digest[index];
	result[index * 2] = hex_chars[ch >> 4];
	result[index * 2 + 1] = hex_chars[ch & 0xF];
    }

    result[ELVIN_SHA1_DIGESTLEN * 2] = '\0';
}


/* Answers a message based on the receiver's current state */
static message_t construct_message(control_panel_t self)
{
    char *user;
    char *text;
    char *mime_type;
    char *mime_args;
    char uuid[UUID_SIZE];
    size_t length = 0;
    char *attachment = NULL;
    message_t message;

    /* Determine our user and text */
    user = get_user(self);
    text = get_text(self);

    /* Determine our MIME args */
    mime_type = get_mime_type(self);
    mime_args = get_mime_args(self);
    create_uuid(self, uuid);

    /* Construct an RFC2045-compliant MIME message */
    if (mime_type != NULL && mime_args != NULL)
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
	NULL, uuid,
	self -> message_id, self -> thread_id);

    /* Free the user */
    if (user != NULL)
    {
	XtFree(user);
    }

    /* Free the text */
    if (text != NULL)
    {
	free(text);
    }

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

    return message;
}


/* Sets the control panel's state from the given message */
static void deconstruct_message(control_panel_t self, message_t message)
{
    menu_item_tuple_t tuple;
    char *mime_type;
    char *attachment;
    size_t length;

    /* Set the user field */
    set_user(self, message_get_user(message));

    /* Set the text field */
    set_text(self, message_get_string(message));

    /* Set the timeout */
    set_timeout(self, message_get_timeout(message));

    /* Select the message to which we're replying */
    HistorySelectId(self -> history, message_get_reply_id(message));

    /* Set the group */
    if ((tuple = get_tuple_from_tag(self, message_get_info(message))) != NULL)
    {
	set_group_selection(self, tuple);
    }

    /* Decode the mime type and mime arg */
    if (message_decode_attachment(message, &mime_type, &attachment) < 0)
    {
	mime_type = NULL;
	attachment = NULL;
	length = 0;
    }

    /* Set the mime type if any */
    if (mime_type == NULL)
    {
	reset_mime_type(self);
    }
    else
    {
	set_mime_type(self, mime_type);
	free(mime_type);
    }

    /* Set the mime args if any */
    if (attachment == NULL)
    {
	set_mime_args(self, "");
    }
    else
    {
	set_mime_args(self, attachment);
	free(attachment);
    }
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
	message_t message;

	/* Clear a spot in the message history */
	if ((message = self -> send_history[self -> send_history_next]) != NULL)
	{
	    message_free(message);
	}

	/* Construct a new message */
	message = construct_message(self);

	/* Record it in the send history */
	self -> send_history[self -> send_history_next] = message;
	self -> send_history_next = (self -> send_history_next + 1) %
	    self -> send_history_count;
	self -> send_history_point = self -> send_history_next;

	/* And send it too */
	self -> selection -> callback(self -> selection -> rock, message);
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
    size_t length;
    char *buffer;

    /* Set the user to be `user@domain' unless domain is the empty
     * string, in which case we just use `user' */
    user = tickertape_user_name(self -> tickertape);
    domain = tickertape_domain_name(self -> tickertape);
    length = strlen(USER_FMT) + strlen(user) + strlen(domain) - 3;

    if (*domain == '\0' || (buffer = (char *)malloc(length)) == NULL)
    {
	set_user(self, user);
    }
    else
    {
	snprintf(buffer, length, USER_FMT, user, domain);
	set_user(self, buffer);
	free(buffer);
    }

    /* Clear the message and mime areas */
    set_text(self, "");
    set_mime_args(self, "");

    /* Clear the selection */
    control_panel_select(self, NULL);

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

/* Allocates and initializes a new control_panel_t */
control_panel_t control_panel_alloc(
    tickertape_t tickertape,
    Widget parent,
    unsigned int send_history_count)
{
    control_panel_t self = (control_panel_t)malloc(sizeof(struct control_panel));

    /* Set the receiver's contents to something sane */
    memset(self, 0, sizeof(struct control_panel));
    self -> tickertape = tickertape;

    /* Allocate the send history buffer */
    self -> send_history_count = MAX(1, send_history_count + 1);
    if ((self -> send_history = (message_t *)calloc(
	     self -> send_history_count, sizeof(message_t))) == NULL)
    {
	perror("calloc() failed");
	exit(1);
    }

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
    /* Free the old status message */
    if (self -> status != NULL)
    {
	free(self -> status);
	self -> status = NULL;
    }

    /* Record the status message */
    if (message != NULL)
    {
	self -> status = strdup(message);
    }

    /* If there's no overriding status message then show this one */
    if (! self -> is_overridden)
    {
	show_status(self, message);
    }
}

/* Displays a message in the status line.  If `message' is NULL then
 * the message will revert to the message set from
 * control_set_status(). */
void control_panel_set_status_message(
    control_panel_t self,
    message_t message)
{
    char *mime_type = NULL;
    char *attachment = NULL;
    int had_attachment = 0;

    /* Bail if we're already displaying that message */
    if (self -> status_message == message)
    {
	return;
    }

    /* Lose our reference to the old status message */
    if (self -> status_message != NULL)
    {
	/* Did it have an attachment? */
	had_attachment = message_has_attachment(self -> status_message);
	message_free(self -> status_message);
	self -> status_message = NULL;
	self -> is_overridden = False;
    }

    /* Record which message we're showing */
    if (message)
    {
	self -> status_message = message_alloc_reference(message);
	message_decode_attachment(message, &mime_type, &attachment);
    }

    /* If there's a MIME type we can display (text/xxx or x-elvin/url)
     * then display it now */
    if (mime_type && attachment)
    {
	/* This is an ugly hack... */
	if (strcmp(mime_type, mime_types[0]) == 0 ||
	    strncmp(mime_type, mime_types[1], 5) == 0)
	{
	    self -> is_overridden = True;
	    show_status(self, attachment);
	    free(mime_type);
	    free(attachment);
	    return;
	}

	free(mime_type);
	free(attachment);
    }

    /* Display the status if the previous message had an attachment */
    if (had_attachment)
    {
	show_status(self, self -> status);
    }
}

/* This is called when the elvin connection status changes */
void control_panel_set_connected(
    control_panel_t self,
    int is_connected)
{
    /* Record our connection status */
    self -> is_connected = is_connected;

    /* Update the send button's sensitivity */
    update_send_button_sensitive(self);
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

/* Kills a message and its descendents in the history */
void control_panel_kill_thread(control_panel_t self, message_t message)
{
    /* Delegate to the History widget */
    HistoryKillThread(self -> history, message);
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
    if (self -> message_id)
    {
        free(self -> message_id);
        self -> message_id = NULL;
    }

    /* Free the old thread id */
    if (self -> thread_id)
    {
        free(self -> thread_id);
        self -> thread_id = NULL;
    }

    /* If a message_t was provided that is in the menu, then select it
     * and set the message_id so that we can do threading */
    if (message != NULL)
    {
	char *id;

	if ((tuple = get_tuple_from_tag(self, message_get_info(message))) != NULL)
	{
	    self -> selection = tuple;
	    set_group_selection(self, tuple);
        }

        /* Copy the message id */
        if ((id = message_get_id(message)) != NULL)
        {
            self -> message_id = strdup(id);
        }

        /* Copy the thread id */
        if ((id = message_get_thread_id(message)) != NULL)
        {
            self -> thread_id = strdup(id);
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

/* Show the previous item the user has sent */
void control_panel_history_prev(control_panel_t self)
{
    message_t message;
    int new_point;

    /* See if there is a previous message to visit */
    new_point = (self -> send_history_count + self -> send_history_point - 1) %
	self -> send_history_count;
    if (new_point == self -> send_history_next || self -> send_history[new_point] == NULL)
    {
	return;
    }

    /* If we're leaving the current item then record what's there so far */
    if (self -> send_history_point == self -> send_history_next)
    {
	/* Free the previous contents */
	if ((message = self -> send_history[self -> send_history_next]) != NULL)
	{
	    message_free(message);
	}

	/* Record the current message in the history */
	self -> send_history[self -> send_history_next] = construct_message(self);
    }

    /* Update the send history's point */
    self -> send_history_point = new_point;
    deconstruct_message(self, self -> send_history[self -> send_history_point]);
}

/* Show the next item the user has sent */
void control_panel_history_next(control_panel_t self)
{
    /* Don't go past the end */
    if (self -> send_history_point == self -> send_history_next)
    {
	return;
    }

    self -> send_history_point = (self -> send_history_point + 1) %
	self -> send_history_count;
    deconstruct_message(self, self -> send_history[self -> send_history_point]);
}
