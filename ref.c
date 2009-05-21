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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h>
#ifdef HAVE_STDLIB_H
# include <stdlib.h>
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h>
#endif

#include "globals.h"
#include "ref.h"
#include "utils.h"

#if defined(DEBUG)

void
acquire_explicit_ref(explicit_ref_t *list, const char *obj_type, void *obj,
		     const char *file, int line, const char *type, void *rock)
{
    explicit_ref_t self;

    /* Allocate memory to hold the reference. */
    self = malloc(sizeof(struct explicit_ref));
    ASSERT(self != NULL);

    /* Record the reference information. */
    self->file = file;
    self->line = line;
    self->type = type;
    self->rock = rock;

    /* Push it onto the head of the list. */
    self->next = *list;
    *list = self;

    DPRINTF((1, "%s:%d: acquiried %s reference to %s %p with rock=%p\n",
	     file, line, type, obj_type, obj, rock));
}
void
release_explicit_ref(explicit_ref_t *list, const char *obj_type, void *obj,
		     const char *file, int line, const char *type, void *rock)
{
    explicit_ref_t prev, self;

    /* Traverse the linked list looking for a match. */
    prev = NULL;
    for (self = *list; self != NULL; self = self->next) {
	if (self->type == type && self->rock == rock) {
	    break;
	}
        prev = self;
    }

    /* Make sure it was found. */
    ASSERT(self != NULL);

    /* Unlink it. */
    if (prev == NULL) {
	ASSERT(*list == self);
	*list = self->next;
    } else {
	ASSERT(*list != self);
	prev->next = self->next;
    }

    DPRINTF((1, "%s:%d: freeing %s reference to %s %p acquired "
	     "at %s:%d with rock=%p\n", file, line, type, obj_type, obj,
	     self->file, self->line, self->rock));
    free(self);
}

#endif /* DEBUG */

/**********************************************************************/
