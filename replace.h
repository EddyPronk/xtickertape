/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1997-2002.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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

#ifndef REPLACE_H
#define REPLACE_H

/* Replacements for standard functions */


#ifndef HAVE_MEMSET
/* A slow but correct implementation of memset */
void *memset(void *s, int c, size_t n);
#endif /* HAVE_MEMSET */

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif /* HAVE_STRCASECMP */

#ifndef HAVE_STRCHR
char *strchr(const char *s, int c);
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif
#endif /* REPLACE_H */
