/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
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

   Description: 
             Macros for runtime typechecking

****************************************************************/

#ifndef SANITY_H
#define SANITY_H

#ifndef lint
static const char cvs_SANITY_H[] = "$Id: sanity.h,v 1.3 1998/12/24 05:48:31 phelps Exp $";
#endif /* lint */

/* Sanity checking */
#ifdef SANITY

#define SANITY_CHECK(sanity_state) \
{ \
    if (sanity_state -> sanity_check != sanity_value) \
    { \
	fprintf(stderr, "*** %s failed sanity check at %s:%d!\n", sanity_value, __FILE__, __LINE__); \
	fprintf(stderr, "     (It thinks it's a %s)\n", sanity_state -> sanity_check); \
	exit(1); \
    } \
}

#else /* SANITY */

#define SANITY_CHECK(sanity_state)

#endif /* SANITY */

#endif /* SANITY_H */
