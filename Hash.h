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

   Description: 
             Generic support for hash tables with strings for keys

****************************************************************/

#ifndef HASH_H
#define HASH_H

#ifndef lint
static const char cvs_HASH_H[] = "$Id: Hash.h,v 1.2 1998/12/24 05:48:28 phelps Exp $";
#endif /* lint */

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
