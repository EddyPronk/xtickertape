/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2003.
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
static const char cvsid[] = "$Id: tickertape.c,v 1.107 2003/01/28 15:58:47 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h> /* fclose, fopen, fprintf, fputc, fputs, perror, snprintf */
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* exit, free, getenv, malloc, realloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* strcat, strcmp, strcpy, strdup, strlen, strrchr */
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h> /* fork, mkdir, open, stat, waitpid */
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h> /* mkdir, open, stat */
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h> /* waitpid */
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned int)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 0xFF) == 0)
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h> /* open */
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h> /* close, dup2, execl, fork, pipe, read, stat, write */
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h> /* errno */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include <elvin/error.h>
#include "replace.h"
#include "errors.h"
#include "message.h"
#include "tickertape.h"
#include "Scroller.h"
#include "panel.h"

#ifdef ENABLE_LISP_INTERPRETER
#include "vm.h"
#include "parser.h"
#endif /* ENABLE_LISP_INTERPRETER */

#include "keys.h"
#include "key_table.h"
#include "keys_parser.h"
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
#define DEFAULT_KEYS_FILE "keys"

#define METAMAIL_OPTIONS "-x", "-B", "-q"

/* How long to wait before we tell the user we're having trouble connecting */
#define BUFFER_SIZE 1024

#define CONNECT_MSG "Connected to elvin server: %s"
#define LOST_CONNECT_MSG "Lost connection to elvin server %s"
#define PROTOCOL_ERROR_MSG "Protocol error encountered with server: %s"
#define UNKNOWN_STATUS_MSG "Unknown status: %d"
#define DROP_WARN_MSG "One or more packets were dropped"

/* compatability code for status callback */
#if ! defined(ELVIN_VERSION_AT_LEAST)
# define CONN_CLOSED_MSG "Connection closed by server: %s"
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
# define CONN_CLOSED_MSG "Connection closed by server"
#else
# error unknown elvin library version
#endif

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
#  define ELVIN_RETURN_FAILURE
#  define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
#  define ELVIN_RETURN_FAILURE 0
#  define ELVIN_RETURN_SUCCESS 1
#endif

#define GROUP_SUB "TICKERTAPE == \"%s\" || Group == \"%s\""

#define F_USER "user"

/* The tickertape data type */
struct tickertape
{
    /* A convenient error context */
    elvin_error_t error;

    /* The application-shell resources */
    XTickertapeRec *resources;

    /* The elvin connection handle */
    elvin_handle_t handle;

#ifdef ENABLE_LISP_INTERPRETER
    /* The interpreter's virtual machine */
    vm_t vm;

    /* The interpreter's parser */
    parser_t parser;
#endif /* ENABLE_LISP_INTERPRETER */

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

    /* The keys file from which we read ours and others' key locations */
    char *keys_file;

    /* The directory in which to start looking for key files with relative paths */
    char *keys_dir;


    /* The top-level widget */
    Widget top;

    /* The receiver's groups subscriptions (from the groups file) */
    group_sub_t *groups;

    /* The number of groups subscriptions the receiver has */
    int groups_count;

    /* The receiver's usenet subscription (from the usenet file) */
    usenet_sub_t usenet_sub;

    /* The keys available for use */
    key_table_t keys;

    /* The receiver's mail subscription */
    mail_sub_t mail_sub;

    /* The control panel */
    control_panel_t control_panel;

    /* The ScrollerWidget */
    Widget scroller;
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
static int parse_keys_file(tickertape_t self);
static void init_ui(tickertape_t self);
static char *tickertape_ticker_dir(tickertape_t self);
#if 0
static char *tickertape_config_filename(tickertape_t self);
#endif
static char *tickertape_groups_filename(tickertape_t self);
static char *tickertape_usenet_filename(tickertape_t self);
static char *tickertape_keys_filename(tickertape_t self);
static char *tickertape_keys_directory(tickertape_t self);


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
#if defined(HAVE_MKDIR)
	return mkdir(dirname, 0777);
#else
	fprintf(stderr, "Please create the directory %s\n", dirname);
	exit(1);
#endif
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

    /* Delegate to the control panel */
    control_panel_kill_thread(self -> control_panel, message);

    /* Clean the killed messages out of the scroller */
    ScPurgeKilled(self -> scroller);
}

