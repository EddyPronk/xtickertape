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
#include <stdio.h> /* exit, fprintf, snprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* exit, free, malloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* strdup, strlen */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "replace.h"
#include "message.h"
#include "mbox_parser.h"
#include "mail_sub.h"
#include "utils.h"

#define MAIL_SUB "require(elvinmail) && user == \"%s\""
#define FOLDER_FMT "+%s"

/* The fields in the notification which we access */
#define F_FROM "From"
#define F_FOLDER "folder"
#define F_SUBJECT "Subject"

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
# define ELVIN_RETURN_FAILURE
# define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
# define ELVIN_RETURN_FAILURE 0
# define ELVIN_RETURN_SUCCESS 1
#endif

/* The MailSubscription data type */
struct mail_sub {
    /* The receiver's user name */
    char *user;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* The receiver's subscription id */
    elvin_subscription_t subscription;

    /* The receiver's rfc822 mailbox parser */
    mbox_parser_t parser;

    /* The receiver's callback function */
    mail_sub_callback_t callback;

    /* The receiver's callback user data */
    void *rock;
};


#if !defined(ELVIN_VERSION_AT_LEAST)
/* Delivers a notification which matches the receiver's e-mail subscription */
static void
notify_cb(elvin_handle_t handle,
          elvin_subscription_t subscription,
          elvin_notification_t notification,
          int is_secure,
          void *rock,
          elvin_error_t error)
{
    mail_sub_t self = (mail_sub_t)rock;
    message_t message;
    elvin_basetypes_t type;
    elvin_value_t value;
    char *from;
    char *folder;
    char *subject;
    char *buffer = NULL;
    size_t length;

    /* Get the name from the `From' field */
    if (elvin_notification_get(notification, F_FROM, &type, &value, error) &&
        type == ELVIN_STRING) {
        from = value.s;

        /* Split the user name from the address. */
        if (mbox_parser_parse(self->parser, from) == 0) {
            from = mbox_parser_get_name(self->parser);
            if (*from == '\0') {
                /* Otherwise resort to the e-mail address */
                from = mbox_parser_get_email(self->parser);
            }
        }
    } else {
        from = "anonymous";
    }

    /* Get the folder field */
    if (elvin_notification_get(notification, F_FOLDER, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        folder = value.s;

        /* Format the folder name to use as the group */
        length = strlen(FOLDER_FMT) + strlen(folder) - 1;
        buffer = malloc(length);
        if (buffer != NULL) {
            snprintf(buffer, length, FOLDER_FMT, folder);
            folder = buffer;
        }
    } else {
        folder = "mail";
    }

    /* Get the subject field */
    if (elvin_notification_get(notification, F_SUBJECT, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        subject = value.s;
    } else {
        subject = "No subject";
    }

    /* Construct a message_t out of all of that */
    message = message_alloc(
        NULL, folder, from, subject, 600,
        NULL, 0,
        NULL, NULL, NULL, NULL);

    self->callback(self->rock, message, False);

    /* Clean up */
    message_free(message);

    /* Free the folder name */
    if (buffer != NULL) {
        free(buffer);
    }
}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* Delivers a notification which matches the receiver's e-mail subscription */
static int
notify_cb(elvin_handle_t handle,
          elvin_subscription_t subscription,
          elvin_notification_t notification,
          int is_secure,
          void *rock,
          elvin_error_t error)
{
    mail_sub_t self = (mail_sub_t)rock;
    message_t message;
    char *value;
    const char *from;
    const char *folder;
    const char *subject;
    char *buffer = NULL;
    size_t length;
    int found;

    /* Get the name from the `From' field */
    if (!elvin_notification_get_string(notification, F_FROM, &found, &value,
                                       error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    from = "anonymous";
    if (found) {
        /* Split the user name from the address */
        if (mbox_parser_parse(self->parser, value) == 0) {
            from = mbox_parser_get_name(self->parser);
            if (*from == 0) {
                /* Otherwise resort to the e-mail address */
                from = mbox_parser_get_email(self->parser);
            }
        }
    }

    /* Get the folder field */
    if (!elvin_notification_get_string(notification, F_FOLDER, NULL, &value,
                                       error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    folder = "mail";
    if (found) {
        /* Format the folder name to use as the group */
        length = strlen(FOLDER_FMT) + strlen(value) - 1;
        buffer = malloc(length);
        if (buffer != NULL) {
            snprintf(buffer, length, FOLDER_FMT, value);
            folder = buffer;
        }
    }

    /* Get the subject field */
    if (!elvin_notification_get_string(notification, F_SUBJECT, &found,
                                       &value, error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    /* Have a default subject */
    subject = "[No subject]";
    if (found) {
        subject = value;
    }

    /* Construct a message_t out of all of that */
    message = message_alloc(
        NULL, folder, from, subject, 60,
        NULL, 0, NULL, NULL, NULL, NULL);

    self->callback(self->rock, message, False);

    /* Clean up */
    message_free(message);

    /* Free the folder name */
    if (buffer != NULL) {
        free(buffer);
    }

    return 1;
}
#else
# error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */

/*
 *
 * Exported functions
 *
 */

/* Answers a new mail_sub_t */
mail_sub_t
mail_sub_alloc(const char *user, mail_sub_callback_t callback, void *rock)
{
    mail_sub_t self;

    /* Allocate memory for the receiver */
    self = malloc(sizeof(struct mail_sub));
    if (self == NULL) {
        fprintf(stderr, PACKAGE ": out of memory\n");
        exit(1);
    }

    /* Allocate memory for the rfc822 mailbox parser */
    self->parser = mbox_parser_alloc();
    if (self->parser == NULL) {
        free(self);
        return NULL;
    }

    self->user = strdup(user);
    self->handle = NULL;
    self->subscription = 0;
    self->callback = callback;
    self->rock = rock;
    return self;
}

/* Releases the resources consumed by the receiver */
void
mail_sub_free(mail_sub_t self)
{
    if (self->user != NULL) {
        free(self->user);
    }

    if (self->parser != NULL) {
        mbox_parser_free(self->parser);
    }

    free(self);
}

/* Callback for a subscribe request */
static
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
int
#endif
subscribe_cb(elvin_handle_t handle,
             int result,
             elvin_subscription_t subscription,
             void *rock,
             elvin_error_t error)
{
    mail_sub_t self = (mail_sub_t)rock;

    self->subscription = subscription;

    return ELVIN_RETURN_SUCCESS;
}

/* Sets the receiver's connection */
void
mail_sub_set_connection(mail_sub_t self,
                        elvin_handle_t handle,
                        elvin_error_t error)
{
    /* Unsubscribe from the old connection */
    if (self->handle != NULL) {
        if (elvin_async_delete_subscription(
                self->handle, self->subscription,
                NULL, NULL,
                error) == 0) {
            eeprintf(error, "elvin_async_delete_subscription failed\n");
            exit(1);
        }
    }

    self->handle = handle;

    /* Subscribe using the new handle */
    if (self->handle != NULL) {
        size_t length;
        char *buffer;

        length = strlen(MAIL_SUB) + strlen(self->user) - 1;
        buffer = malloc(length);
        if (buffer == NULL) {
            return;
        }

        snprintf(buffer, length, MAIL_SUB, self->user);

        /* Subscribe to elvinmail notifications */
        if (elvin_async_add_subscription(self->handle, buffer, NULL, 1,
                                         notify_cb, self,
                                         subscribe_cb, self,
                                         error) == 0) {
            eeprintf(error, "elvin_xt_add_subscription failed\n");
            exit(1);
        }

        /* Clean up */
        free(buffer);
    }
}

/**********************************************************************/
