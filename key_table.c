/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2002.
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
static const char cvsid[] = "$Id: key_table.c,v 1.3 2002/04/23 16:22:23 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* calloc, free, malloc, realloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memcpy, memset, strcmp, strdup */
#endif
#include "key_table.h"

#define TABLE_MIN_SIZE 8

typedef struct key_entry *key_entry_t;
struct key_entry
{
    /* The label the user gave to the key. */
    char *name;

    /* The key data. */
    char *data;

    /* The length of the key. */
    int length;

    /* Whether the key is private. */
    int is_private;
};

static void key_entry_free(key_entry_t self);

/* Allocates and initializes a new key_entry_t */
static key_entry_t key_entry_alloc(
    char *name,
    char *data,
    int length,
    int is_private)
{
    key_entry_t self;

    /* Allocate memory for the key's context */
    if ((self = malloc(sizeof(struct key_entry))) == NULL)
    {
        return self;
    }

    /* Sanitize it */
    memset(self, 0, sizeof(struct key_entry));

    /* Take a copy of the name */
    if ((self -> name = strdup(name)) == NULL)
    {
        key_entry_free(self);
        return NULL;
    }

    /* Take a copy of the data */
    if ((self -> data = malloc(length)) == NULL)
    {
        key_entry_free(self);
        return NULL;
    }
    memcpy(self -> data, data, length);

    /* Store the other values */
    self -> length = length;
    self -> is_private = is_private;
    return self;
}

/* Releases the resources consumed by the receiver */
static void key_entry_free(key_entry_t self)
{
    /* Free the key name. */
    if (self -> name != NULL)
    {
        free(self -> name);
    }

    /* Free the key data. */
    if (self -> data != NULL)
    {
        free(self -> data);
    }

    /* Free the memory used by the key's context. */
    free(self);
}

struct key_table
{
    /* The entries in the table. */
    key_entry_t *entries;

    /* The number of entries in the table (with keys in them). */
    int entries_used;

    /* The number of entries that can fit in the table. */
    int entries_size;
};

/* Allocates and initializes a new key_table */
key_table_t key_table_alloc()
{
    key_table_t self;

    /* Allocate memory for the new table's context */
    if ((self = (key_table_t)malloc(sizeof(struct key_table))) == NULL)
    {
        return NULL;
    }

    /* Sanitize it */
    memset(self, 0, sizeof(struct key_table));

    /* Initialize its contents */
    self -> entries_used = 0;
    self -> entries_size = TABLE_MIN_SIZE;
    if ((self -> entries = calloc(self -> entries_size, sizeof(key_entry_t))) == NULL)
    {
        key_table_free(self);
        return NULL;
    }

    return self;
}

/* Frees the resources consumed by the key_table */
void key_table_free(key_table_t self)
{
    int i;

    if (self -> entries != NULL)
    {
        /* Free each key in the table. */
        for (i = 0; i < self -> entries_used; i++)
        {
            key_entry_free(self -> entries[i]);
        }

        /* Free the table. */
        free(self -> entries);
    }

    /* Free the table's context. */
    free(self);
}

/* Returns the position of the key with the given name in the table. */
static key_entry_t *key_table_search(key_table_t self, char *name)
{
    int i;

    /* Simple linear search. */
    for (i = 0; i < self -> entries_used; i++)
    {
        if (strcmp(self -> entries[i] -> name, name) == 0)
        {
            /* We found it. */
            return self -> entries + i;
        }
    }

    /* No banana. */
    return NULL;
}

/* Returns the information about the named key. */
int key_table_lookup(
    key_table_t self,
    char *name,
    char **data_out,
    int *length_out,
    int *is_private_out)
{
    key_entry_t *entry;

    /* Find the position of the key entry. */
    if ((entry = key_table_search(self, name)) == NULL)
    {
        return -1;
    }

    /* Return the data. */
    if (data_out != NULL)
    {
        *data_out = (*entry) -> data;
    }

    /* Return the length. */
    if (length_out != NULL)
    {
        *length_out = (*entry) -> length;
    }

    /* Return whether the key is private. */
    if (is_private_out != NULL)
    {
        *is_private_out = (*entry) -> is_private;
    }

    return 0;
}

/* Adds a new key to the table. */
int key_table_add(
    key_table_t self,
    char *name,
    char *data,
    int length,
    int is_private)
{
    key_entry_t entry;

    /* Allocate a new entry. */
    if ((entry = key_entry_alloc(name, data, length, is_private)) == NULL)
    {
        return -1;
    }

    /* Grow the table if necessary */
    if (! (self -> entries_size < self -> entries_used))
    {
        key_entry_t *new_entries;

        if ((new_entries = realloc(self -> entries, self -> entries_size * 2)) == NULL)
        {
            key_entry_free(entry);
            return -1;
        }

        self -> entries = new_entries;
        self -> entries_size *= 2;
    }

    /* Append this key. */
    self -> entries[self -> entries_used++] = entry;
    return 0;
}

/* Remove the key with the given name from the table. */
int key_table_remove(key_table_t self, char *name)
{
    key_entry_t *position;

    /* Find the location of the key in the table. */
    if ((position = key_table_search(self, name)) == NULL)
    {
        return -1;
    }

    /* Free it. */
    key_entry_free(*position);

    /* Fill up the empty space. */
    *position = self -> entries[--self -> entries_used];
    return 0;
}
