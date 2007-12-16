/***********************************************************************

  Copyright (C) 2002, 2004 by Mantara Software
  (ABN 17 105 665 594). All Rights Reserved.

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

#ifndef lint
static const char cvsid[] = "$Id: key_table.c,v 1.11 2007/12/16 23:54:28 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STDIO_H
#include <stdio.h> /* fprintf */
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* calloc, free, malloc, qsort, realloc */
#endif
#ifdef HAVE_STRING_H
#include <string.h> /* memcpy, memset, strcmp, strdup, strerror */
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h> /* errno */
#endif
#include <elvin/elvin.h>
#include "replace.h"
#include "key_table.h"

#define TABLE_MIN_SIZE 8

#if !defined(ELVIN_VERSION_AT_LEAST)
#define ELVIN_SHA1_DIGESTLEN SHA1DIGESTLEN
#define elvin_sha1_digest(client, data, length, public_key, error) \
    elvin_sha1digest(data, length, public_key)
#define ELVIN_KEYS_ALLOC(client, error) \
    elvin_keys_alloc(error)
#define ELVIN_KEYS_ADD(keys, scheme, key_set_index, bytes, length, rock, error) \
    elvin_keys_add(keys, scheme, key_set_index, bytes, length, error)
#elif ELVIN_VERSION_AT_LEAST(4, 1, -1)
#define ELVIN_KEYS_ADD elvin_keys_add
#define ELVIN_KEYS_ALLOC elvin_keys_alloc
#else
#error "Unsupported version of libelvin"
#endif

#ifdef DEBUG
#define DPRINTF(x) fprintf x
#else
#define DPRINTF(x)
#endif

typedef struct key_entry *key_entry_t;
struct key_entry
{
    /* The label the user gave to the key. */
    char *name;

    /* The raw key data (private keys only). */
    char *data;

    /* The length of the raw key data. */
    int data_length;

    /* The hashed key data. */
    char *hash;

    /* The length of the hashed key data. */
    int hash_length;

    /* Whether the key is private. */
    int is_private;
};

static void key_entry_free(key_entry_t self);

/* Allocates and initializes a new key_entry_t */
static key_entry_t
key_entry_alloc(
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

    if (is_private)
    {
        /* Take a copy of the data */
        if ((self -> data = malloc(length)) == NULL)
        {
            key_entry_free(self);
            return NULL;
        }
        memcpy(self -> data, data, length);
        self -> data_length = length;

        /* Compute the hash */
        if ((self -> hash = malloc(ELVIN_SHA1_DIGESTLEN)) == NULL) {
            key_entry_free(self);
            return NULL;
        }

        elvin_sha1_digest(NULL, data, length, self -> hash, NULL);
        self -> hash_length = ELVIN_SHA1_DIGESTLEN;
    }
    else
    {
        if ((self -> hash = malloc(length)) == NULL)
        {
            key_entry_free(self);
            return NULL;
        }
        memcpy(self -> hash, data, length);
        self -> hash_length = length;
    }

    /* Store the other values */
    self -> is_private = is_private;
    return self;
}

/* Releases the resources consumed by the receiver */
static void
key_entry_free(key_entry_t self)
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

    /* Free the hash data */
    if (self -> hash != NULL)
    {
        free(self -> hash);
    }

    /* Free the memory used by the key's context. */
    free(self);
}

static void
key_entry_add_to_keys(
    key_entry_t self,
    int is_for_notify,
    elvin_keys_t keys,
    elvin_error_t error)
{
    /* If this is a private key then add the raw data */
    if (self -> is_private)
    {
        if (! ELVIN_KEYS_ADD(
                keys,
                ELVIN_KEY_SCHEME_SHA1_DUAL,
                is_for_notify ?
                ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX :
                ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX,
                self -> data, self -> data_length, NULL,
                error))
        {
            fprintf(stderr, PACKAGE ": elvin_keys_add failed for key %s\n", self -> name);
            abort();
        }
    }

    /* Add the hashed key data */
    if (! ELVIN_KEYS_ADD(
            keys,
            ELVIN_KEY_SCHEME_SHA1_DUAL,
            is_for_notify ?
            ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX :
            ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX,
            self -> hash, self -> hash_length, NULL,
            error))
    {
        fprintf(stderr, PACKAGE ": elvin_keys_add failed for key %s\n", self -> name);
        abort();
    }

    /* Add it as a producer key too */
    if (! ELVIN_KEYS_ADD(
            keys,
            is_for_notify ?
            ELVIN_KEY_SCHEME_SHA1_CONSUMER :
            ELVIN_KEY_SCHEME_SHA1_PRODUCER,
            is_for_notify ?
            ELVIN_KEY_SHA1_CONSUMER_INDEX :
            ELVIN_KEY_SHA1_PRODUCER_INDEX,
            self -> hash, self -> hash_length, NULL,
            error))
    {
        fprintf(stderr, PACKAGE ": elvin_keys_add failed for key %s\n", self -> name);
        abort();
    }
}

