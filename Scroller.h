/* $Id: Scroller.h,v 1.2 1997/02/09 07:02:37 phelps Exp $ */

#ifndef SCROLLER_H
#define SCROLLER_H

typedef struct Scroller_t *Scroller;

#include "Graphics.h"
#include "Message.h"




#ifndef FREQUENCY
# define FREQUENCY 24 /* Hz */
#endif

/* Creates a new Scroller on the given displayName and screen */
Scroller Scroller_alloc(char *displayName, int screen);

/* Updates the fd_set's with the receiver's interests */
void Scroller_fdSet(Scroller self, fd_set *readSet, fd_set *writeSet, fd_set *exceptSet);

/* Checks the fd_set's to see if they apply to the receiver, and handles them if they do */
void Scroller_handle(Scroller self, fd_set *readSet, fd_set *writeSet, fd_set *exceptSet);

/* Adds a message to the receiver */
void Scroller_addMessage(Scroller self, Message message);


/* Redraws the scroller */
void Scroller_refresh(Scroller self);

/* Paints all of the messages onto a Pixmap */
void Scroller_paintMessages(Scroller self);

/* Move the scroller display one pixel left */
void Scroller_tick(Scroller self);

#endif
