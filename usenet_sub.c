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
#include <stdio.h> /* fprintf, snprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* exit, free, malloc, realloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* strlen */
#endif
#include <X11/Intrinsic.h>
#include <elvin/elvin.h>
#include <elvin/xt_mainloop.h>
#include "replace.h"
#include "usenet_sub.h"

/* Some notification field names */
#define BODY "BODY"
#define FROM_NAME "FROM_NAME"
#define FROM_EMAIL "FROM_EMAIL"
#define FROM "From"
#define KEYWORDS "KEYWORDS"
#define MESSAGE_ID "Message-ID"
#define MIME_ARGS "MIME_ARGS"
#define MIME_TYPE "MIME_TYPE"
#define NEWSGROUPS "NEWSGROUPS"
#define SUBJECT "SUBJECT"
#define URL_MIME_TYPE "x-elvin/url"
#define NEWS_MIME_TYPE "x-elvin/news"
#define XPOSTS "CROSS_POSTS"
#define X_NNTP_HOST "X-NNTP-Host"

#define F_MATCHES "regex(%s, \"%s\")"
#define F_NOT_MATCHES "!regex(%s, \"%s\")"
#define F_EQ "%s == %s"
#define F_STRING_EQ "%s == \"%s\""
#define F_NEQ "%s != %s"
#define F_STRING_NEQ "%s != \"%s\""
#define F_LT "%s < %s"
#define F_GT "%s > %s"
#define F_LE "%s <= %s"
#define F_GE "%s >= %s"

#define ONE_SUB "ELVIN_CLASS == \"NEWSWATCHER\" && (%s)"


#define PATTERN_ONLY "%sregex(NEWSGROUPS, \"%s\")"
#define PATTERN_PLUS "(%sregex(NEWSGROUPS, \"%s\"))"

#define USENET_PREFIX "usenet: %s"
#define NEWS_URL "news://%s/%s"

#define ATTACHMENT_FMT                     \
    "MIME-Version: 1.0\n"                  \
    "Content-Type: %s; charset=us-ascii\n" \
    "\n"                                   \
    "%s\n"

/* compatability code for return code of callbacks */
#if !defined(ELVIN_VERSION_AT_LEAST)
# define ELVIN_RETURN_FAILURE
# define ELVIN_RETURN_SUCCESS
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
# define ELVIN_RETURN_FAILURE 0
# define ELVIN_RETURN_SUCCESS 1
#endif

/* The structure of a usenet subscription */
struct usenet_sub {
    /* The receiver's subscription expression */
    char *expression;

    /* The length of the subscription expression */
    size_t expression_size;

    /* The receiver's elvin connection handle */
    elvin_handle_t handle;

    /* The reciever's subscription */
    elvin_subscription_t subscription;

    /* The receiver's callback */
    usenet_sub_callback_t callback;

    /* The argument for the receiver's callback */
    void *rock;

    /* Non-zero if the receiver is waiting on a change to the subscription */
    int is_pending;
};

#if !defined(ELVIN_VERSION_AT_LEAST)
/* Delivers a notification which matches the receiver's subscription
 * expression */
