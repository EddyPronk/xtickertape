/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2001.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
static const char cvsid[] = "$Id: tickertape.c,v 1.80 2001/08/25 14:04:45 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include <elvin/error.h>
#include "errors.h"
#include "message.h"
#include "history.h"
#include "tickertape.h"
#include "Scroller.h"
#include "panel.h"

#include "vm.h"
#include "parser.h"

#include "groups.h"
#include "groups_parser.h"
#include "group_sub.h"
#include "usenet.h"
#include "usenet_parser.h"
#include "usenet_sub.h"
#include "mail_sub.h"

#define DEFAULT_TICKERDIR ".ticker"
#define DEFAULT_CONFIG_FILE "xtickertape.conf"
#define DEFAULT_GROUPS_FILE "groups"
#define DEFAULT_USENET_FILE "usenet"

#define METAMAIL_OPTIONS "-x", "-B", "-q", "-b"

/* How long to wait before we tell the user we're having trouble connecting */
#define BUFFER_SIZE 1024

#define CONNECT_MSG "Connected to elvin server: %s"
#define LOST_CONNECT_MSG "Lost connection to elvin server %s"
#define CONN_CLOSED_MSG "Connection closed by server: %s"
#define PROTOCOL_ERROR_MSG "Protocol error encountered with server: %s"
#define UNKNOWN_STATUS_MSG "Unknown status: %d"
#define DROP_WARN_MSG "One or more packets were dropped"

#define GROUP_SUB "TICKERTAPE == \"%s\""

#define F_TICKERTAPE_STARTUP "tickertape.startup"
#define F_USER "user"

/* The tickertape data type */
struct tickertape
{
    /* A convenient error context */
    elvin_error_t error;

    /* The elvin connection handle */
    elvin_handle_t handle;

    /* The interpreter's virtual machine */
    vm_t vm;

    /* The interpreter's parser */
    parser_t parser;

    /* The user's name */
    char *user;

    /* The local domain name */
    char *domain;

    /* The receiver's ticker directory */
    char *ticker_dir;

    /* The config file filename */
    char *config_file;

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
static void menu_callback(Widget widget, tickertape_t self, message_t message);
static void receive_callback(void *rock, message_t message, int show_attachment);
static int parse_groups_file(tickertape_t self);
static void init_ui(tickertape_t self);
#if 0
static void publish_startup_notification(tickertape_t self);
static void disconnect_callback(tickertape_t self, connection_t connection);
static void reconnect_callback(tickertape_t self, connection_t connection);
#endif /* 0 */
static char *tickertape_ticker_dir(tickertape_t self);
#if 0
static char *tickertape_config_filename(tickertape_t self);
#endif
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
static void receive_callback(void *rock, message_t message, int show_attachment)
{
    tickertape_t self = (tickertape_t)rock;

#if 1
    control_panel_add_message(self -> control_panel, message);
    ScAddMessage(self -> scroller, message);
#else
    /* Add the message to the history */
    if (history_add(self -> history, message) == 0)
    {
	ScAddMessage(self -> scroller, message);
    }
#endif

    /* Show the attachment if requested */
    if (show_attachment != False)
    {
	tickertape_show_attachment(self, message);
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


/* Open a configuration file.  If the file doesn't exist, try to
 * create it and fill it with the default groups file information.
 * Returns the file descriptor of the open config file on success,
 * -1 on failure */
static int open_config_file(
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

    /* Write the default config file */
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

#if 0
/* The callback for the config file parser */
static int parse_config_callback(
    void *rock,
    uint32_t count,
    subscription_t *subs,
    elvin_error_t error)
{
    printf("parsed!\n");
}
#endif

#if 0
/* Reads the configuration file */
static int read_config_file(tickertape_t self, elvin_error_t error)
{
    char *filename = tickertape_config_filename(self);
    parser_t parser;
    int fd;

    /* Make sure we can read the config file */
    if ((fd = open_config_file(self, filename, default_config_file)) < 0)
    {
	return 0;
    }

    /* Allocate a new config file parser */
    if (! (parser = parser_alloc(parse_config_callback, self, error)))
    {
	return 0;
    }

    /* Use it to read the config file */
    if (! parser_parse_file(parser, fd, filename, error))
    {
	return 0;
    }

    /* Close the file */
    if (close(fd) < 0)
    {
	perror("close(): failed");
	exit(1);
    }

    /* Clean up */
    return parser_free(parser, error);
}
#endif

/* The callback for the groups file parser */
static int parse_groups_callback(
    tickertape_t self, char *name,
    int in_menu, int has_nazi,
    int min_time, int max_time,
    elvin_keys_t producer_keys,
    elvin_keys_t consumer_keys)
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
	    name, expression,
	    in_menu, has_nazi,
	    min_time, max_time,
	    producer_keys, consumer_keys,
	    receive_callback, self)) == NULL)
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
    if ((fd = open_config_file(self, filename, default_groups_file)) < 0)
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
    if ((self -> usenet_sub = usenet_sub_alloc(receive_callback, self)) == NULL)
    {
	usenet_parser_free(parser);
	return -1;
    }

