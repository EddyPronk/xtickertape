/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2000.
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
static const char cvsid[] = "$Id: subscription.c,v 2.7 2001/08/25 14:04:45 phelps Exp $";
#endif /* lint */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <elvin/elvin.h>
#include <elvin/memory.h>
#include "errors.h"
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

/* Extracts a string value (or null) from the subscription */
static int extract_string(
    subscription_t self,
    env_t env,
    char *name,
    char **string_out,
    elvin_error_t error)
{
    value_t *value;
    value_t result;

    /* If the value isn't present then return NULL */
    if (! (value = env_get(self -> env, name, error)))
    {
	*string_out = NULL;
	return 1;
    }

    /* Evaluate it differently depending on its type */
    switch (value -> type)
    {
	case VALUE_NONE:
	case VALUE_OPAQUE:
	{
	    *string_out = NULL;
	    return 1;
	}

	/* Just convert the value into a string */
	case VALUE_INT32:
	case VALUE_INT64:
	case VALUE_FLOAT:
	case VALUE_STRING:
	case VALUE_LIST:
	{
	    if (! (*string_out = value_to_string(value, error)))
	    {
		return 0;
	    }

	    return 1;
	}

	/* Evaluate the block within the context of the environment */
	case VALUE_BLOCK:
	{
	    if (! ast_eval(value -> value.block, env, &result, error))
	    {
		return 0;
	    }

	    /* Convert the resulting value into a string */
	    switch (result.type)
	    {
		case VALUE_NONE:
		case VALUE_OPAQUE:
		{
		    *string_out = NULL;
		    return value_free(&result, error);
		}

		case VALUE_INT32:
		case VALUE_INT64:
		case VALUE_FLOAT:
		case VALUE_STRING:
		case VALUE_LIST:
		case VALUE_BLOCK:
		{
		    if (! (*string_out = value_to_string(&result, error)))
		    {
			return 0;
		    }

		    return value_free(&result, error);
		}
	    }
	}

	default:
	{
	    fprintf(stderr, "extract_string(): invalid value type: %d\n", value -> type);
	    abort();
	}
    }
}


