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
             Manages tickertape group subscriptions and can parse the
	     groups file

****************************************************************/

#ifndef USENET_SUB_H
#define USENET_SUB_H

#ifndef lint
static const char cvs_USENET_SUB_H[] = "$Id: usenet_sub.h,v 1.4 1999/12/16 07:32:44 phelps Exp $";
#endif /* lint */

/* The subscription data type */
typedef struct usenet_sub *usenet_sub_t;

#include "usenet_parser.h"
#include "message.h"

/* The format of the callback function */
typedef void (*usenet_sub_callback_t)(void *rock, message_t message);

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
