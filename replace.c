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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STDLIB_H
#include <stdlib.h> /* NULL */
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h> /* toupper */
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif
#include "replace.h"

/* Replacements for standard functions */


#ifndef HAVE_MEMSET
/* A slow but correct implementation of memset */
void *memset(void *s, int c, size_t n)
{
    char *point = (char *)s;
    char *end = point + n;

    while (point < end)
    {
	*(point++) = c;
    }

    return s;
}
#endif

#ifndef HAVE_STRCASECMP
/* A slow but correct implementation of strcasecmp */
int strcasecmp(const char *s1, const char *s2)
{
    size_t i = 0;
    int c1, c2;

    while (1)
    {
	c1 = toupper(s1[i]);
	c2 = toupper(s2[i]);

	if (c1 == 0)
	{
	    if (c2 == 0)
	    {
		return 0;
	    }
	    else
	    {
		return -1;
	    }
	}

	if (c2 == 0)
	{
	    return 1;
	}
	
	if (c1 < c2)
	{
	    return -1;
	}

	if (c1 > c2)
	{
	    return 1;
	}

	i++;
    }
}
#endif


#ifndef HAVE_STRCHR
/* A slow but correct implementation of strchr */
char *strchr(const char *s, int c)
{
    size_t i = 0;
    int ch;

    while ((ch = s[i]) != 0)
    {
	if (ch == c)
	{
	    return s + i;
	}

	i++;
    }

    return NULL;
}
#endif

#ifndef HAVE_STRRCHR
/* A slow but correct implementation of strrchr */
char *strrchr(const char *s, int c)
{
    char *result = NULL;
    int ch;

    while ((ch = *s) != 0)
    {
	if (ch == c)
	{
	    result = (char *)s;
	}

	s++;
    }

    return result;
}
#endif


#ifndef HAVE_STRDUP
/* A slow but correct implementation of strdup */
char *strdup(const char *s)
{
    size_t length;
    char *result;

    length = strlen(s) + 1;
    if ((result = (char *)malloc(length)) == NULL)
    {
	return NULL;
    }

    memcpy(result, s, length);
    return result;
}
#endif
