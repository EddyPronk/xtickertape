/* $Id: MessageView.h,v 1.3 1997/02/10 13:31:58 phelps Exp $ */

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

typedef struct MessageView_t *MessageView;

#include "Message.h"

/* Prints debugging information */
void MessageView_debug(MessageView self);

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(TickertapeWidget widget, Message message);

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self);

/* Answers the width (in pixels) of the receiver */
unsigned int MessageView_getWidth(MessageView self);

/* Redisplays the receiver on the drawable */
void MessageView_redisplay(MessageView self, Drawable drawable, int x, int y);


#if 0
/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self);

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self);

/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self);


#endif /* 0 */
#endif /* MESSAGEVIEW_H */
