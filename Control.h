/* $Id: Control.h,v 1.7 1998/10/21 04:03:46 arnold Exp $ */

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

/* The ControlPanel datatype */
typedef struct ControlPanel_t *ControlPanel;

#include "Message.h"

/* The ControlPanelCallback type */
typedef void (*ControlPanelCallback)(void *context, Message message);


/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(Widget parent, char *user);

/* Releases the resources used by the receiver */
void ControlPanel_free(ControlPanel self);

/* Adds a subscription to the receiver */
void *ControlPanel_addSubscription(
    ControlPanel self, char *title,
    ControlPanelCallback callback, void *context);

/* Removes a subscription from the receiver (info was returned by addSubscription) */
void ControlPanel_removeSubscription(ControlPanel self, void *info);

/* Retitles an entry */
void ControlPanel_retitleSubscription(ControlPanel self, void *info, char *title);


/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self, Message message);

/* Answers the receiver's values as a Message */
Message ControlPanel_createMessage(ControlPanel self);

/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget);

#endif /* CONTROLPANEL_H */
