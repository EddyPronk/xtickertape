/* $Id: Hash.c,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Hash.h"

/* The hashtable associations */
typedef struct HashEntry_t *HashEntry;


struct Hashtable_t
{
    unsigned long size;
    unsigned long count;
    HashEntry *table;
};

struct HashEntry_t
{
    HashEntry next;
    char *key;
    void *value;
};


/* Allocates a new Hashtable */
Hashtable Hashtable_alloc(int size)
{
    Hashtable hash;

    hash = (Hashtable) malloc(sizeof(struct Hashtable_t));
    hash -> size = size;
    hash -> count = 0;
    hash -> table = calloc(size, sizeof(HashEntry));
    return hash;
}


/* Releases a Hashtable [Doesn't release table contents] */
void Hashtable_free(Hashtable self)
{
    /* FIX THIS: need to release the HashEntry's as well */
    free(self -> table);
    free(self);
}


/* Allocates a new HashEntry */
HashEntry HashEntry_alloc(char *key, void *value)
{
    HashEntry entry;

    entry = (HashEntry) malloc(sizeof(struct HashEntry_t));
    entry -> next = NULL;
    entry -> key = strdup(key);
    entry -> value = value;
    return entry;
}

/* Releases a HashEntry */
void HashEntry_free(HashEntry self)
{
    free(self -> key);
    free(self);
}


/* Compute the hash value of a string */
unsigned long Hashtable_hash(Hashtable self, char *key)
{
    unsigned long hash = 0;
    char *pointer = key;

    while (*pointer != '\0')
    {
	hash = hash * 3 + *pointer++;
    }

    return hash % self -> size;
}


/* Gets an entry from the Hashtable */
void *Hashtable_get(Hashtable self, char *key)
{
    HashEntry entry = self -> table[Hashtable_hash(self, key)];
    while (entry != NULL)
    {
	if (strcmp(key, entry -> key) == 0)
	{
	    return entry -> value;
	}

	entry = entry -> next;
    }

    /* not found */
    return NULL;
}

/* Puts an entry in the Hashtable, returning the old value */
void *Hashtable_put(Hashtable self, char *key, void *value)
{
    unsigned long index = Hashtable_hash(self, key);
    HashEntry entry = self -> table[index];
    HashEntry previous = NULL;

    while (entry != NULL)
    {
	if (strcmp(key, entry -> key) == 0)
	{
	    void *oldValue = entry -> value;
	    entry -> value = value;
	    return oldValue;
	}

	previous = entry;
	entry = entry -> next;
    }

    /* Not found - create a new entry */
    entry = HashEntry_alloc(key, value);
    if (previous == NULL)
    {
	self -> table[index] = entry;
    }
    else
    {
	previous -> next = entry;
    }
    self -> count++;

    return NULL;
}


/* Removes an entry from the Hashtable */
void *Hashtable_remove(Hashtable self, char *key)
{
    unsigned long index = Hashtable_hash(self, key);
    HashEntry entry = self -> table[index];
    HashEntry previous = NULL;

    while (entry != NULL)
    {
	if (strcmp(key, entry -> key) == 0)
	{
	    void *value = entry -> value;
	    if (previous == NULL)
	    {
		self -> table[index] = entry -> next;
	    }
	    else
	    {
		previous -> next = entry -> next;
	    }

	    HashEntry_free(entry);
	    self -> count--;
	    return value;
	}

	previous = entry;
	entry = entry -> next;
    }

    /* not found */
    return NULL;
}

/* Enumeration */
void Hashtable_do(Hashtable self, void (*function)(void *value))
{
    unsigned long index;
    HashEntry entry, next;

    for (index = 0; index < self -> size; index++)
    {
	entry = self -> table[index];
	while (entry != NULL)
	{
	    next = entry -> next;
	    (*function)(entry -> value);
	    entry = next;
	}
    }
}


/* Enumeration with context */
void Hashtable_doWith(Hashtable self, void (*function)(void *value, void *context), void *context)
{
    unsigned long index;
    HashEntry entry, next;

    for (index = 0; index < self -> size; index++)
    {
	entry = self -> table[index];

	while (entry != NULL)
	{
	    /* remember next in case entry gets removed by (*function) */
	    next = entry -> next;
	    (*function)(entry -> value, context);
	    entry = next;
	}
    }
}


/* Print debugging information to stdout */
void Hashtable_debug(Hashtable self)
{
    Hashtable_fdebug(self, stdout);
}


/* Print debugging information to out */
void Hashtable_fdebug(Hashtable self, FILE *out)
{
    unsigned long index;
    HashEntry entry;

    for (index = 0; index < self -> size; index++)
    {
	entry = self -> table[index];

	while (entry != NULL)
	{
	    fprintf(out, "[%ld] 0x%p key=\"%s\"\n", index, entry -> value, entry -> key);
	    entry = entry -> next;
	}
    }
    fflush(out);
}
