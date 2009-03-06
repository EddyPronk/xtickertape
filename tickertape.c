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
#include <stdio.h> /* fclose, fopen, fprintf, fputc, fputs, perror, snprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* exit, free, getenv, malloc, realloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* strcat, strcmp, strcpy, strdup, strlen, strrchr */
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h> /* fork, mkdir, open, stat, waitpid */
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h> /* mkdir, open, stat */
#endif
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h> /* waitpid */
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned int)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 0xFF) == 0)
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h> /* open */
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h> /* close, dup2, execlp, fork, pipe, read, stat, write */
#endif
#ifdef HAVE_ERRNO_H
# include <errno.h> /* errno */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include <elvin/error.h>
#include "replace.h"
/*#include "errors.h"*/
#include "message.h"
#include "tickertape.h"
#include "Scroller.h"
#include "panel.h"

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
#define NO_DISCOVERY_MSG "Unable to use service discovery"
#define UNKNOWN_STATUS_MSG "Unknown status: %d"
#define DROP_WARN_MSG "One or more packets were dropped"

/* compatability code for status callback */
#if !defined(ELVIN_VERSION_AT_LEAST)
# define CONN_CLOSED_MSG "Connection closed by server: %s"
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
# define CONN_CLOSED_MSG "Connection closed by server"
#else
# error unknown elvin library version
#endif

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
# define ELVIN_RETURN_FAILURE
# define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
# define ELVIN_RETURN_FAILURE 0
# define ELVIN_RETURN_SUCCESS 1
#endif

#define GROUP_SUB "TICKERTAPE == \"%s\" || Group == \"%s\""

#define F_USER "user"

/* The tickertape data type */
struct tickertape {
    /* A convenient error context */
    elvin_error_t error;

    /* The application-shell resources */
    XTickertapeRec *resources;

    /* The elvin connection handle */
    elvin_handle_t handle;

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

    /* The directory in which to start looking for key files with
     * relative paths */
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
static int
mkdirhier(char *dirname);
static void
menu_callback(Widget widget, XtPointer closure, XtPointer call_data);
static void
receive_callback(void *rock, message_t message, int show_attachment);
static int
parse_groups_file(tickertape_t self);
static int
parse_keys_file(tickertape_t self);
static void
init_ui(tickertape_t self);
static const char *
tickertape_ticker_dir(tickertape_t self);


static const char *
tickertape_groups_filename(tickertape_t self);
static const char *
tickertape_usenet_filename(tickertape_t self);
static const char *
tickertape_keys_filename(tickertape_t self);
static const char *
tickertape_keys_directory(tickertape_t self);


/*
 *
 * Static functions
 *
 */


/* Makes the named directory and any parent directories needed */
static int
mkdirhier(char *dirname)
{
    char *pointer;
    struct stat statbuf;

    /* End the recursion */
    if (*dirname == '\0') {
        return 0;
    }

    /* Find the last '/' in the directory name */
    pointer = strrchr(dirname, '/');
    if (pointer != NULL) {
        int result;

        /* Call ourselves recursively to make sure the parent
         * directory exists */
        *pointer = '\0';
        result = mkdirhier(dirname);
        *pointer = '/';

        /* If we couldn't create the parent directory, then fail */
        if (result < 0) {
            return result;
        }
    }

    /* If a file named `dirname' exists then make sure it's a directory */
    if (stat(dirname, &statbuf) == 0) {
        if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
            errno = EEXIST;
            return -1;
        }

        return 0;
    }

