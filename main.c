/* $Id: main.c,v 1.40 1998/12/23 11:33:27 phelps Exp $ */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif /* HAVE_GETOPT_H */
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/Editres.h>
#include "Tickertape.h"

#include "red.xbm"
#include "white.xbm"
#include "mask.xbm"

#define HOST "elvin"
#define PORT 5678

/* The list of long options */
#ifdef HAVE_LIBIBERTY
static struct option long_options[] =
{
    { "host", required_argument, NULL, 'h' },
    { "port", required_argument, NULL, 'p' },
    { "user", required_argument, NULL, 'u' },
    { "groups", required_argument, NULL, 'g' },
    { "news", required_argument, NULL, 'n' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'e' },
    { NULL, no_argument, NULL, '\0' }
};
#endif /* HAVE_LIBIBERTY */


/* Static function headers */
static void NotifyAction(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);
static void QuitAction(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);
static void Usage(int argc, char *argv[]);
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
static void Usage(int argc, char *argv[])
{
    fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
    fprintf(stderr, "  -h hostname, --host=hostname\n");
    fprintf(stderr, "  -p port,     --port=port\n");
    fprintf(stderr, "  -u username, --user=username\n");
    fprintf(stderr, "  -g filename, --groups=filename\n");
    fprintf(stderr, "  -n filename, --news=filename\n");
    fprintf(stderr, "  -v,          --version\n");
    fprintf(stderr, "               --help\n");
}

/* Parses arguments and sets stuff up */
static void ParseArgs(
    int argc, char *argv[],
    char **user_return,
    char **groupsFile_return, char **usenetFile_return,
    char **host_return, int *port_return)
{
    int choice;

    /* Initialize arguments to sane values */
    *user_return = NULL;
    *groupsFile_return = NULL;
    *usenetFile_return = NULL;
    *host_return = HOST;
    *port_return = PORT;

    /* Read each argument using getopt */
#ifdef HAVE_LIBIBERTY
    while ((choice = getopt(argc, argv, "h:p:u:g:n:v")) > 0)
#else
    while ((choice = getopt_long(argc, argv, "h:p:u:g:n:v", long_options, NULL)) > 0)
#endif /* HAVE_LIBIBERTY */
    {
	switch (choice)
	{
	    /* --host= or -h */
	    case 'h':
	    {
		*host_return = optarg;
		printf("host=%s\n", *host_return);
		break;
	    }

	    /* --port= or -p */
	    case 'p':
	    {
		*port_return = atoi(optarg);
		printf("port=%d\n", *port_return);
		break;
	    }

	    /* --user= or -u */
	    case 'u':
	    {
		*user_return = optarg;
		printf("user=%s\n", *user_return);
		break;
	    }

	    /* --groups= or -g */
	    case 'g':
	    {
		*groupsFile_return = optarg;
		break;
	    }

	    /* --news= or -n */
	    case 'n':
	    {
		*usenetFile_return = optarg;
		break;
	    }

	    /* --version or -v */
	    case 'v':
	    {
		printf("%s version %s\n", PACKAGE, VERSION);
		exit(0);
	    }

	    /* --help */
	    case 'e':
	    {
		Usage(argc, argv);
		exit(0);
	    }

	    /* Unknown option */
	    case '?':
	    {
		Usage(argc, argv);
		exit(1);
	    }
	}
    }

    /* Set the user name if not specified on the command line */
    if (*user_return == NULL)
    {
	*user_return = getpwuid(getuid()) -> pw_name;
    }

    /* Set the groups/news filenames if not specified on the command line */
    if ((*groupsFile_return == NULL) || (*usenetFile_return == NULL))
    {
	char *home;
	char *ticker;

	/* Try $TICKERDIR */
	if ((ticker = getenv("TICKERDIR")) == NULL)
	{
	    /* No luck.  Try $HOME/.ticker */
	    if ((home = getenv("HOME")) == NULL)
	    {
		/* Eek!  $HOME isn't set.  Use "/" */
		home = "/";
	    }

	    /* ticker then become $HOME/.ticker */
	    ticker = malloc(strlen(home) + sizeof("/.ticker"));
	    sprintf(ticker, "%s/.ticker");
	}

	/* Set the groups filename if appropriate */
	if (*groupsFile_return == NULL)
	{
	    *groupsFile_return = (char *)malloc(strlen(ticker) + sizeof("/groups"));
		sprintf(*groupsFile_return, "%s/groups", ticker);
	}

	/* Set the news filename if appropriate */
	if (*usenetFile_return == NULL)
	{
	    *usenetFile_return = (char *)malloc(strlen(ticker) + sizeof("/usenet"));
	    sprintf(*usenetFile_return, "%s/usenet", ticker);
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
#ifdef HAVE_LIBXEXT
    mask = XCreatePixmapFromBitmapData(
	display, pixmap, mask_bits, mask_width, mask_height,
	0, 1, 1);
    XShapeCombineMask(display, window, 0, 0, 0, mask, X_ShapeCombine);
#endif /* HAVE_LIBEXT */

    /* Set the window's background to be the pixmap */
    XSetWindowBackgroundPixmap(display, window, pixmap);

    return window;
}


#ifndef HAVE_STRDUP
/* Define strdup if it's not in the standard C library */
char *strdup(char *string)
{
    char *result;

    /* Allocate some heap memory for the copy of the string */
    if ((result = (char *)malloc(strlen(string) + 1)) == NULL)
    {
	return NULL;
    }

    /* Copy the characters */
    strcpy(result, string);
    return result;
}
#endif /* HAVE_STRDUP */

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

    /* Enable editres support */
#ifdef HAVE_LIBXMU
    XtAddEventHandler(top, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif /* HAVE_LIBXMU */

    /* Let 'er rip! */
    XtAppMainLoop(context);

    /* Keep the compiler happy */
    return 0;
}

