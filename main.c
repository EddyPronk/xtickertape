/* $Id: main.c,v 1.33 1998/10/29 04:23:10 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include "Ticker.h"

#include "red.xbm"
#include "white.xbm"
#include "mask.xbm"

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
    char **user_return,
    char **groupsFile_return, char **usenetFile_return,
    char **host_return, int *port_return);
static Window CreateIcon(Widget shell);


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
    fprintf(stderr, "  -usenetfile filename\n");
}

/* Parses arguments and sets stuff up */
static void ParseArgs(
    int argc, char *argv[],
    char **user_return,
    char **groupsFile_return, char **usenetFile_return,
    char **host_return, int *port_return)
{
    int index = 1;
    *user_return = NULL;
    *groupsFile_return = NULL;
    *usenetFile_return = NULL;
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
		    *groupsFile_return = argv[index++];
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

	    /* user */
	    case 'u':
	    {
		if (strcmp(arg, "-user") == 0)
		{
		    *user_return = argv[index++];
		    break;
		}

		if (strcmp(arg, "-usenetfile") == 0)
		{
		    *usenetFile_return = argv[index++];
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
    if ((*groupsFile_return == NULL) || (*usenetFile_return == NULL))
    {
	char *dir;

	/* Try $TICKERDIR/groups */
	dir = getenv("TICKERDIR");
	if (dir != NULL)
	{
	    if (*groupsFile_return == NULL)
	    {
		*groupsFile_return = (char *)malloc(strlen(dir) + sizeof("/groups"));
		sprintf(*groupsFile_return, "%s/groups", dir);
	    }

	    if (*usenetFile_return == NULL)
	    {
		*usenetFile_return = (char *)malloc(strlen(dir) + sizeof("/usenet"));
		sprintf(*usenetFile_return, "%s/usenet", dir);
	    }

	    return;
	}

	/* No $TICKERDIR.  Try $HOME/.ticker/groups */
	dir = getenv("HOME");
	if (dir != NULL)
	{
	    if (*groupsFile_return == NULL)
	    {
		*groupsFile_return = (char *)malloc(strlen(dir) + sizeof("/.ticker/groups"));
		sprintf(*groupsFile_return, "%s/.ticker/groups", dir);
	    }

	    if (*usenetFile_return == NULL)
	    {
		*usenetFile_return = (char *)malloc(strlen(dir) + sizeof("/.ticker/usenet"));
		sprintf(*usenetFile_return ,"%s/.ticker/usenet", dir);
	    }
	}
    }
}


/* Create the icon window */
static Window CreateIcon(Widget shell)
{
    Display *display = XtDisplay(shell);
    Screen *screen = XtScreen(shell);
    Colormap colormap = XDefaultColormapOfScreen(screen);
    int depth = DefaultDepthOfScreen(screen);
    unsigned long black = BlackPixelOfScreen(screen);
    Window window;
    Pixmap pixmap, mask;
    XColor color;
    GC gc;
    XGCValues values;

    /* Create the actual icon window */
    window = XCreateSimpleWindow(
	display, RootWindowOfScreen(screen),
	0, 0, mask_width, mask_height, 0,
	CopyFromParent, CopyFromParent);

    /* Create a pixmap from the red bitmap data */
    color.red = 0xFFFF;
    color.green = 0x0000;
    color.blue = 0x0000;
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(display, colormap, &color);
    pixmap = XCreatePixmapFromBitmapData(
	display, window, red_bits, red_width, red_height,
	color.pixel, black, depth);

    /* Create a graphics context */
    values.function = GXor;
    gc = XCreateGC(display, pixmap, GCFunction, &values);

    /* Create a pixmap for the white 'e' and paint it on top */
    mask = XCreatePixmapFromBitmapData(
	display, pixmap, white_bits, white_width, white_height,
	WhitePixelOfScreen(screen), black, depth);
    XCopyArea(display, mask, pixmap, gc, 0, 0, white_width, white_height, 0, 0);
    XFreePixmap(display, mask);

    /* Create a shape mask and apply it to the window */
    mask = XCreatePixmapFromBitmapData(
	display, pixmap, mask_bits, mask_width, mask_height,
	0, 1, 1);
    XShapeCombineMask(display, window, 0, 0, 0, mask, X_ShapeCombine);

    /* Set the window's background to be the pixmap */
    XSetWindowBackgroundPixmap(display, window, pixmap);

    return window;
}


/* Parse args and go */
int main(int argc, char *argv[])
{
    XtAppContext context;
    Widget top;
    char *user;
    char *groupsFile;
    char *usenetFile;
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
    ParseArgs(argc, argv, &user, &groupsFile, &usenetFile, &host, &port);

    /* Create an Icon for the root shell */
    XtVaSetValues(top, XtNiconWindow, CreateIcon(top), NULL);

    /* Create a Tickertape */
    tickertape = Tickertape_alloc(user, groupsFile, usenetFile, host, port, top);

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

