/* $Id: Scroller.c,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <X11/Xlib.h>
#include "Scroller.h"
#include "Graphics.h"
#include "FontInfo.h"
#include "MessageView.h"
#include "List.h"

/*#define FONT "-adobe-helvetica-medium-r-normal--12-120-75-75-p-67-iso8859-1"*/
#define FONT "*-times-medium-r-normal-*-18-*"
#define END_SPACING 10
#define WIDTH 800

struct Scroller_t
{
    Graphics graphics;
    FontInfo fontInfo;
    unsigned int width;
    unsigned int height;
    Window window;
    List pending;
    List messages;
    List widths;
    long offset;
    long messagesWidth;
    int clock;
    int isStalled;
};



/* Initializes the receiver with the given displayName and screen */
Scroller Scroller_alloc(char *displayName, int screen)
{
    Scroller self = (Scroller) malloc(sizeof(struct Scroller_t));
    self -> graphics = Graphics_alloc(displayName, screen);
    self -> fontInfo = Graphics_allocFontInfo(self -> graphics, FONT);
    self -> width = WIDTH;
    self -> height = FontInfo_getHeight(self -> fontInfo);
    Graphics_setFont(self -> graphics, self -> fontInfo);

    /* Create a window */
    self -> window = Graphics_allocWindow(
	self -> graphics, None,
	"Tickertape", "tickertape", "Tickertape scroller",
	0, 0, self -> width, self -> height,
	Graphics_getWhitePixel(self -> graphics),
	ButtonPressMask | StructureNotifyMask);

    /* Map the window */
    Graphics_mapWindow(self -> graphics, self -> window);

    /* Make sure our changes get flushed to the server */
    Graphics_flush(self -> graphics);

    /* List of messages we're scrolling */
    self -> pending = List_alloc();
    self -> messages = List_alloc();
    self -> widths = List_alloc();

    /* Add the spacers */
    List_addLast(self -> messages, NULL);
    List_addLast(self -> widths, (void *)self -> width);
    List_addLast(self -> messages, NULL);
    List_addLast(self -> widths, (void *)self -> width);

    self -> offset = self -> width - 1;
    self -> messagesWidth = 0;
    self -> clock = FREQUENCY;
    self -> isStalled = 0;

    return self;
}


/* Updates the fd_set's with the receiver's interests */
void Scroller_fdSet(Scroller self, fd_set *readSet, fd_set *writeSet, fd_set *exceptSet)
{
    FD_SET(Graphics_getFD(self -> graphics), readSet);
}

/* Checks the fd_set's to see if they apply to the receiver, and handles them if they do */
void Scroller_handle(Scroller self, fd_set *readSet, fd_set *writeSet, fd_set *exceptSet)
{
    XEvent event;

    if (FD_ISSET(Graphics_getFD(self -> graphics), exceptSet))
    {
	fprintf(stderr, "*** exception on X windows socket (errno=%d)\n", errno);
	exit(1);
    }

    if (FD_ISSET(Graphics_getFD(self -> graphics), readSet))
    {
	XNextEvent(Graphics_getDisplay(self -> graphics), &event);

	if (event.type == Expose)
	{
	    Scroller_refresh(self);
	}
	else if (event.type == ConfigureNotify)
	{
	    XConfigureEvent e = event.xconfigure;
	    self -> width = e.width;
	}
	else if (event.type == ButtonPress)
	{
	    Message message;

	    message = Message_alloc("tickertape", "internal", "buttonpress ignored", 30);
	    Scroller_addMessage(self, message);
	}
	else if (event.type == NoExpose)
	{
	    /* what does this mean? */
	}
	else
	{
	    fprintf(stderr, "X event type=%d ignored\n", event.type);
	}
    }
}


/* Adds a message to the receiver */
void Scroller_addMessage(Scroller self, Message message)
{
    MessageView view;

    /* Add the message to the queue */
    view = MessageView_alloc(self -> graphics, message, self -> fontInfo);
    List_addLast(self -> pending, view);
    printf("\n---\n");
    List_do(self -> pending, MessageView_debug);
    printf("---\n");
}


