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
static const char cvsid[] = "$Id: tickertape.c,v 1.26 1999/10/04 11:30:20 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <X11/Intrinsic.h>
#include <sys/utsname.h>
#include "sanity.h"
#include "message.h"
#include "history.h"
#include "tickertape.h"
#include "Hash.h"
#include "Scroller.h"
#include "Control.h"
#include "groups.h"
#include "groups_parser.h"
#include "group_sub.h"
#include "usenet.h"
#include "usenet_parser.h"
#include "usenet_sub.h"
#include "mail_sub.h"
#include "connect.h"
#ifdef ORBIT
#include "orbit_sub.h"
#endif /* ORBIT */

#ifdef SANITY
static char *sanity_value = "Ticker";
static char *sanity_freed = "Freed Ticker";
#endif /* SANITY */

#define DEFAULT_TICKERDIR ".ticker"
#define DEFAULT_GROUPS_FILE "groups"
#define DEFAULT_USENET_FILE "usenet"
#define DEFAULT_DOMAIN "no.domain.name"

#define METAMAIL_OPTIONS "-B -q -b -c"

#define BUFFER_SIZE 1024

#define RECONNECT_MSG "Connected to elvin server at %s:%d."
#define NO_CONNECT_MSG "Unable to connect to elvin server at %s:%d.  Still trying..."
#define LOST_CONNECT_MSG \
"Lost connection to elvin server at %s:%d.  Attempting to reconnect..."

#define ORBIT_SUB "exists(orbit.view_update) && exists(tickertape) && user == \"%s\""

/* The tickertape data type */
struct tickertape
{
    /* The user's name */
    char *user;

    /* The local domain name */
    char *domain;

    /* The receiver's ticker directory */
    char *tickerDir;

    /* The group file from which we read our subscriptions */
    char *groups_file;

    /* The usenet file from which we read our usenet subscription */
    char *usenet_file;

    /* The top-level widget */
    Widget top;

    /* The receiver's groups subscriptions (from the groups file) */
    group_sub_t *groups;

    /* The number of groups subscriptions the receiver has */
    int groups_count;

    /* The receiver's usenet subscription (from the usenet file) */
    usenet_sub_t usenet_sub;

    /* The receiver's mail subscription */
    mail_sub_t mail_sub;

#ifdef ORBIT
    /* The receiver's Orbit-related subscriptions */
    Hashtable orbitSubscriptionsById;
#endif /* ORBIT */

    /* The elvin connection */
    connection_t connection;

    /* The control panel */
    ControlPanel control_panel;

    /* The ScrollerWidget */
    ScrollerWidget scroller;

    /* The history */
    history_t history;
};


/*
 *
 * Static function headers
 *
 */
static int mkdirhier(char *dirname);
static void publish_startup_notification(tickertape_t self);
static void menu_callback(Widget widget, tickertape_t self, message_t message);
static void receive_callback(tickertape_t self, message_t message);
static int parse_groups_file(tickertape_t self);
static void init_ui(tickertape_t self);
static void disconnect_callback(tickertape_t self, connection_t connection);
static void reconnect_callback(tickertape_t self, connection_t connection);
#ifdef ORBIT
static void orbit_callback(tickertape_t self, en_notify_t notification);
static void subscribe_to_orbit(tickertape_t self);
#endif /* ORBIT */
static char *tickertape_ticker_dir(tickertape_t self);
static char *tickertape_groups_filename(tickertape_t self);
static char *tickertape_usenet_filename(tickertape_t self);


/*
 *
 * Static functions
 *
 */

/* Looks up the domain name of the host */
static char *get_domain()
{
    struct utsname name;
    struct hostent *host;
    char *pointer;

    /* Grab the node name */
    if (uname(&name) < 0)
    {
	return DEFAULT_DOMAIN;
    }

    /* Look up the host with gethostbyname */
    host = gethostbyname(name.nodename);

    /* Skip everything up to and including the first `.' */
    for (pointer = host -> h_name; *pointer != '\0'; pointer++)
    {
	if (*pointer == '.')
	{
	    return pointer + 1;
	}
    }

    return DEFAULT_DOMAIN;
}

