/* $Id: MessageView.c,v 1.11 1997/02/17 02:28:04 phelps Exp $ */

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

/*#define SANITY_CHECK(x) SanityCheck(x)*/
#define SANITY_CHECK(x)

struct MessageView_t
{
    /* State */
#ifdef SANITY_CHECK
    int isDeleted;
#endif /* SANITY_CHECK */
    unsigned int refcount;
    TickertapeWidget widget;
    Message message;
    unsigned int fadeLevel;

    /* Cache */
    XtIntervalId timer;
    Pixmap pixmap;
    unsigned int ascent;
    unsigned int height;
    unsigned int groupWidth;
    unsigned int userWidth;
    unsigned int stringWidth;
    unsigned int separatorWidth;
};


#ifdef SANITY_CHECK
void Suicide(MessageView self)
{
    exit(0);
}

void SanityCheck(MessageView self)
{
    if (self -> isDeleted)
    {
	Suicide(self);
    }
}
#endif /* SANITY_CHECK */

/* Displays the receiver on the drawable */
static void paint(MessageView self, Drawable drawable, int x, int y)
{
    int xpos = x;
    int level;
    char *string;
    GC gc;

    SANITY_CHECK(self);
    level = self -> fadeLevel;
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
static void setClock(MessageView self)
{
    SANITY_CHECK(self);

    self -> timer = TtStartTimer(
	self -> widget,
	1000 * Message_getTimeout(self -> message) / TtGetFadeLevels(self -> widget),
	tick,
	(XtPointer) self);
}

/* This gets called when the colors need to fade */
static void tick(MessageView self, XtIntervalId *ignored)
{
    unsigned int maxLevels;

    SANITY_CHECK(self);
    self -> timer = 0;
    maxLevels = TtGetFadeLevels(self -> widget);

    /* Don't get older than we have to */
    if (++self -> fadeLevel >= maxLevels)
    {
	return;
    }

    paint(self, self -> pixmap, 0, self -> ascent);
#ifdef DEBUG    
    printf(":"); fflush(stdout);
#endif /* DEBUG */
    setClock(self);
}

/* Answers the offset to the baseline we should use */
static unsigned int getAscent(MessageView self)
{
    XFontStruct *font;
    unsigned int ascent;

    SANITY_CHECK(self);
    font = TtFontForGroup(self -> widget);
    ascent = font -> ascent;

    font = TtFontForUser(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    font = TtFontForString(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;
    
    font = TtFontForSeparator(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    return ascent;
}

/* Answers the number of pixels below the baseline we need */
static unsigned int getDescent(MessageView self)
{
    XFontStruct *font;
    unsigned int descent;

    SANITY_CHECK(self);
    font = TtFontForGroup(self -> widget);
    descent = font -> descent;

    font = TtFontForUser(self -> widget);
    descent = (descent > font -> descent) ? descent : font -> descent;

    font = TtFontForString(self -> widget);
    descent = (descent > font -> descent) ? descent : font -> descent;
    
    font = TtFontForSeparator(self -> widget);
    descent = (descent > font -> descent) ? descent : font -> descent;

    return descent;
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
    SANITY_CHECK(self);
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
    SANITY_CHECK(self);
    printf("MessageView (0x%p)\n", self);
    printf("  refcount = %u\n", self -> refcount);
    printf("  widget = 0x%p\n", self -> widget);
    printf("  fadeLevel = %d\n", self -> fadeLevel);
    printf("  message = Message[%s:%s:%s (%ld)]\n",
	   Message_getGroup(self -> message),
	   Message_getUser(self -> message),
	   Message_getString(self -> message),
	   Message_getTimeout(self -> message)
	);
    printf("  timer = 0x%lx\n", self -> timer);
    printf("  pixmap = 0x%lx\n", self -> pixmap);
    printf("  ascent = %u\n", self -> ascent);
    printf("  height = %u\n", self -> height);
    printf("  groupWidth = %u\n", self -> groupWidth);
    printf("  userWidth = %u\n", self -> userWidth);
    printf("  stringWidth = %u\n", self -> stringWidth);
    printf("  separatorWidth = %u\n", self -> separatorWidth);
}

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(TickertapeWidget widget, Message message)
{
    MessageView self = (MessageView) malloc(sizeof(struct MessageView_t));

#ifdef SANITY_CHECK
    self -> isDeleted = FALSE;
#endif /* SANITY_CHECK */
    self -> refcount = 0;
    self -> widget = widget;
    self -> message = message;
    self -> fadeLevel = 0;
    self -> ascent = getAscent(self);
    self -> height = self -> ascent + getDescent(self);
    computeWidths(self);
    self -> timer = 0;
    self -> pixmap = TtCreatePixmap(self -> widget, MessageView_getWidth(self));
    paint(self, self -> pixmap, 0, self -> ascent);
    setClock(self);
    return self;
}

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self)
{
    if (self -> timer)
    {
	TtStopTimer(self -> widget, self -> timer);
    }

#ifdef SANITY_CHECK
    self -> isDeleted = True;
#else /* SANITY_CHECK */
    Message_free(self -> message);
    XFreePixmap(XtDisplay(self -> widget), self -> pixmap);
    free(self);
#endif /* SANITY_CHECK */
}



/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self)
{
    SANITY_CHECK(self);
    self -> refcount++;
    return self;
}

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self)
{
    SANITY_CHECK(self);

    if (self -> refcount > 1)
    {
	self -> refcount--;
    }
    else
    {
#ifdef DEBUG	
	printf("MessageView_free: 0x%p\n", self); fflush(stdout);
#endif /* DEBUG */
	MessageView_free(self);
    }
}




/* Answers the width (in pixels) of the receiver */
unsigned int MessageView_getWidth(MessageView self)
{
    SANITY_CHECK(self);

    return (self -> groupWidth) +
	(self -> userWidth) +
	(self -> stringWidth) +
	((self -> separatorWidth) << 1) +
	SPACING;
}

/* Redisplays the receiver on the drawable */
void MessageView_redisplay(MessageView self, Drawable drawable, int x, int y)
{
    SANITY_CHECK(self);
    XCopyArea(XtDisplay(self -> widget), self -> pixmap,
	      drawable, TtGCForBackground(self -> widget),
	      0, 0, MessageView_getWidth(self), self -> height, x, y);
}


/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self)
{
    SANITY_CHECK(self);
    return ((self -> fadeLevel) == TtGetFadeLevels(self -> widget));
}
