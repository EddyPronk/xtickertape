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
static const char cvsid[] = "$Id: Control.c,v 1.45 1999/08/22 07:54:49 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <pwd.h>
#include "sanity.h"
#include "history.h"
#include "Control.h"
#include "List.h"

#include <Xm/XmAll.h>
#include <X11/Xmu/Editres.h>

#ifdef SANITY
static char *sanity_value = "ControlPanel";
static char *sanity_freed = "Freed ControlPanel";
#endif /* SANITY */


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

/* The default timeout */
#define DEFAULT_TIMEOUT "5"


/* Used in callbacks to denote both a ControlPanel and a callback */
typedef struct MenuItemTuple_t
{
    ControlPanel controlPanel;
    Widget widget;
    char *title;
    ControlPanelCallback callback;
    void *context;
} *MenuItemTuple;

/* The pieces of the control panel */
struct ControlPanel_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */

    /* The receiver's tickertape */
    tickertape_t tickertape;

    /* The receiver's top-level widget */
    Widget top;

    /* The receiver's user text widget */
    Widget user;

    /* The receiver's group menu button */
    Widget group;

    /* The receiver's group menu */
    Widget groupMenu;

    /* The receiver's timeout menu button */
    Widget timeout;

    /* The receiver's default timeout menu button */
    Widget defaultTimeout;

    /* The receiver's mime menu button */
    Widget mimeType;

    /* The receiver's mime args text widget */
    Widget mimeArgs;

    /* The receiver's text text widget */
    Widget text;

    /* The receiver's "Send" button widget */
    Widget send;

    /* The receiver's history list widget */
    Widget history;

    /* The uuid counter */
    int uuid_count;

    /* The receiver's about box widget */
    Widget aboutBox;

    /* The currently selected subscription (group) */
    MenuItemTuple selection;

    /* The list of subscriptions to appear in the group menu */
    List subscriptions;

    /* The list of timeouts to appear in the timeout menu */
    char **timeouts;

    /* The message to which we are replying (0 if none) */
    char *messageId;
};


/*
 *
 * Static function prototypes
 *
 */
static void FileGroups(Widget widget, ControlPanel self, XtPointer unused);
static void FileUsenet(Widget widget, ControlPanel self, XtPointer unused);
static void FileExit(Widget widget, ControlPanel self, XtPointer unused);
static void HelpAbout(Widget widget, ControlPanel self, XtPointer unused);
static void CreateFileMenu(ControlPanel self, Widget parent);
static void CreateOptionsMenu(ControlPanel self, Widget parent);
static Widget CreateAboutBox(ControlPanel self, Widget parent);
static void CreateHelpMenu(ControlPanel self, Widget parent);
static Widget CreateGroupMenu(ControlPanel self, Widget parent);
static Widget CreateTimeoutMenu(ControlPanel self, Widget parent);
static void CreateTopBox(ControlPanel self, Widget parent);
static void CreateTextBox(ControlPanel self, Widget parent);
static Widget CreateMimeTypeMenu(ControlPanel self, Widget parent);
static void CreateMimeBox(ControlPanel self, Widget parent);
static void CreateBottomBox(ControlPanel self, Widget parent);
static void InitializeUserInterface(ControlPanel self, Widget parent);
static void SetUser(ControlPanel self, char *user);
static char *GetUser(ControlPanel self);
static void SetText(ControlPanel self, char *text);
static char *GetText(ControlPanel self);
static void ResetTimeout(ControlPanel self);
static int GetTimeout(ControlPanel self);
static void SetMimeArgs(ControlPanel self, char *args);
static char *GetMimeArgs(ControlPanel self);
static char *GetMimeType(ControlPanel self);
Message ConstructMessage(ControlPanel self);
static void ActionSend(Widget button, ControlPanel self, XtPointer ignored);
static void ActionClear(Widget button, ControlPanel self, XtPointer ignored);
static void ActionCancel(Widget button, ControlPanel self, XtPointer ignored);
static void ActionReturn(Widget textField, ControlPanel self, XmAnyCallbackStruct *cbs);
static void ActionDismiss(Widget button, ControlPanel self, XtPointer ignored);
static void prepare_reply(ControlPanel self, Message message);