/* Extracts an int32 value from the environment */
static int extract_int32(
    subscription_t self,
    env_t env,
    char *name,
    int32_t *value_out,
    elvin_error_t error)
{
    value_t *value;
    value_t result;

    /* If the value isn't present then return NULL */
    if (! (value = env_get(self -> env, name, error)))
    {
	*value_out = 0;
	return 1;
    }

    /* Evaluate it differently depending on its type */
    switch (value -> type)
    {
	case VALUE_NONE:
	case VALUE_OPAQUE:
	case VALUE_STRING:
	case VALUE_LIST:
	{
	    *value_out = 0;
	    return 1;
	}

	case VALUE_INT32:
	{
	    *value_out = value -> value.int32;
	    return 1;
	}

	case VALUE_INT64:
	{
	    *value_out = (int32_t)value -> value.int64;
	    return 1;
	}

	case VALUE_FLOAT:
	{
	    *value_out = (int32_t)value -> value.real64;
	    return 1;
	}

	/* Evaluate the block within the context of the environment */
	case VALUE_BLOCK:
	{
	    if (! ast_eval(value -> value.block, env, &result, error))
	    {
		return 0;
	    }

	    /* Convert the resulting value into a string */
	    switch (result.type)
	    {
		case VALUE_NONE:
		case VALUE_STRING:
		case VALUE_OPAQUE:
		case VALUE_LIST:
		case VALUE_BLOCK:
		{
		    *value_out = 0;
		    return 1;
		}

		case VALUE_INT32:
		{
		    *value_out = result.value.int32;
		    return 1;
		}

		case VALUE_INT64:
		{
		    *value_out = (int32_t)result.value.int64;
		    return 1;
		}

		case VALUE_FLOAT:
		{
		    *value_out = (int32_t)result.value.real64;
		    return 1;
		}

		default:
		{
		    fprintf(stderr, "extract_int32(): invalid type: %d\n", value -> type);
		}
	    }

	    return 1;
	}

	default:
	{
	    fprintf(stderr, "extract_int32(): invalid type: %d\n", value -> type);
	    abort();
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

#if 0
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
#endif
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

#if 0
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
#endif
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

#if 0
/* Helper function for notification_to_env */
static int update_env(
    void *rock,
    char *name,
    elvin_basetypes_t type,
    elvin_value_t value,
    elvin_error_t error)
{
    env_t env = (env_t)rock;
    value_t result;

    /* Copy the value into the result */
    switch (type)
    {
	case ELVIN_INT32:
	{
	    result.type = VALUE_INT32;
	    result.value.int32 = value.i;
	    break;
	}

	case ELVIN_INT64:
	{
	    result.type = VALUE_INT64;
	    result.value.int64 = value.h;
	    break;
	}

	case ELVIN_REAL64:
	{
	    result.type = VALUE_FLOAT;
	    result.value.real64 = value.d;
	    break;
	}

	case ELVIN_STRING:
	{
	    result.type = VALUE_STRING;
	    result.value.string = value.s;
	    break;
	}

	case ELVIN_OPAQUE:
	{
	    result.type = VALUE_OPAQUE;
	    result.value.opaque.length = value.o.length;
	    result.value.opaque.data = value.o.data;
	    break;
	}

	default:
	{
	    fprintf(stderr, "invalid type in notification: %d\n", type);
	    abort();
	}
    }

    /* Add the value to the environment */
    if (! env_put(env, name, &result, error))
    {
	return 0;
    }

    return 1;
}

/* Constructs an environment out of a notification */
static env_t notification_to_env(elvin_notification_t notification, elvin_error_t error)
{
    env_t env;

    /* Allocate a new environment */
    if (! (env = env_alloc(error)))
    {
	return NULL;
    }

    /* Traverse the notification and copy fields */
    if (! elvin_notification_traverse(notification, update_env, env, error))
    {
	env_free(env, NULL);
	return NULL;
    }

    return env;
}
#else
/* Assumes that an environment is a supertype of a notification */
static env_t notification_to_env(elvin_notification_t notification, elvin_error_t error)
{
    return (env_t)notification;
}
#endif

/* Transforms a notification into a message */
message_t subscription_transmute(
    subscription_t self,
    elvin_notification_t notification,
    elvin_error_t error)
{
    env_t env;
    char *group = NULL;
    char *user = "anonymous";
    char *string = "no message";
    uint32_t timeout = 0;
    char *mime_type = NULL;
    char *mime_args = NULL;
    char *replace_id = NULL;
    char *message_id = NULL;
    char *reply_id = NULL;
    message_t message;

    /* Transform the notification into an environment */
    if (! (env = notification_to_env(notification, error)))
    {
	return NULL;
    }

    /* FIX THIS: Evaluate the subscription fields */
    /* FIX THIS: an environment is not the same as a notification! */
    if (! extract_string(self, env, "group", &group, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "user", &user, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "message", &string, error))
    {
	return NULL;
    }

    if (! extract_int32(self, env, "timeout", &timeout, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "mime-type", &mime_type, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "mime-args", &mime_args, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "replace-id", &replace_id, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "message-id", &message_id, error))
    {
	return NULL;
    }

    if (! extract_string(self, env, "reply-id", &reply_id, error))
    {
	return NULL;
    }

    /* create a message */
    if (! (message = message_alloc(
	self -> tag,
	group ? group : self -> tag,
	user ? user : "anonymous",
	string ? string : "no message",
	timeout ? timeout : 10,
	mime_type, mime_args,
	replace_id, message_id, reply_id)))
    {
	fprintf(stderr, "message_alloc(): failed\n");
	abort();
    }

    /* Clean up */
#if 0
    if (! env_free(env, error))
    {
	return NULL;
    }
#endif

    if (group && ! ELVIN_FREE(group, error))
    {
	return NULL;
    }

    if (user && ! ELVIN_FREE(user, error))
    {
	return NULL;
    }

    if (string && ! ELVIN_FREE(string, error))
    {
	return NULL;
    }

    if (mime_type && ! ELVIN_FREE(mime_type, error))
    {
	return NULL;
    }

    if (mime_args && ! ELVIN_FREE(mime_args, error))
    {
	return NULL;
    }

    if (replace_id && ! ELVIN_FREE(replace_id, error))
    {
	return NULL;
    }

    if (message_id && ! ELVIN_FREE(message_id, error))
    {
	return NULL;
    }

    if (reply_id && ! ELVIN_FREE(reply_id, error))
    {
	return NULL;
    }

    return message;
}
