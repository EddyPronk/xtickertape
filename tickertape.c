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
static const char cvsid[] = "$Id: tickertape.c,v 1.44 1999/12/12 23:38:24 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include "message.h"
#include "history.h"
#include "tickertape.h"
#include "Scroller.h"
#include "panel.h"
#include "groups.h"
#include "groups_parser.h"
#include "group_sub.h"
#include "usenet.h"
#include "usenet_parser.h"
#include "usenet_sub.h"
#include "mail_sub.h"
#ifdef ORBIT
#include "orbit_sub.h"
#endif /* ORBIT */

#define DEFAULT_TICKERDIR ".ticker"
#define DEFAULT_GROUPS_FILE "groups"
#define DEFAULT_USENET_FILE "usenet"

#define METAMAIL_OPTIONS "-x -B -q -b -c"
#define METAMAIL_FORMAT "%s %s %s %s"

/* How long to wait before we tell the user we're having trouble connecting */
#define CONNECT_TIMEOUT 3 * 1000
#define BUFFER_SIZE 1024

#define RECONNECT_MSG "Connected to elvin server at %s:%d."
#define NO_CONNECT_MSG "Unable to connect to elvin server at %s:%d.  Still trying..."
#define LOST_CONNECT_MSG \
"Lost connection to elvin server at %s:%d.  Attempting to reconnect..."
#define SLOW_CONNECT_MSG "The elvin server is slow to reply.  Still trying..."

#define GROUP_SUB "TICKERTAPE == \"%s\""
#define ORBIT_SUB "exists(orbit.view_update) && exists(tickertape) && user == \"%s\""

#define F_TICKERTAPE_STARTUP "tickertape.startup"
#define F_USER "user"

/* The tickertape data type */
struct tickertape
{
    /* A convenient error context */
    dstc_error_t error;

    /* The elvin connection handle */
    elvin_handle_t handle;

    /* The user's name */
    char *user;

    /* The local domain name */
    char *domain;

    /* The receiver's ticker directory */
    char *ticker_dir;

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
    orbit_sub_t *orbit_subs;

    /* The number of subscriptions in orbit_subs */
    int orbit_sub_count;
#endif /* ORBIT */

    /* The control panel */
    control_panel_t control_panel;

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
#if 0
static void disconnect_callback(tickertape_t self, connection_t connection);
static void reconnect_callback(tickertape_t self, connection_t connection);
#endif /* 0 */
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
    elvin_notification_t notification;

    /* If we haven't managed to connect, then don't try sending */
    if (self -> handle == NULL)
    {
	return;
    }

    /* Create a nice startup notification */
    if ((notification = elvin_notification_alloc(self -> error)) == NULL)
    {
	fprintf(stderr, "elvin_notification_alloc(): failed\n");
	abort();
    }

    if (elvin_notification_add_string8(
	notification,
	F_TICKERTAPE_STARTUP,
	VERSION,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string8(): failed\n");
	abort();
    }

    if (elvin_notification_add_string8(
	notification,
	F_USER,
	self -> user,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string8(): failed\n");
	abort();
    }

    /* No keys support yet */
    elvin_async_notify(self -> handle, notification, NULL, self -> error);
    elvin_notification_free(notification, self -> error);
}

/* Callback for a menu() action in the Scroller */
static void menu_callback(Widget widget, tickertape_t self, message_t message)
{
    control_panel_select(self -> control_panel, message);
    control_panel_show(self -> control_panel);
}

/* Callback for an action() action in the Scroller */
static void mime_callback(Widget widget, tickertape_t self, message_t message)
{
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
    if (message == NULL)
    {
	return;
    }

    history_kill_thread(self -> history, self -> scroller, message);
}

/* Receive a message_t matched by a subscription */
static void receive_callback(tickertape_t self, message_t message)
{
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
static int open_subscription_file(
    tickertape_t self,
    char *filename,
    char *template)
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

    /* Construct the subscription expression */
    if ((expression = (char *)malloc(
	strlen(GROUP_SUB) + strlen(name) - 1)) == NULL)
    {
	return -1;
    }

    sprintf(expression, GROUP_SUB, name);

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

    /* Clean up */
    free(expression);
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
	(groups_parser_callback_t)parse_groups_callback,
	self,
	filename)) == NULL)
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
	    close(fd);
	    groups_parser_free(parser);
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
    return usenet_sub_add(
	self -> usenet_sub,
	has_not,
	pattern,
	expressions,
	count);
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
	    close(fd);
	    usenet_parser_free(parser);
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
	if (*pointer != NULL && 
	    strcmp(group_sub_expression(*pointer), expression) == 0)
	{
	    return pointer - groups;
	}
    }

    return -1;
}