/* This gets called when the user selects the "reloadGroups" menu item */
static void FileGroups(Widget widget, ControlPanel self, XtPointer unused)
{
    tickertape_reload_groups(self -> tickertape);
}

/* This gets called when the user selects the "reloadUsenet" menu item */
static void FileUsenet(Widget widget, ControlPanel self, XtPointer unused)
{
    tickertape_reload_usenet(self -> tickertape);
}


/* This gets called when the user selects the "exit" menu item from the file menu */
static void FileExit(Widget widget, ControlPanel self, XtPointer unused)
{
    tickertape_quit(self -> tickertape);
}

/* Construct the File menu widget */
static void CreateFileMenu(ControlPanel self, Widget parent)
{
    Widget menu;
    Widget item;
    SANITY_CHECK(self);

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the `reload groups' menu item */
    item = XtVaCreateManagedWidget("reloadGroups", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileGroups, (XtPointer)self);

    /* Create the `reload usenet' menu item */
    item = XtVaCreateManagedWidget("reloadUsenet", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileUsenet, (XtPointer)self);

    /* Create a separator */
    XtVaCreateManagedWidget("separator", xmSeparatorGadgetClass, menu, NULL);

    /* Create the "exit" menu item */
    item = XtVaCreateManagedWidget("exit", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileExit, (XtPointer)self);

    /* Create the menu's cascade button */
    XtVaCreateManagedWidget(
	"fileMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);
}

/* This is called when the `threaded' toggle button is selected */
static void OptionsThreaded(
    Widget widget,
    ControlPanel self,
    XmToggleButtonCallbackStruct *info)
{
    printf("threaded (set=%d)!\n", info -> set);
    history_set_threaded(tickertape_history(self -> tickertape), info -> set);
}

/* Creates the `Options' menu */
static void CreateOptionsMenu(ControlPanel self, Widget parent)
{
    Widget menu;
    Widget item;
    SANITY_CHECK(self);

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the `threaded' menu item */
    item = XtVaCreateManagedWidget("threaded", xmToggleButtonGadgetClass, menu, NULL);
    XtAddCallback(
	item, XmNvalueChangedCallback,
	(XtCallbackProc)OptionsThreaded, (XtPointer)self);
    history_set_threaded(tickertape_history(self -> tickertape), XmToggleButtonGetState(item));

    /* Create the menu's cascade button */
    XtVaCreateManagedWidget(
	"optionsMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);
}


/* Creates the About box */
static Widget CreateAboutBox(ControlPanel self, Widget parent)
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
    top = XtVaCreatePopupShell(
	"aboutBox", topLevelShellWidgetClass, parent,
	XmNdeleteResponse, XmDO_NOTHING,
	NULL);

    /* Add a handler for the WM_DELETE_WINDOW protocol */
    wm_delete_window = XmInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(top, wm_delete_window, (XtCallbackProc)ActionDismiss, (XtPointer)self);

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
	(XtCallbackProc)ActionDismiss, (XtPointer)self);

    XtManageChild(buttonForm);

    /* Manage all of the widgets */
    XtManageChild(form);
    return top;
}


/* Pop up an about box */
static void HelpAbout(Widget widget, ControlPanel self, XtPointer unused)
{
    if (self -> aboutBox == NULL)
    {
	self -> aboutBox = CreateAboutBox(self, XtParent(self -> top));
    }

    XtPopup(self -> aboutBox, XtGrabNone);
}

/* Construct the Help menu widget */
static void CreateHelpMenu(ControlPanel self, Widget parent)
{
    Widget menu;
    Widget item;
    Widget cascade;
    SANITY_CHECK(self);

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the "about" menu item */
    item = XtVaCreateManagedWidget("about", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)HelpAbout, (XtPointer)self);

    /* Create the menu's cascade button */
    cascade = XtVaCreateManagedWidget(
	"helpMenu", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);

    /* Tell the menubar that this menu is the help menu */
    XtVaSetValues(parent, XmNmenuHelpWidget, cascade, NULL);
}



/* This function is used to order the elements of the group menu */
static Cardinal OrderGroupMenu(Widget item)
{
    ControlPanel self;
    MenuItemTuple tuple;
    XtVaGetValues(item, XmNuserData, &tuple, NULL);

    /* I don't know what this is */
    if (tuple == NULL)
    {
	return 0;
    }

    self = tuple -> controlPanel;
    SANITY_CHECK(self);

    /* Locate the menu item tuple corresponding to the given item */
    return List_index(self -> subscriptions, tuple);
}

/* Construct the menu for the Group list */
static Widget CreateGroupMenu(ControlPanel self, Widget parent)
{
    Widget widget;
    Arg args[2];
    SANITY_CHECK(self);

    /* Create a pulldown menu */
    XtSetArg(args[0], XmNinsertPosition, OrderGroupMenu);
    self -> groupMenu = XmCreatePulldownMenu(parent, "_pulldown", args, 1);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, self -> groupMenu);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "group", args, 2);

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* Creates the popup menu for timeout selection */
static Widget CreateTimeoutMenu(ControlPanel self, Widget parent)
{
    Widget menu, widget;
    Arg args[2];
    char **pointer;
    SANITY_CHECK(self);

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
	    self -> defaultTimeout = item;
	}
    }

    /* Manage the option button and return it */
    XtManageChild(widget);
    return widget;
}

/* This is called when an item in the history list is selected */
void history_selection_callback(Widget widget, ControlPanel self, XmListCallbackStruct *info)
{
    Message message;

    /* Reconfigure to reply to the selected message (or lack thereof) */
    message = history_get(tickertape_history(self -> tickertape), info -> item_position);
    prepare_reply(self, message);
}

/* Constructs the history list */
static void CreateHistoryBox(ControlPanel self, Widget parent)
{
    Arg args[7];

    XtSetArg(args[0], XmNleftAttachment, XmATTACH_FORM);
    XtSetArg(args[1], XmNrightAttachment, XmATTACH_FORM);
    XtSetArg(args[2], XmNtopAttachment, XmATTACH_FORM);
    XtSetArg(args[3], XmNbottomAttachment, XmATTACH_FORM);
    XtSetArg(args[4], XmNselectionPolicy, XmBROWSE_SELECT);
    XtSetArg(args[5], XmNitemCount, 0);
    XtSetArg(args[6], XmNvisibleItemCount, 3);
    self -> history = XmCreateScrolledList(parent, "history", args, 7);

    /* Add callbacks for interesting things */
    XtAddCallback(
	self -> history, XmNbrowseSelectionCallback,
	(XtCallbackProc)history_selection_callback, (XtPointer)self);

    XtManageChild(self -> history);

    /* Tell the tickertape's history to use this List widget */
    history_set_list(tickertape_history(self -> tickertape), self -> history);
}

/* Constructs the top box of the Control Panel */
static void CreateTopBox(ControlPanel self, Widget parent)
{
    Widget form, label;
    SANITY_CHECK(self);

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
    self -> timeout = CreateTimeoutMenu(self, parent);
    XtVaSetValues(
	self -> timeout,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    /* The "Group" menu button */
    self -> group = CreateGroupMenu(self, parent);
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
static void CreateTextBox(ControlPanel self, Widget parent)
{
    Widget label;
    SANITY_CHECK(self);

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
	(XtCallbackProc)ActionReturn, (XtPointer)self);
}

/* Constructs the Mime type menu */
static Widget CreateMimeTypeMenu(ControlPanel self, Widget parent)
{
    Widget pulldown, widget;
    Arg args[2];
    SANITY_CHECK(self);

    /* Create a pulldown menu */
    pulldown = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create an Option menu button with the above menu */
    XtSetArg(args[0], XmNsubMenuId, pulldown);
    XtSetArg(args[1], XmNtraversalOn, False);
    widget = XmCreateOptionMenu(parent, "mimeType", args, 2);

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
static void CreateMimeBox(ControlPanel self, Widget parent)
{
    SANITY_CHECK(self);

    self -> mimeType = CreateMimeTypeMenu(self, parent);
    XtVaSetValues(
	self -> mimeType,
	XmNleftAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	NULL);

    self -> mimeArgs = XtVaCreateManagedWidget(
	"mimeArgs", xmTextFieldWidgetClass, parent,
	XmNtraversalOn, True,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNrightAttachment, XmATTACH_FORM,
	XmNleftWidget, self -> mimeType,
	NULL);

    /* Add a callback to the text field so that we can send the
     * notification if the user hits Return */
    XtAddCallback(
	self -> mimeArgs, XmNactivateCallback,
	(XtCallbackProc)ActionReturn, (XtPointer)self);
}

/* Creates the bottom box (where the buttons live) */
static void CreateBottomBox(ControlPanel self, Widget parent)
{
    Widget button;
    SANITY_CHECK(self);

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

    /* Have it call ActionSend when it gets pressed */
    XtAddCallback(
	self -> send, XmNactivateCallback,
	(XtCallbackProc)ActionSend, (XtPointer)self);


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

    /* Have it call ActionClear when it gets pressed */
    XtAddCallback(
	button, XmNactivateCallback,
	(XtCallbackProc)ActionClear, (XtPointer)self);


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

    /* Have it call ActionCancel when it gets pressed */
    XtAddCallback(
	button, XmNactivateCallback,
	(XtCallbackProc)ActionCancel, (XtPointer)self);
}

/* Constructs the entire control panel */
static void InitializeUserInterface(ControlPanel self, Widget parent)
{
    Widget form;
    Widget menubar;
    Widget historyForm;
    Widget topForm;
    Widget textForm;
    Widget mimeForm;
    Widget buttonForm;
    Atom wm_delete_window;
    SANITY_CHECK(self);

    /* Set some variables to sane values */
    self -> aboutBox = NULL;

    /* Create a popup shell for the receiver */
    self -> top = XtVaCreatePopupShell(
	"controlPanel", topLevelShellWidgetClass, parent,
	XmNdeleteResponse, XmDO_NOTHING,
	NULL);

    /* Add a handler for the WM_DELETE_WINDOW protocol */
    wm_delete_window = XmInternAtom(XtDisplay(self -> top), "WM_DELETE_WINDOW", False);
    XmAddWMProtocolCallback(self -> top, wm_delete_window, (XtCallbackProc)ActionCancel, (XtPointer)self);

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
    CreateFileMenu(self, menubar);

    /* Create the `options' menu */
    CreateOptionsMenu(self, menubar);

    /* Create the `help' menu and tell Motif that it's the Help menu */
    CreateHelpMenu(self, menubar);

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

    CreateTopBox(self, topForm);
    XtManageChild(topForm);

    /* Create the text box */
    textForm = XtVaCreateWidget(
	"textForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, topForm,
	NULL);
    CreateTextBox(self, textForm);
    XtManageChild(textForm);

    /* Create the mime box */
    mimeForm = XtVaCreateWidget(
	"mimeForm", xmFormWidgetClass, form,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_WIDGET,
	XmNtopWidget, textForm,
	NULL);
    CreateMimeBox(self, mimeForm);
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
    CreateBottomBox(self, buttonForm);
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

    CreateHistoryBox(self, historyForm);
    XtManageChild(historyForm);

    /* Manage the form widget */
    XtManageChild(form);

    /* Make sure that we can use Editres */
    XtAddEventHandler(self -> top, (EventMask)0, True, _XEditResCheckMessages, NULL);
}


/* Sets the Option menu's selection */

/* This gets called when the user selects a group from the UI */
static void SelectGroup(Widget widget, MenuItemTuple tuple, XtPointer ignored)
{
    ControlPanel self = tuple -> controlPanel;
    SANITY_CHECK(self);

    if (self -> selection != tuple)
    {
	self -> selection = tuple;
	XmListDeselectAllItems(self -> history);
    }

    /* Changing the group prevents a reply */
    if (self -> messageId != NULL)
    {
	free(self -> messageId);
	self -> messageId = NULL;
    }
}

/* Programmatically sets the selection in the Group option */
static void SetGroupSelection(ControlPanel self, MenuItemTuple tuple)
{
    SANITY_CHECK(self);

    if (tuple == NULL)
    {
	self -> selection = NULL;
    }
    else
    {
	/* Tell Motif that the menu item was selected, which will call SelectGroup for us */
	XtCallActionProc(tuple -> widget, "ArmAndActivate", NULL, NULL, 0);
    }
}


/* Sets the receiver's user */
static void SetUser(ControlPanel self, char *user)
{
    SANITY_CHECK(self);

    XmTextSetString(self -> user, user);
}


/* Answers the receiver's user */
static char *GetUser(ControlPanel self)
{
    char *user;
    SANITY_CHECK(self);

    XtVaGetValues(self -> user, XmNvalue, &user, NULL);
    return user;
}


/* Sets the receiver's text */
static void SetText(ControlPanel self, char *text)
{
    SANITY_CHECK(self);

    XmTextSetString(self -> text, text);
}

/* Answers the receiver's text */
static char *GetText(ControlPanel self)
{
    char *text;
    SANITY_CHECK(self);

    XtVaGetValues(self -> text, XmNvalue, &text, NULL);
    return text;
}


/* Sets the receiver's timeout */
static void ResetTimeout(ControlPanel self)
{
    SANITY_CHECK(self);

    /* Pretend that the user selected the default timeout item */
    XtCallActionProc(
	self -> defaultTimeout,
	"ArmAndActivate", NULL,
	NULL, 0);
}

/* Answers the receiver's timeout */
static int GetTimeout(ControlPanel self)
{
    XmString string;
    char *timeout;
    int result;
    SANITY_CHECK(self);

    XtVaGetValues(XmOptionButtonGadget(self -> timeout), XmNlabelString, &string, NULL);
    XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &timeout);
    XmStringFree(string);

    result = atoi(timeout);
    XtFree(timeout);
    return result;
}

