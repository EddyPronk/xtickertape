/* $Id: Control.c,v 1.1 1997/02/14 10:52:30 phelps Exp $ */

#include "Control.h"
#include <X11/Shell.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/SimpleMenu.h>
#include <X11/Xaw/SmeBSB.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/MenuButton.h>

char *groups[] =
{
    "Chat",
    "level7",
    "phelps",
    NULL
};

char *timeouts[] =
{
    "1",
    "10",
    "30",
    "60",
    NULL
};



struct ControlPanel_t
{
    ControlPanelCallback callback;
    void *context;
    Widget top;
    Widget user;
    Widget group;
    Widget timeout;
    Widget text;
    char **groups;
    char **timeouts;
};


/*
 * Actions
 */
static void ActionOK(Widget button, XtPointer context, XtPointer ignored)
{
    ControlPanel self = (ControlPanel) context;
    if (self -> callback)
    {
	Message message = ControlPanel_createMessage(self);
	(*self -> callback)(message, self -> context);
    }
    XtPopdown(self -> top);
}

static void ActionClear(Widget button, XtPointer context, XtPointer ignored)
{
    printf("clear!?\n");
}

static void ActionCancel(Widget button, XtPointer context, XtPointer ignored)
{
    ControlPanel self = (ControlPanel) context;
    XtPopdown(self -> top);
}



/* Sets the label of a Widget */
static void setLabel(Widget widget, char *label)
{
    Arg arg;

    arg.name = XtNlabel;
    arg.value = (XtArgVal)label;
    XtSetValues(widget, &arg, 1);
}

/* Answers the label of a Widget */
static char *getLabel(Widget widget)
{
    Arg arg;
    char *label;

    arg.name = XtNlabel;
    arg.value = (XtArgVal)&label;
    XtGetValues(widget, &arg, 1);
    return label;
}

/*
static void setString(Widget widget, char *string)
{
    Arg arg;

    arg.name = XtNstring;
    arg.value = (XtArgVal)string;
    XtSetValues(widget, &arg, 1);
}
*/

static char *getString(Widget widget)
{
    Arg arg;
    char *string;

    arg.name = XtNstring;
    arg.value = (XtArgVal)&string;
    XtGetValues(widget, &arg, 1);
    return string;
}


static void setMBValue(Widget item, XtPointer context, XtPointer ignored)
{
    setLabel((Widget)context, getLabel(item));
}


/* Constructs the User label box */
static void createUserBox(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"userBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	NULL);
    XtVaCreateManagedWidget(
	"userLabel", labelWidgetClass, box,
	XtNlabel, "User",
	XtNborderWidth, 0,
	NULL);
    self -> user = XtVaCreateManagedWidget(
	"user", asciiTextWidgetClass, box,
	XtNeditType, XawtextEdit,
	XtNstring, "phelps",
	NULL);
}

static void createGroupMenu(ControlPanel self, Widget parent)
{
    char **group;
    Widget menu = XtVaCreatePopupShell(
	"groupMenu", simpleMenuWidgetClass, parent,
	NULL);
    
    for (group = self -> groups; *group != NULL; group++)
    {
	Widget item = XtVaCreateManagedWidget(
	    *group, smeBSBObjectClass, menu,
	    NULL);
	XtAddCallback(item, XtNcallback, setMBValue, parent);
    }
}

static void createGroupBox(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"groupBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	NULL);
    XtVaCreateManagedWidget(
	"groupLabel", labelWidgetClass, box,
	XtNlabel, "group",
	XtNborderWidth, 0,
	NULL);
    self -> group = XtVaCreateManagedWidget(
	"groupMenu", menuButtonWidgetClass, box,
	XtNlabel, self -> groups[0],
	XtNmenuName, "groupMenu",
	NULL);
    createGroupMenu(self, self -> group);
}

