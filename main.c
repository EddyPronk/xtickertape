/* $Id: main.c,v 1.31 1998/10/21 01:58:09 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include "Ticker.h"

#define HOST "elvin"
#define PORT 5678

/* Static function headers */
static void NotifyAction(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);
static void QuitAction(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);
static void Usage(char *argv[]);
static void ParseArgs(
    int argc, char *argv[],
    char **user_return, char **file_return,
    char **host_return, int *port_return);


/* The Tickertape */
static Tickertape tickertape;

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
    Tickertape_handleNotify(tickertape, widget);
}

/* Callback for when the Window Manager wants to close a window */
static void QuitAction(Widget widget, XEvent *event, String *params, Cardinal *cparams)
{
    Tickertape_handleQuit(tickertape, widget);
}

/* Print out usage message */
static void Usage(char *argv[])
{
    fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
    fprintf(stderr, "  -host hostname\n");
    fprintf(stderr, "  -port port\n");
    fprintf(stderr, "  -user username\n");
    fprintf(stderr, "  -groupsfile filename\n");
}

/* Parses arguments and sets stuff up */
static void ParseArgs(
    int argc, char *argv[],
    char **user_return, char **file_return,
    char **host_return, int *port_return)
{
    int index = 1;
    *user_return = NULL;
    *file_return = NULL;
    *host_return = HOST;
    *port_return = PORT;

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
	    /* groupsfile */
	    case 'g':
	    {
		if (strcmp(arg, "-groupsfile") == 0)
		{
		    *file_return = argv[index++];
		    break;
		}

		Usage(argv);
		exit(1);
	    }

	    /* user */
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

    /* Set the groups file name if not specified on the command line */
    if (*file_return == NULL)
    {
	char *dir;

	/* Try $TICKERDIR/groups */
	dir = getenv("TICKERDIR");
	if (dir != NULL)
	{
	    *file_return = malloc(strlen(dir) + sizeof("/groups"));
	    sprintf(*file_return, "%s/groups", dir);
	    return;
	}

	/* No $TICKERDIR.  Try $HOME/.ticker/groups */
	dir = getenv("HOME");
	if (dir != NULL)
	{
	    *file_return = malloc(strlen(dir) + sizeof("/.ticker/groups"));
	    sprintf(*file_return, "%s/.ticker/groups", dir);
	    return;
	}

	/* No $HOME either.  Give up. */
	*file_return = NULL;
    }
}




/* Parse args and go */
int main(int argc, char *argv[])
{
    XtAppContext context;
    Widget top;
    char *user;
    char *file;
    char *host;
    int port;
    Atom deleteAtom;

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv, NULL,
	XtNtitle, "Tickertape",
	XtNborderWidth, 0,
	NULL);

    /* Add a calback for when it gets destroyed */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Scan what's left of the arguments */
    ParseArgs(argc, argv, &user, &file, &host, &port);

    /* Create a Tickertape */
    tickertape = Tickertape_alloc(user, file, host, port, top);

    /* Quit when the window is closed */
    XtOverrideTranslations(
	top, XtParseTranslationTable("<Message>WM_PROTOCOLS: quit()"));
    deleteAtom = XInternAtom(XtDisplay(top), "WM_DELETE_WINDOW", FALSE);
    XSetWMProtocols(XtDisplay(top), XtWindow(top), &deleteAtom, 1);

    /* Let 'er rip! */
    XtAppMainLoop(context);

    /* Keep the compiler happy */
    return 0;
}

