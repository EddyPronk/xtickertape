/* $Id: main.c,v 1.9 1997/02/12 13:51:57 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "Message.h"
#include "BridgeConnection.h"
#include "Tickertape.h"

#define CONNECTING 1
#define HOSTNAME "fatcat.dstc.edu.au"
#define PORT 8800


/* Shuts everything down and go away */
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);

/* The default application actions table */
static XtActionsRec actions[] =
{
    {"quit", QuitAction}
};


/* Shuts everything down and go away */
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams)
{
    printf("QuitAction\n");
    exit(0);
}


int main(int argc, char *argv[])
{
    BridgeConnection connection;
    List subscriptions;
    Widget top;
    TickertapeWidget tickertape;
    XtAppContext context;
    Atom deleteAtom;

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
	NULL, 0, &argc, argv,
	NULL, NULL);

    /* Add a calback for when it gets destroyed (?) */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Create the tickertape widget */
    tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"ticker", tickertapeWidgetClass, top,
	NULL, 0);

    /* listen for messages from the bridge */
    connection = BridgeConnection_alloc(tickertape, "fatcat", 8800, subscriptions);
    XtAppAddInput(
	context,
	BridgeConnection_getFD(connection),
	(XtPointer) XtInputReadMask,
	(XtInputCallbackProc) BridgeConnection_read,
	connection);

    /* Build the widget */
    XtRealizeWidget(top);

    /* Quit when the window is closed */
    XtOverrideTranslations(top, XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()"));
    deleteAtom = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", FALSE);
    XSetWMProtocols(XtDisplay(top), XtWindow(top), &deleteAtom, 1);

    /* Let 'er rip! */
    XtAppMainLoop(context);

    return 0;
}