    /* Make sure we can read the usenet file */
    if ((fd = open_config_file(self, filename, defaultUsenetFile)) < 0)
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
	/* Clean up the mess */
	for (index = 0; index < self -> groups_count; index++)
	{
	    group_sub_free(self -> groups[index]);
	}

	/* Put the old groups back into place */
	self -> groups = old_groups;
	self -> groups_count = old_count;
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

    if (elvin_notification_add_string(
	notification,
	F_TICKERTAPE_STARTUP,
	VERSION,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    if (elvin_notification_add_string(
	notification,
	F_USER,
	self -> user,
	self -> error) == 0)
    {
	fprintf(stderr, "elvin_notification_add_string(): failed\n");
	abort();
    }

    /* No keys support yet */
    elvin_xt_notify(self -> handle, notification, NULL, self -> error);
    elvin_notification_free(notification, self -> error);
}
#endif

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
    if ((buffer = (char *)malloc(strlen(CONNECT_MSG) + strlen(host) + 5 - 3)) == NULL)
    {
	return;
    }

    sprintf(buffer, CONNECT_MSG, host, port);

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



/* This is called when our connection request is handled */
static void connect_cb(
    elvin_handle_t handle, int result,
    void *rock, elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    int index;

    /* Record the elvin_handle */
    self -> handle = handle;

    /* Check for a failure to connect */
    if (result == 0)
    {
	fprintf(stderr, "%s: unable to connect\n", PACKAGE);
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

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
}


/* Callback for elvin status changes */
static void status_cb(
    elvin_handle_t handle, char *url,
    elvin_status_event_t event,
    void *rock,
    elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    message_t message;
    char *buffer = NULL;
    char *string;

    /* Construct an appropriate message string */
    switch (event)
    {
	/* We were unable to (re)connect */
	case ELVIN_STATUS_CONNECTION_FAILED:
	{
	    fprintf(stderr, "%s: unable to connect\n", PACKAGE);
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	case ELVIN_STATUS_CONNECTION_FOUND:
	{
	    /* Tell the control panel that we're connected */
	    control_panel_set_connected(self -> control_panel, True);
	    
	    /* Make room for a combined string and URL */
	    if ((buffer = (char *)malloc(sizeof(CONNECT_MSG) + strlen(url) - 2)) == NULL)
	    {
		return;
	    }

	    sprintf(buffer, CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_LOST:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a combined string and URL */
	    if ((buffer = (char *)malloc(sizeof(LOST_CONNECT_MSG) + strlen(url) - 2)) == NULL)
	    {
		return;
	    }

	    sprintf(buffer, LOST_CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_CLOSED:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a message string */
	    if ((buffer = (char *)malloc(sizeof(CONN_CLOSED_MSG) + strlen(url) - 2)) == NULL)
	    {
		return;
	    }

	    sprintf(buffer, CONN_CLOSED_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_DROP_WARN:
	{
	    string = DROP_WARN_MSG;
	    break;
	}

	case ELVIN_STATUS_PROTOCOL_ERROR:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a message string */
	    if ((buffer = (char *)malloc(sizeof(PROTOCOL_ERROR_MSG) + strlen(url) - 2)) == NULL)
	    {
		return;
	    }

	    sprintf(buffer, PROTOCOL_ERROR_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_IGNORED_ERROR:
	{
	    fprintf(stderr, "%s: status ignored\n", PACKAGE);
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	case ELVIN_STATUS_CLIENT_ERROR:
	{
	    fprintf(stderr, "%s: client error\n", PACKAGE);
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	default:
	{
	    buffer = (char *)malloc(sizeof(UNKNOWN_STATUS_MSG) + 11);
	    sprintf(buffer, UNKNOWN_STATUS_MSG, event);
	    string = buffer;
	    break;
	}
    }

    /* Construct a message for the string and add it to the scroller */
    if ((message = message_alloc(
	NULL, "internal", "tickertape", string, 30,
	NULL, NULL, 0,
	NULL, NULL, NULL)) != NULL)
    {
	receive_callback(self, message, False);
	message_free(message);
    }

    /* Clean up */
    if (buffer != NULL)
    {
	free(buffer);
    }
}


/* The callback from the parser */
static int parsed(vm_t vm, parser_t parser, void *rock, elvin_error_t error)
{
    /* Try to evaluate what we've just parsed */
    if (! vm_eval(vm, error) || ! vm_print(vm, error))
    {
	elvin_error_fprintf(stderr, error);
	elvin_error_clear(error);

	/* Clean up the vm */
	if (! vm_gc(vm, error))
	{
	    return 0;
	}
    }

    return 1;
}

/* The function to call when the control panel sends a message to the
 * scheme interpreter group */
static void interp_cb(XtPointer rock, int *source, XtInputId *id)
{
    tickertape_t self = (tickertape_t)rock;
    char buffer[4096];
    ssize_t length;

    /* Read from stdin */
    if ((length = read(*source, buffer, 4096)) < 0)
    {
	perror("read(): failed");
	exit(1);
    }

    /* Send the info to the parser */
    if (! parser_read_buffer(self -> parser, buffer, length, self -> error))
    {
	elvin_error_fprintf(stderr, self -> error);
    }

    /* Watch for end of input */
    if (length == 0)
    {
	XtRemoveInput(*id);
	printf("[EOF]\n");
	fclose(stdin);
	return;
    }

    printf("> "); fflush(stdout);
}


/* The `and' special form */
static int prim_and(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* No args evaluates to `t' */
    if (argc == 0)
    {
	return vm_push_symbol(vm, "t", error);
    }

    /* Look for any nil arguments */
    while (argc > 1)
    {
	object_type_t type;

	/* Unroll the next arg onto the top of the stack */
	if (! vm_unroll(vm, argc - 1, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_type(vm, &type, error))
	{
	    return 0;
	}

	/* If the arg evaluated to nil then return that */
	if (type == SEXP_NIL)
	{
	    return 1;
	}

	/* Otherwise pop it and go on to the next arg */
	if (! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	argc--;
    }

    /* Return whatever the last arg evaluates to */
    return vm_eval(vm, error);
}

/* The `car' subroutine */
static int prim_car(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "car", argc);
	return 0;
    }

    return vm_car(vm, error);
}

/* The `cdr' subroutine */
static int prim_cdr(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "cdr", argc);
	return 0;
    }

    return vm_cdr(vm, error);
}

/* The `cond' special form */
static int prim_cond(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* Find a clause with a non-nil condition */
    while (argc > 0)
    {
	object_type_t type;

	/* Evaluate the condition of the next clause */
	if (! vm_unroll(vm, argc - 1, error) ||
	    ! vm_dup(vm, error) ||
	    ! vm_car(vm, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_type(vm, &type, error) ||
	    ! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	/* If it evaluates to non-nil then evaluate the clause as if it
	 * were part of a progn statement */
	if (type != SEXP_NIL)
	{
	    return
		vm_cdr(vm, error) &&
		vm_push_symbol(vm, "progn", error) &&
		vm_swap(vm, error) &&
		vm_make_cons(vm, error) &&
		vm_eval(vm, error);
	}

	/* Dump this clause and move on to the next one */
	if (! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	argc--;
    }

    /* Nothing matched so we return nil */
    return vm_push_nil(vm, error);
}

/* The `cons' subroutine */
static int prim_cons(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 2)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "cons", argc);
	return 0;
    }

    return vm_make_cons(vm, error);
}

/* The `define' special form */
static int prim_define(vm_t vm, uint32_t argc, elvin_error_t error)
{
    object_type_t type;

    if (argc < 2)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "define", argc);
	return 0;
    }

    /* Get the first argument */
    if (! vm_unroll(vm, argc - 1, error) || ! vm_type(vm, &type, error))
    {
	return 0;
    }

    switch (type)
    {
	/* A list indicates a function definition */
	case SEXP_CONS:
	{
	    /* Grab the car of the list to use as the function name */
	    if (! vm_dup(vm, error) ||
		! vm_car(vm, error) ||
		! vm_type(vm, &type, error))
	    {
		return 0;
	    }

	    /* Make sure that we've got a symbol */
	    if (type != SEXP_SYMBOL)
	    {
		ELVIN_ERROR_INTERP_TYPE_MISMATCH(error, "<value>", "symbol");
		return 0;
	    }

	    /* Roll it to the top of the stack, create a lambda and assign */
	    return
		vm_roll(vm, argc, error) &&
		vm_cdr(vm, error) &&
		vm_roll(vm, argc - 1, error) &&
		vm_push_symbol(vm, "progn", error) &&
		vm_roll(vm, argc - 1, error) &&
		vm_make_list(vm, argc, error) &&
		vm_make_lambda(vm, error) &&
		vm_assign(vm, error);
	}

	/* A symbol is a simple assignment */
	case SEXP_SYMBOL:
	{
	    /* Complain if there are too many args */
	    if (argc > 2)
	    {
		ELVIN_ERROR_INTERP_WRONG_ARGC(error, "define", argc);
		return 0;
	    }

	    /* Evaluate the value and assign it */
	    return vm_swap(vm, error) && vm_eval(vm, error) && vm_assign(vm, error);
	}

	/* Anything else is an error */
	default:
	{
	    ELVIN_ERROR_INTERP_TYPE_MISMATCH(error, "<value>", "symbol");
	    return 0;
	}
    }
}

/* The `defun' special form */
static int prim_defun(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc < 2)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "defun", argc);
	return 0;
    }

    /* Turn the body forms into a progn */
    return
	vm_push_symbol(vm, "progn", error) &&
	vm_roll(vm, argc - 2, error) &&
	vm_make_list(vm, argc - 1, error) &&
	vm_make_lambda(vm, error) &&
	vm_assign(vm, error);
}

/* The `eq' subroutine */
static int prim_eq(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 2)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "eq", argc);
	return 0;
    }

