/* $Id: Control.h,v 1.4 1998/08/26 06:00:40 phelps Exp $ */

#ifndef CONTROLPANEL_H
#define CONTROLPANEL_H

#include <stdio.h>
#include <stdlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include "Message.h"
#include "List.h"

typedef void (*ControlPanelCallback)(Message message, void *context);
typedef struct ControlPanel_t *ControlPanel;

/* Constructs the Tickertape Control Panel */
ControlPanel ControlPanel_alloc(
    Widget parent, List subscriptions,
    ControlPanelCallback callback, void *context);

/* Releases the resources used by the receiver */
void ControlPanel_free(ControlPanel self);

/* Makes the ControlPanel window visible */
void ControlPanel_show(ControlPanel self, Message view);

/* Answers the receiver's values as a Message */
Message ControlPanel_createMessage(ControlPanel self);

/* Handle notifications */
void ControlPanel_handleNotify(ControlPanel self, Widget widget);

#endif /* CONTROLPANEL_H */