/* Request from the control panel to reload groups file */
void tickertape_reload_groups(tickertape_t self)
{
    group_sub_t *old_groups = self -> groups;
    int old_count = self -> groups_count;
    int index;
    int count;

    self -> groups = NULL;
    self -> groups_count = 0;

    /* Read the new-and-improved groups file */
    if (parse_groups_file(self) < 0)
    {
	perror("trouble reading groups file");
	return;
    }

    /* Reuse elvin subscriptions whenever possible */
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_t group = self -> groups[index];
	int old_index;

	/* Look for a match */
	if ((old_index = find_group(
	    old_groups,
	    old_count,
	    group_sub_expression(group))) < 0)
	{
	    /* None found.  Set the subscription's connection */
	    group_sub_set_connection(group, self -> handle, self -> error);
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
	    group_sub_set_connection(old_group, NULL, self -> error);
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
	group_sub_set_control_panel_index(
	    self -> groups[index],
	    self -> control_panel,
	    &count);
    }
}

/* Request from the control panel to reload usenet file */
void tickertape_reload_usenet(tickertape_t self)
{
    /* Release the old usenet subscription */
    usenet_sub_set_connection(self -> usenet_sub, NULL, self -> error);
    usenet_sub_free(self -> usenet_sub);
    self -> usenet_sub = NULL;

    /* Try to read in the new one */
    if (parse_usenet_file(self) < 0)
    {
	return;
    }

    /* Set its connection */
    usenet_sub_set_connection(
	self -> usenet_sub,
	self -> handle,
	self -> error);
}


/* Initializes the User Interface */
static void init_ui(tickertape_t self)
{
    int index;

    /* Allocate the control panel */
    if ((self -> control_panel = control_panel_alloc(self, self -> top)) == NULL)
    {
	return;
    }

    /* Tell our group subscriptions about the control panel */
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_control_panel(self -> groups[index], self -> control_panel);
    }

    /* Create the scroller widget */
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



#if 0
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
    message = message_alloc(
	NULL, "internal", "tickertape", buffer, 30,
	NULL, NULL,
	NULL, NULL, NULL);

    receive_callback(self, message);
    message_free(message);

    /* Release our connect message */
    free(buffer);

    /* Republish the startup notification */
    publish_startup_notification(self);
}
#endif /* 0 */

#if 0
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
	NULL, "internal", "tickertape", buffer, 10,
	NULL, NULL,
	NULL, NULL, NULL);

    receive_callback(self, message);
    message_free(message);

    free(buffer);
}
#endif /* 0 */



#ifdef ORBIT
/*
 *
 * Orbit-related functions
 *
 */

/* Adds a new orbit_sub_t to the orbit_subs array */
static void add_orbit_sub(tickertape_t self, orbit_sub_t orbit_sub)
{
    /* Expand the array */
    if ((self -> orbit_subs = (orbit_sub_t *)realloc(
	self -> orbit_subs,
	sizeof(orbit_sub_t) * self -> orbit_sub_count)) == NULL)
    {
	return;
    }

    /* Add the subscription at the end */
    self -> orbit_subs[self -> orbit_sub_count++] = orbit_sub;
}

/* Removes an orbit_sub_t from the orbit_subs array */
static orbit_sub_t remove_orbit_sub(tickertape_t self, char *id)
{
    orbit_sub_t match = NULL;
    int index;

    for (index = 0; index < self -> orbit_sub_count; index++)
    {
	/* Have we already found a match? */
	if (match != NULL)
	{
	    self -> orbit_subs[index - 1] = self -> orbit_subs[index];
	}
	/* Otherwise, do we have a match? */
	else if (strcmp(orbit_sub_get_id(self -> orbit_subs[index]), id) == 0)
	{
	    match = self -> orbit_subs[index];
	}
    }

    /* Adjust the number of subscriptions */
    if (match != NULL)
    {
	self -> orbit_sub_count--;
    }

    return match;
}

