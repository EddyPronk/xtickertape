/* $Id: Control.c,v 1.23 1998/10/28 07:42:14 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

#include "sanity.h"
#include "Control.h"
#include "List.h"

#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/MenuButton.h>

#ifdef SANITY
static char *sanity_value = "ControlPanel";
static char *sanity_freed = "Freed ControlPanel";
#endif /* SANITY */


char *timeouts[] =
{
    "1",
    "5",
    "10",
    "30",
    "60",
    NULL
};



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

    /* The receiver's mime menu button */
    Widget mimeType;

    /* The receiver's mime args text widget */
    Widget mimeArgs;

    /* The receiver's text text widget */
    Widget text;

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
};


/*
 *
 * Static function prototypes
 *
 */
static void SetLabel(Widget widget, char *label);
static char *GetLabel(Widget widget);
static void SetString(Widget widget, char *string);
static char *GetString(Widget widget);
static void SetGroupValue(Widget item, MenuItemTuple tuple, XtPointer unused);
static void SetTimeoutValue(Widget item, XtPointer ignored, XtPointer unused);
static void SetMimeTypeValue(Widget widget, XtPointer ignored, XtPointer unused);
static Widget CreateUserBox(ControlPanel self, Widget parent);
static Widget CreateGroupMenu(ControlPanel self, Widget parent);
static Widget CreateGroupBox(ControlPanel self, Widget parent, Widget left);
static Widget CreateTimeoutMenu(ControlPanel self, Widget parent);
static Widget CreateTimeoutBox(ControlPanel self, Widget parent, Widget left);
static Widget CreateTopBox(ControlPanel self, Widget parent);
static Widget CreateTextBox(ControlPanel self, Widget parent);
static Widget CreateMimeTypeMenu(ControlPanel self, Widget parent);
static Widget CreateMimeBox(ControlPanel self, Widget parent);
static Widget CreateBottomBox(ControlPanel self, Widget parent);
static Widget CreateControlPanelPopup(ControlPanel self, Widget parent);
static void SetSelection(ControlPanel self, MenuItemTuple tuple);
static char *GetUser(ControlPanel self);
static void SetUser(ControlPanel self, char *user);
static char *GetText(ControlPanel self);
static void SetText(ControlPanel self, char *text);
static int GetTimeout(ControlPanel self);
static void SetMimeArgs(ControlPanel self, char *args);
static char *GetMimeArgs(ControlPanel self);
static char *GetMimeType(ControlPanel self);
Message ConstructMessage(ControlPanel self);
static void ActionOK(Widget button, ControlPanel self, XtPointer ignored);
static void ActionClear(Widget button, ControlPanel self, XtPointer ignored);
static void ActionCancel(Widget button, ControlPanel self, XtPointer ignored);


/* Sets the value of a Widget's "label" resource */
static void SetLabel(Widget widget, char *label)
{
    XtVaSetValues(widget, XtNlabel, label, NULL);
}

/* Answers the value of a Widget's "label" resource */
static char *GetLabel(Widget widget)
{
    char *label;

    XtVaGetValues(widget, XtNlabel, &label, NULL);
    return label;
}

/* Sets the value of a Widget's "string" resource */
static void SetString(Widget widget, char *string)
{
    XtVaSetValues(widget, XtNstring, string, NULL);
}

/* Answers the value of a Widget's "string" resource */
static char *GetString(Widget widget)
{
    char *string;

    XtVaGetValues(widget, XtNstring, &string, NULL);
    return string;
}

/* Sets the label of a MenuButton to the label of a Menu Object */
static void SetGroupValue(Widget item, MenuItemTuple tuple, XtPointer unused)
{
    ControlPanel self = tuple -> controlPanel;
    SANITY_CHECK(self);

    SetSelection(self, tuple);
}

/* Sets the label of a MenuButton to the label of a Menu Object */
static void SetTimeoutValue(Widget widget, XtPointer ignored, XtPointer unused)
{
    SetLabel(XtParent(XtParent(widget)), GetLabel(widget));
}

/* Sets the label of the MimeType field */
static void SetMimeTypeValue(Widget widget, XtPointer ignored, XtPointer unused)
{
    SetLabel(XtParent(XtParent(widget)), GetLabel(widget));
}

/* Constructs the User label box */
static Widget CreateUserBox(ControlPanel self, Widget parent)
{
    Widget form, label;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"userForm", formWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawRubber,
	NULL);
    label = XtVaCreateManagedWidget(
	"userLabel", labelWidgetClass, form,
	XtNlabel, "User",
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	NULL);
    self -> user = XtVaCreateManagedWidget(
	"user", asciiTextWidgetClass, form,
	XtNeditType, XawtextEdit,
	XtNstring, self -> username,
	XtNfromHoriz, label,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainRight,
	NULL);
    XtOverrideTranslations(self -> user, XtParseTranslationTable("<Key>Return: notify()"));
    return form;
}


