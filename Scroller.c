/* $Id: Scroller.c,v 1.2 1997/02/05 09:15:52 phelps Exp $ */

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
#define WIDTH 400

struct Scroller_t
{
    Graphics graphics;
    FontInfo fontInfo;
    unsigned int width;
    unsigned int height;
    Window window;
    List messages;
    List visible;
    List widths;
    unsigned int nextVisible;
    long offset;
    long messagesWidth;
    long visibleWidth;
    int clock;
    int isSuspended;
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
    self -> messages = List_alloc();
    self -> visible = List_alloc();
    self -> widths = List_alloc();
    self -> nextVisible = 0;

    /* Add the spacers */
    List_addLast(self -> visible, NULL);
    List_addLast(self -> widths, (void *)self -> width);
    self -> visibleWidth = self -> width;

    self -> offset = 0;
    self -> messagesWidth = 0;
    self -> clock = FREQUENCY;
    self -> isSuspended = 1;

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

	    message = Message_alloc("tickertape", "internal", "buttonpress ignored", 90);
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

/* Stalls the Scroller thread (FIX THIS: should play with timer) */
void Scroller_suspend(Scroller self)
{
    printf("suspending...\n");
    self -> isSuspended = 1;
}

/* Resume the Scroller thread (FIX THIS: should play with timer) */
void Scroller_resume(Scroller self)
{
    printf("resuming...\n");
    self -> isSuspended = 0;
}




/* Adds a MessageView to the receiver */
void Scroller_addMessageView(Scroller self, MessageView view)
{
    MessageView_allocReference(view);
    List_addLast(self -> messages, view);
    self -> messagesWidth += MessageView_getWidth(view);

    if (self -> isSuspended)
    {
	Scroller_resume(self);
    }
}

/* Removes a MessageView from the receiver */
void Scroller_removeMessageView(Scroller self, MessageView view)
{
    List_remove(self -> messages, view);
    self -> messagesWidth -= MessageView_getWidth(view);
    MessageView_freeReference(view);
}


/* Creates a MessageView for the Message and adds it to the receiver */
void Scroller_addMessage(Scroller self, Message message)
{
    MessageView view;

    view = MessageView_alloc(self -> graphics, message, self -> fontInfo);
    Scroller_addMessageView(self, view);
}




/* Redraws a message view at the appropriate location */
void Scroller_refreshMessageView(MessageView view, void *context[])
{
    Scroller self = context[0];
    long offset = (long) context[1];
    unsigned int index = (unsigned int) context[2];
    unsigned long swidth = (unsigned long)List_get(self -> widths, index);

    context[1] = (void *)(offset + swidth);
    context[2] = (void *)(index + 1);

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

    List_doWith(self -> visible, Scroller_refreshMessageView, context);
    Graphics_flush(self -> graphics);
}



/* Move the scroller display one pixel left */
void Scroller_scroll(Scroller self)
{
    MessageView view;
    unsigned long width;

    /* increment the offset */
    self -> offset++;

    /* Check if a message has vanished off the left edge */
    while ((width = (unsigned long) List_first(self -> widths)) <= self -> offset)
    {
	MessageView view = List_dequeue(self -> visible);
	List_dequeue(self -> widths);

	if (view != NULL)
	{
	    MessageView_freeReference(view);
	}

	self -> offset -= width;
	self -> visibleWidth -= width;
    }

    /* Determine if message(s) should be added to the end */
    while (self -> visibleWidth - self -> offset <= (signed)(self -> width))
    {
	MessageView view;

	view = List_get(self -> messages, self -> nextVisible);
	while ((view != NULL) && (MessageView_isTimedOut(view)))
	{
	    Scroller_removeMessageView(self, view);
	    view = List_get(self -> messages, self -> nextVisible);
	}
	self -> nextVisible++;

	if (view == NULL)
	{
	    MessageView last = List_last(self -> visible);

	    if (last == NULL)
	    {
		/* If we're displaying only spaces then suspend */
		if (List_first(self -> visible) == NULL)
		{
		    Scroller_suspend(self);
		    self -> nextVisible = 0;
		    
		    return;
		}

		/* Messages list is empty */
		if (List_last(self -> visible) == NULL)
		{
		    /* There's already a space, so add a spacer to fill to end */
		    width = self -> width - (unsigned long)List_last(self -> widths);
		}
		else
		{
		    /* Otherwise add a spacer the width of the scroller */
		    width = self -> width;
		}
	    }
	    else
	    {
		width = self -> width - MessageView_getWidth(last);
		width = width > END_SPACING ? width : END_SPACING;
	    }

	    self -> nextVisible = 0;
	}
	else
	{
	    MessageView_allocReference(view);
	    width = MessageView_getWidth(view);
	}

	List_addLast(self -> visible, view);
	List_addLast(self -> widths, (void *)width);
	self -> visibleWidth += width;
    }
}

/* Move the scroller display one pixel left */
void Scroller_tick(Scroller self)
{
    /* Abort if stalled (FIX THIS: should be taken care of by playing with timer) */
    if (self -> isSuspended)
    {
	return;
    }

    /* Fade messages */
    if (self -> clock-- <= 0)
    {
	List_do(self -> messages, MessageView_tick);
	self -> clock = FREQUENCY;
    }

    Scroller_scroll(self);
    Scroller_refresh(self);
}

