/***********************************************************************

  Copyright (C) 2002-2004 by Mantara Software
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
/*
 * Description: 
 *   Parses the Tickertape keys file
 */

#ifndef KEYS_PARSER_H
#define KEYS_PARSER_H

#ifndef lint
static const char cvs_KEYS_PARSER_H[] = "$Id: keys_parser.h,v 1.4 2004/08/03 12:29:16 phelps Exp $";
#endif /* lint */

#include <elvin/elvin.h>

/* The keys parser data type */
typedef struct keys_parser *keys_parser_t;

/* The keys_parser callback type */
typedef int (*keys_parser_callback_t)(
    void *rock,
    char *name,
    char *key_data,
    int key_length,
    int is_private);

/* Allocates and initializes a new keys file parser */
keys_parser_t keys_parser_alloc(
    char *tickerdir,
    keys_parser_callback_t callback,
    void *rock,
    char *tag);

/* Frees the resources consumed by the receiver */
void keys_parser_free(keys_parser_t self);

/* Parses the given buffer, calling callbacks for each subscription
 * expression that is successfully read.  A zero-length buffer is
 * interpreted as an end-of-input marker */
int keys_parser_parse(keys_parser_t self, char *buffer, size_t length);

#endif /* KEYS_PARSER_H */
