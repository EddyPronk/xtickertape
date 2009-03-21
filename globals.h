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

#ifndef GLOBALS_H
#define GLOBALS_H

#include <X11/X.h>

#if defined(ELVIN_VERSION_AT_LEAST)
# if ELVIN_VERSION_AT_LEAST(4, 1, -1)
extern elvin_client_t client;
# else
#  error "Unsupported Elvin library version"
# endif
#endif

typedef enum {
    AN_CHARSET_ENCODING,
    AN_CHARSET_REGISTRY,
    AN_COMPOUND_TEXT,
    AN_TARGETS,
    AN_TEXT,
    AN_UTF8_STRING,
    AN__MOTIF_CLIPBOARD_TARGETS,
    AN__MOTIF_DEFERRED_CLIPBOARD_TARGETS,
    AN__MOTIF_EXPORT_TARGETS,
    AN__MOTIF_LOSE_SELECTION,
    AN__MOTIF_SNAPSHOT,
    AN_MAX = AN__MOTIF_SNAPSHOT
} atom_index_t;

extern Atom atoms[AN_MAX + 1];

#endif /* GLOBALS_H */