    return vm_eq(vm, error);
}

/* The `if' special form */
static int prim_if(vm_t vm, uint32_t argc, elvin_error_t error)
{
    object_type_t type;

    /* Make sure we have a test and a true form */
    if (argc < 2)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "if", argc);
	return 0;
    }

    /* Rotate the condition to the top of the stack and evaluate it */
    if (! vm_unroll(vm, argc - 1, error) || ! vm_eval(vm, error))
    {
	return 0;
    }

    /* Is it nil? */
    if (! vm_type(vm, &type, error))
    {
	return 0;
    }

    /* If the value is not nil then evaluate the true form */
    if (type != SEXP_NIL)
    {
	return vm_unroll(vm, argc - 1, error) && vm_eval(vm, error);
    }

    /* If there is no false form then return nil */
    if (argc == 2)
    {
	return vm_push_nil(vm, error);
    }

    /* Evaluate all args before the last */
    while (argc > 3)
    {
	/* Unroll the next arg onto the top of the stack */
	if (! vm_unroll(vm, argc - 2, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	argc--;
    }

    /* Return whatever the last arg evaluates to */
    return vm_unroll(vm, 1, error) && vm_eval(vm, error);
}

/* The `gc' subroutine */
static int prim_gc(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 0)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "gc", argc);
	return 0;
    }

    return vm_gc(vm, error);
}