/* Makes the named directory and any parent directories needed */
static int mkdirhier(char *dirname)
{
    char *pointer;
    struct stat statbuf;

    /* End the recursion */
    if (*dirname == '\0')
    {
	return 0;
    }

    /* Find the last '/' in the directory name */
    if ((pointer = strrchr(dirname, '/')) != NULL)
    {
	int result;

	/* Call ourselves recursively to make sure the parent
	 * directory exists */
	*pointer = '\0';
	result = mkdirhier(dirname);
	*pointer = '/';

	/* If we couldn't create the parent directory, then fail */
	if (result < 0)
	{
	    return result;
	}
    }

    /* If a file named `dirname' exists then make sure it's a directory */
    if (stat(dirname, &statbuf) == 0)
    {
	if ((statbuf.st_mode & S_IFMT) != S_IFDIR)
	{
	    errno = EEXIST;
	    return -1;
	}
	
	return 0;
    }

    /* If the directory doesn't exist then try to create it */
    if (errno == ENOENT)
    {
	return mkdir(dirname, 0777);
    }

    return -1;
}

/* Publishes a notification indicating that the receiver has started */
static void publish_startup_notification(tickertape_t self)
{
    en_notify_t notification;
    SANITY_CHECK(self);

    /* If we haven't managed to connect, then don't try sending */
    if (self -> connection == NULL)
    {
	return;
    }

    notification = en_new();
    en_add_string(notification, "tickertape.startup", VERSION);
    en_add_string(notification, "user", self -> user);
    connection_publish(self -> connection, notification);
    en_free(notification);
}

/* Callback for a menu() action in the Scroller */
static void menu_callback(Widget widget, tickertape_t self, message_t message)
{
    SANITY_CHECK(self);
    ControlPanel_select(self -> control_panel, message);
    ControlPanel_show(self -> control_panel);
}

/* Callback for an action() action in the Scroller */
static void mime_callback(Widget widget, tickertape_t self, message_t message)
{
    SANITY_CHECK(self);

    /* Bail if no message provided */
    if (message == NULL)
    {
	return;
    }

    /* Show the message's MIME attachment */
    tickertape_show_attachment(self, message);
}

/* Callback for a kill() action in the Scroller */
static void kill_callback(Widget widget, tickertape_t self, message_t message)
{
    SANITY_CHECK(self);

    if (message == NULL)
    {
	return;
    }

    history_kill_thread(self -> history, self -> scroller, message);
}

/* Receive a message_t matched by a subscription */
static void receive_callback(tickertape_t self, message_t message)
{
    SANITY_CHECK(self);

    /* Add the message to the history */
    if (history_add(self -> history, message) == 0)
    {
	ScAddMessage(self -> scroller, message);
    }
}


/* Write the template to the given file, doing some substitutions */
static int write_default_file(tickertape_t self, FILE *out, char *template)
{
    char *pointer;

    for (pointer = template; *pointer != '\0'; pointer++)
    {
	if (*pointer == '%')
	{
	    switch (*(++pointer))
	    {
		/* Don't freak out on a terminal `%' */
		case '\0':
		{
		    fputc(*pointer, out);
		    return 0;
		}

		/* user name */
		case 'u':
		{
		    fputs(self -> user, out);
		    break;
		}

		/* domain name */
		case 'd':
		{
		    fputs(self -> domain, out);
		    break;
		}

		/* Anything else */
		default:
		{
		    fputc('%', out);
		    fputc(*pointer, out);
		    break;
		}
	    }
	}
	else
	{
	    fputc(*pointer, out);
	}
    }

    return 0;
}

/* Open a subscriptions file.  If the file doesn't exist, try to
 * create it and fill it with the default groups file information.
 * Returns the file descriptor of the groups file on success,
 * -1 on failure */