/* Construct the menu for the Group list */
static Widget CreateGroupMenu(ControlPanel self, Widget parent)
{
    SANITY_CHECK(self);

    return XtVaCreatePopupShell(
	"groupMenu", simpleMenuWidgetClass, parent, NULL);
}

/* Create the Group box */
static Widget CreateGroupBox(ControlPanel self, Widget parent, Widget left)
{
    Widget form, label;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"groupForm", formWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNfromHoriz, left,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawRubber,
	XtNright, XawRubber,
	NULL);
    label = XtVaCreateManagedWidget(
	"groupLabel", labelWidgetClass, form,
	XtNlabel, "Group",
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	NULL);
    self -> group = XtVaCreateManagedWidget(
	"groupMenu", menuButtonWidgetClass, form,
	XtNmenuName, "groupMenu",
	XtNresize, False,
	XtNwidth, 70,
	XtNfromHoriz, label,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainRight,
	NULL);
    self -> groupMenu = CreateGroupMenu(self, self -> group);
    return form;
}


/* Creates the popup menu for timeout selection */
static Widget CreateTimeoutMenu(ControlPanel self, Widget parent)
{
    char **timeout;
    Widget menu;

    SANITY_CHECK(self);
    menu = XtVaCreatePopupShell("timeoutMenu", simpleMenuWidgetClass, parent, NULL);
    
    for (timeout = self -> timeouts; *timeout != NULL; timeout++)
    {
	Widget item = XtVaCreateManagedWidget(
	    *timeout, smeBSBObjectClass, menu,
	    NULL);
	XtAddCallback(item, XtNcallback, SetTimeoutValue, NULL);
    }

    return menu;
}

/* Creates the timeout box */
static Widget CreateTimeoutBox(ControlPanel self, Widget parent, Widget left)
{
    Widget form, label;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"timeoutForm", formWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNfromHoriz, left,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawRubber,
	XtNright, XawChainRight,
	NULL);
    label = XtVaCreateManagedWidget(
	"timeoutLabel", labelWidgetClass, form,
	XtNlabel, "Timeout",
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	NULL);
    self -> timeout = XtVaCreateManagedWidget(
	"timeout", menuButtonWidgetClass, form,
	XtNmenuName, "timeoutMenu",
	XtNresize, False,
	XtNwidth, 70,
	XtNfromHoriz, label,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainRight,
	XtNlabel, "5",
	NULL);
    CreateTimeoutMenu(self, self -> timeout);
    return form;
}

/* Constructs the top box of the Control Panel */
static Widget CreateTopBox(ControlPanel self, Widget parent)
{
    Widget form, widget;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"topForm", formWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNshowGrip, False,
	NULL);
    widget = CreateUserBox(self, form);
    widget = CreateGroupBox(self, form, widget);
    CreateTimeoutBox(self, form, widget);
    return form;
}


/* Constructs the Text box */
static Widget CreateTextBox(ControlPanel self, Widget parent)
{
    Widget form, label;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"textForm", formWidgetClass, parent,
	XtNborderWidth, 0,
	XtNshowGrip, False,
	NULL);
    label = XtVaCreateManagedWidget(
	"textLabel", labelWidgetClass, form,
	XtNlabel, "Text",
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	XtNresizable, False,
	NULL);
    self -> text = XtVaCreateManagedWidget(
	"text", asciiTextWidgetClass, form,
	XtNeditType, XawtextEdit,
	XtNfromHoriz, label,
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainRight,
	NULL);
    XtOverrideTranslations(self -> text, XtParseTranslationTable("<Key>Return: notify()"));
    return form;
}

/* Constructs the Mime type menu */
static Widget CreateMimeTypeMenu(ControlPanel self, Widget parent)
{
    Widget menu;
    Widget item;

    SANITY_CHECK(self);
    menu = XtVaCreatePopupShell("mimeTypeMenu", simpleMenuWidgetClass, parent, NULL);

    /* For now we only support one type... */
    item = XtVaCreateManagedWidget(
	"x-elvin/url", smeBSBObjectClass, menu,
	NULL);
    XtAddCallback(item, XtNcallback, SetMimeTypeValue, NULL);

    return menu;
}

