/* $Id: main.c,v 1.27 1998/10/15 09:25:26 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "Message.h"
#include "Subscription.h"
#include "ElvinConnection.h"
#include "Control.h"
#include "Tickertape.h"

# define PORT 5678

#ifdef ELVIN_HOSTNAME
#define HOSTNAME ELVIN_HOSTNAME
#else /* ELVIN_HOSTNAME */
#define HOSTNAME "fatcat"
#endif /* ELVIN_HOSTNAME */
#define BUFFERSIZE 8192

/* Static function headers */
static void NotifyAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);
static void Click(Widget widget, XtPointer ignored, XtPointer context);
static void SendMessage(Message message, void *context);
static void ReceiveMessage(Message message, void *context);
static FILE *GetGroupFile();


/* The ControlPanel popup */
static ControlPanel controlPanel;

/* The top-level widget */
static Widget top;

/* The application context */
static XtAppContext context;

/* The tickertape widget */
TickertapeWidget tickertape;

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
static void Click(Widget widget, XtPointer ignored, XtPointer context)
{
    ControlPanel_show(controlPanel, (Message) context);
}


/* Registers an ElvinConnection with the Xt event loop */
static void *RegisterInput(int fd, void *callback, void *rock)
{
    return (void *) XtAppAddInput(
	context,
	fd, (XtPointer) XtInputReadMask,
	(XtInputCallbackProc) callback, (XtPointer) rock);
}

/* Unregisters an ElvinConnection with the Xt event loop */
static void UnregisterInput(void *rock)
{
    XtRemoveInput((XtInputId) rock);
}

/* Registers an ElvinConnection timer with the Xt event loop */
static void RegisterTimer(unsigned long interval, void *callback, void *rock)
{
    XtAppAddTimeOut(context, interval, (XtTimerCallbackProc) callback, (XtPointer) rock);
}

/* Callback for message to send from control panel */
static void SendMessage(Message message, void *context)
{
    ElvinConnection connection = (ElvinConnection)context;

    ElvinConnection_send(connection, message);
    Message_free(message);
}

/* Callback for message to show in scroller */
static void ReceiveMessage(Message message, void *context)
{
    TickertapeWidget tickertape = (TickertapeWidget)context;
    TtAddMessage(tickertape, message);
}


/* Answers the user's group file */
static FILE *GetGroupFile()
{
    char buffer[BUFFERSIZE];
    char *dir;

    dir = getenv("TICKERDIR");
    if (dir != NULL)
    {
	sprintf(buffer, "%s/group", dir);
    }
    else
    {
	sprintf(buffer, "%s/.ticker/groups", getenv("HOME"));
    }
#ifdef DEBUG    
    printf("reading group file \"%s\"\n", buffer);
#endif /* DEBUG */
    return fopen(buffer, "r");
}


/* Parse args and go */
int main(int argc, char *argv[])
{
    ElvinConnection connection;
    FILE *file;
    List subscriptions;
    Atom deleteAtom;
    char *hostname = HOSTNAME;
    int port = PORT;
    int index = 1;

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv, NULL,
	XtNtitle, "Tickertape",
	XtNborderWidth, 0,
	NULL);

    /* Add a calback for when it gets destroyed (?) */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Create the tickertape widget */
    tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"scroller", tickertapeWidgetClass, top,
	NULL);
    XtAddCallback((Widget)tickertape, XtNcallback, Click, NULL);

    /* Read args for hostname and port */
    if (index < argc)
    {
	hostname = argv[index++];
    }
    if (index < argc)
    {
	port = atoi(argv[index++]);
    }

    /* Read subscriptions from the groups file */
    subscriptions = List_alloc();
    file = GetGroupFile();
    if (file == NULL)
    {
	printf("*** unable to read ${TICKERDIR}/groups\n");
	exit(1); /* This shouldn't need to exit! */
    }
    else
    {
	Subscription_readFromGroupFile(file, subscriptions, ReceiveMessage, tickertape);
	fclose(file);
    }


    /* listen for messages from elvin */
    connection = ElvinConnection_alloc(
	hostname, port, subscriptions,
	Subscription_alloc("tickertape", "", 0, 0, 10, 10, ReceiveMessage, tickertape));

    /* Build the widget */
    controlPanel = ControlPanel_alloc(top, subscriptions, SendMessage, connection);
    XtRealizeWidget(top);

    /* Quit when the window is closed */
    XtOverrideTranslations(top, XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()"));
    deleteAtom = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", FALSE);
    XSetWMProtocols(XtDisplay(top), XtWindow(top), &deleteAtom, 1);

    /* Register the elvin connection with the event loop */
    ElvinConnection_register(connection, RegisterInput, UnregisterInput, RegisterTimer);

    /* Let 'er rip! */
    XtAppMainLoop(context);

    return 0;
}