static int open_subscription_file(tickertape_t self, char *filename, char *template)
{
    int fd;
    FILE *out;

    /* Try to just open the file */
    if (! ((fd = open(filename, O_RDONLY)) < 0))
    {
	return fd;
    }

    /* If the file exists, then bail now */
    if (errno != ENOENT)
    {
	return -1;
    }

    /* Otherwise, try to create a default groups file */
    if ((out = fopen(filename, "w")) == NULL)
    {
	return -1;
    }

    /* Write the default subscriptions file */
    if (write_default_file(self, out, template) < 0)
    {
	fclose(out);
	return -1;
    }

    fclose(out);

    /* Try to open the file again */
    if ((fd = open(filename, O_RDONLY)) < 0)
    {
	perror("unable to read from groups file");
	return -1;
    }

    /* Success */
    return fd;
}


/* The callback for the groups file parser */
static int parse_groups_callback(
    tickertape_t self, char *name,
    int in_menu, int has_nazi,
    int min_time, int max_time)
{
    char *expression;
    group_sub_t subscription;

    expression = (char *)malloc(sizeof("TICKERTAPE == \"\"") + strlen(name));
    sprintf(expression, "TICKERTAPE == \"%s\"", name);

    /* Allocate us a subscription */
    if ((subscription = group_sub_alloc(
	    name, expression, in_menu, has_nazi, min_time, max_time,
	    (group_sub_callback_t)receive_callback, self)) == NULL)
    {
	return -1;
    }

    /* Add it to the end of the array */
    self -> groups = (group_sub_t *)realloc(
	self -> groups, sizeof(group_sub_t) * (self -> groups_count + 1));
    self -> groups[self -> groups_count++] = subscription;

    return 0;
}

/* Parse the groups file and update the groups table accordingly */
static int parse_groups_file(tickertape_t self)
{
    char *filename = tickertape_groups_filename(self);
    groups_parser_t parser;
    int fd;

    /* Allocate a new groups file parser */
    if ((parser = groups_parser_alloc(
	(groups_parser_callback_t)parse_groups_callback, self, filename)) == NULL)
    {
	return -1;
    }

    /* Make sure we can read the groups file */
    if ((fd = open_subscription_file(self, filename, default_groups_file)) < 0)
    {
	groups_parser_free(parser);
	return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    while (1)
    {
	char buffer[BUFFER_SIZE];
	ssize_t length;

	/* Read from the file */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    close(fd);
	    groups_parser_free(parser);
	    return -1;
	}

	/* Send it to the parser */
	if (groups_parser_parse(parser, buffer, length) < 0)
	{
	    close(fd);
	    groups_parser_free(parser);
	    return -1;
	}

	/* Watch for end-of-file */
	if (length == 0)
	{
	    return 0;
	}
    }
}


/* The callback for the usenet file parser */
static int parse_usenet_callback(
    tickertape_t self, int has_not, char *pattern,
    struct usenet_expr *expressions, size_t count)
{
    /* Forward the information to the usenet subscription */
    return usenet_sub_add(self -> usenet_sub, has_not, pattern, expressions, count);
}

/* Parse the usenet file and update the usenet subscription accordingly */
static int parse_usenet_file(tickertape_t self)
{
    char *filename = tickertape_usenet_filename(self);
    usenet_parser_t parser;
    int fd;

    /* Allocate a new usenet file parser */
    if ((parser = usenet_parser_alloc(
	(usenet_parser_callback_t)parse_usenet_callback,
	self, filename)) == NULL)
    {
	return -1;
    }

    /* Allocate a new usenet subscription */
    if ((self -> usenet_sub = usenet_sub_alloc(
	(usenet_sub_callback_t)receive_callback, self)) == NULL)
    {
	usenet_parser_free(parser);
	return -1;
    }

    /* Make sure we can read the usenet file */
    if ((fd = open_subscription_file(self, filename, defaultUsenetFile)) < 0)
    {
	usenet_parser_free(parser);
	return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    while (1)
    {
	char buffer[BUFFER_SIZE];
	ssize_t length;

	/* Read from the fiel */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    close(fd);
	    usenet_parser_free(parser);
	    return -1;
	}

	/* Send it to the parser */
	if (usenet_parser_parse(parser, buffer, length) < 0)
	{
	    close(fd);
	    usenet_parser_free(parser);
	    return -1;
	}

	/* Watch for end-of-file */
	if (length == 0)
	{
	    return 0;
	}
    }
}