/* Constructs the Mime box */
static Widget CreateMimeBox(ControlPanel self, Widget parent)
{
    Widget form, label;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"mimeForm", formWidgetClass, parent,
	XtNborderWidth, 0,
	XtNshowGrip, False,
	NULL);

    label = XtVaCreateManagedWidget(
	"mimeLabel", labelWidgetClass, form,
	XtNlabel, "Mime",
	XtNborderWidth, 0,
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	XtNbottom, XawChainTop,
	XtNresizable, False,
	NULL);

    self -> mimeType = XtVaCreateManagedWidget(
	"mimeTypeMenu", menuButtonWidgetClass, form,
	XtNmenuName, "mimeTypeMenu",
	XtNresize, False,
	XtNfromHoriz, label,
	XtNtop, XawChainTop,
	XtNbottom, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainLeft,
	XtNlabel, "x-elvin/url",
	XtNresizable, False,
	NULL);
    CreateMimeTypeMenu(self, self -> mimeType);

    self -> mimeArgs = XtVaCreateManagedWidget(
	"mimeArgs", asciiTextWidgetClass, form,
	XtNeditType, XawtextEdit,
	XtNstring, "",
	XtNfromHoriz, self -> mimeType,
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNright, XawChainRight,
	NULL);
    XtOverrideTranslations(self -> mimeArgs, XtParseTranslationTable("<Key>Return: notify()"));

    return form;
}

/* Creates the bottom box (where the buttons live) */
static Widget CreateBottomBox(ControlPanel self, Widget parent)
{
    Widget form, button;

    SANITY_CHECK(self);
    form = XtVaCreateManagedWidget(
	"bottomForm", formWidgetClass, parent,
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNbottom, XawChainTop,
	XtNright, XawChainRight,
	XtNborderWidth, 0,
	XtNallowResize, False,
	NULL);
    button = XtVaCreateManagedWidget(
	"ok", commandWidgetClass, form,
	XtNlabel, "OK",
	XtNwidth, 50, /* force buttons to be equal width */
	XtNtop, XawChainTop,
	XtNleft, XawChainLeft,
	XtNbottom, XawChainTop,
	XtNright, XawRubber,
	NULL);
    XtAddCallback(button, XtNcallback, (XtCallbackProc)ActionOK, self);

    button = XtVaCreateManagedWidget(
	"clear", commandWidgetClass, form,
	XtNlabel, "Clear",
	XtNwidth, 50,
	XtNtop, XawChainTop,
	XtNleft, XawRubber,
	XtNbottom, XawChainTop,
	XtNright, XawRubber,
	XtNfromHoriz, button,
	NULL);
    XtAddCallback(button, XtNcallback, (XtCallbackProc)ActionClear, self);

    button = XtVaCreateManagedWidget(
	"cancel", commandWidgetClass, form,
	XtNlabel, "Cancel",
	XtNwidth, 50,
	XtNtop, XawChainTop,
	XtNleft, XawRubber,
	XtNbottom, XawChainTop,
	XtNright, XawChainRight,
	XtNfromHoriz, button,
	NULL);
    XtAddCallback(button, XtNcallback, (XtCallbackProc)ActionCancel, self);
    return form;
}

/* Constructs the entire control panel */
static Widget CreateControlPanelPopup(ControlPanel self, Widget parent)
{
    Widget box;

    SANITY_CHECK(self);
    box = XtVaCreateManagedWidget("paned", panedWidgetClass, parent, NULL);
    CreateTopBox(self, box);
    CreateTextBox(self, box);
    CreateMimeBox(self, box);
    CreateBottomBox(self, box);
    return box;
}

/* Sets the receiver's group */
static void SetSelection(ControlPanel self, MenuItemTuple tuple)
{
    SANITY_CHECK(self);
    self -> selection = tuple;

    if (self -> selection == NULL)
    {
	SetLabel(self -> group, "");
    }
    else
    {
	SetLabel(self -> group, tuple -> title);
    }

    self -> messageId = 0;
}

/* Answers the receiver's user */
static char *GetUser(ControlPanel self)
{
    SANITY_CHECK(self);
    return GetString(self -> user);
}

/* Sets the receiver's user */
static void SetUser(ControlPanel self, char *user)
{
    SANITY_CHECK(self);
    SetString(self -> user, user);
}


/* Answers the receiver's text */
static char *GetText(ControlPanel self)
{
    SANITY_CHECK(self);
    return GetString(self -> text);
}

/* Sets the receiver's text */
static void SetText(ControlPanel self, char *text)
{
    SANITY_CHECK(self);
    SetString(self -> text, text);
}

/* Answers the receiver's timeout */
static int GetTimeout(ControlPanel self)
{
    SANITY_CHECK(self);
    return atoi(GetLabel(self -> timeout));
}

