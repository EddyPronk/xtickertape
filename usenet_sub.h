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

#ifndef USENET_SUB_H
#define USENET_SUB_H

/* The subscription data type */
typedef struct usenet_sub *usenet_sub_t;

#include "usenet_parser.h"
#include "message.h"

/* The format of the callback function */
typedef void (*usenet_sub_callback_t)(void *rock, message_t message,
                                      int show_attachment);

/* Allocates and initializes a new usenet_sub_t */
usenet_sub_t
usenet_sub_alloc(usenet_sub_callback_t callback, void *rock);


/* Releases the resources consumed by the receiver */
void
usenet_sub_free(usenet_sub_t self);


/* Adds a new entry to the usenet subscription */
int
usenet_sub_add(usenet_sub_t self,
               int has_not,
               const char *pattern,
               struct usenet_expr *expressions,
               size_t count);


/* Sets the receiver's elvin connection */
void
usenet_sub_set_connection(usenet_sub_t self,
                          elvin_handle_t handle,
                          elvin_error_t error);


#endif /* USENET_SUB_H */
