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

****************************************************************/

#ifndef SUBSCRIPTION_H
#define SUBSCRIPTION_H

#ifndef lint
static const char cvs_SUBSCRIPTION_H[] = "$Id: subscription.h,v 2.4 2000/07/11 04:27:30 phelps Exp $";
#endif /* lint */

#include <message.h>

/* Allocates and initializes a new subscription */
subscription_t subscription_alloc(char *tag, env_t env, elvin_error_t error);

/* Frees the resources consumed by the subscription */
int subscription_free(subscription_t self, elvin_error_t error);

/* Transforms a notification into a message */
message_t subscription_transmute(
    subscription_t self,
    elvin_notification_t notification,
    elvin_error_t error);

#endif /* SUBSCRIPTION_H */