static void
key_entry_promote(
    key_entry_t self,
    int is_for_notify,
    elvin_keys_t keys,
    elvin_error_t error)
{
    /* Sanity check */
    if (! self -> is_private)
    {
        fprintf(stderr, PACKAGE ": internal error\n");
        abort();
    }

    if (! ELVIN_KEYS_ADD(
            keys,
            ELVIN_KEY_SCHEME_SHA1_DUAL,
            is_for_notify ?
            ELVIN_KEY_SHA1_DUAL_PRODUCER_INDEX :
            ELVIN_KEY_SHA1_DUAL_CONSUMER_INDEX,
            self -> data, self -> data_length, NULL,
            error))
    {
        fprintf(stderr, PACKAGE ": elvin_keys_add failed for key %s\n", self -> name);
        abort();
    }
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
static key_entry_t *
key_table_search(key_table_t self, char *name)
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
int
key_table_lookup(
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
        if ((*entry) -> is_private)
        {
            *data_out = (*entry) -> data;
        }
        else
        {
            *data_out = (*entry) -> hash;
        }
    }

    /* Return the length. */
    if (length_out != NULL)
    {
        if ((*entry) -> is_private)
        {
            *length_out = (*entry) -> data_length;
        }
        else
        {
            *length_out = (*entry) -> hash_length;
        }
    }

    /* Return whether the key is private. */
    if (is_private_out != NULL)
    {
        *is_private_out = (*entry) -> is_private;
    }

    return 0;
}

