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
static const char cvsid[] = "$Id: history.c,v 1.24 1999/10/07 00:23:58 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/IntrinsicP.h>
#include <Xm/List.h>
#include "glyph.h"
#include "ScrollerP.h"
#include "history.h"

#ifdef DEBUG
#define MAX_LIST_COUNT 5
#else /* DEBUG */
#define MAX_LIST_COUNT 30
#endif /* DEBUG */


/* The structure of a node in a message history */
typedef struct history_node *history_node_t;
struct history_node
{
    /* The node's reference count */
    int ref_count;

    /* True if the receiver has been killed */
    int is_killed;

    /* The node's message */
    message_t message;

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
static history_node_t history_node_alloc(message_t message)
{
    history_node_t self;

    /* Allocate memory for the new node */
    if ((self = (history_node_t) malloc(sizeof(struct history_node))) == NULL)
    {
	return NULL;
    }

    /* Initialize the fields to sane values */
    self -> ref_count = 1;
    self -> is_killed = False;
    self -> message = message_alloc_reference(message);
    self -> previous = NULL;
    self -> parent = NULL;
    self -> previous_response = NULL;
    self -> child_count = 0;
    self -> last_response = NULL;

    return self;
}

/* Allocates another reference to a history_node_t */
static history_node_t history_node_alloc_reference(history_node_t self)
{
    self -> ref_count++;
    return self;
}

/* Releases the resources consumed by the receiver */
static void history_node_free(history_node_t self)
{
    /* Decrement the reference count */
    if ((--self -> ref_count > 0) || (self -> child_count > 0))
    {
	return;
    }

    /* Release our reference to the message */
    message_free(self -> message);
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
    char *group = message_get_group(self -> message);
    char *user = message_get_user(self -> message);
    char *string = message_get_string(self -> message);
    int spaces = is_threaded ? (history_node_indent(self) * 2) : 0;
    char *buffer = (char *) malloc(strlen(group) + strlen(user) + strlen(string) + spaces + 3);
    char *pointer = buffer;
    char *tag;
    XmString result;
    int i;

    /* Set up the indent */
    for (i = 0; i < spaces; i++)
    {
	*(pointer++) = ' ';
    }

    /* Print the message */
    sprintf(pointer, "%s:%s:%s", group, user, string);
    tag = message_has_attachment(self -> message) ? "MIME" : XmFONTLIST_DEFAULT_TAG;
    result = XmStringCreate(buffer, tag);

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
	    message_get_group(self -> message),
	    message_get_user(self -> message),
	    message_get_string(self -> message));
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
    /* Nonzero if the history is being displayed threaded */
    int is_threaded;

    /* The last node in the history (in receipt order) */
    history_node_t last;

    /* The number of nodes in the threaded history */
    int threaded_count;

