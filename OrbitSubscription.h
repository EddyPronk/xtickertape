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
             Manages Orbit-specific GroupZone-related subscriptions

****************************************************************/

#ifndef ORBIT_SUBSCRIPTION_H
#define ORBIT_SUBSCRIPTION_H

#ifndef lint
static const char cvs_ORBIT_SUBSCRIPTION_H[] = "$Id: OrbitSubscription.h,v 1.4 1999/09/09 14:29:48 phelps Exp $";
#endif /* lint */

/* The OrbitSubscription data type */
typedef struct OrbitSubscription_t *OrbitSubscription;

#include "message.h"
#include "ElvinConnection.h"
#include "Control.h"

typedef void (*OrbitSubscriptionCallback)(void *context, message_t message);



/* Answers a new OrbitSubscription */
OrbitSubscription OrbitSubscription_alloc(
    char *title, char *id,
    OrbitSubscriptionCallback callback, void *context);

/* Releases resources used by an OrbitSubscription */
void OrbitSubscription_free(OrbitSubscription self);

/* Prints debugging information */
void OrbitSubscription_debug(OrbitSubscription self);

/* Sets the receiver's title */
void OrbitSubscription_setTitle(OrbitSubscription self, char *title);

/* Sets the receiver's title */
char *OrbitSubscription_getTitle(OrbitSubscription self);

/* Sets the receiver's ElvinConnection */
void OrbitSubscription_setConnection(OrbitSubscription self, ElvinConnection connection);

/* Sets the receiver's ControlPanel */
void OrbitSubscription_setControlPanel(OrbitSubscription self, ControlPanel controlPanel);

#endif /* ORBIT_SUBSCRIPTION_H */
