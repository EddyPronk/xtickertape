/***************************************************************

  Copyright (C) 2002, 2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

  This software is the property of the Mantara Software. All use
  of this software is permitted only under the terms of a
  license agreement with Mantara Software. If you do not have
  such a license, then you have no rights to use this software
  in any manner and should contact Mantara at the address below
  to determine an appropriate licensing arrangement.
  
     Mantara Software
     PO Box 1820
     Toowong QLD 4066
     Australia
     Tel: +61 7 3876 8844
     Fax: +61 7 3876 8843
     Email: licensing@mantara.com
  
     Web: http://www.mantara.com
  
  This software is being provided "AS IS" without warranty of
  any kind. In no event shall Mantara Software be liable for
  damage of any kind arising out of or in connection with the
  use or performance of this software.

****************************************************************/

#ifndef REPLACE_H
#define REPLACE_H

/* Replacements for standard functions */


#ifndef HAVE_MEMSET
/* A slow but correct implementation of memset */
void *memset(void *s, int c, size_t n);
#endif /* HAVE_MEMSET */

#ifndef HAVE_SNPRINTF
/* A dodgy hack for platforms without snprintf() */
int snprintf(char *s, size_t n, const char *format, ...);
#endif

#ifndef HAVE_STRCASECMP
int strcasecmp(const char *s1, const char *s2);
#endif /* HAVE_STRCASECMP */

#ifndef HAVE_STRCHR
char *strchr(const char *s, int c);
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif

#ifndef HAVE_STRRCHR
char *strrchr(const char *s, int c);
#endif

#ifndef HAVE_STRERROR
char *strerror(int errnum);
#endif
#endif /* REPLACE_H */
