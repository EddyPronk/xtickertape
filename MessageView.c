/* $Id: MessageView.c,v 1.4 1997/02/10 13:31:58 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include "TickertapeP.h"
#include "MessageView.h"

#define SPACING 10
#define SEPARATOR ":"

struct MessageView_t
{
    /* State */
    unsigned int refcount;
    TickertapeWidget widget;
    Message message;
    unsigned int fadeLevel;

    /* Cache */
    Pixmap pixmap;
    unsigned int ascent;
    unsigned int groupWidth;
    unsigned int userWidth;
    unsigned int stringWidth;
    unsigned int separatorWidth;
};



/* Displays the receiver on the drawable */
static void paint(MessageView self, Drawable drawable, int x, int y)
{
    int xpos = x;
    int level = self -> fadeLevel;
    char *string;
    GC gc;

    gc = TtGCForGroup(self -> widget, level);
    string = Message_getGroup(self -> message);
    XDrawString(XtDisplay(self -> widget), drawable, gc, xpos, y, string, strlen(string));
    xpos += self -> groupWidth;

    gc = TtGCForSeparator(self -> widget, level);
    string = SEPARATOR;
    XDrawString(XtDisplay(self -> widget), drawable, gc, xpos, y, string, strlen(string));
    xpos += self -> separatorWidth;

    gc = TtGCForUser(self -> widget, level);
    string = Message_getUser(self -> message);
    XDrawString(XtDisplay(self -> widget), drawable, gc, xpos, y, string, strlen(string));
    xpos += self -> userWidth;

    gc = TtGCForSeparator(self -> widget, level);
    string = SEPARATOR;
    XDrawString(XtDisplay(self -> widget), drawable, gc, xpos, y, string, strlen(string));
    xpos += self -> separatorWidth;

    gc = TtGCForString(self -> widget, level);
    string = Message_getString(self -> message);
    XDrawString(XtDisplay(self -> widget), drawable, gc, xpos, y, string, strlen(string));
}


static void tick();

/* Set a timer to call tick when the colors should fade */
static void startClock(MessageView self)
{
    TtStartTimer(
	self -> widget,
	1000 * Message_getTimeout(self -> message) / TtGetFadeLevels(self -> widget),
	tick,
	self);
}

/* This gets called when the colors need to fade */
static void tick(MessageView self, XtIntervalId *ignored)
{
    unsigned int maxLevels = TtGetFadeLevels(self -> widget);

    /* Don't get older than we have to */
    if (self -> fadeLevel + 1 >= maxLevels)
    {
	return;
    }

    self -> fadeLevel++;
    paint(self, self -> pixmap, 0, self -> ascent);
    printf(":");
    fflush(stdout);
    startClock(self);
}

/* Answers the offset to the baseline we should use */
static unsigned int getAscent(MessageView self)
{
    XFontStruct *font = TtFontForGroup(self -> widget);
    unsigned int ascent = font -> ascent;

    font = TtFontForUser(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    font = TtFontForString(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;
    
    font = TtFontForSeparator(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    return ascent;
}

/* Computes the number of pixels used to display string using font */
static unsigned long getStringWidth(XFontStruct *font, char *string)
{
    unsigned int first = font -> min_char_or_byte2;
    unsigned int last = font -> max_char_or_byte2;
    unsigned long width = 0;
    unsigned int defaultWidth;
    unsigned char *pointer;

    /* make sure default_char is valid */
    if ((first <= font -> default_char) && (font -> default_char <= last))
    {
	defaultWidth = font -> per_char[font -> default_char].width;
    }
    /* if no default char then see if a space will work */
    else if ((first <= ' ') && (' ' <= last))
    {
	defaultWidth = font -> per_char[' '].width;
    }
    /* abandon all hope and use max_width */
    else
    {
	defaultWidth = font -> max_bounds.width;
    }

    /* add up character widths */
    for (pointer = (unsigned char *)string; *pointer != '\0'; pointer++)
    {
	unsigned char ch = *pointer;

	if ((first <= ch) && (ch <= last))
	{
	    width += font -> per_char[ch - first].width;
	}
	else
	{
	    width += defaultWidth;
	}
    }

    return width;
}


/* Computes the widths of the various components of the MessageView */
static void computeWidths(MessageView self)
{
    self -> groupWidth = getStringWidth(
	TtFontForGroup(self -> widget),
	Message_getGroup(self -> message));

    self -> userWidth = getStringWidth(
	TtFontForUser(self -> widget),
	Message_getUser(self -> message));

    self -> stringWidth = getStringWidth(
	TtFontForString(self -> widget),
	Message_getString(self -> message));

    self -> stringWidth = getStringWidth(
	TtFontForString(self -> widget),
	Message_getString(self -> message));

    self -> separatorWidth = getStringWidth(
	TtFontForSeparator(self -> widget),
	SEPARATOR);
}




/* Prints debugging information */
void MessageView_debug(MessageView self)
{
    printf("MessageView (0x%p)", self);
    printf("  refcount = %u\n", self -> refcount);
    printf("  widget = 0x%p\n", self -> widget);
    printf("  fadeLevel = %d\n", self -> fadeLevel);
    printf("  message = Message[%s:%s:%s (%ld)]\n",
	   Message_getGroup(self -> message),
	   Message_getUser(self -> message),
	   Message_getString(self -> message),
	   Message_getTimeout(self -> message)
	);
    printf("  pixmap = 0x%lx\n", self -> pixmap);
    printf("  ascent = %u\n", self -> ascent);
    printf("  groupWidth = %u\n", self -> groupWidth);
    printf("  userWidth = %u\n", self -> userWidth);
    printf("  stringWidth = %u\n", self -> stringWidth);
    printf("  separatorWidth = %u\n", self -> separatorWidth);
}

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(TickertapeWidget widget, Message message)
{
    MessageView self = (MessageView) malloc(sizeof(struct MessageView_t));

    self -> refcount = 0;
    self -> widget = widget;
    self -> message = message;
    self -> fadeLevel = 0;
    self -> ascent = getAscent(self);
    computeWidths(self);
    self -> pixmap = TtCreatePixmap(self -> widget, MessageView_getWidth(self));
    paint(self, self -> pixmap, 0, self -> ascent);
    startClock(self);
    return self;
}

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self)
{
    free(self);
}


/* Answers the width (in pixels) of the receiver */
unsigned int MessageView_getWidth(MessageView self)
{
    return (self -> groupWidth) +
	(self -> userWidth) +
	(self -> stringWidth) +
	((self -> separatorWidth) << 1) +
	SPACING;
}

/* Redisplays the receiver on the drawable */
void MessageView_redisplay(MessageView self, Drawable drawable, int x, int y)
{
    /* FIX THIS: height should be computed somehow */
    unsigned int height = 1000;

    XCopyArea(XtDisplay(self -> widget), self -> pixmap,
	      drawable, TtGCForBackground(self -> widget),
	      0, 0, MessageView_getWidth(self), height, x, y);
}





#if 0
/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(TickertapeWidget widget, Message message)
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


#endif /* 0 */