/* The `lambda' special form */
static int prim_lambda(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* Make sure we at least have an argument list */
    if (argc < 1)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "lambda", argc);
	return 0;
    }

    /* Turn the body forms into a progn */
    return
	vm_push_symbol(vm, "progn", error) &&
	vm_roll(vm, argc - 1, error) &&
	vm_make_list(vm, argc, error) &&
	vm_make_lambda(vm, error);
}

/* The `let' special form */
static int prim_let(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* Make sure we at least have the bindings */
    if (argc < 1)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "let", argc);
	return 0;
    }

    /* Turn the body forms into a big progn */
    return
	vm_push_symbol(vm, "progn", error) &&
	vm_roll(vm, argc - 1, error) &&
	vm_make_list(vm, argc, error) &&
	vm_let(vm, error);
}

/* The `list' subroutine */
static int prim_list(vm_t vm, uint32_t argc, elvin_error_t error)
{
    return vm_make_list(vm, argc, error);
}

/* The `or' special form */
static int prim_or(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* No arguments evaluates to `nil' */
    if (argc == 0)
    {
	return vm_push_nil(vm, error);
    }

    /* Look for any true arguments */
    while (argc > 1)
    {
	object_type_t type;

	/* Unroll the next arg onto the top of the stack */
	if (! vm_unroll(vm, argc - 1, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_type(vm, &type, error))
	{
	    return 0;
	}

	/* If the argument evaluated to non-nil then return that */
	if (type != SEXP_NIL)
	{
	    return 1;
	}

	/* Otherwise pop it and go on to the next arg */
	if (! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	argc--;
    }

    /* Return whatever the last argument evalutes to */
    return vm_eval(vm, error);
}

/* The `+' subroutine */
static int prim_plus(vm_t vm, uint32_t argc, elvin_error_t error)
{
    uint32_t i;

    /* Return 0 if there are no args */
    if (argc == 0)
    {
	return vm_push_integer(vm, 0, error);
    }

    /* Add up all of the args */
    for (i = 1; i < argc; i++)
    {
	if (! vm_add(vm, error))
	{
	    return 0;
	}
    }

    return 1;
}

/* The `quote' special form */
static int prim_quote(vm_t vm, uint32_t argc, elvin_error_t error)
{
    if (argc != 1)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "quote", argc);
	return 0;
    }

    /* Our result is what's already on the stack */
    return 1;
}

