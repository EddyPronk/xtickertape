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

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

#ifndef lint
static const char cvs_ELVINCONNECTION_H[] = "$Id: ElvinConnection.h,v 1.13 1998/12/24 05:48:27 phelps Exp $";
#endif /* lint */

typedef struct ElvinConnection_t *ElvinConnection;

#include <elvin3/elvin.h>
#include <elvin3/element.h>
#include <X11/Intrinsic.h>

/* Callback types */
typedef void (*NotifyCallback)(void *context, en_notify_t notification);
typedef void (*DisconnectCallback)(void *context, ElvinConnection self);
typedef void (*ReconnectCallback)(void *context, ElvinConnection self);

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(
    char *hostname, int port, XtAppContext app_context,
    DisconnectCallback disconnectCallback, void *disconnectContext,
    ReconnectCallback callback, void *reconnectContext);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

/* Answers the receiver's host */
char *ElvinConnection_host(ElvinConnection self);

/* Answers the receiver's port */
int ElvinConnection_port(ElvinConnection self);

/* Registers a callback for when the given expression is matched */
void *ElvinConnection_subscribe(
    ElvinConnection self, char *expression,
    NotifyCallback callback, void *context);

/* Unregisters a callback (info was returned by ElvinConnection_subscribe) */
void ElvinConnection_unsubscribe(ElvinConnection self, void *info);

/* Sends a message by posting an Elvin event */
void ElvinConnection_send(ElvinConnection self, en_notify_t notification);

#endif /* ELVINCONNECTION_H */