void Scroller_refreshMessageView(MessageView view, void *context[])
{
    Scroller self = context[0];
    long offset = (long) context[1];
    unsigned int index = (unsigned int) context[2];
    unsigned long swidth = (unsigned long)List_get(self -> widths, index);

    context[1] = (void *)(offset + swidth);
    context[2] = (void *)(index + 1);
    /* Don't worry about messages past the right edge */
/*    if (self -> width + offset < 0)
    {
	return;
    }*/

    if (view != NULL)
    {
	MessageView_display(view, self -> window, offset);
    }
}


/* Redraws the scroller */
void Scroller_refresh(Scroller self)
{
    void *context[3];
    unsigned long offset = 0 - self -> offset;

    context[0] = self;
    context[1] = (void *)offset;
    context[2] = (unsigned int) 0;

    List_doWith(self -> messages, Scroller_refreshMessageView, context);
/*    List_doWith(self -> messages, Scroller_refreshMessageView, context);*/
    Graphics_flush(self -> graphics);
}


/* Pull items off the pending queue and add them to the end of the messages list */
void Scroller_addQueued(Scroller self)
{
    MessageView pending;

    /* copy in the pending messages */
    while ((pending = List_dequeue(self -> pending)) != NULL)
    {
	unsigned long width = MessageView_getWidth(pending);
	
	MessageView_allocReference(pending);
	List_addLast(self -> messages, pending);
	List_addLast(self -> widths, (void *)width);
	self -> messagesWidth += width;
    }
}

/* Remove the first message from the queue and copy the 2nd to the end */
void Scroller_rotate(Scroller self)
{
    MessageView first, second;
    unsigned long wFirst, wSecond;

    /* Discard the first message */
    wFirst = (unsigned long) List_dequeue(self -> widths);
    if ((first = List_dequeue(self -> messages)) != NULL)
    {
	MessageView_freeReference(first);
	if (MessageView_getReferenceCount(first) == 0)
	{
	    self -> messagesWidth -= wFirst;
	    MessageView_free(first);
	}
    }

    /* Duplicate the second to the end of the list */
    second = List_first(self -> messages);
    wSecond = (unsigned long) List_first(self -> widths);
    if (second == NULL)
    {
	unsigned long newWidth;

	/* Add the pending messages to the queue */
	printf("dequeue\n");
	Scroller_addQueued(self);

	/* Figure out new width */
	newWidth = self -> width - self -> messagesWidth;
	newWidth = newWidth < END_SPACING ? END_SPACING : newWidth;
	List_addLast(self -> messages, second);
	List_addLast(self -> widths, (void *)newWidth);
    }
    else if (! MessageView_isTimedOut(second))
    {
	MessageView_allocReference(second);
	List_addLast(self -> messages, second);
	List_addLast(self -> widths, (void *)wSecond);
    }
}


/* Move the scroller display one pixel left */
void Scroller_tick(Scroller self)
{
    MessageView view;
    unsigned long width;

    /* Abort if no messages */
    if (self -> messagesWidth == 0)
    {
	if (List_size(self -> pending) > 0)
	{
	    Scroller_addQueued(self);
	}
	else if (self -> isStalled)
	{
	    return;
	}
	else
	{
	    printf("stalling...\n");
	    /* Shortcut so new messages show up quickly */
	    List_dequeue(self -> widths);
	    List_dequeue(self -> widths);
	    List_addLast(self -> widths, (void *)self -> width);
	    List_addLast(self -> widths, (void *)self -> width);
	    self -> offset = self -> width - 1;
	    self -> isStalled = 1;
	}
    }
    else
    {
	if (self -> isStalled)
	{
	    printf("restarted\n");
	    self -> isStalled = 0;
	}
	self -> offset++;
    }

    if (self -> clock-- <= 0)
    {
	List_do(self -> messages, MessageView_tick);
	self -> clock = FREQUENCY;
    }

    view = List_first(self -> messages);
    width = (unsigned long) List_first(self -> widths);
    if ((self -> messagesWidth) > 0 && (width <= self -> offset))
    {
	Scroller_rotate(self);
	self -> offset = 0;
    }
    Scroller_refresh(self);
}
