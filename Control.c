/* $Id: Control.c,v 1.11 1998/10/15 04:18:32 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>

#include "sanity.h"
#include "Control.h"
#include "Subscription.h"

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


/*
 * Prototypes for actions
 */
static void ActionOK(Widget button, XtPointer context, XtPointer ignored);
static void ActionClear(Widget button, XtPointer context, XtPointer ignored);
static void ActionCancel(Widget button, XtPointer context, XtPointer ignored);


/* The pieces of the control panel */
struct ControlPanel_t
{
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */
    ControlPanelCallback callback;
    void *context;
    Widget top;
    Widget user;
    Widget group;
    Widget timeout;
    Widget text;
    List subscriptions;
    char **timeouts;
    char *username;
};


/* Determines the user's login name */
static char *GetUsername()
{
    struct passwd *password = getpwuid(getuid());
    return password -> pw_name;
}


/* Sets the value of a Widget's "label" resource */
static void SetLabel(Widget widget, char *label)
{
    Arg arg;

    arg.name = XtNlabel;
    arg.value = (XtArgVal)label;
    XtSetValues(widget, &arg, 1);
}

/* Answers the value of a Widget's "label" resource */
static char *GetLabel(Widget widget)
{
    Arg arg;
    char *label;

    arg.name = XtNlabel;
    arg.value = (XtArgVal)&label;
    XtGetValues(widget, &arg, 1);
    return label;
}

/* Sets the value of a Widget's "string" resource */
static void SetString(Widget widget, char *string)
{
    Arg arg;

    arg.name = XtNstring;
    arg.value = (XtArgVal)string;
    XtSetValues(widget, &arg, 1);
}

/* Answers the value of a Widget's "string" resource */
static char *GetString(Widget widget)
{
    Arg arg;
    char *string;

    arg.name = XtNstring;
    arg.value = (XtArgVal)&string;
    XtGetValues(widget, &arg, 1);
    return string;
}

/* Sets the label of a MenuButton to the label of a Menu Object */
static void SetMBValue(Widget item, XtPointer context, XtPointer ignored)
{
    SetLabel((Widget)context, GetLabel(item));
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

/* Construct a menu item for the group list */
static void CreateGroupMenuItem(Subscription subscription, Widget context[])
{
    Widget parent = context[0];
    Widget menu = context[1];

    if (Subscription_isInMenu(subscription))
    {
	Widget item = XtVaCreateManagedWidget(
	    Subscription_getGroup(subscription), smeBSBObjectClass, menu,
	    NULL);
	XtAddCallback(item, XtNcallback, SetMBValue, parent);
    }
}

/* Construct the menu for the Group list */
static Widget CreateGroupMenu(ControlPanel self, Widget parent)
{
    Widget context[2];
    Widget menu;

    SANITY_CHECK(self);
    menu = XtVaCreatePopupShell("groupMenu", simpleMenuWidgetClass, parent, NULL);
    context[0] = parent;
    context[1] = menu;
    List_doWith(self -> subscriptions, CreateGroupMenuItem, context);
    return menu;
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
	XtNlabel, Subscription_getGroup(List_first(self -> subscriptions)),
	NULL);
    CreateGroupMenu(self, self -> group);
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
	XtAddCallback(item, XtNcallback, SetMBValue, parent);
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
	XtNlabel, "text",
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
    XtAddCallback(button, XtNcallback, ActionOK, self);

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
    XtAddCallback(button, XtNcallback, ActionClear, self);

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
    XtAddCallback(button, XtNcallback, ActionCancel, self);
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
    CreateBottomBox(self, box);
    return box;
}

/* Answers the receiver's group */
static char *GetGroup(ControlPanel self)
{
    SANITY_CHECK(self);
    return GetLabel(self -> group);
}

/* Sets the receiver's group */
static void SetGroup(ControlPanel self, char *group)
{
    SANITY_CHECK(self);
    SetLabel(self -> group, group);
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

/* Sets the receiver's timeout */
static void SetTimeout(ControlPanel self, int timeout)
{
    char buffer[80];

    SANITY_CHECK(self);
    sprintf(buffer, "%d", timeout);
    SetLabel(self -> timeout, buffer);
}


/* Helper function for SubscriptionForGroup */
static void MatchSubscription(Subscription self, char *group, Subscription *result)
{
    if (strcmp(Subscription_getGroup(self), group) == 0)
    {
	*result = self;
    }
}

/* Answers the subscription matching the given group */
static Subscription SubscriptionForGroup(ControlPanel self, char *group)
{
    Subscription result = NULL;
    List_doWithWith(self -> subscriptions, MatchSubscription, group, &result);
    return result;
}


/*
 * Actions
 */
/* Callback for OK button */
static void ActionOK(Widget button, XtPointer context, XtPointer ignored)
{
    ControlPanel self = (ControlPanel) context;
    SANITY_CHECK(self);

    if (self -> callback)
    {
	Message message = ControlPanel_createMessage(self);
	(*self -> callback)(message, self -> context);
    }
    XtPopdown(self -> top);
}

/* Callback for Clear button */
static void ActionClear(Widget button, XtPointer context, XtPointer ignored)
{
    ControlPanel self = (ControlPanel) context;

    SANITY_CHECK(self);
    SetUser(self, self -> username);
    SetText(self, "");
    SetTimeout(self, 10);
}

/* Callback for Cancel button */
static void ActionCancel(Widget button, XtPointer context, XtPointer ignored)
{
    ControlPanel self = (ControlPanel) context;

    SANITY_CHECK(self);
    XtPopdown(self -> top);
}



/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(
    Widget parent, List subscriptions,
    ControlPanelCallback callback, void *context)
{
    ControlPanel self = (ControlPanel) malloc(sizeof(struct ControlPanel_t));
    Atom deleteAtom;

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> subscriptions = subscriptions;
    self -> callback = callback;
    self -> context = context;
    self -> username = GetUsername();
    self -> top = XtVaCreatePopupShell(
	"controlPanel", transientShellWidgetClass, parent,
	XtNtitle, "Tickertape Control Panel",
	NULL);
    self -> timeouts = timeouts;

    CreateControlPanelPopup(self, self -> top);
    ActionClear(NULL, (XtPointer)self, NULL);

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


/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self, Message message)
{
    SANITY_CHECK(self);

    /* If a Message has been provided then select its group in the menu */
    if (message)
    {
	Subscription subscription = SubscriptionForGroup(self, Message_getGroup(message));
	if (Subscription_isInMenu(subscription))
	{
	    SetGroup(self, Subscription_getGroup(subscription));
	}
    }

    XtPopup(self -> top, XtGrabNone);
}


/* Answers the receiver's values as a Message */
Message ControlPanel_createMessage(ControlPanel self)
{
    SANITY_CHECK(self);
    /* FIX THIS: should include MIME stuff */
    return Message_alloc(
	GetGroup(self),
	GetUser(self),
	GetText(self),
	GetTimeout(self),
	NULL,
	NULL);
}

/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget)
{
    SANITY_CHECK(self);

    if (widget == self -> text)
    {
	ActionOK(widget, (XtPointer)self, NULL);
    }
}
