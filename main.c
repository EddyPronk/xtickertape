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
static const char cvsid[] = "$Id: main.c,v 1.61 1999/11/04 03:36:21 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <getopt.h>
#include <pwd.h>
#include <signal.h>
#include <netdb.h>
#include <sys/utsname.h>
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
#define DEFAULT_DOMAIN "no.domain"

/* The list of long options */
static struct option long_options[] =
{
    { "host", required_argument, NULL, 'h' },
    { "port", required_argument, NULL, 'p' },
    { "user", required_argument, NULL, 'u' },
    { "domain" , required_argument, NULL, 'D' },
    { "groups", required_argument, NULL, 'G' },
    { "usenet", required_argument, NULL, 'U' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'z' },
    { NULL, no_argument, NULL, '\0' }
};

/* Static function headers */
static void do_quit(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);
static void usage(int argc, char *argv[]);
static void parse_args(
    int argc, char *argv[],
    char **user_return, char **domain_return,
    char **ticker_dir_return,
    char **groups_file_return, char **usenet_file_return,
    char **host_return, int *port_return);
static Window create_icon(Widget shell);
static void reload_subs(int signum);


/* The Tickertape */
static tickertape_t tickertape;

/* The default application actions table */
static XtActionsRec actions[] =
{
    {"quit", do_quit}
};

/* Callback for when the Window Manager wants to close a window */
static void do_quit(Widget widget, XEvent *event, String *params, Cardinal *cparams)
{
    tickertape_quit(tickertape);
}

/* Print out usage message */
static void usage(int argc, char *argv[])
{
    fprintf(stderr, "usage: %s [OPTION]...\n", argv[0]);
    fprintf(stderr, "  -h host,     --host=host\n");
    fprintf(stderr, "  -p port,     --port=port\n");
    fprintf(stderr, "  -u username, --user=username\n");
    fprintf(stderr, "  -G filename, --groups=filename\n");
    fprintf(stderr, "  -U filename, --usenet=filename\n");
    fprintf(stderr, "  -v,          --version\n");
    fprintf(stderr, "               --help\n");
}

/* Returns the name of the user who started this program */
static char *get_user()
{
    char *user;

    /* First try the `USER' environment variable */
    if ((user = getenv("USER")) != NULL)
    {
	return user;
    }

    /* If that isn't set try `LOGNAME' */
    if ((user = getenv("LOGNAME")) != NULL)
    {
	return user;
    }

    /* Go straight to the source for an unequivocal answer */
    return getpwuid(getuid()) -> pw_name;
}

/* Looks up the domain name of the host */
static char *get_domain()
{
    struct utsname name;
    struct hostent *host;
    char *domain;

    /* If the `DOMAIN' environment variable is set then use it */
    if ((domain = getenv("DOMAIN")) != NULL)
    {
	return domain;
    }

    /* Otherwise look up the node name */
    if (! (uname(&name) < 0))
    {
	/* Use that to get the host entry */
	host = gethostbyname(name.nodename);

	/* Strip everything up to and including the first `.' */
	for (domain = host -> h_name; *domain != '\0'; domain++)
	{
	    if (*domain == '.')
	    {
		return domain + 1;
	    }
	}
    }

    /* No luck.  Resort to a default domain */
    return DEFAULT_DOMAIN;
}

/* Parses arguments and sets stuff up */
static void parse_args(
    int argc, char *argv[],
    char **user_return, char **domain_return,
    char **ticker_dir_return,
    char **groups_file_return, char **usenet_file_return,
    char **host_return, int *port_return)
{
    int choice;

    /* Initialize arguments to sane values */
    *user_return = NULL;
    *domain_return = NULL;
    *ticker_dir_return = NULL;
    *groups_file_return = NULL;
    *usenet_file_return = NULL;
    *host_return = NULL;
    *port_return = 0;

    /* Read each argument using getopt */
    while ((choice = getopt_long(argc, argv, "h:p:u:D:U:G:v", long_options, NULL)) > 0)
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

	    /* --domain= or -D */
	    case 'D':
	    {
		*domain_return = optarg;
		break;
	    }

	    /* --groups= or -G */
	    case 'G':
	    {
		*groups_file_return = optarg;
		break;
	    }

	    /* --usenet= or -U */
	    case 'U':
	    {
		*usenet_file_return = optarg;
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
		usage(argc, argv);
		exit(0);
	    }

	    /* Unknown option */
	    default:
	    {
		usage(argc, argv);
		exit(1);
	    }
	}
    }

    /* Generate a user name if none provided */
    if (*user_return == NULL)
    {
	*user_return = get_user();
    }

    /* Generate a domain name if none provided */
    if (*domain_return == NULL)
    {
	*domain_return = get_domain();
    }

    /* Generate a host name if none provided */
    if (*host_return == NULL)
    {
	/* Try the environment variable */
	if ((*host_return = getenv("ELVIN_HOST")) == NULL)
	{
	    *host_return = HOST;
	}
    }

    /* Generate a port number if none provided */
    if (*port_return == 0)
    {
	char *port;

	/* Try the environment variable */
	if ((port = getenv("ELVIN_PORT")) != NULL)
	{
	    *port_return = atoi(port);
	}

	/* Otherwise use the default */
	if (*port_return == 0)
	{
	    *port_return = PORT;
	}
    }
}


/* Create the icon window */
static Window create_icon(Widget shell)
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

/* Signal handler which causes xtickertape to reload its subscriptions */
static void reload_subs(int signum)
{
    /* Put the signal handler back in place */
    signal(SIGHUP, reload_subs);

    /* Reload groups and usenet files */
    tickertape_reload_groups(tickertape);
    tickertape_reload_usenet(tickertape);
}

#ifndef HAVE_STRDUP
/* Define strdup if it's not in the standard C library */
char *strdup(const char *string)
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
    char *domain;
    char *ticker_dir;
    char *groups_file;
    char *usenet_file;
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
    parse_args(argc, argv, &user, &domain, &ticker_dir, &groups_file, &usenet_file, &host, &port);

    /* Create an Icon for the root shell */
    XtVaSetValues(top, XtNiconWindow, create_icon(top), NULL);

    /* Create a Tickertape */
    tickertape = tickertape_alloc(user, domain, ticker_dir, groups_file, usenet_file, host, port, top);

    /* Set up SIGHUP to reload the subscriptions */
    signal(SIGHUP, reload_subs);

#ifdef HAVE_LIBXMU
    /* Enable editres support */
    XtAddEventHandler(top, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif /* HAVE_LIBXMU */

    /* Let 'er rip! */
    XtAppMainLoop(context);

    /* Keep the compiler happy */
    return 0;
}

