/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1999.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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

#ifndef ORBIT_SUB_H
#define ORBIT_SUB_H

#ifndef lint
static const char cvs_ORBIT_SUB_H[] = "$Id: orbit_sub.h,v 1.4 2001/08/25 14:04:44 phelps Exp $";
#endif /* lint */

/* The orbit_sub data type */
typedef struct orbit_sub *orbit_sub_t;

#include "message.h"
#include "connect.h"
#include "panel.h"

/* The orbit_sub callbacktype */
typedef void (*orbit_sub_callback_t)(void *rock, message_t message);



/* Allocates and initializes a new orbit_sub_t */
orbit_sub_t orbit_sub_alloc(char *title, char *id, orbit_sub_callback_t callback, void *rock);

/* Releases resources used by the receiver */
void orbit_sub_free(orbit_sub_t self);

/* Answers the receiver's zone id */
char *orbit_sub_get_id(orbit_sub_t self);

/* Sets the receiver's title */
void orbit_sub_set_title(orbit_sub_t self, char *title);

/* Answers the receiver's title */
char *orbit_sub_get_title(orbit_sub_t self);

/* Sets the receiver's connection */
void orbit_sub_set_connection(orbit_sub_t self, connection_t connection);

/* Sets the receiver's control panel */
void orbit_sub_set_control_panel(orbit_sub_t self, control_panel_t control_panel);

#endif /* ORBIT_SUBSCRIPTION_H */