/* Returns the index of the group with the given expression (-1 it none) */
static int find_group(group_sub_t *groups, int count, char *expression)
{
    group_sub_t *pointer;

    for (pointer = groups; pointer < groups + count; pointer++)
    {
	if ((*pointer != NULL) && (strcmp(group_sub_expression(*pointer), expression) == 0))
	{
	    return pointer - groups;
	}
    }

    return -1;
}

/* Request from the ControlPanel to reload groups file */
void tickertape_reload_groups(tickertape_t self)
{
    group_sub_t *old_groups = self -> groups;
    int old_count = self -> groups_count;
    int index;
    int count;

    self -> groups = NULL;
    self -> groups_count = 0;

    /* Read the new-and-improved groups file */
    parse_groups_file(self);

    /* Reuse elvin subscriptions whenever possible */
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_t group = self -> groups[index];
	int old_index;

	/* Look for a match */
	if ((old_index = find_group(old_groups, old_count, group_sub_expression(group))) < 0)
	{
	    /* None found.  Set the subscription's connection */
	    group_sub_set_connection(group, self -> connection);
	}
	else
	{
	    group_sub_t old_group = old_groups[old_index];

	    group_sub_update_from_sub(old_group, group);
	    group_sub_free(group);
	    self -> groups[index] = old_group;
	    old_groups[old_index] = NULL;
	}
    }

    /* Free the remaining old subscriptions */
    for (index = 0; index < old_count; index++)
    {
	group_sub_t old_group = old_groups[index];

	if (old_group != NULL)
	{
	    group_sub_set_connection(old_group, NULL);
	    group_sub_set_control_panel(old_group, NULL);
	    group_sub_free(old_group);
	}
    }

    /* Release the old array */
    free(old_groups);

    /* Renumber the items in the control panel */
    count = 0;
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_control_panel_index(self -> groups[index], self -> control_panel, &count);
    }
}


/* Request from the ControlPanel to reload usenet file */
void tickertape_reload_usenet(tickertape_t self)
{
    /* Release the old usenet subscription */
    usenet_sub_free(self -> usenet_sub);
    self -> usenet_sub = NULL;

    /* Try to read in the new one */
    if (parse_usenet_file(self) < 0)
    {
	return;
    }

    /* Set its connection */
    usenet_sub_set_connection(self -> usenet_sub, self -> connection);
}


/* Initializes the User Interface */
static void init_ui(tickertape_t self)
{
    int index;

    self -> control_panel = ControlPanel_alloc(self, self -> top);

    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_control_panel(self -> groups[index], self -> control_panel);
    }

    self -> scroller = (ScrollerWidget) XtVaCreateManagedWidget(
	"scroller", scrollerWidgetClass, self -> top,
	NULL);
    XtAddCallback(
	(Widget)self -> scroller, XtNcallback,
	(XtCallbackProc)menu_callback, self);
    XtAddCallback(
	(Widget)self -> scroller, XtNattachmentCallback,
	(XtCallbackProc)mime_callback, self);
    XtAddCallback(
	(Widget)self -> scroller, XtNkillCallback,
	(XtCallbackProc)kill_callback, self);
    XtRealizeWidget(self -> top);
}



/* This is called when we get our elvin connection back */
static void reconnect_callback(tickertape_t self, connection_t connection)
{
    char *host;
    int port;
    char *buffer;
    message_t message;

    /* Construct a reconnect message */
    host = connection_host(connection);
    port = connection_port(connection);
    if ((buffer = (char *)malloc(strlen(RECONNECT_MSG) + strlen(host) + 5 - 3)) == NULL)
    {
	return;
    }

    sprintf(buffer, RECONNECT_MSG, host, port);

    /* Display the message on the scroller */
    message = message_alloc(NULL, "internal", "tickertape", buffer, 30, NULL, NULL, 0, 0);
    receive_callback(self, message);
    message_free(message);

    /* Release our connect message */
    free(buffer);

    /* Republish the startup notification */
    publish_startup_notification(self);
}

