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
             Transforms elvinmail notifications into tickertape
	     messages suitable for scrolling

****************************************************************/

#ifndef MAIL_SUBSCRIPTION_H
#define MAIL_SUBSCRIPTION_H

#ifndef lint
static const char cvs_MAIL_SUBSCRIPTION_H[] = "$Id: MailSubscription.h,v 1.3 1999/09/09 14:29:46 phelps Exp $";
#endif /* lint */

/* The MailSubscription data type */
typedef struct MailSubscription_t *MailSubscription;

#include "message.h"
#include "ElvinConnection.h"

/* The MailSubscription callback type */
typedef void (*MailSubscriptionCallback)(void *context, message_t message);


/* Exported functions */

/* Answers a new MailSubscription */
MailSubscription MailSubscription_alloc(
    char *user,
    MailSubscriptionCallback callback, void *context);

/* Releases the resources consumed by the receiver */
void MailSubscription_free(MailSubscription self);

/* Prints debugging information about the receiver */
void MailSubscription_debug(MailSubscription self);

/* Sets the receiver's ElvinConnection */
void MailSubscription_setConnection(MailSubscription self, ElvinConnection connection);

#endif /* MAIL_SUBSCRIPTION_H */
