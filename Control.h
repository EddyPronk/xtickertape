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

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#ifndef lint
static const char cvs_CONTROLPANEL_H[] = "$Id: Control.h,v 1.14 1999/08/19 07:29:29 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* The ControlPanel datatype */
typedef struct ControlPanel_t *ControlPanel;

#include "Message.h"
#include "tickertape.h"

/* The ControlPanelCallback type */
typedef void (*ControlPanelCallback)(void *context, Message message);

/* The ReloadCallback type */
typedef void (*ReloadCallback)(void *context);

/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(tickertape_t tickertape, Widget parent);

/* Releases the resources used by the receiver */
void ControlPanel_free(ControlPanel self);

/* Adds a subscription to the receiver */
void *ControlPanel_addSubscription(
    ControlPanel self, char *title,
    ControlPanelCallback callback, void *context);

/* Removes a subscription from the receiver (info was returned by addSubscription) */
void ControlPanel_removeSubscription(ControlPanel self, void *info);

/* Changes the location of the subscription within the ControlPanel */
void ControlPanel_setSubscriptionIndex(ControlPanel self, void *info, int index);

/* Retitles an entry */
void ControlPanel_retitleSubscription(ControlPanel self, void *info, char *title);

/* Makes the ControlPanel window visible */
void ControlPanel_select(ControlPanel self, Message message);

/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self);

/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget);

#endif /* CONTROLPANEL_H */