    /* If the directory doesn't exist then try to create it */
    if (errno == ENOENT) {
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
static void
menu_callback(Widget widget, XtPointer closure, XtPointer call_data)
{
    tickertape_t self = (tickertape_t)closure;
    message_t message = (message_t)call_data;

    control_panel_select(self->control_panel, message);
    control_panel_show(self->control_panel);
}

/* Callback for an action() action in the Scroller */
static void
mime_callback(Widget widget, XtPointer closure, XtPointer call_data)
{
    tickertape_t self = (tickertape_t)closure;
    message_t message = (message_t)call_data;

    /* Bail if no message provided */
    if (message == NULL) {
        return;
    }

    /* Show the message's MIME attachment */
    tickertape_show_attachment(self, message);
}

/* Callback for a kill() action in the Scroller */
static void
kill_callback(Widget widget, XtPointer closure, XtPointer call_data)
{
    tickertape_t self = (tickertape_t)closure;
    message_t message = (message_t)call_data;

    if (message == NULL) {
        return;
    }

    /* Delegate to the control panel */
    control_panel_kill_thread(self->control_panel, message);

    /* Clean the killed messages out of the scroller */
    ScPurgeKilled(self->scroller);
}

/* Receive a message_t matched by a subscription */
static void
receive_callback(void *rock, message_t message, int show_attachment)
{
    tickertape_t self = (tickertape_t)rock;

    /* Add the message to the control panel.  This will mark it as
     * killed if it is added to a thread which has been killed */
    control_panel_add_message(self->control_panel, message);

    /* Add the message to the scroller if it hasn't been killed */
    if (!message_is_killed(message)) {
        ScAddMessage(self->scroller, message);

        /* Show the attachment if requested */
        if (show_attachment) {
            tickertape_show_attachment(self, message);
        }
    }
}

/* Write the template to the given file, doing some substitutions */
static int
write_default_file(tickertape_t self, FILE *out, const char *template)
{
    const char *pointer;

    for (pointer = template; *pointer != '\0'; pointer++) {
        if (*pointer == '%') {
            switch (*(++pointer)) {
            case '\0':
                /* Don't freak out on a terminal `%' */
                fputc(*pointer, out);
                return 0;

            case 'u':
                /* user name */
                fputs(self->user, out);
                break;

            case 'd':
                /* domain name */
                fputs(self->domain, out);
                break;

            case 'h':
                /* home directory */
                fputs(getenv("HOME"), out);
                break;

            default:
                /* Anything else */
                fputc('%', out);
                fputc(*pointer, out);
                break;
            }
        } else {
            fputc(*pointer, out);
        }
    }

    return 0;
}

/* Open a configuration file.  If the file doesn't exist, try to
 * create it and fill it with the default groups file information.
 * Returns the file descriptor of the open config file on success,
 * -1 on failure */
static int
open_config_file(tickertape_t self, const char *filename, const char *template)
{
    int fd;
    FILE *out;

    /* Try to just open the file */
    fd = open(filename, O_RDONLY);
    if (fd >= 0) {
        return fd;
    }

    /* If the file exists, then bail now */
    if (errno != ENOENT) {
        return -1;
    }

    /* Otherwise, try to create a default groups file */
    out = fopen(filename, "w");
    if (out == NULL) {
        return -1;
    }

    /* Write the default config file */
    if (write_default_file(self, out, template) < 0) {
        fclose(out);
        return -1;
    }

    fclose(out);

    /* Try to open the file again */
    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("unable to read from groups file");
        return -1;
    }

    /* Success */
    return fd;
}

/* The callback for the groups file parser */
static int
parse_groups_callback(void *rock,
                      const char *name,
                      int in_menu,
                      int has_nazi,
                      int min_time,
                      int max_time,
                      char *const *key_names,
                      int key_count)
{
    tickertape_t self = (tickertape_t)rock;
    size_t length;
    char *expression;
    group_sub_t subscription;

    /* Construct the subscription expression */
    length = strlen(GROUP_SUB) + 2 * strlen(name) - 3;
    expression = malloc(length);
    if (expression == NULL) {
        return -1;
    }

    snprintf(expression, length, GROUP_SUB, name, name);

    /* Allocate us a subscription */
    subscription = group_sub_alloc(name, expression,
                                   in_menu, has_nazi,
                                   min_time * 60, max_time * 60,
                                   self->keys, key_names, key_count,
                                   receive_callback, self);
    if (subscription == NULL) {
        return -1;
    }

    /* Add it to the end of the array */
    self->groups = realloc(self->groups,
                           sizeof(group_sub_t) * (self->groups_count + 1));
    self->groups[self->groups_count++] = subscription;

    /* Clean up */
    free(expression);
    return 0;
}

/* Parse the groups file and update the groups table accordingly */
static int
parse_groups_file(tickertape_t self)
{
    const char *filename = tickertape_groups_filename(self);
    groups_parser_t parser;
    int fd;

    /* Allocate a new groups file parser */
    parser = groups_parser_alloc(parse_groups_callback, self, filename);
    if (parser == NULL) {
        return -1;
    }

    /* Make sure we can read the groups file */
    fd = open_config_file(self, filename, default_groups_file);
    if (fd < 0) {
        groups_parser_free(parser);
        return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    for (;;) {
        char buffer[BUFFER_SIZE];
        ssize_t length;

        /* Read from the file */
        length = read(fd, buffer, BUFFER_SIZE);
        if (length < 0) {
            close(fd);
            groups_parser_free(parser);
            return -1;
        }

        /* Send it to the parser */
        if (groups_parser_parse(parser, buffer, length) < 0) {
            close(fd);
            groups_parser_free(parser);
            return -1;
        }

        /* Watch for end-of-file */
        if (length == 0) {
            close(fd);
            groups_parser_free(parser);
            return 0;
        }
    }
}

/* The callback for the usenet file parser */
static int
parse_usenet_callback(tickertape_t self,
                      int has_not,
                      const char *pattern,
                      struct usenet_expr *expressions,
                      size_t count)
{
    /* Forward the information to the usenet subscription */
    return usenet_sub_add(self->usenet_sub, has_not, pattern,
                          expressions, count);
}

/* Parse the usenet file and update the usenet subscription accordingly */
static int
parse_usenet_file(tickertape_t self)
{
    const char *filename = tickertape_usenet_filename(self);
    usenet_parser_t parser;
    int fd;

    /* Allocate a new usenet file parser */
    parser = usenet_parser_alloc(
        (usenet_parser_callback_t)parse_usenet_callback,
        self, filename);
    if (parser == NULL) {
        return -1;
    }

    /* Allocate a new usenet subscription */
    self->usenet_sub = usenet_sub_alloc(receive_callback, self);
    if (self->usenet_sub == NULL) {
        usenet_parser_free(parser);
        return -1;
    }

    /* Make sure we can read the usenet file */
    fd = open_config_file(self, filename, defaultUsenetFile);
    if (fd < 0) {
        usenet_parser_free(parser);
        return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    for (;;) {
        char buffer[BUFFER_SIZE];
        ssize_t length;

        /* Read from the fiel */
        length = read(fd, buffer, BUFFER_SIZE);
        if (length < 0) {
            close(fd);
            usenet_parser_free(parser);
            return -1;
        }

        /* Send it to the parser */
        if (usenet_parser_parse(parser, buffer, length) < 0) {
            close(fd);
            usenet_parser_free(parser);
            return -1;
        }

        /* Watch for end-of-file */
        if (length == 0) {
            close(fd);
            usenet_parser_free(parser);
            return 0;
        }
    }
}

/* The callback for the keys file parser */
static int
parse_keys_callback(void *rock,
                    const char *name,
                    const char *data,
                    int length,
                    int is_private)
{
    tickertape_t self = (tickertape_t)rock;

    /* Check whether a key with that name already exists */
    if (key_table_lookup(self->keys, name, NULL, NULL, NULL) == 0) {
        return -1;
    }

    /* Add the key to the table */
    if (key_table_add(self->keys, name, data, length, is_private) < 0) {
        return -1;
    }

    return 0;
}

/* Parse the keys file and update the keys table accordingly */
static int
parse_keys_file(tickertape_t self)
{
    const char *filename = tickertape_keys_filename(self);
    keys_parser_t parser;
    int fd;

    /* Allocate a new keys file parser */
    parser = keys_parser_alloc(tickertape_keys_directory(self),
                               parse_keys_callback, self,
                               filename);
    if (parser == NULL) {
        return -1;
    }

    /* Allocate a new key table */
    self->keys = key_table_alloc();
    if (self->keys == NULL) {
        keys_parser_free(parser);
        return -1;
    }

    /* Make sure we can read the keys file */
    fd = open_config_file(self, filename, default_keys_file);
    if (fd < 0) {
        keys_parser_free(parser);
        return -1;
    }

    /* Keep reading from the file until we've read it all or got an error */
    for (;;) {
        char buffer[BUFFER_SIZE];
        ssize_t length;

        /* Read a chunk of the file */
        length = read(fd, buffer, BUFFER_SIZE);
        if (length < 0) {
            close(fd);
            keys_parser_free(parser);
            return -1;
        }

        /* Send it to the parser */
        if (keys_parser_parse(parser, buffer, length) < 0) {
            close(fd);
            keys_parser_free(parser);
            return -1;
        }

        /* Watch for the end-of-file */
        if (length == 0) {
            close(fd);
            keys_parser_free(parser);
            return 0;
        }
    }
}

/* Returns the index of the group with the given expression (-1 if none) */
static int
find_group(group_sub_t *groups, int count, const char *expression)
{
    group_sub_t *pointer;

    for (pointer = groups; pointer < groups + count; pointer++) {
        if (*pointer != NULL &&
            strcmp(group_sub_expression(*pointer), expression) == 0) {
            return pointer - groups;
        }
    }

    return -1;
}

/* Reload the groups, possibly with a change of keys */
static void
reload_groups(tickertape_t self, key_table_t old_keys, key_table_t new_keys)
{
    group_sub_t *old_groups = self->groups;
    int old_count = self->groups_count;
    int index;
    int count;

    self->groups = NULL;
    self->groups_count = 0;

    /* Read the new-and-improved groups file */
    if (parse_groups_file(self) < 0) {
        /* Clean up the mess */
        for (index = 0; index < self->groups_count; index++) {
            group_sub_free(self->groups[index]);
        }

        /* Put the old groups back into place */
        self->groups = old_groups;
        self->groups_count = old_count;
        return;
    }

    /* Reuse elvin subscriptions whenever possible */
    for (index = 0; index < self->groups_count; index++) {
        group_sub_t group = self->groups[index];
        int old_index;

        /* Look for a match */
        old_index = find_group(old_groups, old_count,
                               group_sub_expression(group));
        if (old_index < 0) {
            /* None found.  Set the subscription's connection */
            group_sub_set_connection(group, self->handle, self->error);
        } else {
            group_sub_t old_group = old_groups[old_index];

            group_sub_update_from_sub(old_group, group, old_keys, new_keys);
            group_sub_free(group);
            self->groups[index] = old_group;
            old_groups[old_index] = NULL;
        }
    }

    /* Free the remaining old subscriptions */
    for (index = 0; index < old_count; index++) {
        group_sub_t old_group = old_groups[index];

        if (old_group != NULL) {
            group_sub_set_connection(old_group, NULL, self->error);
            group_sub_set_control_panel(old_group, NULL);
            group_sub_free(old_group);
        }
    }

    /* Release the old array */
    free(old_groups);

    /* Renumber the items in the control panel */
    count = 0;
    for (index = 0; index < self->groups_count; index++) {
        group_sub_set_control_panel_index(self->groups[index],
                                          self->control_panel,
                                          &count);
    }
}

/* Request from the control panel to reload groups file */
void
tickertape_reload_groups(tickertape_t self)
{
    reload_groups(self, self->keys, self->keys);
}

/* Request from the control panel to reload usenet file */
void
tickertape_reload_usenet(tickertape_t self)
{
    /* Release the old usenet subscription */
    usenet_sub_set_connection(self->usenet_sub, NULL, self->error);
    usenet_sub_free(self->usenet_sub);
    self->usenet_sub = NULL;

    /* Try to read in the new one */
    if (parse_usenet_file(self) < 0) {
        return;
    }

    /* Set its connection */
    usenet_sub_set_connection(self->usenet_sub, self->handle, self->error);
}

/* Request from the control panel to reload the keys file */
void
tickertape_reload_keys(tickertape_t self)
{
    key_table_t old_keys;
    int index;

    /* Hang on to the old keys table */
    old_keys = self->keys;
    self->keys = NULL;

    /* Try to read in the new one */
    if (parse_keys_file(self) < 0) {
        fprintf(stderr, PACKAGE ": errors in keys file; not reloading\n");
        key_table_free(self->keys);
        self->keys = old_keys;
        return;
    }

    /* Update all of the group subs */
    for (index = 0; index < self->groups_count; index++) {
        group_sub_update_from_sub(self->groups[index], self->groups[index],
                                  old_keys, self->keys);
    }

    /* Release the old keys table */
    if (old_keys != NULL) {
        key_table_free(old_keys);
    }
}

/* Reload all config files */
void
tickertape_reload_all(tickertape_t self)
{
    key_table_t old_keys;

    /* Hang onto the old keys */
    old_keys = self->keys;
    self->keys = NULL;

    /* Try to read in the new ones */
    if (parse_keys_file(self) < 0) {
        fprintf(stderr, PACKAGE ": errors in keys file; not reloading\n");
        if (self->keys != NULL) {
            key_table_free(self->keys);
        }

        self->keys = old_keys;
    }

    /* Reload the groups file */
    reload_groups(self, old_keys, self->keys);

    /* And the usenet file */
    tickertape_reload_usenet(self);

    /* Release the old keys table */
    if (old_keys != NULL) {
        key_table_free(old_keys);
    }
}

/* Initializes the User Interface */
static void
init_ui(tickertape_t self)
{
    int index;

    /* Allocate the control panel */
    self->control_panel =
        control_panel_alloc(self, self->top,
                            self->resources->send_history_count);
    if (self->control_panel == NULL) {
        return;
    }

    /* Tell our group subscriptions about the control panel */
    for (index = 0; index < self->groups_count; index++) {
        group_sub_set_control_panel(self->groups[index], self->control_panel);
    }

    /* Create the scroller widget */
    self->scroller = XtVaCreateManagedWidget(
        "scroller", scrollerWidgetClass, self->top,
        NULL);
    XtAddCallback(self->scroller, XtNcallback, menu_callback, self);
    XtAddCallback(self->scroller, XtNattachmentCallback, mime_callback, self);
    XtAddCallback(self->scroller, XtNkillCallback, kill_callback, self);
    XtRealizeWidget(self->top);
}

/* This is called when our connection request is handled */
static
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
int
#endif
connect_cb(elvin_handle_t handle, int result, void *rock, elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    int index;

    /* Record the elvin_handle */
    self->handle = handle;

    /* Check for a failure to connect */
    if (result == 0) {
        fprintf(stderr, "%s: unable to connect\n", PACKAGE);
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Tell the control panel that we're connected */
    control_panel_set_connected(self->control_panel, 1);

    /* Subscribe to the groups */
    for (index = 0; index < self->groups_count; index++) {
        group_sub_set_connection(self->groups[index], handle, error);
    }

    /* Subscribe to usenet */
    if (self->usenet_sub != NULL) {
        usenet_sub_set_connection(self->usenet_sub, handle, error);
    }

    /* Subscribe to the Mail subscription if we have one */
    if (self->mail_sub != NULL) {
        mail_sub_set_connection(self->mail_sub, handle, error);
    }

    return ELVIN_RETURN_SUCCESS;
}

#if !defined(ELVIN_VERSION_AT_LEAST)
/* Callback for elvin status changes */
static void
status_cb(elvin_handle_t handle,
          char *url,
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
    switch (event) {
    case ELVIN_STATUS_CONNECTION_FAILED:
        /* We were unable to (re)connect */
        fprintf(stderr, PACKAGE ": unable to connect\n");
        elvin_error_fprintf(stderr, error);
        exit(1);

    case ELVIN_STATUS_CONNECTION_FOUND:
        /* Tell the control panel that we're connected */
        control_panel_set_connected(self->control_panel, True);

        /* Make room for a combined string and URL */
        length = strlen(CONNECT_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, CONNECT_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_CONNECTION_LOST:
        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a combined string and URL */
        length = strlen(LOST_CONNECT_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, LOST_CONNECT_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_CONNECTION_CLOSED:
        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a message string */
        length = strlen(CONN_CLOSED_MSG) + 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, CONN_CLOSED_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_DROP_WARN:
        string = DROP_WARN_MSG;
        break;

    case ELVIN_STATUS_PROTOCOL_ERROR:
        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a message string */
        length = strlen(PROTOCOL_ERROR_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, PROTOCOL_ERROR_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_IGNORED_ERROR:
        fprintf(stderr, "%s: status ignored\n", PACKAGE);
        elvin_error_fprintf(stderr, error);
        exit(1);

    case ELVIN_STATUS_CLIENT_ERROR:
        fprintf(stderr, "%s: client error\n", PACKAGE);
        elvin_error_fprintf(stderr, error);
        exit(1);

    default:
        length = sizeof(UNKNOWN_STATUS_MSG) + 16;
        buffer = malloc(length);
        snprintf(buffer, length, UNKNOWN_STATUS_MSG, event);
        string = buffer;
        break;
    }

    /* Construct a message for the string and add it to the scroller */
    message = message_alloc(NULL, "internal", "tickertape", string, 30,
                            NULL, 0, NULL, NULL, NULL, NULL);
    if (message != NULL) {
        receive_callback(self, message, False);
        message_free(message);
    }

    /* Also display the message on the status line */
    control_panel_set_status(self->control_panel, buffer);

    /* Clean up */
    if (buffer != NULL) {
        free(buffer);
    }
}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* Callback for elvin status changes */
static int
status_cb(elvin_handle_t handle,
          elvin_status_event_t event,
          void *rock,
          elvin_error_t error)
{
    tickertape_t self = (tickertape_t)rock;
    message_t message;
    size_t length;
    char *buffer = NULL;
    const char *string;
    const char *url;

    /* Construct an appropriate message string */
    switch (event->type) {
    case ELVIN_STATUS_CONNECTION_FAILED:
        /* We were unable to (re)connect */
        fprintf(stderr, PACKAGE ": unable to connect\n");
        elvin_error_fprintf(stderr, event->details.connection_failed.error);
        exit(1);

    case ELVIN_STATUS_CONNECTION_FOUND:
        /* Stringify the URL */
        url = elvin_url_get_canonical(event->details.connection_found.url,
                                      error);
        if (url == NULL) {
            fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
            elvin_error_fprintf(stderr, error);
            exit(1);
        }

        /* Tell the control panel that we're connected */
        control_panel_set_connected(self->control_panel, True);

        /* Make room for a combined string and URL */
        length = strlen(CONNECT_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, CONNECT_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_CONNECTION_LOST:
        /* Stringify the URL */
        url = elvin_url_get_canonical(event->details.connection_lost.url,
                                      error);
        if (url == NULL) {
            fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
            elvin_error_fprintf(stderr, error);
            exit(1);
        }

        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a combined string and URL */
        length = strlen(LOST_CONNECT_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, LOST_CONNECT_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_CONNECTION_CLOSED:
        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a message string */
        length = strlen(CONN_CLOSED_MSG) + 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, CONN_CLOSED_MSG);
        string = buffer;
        break;

    /* Connection warnings go to the status line */
    case ELVIN_STATUS_CONNECTION_WARN:
        /* Get a big buffer */
        buffer = malloc(BUFFER_SIZE);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        /* Print the error message into it */
# if !defined(ELVIN_VERSION_AT_LEAST)
        elvin_error_snprintf(buffer, BUFFER_SIZE,
                             event->details.connection_warn.error);
# elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
        elvin_error_snprintf(buffer, BUFFER_SIZE, NULL,
                             event->details.connection_warn.error);
# endif
        string = buffer;

        /* Display it on the status line */
        control_panel_set_status(self->control_panel, buffer);

        /* Clean up */
        free(buffer);
        return 1;

    case ELVIN_STATUS_DROP_WARN:
        string = DROP_WARN_MSG;
        break;

    case ELVIN_STATUS_PROTOCOL_ERROR:
        /* Stringify the URL */
        url = elvin_url_get_canonical(event->details.protocol_error.url,
                                      error);
        if (url == NULL) {
            fprintf(stderr, PACKAGE ": elvin_url_get_canonical() failed\n");
            elvin_error_fprintf(stderr, error);
            exit(1);
        }

        /* Tell the control panel that we're no longer connected */
        control_panel_set_connected(self->control_panel, False);

        /* Make room for a message string */
        length = strlen(PROTOCOL_ERROR_MSG) + strlen(url) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            perror(PACKAGE ": malloc() failed");
            exit(1);
        }

        snprintf(buffer, length, PROTOCOL_ERROR_MSG, url);
        string = buffer;
        break;

    case ELVIN_STATUS_NO_DISCOVERY:
        string = NO_DISCOVERY_MSG;
        break;

    case ELVIN_STATUS_IGNORED_ERROR:
        fprintf(stderr, "%s: status ignored\n", PACKAGE);
        elvin_error_fprintf(stderr, event->details.ignored_error.error);
        exit(1);

    case ELVIN_STATUS_CLIENT_ERROR:
        fprintf(stderr, "%s: client error\n", PACKAGE);
        elvin_error_fprintf(stderr, event->details.client_error.error);
        exit(1);

    default:
        length = sizeof(UNKNOWN_STATUS_MSG) + 16;
        buffer = malloc(length);
        snprintf(buffer, length, UNKNOWN_STATUS_MSG, event->type);
        string = buffer;
        break;
    }

    /* Construct a message for the string and add it to the scroller */
    message = message_alloc(NULL, "internal", "tickertape", string, 30,
                            NULL, 0, NULL, NULL, NULL, NULL);
    if (message != NULL) {
        receive_callback(self, message, False);
        message_free(message);
    }

    /* Also display the message on the status line */
    control_panel_set_status(self->control_panel, buffer);

    /* Clean up */
    if (buffer != NULL) {
        free(buffer);
    }

    return 1;
}
#else
# error "Unsupported Elvin library version"
#endif

/*
 *
 * Exported function definitions
 *
 */

/* Answers a new tickertape_t for the given user using the given file as
 * her groups file and connecting to the notification service */
tickertape_t
tickertape_alloc(XTickertapeRec *resources,
                 elvin_handle_t handle,
                 const char *user,
                 const char *domain,
                 const char *ticker_dir,
                 const char *config_file,
                 const char *groups_file,
                 const char *usenet_file,
                 const char *keys_file,
                 const char *keys_dir,
                 Widget top,
                 elvin_error_t error)
{
    tickertape_t self;

    /* Allocate some space for the new tickertape */
    self = malloc(sizeof(struct tickertape));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize its contents to sane values */
    self->resources = resources;
    self->error = error;
    self->handle = NULL;
    self->user = strdup(user);
    self->domain = strdup(domain);
    self->ticker_dir = (ticker_dir == NULL) ? NULL : strdup(ticker_dir);
    self->config_file = (config_file == NULL) ? NULL : strdup(config_file);
    self->groups_file = (groups_file == NULL) ? NULL : strdup(groups_file);
    self->usenet_file = (usenet_file == NULL) ? NULL : strdup(usenet_file);
    self->keys_file = (keys_file == NULL) ? NULL : strdup(keys_file);
    self->keys_dir = (keys_dir == NULL) ? NULL : strdup(keys_dir);
    self->top = top;
    self->groups = NULL;
    self->groups_count = 0;
    self->usenet_sub = NULL;
    self->mail_sub = NULL;
    self->control_panel = NULL;
    self->scroller = NULL;

    /* Read the keys from the keys file */
    if (parse_keys_file(self) < 0) {
        exit(1);
    }

    /* Read the subscriptions from the groups file */
    if (parse_groups_file(self) < 0) {
        exit(1);
    }

    /* Read the subscriptions from the usenet file */
    if (parse_usenet_file(self) < 0) {
        exit(1);
    }

    /* Initialize the e-mail subscription */
    self->mail_sub = mail_sub_alloc(user, receive_callback, self);

    /* Draw the user interface */
    init_ui(self);

    /* Set the handle's status callback */
    if (!elvin_handle_set_status_cb(handle, status_cb, self, self->error)) {
        fprintf(stderr, PACKAGE ": elvin_handle_set_status_cb(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    /* Connect to the elvin server */
    if (elvin_async_connect(handle, connect_cb, self, self->error) == 0) {
        fprintf(stderr, PACKAGE ": elvin_async_connect(): failed\n");
        elvin_error_fprintf(stderr, error);
        exit(1);
    }

    return self;
}

/* Free the resources consumed by a tickertape */
void
tickertape_free(tickertape_t self)
{
    int index;

    /* How do we free a Widget? */

    if (self->user != NULL) {
        free(self->user);
    }

    if (self->domain != NULL) {
        free(self->domain);
    }

    if (self->ticker_dir != NULL) {
        free(self->ticker_dir);
    }

    if (self->config_file != NULL) {
        free(self->config_file);
    }

    if (self->groups_file != NULL) {
        free(self->groups_file);
    }

    if (self->keys_file != NULL) {
        free(self->keys_file);
    }

    for (index = 0; index < self->groups_count; index++) {
        group_sub_set_connection(self->groups[index], NULL, self->error);
        group_sub_free(self->groups[index]);
    }

    if (self->groups != NULL) {
        free(self->groups);
    }

    if (self->usenet_sub != NULL) {
        usenet_sub_set_connection(self->usenet_sub, NULL, self->error);
        usenet_sub_free(self->usenet_sub);
    }

    if (self->keys != NULL) {
        key_table_free(self->keys);
    }

    if (self->control_panel) {
        control_panel_free(self->control_panel);
    }

    free(self);
}

/* Answers the tickertape's user name */
const const char *
tickertape_user_name(tickertape_t self)
{
    return self->user;
}

/* Answers the tickertape's domain name */
const char *
tickertape_domain_name(tickertape_t self)
{
    return self->domain;
}

/* Show the previous item in the history */
void
tickertape_history_prev(tickertape_t self)
{
    control_panel_history_prev(self->control_panel);
}

/* Show the next item in the history */
void
tickertape_history_next(tickertape_t self)
{
    control_panel_history_next(self->control_panel);
}

/* Quit the application */
void
tickertape_quit(tickertape_t self)
{
    XtDestroyApplicationContext(XtWidgetToApplicationContext(self->top));
    exit(0);
}

/* Answers the receiver's ticker_dir filename */
static const char *
tickertape_ticker_dir(tickertape_t self)
{
    const char *dir;
    size_t length;

    /* See if we've already looked it up */
    if (self->ticker_dir != NULL) {
        return self->ticker_dir;
    }

    /* Use the TICKERDIR environment variable if it is set */
    dir = getenv("TICKERDIR");
    if (dir != NULL) {
        self->ticker_dir = strdup(dir);
    } else {
        /* Otherwise grab the user's home directory */
        dir = getenv("HOME");
        if (dir == NULL) {
            dir = "/";
        }

        /* And append /.ticker to the end of it */
        length = strlen(dir) + strlen(DEFAULT_TICKERDIR) + 2;
        self->ticker_dir = malloc(length);
        snprintf(self->ticker_dir, length, "%s/%s", dir, DEFAULT_TICKERDIR);
    }

    /* Make sure the TICKERDIR exists
     * Note: we're being clever here and assuming that nothing will
     * call tickertape_ticker_dir() if both groups_file and usenet_file
     * are set */
    if (mkdirhier(self->ticker_dir) < 0) {
        perror("unable to create tickertape directory");
    }

    return self->ticker_dir;
}

/* Answers the receiver's groups file filename */
static const char *
tickertape_groups_filename(tickertape_t self)
{
    if (self->groups_file == NULL) {
        const char *dir = tickertape_ticker_dir(self);
        size_t length = strlen(dir) + strlen(DEFAULT_GROUPS_FILE) + 2;

        self->groups_file = malloc(length);
        if (self->groups_file == NULL) {
            perror("unable to allocate memory");
            exit(1);
        }

        snprintf(self->groups_file, length, "%s/%s", dir, DEFAULT_GROUPS_FILE);
    }

    return self->groups_file;
}

/* Answers the receiver's usenet filename */
static const char *
tickertape_usenet_filename(tickertape_t self)
{
    if (self->usenet_file == NULL) {
        const char *dir = tickertape_ticker_dir(self);
        size_t length = strlen(dir) + strlen(DEFAULT_USENET_FILE) + 2;

        self->usenet_file = malloc(length);
        if (self->usenet_file == NULL) {
            perror("unable to allocate memory");
            exit(1);
        }

        snprintf(self->usenet_file, length, "%s/%s", dir, DEFAULT_USENET_FILE);
    }

    return self->usenet_file;
}

/* Answers the receiver's keys filename */
static const char *
tickertape_keys_filename(tickertape_t self)
{
    if (self->keys_file == NULL) {
        const char *dir = tickertape_ticker_dir(self);
        size_t length;

        length = strlen(dir) + sizeof(DEFAULT_KEYS_FILE) + 1;
        self->keys_file = malloc(length);
        if (self->keys_file == NULL) {
            perror("unable to allocate memory");
            exit(1);
        }

        snprintf(self->keys_file, length, "%s/%s", dir, DEFAULT_KEYS_FILE);
    }

    return self->keys_file;
}

/* Answers the receiver's keys directory */
static const char *
tickertape_keys_directory(tickertape_t self)
{
    if (self->keys_dir == NULL) {
        const char *file = tickertape_keys_filename(self);
        const char *point;
        size_t length;

        /* Default to the directory name from the keys filename */
        point = strrchr(file, '/');
        if (point == NULL) {
            self->keys_dir = strdup("");
        } else {
            length = point - file;
            self->keys_dir = malloc(length + 1);
            if (self->keys_dir == NULL) {
                perror("Unable to allocate memory");
                exit(1);
            }

            memcpy(self->keys_dir, file, length);
            self->keys_dir[length] = 0;
        }
    }

    return self->keys_dir;
}

/* Displays a message's MIME attachment */
int
tickertape_show_attachment(tickertape_t self, message_t message)
{
#if defined(HAVE_DUP2) && defined(HAVE_FORK)
    const char *attachment;
    size_t count;
    const char *pointer;
    const char *end;
    pid_t pid;
    int fds_stdin[2];
    int status;

    /* If metamail is not defined then we're done */
    if (self->resources->metamail == NULL ||
        *self->resources->metamail == '\0') {
# if defined(DEBUG)
        printf("metamail not defined\n");
# endif /* DEBUG */
        return -1;
    }

    /* If the message has no attachment then we're done */
    count = message_get_attachment(message, &attachment);
    if (count == 0) {
# if defined(DEBUG)
        printf("no attachment\n");
# endif /* DEBUG */
        return -1;
    }

    /* Create a pipe to send the file to metamail */
    if (pipe(fds_stdin) < 0) {
        perror("pipe(): failed");
        return -1;
    }

    /* Fork a child process to invoke metamail */
    pid = fork();
    if (pid == (pid_t)-1) {
        perror("fork(): failed");
        close(fds_stdin[0]);
        close(fds_stdin[1]);
        return -1;
    }

    /* See if we're the child process */
    if (pid == 0) {
        /* Use the pipe as stdin */
        dup2(fds_stdin[0], STDIN_FILENO);

        /* Close off the ends of the pipe */
        close(fds_stdin[0]);
        close(fds_stdin[1]);

        /* Invoke metamail */
        execlp(self->resources->metamail,
               self->resources->metamail,
               METAMAIL_OPTIONS,
               NULL);

        /* We'll only get here if exec fails */
        perror("execlp(): failed");
        exit(1);
    }

    /* We're the parent process. */
    if (close(fds_stdin[0]) < 0) {
        perror("close(): failed");
        return -1;
    }

    /* Write the mime args into the pipe */
    end = attachment + count;
    pointer = attachment;
    while (pointer < end) {
        ssize_t length;

        /* Write as much as we can */
        length = write(fds_stdin[1], pointer, end - pointer);
        if (length < 0) {
            perror("unable to write to metamail pipe");
            return -1;
        }

        pointer += length;
    }

    /* Make sure it gets written */
    if (close(fds_stdin[1]) < 0) {
        perror("close(): failed");
        return -1;
    }

    /* Wait for the child process to exit */
    if (waitpid(pid, &status, 0) == (pid_t)-1) {
        perror("waitpid(): failed");
        return -1;
    }

    /* Did the child process exit nicely? */
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        return 0;
    }
# if defined(DEBUG)
    /* Did it exit badly? */
    if (WIFEXITED(status)) {
        fprintf(stderr, "%s exit status: %d\n",
                self->resources->metamail,
                WEXITSTATUS(status));
        return -1;
    }

    fprintf(stderr, "%s died badly\n", self->resources->metamail);
# endif
#endif /* METAMAIL & DUP2 & FORK */

    return -1;
}

/**********************************************************************/
