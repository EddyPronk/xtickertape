/* $Id: main.c,v 1.30 1998/10/16 02:09:19 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "Message.h"
#include "Subscription.h"
#include "ElvinConnection.h"
#include "Control.h"
#include "Tickertape.h"

#define HOST "elvin"
#define PORT 5678
#define BUFFERSIZE 8192

/* Static function headers */
static void NotifyAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams);
static void Click(Widget widget, XtPointer ignored, XtPointer context);
static void *RegisterInput(int fd, void *callback, void *rock);
static void UnregisterInput(void *rock);
static void RegisterTimer(unsigned long interval, void *callback, void *rock);
static void SendMessage(Message message, void *context);
static void ReceiveMessage(Message message, void *context);
static FILE *GetGroupFile();
static void Usage(char *argv[]);
static void ParseArgs(
    int argc, char *argv[],
    char **host_return, int *port_return,
    char **user_return);


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
    /* if main window or the tickertape then quit */
    if ((widget == top) || (widget == (Widget) tickertape))
    {
	XtDestroyApplicationContext(XtWidgetToApplicationContext(widget));
	exit(0);
    }

    /* If the control panel then hide the widget */
    XtPopdown(widget);
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

    return fopen(buffer, "r");
}


/* Print out usage message */
static void Usage(char *argv[])
{
    fprintf(stderr, "usage: %s [-host hostname] [-port port] [-user username]\n", argv[0]);
}

/* Parses arguments and sets stuff up */
static void ParseArgs(
    int argc, char *argv[],
    char **host_return, int *port_return,
    char **user_return)
{
    int index = 1;
    *host_return = HOST;
    *port_return = PORT;
    *user_return = NULL;

    while (index < argc)
    {
	char *arg = argv[index++];

	/* Make sure options begin with '-' */
	if (arg[0] != '-')
	{
	    Usage(argv);
	    exit(1);
	}

	/* Make sure the option has an argument */
	if (index >= argc)
	{
	    Usage(argv);
	    exit(1);
	}

	/* Deal with the argument */
	switch (arg[1])
	{
	    /* host */
	    case 'h':
	    {
		if (strcmp(arg, "-host") == 0)
		{
		    *host_return = argv[index++];
		    break;
		}

		Usage(argv);
		exit(1);
	    }

	    /* port */
	    case 'p':
	    {
		if (strcmp(arg, "-port") == 0)
		{
		    *port_return = atoi(argv[index++]);
		    break;
		}

		Usage(argv);
		exit(1);
	    }

	    case 'u':
	    {
		if (strcmp(arg, "-user") == 0)
		{
		    *user_return = argv[index++];
		    break;
		}

		Usage(argv);
		exit(1);
	    }

	    /* Anything else is bogus */
	    default:
	    {
		Usage(argv);
		exit(1);
	    }
	}
    }

    /* Set the user name if not specified on the command line */
    if (*user_return == NULL)
    {
	*user_return = getpwuid(getuid()) -> pw_name;
    }
}




/* Parse args and go */
int main(int argc, char *argv[])
{
    ElvinConnection connection;
    FILE *file;
    List subscriptions;
    Atom deleteAtom;
    char *host;
    int port;
    char *user;
    int index = 1;

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv, NULL,
	XtNtitle, "Tickertape",
	XtNborderWidth, 0,
	NULL);

    /* Parse the arguments list now that Xt has had a look */
    ParseArgs(argc, argv, &host, &port, &user);

    /* Add a calback for when it gets destroyed (?) */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Create the tickertape widget */
    tickertape = (TickertapeWidget) XtVaCreateManagedWidget(
	"scroller", tickertapeWidgetClass, top,
	NULL);
    XtAddCallback((Widget)tickertape, XtNcallback, Click, NULL);


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
	host, port, user, subscriptions,
	Subscription_alloc("tickertape", "", 0, 0, 10, 10, ReceiveMessage, tickertape));

    /* Build the widget */
    controlPanel = ControlPanel_alloc(top, user, subscriptions, SendMessage, connection);
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
