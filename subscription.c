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
static const char cvsid[] = "$Id: subscription.c,v 2.3 2000/07/10 12:45:15 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "ast.h"
#include "subscription.h"

#define DEFAULT_EXP "TICKERTAPE == '%s'"

struct subscription
{
    /* The environment to use when evaluating notifications */
    env_t env;

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
};



/* Returns the default expression for a subscription */
uchar *alloc_default_expression(char *name, elvin_error_t error)
{
    uchar *buffer;

    /* FIX THIS: we need to escape single quotes and backslashes */
    if (! (buffer = (uchar *)ELVIN_MALLOC(sizeof(DEFAULT_EXP) + strlen(name), error)))
    {
	return NULL;
    }

    sprintf((char *)buffer, DEFAULT_EXP, name);
    return buffer;
}

/* Evaluates a value_t to 0 or 1 */
static int eval_bool(value_t *value, elvin_error_t error)
{
    switch (value -> type)
    {
	/* Nil is equivalent to false */
	case VALUE_NONE:
	{
	    return 0;
	}

	/* Zero is false, anything else is true */
	case VALUE_INT32:
	{
	    return value -> value.int32 != 0;
	}

	/* Ditto */
	case VALUE_INT64:
	{
	    return value -> value.int64 != 0;
	}

	case VALUE_FLOAT:
	{
	    return value -> value.real64 != 0.0;
	}

	/* Anything else is simply true */
	default:
	{
	    return 1;
	}
    }
}

