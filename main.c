/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h> /* fprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* exit, getenv */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* strdup */
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h> /* getopt */
#endif
#ifdef HAVE_GETOPT_H
# include <getopt.h> /* getopt_long */
#endif
#ifdef HAVE_PWD_H
# include <pwd.h> /* getpwuid */
#endif
#ifdef HAVE_SIGNAL_H
# include <signal.h> /* signal */
#endif
#ifdef HAVE_NETDB_H
# include <netdb.h> /* gethostbyname */
#endif
#ifdef HAVE_SYS_UTSNAME_H
# include <sys/utsname.h> /* uname */
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif
#ifdef HAVE_VALGRIND_VALGRIND_H
# include <valgrind/valgrind.h>
#endif
#ifdef HAVE_VALGRIND_MEMCHECK_H
# include <valgrind/memcheck.h>
#endif
#include <X11/Xlib.h> /* X.* */
#include <X11/Intrinsic.h> /* Xt* */
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/Editres.h>
#include <Xm/XmStrDefs.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "globals.h"
#include "replace.h"
#include "message.h"
#include "tickertape.h"
#include "utils.h"

#include "red.xbm"
#include "white.xbm"
#include "mask.xbm"

#define DEFAULT_DOMAIN "no.domain"

/* The table of atoms to intern. */
struct atom_info {
    /* The index in the global atoms array. */
    atom_index_t index;

    /* The string corresponding to the atom's name. */
    const char *name;
};

