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

#ifndef KEY_TABLE_H
#define KEY_TABLE_H

#ifndef lint
static const char cvs_KEY_TABLE_H[] = "$Id: key_table.h,v 1.2 2004/08/02 15:15:57 phelps Exp $";
#endif /* lint */

/* The key table data type */
typedef struct key_table *key_table_t;

/* Allocates and initializes a new key_table */
key_table_t key_table_alloc();

/* Frees the resources consumed by the key_table */
void key_table_free(key_table_t self);

/* Adds a new key to the table. */
int key_table_add(
    key_table_t self,
    char *name,
    char *data,
    int length,
    int is_private);

/* Remove the key with the given name from the table. */
int key_table_remove(key_table_t self, char *name);

/* Returns the information about the named key. */
int key_table_lookup(
    key_table_t self,
    char *name,
    char **data_out,
    int *length_out,
    int *is_private_out);

void key_table_diff(
    key_table_t old_key_table,
    char **old_key_names,
    int old_key_count,
    key_table_t new_key_table,
    char **new_key_names,
    int new_key_count,
    int is_for_notify,
    elvin_keys_t *keys_to_add_out,
    elvin_keys_t *keys_to_remove_out);

#endif /* KEY_TABLE_H */
