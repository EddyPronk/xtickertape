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

#ifndef CONNECT_H
#define CONNECT_H

#ifndef lint
static const char cvs_CONNECT_H[] = "$Id: connect.h,v 1.1 1999/09/26 14:00:18 phelps Exp $";
#endif /* lint */

typedef struct connection *connection_t;

#include <X11/Intrinsic.h>
#include <elvin3/elvin.h>

/* Callback types */
typedef void (*notify_callback_t)(void *rock, en_notify_t notification);
typedef void (*disconnect_callback_t)(void *rock, connection_t self);
typedef void (*reconnect_callback_t)(void *rock, connection_t self);

/* Answers a new connection_t */
connection_t connection_alloc(
    char *hostname, int port, XtAppContext app_context,
    disconnect_callback_t disconnectCallback, void *disconnect_rock,
    reconnect_callback_t callback, void *reconnect_rock);

/* Releases the resources used by the connection_t */
void connection_free(connection_t self);

/* Answers the receiver's host */
char *connection_host(connection_t self);

/* Answers the receiver's port */
int connection_port(connection_t self);

/* Registers a callback for when the given expression is matched */
void *connection_subscribe(
    connection_t self, char *expression,
    notify_callback_t callback, void *rock);

/* Unregisters a callback (info was returned by connection_subscribe) */
void connection_unsubscribe(connection_t self, void *info);

/* Sends a message by posting an Elvin event */
void connection_publish(connection_t self, en_notify_t notification);

#endif /* CONNECT_H */