/* Converts a list of strings into an elvin_keys_t */
static int eval_producer_keys(
    value_t *value,
    elvin_keys_t *keys_out,
    elvin_error_t error)
{
    elvin_keys_t keys;
    elvin_key_t key;
    uint32_t count, i;

    /* Don't accidentally return anything interesting */
    *keys_out = NULL;

    /* Make sure we were given a list */
    if (value -> type != VALUE_LIST)
    {
	fprintf(stderr, "type mismatch: keys must be lists\n");
	return 0;
    }

    /* If the list is empty then don't bother creating a key set */
    if (value -> value.list.count == 0)
    {
	return 1;
    }

    /* Allocate some room for the key set */
    count = value -> value.list.count;
    if (! (keys = elvin_keys_alloc(count, error)))
    {
	return 0;
    }

    /* Go through the list and add keys */
    for (i = 0; i < count; i++)
    {
	value_t *item = &value -> value.list.items[i];

	/* Make sure the key is a string */
	if (item -> type != VALUE_STRING)
	{
	    fprintf(stderr, "bad producer key type: %d\n", value -> type);
	    return 0;
	}

	/* Convert it to a producer key */
	if (! (key = elvin_keyraw_alloc(
	    item -> value.string,
	    strlen(item -> value.string),
	    error)))
	{
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Add it to the key set */
	if (! elvin_keys_add(keys, key, error))
	{
	    elvin_key_free(key, NULL);
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Release our reference to the key */
	if (! elvin_key_free(key, error))
	{
	    return 0;
	}
    }

    /* Success! */
    *keys_out = keys;
    return 1;
}

/* Converts a list of strings into an elvin_keys_t */
static int eval_consumer_keys(
    value_t *value,
    elvin_keys_t *keys_out,
    elvin_error_t error)
{
    elvin_keys_t keys;
    elvin_key_t key;
    uint32_t i;

    /* Don't accidentally return anything interesting */
    *keys_out = NULL;

    /* If the list is empty then don't bother creating a key set */
    if (value -> value.list.count == 0)
    {
	return 1;
    }

    /* Allocate some room for the key set */
    if (! (keys = elvin_keys_alloc(value -> value.list.count, error)))
    {
	return 0;
    }

    /* Go through the list and add keys */
    for (i = 0; i < value -> value.list.count; i++)
    {
	value_t *item = &value -> value.list.items[i];

	if (item -> type != VALUE_STRING)
	{
	    fprintf(stderr, "bad consumer key type: %d\n", item -> type);
	    return 0;
	}

	/* Convert it into a consumer key */
	if (! (key = elvin_keyprime_from_hexstring(item -> value.string, error)))
	{
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Add it to the key set */
	if (! elvin_keys_add(keys, key, error))
	{
	    elvin_key_free(key, NULL);
	    elvin_keys_free(keys, NULL);
	    return 0;
	}

	/* Release our reference to the key */
	if (! elvin_key_free(key, error))
	{
	    return 0;
	}
    }

    /* Success! */
    *keys_out = keys;
    return 1;
}



/* Allocates and initializes a new subscription */
subscription_t subscription_alloc(char *tag, env_t env, elvin_error_t error)
{
    subscription_t self;
    value_t *value;

    /* Allocate some memory for the new subscription */
    if (! (self = (subscription_t)ELVIN_MALLOC(sizeof(struct subscription), error)))
    {
	return NULL;
    }

    /* Set all of the values to sane things */
    memset(self, 0, sizeof(struct subscription));

    /* Record the environment */
    self -> env = env;

    /* Copy the tag string */
    if (! (self -> tag = ELVIN_STRDUP(tag, error)))
    {
	subscription_free(self, NULL);
	return NULL;
    }

    printf("tag=`%s'\n", tag);

    /* Get the name out of the environment */
    if (! (value = env_get(env, "name", error)))
    {
	if (! (self -> name = ELVIN_STRDUP(self -> tag, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }
    else
    {
	if (! (self -> name = value_to_string(value, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }

    /* Get the subscription expression */
    if (! (value = env_get(env, "subscription", error)))
    {
	if (! (self -> expression = alloc_default_expression(self -> name, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }
    else
    {
	if (! (self -> expression = value_to_string(value, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }

    /* Get the producer keys */
    if (! (value = env_get(env, "producer-keys", error)))
    {
	self -> producer_keys = NULL;
    }
    else
    {
	if (! (eval_producer_keys(value, &self -> producer_keys, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }

    /* Get the consumer keys */
    if (! (value = env_get(env, "consumer-keys", error)))
    {
	self -> consumer_keys = NULL;
    }
    else
    {
	if (! (eval_consumer_keys(value, &self -> consumer_keys, error)))
	{
	    subscription_free(self, NULL);
	    return NULL;
	}
    }


    /* Should this subscription be in the menu? */
    if (! (value = env_get(env, "in-menu", error)))
    {
	self -> in_menu = 1;
    }
    else
    {
	self -> in_menu = eval_bool(value, error);
    }

    /* Is the mime nazi enabled? */
    if (! (value = env_get(env, "auto-mime", error)))
    {
	self -> auto_mime = 0;
    }
    else
    {
	self -> auto_mime = eval_bool(value, error);
    }

    return self;
}

/* Frees the resources consumed by the subscription */
int subscription_free(subscription_t self, elvin_error_t error)
{
    /* Free the evaluation environment */
    if (self -> env != NULL)
    {
	if (! elvin_hash_free(self -> env, error))
	{
	    return 0;
	}
    }

    /* Free the tag */
    if (self -> tag != NULL)
    {
	if (! ELVIN_FREE(self -> tag, error))
	{
	    return 0;
	}
    }

    /* Free the name */
    if (self -> name != NULL)
    {
	if (! ELVIN_FREE(self -> name, error))
	{
	    return 0;
	}
    }

    /* Free the subscription expression */
    if (self -> expression != NULL)
    {
	if (! ELVIN_FREE(self -> expression, error))
	{
	    return 0;
	}
    }

    /* Free the notification keys */
    if (self -> producer_keys != NULL)
    {
	if (! elvin_keys_free(self -> producer_keys, error))
	{
	    return 0;
	}
    }

    /* Free the subscription keys */
    if (self -> consumer_keys != NULL)
    {
	if (! elvin_keys_free(self -> consumer_keys, error))
	{
	    return 0;
	}
    }

    return ELVIN_FREE(self, error);
}