/* The `progn' special form */
static int prim_progn(vm_t vm, uint32_t argc, elvin_error_t error)
{
    /* No args evaluates to nil */
    if (argc == 0)
    {
	return vm_push_nil(vm, error);
    }

    /* Evaluate all but the last argument */
    while (argc > 1)
    {
	if (! vm_unroll(vm, argc - 1, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_pop(vm, NULL, error))
	{
	    return 0;
	}

	argc--;
    }

    /* Evaluate the last argument and return its value */
    return vm_eval(vm, error);
}

/* The `setq' special form */
static int prim_setq(vm_t vm, uint32_t argc, elvin_error_t error)
{
    uint32_t i;

    /* Zero arguments evaluates to `nil' */
    if (argc == 0)
    {
	return vm_push_nil(vm, error);
    }

    /* FIX THIS: setq should allow any even number of args */
    if (argc % 2 != 0)
    {
	ELVIN_ERROR_INTERP_WRONG_ARGC(error, "setq", argc);
	return 0;
    }

    /* Evaluate all args but the last two */
    for (i = argc - 1; i > 1; i -= 2)
    {
	if (! vm_unroll(vm, i, error) ||
	    ! vm_unroll(vm, i, error) ||
	    ! vm_eval(vm, error) ||
	    ! vm_assign(vm, error) ||
	    ! vm_pop(vm, NULL, error))
	{
	    return 0;
	}
    }

    /* Evaluate the last two arguments */
    return vm_eval(vm, error) && vm_assign(vm, error);
}


/* Defines a subroutine in the root environment */
static int define_subr(vm_t vm, char *name, prim_t func, elvin_error_t error)
{
    return
	vm_push_symbol(vm, name, error) &&
	vm_push_subr(vm, func, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error);
}

/* Defines a special form */
static int define_special(vm_t vm, char *name, prim_t func, elvin_error_t error)
{
    return
	vm_push_symbol(vm, name, error) &&
	vm_push_special_form(vm, func, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error);
}

/* Populates the root environment */
static int populate_env(tickertape_t self, elvin_error_t error)
{
    vm_t vm = self -> vm;

    /* Populate the root environment */
    return 
	vm_push_string(vm, "nil", error) &&
	vm_make_symbol(vm, error) &&
	vm_push_nil(vm, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error) &&

	vm_push_string(vm, "t", error) &&
	vm_make_symbol(vm, error) &&
	vm_dup(vm, error) &&
	vm_assign(vm, error) &&
	vm_pop(vm, NULL, error) &&

	define_special(vm, "and", prim_and, error) &&
	define_subr(vm, "car", prim_car, error) &&
#if 0
	define_special(vm, "catch", prim_catch, error) &&
#endif
	define_subr(vm, "cdr", prim_cdr, error) &&
	define_special(vm, "cond", prim_cond, error) &&
	define_subr(vm, "cons", prim_cons, error) &&
	define_special(vm, "define", prim_define, error) &&
	define_special(vm, "defun", prim_defun, error) &&
	define_subr(vm, "eq", prim_eq, error) &&
	define_special(vm, "if", prim_if, error) &&
#if 0
	define_subr(vm, "format", prim_format, error) &&
#endif
	define_subr(vm, "gc", prim_gc, error) &&
	define_special(vm, "lambda", prim_lambda, error) &&
	define_special(vm, "let", prim_let, error) &&
	define_subr(vm, "list", prim_list, error) &&
	define_special(vm, "or", prim_or, error) &&
	define_subr(vm, "+", prim_plus, error) &&
	define_special(vm, "progn", prim_progn, error) &&
	define_special(vm, "quote", prim_quote, error) &&
	define_special(vm, "setq", prim_setq, error);
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
    char *config_file,
    char *groups_file,
    char *usenet_file,
    Widget top,
    elvin_error_t error)
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
    self -> config_file = (config_file == NULL) ? NULL : strdup(config_file);
    self -> groups_file = (groups_file == NULL) ? NULL : strdup(groups_file);
    self -> usenet_file = (usenet_file == NULL) ? NULL : strdup(usenet_file);
    self -> top = top;
    self -> groups = NULL;
    self -> groups_count = 0;
    self -> usenet_sub = NULL;
    self -> mail_sub = NULL;
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
    self -> mail_sub = mail_sub_alloc(user, receive_callback, self);

    /* Draw the user interface */
    init_ui(self);

    /* Initialize the scheme virtual machine */
    if ((self -> vm = vm_alloc(error)) == NULL)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Populate its root environment */
    if (populate_env(self, error) == 0)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Allocate a new parser */
    if ((self -> parser = parser_alloc(self -> vm, parsed, self, error)) == NULL)
    {
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

    /* Add an I/O handler for stdin */
    XtAppAddInput(
	XtWidgetToApplicationContext(self -> top),
	STDIN_FILENO,
	(XtPointer)XtInputReadMask,
	interp_cb, self);
    printf("> "); fflush(stdout);

    /* Set the handle's status callback */
    handle -> status_cb = status_cb;
    handle -> status_closure = self;

    /* Connect to the elvin server */
    if (elvin_xt_connect(handle, connect_cb, self, self -> error) == 0)
    {
	fprintf(stderr, PACKAGE ": elvin_xt_connect(): failed\n");
	elvin_error_fprintf(stderr, error);
	exit(1);
    }

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

    if (self -> config_file != NULL)
    {
	free(self -> config_file);
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

#if 0
/* Answers the receiver's config file filename */
static char *tickertape_config_filename(tickertape_t self)
{
    if (self -> config_file == NULL)
    {
	char *dir = tickertape_ticker_dir(self);
	self -> config_file = (char *)malloc(strlen(dir) + sizeof(DEFAULT_CONFIG_FILE) + 1);
	sprintf(self -> config_file, "%s/%s", dir, DEFAULT_CONFIG_FILE);
    }

    return self -> config_file;
}
#endif

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
    size_t mime_length;
    char *pointer;
    char *end;
    pid_t pid;
    int fds[2];
    int status;

    /* If the message has no attachment then we're done */
    if (((mime_type = message_get_mime_type(message)) == NULL) ||
	((mime_length = message_get_mime_args(message, &mime_args)) == 0))
    {
#ifdef DEBUG
	printf("no attachment\n");
#endif /* DEBUG */
	return -1;
    }

#ifdef DEBUG
    printf("attachment: \"%s\" \"%s\"\n", mime_type, mime_args);
#endif /* DEBUG */

    /* Create a pipe to send the file to metamail */
    if (pipe(fds) < 0)
    {
	perror("pipe(): failed");
	return -1;
    }

    /* Fork a child process to invoke metamail */
    if ((pid = fork()) == (pid_t)-1)
    {
	perror("fork(): failed");
	close(fds[0]);
	close(fds[1]);
	return -1;
    }

    /* See if we're the child process */
    if (pid == 0)
    {
	/* Use the pipe as stdin */
	dup2(fds[0], STDIN_FILENO);
	close(fds[1]);
	close(fds[0]);

	/* Invoke metamail */
	execl(
	    METAMAIL, METAMAIL,
	    METAMAIL_OPTIONS,
	    "-c", mime_type,
	    NULL);

	/* We'll only get here if exec fails */
	perror("execl(): failed");
	exit(1);
    }

    /* We're the parent process. */
    if (close(fds[0]) < 0)
    {
	perror("close(): failed");
	return -1;
    }

    /* Write the mime args into the pipe */
    end = mime_args + mime_length;
    pointer = mime_args;
    while (pointer < end)
    {
	ssize_t length;

	/* Write as much as we can */
	if ((length = write(fds[1], pointer, end - pointer)) < 0)
	{
	    perror("unable to write to temporary file");
	    return -1;
	}

	pointer += length;
    }

    /* Make sure it gets written */
    if (close(fds[1]) < 0)
    {
	perror("close(): failed");
	return -1;
    }

    /* Wait for the child process to exit */
    if (waitpid(pid, &status, 0) == (pid_t)-1)
    {
	perror("waitpid(): failed");
	return -1;
    }

    /* Did the child process exit nicely? */
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
	return 0;
    }
#ifdef DEBUG
    /* Did it exit badly? */
    if (WIFEXITED(status))
    {
	fprintf(stderr, METAMAIL " exit status: %d\n", WEXITSTATUS(status));
	return -1;
    }

    fprintf(stderr, METAMAIL " died badly\n");
#endif
#endif /* METAMAIL */

    return -1;
}
