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
             Transforms NewsWatcher notifications into tickertape
	     messages suitable for scrolling

****************************************************************/

#ifndef USENET_SUBSCRIPTION_H
#define USENET_SUBSCRIPTION_H

#ifndef lint
static const char cvs_USENET_SUBSCRIPTION_H[] = "$Id: UsenetSubscription.h,v 1.4 1999/05/22 08:35:15 phelps Exp $";
#endif /* lint */

/* The UsenetSubscription data type */
typedef struct UsenetSubscription_t *UsenetSubscription;

#include <stdio.h>
#include "Message.h"
#include "ElvinConnection.h"

typedef void (*UsenetSubscriptionCallback)(void *context, Message message);

/* Write a default `usenet' file onto the output stream */
int UsenetSubscription_writeDefaultUsenetFile(FILE *out);

/* Read the UsenetSubscription from the given file */
UsenetSubscription UsenetSubscription_readFromUsenetFile(
    FILE *usenet, UsenetSubscriptionCallback callback, void *context);

/* Answers a new UsenetSubscription */
UsenetSubscription UsenetSubscription_alloc(
    char *expression,
    UsenetSubscriptionCallback callback,
    void *context);

/* Releases the resources used by an UsenetSubscription */
void UsenetSubscription_free(UsenetSubscription self);

/* Prints debugging information */
void UsenetSubscription_debug(UsenetSubscription self);

/* Sets the receiver's ElvinConnection */
void UsenetSubscription_setConnection(UsenetSubscription self, ElvinConnection connection);

#endif /* USENET_SUBSCRIPTION_H */