    /* The last thread in the history */
    history_node_t last_response;

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
    self -> threaded_count = 0;
    self -> is_threaded = 1;
    self -> last = NULL;
    self -> last_response = NULL;
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

/* Finds the node whose message has the given id */
static history_node_t find_by_id(history_node_t self, char *id)
{
    history_node_t node;

    /* Watch for the no-id case */
    if (id == NULL)
    {
	return NULL;
    }

    /* Search the threaded version of the history */
    for (node = self; node != NULL; node = node -> previous_response)
    {
	char *node_id;
	history_node_t probe;

	/* Watch for a matching id */
	if (((node_id = message_get_id(node -> message)) != NULL) &&
	    (strcmp(id, node_id) == 0))
	{
	    return node;
	}

	/* Check the children */
	if ((probe = find_by_id(node -> last_response, id)) != NULL)
	{
	    return probe;
	}
    }

    /* Not found */
    return NULL;
}

/* Sets up pointers to cope with threaded messages */
static void history_thread_node(history_t self, history_node_t node)
{
    history_node_t parent;

    /* Allocate a reference to the node and add it to our count */
    history_node_alloc_reference(node);
    self -> threaded_count++;

    /* If the message's parent node isn't in the history then pretend there is no parent */
    if ((parent = find_by_id(
	self -> last_response,
	message_get_reply_id(node -> message))) == NULL)
    {
	node -> previous_response = self -> last_response;
	self -> last_response = node;
	return;
    }

    /* Append this node to the end of the parent's replies */
    node -> parent = parent;
    node -> is_killed = parent -> is_killed;
    node -> previous_response = parent -> last_response;
    parent -> last_response = node;

    /* Update the offspring count */
    while (parent != NULL)
    {
	parent -> child_count++;
	parent = parent -> parent;
    }
}


/* Deletes previous_response pointers to the node */
static void history_unthread_node(history_t self, history_node_t node)
{
    history_node_t parent;
    history_node_t probe;

    /* If the node has children then we can't do anything to it yet */
    if (node -> child_count > 0)
    {
	return;
    }

    /* Reduce the child_count of all parent nodes */
    parent = node -> parent;
    for (probe = parent; probe != NULL; probe = probe -> parent)
    {
	probe -> child_count--;
    }

    /* Figure out which list of responses contains the node */
    probe = (parent == NULL) ? self -> last_response : parent -> last_response;

    /* Free this node */
    self -> threaded_count--;
    history_node_free(node);

    /* Watch for the node being the last response */
    if (probe == node)
    {
	/* Remove this node from the parent's responses */
	parent -> last_response = node -> previous_response;
	node -> parent = NULL;

	/* Unthread the parent if appropriate */
	history_unthread_node(self, parent);
	return;
    }

    /* Remove this node from the list of responses */
    while (probe -> previous_response != NULL)
    {
	if (probe -> previous_response == node)
	{
	    /* Disconnect the node from the list of responses */
	    probe -> previous_response = NULL;
	    return;
	}
	    
	probe = probe -> previous_response;
    }

    /* Not found!? */
    fprintf(stderr, "*** Unable to delete the blasted node!\n");
}


/* Sets the history's Motif List widget */
void history_set_list(history_t self, Widget list)
{
    self -> list = list;
}


/* Constructs a threaded version of the history */
static void history_node_get_strings_threaded(history_node_t self, XmString *items, int count)
{
    int max = count;
    history_node_t node = self;

    /* Go through each thread until we run out of room */
    while ((node != NULL) && (max >= 0))
    {
	int index;

	/* Add the start of the thread to the table */
	if ((index = max - (node -> child_count + 1)) >= 0)
	{
	    items[index] = history_node_string(node, True);
	}

	/* Recursively add its children */
	history_node_get_strings_threaded(node -> last_response, items, max);

	/* Go on to the next thread */
	node = node -> previous_response;
	max = index;
    }
}

/* Constructs an unthreaded version of the history */
static void history_get_strings_unthreaded(history_t self, XmString *items, int max)
{
    history_node_t node;
    int index = max;

    for (node = self -> last; node != NULL; node = node -> previous)
    {
	if (index < 0)
	{
	    return;
	}

	items[--index] = history_node_string(node, False);
    }
}

/* Allocates and returns an array of XmStrings to be used for the receiver's list */
static XmString *history_get_strings(history_t self, int count)
{
    XmString *items = (XmString *)calloc(count, sizeof(XmString));

    if (self -> is_threaded)
    {
	history_node_get_strings_threaded(self -> last_response, items, count);
	return items;
    }
    else
    {
	history_get_strings_unthreaded(self, items, count);
	return items;
    }
}

/* Answers the number of messages in the visible portion of the receiver */
static int history_count(history_t self)
{
    /* Cheat -- the threaded_count will be the answer until we have at
     * least MAX_LIST_COUNT messages in the history */
    return self -> threaded_count < MAX_LIST_COUNT ? 
	self -> threaded_count : MAX_LIST_COUNT;
}

/* Answers the index of the currently selected item */
static int history_get_selection_index(history_t self)
{
    int *selection_positions;
    int count;

    /* Ask the widget */
    if (XmListGetSelectedPos(self -> list, &selection_positions, &count))
    {
	return selection_positions[0];
    }

    /* No selection */
    return 0;
}

/* Sets the history's threadedness */
void history_set_threaded(history_t self, int is_threaded)
{
    message_t selection;
    int count;
    XmString *items;
    int index;

    /* Make sure we're changing the value */
    if (self -> is_threaded == is_threaded)
    {
	return;
    }

    /* If we don't have a list to update then we're done */
    if (self -> list == NULL)
    {
	self -> is_threaded = is_threaded;
	return;
    }

    /* Remember the current selection (if any) */
    if ((index = history_get_selection_index(self)) == 0)
    {
	selection = NULL;
    }
    else
    {
	selection = history_get(self, index);
    }

    /* Switch our `threadedness' */
    self -> is_threaded = is_threaded;

    /* Update the list */
    count = history_count(self);
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

    /* Locate the previously selected item */
    index = history_index(self, selection);

    /* Re-select the previously selected item */
    XmListSelectPos(self -> list, index, False);
}

/* Answers the message at the given index */
static history_node_t history_get_node_threaded(history_t self, int index)
{
    history_node_t node = self -> last_response;
    int max = history_count(self) + 1;

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
	    return node;
	}
	else
	{
	    /* Try a response to this node */
	    node = node -> last_response;
	}
    }

    return NULL;
}

/* Answers the message at the given index */
static history_node_t history_get_node_unthreaded(history_t self, int index)
{
    history_node_t probe;
    int i = history_count(self);

    /* Search for our victim */
    for (probe = self -> last; probe != NULL; probe = probe -> previous)
    {
	if (index == i)
	{
	    return probe;
	}

	i--;
    }

    return NULL;
}

/* Answers the history_node_t at the given index */
history_node_t history_get_node(history_t self, int index)
{
    /* Make sure the index is not out-of-bounds */
    if ((index > self -> threaded_count) || (index > MAX_LIST_COUNT))
    {
	return NULL;
    }

    /* Locate the node */
    if (self -> is_threaded)
    {
	return history_get_node_threaded(self, index);
    }
    else
    {
	return history_get_node_unthreaded(self, index);
    }
}