/* This is called when we lose our elvin connection */
static void disconnect_callback(tickertape_t self, connection_t connection)
{
    message_t message;
    char *host = connection_host(connection);
    int port = connection_port(connection);
    char *format;
    char *buffer;

    /* If this is called in the middle of connection_alloc, then
     * self -> connection will be NULL.  Let the user know that we've
     * never managed to connect. */
    if (self -> connection == NULL)
    {
	format = NO_CONNECT_MSG;
    }
    else
    {
	format = LOST_CONNECT_MSG;
    }

    /* Compose our message */
    if ((buffer = (char *)malloc(strlen(format) + strlen(host) + 6)) == NULL)
    {
	return;
    }

    sprintf(buffer, format, host, port);

    /* Display the message on the scroller */
    message = message_alloc(
	NULL,
	"internal", "tickertape",
	buffer, 10,
	NULL, NULL,
	0, 0);
    receive_callback(self, message);
    message_free(message);

    free(buffer);
}



#ifdef ORBIT
/*
 *
 * Orbit-related functions
 *
 */

/* Callback for when we match the Orbit notification */
static void orbit_callback(tickertape_t self, en_notify_t notification)
{
    en_type_t type;
    char *id;
    char *tickertape;
    SANITY_CHECK(self);

    /* Get the id of the zone (if provided) */
    if ((en_search(notification, "zone.id", &type, (void **)&id) != 0) || (type != EN_STRING))
    {
	/* Can't subscribe without a zone id */
	return;
    }

    /* Get the status (if provided) */
    if ((en_search(notification, "tickertape", &type, (void **)&tickertape) != 0) ||
	(type != EN_STRING))
    {
	/* Not provided or not a string -- can't determine a useful course of action */
	en_free(notification);
	return;
    }

    /* See if we're subscribing */
    if (strcasecmp(tickertape, "true") == 0)
    {
	orbit_sub_t subscription;
	char *title;

	/* Get the title of the zone (if provided) */
	if ((en_search(notification, "zone.title", &type, (void **)&title) != 0) ||
	    (type != EN_STRING))
	{
	    title = "Untitled Zone";
	}

	/* If we already have a subscription, then update the title and quit */
	if ((subscription = Hashtable_get(self -> orbitSubscriptionsById, id)) != NULL)
	{
	    orbit_sub_set_title(subscription, title);
	    en_free(notification);
	    return;
	}

	/* Otherwise create a new Subscription and record it in the table */
	subscription = orbit_sub_alloc(
	    title, id,
	    (orbit_sub_callback_t)receive_callback, self);
	Hashtable_put(self -> orbitSubscriptionsById, id, subscription);

	/* Register the subscription with the connection and the ControlPanel */
	orbit_sub_set_connection(subscription, self -> connection);
	orbit_sub_set_control_panel(subscription, self -> control_panel);
    }

    /* See if we're unsubscribing */
    if (strcasecmp(tickertape, "false") == 0)
    {
	orbit_sub_t subscription = Hashtable_remove(self -> orbitSubscriptionsById, id);
	if (subscription != NULL)
	{
	    orbit_sub_set_connection(subscription, NULL);
	    orbit_sub_set_control_panel(subscription, NULL);
	}
    }

    en_free(notification);
}


/* Subscribes to the Orbit meta-subscription */
static void subscribe_to_orbit(tickertape_t self)
{
    char *buffer;

    /* Construct the subscription expression */
    if ((buffer = (char *)malloc(strlen(ORBIT_SUB) + strlen(self -> user) - 1)) == NULL)
    {
	return;
    }

    sprintf(buffer, ORBIT_SUB, self -> user);

    /* Subscribe to the meta-subscription */
    connection_subscribe(
	self -> connection, buffer,
	(notify_callback_t)orbit_callback, self);

    free(buffer);
}

#endif /* ORBIT */

/*
 *
 * Exported function definitions 
 *
 */

/* Answers a new tickertape_t for the given user using the given file as
 * her groups file and connecting to the notification service
 * specified by host and port */
