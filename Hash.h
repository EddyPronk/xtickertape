/*
 * $Id: Hash.h,v 1.1 1997/02/05 06:24:13 phelps Exp $
 * 
 * Generic support for hash tables (keys are strings)
 */

#ifndef HASH_H
#define HASH_H

/* The Hashtable pointer type */
typedef struct Hashtable_t *Hashtable;


/* Allocates a new Hashtable */
Hashtable Hashtable_alloc(int size);

/* Releases a Hashtable [Doesn't release table contents] */
void Hashtable_free(Hashtable self);



/* Gets an entry from the Hashtable */
void *Hashtable_get(Hashtable self, char *key);

/* Puts an entry in the Hashtable, returning the old value */
void *Hashtable_put(Hashtable self, char *key, void *value);

/* Removes an entry from the Hashtable */
void *Hashtable_remove(Hashtable self, char *key);


/* Enumeration */
void Hashtable_do(Hashtable self, void (*function)());

/* Enumeration with context */
void Hashtable_doWith(Hashtable self, void (*function)(), void *context);


/* Print debugging information to stdout */
void Hashtable_debug(Hashtable self);

/* Print debugging information to out */
void Hashtable_fdebug(Hashtable self, FILE *out);


#endif /* HASH_H */