/* Answers the message at the given index */
message_t history_get(history_t self, int index)
{
    history_node_t node;

    if ((node = history_get_node(self, index)) == NULL)
    {
	return NULL;
    }

    return node -> message;
}

/* Answers the message_t the given position */
message_t history_get_at_point(history_t self, int x, int y)
{
    return history_get(self, XmListYToPos(self -> list, y));
}


/* Removes the oldest items from the threaded and unthreaded lists */
static void history_remove_old(history_t self)
{
    history_node_t probe;
    history_node_t previous;

    /* Make sure we've got a full list first */
    if (self -> threaded_count <= MAX_LIST_COUNT)
    {
	return;
    }

    /* Update the List widget if we have one */
    if (self -> list != NULL)
    {
	XmListDeletePos(self -> list, 1);
    }

    /* Locate the oldest node in the unthreaded list */
    probe = self -> last;
    previous = probe -> previous;
    while ((previous != NULL) && (previous -> previous != NULL))
    {
	probe = previous;
	previous = probe -> previous;
    }

    /* Remove it from the unthreaded list */
    probe -> previous = NULL;
    history_node_free(previous);

    /* Free references to the former first item on the threaded list */
    history_unthread_node(self, history_get_node_threaded(self, 0));
}

/* Adds a message to the end of the history */
int history_add(history_t self, message_t message)
{
    history_node_t node;
    int top;
    int count;

    /* Figure out what's visible before we add this message */
    XtVaGetValues(
	self -> list,
	XmNtopItemPosition, &top,
	XmNvisibleItemCount, &count,
	NULL);

    /* Allocate a node to hold the message */
    if ((node = history_node_alloc(message)) == NULL)
    {
	return -1;
    }

    /* If the message is part of a thread then deal with it */
    history_thread_node(self, node);

    /* Append the node to the end of the history */
    node -> previous = self -> last;
    self -> last = node;

    /* Make sure the list doesn't get too big */
    history_remove_old(self);

    /* Update the List widget */
    if (self -> list != NULL)
    {
	/* Add the item to the List widget */
	XmString string = history_node_string(node, self -> is_threaded);
	XmListAddItem(self -> list, string, history_index(self, node -> message));
	XmStringFree(string);

	/* If the bottom was visible before then make sure it remains so */
	if (top + count >= history_count(self))
	{
	    XmListSetBottomPos(self -> list, history_count(self));
	}
    }

    return node -> is_killed;
}



/* Answers the index of given message in the history */
static int history_index_unthreaded(history_t self, message_t message)
{
    history_node_t probe;
    int index;

    index = history_count(self);
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

/* Answers the absolute index of the given node in the history.
 * The index of the first node should be before_index + 1 */
static int history_node_index_threaded(history_node_t self, int before_index)
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

    return count + before_index;
}

/* Answers the absolute index of the given message in the history */
static int history_index_threaded(history_t self, message_t message)
{
    history_node_t node;

    /* Locate the node for the message */
    for (node = self -> last; node != NULL; node = node -> previous)
    {
	if (node -> message == message)
	{
	    int before_index = history_count(self) - self -> threaded_count;
	    return history_node_index_threaded(node, before_index);
	}
    }

    return -1;
}

/* Answers the index of given message in the history */
int history_index(history_t self, message_t message)
{
    if (self -> is_threaded)
    {
	return history_index_threaded(self, message);
    }

    return history_index_unthreaded(self, message);
}


/* Kill off a thread */
void history_node_kill_thread(history_node_t self, ScrollerWidget scroller)
{
    history_node_t child;

    /* If we're already killed then there's nothing left to do */
    if (self -> is_killed)
    {
	return;
    }

    /* Delete the receiver */
    self -> is_killed = True;
    ScDeleteMessage(scroller, self -> message);

    /* Kill its children */
    for (child = self -> last_response; child != NULL; child = child -> previous_response)
    {
	history_node_kill_thread(child, scroller);
    }
}

/* Kill off a thread */
void history_kill_thread(history_t self, ScrollerWidget scroller, message_t message)
{
    history_node_t node;

    /* Locate the node for the message */
    for (node = self -> last; node != NULL; node = node -> previous)
    {
	if (node -> message == message)
	{
	    history_node_kill_thread(node, scroller);
	}
    }
}



/* Just for debugging */
void history_print_unthreaded(history_t self, FILE *out)
{
    fprintf(out, "unthreaded (??):\n");

    /* Fob this off on the nodes */
    history_node_print_unthreaded(self -> last, out);
}

void history_print_threaded(history_t self, FILE *out)
{
    fprintf(out, "threaded (%d):\n", self -> threaded_count);

    /* Fob this off on the nodes */
    history_node_print_threaded(self -> last_response, out);
}

