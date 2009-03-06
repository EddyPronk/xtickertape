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

#ifndef MBOX_PARSER_H
#define MBOX_PARSER_H

#include <stdio.h>

typedef struct mbox_parser *mbox_parser_t;

/*
 * Allocates a new RFC-822 mailbox parser.  This takes a strings as
 * input, which it parses as a mailbox (commented e-mail address) into
 * e-mail address and user name portions
 *
 * return values:
 *     success: a valid mbox_parser_t
 *     failure: NULL
 */
mbox_parser_t
mbox_parser_alloc();


/* Frees the resources consumed by the receiver */
void
mbox_parser_free(mbox_parser_t self);


/* Prints out debugging information about the receiver */
void
mbox_parser_debug(mbox_parser_t self, FILE *out);


/* Parses `mailbox' as an RFC 822 mailbox, separating out the name and
 * e-mail address portions which can subsequently be accessed with the
 * mbox_parser_get_name() and mbox_parser_get_email() functions.
 *
 * return values:
 *     success: 0
 *     failure: -1
 */
int
mbox_parser_parse(mbox_parser_t self, const char *mailbox);


/*
 * Answers a pointer to the e-mail address (only valid after
 * a successful call to mbox_parser_parse).
 */
const char *
mbox_parser_get_email(mbox_parser_t self);


/*
 * Answers a pointer to the name (only valid after a successful call
 * to mbox_parser_parse).  This will return NULL if no name was present in
 * the most recently parsed mailbox.
 */
const char *
mbox_parser_get_name(mbox_parser_t self);


#endif /* MBOX_PARSER_H */