/* Sets the receiver's MIME args */
static void SetMimeArgs(ControlPanel self, char *args)
{
    SANITY_CHECK(self);
    SetString(self -> mimeArgs, args);
}

/* Answers the receiver's MIME args */
static char *GetMimeArgs(ControlPanel self)
{
    char *args;
    SANITY_CHECK(self);

    args = GetString(self -> mimeArgs);
    if (strlen(args) > 0)
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
    SANITY_CHECK(self);
    return GetLabel(self -> mimeType);
}


/* Answers a message based on the receiver's current state */
Message ConstructMessage(ControlPanel self)
{
    char *mimeArgs;
    SANITY_CHECK(self);
    
    /* Determine our MIME args */
    mimeArgs = GetMimeArgs(self);

    /* Construct a message */
    return Message_alloc(
	self -> selection,
	self -> selection -> title,
	GetUser(self),
	GetText(self),
	GetTimeout(self),
	(mimeArgs == NULL) ? NULL : GetMimeType(self),
	mimeArgs,
	random(),
	self -> messageId);
}

/*
 *
 * Actions
 *
 */

/* Callback for OK button */
static void ActionOK(Widget button, ControlPanel self, XtPointer ignored)
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
}

/* Callback for Cancel button */
static void ActionCancel(Widget button, ControlPanel self, XtPointer ignored)
{
    SANITY_CHECK(self);
    XtPopdown(self -> top);
}


/*
 *
 * Exported functions
 *
 */

/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(Widget parent, char *user)
{
    ControlPanel self = (ControlPanel) malloc(sizeof(struct ControlPanel_t));
    Atom deleteAtom;

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> top = XtVaCreatePopupShell(
	"controlPanel", transientShellWidgetClass, parent,
	XtNtitle, "Tickertape Control Panel",
	NULL);

    self -> selection = NULL;
    self -> subscriptions = List_alloc();
    self -> timeouts = timeouts;
    self -> username = strdup(user);
    self -> messageId = 0;

    CreateControlPanelPopup(self, self -> top);
    ActionClear(NULL, self, NULL);

    XtOverrideTranslations(
	self -> top,
	XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()"));
    XtRealizeWidget(self -> top);
    deleteAtom = XInternAtom(XtDisplay(self -> top), "WM_DELETE_WINDOW", False);
    XSetWMProtocols(XtDisplay(self -> top), XtWindow(self -> top), &deleteAtom, 1);
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


/* Adds a subscription to the receiver */
void *ControlPanel_addSubscription(
    ControlPanel self, char *title,
    ControlPanelCallback callback, void *context)
{
    Widget item;
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    item = XtVaCreateManagedWidget(
	title, smeBSBObjectClass, self -> groupMenu,
	NULL);

    if ((tuple = (MenuItemTuple) malloc(sizeof(struct MenuItemTuple_t))) == NULL)
    {
	fprintf(stderr, "*** Out of memory\n");
	exit(1);
    }

    tuple -> controlPanel = self;
    tuple -> widget = item;
    tuple -> title = strdup(title);
    tuple -> callback = callback;
    tuple -> context = context;

    XtAddCallback(item, XtNcallback, (XtCallbackProc)SetGroupValue, (XtPointer)tuple);

    if (self -> selection == NULL)
    {
	SetSelection(self, tuple);
    }

    List_addLast(self -> subscriptions, tuple);

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
	SetSelection(self, List_first(self -> subscriptions));
    }

    /* Free up some memory */
    free(tuple -> title);
    free(tuple);
}

/* Retitles an entry */
void ControlPanel_retitleSubscription(ControlPanel self, void *info, char *title)
{
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    /* Update the Widget's label */
    tuple = (MenuItemTuple)info;
    free(tuple -> title);
    tuple -> title = title;
    SetLabel(tuple -> widget, tuple -> title);


    /* If this is the selected item, then update the MenuButton's label as well */
    if (self -> selection == tuple)
    {
	SetLabel(self -> group, title);
    }
}


/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self, Message message)
{
    MenuItemTuple tuple;
    SANITY_CHECK(self);

    if (message != NULL)
    {
	tuple = (MenuItemTuple) Message_getInfo(message);

	if (tuple != NULL)
	{
	    SetSelection(self, tuple);
	}

	self -> messageId = Message_getId(message);
    }
    else
    {
	self -> messageId = 0;
    }

    XtPopup(self -> top, XtGrabNone);
}


/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget)
{
    SANITY_CHECK(self);

    /* Press the magic OK button */
    ActionOK(widget, self, NULL);
}
