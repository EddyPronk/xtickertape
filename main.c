/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: main.c,v 1.52 1999/08/19 01:46:10 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <getopt.h>
#include <pwd.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/Editres.h>
#include "tickertape.h"

#include "red.xbm"
#include "white.xbm"
#include "mask.xbm"

#define HOST "elvin"
#define PORT 5678

/* The list of long options */
static struct option long_options[] =
{
    { "host", required_argument, NULL, 'h' },
    { "port", required_argument, NULL, 'p' },
    { "user", required_argument, NULL, 'u' },
    { "groups", required_argument, NULL, 'g' },
    { "news", required_argument, NULL, 'n' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'z' },
    { NULL, no_argument, NULL, '\0' }
};

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
    char **user_return, char **tickerDir_return,
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
    fprintf(stderr, "  -h host,     --host=host\n");
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
    char **user_return, char **tickerDir_return,
    char **groupsFile_return, char **usenetFile_return,
    char **host_return, int *port_return)
{
    int choice;

    /* Initialize arguments to sane values */
    *user_return = NULL;
    *tickerDir_return = NULL;
    *groupsFile_return = NULL;
    *usenetFile_return = NULL;
    *host_return = HOST;
    *port_return = PORT;

    /* Read each argument using getopt */
    while ((choice = getopt_long(argc, argv, "h:p:u:g:n:v", long_options, NULL)) > 0)
    {
	switch (choice)
	{
	    /* --host= or -h */
	    case 'h':
	    {
		*host_return = optarg;
		break;
	    }

	    /* --port= or -p */
	    case 'p':
	    {
		*port_return = atoi(optarg);
		break;
	    }

	    /* --user= or -u */
	    case 'u':
	    {
		*user_return = optarg;
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
	    case 'z':
	    {
		Usage(argc, argv);
		exit(0);
	    }

	    /* Unknown option */
	    default:
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
    char *tickerDir;
    char *groupsFile;
    char *usenetFile;
    char *host;
    int port;

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "Tickertape",
	NULL, 0, &argc, argv, NULL,
	XtNborderWidth, 0,
	NULL);

    /* Add a calback for when it gets destroyed */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Scan what's left of the arguments */
    ParseArgs(argc, argv, &user, &tickerDir, &groupsFile, &usenetFile, &host, &port);

    /* Create an Icon for the root shell */
    XtVaSetValues(top, XtNiconWindow, CreateIcon(top), NULL);

    /* Create a Tickertape */
    tickertape = Tickertape_alloc(user, tickerDir, groupsFile, usenetFile, host, port, top);

#ifdef HAVE_LIBXMU
    /* Enable editres support */
    XtAddEventHandler(top, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif /* HAVE_LIBXMU */

    /* Let 'er rip! */
    XtAppMainLoop(context);

    /* Keep the compiler happy */
    return 0;
}

