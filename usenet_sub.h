/***************************************************************

  Copyright (C) 1999-2001, 2004 by Mantara Software
  (ABN 17 105 665 594). All Rights Reserved.

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

#ifndef USENET_SUB_H
#define USENET_SUB_H

#ifndef lint
static const char cvs_USENET_SUB_H[] = "$Id: usenet_sub.h,v 1.7 2004/08/02 22:24:17 phelps Exp $";
#endif /* lint */

/* The subscription data type */
typedef struct usenet_sub *usenet_sub_t;

#include "usenet_parser.h"
#include "message.h"

/* The format of the callback function */
typedef void (*usenet_sub_callback_t)(void *rock, message_t message, int show_attachment);

/* Allocates and initializes a new usenet_sub_t */
usenet_sub_t usenet_sub_alloc(usenet_sub_callback_t callback, void *rock);
    
/* Releases the resources consumed by the receiver */
void usenet_sub_free(usenet_sub_t self);

/* Adds a new entry to the usenet subscription */
int usenet_sub_add(
    usenet_sub_t self, int has_not, char *pattern,
    struct usenet_expr *expressions, size_t count);

/* Sets the receiver's elvin connection */
void usenet_sub_set_connection(usenet_sub_t self, elvin_handle_t handle, elvin_error_t error);

#endif /* USENET_SUB_H */
