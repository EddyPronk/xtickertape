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
static const char cvsid[] = "$Id: subscription.c,v 2.1 2000/07/07 10:42:06 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include "ast.h"
#include "subscription.h"

struct subscription
{
    /* The subscription's unique tag */
    char *tag;

    /* The name that appears in the menu */
    char *name;

    /* The subscription expression */
    char *expression;

    /* The notification keys */
    elvin_keys_t producer_keys;

    /* The subscription keys */
    elvin_keys_t consumer_keys;

    /* Should this subscription appear in the menu? */
    int in_menu;

    /* Should mime attachments automatically be opened? */
    int auto_mime;

    /* The group to use when displaying matching messages */
    value_t group;

    /* The user to use when displaying matching messages */
    value_t user;

    /* The message to use when displaying matching messages */
    value_t message;

    /* The timeout to use when displaying matching messages */
    value_t timeout;

    /* The mime-type to use when displaying attachments */
    value_t mime_type;

    /* The mime-args to use when displaying attachments */
    value_t mime_args;

    /* The message-id to use when replying to a message */
    value_t message_id;

    /* The in-reply-to id to use when threading messages */
    value_t reply_id;
};


/* Allocates and initializes a new subscription */
subscription_t subscription_alloc(
    char *tag,
    char *name,
    uchar *expression,
    int in_menu,
    int auto_mime,
    elvin_keys_t producer_keys,
    elvin_keys_t consumer_keys,
    value_t group,
    value_t user,
    value_t message,
    value_t timeout,
    value_t mime_type,
    value_t mime_args,
    value_t message_id,
    value_t reply_id,
    elvin_error_t error)
{
    printf("tag: %s\n", tag ? tag : "NULL");
    printf("name: %s\n", name ? name : "NULL");
    printf("expression: %s\n", expression ? (char *)expression : "NULL");
    printf("in-menu: %s\n", in_menu ? "true" : "false");
    printf("auto-mime: %s\n", auto_mime ? "true" : "false");
    printf("producer_keys: %p\n", producer_keys);
    printf("consumer_keys: %p\n\n", consumer_keys);
    return (subscription_t)-1;
}

/* Frees the resources consumed by the subscription */
int subscription_free(subscription_t self, elvin_error_t error)
{
    fprintf(stderr, "subscription_free(): not yet implemented\n");
    abort();
}