/* Locates the subscription in the orbit_subs array */
static orbit_sub_t find_orbit_sub(tickertape_t self, char *id)
{
    int index;

    for (index = 0; index < self -> orbit_sub_count; index++)
    {
	if (strcmp(orbit_sub_get_id(self -> orbit_subs[index]), id) == 0)
	{
	    return self -> orbit_subs[index];
	}
    }

    return NULL;
}

/* Callback for when we match the Orbit notification */
static void orbit_callback(tickertape_t self, en_notify_t notification)
{
    en_type_t type;
    char *id;
    char *tickertape;

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

	/* See if we already have a matching subscription */
	if ((subscription = find_orbit_sub(self, id)) != NULL)
	{
	    /* We do.  Update its title and we're done */
	    orbit_sub_set_title(subscription, title);
	    en_free(notification);
	    return;
	}

	/* Otherwise we need to make a new one */
	if ((subscription = orbit_sub_alloc(
	    title, id,
	    (orbit_sub_callback_t)receive_callback, self)) == NULL)
	{
	    en_free(notification);
	    return;
	}

	/* Add the new subscription to the end */
	add_orbit_sub(self, subscription);

	/* Register the subscription with the connection and the control panel */
	orbit_sub_set_connection(subscription, self -> connection);
	orbit_sub_set_control_panel(subscription, self -> control_panel);
    }
    /* See if we're unsubscribing */
    else if (strcasecmp(tickertape, "false") == 0)
    {
	orbit_sub_t subscription;

	/* Remove the subscription from the array */
	if ((subscription = remove_orbit_sub(self, id)) != NULL)
	{
	    orbit_sub_set_connection(subscription, NULL);
	    orbit_sub_set_control_panel(subscription, NULL);
	    orbit_sub_free(subscription);
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
    printf("self -> user = \"%s\"\n", self -> user);
    printf("orbit subscription is \"%s\"\n", buffer);

    /* Subscribe to the meta-subscription */
    connection_subscribe(
	self -> connection, buffer,
	(notify_callback_t)orbit_callback, self);

    free(buffer);
}

#endif /* ORBIT */

/* This is called when our connection request hasn't been handled in a
 * timely fasion (usually means the server is broken or a very long
 * way away). */
static void connect_timeout(XtPointer rock, XtIntervalId *interval)
{
    tickertape_t self = (tickertape_t)rock;
    message_t message;

    /* If we've successfully connected then we're done */
    if (self -> handle != NULL)
    {
	return;
    }

    /* Otherwise alert the user that we're having trouble connecting */
    message = message_alloc(
	NULL,
	"internal", "tickertape",
	SLOW_CONNECT_MSG, 10,
	NULL, NULL,
	NULL, NULL, NULL);
    receive_callback(self, message);
    message_free(message);
}

/* This is called when our connection request is handled */
static void connect_cb(
    elvin_handle_t handle, int result,
    void *rock, dstc_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    int index;

    /* Record the elvin_handle */
    self -> handle = handle;

    /* Tell the control panel that we're connected */
    control_panel_set_connected(self -> control_panel, 1);

    /* Subscribe to the groups */
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_connection(self -> groups[index], handle, error);
    }

    /* Subscribe to usenet */
    if (self -> usenet_sub != NULL)
    {
	usenet_sub_set_connection(self -> usenet_sub, handle, error);
    }

    /* Subscribe to the Mail subscription if we have one */
    if (self -> mail_sub != NULL)
    {
	mail_sub_set_connection(self -> mail_sub, handle, error);
    }

#ifdef ORBIT
    /* Listen for Orbit-related notifications and alert */
    subscribe_to_orbit(self);
#endif /* ORBIT */
}

/*
 *
 * Exported function definitions 
 *
 */

/* Answers a new tickertape_t for the given user using the given file as
 * her groups file and connecting to the notification service */
