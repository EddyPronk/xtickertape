/* $Id: MessageView.h,v 1.2 1997/02/10 08:07:34 phelps Exp $ */

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

/* Displays the receiver on the drawable */
void MessageView_redisplay(
    MessageView self, TickertapeWidget widget, Drawable drawable,
    int x, int y);

#if 0
/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self);

/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self);

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self);



/* Answers the width of the receiver */
unsigned long MessageView_getWidth(MessageView self);

/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self);

/* Display the receiver */
void MessageView_redisplay(MessageView self, Drawable drawable, GC gc);


/* One second has passed */
void MessageView_tick(MessageView self);

#endif /* 0 */
#endif /* MESSAGEVIEW_H */
