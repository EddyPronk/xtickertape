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
static const char cvsid[] = "$Id: history.c,v 1.11 1999/08/22 08:14:31 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <Xm/List.h>
#include "history.h"

#define MAX_LIST_COUNT 30
/*#define MAX_LIST_COUNT 5*/

/* The structure of a node in a message history */
typedef struct history_node *history_node_t;
struct history_node
{
    /* The node's message */
    Message message;

    /* The previous node (by order of receipt) */
    history_node_t previous;

    /* The node to which the receiver is a reply */
    history_node_t parent;

    /* The previous response (to the receiver's parent) node */
    history_node_t previous_response;

    /* The receiver's total number of `offspring' (children, grandchildren, etc) */
    int child_count;

    /* The last response to the receiver */
    history_node_t last_response;
};


/* Allocates and initializes a new history_node_t */
static history_node_t history_node_alloc(Message message)
{
    history_node_t self;

    /* Allocate memory for the new node */
    if ((self = (history_node_t) malloc(sizeof(struct history_node))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> message = Message_allocReference(message);
    self -> previous = NULL;
    self -> parent = NULL;
    self -> previous_response = NULL;
    self -> child_count = 0;
    self -> last_response = NULL;

    return self;
}

/* Releases the resources consumed by the receiver */
static void history_node_free(history_node_t self)
{
    /* Release our reference to the Message */
    Message_free(self -> message);
    free(self);
}

/* Answers the indentation level of the receiver */
static int history_node_indent(history_node_t self)
{
    history_node_t probe = self -> parent;
    int count = 0;

    /* Count how many ancestors we have */
    while (probe != NULL)
    {
	probe = probe -> parent;
	count++;
    }

    return count;
}

/* Constructs a Motif string representation of the receiver's message */
static XmString history_node_string(history_node_t self, int is_threaded)
{
    char *group = Message_getGroup(self -> message);
    char *user = Message_getUser(self -> message);
    char *string = Message_getString(self -> message);
    int spaces = is_threaded ? (history_node_indent(self) * 2) : 0;
    char *buffer = (char *) malloc(strlen(group) + strlen(user) + strlen(string) + spaces + 3);
    XmString result;
    int i;

    /* Set up the indent */
    for (i = 0; i < spaces; i++)
    {
	buffer[i] = ' ';
    }

    sprintf(buffer + spaces, "%s:%s:%s", group, user, string);
    result = XmStringCreateSimple(buffer);
    free(buffer);

    return result;
}

/* Prints the receiver to the output file */
static void history_node_print(history_node_t self, int indent, FILE *out)
{
    int i;

    for (i = 0; i < indent; i++)
    {
	fprintf(out, "  ");
    }

    fprintf(out, "%s:%s:%s\n",
	    Message_getGroup(self -> message),
	    Message_getUser(self -> message),
	    Message_getString(self -> message));
}

/* Prints the history list ending with the receiver */
static void history_node_print_unthreaded(history_node_t self, FILE *out)
{
    /* Watch for the beginning of history */
    if (self == NULL)
    {
	return;
    }

    /* Print all of the previous nodes followed by this one */
    history_node_print_unthreaded(self -> previous, out);
    history_node_print(self, 0, out);
}


/* Prints the threaded history list ending with the receiver */
static void history_node_print_threaded(history_node_t self, FILE *out)
{
    /* Watch for the beginning of history */
    if (self == NULL)
    {
	return;
    }

    /* Print the previous responses */
    history_node_print_threaded(self -> previous_response, out);

    /* Print the receiver */
    history_node_print(self, history_node_indent(self), out);

    /* Print our responses */
    history_node_print_threaded(self -> last_response, out);
}


/* The structure of a threaded (or not) message history */
struct history
{
    /* The number of nodes in the history */
    int count;

    /* Nonzero if the history is being displayed threaded */
    int is_threaded;

    /* The last node in the history (in receipt order) */
    history_node_t last;

    /* The last thread in the history */
    history_node_t last_thread;

    /* The receiver's Motif List widget */
    Widget list;
};


/* Allocates and initializes a new empty history */
history_t history_alloc()
{
    history_t self;

    /* Allocate memory for the new node */
    if ((self = (history_t) malloc(sizeof(struct history))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> count = 0;
    self -> is_threaded = 1;
    self -> last = NULL;
    self -> last_thread = NULL;
    self -> list = NULL;

    return self;
}

/* Frees the history */
void history_free(history_t self)
{
    history_node_t probe;
    
    /* Free each node */
    probe = self -> last;
    while (probe != NULL)
    {
	history_node_t previous = probe -> previous;
	history_node_free(probe);
	probe = previous;
    }

    free(self);
}

/* Finds the node whose Message has the given id */
static history_node_t find_by_id(history_t self, char *id)
{
    history_node_t node;

    /* Watch for the no-id case */
    if (id == NULL)
    {
	return NULL;
    }

    /* Search the history from youngest to oldest */
    for (node = self -> last; node != NULL; node = node -> previous)
    {
	/* Return if we have a match */
	if (strcmp(Message_getId(node -> message), id) == 0)
	{
	    return node;
	}
    }

    /* Not found */
    return NULL;
}

/* Sets up pointers to cope with threaded messages */
static void history_thread_node(history_t self, history_node_t node)
{
    history_node_t parent;

    /* If the message's parent node isn't in the history then pretend there is no parent */
    if ((parent = find_by_id(self, Message_getReplyId(node -> message))) == NULL)
    {
	node -> previous_response = self -> last_thread;
	self -> last_thread = node;
	return;
    }

    /* Append this node to the end of the parent's replies */
    node -> parent = parent;
    node -> previous_response = parent -> last_response;
    parent -> last_response = node;

    /* Update the offspring count */
    while (parent != NULL)
    {
	parent -> child_count++;
	parent = parent -> parent;
    }
}

/* Sets the history's Motif List widget */
void history_set_list(history_t self, Widget list)
{
    self -> list = list;
}


/* Constructs a threaded version of the history */
static void history_node_get_strings_threaded(history_node_t self, XmString *items, int max)
{
    int index;

    for (index = 0; index < max; index++)
    {
	items[index] = XmStringCreateSimple("not yet implemented");
    }
}

/* Constructs an unthreaded version of the history */
static void history_node_get_strings_unthreaded(history_node_t self, XmString *items, int max)
{
    int index;

    for (index = 0; index < max; index++)
    {
	items[index] = XmStringCreateSimple("not yet implemented");
    }
}

/* Allocates and returns an array of XmStrings to be used for the receiver's list */
static XmString *history_get_strings(history_t self, int count)
{
    XmString *items = (XmString *)calloc(count, sizeof(XmString));

    if (self -> is_threaded)
    {
	history_node_get_strings_threaded(self -> last_thread, items, count);
	return items;
    }
    else
    {
	history_node_get_strings_unthreaded(self -> last, items, count);
	return items;
    }
}

/* Sets the history's threadedness */
void history_set_threaded(history_t self, int is_threaded)
{
    int count;
    XmString *items;
    int index;

    /* Make sure we're changing the value */
    if (self -> is_threaded == is_threaded)
    {
	return;
    }

    /* Set our `threadedness' */
    self -> is_threaded = is_threaded;

    /* If we don't have a list to update then we're done */
    if (self -> list == NULL)
    {
	return;
    }

    /* Update the list */
    /* FIX THIS: this can be much more efficient */
    count = (self -> count < MAX_LIST_COUNT) ? self -> count : MAX_LIST_COUNT;
    items = history_get_strings(self, count);

    /* Replace the old list with the new one */
    XmListReplaceItemsPos(self -> list, items, count, 1);

    /* Free the strings */
    for (index = 0; index < count; index++)
    {
	XmStringFree(items[index]);
    }

    /* Free the array */
    free(items);

    /* Probably need to reselect */
}


/* Updates the history's list widget after a message was added */
static void history_update_list(history_t self, history_node_t node)
{
    XmString string;

    /* Make sure we have a list to play with */
    if (self -> list == NULL)
    {
	return;
    }

    /* Make sure the list doesn't grow without bound */
    if (self -> count > MAX_LIST_COUNT)
    {
	XmListDeletePos(self -> list, 1);
    }

    /* Add the string to the end of the list */
    string = history_node_string(node, self -> is_threaded);
    XmListAddItem(self -> list, string, history_index(self, node -> message));
    XmStringFree(string);
}

/* Adds a message to the end of the history */
int history_add(history_t self, Message message)
{
    history_node_t node;

    /* Allocate a node to hold the message */
    if ((node = history_node_alloc(message)) == NULL)
    {
	return -1;
    }

    /* If the message is part of a thread then deal with it */
    history_thread_node(self, node);

    /* Append the node to the end of the history */
    self -> count++;
    node -> previous = self -> last;
    self -> last = node;

    /* Update the List widget */
    history_update_list(self, node);

    return 0;
}

/* Answers the Message at the given index */
static Message history_unthreaded_get(history_t self, int index)
{
    history_node_t probe;
    int i = self -> count;

    /* Search for our victim */
    for (probe = self -> last; probe != NULL; probe = probe -> previous)
    {
	if (index == i)
	{
	    return probe -> message;
	}

	i--;
    }

    return NULL;
}


/* Answers the Message at the given index */
static Message history_threaded_get(history_t self, int index)
{
    history_node_t node = self -> last_thread;
    int max = self -> count + 1;

    /* Watch for out of bounds */
    if (! (index < max))
    {
	return NULL;
    }

    /* Keep looking until we run out of nodes */
    while (node != NULL)
    {
	int node_index = max - (node -> child_count + 1);

	if (index < node_index)
	{
	    /* Try previous thread */
	    max = node_index;
	    node = node -> previous_response;
	}
	else if (node_index == index)
	{
	    /* Found a match */
	    return node -> message;
	}
	else
	{
	    /* Try a response to this node */
	    node = node -> last_response;
	}
    }

    return NULL;
}

/* Answers the Message at the given index */
Message history_get(history_t self, int index)
{
    int i = index;

    /* Convert the index into an absolute number */
    if (self -> count > MAX_LIST_COUNT)
    {
	i += (self -> count - MAX_LIST_COUNT);
    }

    /* Make sure the index is not out-of-bounds */
    if ((i < 1) || (i > self -> count))
    {
	return NULL;
    }

    /* Locate the node */
    if (self -> is_threaded)
    {
	return history_threaded_get(self, i);
    }
    else
    {
	return history_unthreaded_get(self, i);
    }
}

/* Answers the index of given Message in the history */
static int history_unthreaded_index(history_t self, Message message)
{
    history_node_t probe;
    int index;

    index = self -> count;
    for (probe = self -> last; probe != NULL; probe = probe -> previous)
    {
	if (probe -> message == message)
	{
	    return index;
	}

	index--;
    }

    return -1;
}

/* Answers the absolute index of the given node in the history */
static int history_node_threaded_index(history_node_t self)
{
    history_node_t thread;
    int count = 0;

    /* Go through each layer of threading */
    for (thread = self; thread != NULL; thread = thread -> parent)
    {
	history_node_t probe = thread -> previous_response;

	count++;
	while (probe != NULL)
	{
	    count += probe -> child_count + 1;
	    probe = probe -> previous_response;
	}
    }

    return count;
}

/* Answers the absolute index of the given Message in the history */
static int history_threaded_index(history_t self, Message message)
{
    history_node_t node;

    /* Locate the node for the message */
    for (node = self -> last; node != NULL; node = node -> previous)
    {
	if (node -> message == message)
	{
	    return history_node_threaded_index(node);
	}
    }

    return -1;
}

/* Answers the index of given Message in the history */
int history_index(history_t self, Message message)
{
    int index;

    if (self -> is_threaded)
    {
	index = history_threaded_index(self, message);
    }
    else
    {
	index = history_unthreaded_index(self, message);
    }

    if (self -> count > MAX_LIST_COUNT)
    {
	index -= self -> count - MAX_LIST_COUNT;
    }

    return (index < 1) ? -1 : index;
}

/* Just for debugging */
void history_print(history_t self, FILE *out)
{
    fprintf(out, "unthreaded (%d):\n", self -> count);

    /* Fob this off on the nodes */
    history_node_print_unthreaded(self -> last, out);
}

void history_print_threaded(history_t self, FILE *out)
{
    fprintf(out, "threaded (%d):\n", self -> count);

    /* Fob this off on the nodes */
    history_node_print_threaded(self -> last_thread, out);
}