tickertape_t tickertape_alloc(
    elvin_handle_t handle,
    char *user, char *domain, 
    char *ticker_dir,
    char *groups_file, char *usenet_file,
    Widget top,
    dstc_error_t error)
{
    tickertape_t self;

    /* Allocate some space for the new tickertape */
    if ((self = (tickertape_t) malloc(sizeof(struct tickertape))) == NULL)
    {
	return NULL;
    }

    /* Initialize its contents to sane values */
    self -> error = error;
    self -> handle = NULL;
    self -> user = strdup(user);
    self -> domain = strdup(domain);
    self -> ticker_dir = (ticker_dir == NULL) ? NULL : strdup(ticker_dir);
    self -> groups_file = (groups_file == NULL) ? NULL : strdup(groups_file);
    self -> usenet_file = (usenet_file == NULL) ? NULL : strdup(usenet_file);
    self -> top = top;
    self -> groups = NULL;
    self -> groups_count = 0;
    self -> usenet_sub = NULL;
    self -> mail_sub = NULL;
#ifdef ORBIT
    self -> orbit_subs = NULL;
    self -> orbit_sub_count = 0;
#endif /* ORBIT */
    self -> control_panel = NULL;
    self -> scroller = NULL;
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

    /* Connect to the elvin server */
    if (elvin_async_connect(handle, connect_cb, self, self -> error) == 0)
    {
	fprintf(stderr, "*** elvin_async_connect(): failed\n");
	exit(1);
    }

    /* Register a timeout in case the connect request doesn't come back */
    XtAppAddTimeOut(
	XtWidgetToApplicationContext(top), CONNECT_TIMEOUT,
	connect_timeout, self);
    return self;
}

/* Free the resources consumed by a tickertape */
void tickertape_free(tickertape_t self)
{
    int index;

    /* How do we free a Widget? */

    if (self -> user != NULL)
    {
	free(self -> user);
    }

    if (self -> domain != NULL)
    {
	free(self -> domain);
    }

    if (self -> ticker_dir != NULL)
    {
	free(self -> ticker_dir);
    }

    if (self -> groups_file != NULL)
    {
	free(self -> groups_file);
    }

    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_connection(self -> groups[index], NULL, self -> error);
	group_sub_free(self -> groups[index]);
    }

    if (self -> groups != NULL)
    {
	free(self -> groups);
    }

    if (self -> usenet_sub != NULL)
    {
	usenet_sub_set_connection(self -> usenet_sub, NULL, self -> error);
	usenet_sub_free(self -> usenet_sub);
    }

#ifdef ORBIT
    for (index = 0; index < self -> orbit_sub_count; index++)
    {
	orbit_sub_free(self -> orbit_subs[index], self -> error);
    }

    if (self -> orbit_subs != NULL)
    {
	free(self -> orbit_subs);
    }
#endif /* ORBIT */

    if (self -> control_panel)
    {
	control_panel_free(self -> control_panel);
    }

    /* How do we free a ScrollerWidget? */

    if (self -> history != NULL)
    {
	history_free(self -> history);
    }

    free(self);
}

/* Answers the tickertape's user name */
char *tickertape_user_name(tickertape_t self)
{
    return self -> user;
}

/* Answers the tickertape's domain name */
char *tickertape_domain_name(tickertape_t self)
{
    return self -> domain;
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

/* Answers the receiver's ticker_dir filename */
static char *tickertape_ticker_dir(tickertape_t self)
{
    char *dir;

    /* See if we've already looked it up */
    if (self -> ticker_dir != NULL)
    {
	return self -> ticker_dir;
    }

    /* Use the TICKERDIR environment variable if it is set */
    if ((dir = getenv("TICKERDIR")) != NULL)
    {
	self -> ticker_dir = strdup(dir);
    }
    else
    {
	/* Otherwise grab the user's home directory */
	if ((dir = getenv("HOME")) == NULL)
	{
	    dir = "/";
	}

	/* And append /.ticker to the end of it */
	self -> ticker_dir = (char *)malloc(strlen(dir) + strlen(DEFAULT_TICKERDIR) + 2);
	sprintf(self -> ticker_dir, "%s/%s", dir, DEFAULT_TICKERDIR);
    }

    /* Make sure the TICKERDIR exists 
     * Note: we're being clever here and assuming that nothing will
     * call tickertape_ticker_dir() if both groups_file and usenet_file
     * are set */
    if (mkdirhier(self -> ticker_dir) < 0)
    {
	perror("unable to create tickertape directory");
    }

    return self -> ticker_dir;
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
    if ((buffer = (char *) malloc(
	sizeof(METAMAIL_FORMAT) +
	strlen(METAMAIL) + strlen(METAMAIL_OPTIONS) + 
	strlen(mime_type) + strlen(filename) - 8)) != NULL)
    {
	sprintf(buffer, METAMAIL_FORMAT,
		METAMAIL, METAMAIL_OPTIONS,
		mime_type, filename);
	system(buffer);
	free(buffer);
    }

    /* Clean up */
    unlink(filename);
#endif /* METAMAIL */

    return 0;
}