tickertape_t tickertape_alloc(
    char *user, char *tickerDir,
    char *groups_file, char *usenet_file,
    char *host, int port,
    Widget top)
{
    int index;
    tickertape_t self = (tickertape_t) malloc(sizeof(struct tickertape));
    
    self -> user = strdup(user);
    self -> domain = strdup(get_domain());
    self -> tickerDir = (tickerDir == NULL) ? NULL : strdup(tickerDir);
    self -> groups_file = (groups_file == NULL) ? NULL : strdup(groups_file);
    self -> usenet_file = (usenet_file == NULL) ? NULL : strdup(usenet_file);
    self -> top = top;

    self -> groups = NULL;
    self -> groups_count = 0;

    self -> usenet_sub = NULL;
    self -> mail_sub = NULL;
    self -> connection = NULL;
    self -> history = history_alloc();

    /* Read the subscriptions from the groups file */
    if (parse_groups_file(self) < 0)
    {
	exit(1);
    }

    /* Read the subscriptions from the usenet file */
    if (parse_usenet_file(self) < 0)
    {
	exit(1);
    }

    /* Initialize the e-mail subscription */
    self -> mail_sub = mail_sub_alloc(user, (mail_sub_callback_t)receive_callback, self);

    /* Draw the user interface */
    init_ui(self);

    /* Connect to elvin and subscribe */
    self -> connection = connection_alloc(
	host, port, 	XtWidgetToApplicationContext(top),
	(disconnect_callback_t)disconnect_callback, self,
	(reconnect_callback_t)reconnect_callback, self);

    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_connection(self -> groups[index], self -> connection);
    }

    /* Subscribe to the usenet_sub */
    if (self -> usenet_sub != NULL)
    {
	usenet_sub_set_connection(self -> usenet_sub, self -> connection);
    }

    /* Subscribe to the Mail subscription if we have one */
    if (self -> mail_sub != NULL)
    {
	mail_sub_set_connection(self -> mail_sub, self -> connection);
    }

#ifdef ORBIT
    /* Listen for Orbit-related notifications and alert the world to our presence */
    self -> orbitSubscriptionsById = Hashtable_alloc(37);
    subscribe_to_orbit(self);
#endif /* ORBIT */

    publish_startup_notification(self);
    return self;
}

/* Free the resources consumed by a tickertape */
void tickertape_free(tickertape_t self)
{
    int index;

    /* How do we free a Widget? */

    if (self -> user)
    {
	free(self -> user);
    }

    if (self -> groups_file)
    {
	free(self -> groups_file);
    }

    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_free(self -> groups[index]);
    }

    free(self -> groups);

#ifdef ORBIT
    if (self -> orbitSubscriptionsById)
    {
	Hashtable_free(self -> orbitSubscriptionsById);
    }
#endif /* ORBIT */

    if (self -> connection)
    {
	connection_free(self -> connection);
    }

    if (self -> control_panel)
    {
	ControlPanel_free(self -> control_panel);
    }

    /* How do we free a ScrollerWidget? */

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else
    free(self);
#endif /* SANITY */
}

#ifdef DEBUG
/* Debugging */
void tickertape_debug(tickertape_t self)
{
    SANITY_CHECK(self);

    printf("tickertape_t\n");
    printf("  user = \"%s\"\n", self -> user);
    printf("  tickerDir = \"%s\"\n", (self -> tickerDir == NULL) ? "null" : self -> tickerDir);
    printf("  groups_file = \"%s\"\n", (self -> groups_file == NULL) ? "null" : self -> groups_file);
    printf("  usenet_file = \"%s\"\n", (self -> usenet_file == NULL) ? "null" : self -> usenet_file);
    printf("  top = 0x%p\n", self -> top);
    printf("  groups = 0x%p\n", self -> groups);
    printf("  groups_count = %d\n", self -> groups_count);
#ifdef ORBIT
    printf("  orbitSubscriptionsById = 0x%p\n", self -> orbitSubscriptionsById);
#endif /* ORBIT */
    printf("  connection = 0x%p\n", self -> connection);
    printf("  control_panel = 0x%p\n", self -> control_panel);
    printf("  scroller = 0x%p\n", self -> scroller);
}
#endif /* DEBUG */

/* Answers the tickertape's user name */
char *tickertape_user_name(tickertape_t self)
{
    return self -> user;
}

