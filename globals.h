/***************************************************************

  Copyright (C) 2002-2004 by Mantara Software (ABN 17 105 665 594).
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

#ifndef GLOBALS_H
#define GLOBALS_H

#ifndef lint
static const char cvs_GLOBALS_H[] = "$Id: globals.h,v 1.4 2004/08/02 22:24:16 phelps Exp $";
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