/* Sets the receiver's MIME args */
static void SetMimeArgs(ControlPanel self, char *args)
{
    SANITY_CHECK(self);

    XmTextSetString(self -> mimeArgs, args);
}

/* Answers the receiver's MIME args */
static char *GetMimeArgs(ControlPanel self)
{
    char *args;
    SANITY_CHECK(self);

    XtVaGetValues(self -> mimeArgs, XmNvalue, &args, NULL);

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
static char *GetMimeType(ControlPanel self)
{
    XmString string;
    char *mimeType;
    SANITY_CHECK(self);

    XtVaGetValues(XmOptionButtonGadget(self -> mimeType), XmNlabelString, &string, NULL);
    XmStringGetLtoR(string, XmFONTLIST_DEFAULT_TAG, &mimeType);
    XmStringFree(string);

    return mimeType;
}


/* Generates a universally unique identifier for a message */
char *GenerateUUID(ControlPanel self)
{
    char *buffer = (char *)malloc(sizeof("YYYYMMDDHHMMSS-11223344-1234-12"));
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

    /* Construct the UUID */
    sprintf(buffer, "%04x%02x%02x%02x%02x%02x-%08lx-%04x-%02x",
	    tm_gmt -> tm_year + 1900, tm_gmt -> tm_mon + 1, tm_gmt -> tm_mday,
	    tm_gmt -> tm_hour, tm_gmt -> tm_min, tm_gmt -> tm_sec,
	    (long)gethostid(),
	    (int)getpid(),
	    self -> uuid_count);

    self -> uuid_count = (self -> uuid_count + 1) % 256;
    return buffer;
}

/* Answers a message based on the receiver's current state */
Message ConstructMessage(ControlPanel self)
{
    char *mimeType;
    char *mimeArgs;
    char *uuid;
    Message message;
    SANITY_CHECK(self);

    /* Determine our MIME args */
    mimeType = GetMimeType(self);
    mimeArgs = GetMimeArgs(self);
    uuid = GenerateUUID(self);

    /* Construct a message */
    message = Message_alloc(
	self -> selection,
	self -> selection -> title,
	GetUser(self),
	GetText(self),
	GetTimeout(self),
	(mimeArgs == NULL) ? NULL : mimeType,
	mimeArgs,
	uuid,
	self -> messageId);

    if (mimeType != NULL)
    {
	XtFree(mimeType);
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
static void ActionSend(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);

    if (self -> selection != NULL)
    {
	Message message = ConstructMessage(self);
	(*self -> selection -> callback)(self -> selection -> context, message);
	Message_free(message);
    }

    XtPopdown(self -> top);
}

/* Callback for `Clear' button */
static void ActionClear(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);

    SetUser(self, tickertape_user_name(self -> tickertape));
    SetText(self, "");
    SetMimeArgs(self, "");
    ResetTimeout(self);
}

/* Callback for `Cancel' button */
static void ActionCancel(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);

    XtPopdown(self -> top);
}


