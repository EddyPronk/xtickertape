/* $Id: Control.c,v 1.27 1998/12/18 07:56:44 phelps Exp $ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include "sanity.h"
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

    /* The currently selected subscription (group) */
    MenuItemTuple selection;

    /* The list of subscriptions to appear in the group menu */
    List subscriptions;

    /* The list of timeouts to appear in the timeout menu */
    char **timeouts;

    /* The default user name */
    char *username;

    /* The message to which we are replying (0 if none) */
    unsigned long messageId;

    /* The receiver's groups reload callback */
    ReloadCallback groupsCallback;

    /* The receiver's groups reload callback context */
    void *groupsContext;

    /* The receiver's usenet reload callback */
    ReloadCallback usenetCallback;

    /* The receiver's usenet reload callback context */
    void *usenetContext;
};


/*
 *
 * Static function prototypes
 *
 */
static void FileGroups(Widget widget, ControlPanel self, XtPointer unused);
static void FileUsenet(Widget widget, ControlPanel self, XtPointer unused);
static void FileExit(Widget widget, ControlPanel self, XtPointer unused);
static void CreateFileMenu(ControlPanel self, Widget parent);
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


/* This gets called when the user selects the "reloadGroups" menu item */
static void FileGroups(Widget widget, ControlPanel self, XtPointer unused)
{
    if (self -> groupsCallback != NULL)
    {
	(*self -> groupsCallback)(self -> groupsContext);
    }
}

/* This gets called when the user selects the "reloadUsenet" menu item */
static void FileUsenet(Widget widget, ControlPanel self, XtPointer unused)
{
    if (self -> usenetCallback != NULL)
    {
	(*self -> usenetCallback)(self -> usenetContext);
    }
}


/* This gets called when the user selects the "exit" menu item from the file menu */
static void FileExit(Widget widget, ControlPanel self, XtPointer unused)
{
    /* Simply exit */
    exit(0);
}

/* Construct the File menu widget */
static void CreateFileMenu(ControlPanel self, Widget parent)
{
    Widget menu;
    Widget item;
    Widget cascade;
    SANITY_CHECK(self);

    /* Create the menu itself */
    menu = XmCreatePulldownMenu(parent, "_pulldown", NULL, 0);

    /* Create the "reload groups" menu item */
    item = XtVaCreateManagedWidget("reloadGroups", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileGroups, (XtPointer)self);

    /* Create the "reload usenet" menu item */
    item = XtVaCreateManagedWidget("reloadUsenet", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileUsenet, (XtPointer)self);

    /* Create a separator */
    XtVaCreateManagedWidget("separator", xmSeparatorGadgetClass, menu, NULL);

    /* Create the "exit" menu item */
    item = XtVaCreateManagedWidget("exit", xmPushButtonGadgetClass, menu, NULL);
    XtAddCallback(item, XmNactivateCallback, (XtCallbackProc)FileExit, (XtPointer)self);

    /* Create the menu's cascade button */
    cascade = XtVaCreateManagedWidget(
	"file", xmCascadeButtonWidgetClass, parent,
	XmNsubMenuId, menu,
	NULL);
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

    /* The "Group" menu button */
    self -> group = CreateGroupMenu(self, parent);
    XtVaSetValues(
	self -> group,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftWidget, form,
	NULL);

    /* The "Timeout" label */
    self -> timeout = CreateTimeoutMenu(self, parent);
    XtVaSetValues(
	self -> timeout,
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNrightAttachment, XmATTACH_FORM,
	XmNtopAttachment, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftWidget, self -> group,
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

    /* Add a single child to it */
    XtVaCreateManagedWidget(
	"x-elvin/url", xmPushButtonGadgetClass, pulldown,
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
    Widget topForm;
    Widget textForm;
    Widget mimeForm;
    Widget buttonForm;
    SANITY_CHECK(self);

    /* Create a popup shell for the receiver */
    self -> top = XtVaCreatePopupShell(
	"controlPanel", topLevelShellWidgetClass, parent,
	XmNtitle, "Tickertape Control Panel",
	XmNdeleteResponse, XmUNMAP,
	NULL);

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

    /* Create the file menu */
    CreateFileMenu(self, menubar);

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
	XmNbottomAttachment, XmATTACH_FORM,
	XmNtopWidget, mimeForm,
	NULL);
    CreateBottomBox(self, buttonForm);
    XtManageChild(buttonForm);

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

    self -> selection = tuple;
    self -> messageId = 0;
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
	XtCallActionProc(
	    tuple -> widget,
	    "ArmAndActivate", NULL,
	    NULL, 0);
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


/* Answers a message based on the receiver's current state */
Message ConstructMessage(ControlPanel self)
{
    char *mimeType;
    char *mimeArgs;
    Message message;
    SANITY_CHECK(self);

    /* Determine our MIME args */
    mimeType = GetMimeType(self);
    mimeArgs = GetMimeArgs(self);

    /* Construct a message */
    message = Message_alloc(
	self -> selection,
	self -> selection -> title,
	GetUser(self),
	GetText(self),
	GetTimeout(self),
	(mimeArgs == NULL) ? NULL : mimeType,
	mimeArgs,
	random(),
	self -> messageId);

    if (mimeType != NULL)
    {
	XtFree(mimeType);
    }

    return message;
}

/*
 *
 * Actions
 *
 */

/* Callback for Send button */
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

/* Callback for Clear button */
static void ActionClear(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);

    SetUser(self, self -> username);
    SetText(self, "");
    SetMimeArgs(self, "");
    ResetTimeout(self);
}

/* Callback for Cancel button */
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


/*
 *
 * Exported functions
 *
 */

/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(
    Widget parent, char *user,
    ReloadCallback groupsCallback, void *groupsContext,
    ReloadCallback usenetCallback, void *usenetContext)
{
    ControlPanel self = (ControlPanel) malloc(sizeof(struct ControlPanel_t));

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */

    /* Set the receiver's contents to something sane */
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
    self -> selection = NULL;
    self -> subscriptions = List_alloc();
    self -> timeouts = timeouts;
    self -> username = strdup(user);
    self -> messageId = 0;
    self -> groupsCallback = groupsCallback;
    self -> groupsContext = groupsContext;
    self -> usenetCallback = usenetCallback;
    self -> usenetContext = usenetContext;

    /* Initialize the UI */
    InitializeUserInterface(self, parent);

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


/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self, Message message)
{
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    /* If a Message was provided that is in the menu, then select it
     * and set the messageId so that we can do threading */
    if (message != NULL)
    {
	tuple = (MenuItemTuple) Message_getInfo(message);

	if (tuple != NULL)
	{
	    SetGroupSelection(self, tuple);
	}

	self -> messageId = Message_getId(message);
    }
    else
    {
	self -> messageId = 0;
    }

    /* Make the receiver visible */
    XtPopup(self -> top, XtGrabNone);
}


/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget)
{
    SANITY_CHECK(self);

    /* Press the magic OK button */
    ActionSend(widget, self, NULL);
}
