/* $Id: MessageView.c,v 1.2 1997/02/05 09:21:40 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "MessageView.h"

#define MESSAGE_SPACING 10

struct MessageView_t
{
    Graphics graphics;
    Drawable drawable;
    Message message;
    FontInfo fontInfo;
    unsigned int groupWidth;
    unsigned int userWidth;
    unsigned int stringWidth;
    unsigned int colonWidth;
    unsigned long intensity;
    Pixmap pixmap;
    unsigned long time;
    unsigned int refcount;
};



/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(Graphics graphics, Message message, FontInfo fontInfo)
{
    MessageView view = (MessageView) malloc(sizeof(struct MessageView_t));

    view -> graphics = graphics;
    view -> message = message;
    view -> fontInfo = fontInfo;
    view -> groupWidth = FontInfo_stringWidth(fontInfo, Message_getGroup(message));
    view -> userWidth = FontInfo_stringWidth(fontInfo, Message_getUser(message));
    view -> stringWidth = FontInfo_stringWidth(fontInfo, Message_getString(message));
    view -> colonWidth = FontInfo_stringWidth(fontInfo, ":");
    view -> intensity = 0;
    view -> pixmap = 0;
    view -> time = 0;
    view -> refcount = 0;
    return view;
}

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self)
{
    /* Free the message */
    if (self -> message)
    {
	Message_free(self -> message);
    }

    if (self -> pixmap)
    {
	Graphics_freePixmap(self -> graphics, self -> pixmap);
    }

    free(self);
}



/* Answers the width of the receiver */
unsigned long MessageView_getWidth(MessageView self)
{
    return self -> groupWidth +
	self -> userWidth +
	self -> stringWidth +
	(2 * self -> colonWidth) +
	MESSAGE_SPACING;
}

/* Answers the height of the receiver */
unsigned long MessageView_getHeight(MessageView self)
{
    return FontInfo_getHeight(self -> fontInfo);
}

/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self)
{
    return Message_getTimeout(self -> message) <= self -> time;
}

/* Answers the number of references to the receiver */
int MessageView_getReferenceCount(MessageView self)
{
    return self -> refcount;
}

/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self)
{
    self -> refcount++;
    return self;
}

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self)
{
    if (self -> refcount > 1)
    {
	self -> refcount--;
    }
    else
    {
	printf("MessageView_free: ");
	MessageView_debug(self);
	MessageView_free(self);
    }
}


/* Answers the pixel for the receiver's separator (:) */
unsigned long MessageView_getSeparatorPixel(MessageView self)
{
    return Graphics_allocPixel(
	self -> graphics,
	self -> intensity,
	self -> intensity,
	self -> intensity);
}


/* Answers the pixel for the receiver's group */
unsigned long MessageView_getGroupPixel(MessageView self)
{
    return Graphics_allocPixel(self -> graphics, self -> intensity, self -> intensity, 0xFFFF);
}

/* Answers the pixel for the receiver's user */
unsigned long MessageView_getUserPixel(MessageView self)
{
    return Graphics_allocPixel(self -> graphics, self -> intensity, 0xFFFF, self ->  intensity);
}

/* Answers the pixl for the receiver's string */
unsigned long MessageView_getStringPixel(MessageView self)
{
    return Graphics_allocPixel(self -> graphics, 0xFFFF, self -> intensity, self -> intensity);
}


/* Forward declaration */
void MessageView_paintPixmap(MessageView self);

/* Create a pixmap for the receiver and draw on it */
Pixmap MessageView_getPixmap(MessageView self)
{
    if (self -> pixmap == 0)
    {
	self -> pixmap = Graphics_allocPixmap(
		self -> graphics,
		MessageView_getWidth(self),
		MessageView_getHeight(self));
	Graphics_setForeground(self -> graphics, Graphics_getWhitePixel(self -> graphics));
	XFillRectangle(
	    Graphics_getDisplay(self -> graphics),
	    self -> pixmap,
	    Graphics_getGC(self -> graphics),
	    0, 0,
	    MessageView_getWidth(self),
	    MessageView_getHeight(self));
	MessageView_paintPixmap(self);
    }

    return self -> pixmap;
}




/* Paint the receiver's message onto its pixmap */
void MessageView_paintPixmap(MessageView self)
{
    Display *display = Graphics_getDisplay(self -> graphics);
    GC gc = Graphics_getGC(self -> graphics);
    Pixmap pixmap = MessageView_getPixmap(self);
    unsigned int ascent = FontInfo_getAscent(self -> fontInfo);
    unsigned long offset = 0;
    char *string;

    Graphics_setForeground(self -> graphics, MessageView_getGroupPixel(self));
    string = Message_getGroup(self -> message);
    XDrawString(display, pixmap, gc, offset, ascent, string, strlen(string));
    offset += self -> groupWidth;

    Graphics_setForeground(self -> graphics, MessageView_getSeparatorPixel(self));
    XDrawString(display, pixmap, gc, offset, ascent, ":", 1);
    offset += self -> colonWidth;

    Graphics_setForeground(self -> graphics, MessageView_getUserPixel(self));
    string = Message_getUser(self -> message);
    XDrawString(display, pixmap, gc, offset, ascent, string, strlen(string));
    offset += self -> userWidth;

    Graphics_setForeground(self -> graphics, MessageView_getSeparatorPixel(self));
    XDrawString(display, pixmap, gc, offset, ascent, ":", 1);
    offset += self -> colonWidth;

    Graphics_setForeground(self -> graphics, MessageView_getStringPixel(self));
    string = Message_getString(self -> message);
    XDrawString(display, pixmap, gc, offset, ascent, string, strlen(string));
}



/* Display the receiver */
void MessageView_display(MessageView self, Drawable drawable, long offset)
{
    Pixmap pixmap = MessageView_getPixmap(self);

    XCopyArea(
	Graphics_getDisplay(self -> graphics),
	pixmap,
	drawable,
	Graphics_getGC(self -> graphics),
	0, 0,
	MessageView_getWidth(self),
	MessageView_getHeight(self),
	offset, 0);
}


/* One second has passed */
void MessageView_tick(MessageView self)
{
    unsigned long timeout;
    unsigned long intensity;

    /* Watch out for the spacer */
    if (self == NULL)
    {
	return;
    }

    /* If we haven't been displayed yet then don't tick */
    if (self -> pixmap == 0)
    {
	return;
    }

    timeout = Message_getTimeout(self -> message);

    self -> time++;
    if (self -> time >= timeout)
    {
	self -> time = timeout;
	intensity = 0xE000;
    }
    else
    {
	intensity = (65535 * self -> time / timeout) & 0xC000;
    }

    if (self -> intensity != intensity)
    {
	self -> intensity = intensity;
	MessageView_paintPixmap(self);
    }	
}


/* Prints debugging information */
void MessageView_debug(MessageView self)
{
    printf("MessageView (0x%p)", self);
    if (self == NULL)
    {
	printf("\n");
	return;
    }

    printf("\"%s\"\n", Message_getString(self -> message));
}