#define DECLARE_ATOM(name) \
    { AN_##name, #name }
#define DECLARE_XmATOM(name) \
    { AN_##name, XmS##name }

Atom atoms[AN_MAX + 1];

/* The table of X11 atoms to intern. */
struct atom_info atom_list[AN_MAX + 1] = {
    DECLARE_ATOM(CHARSET_ENCODING),
    DECLARE_ATOM(CHARSET_REGISTRY),
    DECLARE_XmATOM(TARGETS),
    DECLARE_ATOM(UTF8_STRING),
    DECLARE_XmATOM(_MOTIF_CLIPBOARD_TARGETS)
};

#if defined(HAVE_GETOPT_LONG)
/* The list of long options */
static struct option long_options[] =
{
    { "elvin", required_argument, NULL, 'e' },
    { "scope", required_argument, NULL, 'S' },
    { "proxy", required_argument, NULL, 'H' },
    { "idle", required_argument, NULL, 'I' },
    { "user", required_argument, NULL, 'u' },
    { "domain", required_argument, NULL, 'D' },
    { "ticker-dir", required_argument, NULL, 'T' },
    { "groups", required_argument, NULL, 'G' },
    { "usenet", required_argument, NULL, 'U' },
    { "keys", required_argument, NULL, 'K' },
    { "keys-dir", required_argument, NULL, 'k' },
    { "version", no_argument, NULL, 'v' },
    { "help", no_argument, NULL, 'h' },
    { NULL, no_argument, NULL, '\0' }
};
#endif /* GETOPT_LONG */

#define OPTIONS "e:D:G:H:hI:K:k:S:T:u:U:v"

#if defined(HAVE_GETOPT_LONG)
/* Print out usage message */
static void
usage(int argc, char *argv[])
{
    fprintf(stderr,
            "usage: %s [OPTION]...\n"
            "  -e elvin-url,   --elvin=elvin-url\n"
            "  -S scope,       --scope=scope\n"
            "  -H http-proxy,  --proxy=http-proxy\n"
            "  -I idle-period, --idle=idle-period\n"
            "  -u user,        --user=user\n"
            "  -D domain,      --domain=domain\n"
            "  -T ticker-dir,  --ticker-dir=ticker-dir\n"
            "  -G groups-file, --groups=groups-file\n"
            "  -U usenet-file, --usenet=usenet-file\n"
            "  -K keys-file,   --keys=keys-file\n"
            "  -k keys-dir,    --keys-dir=keys-dir\n"
            "  -v,             --version\n"
            "  -h,             --help\n",
            argv[0]);
}
#else
/* Print out usage message */
static void
usage(int argc, char *argv[])
{
    fprintf(stderr,
            "usage: %s [OPTION]...\n"
            "  -e elvin-url\n"
            "  -S scope\n"
            "  -H http-proxy\n"
            "  -I idle-period\n"
            "  -u user\n"
            "  -D domain\n"
            "  -T ticker-dir\n"
            "  -G groups-file\n"
            "  -U usenet-file\n"
            "  -K keys-file\n"
            "  -k keys-dir\n"
            "  -v\n"
            "  -h\n",
            argv[0]);
}

#endif


#define XtNversionTag "versionTag"
#define XtCVersionTag "VersionTag"
#define XtNmetamail "metamail"
#define XtCMetamail "Metamail"
#define XtNsendHistoryCapacity "sendHistoryCapacity"
#define XtCSendHistoryCapacity "SendHistoryCapacity"

/* The application shell window also has resources */
#define offset(field) XtOffsetOf(XTickertapeRec, field)
static XtResource resources[] =
{
    /* char *version_tag */
    {
        XtNversionTag, XtCVersionTag, XtRString, sizeof(char *),
        offset(version_tag), XtRString, (XtPointer)NULL
    },

    /* Char *metamail */
    {
        XtNmetamail, XtCMetamail, XtRString, sizeof(char *),
        offset(metamail), XtRString, (XtPointer)NULL
    },

    /* Cardinal sendHistoryCapacity */
    {
        XtNsendHistoryCapacity, XtCSendHistoryCapacity, XtRInt, sizeof(int),
        offset(send_history_count), XtRImmediate, (XtPointer)8
    }
};
#undef offset

/* The Tickertape */
static tickertape_t tickertape;

#if defined(ELVIN_VERSION_AT_LEAST)
# if ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* The global elvin client information */
elvin_client_t client = NULL;
# else
#  error "Unsupported Elvin library version"
# endif
#endif


/* Callback for when the Window Manager wants to close a window */
static void
do_quit(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    tickertape_quit(tickertape);
}

/* Callback for going backwards through the history */
static void
do_history_prev(Widget widget,
                XEvent *event,
                String *params,
                Cardinal *nparams)
{
    tickertape_history_prev(tickertape);
}

/* Callback for going forwards through the history */
static void
do_history_next(Widget widget,
                XEvent *event,
                String *params,
                Cardinal *nparams)
{
    tickertape_history_next(tickertape);
}

/* The default application actions table */
static XtActionsRec actions[] =
{
    { "history-prev", do_history_prev },
    { "history-next", do_history_next },
    { "quit", do_quit }
};

/* Returns the name of the user who started this program */
static const char *
get_user()
{
    const char *user;

    /* First try the `USER' environment variable */
    user = getenv("USER");
    if (user != NULL) {
        return user;
    }

    /* If that isn't set try `LOGNAME' */
    user = getenv("LOGNAME");
    if (user != NULL) {
        return user;
    }

    /* Go straight to the source for an unequivocal answer */
    return getpwuid(getuid())->pw_name;
}

/* Looks up the domain name of the host */
static const char *
get_domain()
{
    const char *domain;
#ifdef HAVE_UNAME
    struct utsname name;
# ifdef HAVE_GETHOSTBYNAME
    struct hostent *host;
# endif /* GETHOSTBYNAME */
    const char *point;
    int ch;
#endif /* UNAME */

    /* If the `DOMAIN' environment variable is set then use it */
    domain = getenv("DOMAIN");
    if (domain != NULL) {
        return strdup(domain);
    }

#ifdef HAVE_UNAME
    /* Look up the node name */
    if (uname(&name) < 0) {
        return strdup(DEFAULT_DOMAIN);
    }

# ifdef HAVE_GETHOSTBYNAME
    /* Use that to get the canonical name */
    host = gethostbyname(name.nodename);
    if (host == NULL) {
        domain = strdup(name.nodename);
    } else {
        domain = strdup(host->h_name);
    }
# else /* GETHOSTBYNAME */
    domain = name.nodename;
# endif /* GETHOSTBYNAME */

    /* Strip everything up to and including the first `.' */
    point = domain;
    while ((ch = *(point++)) != 0) {
        if (ch == '.') {
            return strdup(point);
        }
    }

    /* No dots; just use what we have */
    return strdup(domain);
#else /* UNAME */
    return strdup(DEFAULT_DOMAIN);
#endif /* UNAME */
}

/* Parses arguments and sets stuff up */
static void
parse_args(int argc,
           char *argv[],
           elvin_handle_t handle,
           const char **user_return,
           const char **domain_return,
           const char **ticker_dir_return,
           const char **config_file_return,
           const char **groups_file_return,
           const char **usenet_file_return,
           const char **keys_file_return,
           const char **keys_dir_return,
           elvin_error_t error)
{
    const char *http_proxy = NULL;

    /* Initialize arguments to sane values */
    *user_return = NULL;
    *domain_return = NULL;
    *ticker_dir_return = NULL;
    *config_file_return = NULL;
    *groups_file_return = NULL;
    *keys_file_return = NULL;
    *keys_dir_return = NULL;
    *usenet_file_return = NULL;

    /* Read each argument using getopt */
    for (;;) {
        int choice;

#if defined(HAVE_GETOPT_LONG)
        choice = getopt_long(argc, argv, OPTIONS, long_options, NULL);
#else
        choice = getopt(argc, argv, OPTIONS);
#endif
        /* End of options? */
        if (choice < 0) {
            break;
        }

        /* Which option was it then? */
        switch (choice) {
        case 'e':
            /* --elvin= or -e */
            if (elvin_handle_append_url(handle, optarg, error) == 0) {
                fprintf(stderr, PACKAGE ":bad URL: no doughnut \"%s\"\n",
                        optarg);
                elvin_error_fprintf(stderr, error);
                exit(1);
            }

            break;

        case 'S':
            /* --scope= or -S */
            if (!elvin_handle_set_discovery_scope(handle, optarg, error)) {
                fprintf(stderr, PACKAGE ": unable to set scope to %s\n",
                        optarg);
                elvin_error_fprintf(stderr, error);
                exit(1);
            }

            break;

        case 'H':
            /* --proxy= or -H */
            http_proxy = optarg;
            break;

        case 'I':
            /* --idle= or -I */
            if (!elvin_handle_set_idle_period(handle, atoi(optarg), error)) {
                fprintf(stderr, PACKAGE ": unable to set idle period to %s\n",
                        optarg);
                elvin_error_fprintf(stderr, error);
                exit(1);
            }

            break;

        case 'u':
            /* --user= or -u */
            *user_return = optarg;
            break;

        case 'D':
            /* --domain= or -D */
            *domain_return = strdup(optarg);
            break;


        case 'T':
            /* --ticker-dir= or -T */
            *ticker_dir_return = optarg;
            break;

        case 'c':
            /* --config= or -c */
            *config_file_return = optarg;
            break;

        case 'G':
            /* --groups= or -G */
            *groups_file_return = optarg;
            break;

        case 'U':
            /* --usenet= or -U */
            *usenet_file_return = optarg;
            break;

        case 'K':
            /* --keys= or -K */
            *keys_file_return = optarg;
            break;

        case 'k':
            /* --keys-dir= or -k */
            *keys_dir_return = optarg;
            break;

        case 'v':
            /* --version or -v */
            printf(PACKAGE " version " VERSION "\n");
            exit(0);

        case 'h':
            /* --help or -h */
            usage(argc, argv);
            exit(0);

        default:
            /* Unknown option */
            usage(argc, argv);
            exit(1);
        }
    }

    /* Generate a user name if none provided */
    if (*user_return == NULL) {
        *user_return = get_user();
    }

    /* Generate a domain name if none provided */
    if (*domain_return == NULL) {
        *domain_return = get_domain();
    }

    /* If we now have a proxy, then set its property */
    if (http_proxy != NULL) {
        elvin_handle_set_string_property(handle, "http.proxy", http_proxy, NULL);
    }

    return;
}

/* Create the icon window */
static Window
create_icon(Widget shell)
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
    XCopyArea(display, mask, pixmap, gc, 0, 0,
              white_width, white_height, 0, 0);
    XFreePixmap(display, mask);
    XFreeGC(display, gc);

#ifdef HAVE_LIBXEXT
    /* Create a shape mask and apply it to the window */
    mask = XCreateBitmapFromData(display, pixmap, (char *)mask_bits,
                                 mask_width, mask_height);
    XShapeCombineMask(display, window, ShapeBounding, 0, 0, mask, ShapeSet);
#endif /* HAVE_LIBXEXT */

    /* Set the window's background to be the pixmap */
    XSetWindowBackgroundPixmap(display, window, pixmap);
    return window;
}

/* Signal handler which causes xtickertape to reload its subscriptions */
static RETSIGTYPE
reload_subs(int signum)
{
    /* Put the signal handler back in place */
    signal(signum, reload_subs);

    /* Reload configuration files */
    tickertape_reload_all(tickertape);
}

#ifdef USE_VALGRIND
/* Signal handler which invokes valgrind magic. */
static RETSIGTYPE
count_leaks(int signum)
{
    unsigned long leaked, dubious, reachable, suppressed;

    /* Put the signal handler back in place */
    signal(signum, count_leaks);

    /* Get valgrind's current leak counts. */
    VALGRIND_COUNT_LEAKS(leaked, dubious, reachable, suppressed);
    fprintf(stderr, "%s: valgrind: leaked=%lu, dubious=%lu, reachable=%lu, "
            "suppressed=%lu\n", PACKAGE, leaked, dubious, reachable,
            suppressed);
}
#endif /* USE_VALGRIND */

/* Print an error message indicating that the app-defaults file is bogus */
static void
app_defaults_version_error(const char *message)
{
    fprintf(stderr,
            "%s: %s\n\n"
            "This probably because the app-defaults file (XTickertape) "
            "is not installed in\n"
            "the X11 directory tree.  This may be fixed either by "
            "moving the app-defaults\n"
            "file to the correct location or by setting your "
            "XFILESEARCHPATH environment\n"
            "variable to something like /usr/local/lib/X11/%%T/%%N:%%D.  "
            "See the man page for\n"
            "XtResolvePathname for more information.\n", PACKAGE, message);
}

/* Parse args and go */
int
main(int argc, char *argv[])
{
    XtAppContext context;

#ifndef HAVE_XTVAOPENAPPLICATION
    Display *display;
#endif
    XTickertapeRec rc;
    elvin_handle_t handle;
    elvin_error_t error;
    const char *user;
    const char *domain;
    const char *ticker_dir;
    const char *config_file;
    const char *groups_file;
    const char *usenet_file;
    const char *keys_file;
    const char *keys_dir;
    Widget top;
    const char *names[AN_MAX + 1];
    int i;

#ifdef HAVE_XTVAOPENAPPLICATION
    /* Create the toplevel widget */
    top = XtVaOpenApplication(
        &context, "XTickertape",
        NULL, 0,
        &argc, argv, NULL,
        applicationShellWidgetClass,
        XtNborderWidth, 0,
        NULL);
#else
    /* Initialize the X Toolkit */
    XtToolkitInitialize();

    /* Create an application context */
    context = XtCreateApplicationContext();

    /* Open the display */
    display = XtOpenDisplay(context, NULL, NULL, "XTickertape",
                            NULL, 0, &argc, argv);
    if (display == NULL) {
        fprintf(stderr, "Error: Can't open display\n");
        exit(1);
    }

    /* Create the toplevel widget */
    top = XtAppCreateShell(NULL, "XTickertape", applicationShellWidgetClass,
                           display, NULL, 0);
#endif

    /* Load the application shell resources */
    XtGetApplicationResources(top, &rc, resources, XtNumber(resources),
                              NULL, 0);

    /* Make sure our app-defaults file has a version number */
    if (rc.version_tag == NULL) {
        app_defaults_version_error("app-defaults file not found or "
                                   "out of date");
        exit(1);
    }

    /* Make sure that version number is the one we want */
    if (strcmp(rc.version_tag, PACKAGE "-" VERSION) != 0) {
        app_defaults_version_error("app-defaults file has the wrong "
                                   "version number");
        exit(1);
    }

    /* Add a calback for when it gets destroyed */
    XtAppAddActions(context, actions, XtNumber(actions));

#if !defined(ELVIN_VERSION_AT_LEAST)
    /* Initialize the elvin client library */
    error = elvin_xt_init(context);
    if (error == NULL) {
        fprintf(stderr, "*** elvin_xt_init(): failed\n");
        exit(1);
    }

    /* Double-check that the initialization worked */
    if (elvin_error_is_error(error)) {
        fprintf(stderr, "*** elvin_xt_init(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Create a new elvin connection handle */
    handle = elvin_handle_alloc(error);
    if (handle == NULL) {
        fprintf(stderr, PACKAGE ": elvin_handle_alloc() failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
    /* Allocate an error context */
    error = elvin_error_alloc(NULL, NULL);
    if (error == NULL) {
        fprintf(stderr, PACKAGE ": elvin_error_alloc() failed\n");
        exit(1);
    }

    /* Initialize the elvin client library */
    client = elvin_xt_init_default(context, error);
    if (client == NULL) {
        fprintf(stderr, PACKAGE ": elvin_xt_init() failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Create a new elvin connection handle */
    handle = elvin_handle_alloc(client, error);
    if (handle == NULL) {
        fprintf(stderr, PACKAGE ": elvin_handle_alloc() failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }
#else /* ELVIN_VERSION_AT_LEAST */
# error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */

    /* Scan what's left of the arguments */
    parse_args(argc, argv, handle, &user, &domain,
               &ticker_dir, &config_file,
               &groups_file, &usenet_file,
               &keys_file, &keys_dir,
               error);

    /* Intern a bunch of atoms.  We jump through a few hoops in order
     * in order to do this in a single RPC to the X server. */
#ifdef USE_ASSERT
    memset(names, 0, sizeof(names));
#endif /* USE_ASSERT */
    for (i = 0; i <= AN_MAX; i++) {
        ASSERT(names[atom_list[i].index] == NULL);
        names[atom_list[i].index] = atom_list[i].name;
    }

    /* Make sure we've specified a name for each atom. */
    for (i = 0; i <= AN_MAX; i++) {
        ASSERT(names[i] != NULL);
    }

    /* Intern the atoms. */
    if (!XInternAtoms(XtDisplay(top), (char **)names, AN_MAX + 1,
                      False, atoms)) {
        fprintf(stderr, PACKAGE ": XInternAtoms failed\n");
        exit(1);
    }

    /* Create an Icon for the root shell */
    XtVaSetValues(top, XtNiconWindow, create_icon(top), NULL);

    /* Create a tickertape */
    tickertape = tickertape_alloc(
        &rc, handle,
        user, domain,
        ticker_dir, config_file,
        groups_file, usenet_file,
        keys_file, keys_dir,
        top,
        error);

    /* Set up SIGHUP to reload the subscriptions */
    signal(SIGHUP, reload_subs);

#ifdef USE_VALGRIND
    /* Set up SIGUSR1 to invoke valgrind. */
    signal(SIGUSR1, count_leaks);
#endif /* USE_VALGRIND */

#ifdef HAVE_LIBXMU
    /* Enable editres support */
    XtAddEventHandler(top, (EventMask)0, True, _XEditResCheckMessages, NULL);
#endif /* HAVE_LIBXMU */

    /* Let 'er rip! */
    XtAppMainLoop(context);

    /* Keep the compiler happy */
    return 0;
}

/**********************************************************************/