static void
notify_cb(elvin_handle_t handle,
          elvin_subscription_t subscription,
          elvin_notification_t notification,
          int is_secure,
          void *rock,
          elvin_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;
    message_t message;
    elvin_basetypes_t type;
    elvin_value_t value;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *attachment = NULL;
    size_t length;
    char *buffer = NULL;

    /* If we don't have a callback than bail out now */
    if (self->callback == NULL) {
        return;
    }

    /* Get the newsgroups to which the message was posted */
    if (elvin_notification_get(notification, NEWSGROUPS, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        string = value.s;
    } else {
        string = "news";
    }

    /* Prepend `usenet:' to the beginning of the group field */
    length = strlen(USENET_PREFIX) + strlen(string) - 1;
    newsgroups = malloc(length);
    if (newsgroups == NULL) {
        return;
    }

    snprintf(newsgroups, length, USENET_PREFIX, string);

    /* Get the name from the FROM_NAME field (if provided) */
    if (elvin_notification_get(notification, FROM_NAME, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        name = value.s;
    } else {
        /* If no FROM_NAME field then try FROM_EMAIL */
        if (elvin_notification_get(notification, FROM_EMAIL, &type, &value,
                                   error) &&
            type == ELVIN_STRING) {
            name = value.s;
        } else {
            /* If no FROM_EMAIL then try FROM */
            if (elvin_notification_get(notification, FROM, &type, &value,
                                       error) &&
                type == ELVIN_STRING) {
                name = value.s;
            } else {
                name = "anonymous";
            }
        }
    }

    /* Get the SUBJECT field (if provided) */
    if (elvin_notification_get(notification, SUBJECT, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        subject = value.s;
    } else {
        subject = "[no subject]";
    }

    /* Get the MIME_ARGS field (if provided) */
    if (elvin_notification_get(notification, MIME_ARGS, &type, &value,
                               error) &&
        type == ELVIN_STRING) {
        mime_args = value.s;

        /* Get the MIME_TYPE field (if provided) */
        if (elvin_notification_get(notification, MIME_TYPE, &type, &value,
                                   error) &&
            type == ELVIN_STRING) {
            mime_type = value.s;
        } else {
            mime_type = URL_MIME_TYPE;
        }
    } else {
        /* No MIME_ARGS provided.  Construct one using the Message-ID
         * field */
        if (elvin_notification_get(notification, MESSAGE_ID, &type, &value,
                                   error) &&
            type == ELVIN_STRING) {
            char *message_id = value.s;
            char *news_host;

            if (elvin_notification_get(notification, X_NNTP_HOST, &type,
                                       &value, error) &&
                type == ELVIN_STRING) {
                news_host = value.s;
            } else {
                news_host = "news";
            }

            length = strlen(NEWS_URL) + strlen(news_host) +
                     strlen(message_id) - 3;
            buffer = malloc(length);
            if (buffer == NULL) {
                mime_type = NULL;
                mime_args = NULL;
            } else {
                snprintf(buffer, length, NEWS_URL, news_host, message_id);
                mime_args = buffer;
                mime_type = NEWS_MIME_TYPE;
            }
        } else {
            mime_type = NULL;
            mime_args = NULL;
        }
    }

    /* Convert the mime type and args into an attachment */
    if (mime_type == NULL || mime_args == NULL) {
        attachment = NULL;
        length = 0;
    } else {
        length = strlen(ATTACHMENT_FMT) + strlen(mime_type) + strlen(
            mime_args) - 4;
        attachment = malloc(length + 1);
        if (attachment == NULL) {
            length = 0;
        } else {
            snprintf(attachment, length + 1, ATTACHMENT_FMT,
                     mime_type, mime_args);
        }
    }

    /* Construct a message out of all of that */
    message = message_alloc(NULL, newsgroups, name, subject, 300,
                            attachment, length - 1,
                            NULL, NULL, NULL, NULL);
    if (message != NULL) {
        /* Deliver the message */
        self->callback(self->rock, message, False);
        message_free(message);
    }

    /* Clean up */
    free(newsgroups);
    if (buffer != NULL) {
        free(buffer);
    }

    if (attachment != NULL) {
        free(attachment);
    }
}
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
/* Delivers a notification which matches the receiver's subscription
 * expression */
static int
notify_cb(elvin_handle_t handle,
          elvin_subscription_t subscription,
          elvin_notification_t notification,
          int is_secure,
          void *rock,
          elvin_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;
    message_t message;
    char *string;
    char *newsgroups;
    char *name;
    char *subject;
    char *mime_type;
    char *mime_args;
    char *buffer = NULL;
    char *attachment = NULL;
    size_t length = 0;
    int found;

    /* If we don't have a callback than bail out now */
    if (self->callback == NULL) {
        return 1;
    }

    /* Get the newsgroups to which the message was posted */
    if (!elvin_notification_get_string(notification, NEWSGROUPS, &found,
                                       &string, error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    /* Use a reasonable default */
    string = found ? string : "news";

    /* Prepend `usenet:' to the beginning of the group field */
    length = strlen(USENET_PREFIX) + strlen(string) - 1;
    newsgroups = malloc(length);
    if (newsgroups == NULL) {
        return 0;
    }

    snprintf(newsgroups, length, USENET_PREFIX, string);

    /* Get the name from the FROM_NAME field (if provided) */
    if (!elvin_notification_get_string(notification, FROM_NAME, &found, &name,
                                       error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    if (!found) {
        /* No FROM_NAME field, so try FROM_EMAIL */
        if (!elvin_notification_get_string(notification, FROM_EMAIL, &found,
                                           &name, error)) {
            eeprintf(error, "elvin_notification_get_string failed\n");
            exit(1);
        }

        if (!found) {
            /* No FROM_EMAIL, so try FROM */
            if (!elvin_notification_get_string(notification, FROM, &found,
                                               &name, error)) {
                eeprintf(error, "elvin_notification_get_string failed\n");
                exit(1);
            }

            if (!found) {
                /* Give up */
                name = "anonymous";
            }
        }
    }

    /* Get the SUBJECT field (if provided) */
    if (!elvin_notification_get_string(notification, SUBJECT, &found,
                                       &subject, error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    /* Use a default if none found */
    subject = found ? subject : "[no subject]";

    /* Get the MIME_ARGS field (if provided) */
    if (!elvin_notification_get_string(notification, MIME_ARGS, &found,
                                       &mime_args, error)) {
        eeprintf(error, "elvin_notification_get_string failed\n");
        exit(1);
    }

    /* Was the MIME_ARGS field provided? */
    if (found) {
        /* Get the MIME_TYPE field (if provided) */
        if (!elvin_notification_get_string(notification, MIME_TYPE, &found,
                                           &mime_type, error)) {
            eeprintf(error, "elvin_notification_get_string failed\n");
            exit(1);
        }

        /* Use a default if none found */
        mime_type = found ? mime_type : URL_MIME_TYPE;
    } else {
        char *message_id;

        /* No MIME_ARGS.  Look for a message-id */
        if (!elvin_notification_get_string(notification, MESSAGE_ID, &found,
                                           &message_id, error)) {
            eeprintf(error, "elvin_notification_get_string failed\n");
            exit(1);
        }

        if (found) {
            char *news_host;

            /* Look up the news host field */
            if (!elvin_notification_get_string(
                    notification,
                    X_NNTP_HOST,
                    &found, &news_host,
                    error)) {
                eeprintf(error, "elvin_notification_get_string failed\n");
                exit(1);
            }

            news_host = found ? news_host : "news";

            length = strlen(NEWS_URL) + strlen(news_host) +
                strlen(message_id) - 3;
            buffer = malloc(length);
            if (buffer == NULL) {
                mime_type = NULL;
                mime_args = NULL;
            } else {
                snprintf(buffer, length, NEWS_URL, news_host, message_id);
                mime_args = buffer;
                mime_type = NEWS_MIME_TYPE;
            }
        } else {
            mime_type = NULL;
            mime_args = NULL;
        }
    }

    /* Convert the mime type and args into an attachment */
    if (mime_type == NULL || mime_args == NULL) {
        attachment = NULL;
        length = 0;
    } else {
        length = strlen(ATTACHMENT_FMT) - 4 + strlen(mime_type) + strlen(
            mime_args);
        attachment = malloc(length + 1);
        if (attachment == NULL) {
            length = 0;
        } else {
            snprintf(attachment, length + 1, ATTACHMENT_FMT,
                     mime_type, mime_args);
        }
    }

    /* Construct a message out of all of that */
    message = message_alloc(NULL, newsgroups, name, subject, 60,
                            attachment, length,
                            NULL, NULL, NULL, NULL);
    if (message != NULL) {
        /* Deliver the message */
        self->callback(self->rock, message, False);
        message_free(message);
    }

    /* Clean up */
    free(newsgroups);
    if (buffer != NULL) {
        free(buffer);
    }

    return 1;
}
#else
# error "Unsupported Elvin library version"
#endif /* ELVIN_VERSION_AT_LEAST */

/* Construct a portion of the subscription expression */
static char *
alloc_expr(usenet_sub_t self, struct usenet_expr *expression)
{
    const char *field_name;
    const char *format;
    size_t format_length;
    size_t length;
    char *result;

    /* Get the string representation for the field */
    switch (expression->field) {
    case F_BODY:
        /* body -> BODY */
        field_name = BODY;
        break;

    case F_FROM:
        /* from -> FROM_NAME */
        field_name = FROM_NAME;
        break;

    case F_EMAIL:
        /* email -> FROM_EMAIL */
        field_name = FROM_EMAIL;
        break;

    case F_SUBJECT:
        /* subject -> SUBJECT */
        field_name = SUBJECT;
        break;

    case F_KEYWORDS:
        /* keywords -> KEYWORDS */
        field_name = KEYWORDS;
        break;

    case F_XPOSTS:
        /* xposts -> CROSS_POSTS */
        field_name = XPOSTS;
        break;

    default:
        /* Should never get here */
        fprintf(stderr, PACKAGE ": internal error\n");
        return NULL;
    }

    /* Look up the string representation for the operator */
    switch (expression->operator) {
    case O_MATCHES:
        /* matches */
        format = F_MATCHES;
        format_length = sizeof(F_MATCHES);
        break;

    case O_NOT:
        /* not [matches] */
        format = F_NOT_MATCHES;
        format_length = sizeof(F_NOT_MATCHES);
        break;

    case O_EQ:
        /* = */
        if (expression->field == F_XPOSTS) {
            format = F_EQ;
            format_length = sizeof(F_EQ);
            break;
        }

        format = F_STRING_EQ;
        format_length = sizeof(F_STRING_EQ);
        break;

    case O_NEQ:
        /* != */
        if (expression->field == F_XPOSTS) {
            format = F_NEQ;
            format_length = sizeof(F_NEQ);
            break;
        }

        format = F_STRING_NEQ;
        format_length = sizeof(F_STRING_NEQ);
        break;

    case O_LT:
        /* < */
        format = F_LT;
        format_length = sizeof(F_LT);
        break;

    case O_GT:
        /* > */
        format = F_GT;
        format_length = sizeof(F_GT);
        break;

    case O_LE:
        /* <= */
        format = F_LE;
        format_length = sizeof(F_LE);
        break;

    case O_GE:
        /* >= */
        format = F_GE;
        format_length = sizeof(F_GE);
        break;

    default:
        fprintf(stderr, PACKAGE ": internal error\n");
        return NULL;
    }

    /* Allocate space for the result */
    length = format_length + strlen(field_name) +
        strlen(expression->pattern) - 3;
    result = malloc(length);
    if (result == NULL) {
        return NULL;
    }

    /* Construct the string */
    snprintf(result, length, format, field_name, expression->pattern);
    return result;
}

/* Construct a subscription expression for the given group entry */
static char *
alloc_sub(usenet_sub_t self,
          int has_not,
          const char *pattern,
          struct usenet_expr *expressions,
          size_t count)
{
    struct usenet_expr *pointer;
    const char *not_string = has_not ? "!" : "";
    char *result;
    size_t length;

    /* Shortcut -- if there are no expressions then we don't need parens */
    if (count == 0) {
        length = strlen(PATTERN_ONLY) + strlen(not_string) +
                 strlen(pattern) - 3;
        result = malloc(length);
        if (result == NULL) {
            return NULL;
        }

        snprintf(result, length, PATTERN_ONLY, not_string, pattern);
        return result;
    }

    /* Otherwise we have to do this the hard way.  Allocate space for
     * the initial pattern and then extend it for each of the expressions */
    length = strlen(PATTERN_PLUS) + strlen(not_string) + strlen(pattern) - 3;
    result = malloc(length);
    if (result == NULL) {
        return NULL;
    }

    snprintf(result, length, PATTERN_PLUS, not_string, pattern);

    /* Insert the expressions */
    for (pointer = expressions; pointer < expressions + count; pointer++) {
        /* Get the string for the expression */
        char *expr = alloc_expr(self, pointer);
        size_t new_length = length + strlen(expr) + 4;

        /* Expand the buffer */
        result = realloc(result, new_length);
        if (result == NULL) {
            free(expr);
            return NULL;
        }

        /* Overwrite the trailing right paren with the new stuff */
        snprintf(result + length - 2, new_length - length + 2,
                 " && %s)", expr);
        length = new_length;
        free(expr);
    }

    return result;
}

/* Allocates and initializes a new usenet_sub_t */
usenet_sub_t
usenet_sub_alloc(usenet_sub_callback_t callback, void *rock)
{
    usenet_sub_t self;

    /* Allocate memory for the new subscription */
    self = malloc(sizeof(struct usenet_sub));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize its contents */
    self->expression = NULL;
    self->expression_size = 0;
    self->handle = NULL;
    self->subscription = NULL;
    self->callback = callback;
    self->rock = rock;
    self->is_pending = 0;
    return self;
}

/* Releases the resources consumed by the receiver */
void
usenet_sub_free(usenet_sub_t self)
{
    /* Free the subscription expression */
    if (self->expression != NULL) {
        free(self->expression);
        self->expression = NULL;
    }

    /* Don't free a pending subscription */
    if (self->is_pending) {
        return;
    }

    /* It should be safe to free the subscription now */
    free(self);
}

/* Adds a new entry to the usenet subscription */
int
usenet_sub_add(usenet_sub_t self,
               int has_not,
               const char *pattern,
               struct usenet_expr *expressions,
               size_t count)
{
    char *entry_sub;
    size_t entry_length;

    /* Figure out what the new entry is */
    entry_sub = alloc_sub(self, has_not, pattern, expressions, count);
    if (entry_sub == NULL) {
        return -1;
    }

    /* And how long it is */
    entry_length = strlen(entry_sub);

    /* If we had no previous expression, then simply create one */
    if (self->expression == NULL) {
        self->expression_size = strlen(ONE_SUB) + entry_length - 1;
        self->expression = malloc(self->expression_size);
        if (self->expression == NULL) {
            free(entry_sub);
            return -1;
        }

        snprintf(self->expression, self->expression_size, ONE_SUB, entry_sub);
    } else {
        /* We need to extend the existing expression */
        size_t new_size;
        char *new_expression;

        /* Expand the expression buffer */
        new_size = self->expression_size + entry_length + 4;
        new_expression = realloc(self->expression, new_size);
        if (new_expression == NULL) {
            return -1;
        }

        /* Overwrite the trailing right paren with the new stuff */
        snprintf(new_expression + self->expression_size - 2,
                 new_size - self->expression_size + 2,
                 " || %s)", entry_sub);
        self->expression = new_expression;
        self->expression_size = new_size;
    }

    /* If we're connected then resubscribe */
    if (self->handle != NULL) {
        fprintf(stderr, PACKAGE ": hmmm\n");
        abort();
    }

    /* Clean up */
    free(entry_sub);
    return 0;
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
    usenet_sub_t self = (usenet_sub_t)rock;

    self->subscription = subscription;
    self->is_pending = 0;

    /* Unsubscribe if we were pending when we were freed */
    if (self->expression == NULL) {
        usenet_sub_set_connection(self, NULL, error);
    }

    return ELVIN_RETURN_SUCCESS;
}

/* Callback for an unsubscribe request */
static
#if !defined(ELVIN_VERSION_AT_LEAST)
void
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
int
#endif
unsubscribe_cb(elvin_handle_t handle,
               int result,
               elvin_subscription_t subscription,
               void *rock,
               elvin_error_t error)
{
    usenet_sub_t self = (usenet_sub_t)rock;

    self->is_pending = 0;

    /* Free the receiver if it was pending when it was freed */
    if (self->expression == NULL) {
        usenet_sub_free(self);
    }

    return ELVIN_RETURN_SUCCESS;
}

/* Sets the receiver's elvin connection */
void
usenet_sub_set_connection(usenet_sub_t self,
                          elvin_handle_t handle,
                          elvin_error_t error)
{
    /* Disconnect from the old connection */
    if (self->handle != NULL && self->subscription != NULL) {
        if (elvin_async_delete_subscription(self->handle,
                                            self->subscription,
                                            unsubscribe_cb, self,
                                            error) == 0) {
            fprintf(stderr, "elvin_async_delete_subscription(): failed\n");
            exit(1);
        }

        self->is_pending = 1;
    }

    /* Connect to the new one */
    self->handle = handle;

    if (self->handle != NULL && self->expression != NULL) {
        if (elvin_async_add_subscription(self->handle,
                                         self->expression, NULL, 1,
                                         notify_cb, self,
                                         subscribe_cb, self,
                                         error) == 0) {
            fprintf(stderr, "elvin_async_add_subscription(): failed\n");
            exit(1);
        }

        self->is_pending = 1;
    }
}

/**********************************************************************/