/* Adds a new key to the table. */
int
key_table_add(
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
int
key_table_remove(key_table_t self, char *name)
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

/* Order two key entries by their hashed value.  NULL entries are
 * considered to be larger than non-NULL ones. */
static int
entry_compare(const void *val1, const void *val2)
{
    key_entry_t entry1 = *(key_entry_t *)val1;
    key_entry_t entry2 = *(key_entry_t *)val2;
    int result;

    /* Sort NULL keys to the end of the table */
    if (entry1 == NULL)
    {
        if (entry2 == NULL)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else if (entry2 == NULL)
    {
        return -1;
    }

    /* Work out which value comes first */
    if (entry1 -> hash_length < entry2 -> hash_length)
    {
        if ((result = memcmp(entry1 -> hash, entry2 -> hash, entry1 -> hash_length)) == 0)
        {
            return -1;
        }
        else
        {
            return result;
        }
    }
    else if (entry1 -> hash_length > entry2 -> hash_length)
    {
        if ((result = memcmp(entry1 -> hash, entry2 -> hash, entry2 -> hash_length)) == 0)
        {
            return 1;
        }
        else
        {
            return result;
        }
    }
    else
    {
        return memcmp(entry1 -> hash, entry2 -> hash, entry1 -> hash_length);
    }
}

static void
get_sorted_entries(
    key_table_t self,
    char **key_names,
    int key_count,
    int do_warn,
    key_entry_t **entries_out,
    int *count_out)
{
    key_entry_t *entries;
    key_entry_t *entry;
    key_entry_t last;
    int i;

    /* Make sure we don't try to malloc zero bytes */
    if (!key_count) {
        *entries_out = NULL;
        *count_out = 0;
        return;
    }

    /* Allocate memory for the entries */
    if ((entries = malloc(key_count * sizeof(key_entry_t))) == NULL) {
        fprintf(stderr, PACKAGE ": malloc failed: %s\n", strerror(errno));
        exit(1);
    }

    /* Populate it */
    for (i = 0; i < key_count; i++) {
        if ((entry = key_table_search(self, key_names[i])) == NULL) {
            if (do_warn) {
                fprintf(stderr, PACKAGE ": warning: unknown key: \"%s\"\n", key_names[i]);
            }
            entries[i] = NULL;
        } else {
            entries[i] = *entry;
        }
    }

    /* Sort the entries */
    qsort(entries, key_count, sizeof(key_entry_t), entry_compare);

    /* Strip out duplicates and missing keys */
    last = NULL;
    for (i = 0; i < key_count;) {
        /* NULLs are sorted to the end, so we're done */
        if (entries[i] == NULL) {
            break;
        }

        /* Look for duplicate values */
        if (last && last -> hash_length == entries[i] -> hash_length &&
            memcmp(last -> hash, entries[i] -> hash, entries[i] -> hash_length) == 0) {
            if (do_warn) {
                fprintf(stderr, PACKAGE ": warning: duplicate keys: \"%s\" and \"%s\"\n",
                        last -> name, entries[i] -> name);
            }

            /* Keep the private key over a public one */
            if (! last -> is_private && entries[i] -> is_private) {
                last = entries[i];
                entries[i - 1] = entries[i];
            }

            /* Stomp out the duplicate */
            memmove(entries + i, entries + i + 1, (key_count - i - 1) * sizeof(key_entry_t));
            entries[key_count - 1] = NULL;
        } else {
            last = entries[i];
            i++;
        }
    }

    *entries_out = entries;
    *count_out = i;
}

static void
ensure_keys(elvin_keys_t *keys_in_out)
{
    if (*keys_in_out == NULL) {
        *keys_in_out = ELVIN_KEYS_ALLOC(NULL, NULL);
    }
}

void
key_table_diff(
    key_table_t old_key_table,
    char **old_key_names,
    int old_key_count,
    key_table_t new_key_table,
    char **new_key_names,
    int new_key_count,
    int is_for_notify,
    elvin_keys_t *keys_to_add_out,
    elvin_keys_t *keys_to_remove_out)
{
    elvin_keys_t keys_to_add = NULL;
    elvin_keys_t keys_to_remove = NULL;
    key_entry_t *old_entries;
    key_entry_t *new_entries;
    int old_count, new_count;
    int old_index, new_index;
    int result;

    /* Look up the old keys and sort them */
    get_sorted_entries(
        old_key_table,
        old_key_names, 
        old_key_count,
        0,
        &old_entries,
        &old_count);

    /* Look up the new keys and sort them */
    get_sorted_entries(
        new_key_table,
        new_key_names,
        new_key_count,
        1,
        &new_entries,
        &new_count);

    /* Walk the two tables and find differences */
    old_index = 0;
    new_index = 0;
    while (old_index < old_count && new_index < new_count)
    {
        result = entry_compare(old_entries + old_index, new_entries + new_index);
        if (result < 0) 
        {
            DPRINTF((stdout, "removing key: \"%s\"\n", old_entries[old_index] -> name));
            ensure_keys(&keys_to_remove);
            key_entry_add_to_keys(
                old_entries[old_index++],
                is_for_notify,
                keys_to_remove, NULL);
        }
        else if (result > 0)
        {
            DPRINTF((stdout, "adding key: \"%s\"\n", new_entries[new_index] -> name));
            ensure_keys(&keys_to_add);
            key_entry_add_to_keys(
                new_entries[new_index++],
                is_for_notify,
                keys_to_add, NULL);
        }
        else
        {
            if (old_entries[old_index] -> is_private == new_entries[new_index] -> is_private)
            {
                DPRINTF((stdout, "keeping key: \"%s\" -> \"%s\"\n",
                         old_entries[old_index] -> name,
                         new_entries[new_index] -> name));
            }
            else if (old_entries[old_index] -> is_private)
            {
                DPRINTF((stdout, "demoting key: \"%s\" -> \"%s\"\n",
                         old_entries[old_index] -> name,
                         new_entries[new_index] -> name);)
                ensure_keys(&keys_to_remove);
                key_entry_promote(old_entries[old_index],
                                  is_for_notify,
                                  keys_to_remove, NULL);
            }
            else if (new_entries[new_index] -> is_private)
            {
                DPRINTF((stdout, "promoting key: \"%s\" -> \"%s\"\n",
                         old_entries[old_index] -> name,
                         new_entries[new_index] -> name));
                ensure_keys(&keys_to_add);
                key_entry_promote(new_entries[new_index],
                                  is_for_notify,
                                  keys_to_add, NULL);
            }

            old_index++;
            new_index++;
        }
    }

    while (old_index < old_count)
    {
        DPRINTF((stdout, "removing key: \"%s\"\n", old_entries[old_index] -> name));
        ensure_keys(&keys_to_remove);
        key_entry_add_to_keys(
            old_entries[old_index++],
            is_for_notify,
            keys_to_remove, NULL);
    }

    while (new_index < new_count)
    {
        DPRINTF((stdout, "adding key: \"%s\"\n", new_entries[new_index] -> name));
        ensure_keys(&keys_to_add);
        key_entry_add_to_keys(
            new_entries[new_index++],
            is_for_notify,
            keys_to_add, NULL);
    }

    DPRINTF((stdout, "---\n"));

    free(old_entries);
    free(new_entries);

    if (keys_to_add_out)
    {
        *keys_to_add_out = keys_to_add;
    }
    else if (keys_to_add)
    {
        elvin_keys_free(keys_to_add, NULL);
    }

    if (keys_to_remove_out)
    {
        *keys_to_remove_out = keys_to_remove;
    }
    else if (keys_to_remove)
    {
        elvin_keys_free(keys_to_remove, NULL);
    }
}
