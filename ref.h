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

#ifndef EXPLICIT_REF_H
#define EXPLICIT_REF_H

# ifdef DEBUG

/* Reference counting structures and helper functions.  There are
 * several types of struct which don't have a set lifecycle and so are
 * reference counted.  Unfortunately, reference counts thwart valgrind
 * and other memory checkers, so this structure makes it easier to
 * track what still references an object.
 *
 * Each record tracks the file and line where the reference was
 * aqcuired, along with a reference type and and a rock.  These last
 * two are used to identify which item to remove when a reference is
 * released.  Note that although the type is a string, only pointer
 * equality is performed: use static const char *s to ensure that only
 * one instance of the string constant is ever used.
 */
typedef struct explicit_ref *explicit_ref_t;
struct explicit_ref {
    /* The next reference in the linked list. */
    explicit_ref_t next;

    /* The name of the file where the reference was acquired. */
    const char *file;

    /* The line number of the file where the reference was acquried. */
    int line;

    /* The type of reference. */
    const char *type;

    /* A pointer, usually to the referencing object. */
    void *rock;
};

/* Add a new reference to a list of references. */
void
acquire_explicit_ref(explicit_ref_t *list, const char *obj_type, void *obj,
		     const char *file, int line, const char *type, void *rock);

/* Free a reference. */
void
release_explicit_ref(explicit_ref_t *list, const char *obj_type, void *obj,
		     const char *file, int line, const char *type, void *rock);

/* Count the number of references. */
int
explicit_ref_count(explicit_ref_t list);

# endif /* DEBUG */
#endif /* EXPLICIT_REF_H */