/* Receive a message_t matched by a subscription */
static void receive_callback(void *rock, message_t message, int show_attachment)
{
    tickertape_t self = (tickertape_t)rock;

    /* Add the message to the control panel.  This will mark it as
     * killed if it is added to a thread which has been killed */
    control_panel_add_message(self -> control_panel, message);

    /* Add the message to the scroller if it hasn't been killed */
    if (! message_is_killed(message))
    {
	ScAddMessage(self -> scroller, message);

	/* Show the attachment if requested */
	if (show_attachment)
	{
	    tickertape_show_attachment(self, message);
	}
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

		/* home directory */
		case 'h':
		{
		    fputs(getenv("HOME"), out);
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
    elvin_keys_t notification_keys,
    elvin_keys_t subscription_keys)
{
    size_t length;
    char *expression;
    group_sub_t subscription;

    /* Construct the subscription expression */
    length = strlen(GROUP_SUB) + 2 * strlen(name) - 3;
    if ((expression = (char *)malloc(length)) == NULL)
    {
	return -1;
    }

    snprintf(expression, length, GROUP_SUB, name, name);

    /* Allocate us a subscription */
    if ((subscription = group_sub_alloc(
	    name, expression,
	    in_menu, has_nazi,
	    min_time, max_time,
	    notification_keys, subscription_keys,
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
	self, filename, self -> keys)) == NULL)
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

/* The callback for the keys file parser */
static int parse_keys_callback(
    void *rock, char *name,
    char *data, int length,
    int is_private)
{
    tickertape_t self = (tickertape_t)rock;

    /* Check whether a key with that name already exists */
    if (key_table_lookup(self -> keys, name, NULL, NULL, NULL) == 0)
    {
	return -1;
    }

    /* Add the key to the table */
    if (key_table_add(self -> keys, name, data, length, is_private) < 0)
    {
	return -1;
    }

    return 0;
}

/* Parse the keys file and update the keys table accordingly */
static int parse_keys_file(tickertape_t self)
{
    char *filename = tickertape_keys_filename(self);
    keys_parser_t parser;
    int fd;

    /* Allocate a new keys file parser */
    if ((parser = keys_parser_alloc(
	     tickertape_keys_directory(self),
	     parse_keys_callback, self,
	     filename)) == NULL)
    {
	return -1;
    }

    /* Allocate a new key table */
    if ((self -> keys = key_table_alloc()) == NULL)
    {
	keys_parser_free(parser);
	return -1;
    }

    /* Make sure we can read the keys file */
    if ((fd = open_config_file(self, filename, default_keys_file)) < 0)
    {
	keys_parser_free(parser);
	return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    while (1)
    {
	char buffer[BUFFER_SIZE];
	ssize_t length;

	/* Read a chunk of the file */
	if ((length = read(fd, buffer, BUFFER_SIZE)) < 0)
	{
	    close(fd);
	    keys_parser_free(parser);
	    return -1;
	}

	/* Send it to the parser */
	if (keys_parser_parse(parser, buffer, length) < 0)
	{
	    close(fd);
	    keys_parser_free(parser);
	    return -1;
	}

	/* Watch for the end-of-file */
	if (length == 0)
	{
	    close(fd);
	    keys_parser_free(parser);
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
	if (*pointer != NULL && strcmp(group_sub_expression(*pointer), expression) == 0)
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

/* Request from the control panel to reload the keys file */
void tickertape_reload_keys(tickertape_t self)
{
    /* Release the old keys table */
    if (self -> keys != NULL)
    {
	key_table_free(self -> keys);
	self -> keys = NULL;
    }

    /* Try to read in the new one */
    if (parse_keys_file(self) < 0)
    {
	/* FIX THIS: do something? */
	return;
    }
}

/* Initializes the User Interface */
static void init_ui(tickertape_t self)
{
    int index;

    /* Allocate the control panel */
    if ((self -> control_panel = control_panel_alloc(
	     self, self -> top,
	     self -> resources -> send_history_count)) == NULL)
    {
	return;
    }

    /* Tell our group subscriptions about the control panel */
    for (index = 0; index < self -> groups_count; index++)
    {
	group_sub_set_control_panel(self -> groups[index], self -> control_panel);
    }

    /* Create the scroller widget */
    self -> scroller = XtVaCreateManagedWidget(
	"scroller", scrollerWidgetClass, self -> top,
	NULL);
    XtAddCallback(
	self -> scroller, XtNcallback,
	(XtCallbackProc)menu_callback, self);
    XtAddCallback(
	self -> scroller, XtNattachmentCallback,
	(XtCallbackProc)mime_callback, self);
    XtAddCallback(
	self -> scroller, XtNkillCallback,
	(XtCallbackProc)kill_callback, self);
    XtRealizeWidget(self -> top);
}

/* This is called when our connection request is handled */
static 
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4,1,-1)
int
#endif
connect_cb(elvin_handle_t handle, int result,
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

    return ELVIN_RETURN_SUCCESS; 
}


#if ! defined(ELVIN_VERSION_AT_LEAST)
/* Callback for elvin status changes */
static void status_cb(
    elvin_handle_t handle, char *url,
    elvin_status_event_t event,
    void *rock,
    elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    message_t message;
    size_t length;
    char *buffer = NULL;
    char *string;

    /* Construct an appropriate message string */
    switch (event)
    {
	/* We were unable to (re)connect */
	case ELVIN_STATUS_CONNECTION_FAILED:
	{
	    fprintf(stderr, PACKAGE ": unable to connect\n");
	    elvin_error_fprintf(stderr, error);
	    exit(1);
	}

	case ELVIN_STATUS_CONNECTION_FOUND:
	{
	    /* Tell the control panel that we're connected */
	    control_panel_set_connected(self -> control_panel, True);
	    
	    /* Make room for a combined string and URL */
	    length = strlen(CONNECT_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_LOST:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a combined string and URL */
	    length = strlen(LOST_CONNECT_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, LOST_CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_CLOSED:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a message string */
	    length = strlen(CONN_CLOSED_MSG) + 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, CONN_CLOSED_MSG, url);
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
	    length = strlen(PROTOCOL_ERROR_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, PROTOCOL_ERROR_MSG, url);
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
	    length = sizeof(UNKNOWN_STATUS_MSG) + 16;
	    buffer = (char *)malloc(length);
	    snprintf(buffer, length, UNKNOWN_STATUS_MSG, event);
	    string = buffer;
	    break;
	}
    }

    /* Construct a message for the string and add it to the scroller */
    if ((message = message_alloc(
	NULL, "internal", "tickertape", string, 30,
	NULL, 0, NULL, NULL, NULL, NULL)) != NULL)
    {
	receive_callback(self, message, False);
	message_free(message);
    }

    /* Also display the message on the status line */
    control_panel_set_status(self -> control_panel, buffer);

    /* Clean up */
    if (buffer != NULL)
    {
	free(buffer);
    }
}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* Callback for elvin status changes */
static int status_cb(
    elvin_handle_t handle,
    elvin_status_event_t event,
    void *rock,
    elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    message_t message;
    size_t length;
    char *buffer = NULL;
    char *string;

    /* Construct an appropriate message string */
    switch (event -> type)
    {
	/* We were unable to (re)connect */
	case ELVIN_STATUS_CONNECTION_FAILED:
	{
	    fprintf(stderr, PACKAGE ": unable to connect\n");
	    elvin_error_fprintf(stderr, event -> details.connection_failed.error);
	    exit(1);
	}

	case ELVIN_STATUS_CONNECTION_FOUND:
	{
	    char *url;

	    /* Stringify the URL */
	    if ((url = elvin_url_get_canonical(event -> details.connection_found.url, error)) == NULL)
	    {
		fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    /* Tell the control panel that we're connected */
	    control_panel_set_connected(self -> control_panel, True);
	    
	    /* Make room for a combined string and URL */
	    length = strlen(CONNECT_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_LOST:
	{
	    char *url;

	    /* Stringify the URL */
	    if ((url = elvin_url_get_canonical(event -> details.connection_lost.url, error)) == NULL)
	    {
		fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a combined string and URL */
	    length = strlen(LOST_CONNECT_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, LOST_CONNECT_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_CONNECTION_CLOSED:
	{
	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a message string */
	    length = strlen(CONN_CLOSED_MSG) + 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, CONN_CLOSED_MSG);
	    string = buffer;
	    break;
	}

	/* Connection warnings go to the status line */
	case ELVIN_STATUS_CONNECTION_WARN:
	{
	    /* Get a big buffer */
	    if ((buffer = (char *)malloc(BUFFER_SIZE)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }
	    
	    /* Print the error message into it */
	    elvin_error_snprintf(buffer, BUFFER_SIZE, event -> details.connection_warn.error);
	    string = buffer;

	    /* Display it on the status line */
	    control_panel_set_status(self -> control_panel, buffer);

	    /* Clean up */
	    free(buffer);
	    return 1;
	}

	case ELVIN_STATUS_DROP_WARN:
	{
	    string = DROP_WARN_MSG;
	    break;
	}

	case ELVIN_STATUS_PROTOCOL_ERROR:
	{
	    char *url;

	    /* Stringify the URL */
	    if ((url = elvin_url_get_canonical(event -> details.protocol_error.url, error)) == NULL)
	    {
		fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
		elvin_error_fprintf(stderr, error);
		exit(1);
	    }

	    /* Tell the control panel that we're no longer connected */
	    control_panel_set_connected(self -> control_panel, False);

	    /* Make room for a message string */
	    length = strlen(PROTOCOL_ERROR_MSG) + strlen(url) - 1;
	    if ((buffer = (char *)malloc(length)) == NULL)
	    {
		perror(PACKAGE ": malloc() failed");
		exit(1);
	    }

	    snprintf(buffer, length, PROTOCOL_ERROR_MSG, url);
	    string = buffer;
	    break;
	}

	case ELVIN_STATUS_IGNORED_ERROR:
	{
	    fprintf(stderr, "%s: status ignored\n", PACKAGE);
	    elvin_error_fprintf(stderr, event -> details.ignored_error.error);
	    exit(1);
	}

	case ELVIN_STATUS_CLIENT_ERROR:
	{
	    fprintf(stderr, "%s: client error\n", PACKAGE);
	    elvin_error_fprintf(stderr, event -> details.client_error.error);
	    exit(1);
	}

	default:
	{
	    length = sizeof(UNKNOWN_STATUS_MSG) + 16;
	    buffer = (char *)malloc(length);
	    snprintf(buffer, length, UNKNOWN_STATUS_MSG, event -> type);
	    string = buffer;
	    break;
	}
    }

    /* Construct a message for the string and add it to the scroller */
    if ((message = message_alloc(
	NULL, "internal", "tickertape", string, 30,
	NULL, 0, NULL, NULL, NULL, NULL)) != NULL)
    {
	receive_callback(self, message, False);
	message_free(message);
    }

    /* Also display the message on the status line */
    control_panel_set_status(self -> control_panel, buffer);

    /* Clean up */
    if (buffer != NULL)
    {
	free(buffer);
    }

    return 1;
}
#else
#error "Unsupported Elvin library version"
#endif

#ifdef ENABLE_LISP_INTERPRETER
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
#endif /* ENABLE_LISP_INTERPRETER */

/*
 *
 * Exported function definitions 
 *
 */

/* Answers a new tickertape_t for the given user using the given file as
 * her groups file and connecting to the notification service */
tickertape_t tickertape_alloc(
    XTickertapeRec *resources,
    elvin_handle_t handle,
    char *user, char *domain, 
    char *ticker_dir,
    char *config_file,
    char *groups_file,
    char *usenet_file,
    char *keys_file,
    char *keys_dir,
    Widget top,
    elvin_error_t error)
{
    tickertape_t self;

    /* Allocate some space for the new tickertape */
    if ((self = (tickertape_t)malloc(sizeof(struct tickertape))) == NULL)
    {
	return NULL;
    }

    /* Initialize its contents to sane values */
    self -> resources = resources;
    self -> error = error;
    self -> handle = NULL;
    self -> user = strdup(user);
    self -> domain = strdup(domain);
    self -> ticker_dir = (ticker_dir == NULL) ? NULL : strdup(ticker_dir);
    self -> config_file = (config_file == NULL) ? NULL : strdup(config_file);
    self -> groups_file = (groups_file == NULL) ? NULL : strdup(groups_file);
    self -> usenet_file = (usenet_file == NULL) ? NULL : strdup(usenet_file);
    self -> keys_file = (keys_file == NULL) ? NULL : strdup(keys_file);
    self -> keys_dir = (keys_dir == NULL) ? NULL : strdup(keys_dir);
    self -> top = top;
    self -> groups = NULL;
    self -> groups_count = 0;
    self -> usenet_sub = NULL;
    self -> mail_sub = NULL;
    self -> control_panel = NULL;
    self -> scroller = NULL;

    /* Read the keys from the keys file */
    if (parse_keys_file(self) < 0)
    {
	exit(1);
    }

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

#ifdef ENABLE_LISP_INTERPRETER
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
#endif /* ENABLE_LISP_INTERPRETER */

    /* Set the handle's status callback */
    if (! elvin_handle_set_status_cb(handle, status_cb, self, self -> error))
    {
        fprintf(stderr, PACKAGE ": elvin_handle_set_status_cb(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Connect to the elvin server */
    if (elvin_async_connect(handle, connect_cb, self, self -> error) == 0)
    {
	fprintf(stderr, PACKAGE ": elvin_async_connect(): failed\n");
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

    if (self -> keys_file != NULL)
    {
	free(self -> keys_file);
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

    if (self -> keys != NULL)
    {
	key_table_free(self -> keys);
    }

    if (self -> control_panel)
    {
	control_panel_free(self -> control_panel);
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

/* Show the previous item in the history */
void tickertape_history_prev(tickertape_t self)
{
    control_panel_history_prev(self -> control_panel);
}

/* Show the next item in the history */
void tickertape_history_next(tickertape_t self)
{
    control_panel_history_next(self -> control_panel);
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
    size_t length;

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
	length = strlen(dir) + strlen(DEFAULT_TICKERDIR) + 2;
	self -> ticker_dir = (char *)malloc(length);
	snprintf(self -> ticker_dir, length, "%s/%s", dir, DEFAULT_TICKERDIR);
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
	size_t length = strlen(dir) + strlen(DEFAULT_GROUPS_FILE) + 2;

	if ((self -> groups_file = (char *)malloc(length)) == NULL)
	{
	    perror("unable to allocate memory");
	    exit(1);
	}

	snprintf(self -> groups_file, length, "%s/%s", dir, DEFAULT_GROUPS_FILE);
    }

    return self -> groups_file;
}

/* Answers the receiver's usenet filename */
static char *tickertape_usenet_filename(tickertape_t self)
{
    if (self -> usenet_file == NULL)
    {
	char *dir = tickertape_ticker_dir(self);
	size_t length = strlen(dir) + strlen(DEFAULT_USENET_FILE) + 2;

	if ((self -> usenet_file = (char *)malloc(length)) == NULL)
	{
	    perror("unable to allocate memory");
	    exit(1);
	}

	snprintf(self -> usenet_file, length, "%s/%s", dir, DEFAULT_USENET_FILE);
    }

    return self -> usenet_file;
}

/* Answers the receiver's keys filename */
static char *tickertape_keys_filename(tickertape_t self)
{
    if (self -> keys_file == NULL)
    {
	char *dir = tickertape_ticker_dir(self);
	size_t length;

	length = strlen(dir) + sizeof(DEFAULT_KEYS_FILE) + 1;
	if ((self -> keys_file = (char *)malloc(length)) == NULL)
	{
	    perror("unable to allocate memory");
	    exit(1);
	}

	snprintf(self -> keys_file, length, "%s/%s", dir, DEFAULT_KEYS_FILE);
    }

    return self -> keys_file;
}

/* Answers the receiver's keys directory */
static char *tickertape_keys_directory(tickertape_t self)
{
    if (self -> keys_dir == NULL)
    {
	char *file = tickertape_keys_filename(self);
	char *point;
	size_t length;

	/* Default to the directory name from the keys filename */
	if ((point = strrchr(file, '/')) == NULL)
	{
	    self -> keys_dir = strdup("");
	}
	else
	{
	    length = point - file;
	    if ((self -> keys_dir = (char *)malloc(length + 1)) == NULL)
	    {
		perror("Unable to allocate memory");
		exit(1);
	    }

	    memcpy(self -> keys_dir, file, length);
	    self -> keys_dir[length] = 0;
	}
    }

    return self -> keys_dir;
}


/* Displays a message's MIME attachment */
int tickertape_show_attachment(tickertape_t self, message_t message)
{
#if defined(HAVE_DUP2) && defined(HAVE_FORK)
    char *attachment;
    size_t count;
    char *pointer;
    char *end;
    pid_t pid;
    int fds[2];
    int status;

    /* If metamail is not defined then we're done */
    if (self -> resources -> metamail_path == NULL ||
	*self -> resources -> metamail_path == '\0')
    {
#if defined(DEBUG)
	printf("metamail not defined\n");
#endif /* DEBUG */
	return -1;
    }

    /* If the message has no attachment then we're done */
    if ((count = message_get_attachment(message, &attachment)) == 0)
    {
#if defined(DEBUG)
	printf("no attachment\n");
#endif /* DEBUG */
	return -1;
    }

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
	    self -> resources -> metamail_path,
	    self -> resources -> metamail_path,
	    METAMAIL_OPTIONS,
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
    end = attachment + count;
    pointer = attachment;
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
#if defined(DEBUG)
    /* Did it exit badly? */
    if (WIFEXITED(status))
    {
	fprintf(stderr, "%s exit status: %d\n",
		self -> resources -> metamail_path,
		WEXITSTATUS(status));
	return -1;
    }

    fprintf(stderr, METAMAIL " died badly\n");
#endif
#endif /* METAMAIL & DUP2 & FORK */

    return -1;
}
