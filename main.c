/* $Id: main.c,v 1.12 1997/02/15 02:32:16 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "Message.h"
#include "BridgeConnection.h"
#include "Control.h"
#include "Tickertape.h"

#define HOSTNAME "fatcat"
#define PORT 8800


/* Notification of something */
static void NotifyAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);

/* Shuts everything down and go away */
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);

/* The ControlPanel popup */
static ControlPanel controlPanel;

/* The main window widget */
Widget top;

/* The default application actions table */
static XtActionsRec actions[] =
{
    {"notify", NotifyAction},
    {"quit", QuitAction}
};

/* Notification of something */
static void NotifyAction(Widget widget, XEvent *event, String *params, Cardinal *cparams)
{
    /* pass this on to the control panel */
    ControlPanel_handleNotify(controlPanel, widget);
}

/* Callback for when the Window Manager wants to close a window */
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams)
{
    /* if main window then quit */
    if (widget == top)
    {
	XtDestroyApplicationContext(XtWidgetToApplicationContext(widget));
	exit(0);
    }
    /* otherwise just hide the popup */
    else
    {
	XtPopdown(widget);
    }
}

/* Callback for buttonpress in tickertape window */
void Click(Widget widget, XtPointer context, XtPointer ignored)
{
    ControlPanel_show(controlPanel);
}

/* Callback for message to send from control panel */
void sendMessage(Message message, void *context)
{
    BridgeConnection connection = (BridgeConnection)context;

    BridgeConnection_send(connection, message);
    Message_free(message);
}



/* Parse args and go */
int main(int argc, char *argv[])
{
    BridgeConnection connection;
    List subscriptions;
    TickertapeWidget tickertape;
    XtAppContext context;
    Atom deleteAtom;
    char *hostname = HOSTNAME;
    int port = PORT;
    int index = 1;

    /* Connect to the bridge */
    subscriptions = List_alloc();
    List_addLast(subscriptions, "Chat");
    List_addLast(subscriptions, "Coffee");
    List_addLast(subscriptions, "Rooms");
    List_addLast(subscriptions, "level7");
    List_addLast(subscriptions, "email");
    List_addLast(subscriptions, "Files");
    List_addLast(subscriptions, "rwall");
    List_addLast(subscriptions, "elvin_news");
    List_addLast(subscriptions, "phelps");

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv, NULL,
	XtNtitle, "Tickertape",
	XtNborderWidth, 0,
	NULL);

    /* Read args for hostname and port */
    if (index < argc)
    {
	hostname = argv[index++];
    }
    if (index < argc)
    {
	port = atoi(argv[index++]);
    }

    /* Add a calback for when it gets destroyed (?) */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Create the tickertape widget */
    tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"scroller", tickertapeWidgetClass, top,
	NULL);
    XtAddCallback((Widget)tickertape, XtNcallback, Click, NULL);

    /* listen for messages from the bridge */
    connection = BridgeConnection_alloc(tickertape, hostname, port, subscriptions);
    XtAppAddInput(
	context,
	BridgeConnection_getFD(connection),
	(XtPointer) XtInputReadMask,
	(XtInputCallbackProc) BridgeConnection_read,
	connection);

    /* Build the widget */
    controlPanel = ControlPanel_alloc(top, sendMessage, connection);
    XtRealizeWidget(top);

    /* Quit when the window is closed */
    XtOverrideTranslations(top, XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()"));
    deleteAtom = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", FALSE);
    XSetWMProtocols(XtDisplay(top), XtWindow(top), &deleteAtom, 1);

    {
	Message message = Message_alloc("tickertape", "internal", "nevermind", 30);
	TtAddMessage(tickertape, message);
    }

    /* Let 'er rip! */
    XtAppMainLoop(context);

    return 0;
}
