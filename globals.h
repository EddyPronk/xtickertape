/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999-2000.
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

   Description: 
             Represents a Scroller and control panel and manages the
	     connection between them, the notification service and the 
	     user

****************************************************************/

#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef lint
static const char cvs_GLOBALS_H[] = "$Id: globals.h,v 1.3 2003/01/14 16:56:55 phelps Exp $";
#endif /* lint */

#if defined(ELVIN_VERSION_AT_LEAST)
#if ELVIN_VERSION_AT_LEAST(4, 1, -1)
extern elvin_client_t client;
#else
#error "Unsupported Elvin library version"
#endif
#endif

/* Useful macros */
#if !defined(MIN)
# define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#if !defined(MAX)
# define MAX(x, y) ((x) > (y) ? (x) : (y))
#endif

#endif /* GLOBALS_H */
