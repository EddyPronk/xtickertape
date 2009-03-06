/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

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

#ifndef KEY_TABLE_H
#define KEY_TABLE_H

/* The key table data type */
typedef struct key_table *key_table_t;

/* Allocates and initializes a new key_table */
key_table_t
key_table_alloc();


/* Frees the resources consumed by the key_table */
void
key_table_free(key_table_t self);


/* Adds a new key to the table. */
int
key_table_add(key_table_t self,
              const char *name,
              const char *data,
              int length,
              int is_private);


/* Remove the key with the given name from the table. */
int
key_table_remove(key_table_t self, const char *name);


/* Returns the information about the named key. */
int
key_table_lookup(key_table_t self,
                 const char *name,
                 const char **data_out,
                 int *length_out,
                 int *is_private_out);

void
key_table_diff(key_table_t old_key_table,
               char **old_key_names,
               int old_key_count,
               key_table_t new_key_table,
               char **new_key_names,
               int new_key_count,
               int is_for_notify,
               elvin_keys_t *keys_to_add_out,
               elvin_keys_t *keys_to_remove_out);


#endif /* KEY_TABLE_H */
