/* $Id: MessageView.h,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#include <X11/Xlib.h>

typedef struct MessageView_t *MessageView;

#include "Graphics.h"
#include "FontInfo.h"
#include "Message.h"

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(Graphics graphics, Message message, FontInfo fontInfo);

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self);

/* Answers the width of the receiver */
unsigned long MessageView_getWidth(MessageView self);

/* Answers the height of the receiver */
unsigned long MessageView_getHeight(MessageView self);

/* Answers the height of the receiver */
unsigned long MessageView_getHeight(MessageView self);

/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self);

/* Answers the number of references to the receiver */
int MessageView_getReferenceCount(MessageView self);

/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self);

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self);


/* Display the receiver */
void MessageView_display(MessageView self, Drawable drawable, long offset);

/* One second has passed */
void MessageView_tick(MessageView self);

/* Prints debugging information */
void MessageView_debug(MessageView self);

#endif /* MESSAGEVIEW_H */
