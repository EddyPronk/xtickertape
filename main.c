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
static const char cvsid[] = "$Id: main.c,v 1.82 2000/07/06 00:24:48 phelps Exp $";
#endif /* lint */

#include <config.h>
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
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "tickertape.h"

#include "red.xbm"
#include "white.xbm"
#include "mask.xbm"

#define DEFAULT_DOMAIN "no.domain"
#define DEFAULT_SCOPE "DEFAULT"

/* The list of long options */
static struct option long_options[] =
{
    { "elvin", required_argument, NULL, 'e' },
    { "scope", required_argument, NULL, 'S' },
    { "port", required_argument, NULL, 'p' },
    { "user", required_argument, NULL, 'u' },
    { "domain" , required_argument, NULL, 'D' },
    { "groups", required_argument, NULL, 'G' },
    { "usenet", required_argument, NULL, 'U' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, '\0' }
};

/* Static function headers */
static void do_quit(
    Widget widget, XEvent *event,
    String *params, Cardinal *cparams);

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
    fprintf(stderr,
	"usage: %s [OPTION]...\n"
	"  -e elvin-url,   --elvin=elvin-url\n"
	"  -S scope,       --scope=scope\n"
	"  -u user,        --user=user\n"
	"  -D domain,      --domain=domain\n"
	"  -G groups-file, --groups=groups-file\n"
	"  -U usenet-file, --usenet=usenet-file\n"
	"  -v,             --version\n"
	"  -h,             --help\n" ,
	argv[0]);
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
    elvin_handle_t handle,
    char **user_return, char **domain_return,
    char **ticker_dir_return,
    char **groups_file_return, char **usenet_file_return,
    elvin_error_t error)
{
    char *url;
    int choice;

    /* Initialize arguments to sane values */
    handle -> scope = DEFAULT_SCOPE;
    *user_return = NULL;
    *domain_return = NULL;
    *ticker_dir_return = NULL;
    *groups_file_return = NULL;
    *usenet_file_return = NULL;

    /* Read each argument using getopt */
    while ((choice = getopt_long(
	argc, argv,
	"e:S:u:D:U:G:vh", long_options,
	NULL)) > 0)
    {
	switch (choice)
	{
	    /* --elvin= or -e */
	    case 'e':
	    {
		if (elvin_handle_insert_server(handle, -1, optarg, error) == 0)
		{
		    fprintf(stderr, "Bad URL: no doughnut \"%s\"\n", optarg);
		    exit(1);
		}

		break;
	    }

	    /* --scope= or -S */
	    case 'S':
	    {
		handle -> scope = optarg;
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

	    /* --help or -h */
	    case 'h':
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

    /* If the ELVIN_URL environment variable is set then add it to the list */
    if ((url = getenv("ELVIN_URL")) != NULL && *url != '\0')
    {
	if (elvin_handle_insert_server(handle, 0, url, error) == 0)
	{
	    fprintf(stderr, "Bad URL: no doughnut \"%s\"\n", url);
	    exit(1);
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

    /* Allocate the color red by name */
    XAllocNamedColor(display, colormap, "red", &color, &color);

    /* Create a pixmap from the red bitmap data */
    pixmap = XCreatePixmapFromBitmapData(
	display, window, (char *)red_bits, red_width, red_height,
	color.pixel, black, depth);

    /* Create a graphics context */
    values.function = GXxor;
    gc = XCreateGC(display, pixmap, GCFunction, &values);

    /* Create a pixmap for the white 'e' and paint it on top */
    mask = XCreatePixmapFromBitmapData(
	display, pixmap, (char *)white_bits, white_width, white_height,
	WhitePixelOfScreen(screen) ^ black, 0, depth);
    XCopyArea(display, mask, pixmap, gc, 0, 0, white_width, white_height, 0, 0);
    XFreePixmap(display, mask);
    XFreeGC(display, gc);

#ifdef HAVE_LIBXEXT
    /* Create a shape mask and apply it to the window */
    mask = XCreateBitmapFromData(display, pixmap, (char *)mask_bits, mask_width, mask_height);
    XShapeCombineMask(display, window, ShapeBounding, 0, 0, mask, ShapeSet);
#endif /* HAVE_LIBXEXT */

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
    elvin_handle_t handle;
    elvin_error_t error;
    char *user;
    char *domain;
    char *ticker_dir;
    char *groups_file;
    char *usenet_file;
    Widget top;

    /* Create the toplevel widget */
    top = XtVaAppInitialize(
	&context, "XTickertape",
	NULL, 0, &argc, argv, NULL,
	XtNborderWidth, 0,
	NULL);

    /* Add a calback for when it gets destroyed */
    XtAppAddActions(context, actions, XtNumber(actions));

    /* Initialize the elvin client library */
    if ((error = elvin_xt_init("en", context)) == NULL)
    {
	fprintf(stderr, "*** elvin_xt_init(): failed\n");
	abort();
    }

    /* Create a new elvin connection handle */
    if ((handle = elvin_handle_alloc(error)) == NULL)
    {
	fprintf(stderr, "*** elvin_handle_alloc(): failed\n");
	abort();
    }

    /* Scan what's left of the arguments */
    parse_args(argc, argv, handle, &user, &domain, &ticker_dir, &groups_file, &usenet_file, error);

    /* Create an Icon for the root shell */
    XtVaSetValues(top, XtNiconWindow, create_icon(top), NULL);

    /* Create a tickertape */
    tickertape = tickertape_alloc(
	handle,
	user, domain,
	ticker_dir, groups_file, usenet_file,
	top,
	error);

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