static void createTimeoutMenu(ControlPanel self, Widget parent)
{
    char **timeout;
    Widget menu = XtVaCreatePopupShell(
	"timeoutMenu", simpleMenuWidgetClass, parent,
	NULL);
    
    for (timeout = self -> timeouts; *timeout != NULL; timeout++)
    {
	Widget item = XtVaCreateManagedWidget(
	    *timeout, smeBSBObjectClass, menu,
	    NULL);
	XtAddCallback(item, XtNcallback, setMBValue, parent);
    }
}

static void createTimeoutBox(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"timeoutBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	NULL);
    XtVaCreateManagedWidget(
	"timeoutLabel", labelWidgetClass, box,
	XtNlabel, "timeout",
	XtNborderWidth, 0,
	NULL);
    self -> timeout = XtVaCreateManagedWidget(
	"timeout", menuButtonWidgetClass, box,
	XtNlabel, self -> timeouts[0],
	XtNmenuName, "timeoutMenu",
	NULL);
    createTimeoutMenu(self, self -> timeout);
}

/* Constructs the top box of the Control Panel */
static void createTopBox(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"topBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNshowGrip, False,
	NULL);
    createUserBox(self, box);
    createGroupBox(self, box);
    createTimeoutBox(self, box);
}


/* Constructs the Text box */
static void createTextBox(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"textBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	XtNshowGrip, False,
	NULL);
    XtVaCreateManagedWidget(
	"textLabel", labelWidgetClass, box,
	XtNlabel, "text",
	XtNborderWidth, 0,
	NULL);
    self -> text = XtVaCreateManagedWidget(
	"text", asciiTextWidgetClass, box,
	XtNeditType, XawtextEdit,
	NULL);
}


static void createBottomBox(ControlPanel self, Widget parent)
{
    Widget box, button;

    box = XtVaCreateManagedWidget(
	"bottomBox", boxWidgetClass, parent,
	XtNorientation, XtorientHorizontal,
	XtNborderWidth, 0,
	NULL);
    button = XtVaCreateManagedWidget(
	"ok", commandWidgetClass, box,
	XtNlabel, "OK",
	NULL);
    XtAddCallback(button, XtNcallback, ActionOK, self);

    button = XtVaCreateManagedWidget(
	"clear", commandWidgetClass, box,
	XtNlabel, "Clear",
	NULL);
    XtAddCallback(button, XtNcallback, ActionClear, self);

    button = XtVaCreateManagedWidget(
	"cancel", commandWidgetClass, box,
	XtNlabel, "Cancel",
	NULL);
    XtAddCallback(button, XtNcallback, ActionCancel, self);
}

static void createControlPanelPopup(ControlPanel self, Widget parent)
{
    Widget box = XtVaCreateManagedWidget(
	"pane", panedWidgetClass, parent,
	NULL);
    createTopBox(self, box);
    createTextBox(self, box);
    createBottomBox(self, box);
}

/* Answers the receiver's group */
char *getGroup(ControlPanel self)
{
    return getLabel(self -> group);
}

/* Answers the receiver's user */
char *getUser(ControlPanel self)
{
    return getString(self -> user);
}

/* Answers the receiver's text */
char *getText(ControlPanel self)
{
    return getString(self -> text);
}

/* Answers the receiver's timeout */
int getTimeout(ControlPanel self)
{
    return atoi(getLabel(self -> timeout));
}


/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(Widget parent, ControlPanelCallback callback, void *context)
{
    ControlPanel self = (ControlPanel) malloc(sizeof(struct ControlPanel_t));

    self -> callback = callback;
    self -> context = context;
    self -> top = XtVaCreatePopupShell(
	"controlPanel", transientShellWidgetClass,
	parent, NULL);
    self -> groups = groups;
    self -> timeouts = timeouts;
    createControlPanelPopup(self, self -> top);
    
    XtRealizeWidget(self -> top);
    return self;
}

/* Releases the resources used by the receiver */
void ControlPanel_free(ControlPanel self)
{
    free(self);
}


/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self)
{
    XtPopup(self -> top, XtGrabNone);
}


/* Answers the receiver's values as a Message */
Message ControlPanel_createMessage(ControlPanel self)
{
    return Message_alloc(getGroup(self), getUser(self), getText(self), getTimeout(self));
}