/* Answers the tickertape's history_t */
history_t tickertape_history(tickertape_t self)
{
    return self -> history;
}

/* Quit the application */
void tickertape_quit(tickertape_t self)
{
    XtDestroyApplicationContext(XtWidgetToApplicationContext(self -> top));
    exit(0);
}

/* Answers the receiver's tickerDir filename */
static char *tickertape_ticker_dir(tickertape_t self)
{
    /* See if we've already looked it up */
    if (self -> tickerDir != NULL)
    {
	return self -> tickerDir;
    }

    /* Use the TICKERDIR environment variable if it is set */
    if ((self -> tickerDir = getenv("TICKERDIR")) == NULL)
    {
	char *home;

	/* Else look up the user's home directory (default to '/' if not set) */
	if ((home = getenv("HOME")) == NULL)
	{
	    home = "/";
	}

	/* And append /.ticker to it to construct the directory name */
	self -> tickerDir = (char *)malloc(strlen(home) + sizeof(DEFAULT_TICKERDIR) + 1);
	strcpy(self -> tickerDir, home);
	strcat(self -> tickerDir, "/");
	strcat(self -> tickerDir, DEFAULT_TICKERDIR);
    }

    /* Make sure the TICKERDIR exists 
     * Note: we're being clever here and assuming that nothing will
     * call tickertape_ticker_dir() if both groups_file and usenet_file
     * are set */
    if (mkdirhier(self -> tickerDir) < 0)
    {
	perror("unable to create tickertape directory");
    }

    return self -> tickerDir;
}

/* Answers the receiver's groups file filename */
static char *tickertape_groups_filename(tickertape_t self)
{
    if (self -> groups_file == NULL)
    {
	char *dir = tickertape_ticker_dir(self);
	self -> groups_file = (char *)malloc(strlen(dir) + sizeof(DEFAULT_GROUPS_FILE) + 1);
	strcpy(self -> groups_file, dir);
	strcat(self -> groups_file, "/");
	strcat(self -> groups_file, DEFAULT_GROUPS_FILE);
    }

    return self -> groups_file;
}

/* Answers the receiver's usenet filename */
static char *tickertape_usenet_filename(tickertape_t self)
{
    if (self -> usenet_file == NULL)
    {
	char *dir = tickertape_ticker_dir(self);
	self -> usenet_file = (char *)malloc(strlen(dir) + sizeof(DEFAULT_USENET_FILE) + 1);
	strcpy(self -> usenet_file, dir);
	strcat(self -> usenet_file, "/");
	strcat(self -> usenet_file, DEFAULT_USENET_FILE);
    }

    return self -> usenet_file;
}


/* Displays a message's MIME attachment */
int tickertape_show_attachment(tickertape_t self, message_t message)
{
#ifdef METAMAIL
    char *mime_type;
    char *mime_args;
    char *filename;
    char *buffer;
    FILE *file;

    /* If the message has no attachment then we're done */
    if (((mime_type = message_get_mime_type(message)) == NULL) ||
	((mime_args = message_get_mime_args(message)) == NULL))
    {
#ifdef DEBUG
	printf("no attachment\n");
#endif /* DEBUG */
	return -1;
    }

#ifdef DEBUG
    printf("attachment: \"%s\" \"%s\"\n", mime_type, mime_args);
#endif /* DEBUG */

    /* Open up a temp file in which to write the args */
    filename = tmpnam(NULL);
    if ((file = fopen(filename, "wb")) == NULL)
    {
	perror("unable to open a temporary file");
	return -1;
    }

    fputs(mime_args, file);
    fclose(file);

    /* Invoke metamail to display the attachment */
    buffer = (char *) malloc(
	sizeof(METAMAIL) + sizeof(METAMAIL_OPTIONS) + 
	strlen(mime_type) + strlen(filename) +
	sizeof("....>./dev/null.2>&1"));
    sprintf(buffer, "%s %s %s %s > /dev/null 2>&1",
	    METAMAIL, METAMAIL_OPTIONS,
	    mime_type, filename);
    system(buffer);
    free(buffer);

    /* Clean up */
    unlink(filename);
#endif /* METAMAIL */

    return 0;
}