/* Call back for the Return key in the text and mimeArgs fields */
static void ActionReturn(Widget textField, ControlPanel self, XmAnyCallbackStruct *cbs)
{
    SANITY_CHECK(self);

    XtCallActionProc(
	self -> send, "ArmAndActivate", cbs -> event, 
	NULL, 0);
}

/* Callback for the `Dismiss' button on the about box */
static void ActionDismiss(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);

    /* This check is probably a bit paranoid, but it doesn't hurt... */
    if (self -> aboutBox != NULL)
    {
	XtPopdown(self -> aboutBox);
    }
}


/*
 *
 * Exported functions
 *
 */

/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(tickertape_t tickertape, Widget parent)
{
    ControlPanel self = (ControlPanel) malloc(sizeof(struct ControlPanel_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    /* Set the receiver's contents to something sane */
    self -> tickertape = tickertape;
    self -> top = NULL;
    self -> user = NULL;
    self -> group = NULL;
    self -> groupMenu = NULL;
    self -> timeout = NULL;
    self -> defaultTimeout = NULL;
    self -> mimeType = NULL;
    self -> mimeArgs = NULL;
    self -> text = NULL;
    self -> send = NULL;
    self -> history = NULL;
    self -> uuid_count = 0;
    self -> selection = NULL;
    self -> subscriptions = List_alloc();
    self -> timeouts = timeouts;
    self -> messageId = NULL;

    /* Initialize the UI */
    InitializeUserInterface(self, parent);

    /* Tell the history_t about our List */
/*    history_set_list(history, self -> history);*/

    /* Set the various fields to their default values */
    ActionClear(self -> top, self, NULL);

    return self;
}

/* Releases the resources used by the receiver */
void ControlPanel_free(ControlPanel self)
{
    SANITY_CHECK(self);
#ifdef SANITY
    self -> sanity_check = sanity_freed;
#endif /* SANITY */
    free(self);
}



/* Adds a subscription to the receiver at the end of the groups list */
void *ControlPanel_addSubscription(
    ControlPanel self, char *title,
    ControlPanelCallback callback, void *context)
{
    XmString string;
    Widget item;
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    /* Create a tuple to hold callback context */
    if ((tuple = (MenuItemTuple) malloc(sizeof(struct MenuItemTuple_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

    /* Fill in the values of the tuple */
    tuple -> controlPanel = self;
    tuple -> title = strdup(title);
    tuple -> callback = callback;
    tuple -> context = context;

    /* Add the tuple to the receiver's list of subscriptions */
    List_addLast(self -> subscriptions, tuple);

    /* Create a menu item for the subscription.
     * Use the user data field to point to the tuple so that we can
     * find our way back to the ControlPanel when we want to order the 
     * menu items */
    string = XmStringCreateSimple(title);
    item = XtVaCreateManagedWidget(
	title, xmPushButtonWidgetClass, self -> groupMenu,
	XmNlabelString, string,
	XmNuserData, tuple,
	NULL);
    XmStringFree(string);

    /* Add the widget to the tuple */
    tuple -> widget = item;

    /* Add a callback to update the selection when the item is selected */
    XtAddCallback(
	item, XmNactivateCallback,
	(XtCallbackProc)SelectGroup, (XtPointer)tuple);

    /* If this is our first menu item, then select it by default */
    if (self -> selection == NULL)
    {
	SetGroupSelection(self, tuple);
    }

    return tuple;
}


/* Removes a subscription from the receiver (tuple was returned by addSubscription) */
void ControlPanel_removeSubscription(ControlPanel self, void *info)
{
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    /* Bail out if it's not in the List of subscriptions */
    if ((tuple = (MenuItemTuple)List_remove(self -> subscriptions, info)) == NULL)
    {
	return;
    }

    /* Destroy it's widget so it isn't in the menu anymore */
    XtDestroyWidget(tuple -> widget);

    /* If it's the selected item, then change the selection to something else */
    if (self -> selection == tuple)
    {
	MenuItemTuple selection = List_first(self -> subscriptions);
	SetGroupSelection(self, selection);
    }

    /* Free up some memory */
    free(tuple -> title);
    free(tuple);
}

/* Changes the location of the subscription within the ControlPanel */
void ControlPanel_setSubscriptionIndex(ControlPanel self, void *info, int index)
{
    MenuItemTuple tuple;
    XmString string;
    SANITY_CHECK(self);

    /* Bail out if the tuple isn't here */
    if ((tuple = (MenuItemTuple)List_remove(self -> subscriptions, info)) == NULL)
    {
	return;
    }

    /* Remove the item from the menu... */
    XtDestroyWidget(tuple -> widget);

    /* Insert the tuple in the appropriate location */
    List_insert(self -> subscriptions, index, info);

    /* ... and put it back in the proper locations */
    string = XmStringCreateSimple(tuple -> title);
    tuple -> widget = XtVaCreateManagedWidget(
	tuple -> title, xmPushButtonWidgetClass, self -> groupMenu,
	XmNlabelString, string,
	XmNuserData, tuple,
	NULL);
    XmStringFree(string);

    /* Add a callback to update the selection when the item is specified */
    XtAddCallback(
	tuple -> widget, XmNactivateCallback,
	(XtCallbackProc)SelectGroup, (XtPointer)tuple);

    /* If it was the selection, then make sure it is still selected in the menu */
    if (self -> selection == tuple)
    {
	SetGroupSelection(self, tuple);
    }
}

/* Retitles an entry */
void ControlPanel_retitleSubscription(ControlPanel self, void *info, char *title)
{
    MenuItemTuple tuple;
    XmString string;
    SANITY_CHECK(self);

    /* Update the tuple's label */
    tuple = (MenuItemTuple)info;
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
	SetGroupSelection(self, tuple);
    }
}

/* Sets up the receiver to reply to the given message */
static void prepare_reply(ControlPanel self, Message message)
{
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    /* Free the old message id */
    free(self -> messageId);
    self -> messageId = NULL;

    /* If a Message was provided that is in the menu, then select it
     * and set the messageId so that we can do threading */
    if (message != NULL)
    {
	char *id;

	if ((tuple = (MenuItemTuple) Message_getInfo(message)) != NULL)
	{
	    self -> selection = tuple;
	    SetGroupSelection(self, tuple);
	}

	id = Message_getId(message);
	if (id != NULL)
	{
	    self -> messageId = strdup(id);
	}
    }
}

/* Ensures that the given list item is visible */
static void make_index_visible(Widget list, int index)
{
    int top;
    int count;

    /* Figure out what we're looking at */
    XtVaGetValues(list,
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

/* Makes the ControlPanel window visible */
void ControlPanel_select(ControlPanel self, Message message)
{
    int index;

    /* Set up to reply */
    prepare_reply(self, message);

    /* Select the appropriate item in the history */
    if ((index = history_index(tickertape_history(self -> tickertape), message)) < 0)
    {
	XmListDeselectAllItems(self -> history);
	return;
    }

    /* Select the item and make sure it is visible */
    make_index_visible(self -> history, index);
    XmListSelectPos(self -> history, index, False);
}

/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self)
{
    SANITY_CHECK(self);

    XtPopup(self -> top, XtGrabNone);
}


/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget)
{
    SANITY_CHECK(self);

    /* Press the magic OK button */
    ActionSend(widget, self, NULL);
}
