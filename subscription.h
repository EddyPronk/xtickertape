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
static const char cvs_SUBSCRIPTION_H[] = "$Id: subscription.h,v 2.1 2000/07/07 10:42:07 phelps Exp $";
#endif /* lint */

/* The subscription data type */
typedef struct subscription *subscription_t;

/* Allocates and initializes a new subscription */
subscription_t subscription_alloc(
    char *tag,
    char *name,
    uchar *subscription,
    int in_menu,
    int auto_mime,
    elvin_keys_t producer_keys,
    elvin_keys_t consumer_keys,
    value_t group,
    value_t user,
    value_t message,
    value_t timeout,
    value_t mime_type,
    value_t mime_args,
    value_t message_id,
    value_t reply_id,
    elvin_error_t error);

/* Frees the resources consumed by the subscription */
int subscription_free(subscription_t self, elvin_error_t error);

#endif /* SUBSCRIPTION_H */
