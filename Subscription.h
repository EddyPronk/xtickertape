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

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#ifndef lint
static const char cvs_SUBSCRIPTION_H[] = "$Id: Subscription.h,v 1.11 1999/09/09 14:29:49 phelps Exp $";
#endif /* lint */

/* The subscription data type */
typedef struct Subscription_t *Subscription;

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include "List.h"
#include "message.h"
#include "ElvinConnection.h"
#include "Control.h"

/* The format for the callback function */
typedef void (*SubscriptionCallback)(void *context, message_t message);



/* Answer a List containing the subscriptions in the given file or NULL if unable */
List Subscription_readFromGroupFile(
    FILE *in,
    SubscriptionCallback callback, void *context);

/* Creates a default groups file for the named user */
int Subscription_writeDefaultGroupsFile(
    FILE *out, char *username);

/* Answers a new Subscription */
Subscription Subscription_alloc(
    char *group,
    char *expression,
    int inMenu,
    int autoMime,
    int minTime,
    int maxTime,
    SubscriptionCallback callback,
    void *context);

/* Releases resources used by a Subscription */
void Subscription_free(Subscription self);

/* Prints debugging information */
void Subscription_debug(Subscription self);

/* Answers the receiver's subscription expression */
char *Subscription_expression(Subscription self);

/* Updates the receiver to look just like subscription in terms of
 * group, inMenu, autoMime, minTime, maxTime, callback and context,
 * but NOT expression */
void Subscription_updateFromSubscription(Subscription self, Subscription subscription);

/* Sets the receiver's ElvinConnection */
void Subscription_setConnection(Subscription self, ElvinConnection connection);

/* Registers the receiver with the ControlPanel */
void Subscription_setControlPanel(Subscription self, ControlPanel controlPanel);

/* Makes the receiver visible in the ControlPanel's group menu iff
 * inMenu is set, and makes sure it appears at the proper index */
void Subscription_updateControlPanelIndex(
    Subscription self,
    ControlPanel controlPanel,
    int *index);

#endif /* SUBSCRIPTION_H */
