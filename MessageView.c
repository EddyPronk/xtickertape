/* $Id: MessageView.c,v 1.23 1998/10/21 01:58:07 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>

#include "sanity.h"
#include "TickertapeP.h"
#include "MessageView.h"

/* Sanity checking strings */
#ifdef SANITY
static char *sanity_value = "MessageView";
static char *sanity_freed = "Freed MessageView";
#endif /* SANITY */


#define SPACING 10
#define SEPARATOR ":"


/* Static function headers */
static void Paint(MessageView self, Drawable drawable, int x, int y);
static void SetClock(MessageView self);
static void ClearClock(MessageView self);
static void Tick(MessageView self, XtIntervalId *ignored);
static unsigned int GetAscent(MessageView self);
static unsigned int GetDescent(MessageView self);
static unsigned long GetStringWidth(XFontStruct *font, char *string);
static void ComputeWidths(MessageView self);

/* The MessageView data structure */
struct MessageView_t
{
    /* State */
#ifdef SANITY
    char *sanity_check;
#endif /* SANITY */
    unsigned int refcount;
    TickertapeWidget widget;
    Message message;
    unsigned int fadeLevel;
    char isExpired;

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



/* Displays the receiver on the drawable */
static void Paint(MessageView self, Drawable drawable, int x, int y)
{
    int xpos;
    int level;
    char *string;
    GC gc;

    SANITY_CHECK(self);
    xpos = x;
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



/* Set a timer to call tick when the colors should fade */
static void SetClock(MessageView self)
{
    int duration;
    SANITY_CHECK(self);

    if (self -> isExpired)
    {
	/* 1/20th of a second delay on fading messages */
	duration = 50;
    }
    else
    {
	duration = 60 * 1000 *
	    Message_getTimeout(self -> message) / TtGetFadeLevels(self -> widget);
    }

    self -> timer = TtStartTimer(
	self -> widget,
	duration,
	(XtTimerCallbackProc) Tick,
	(XtPointer) self);
}

/* Clear the timer */
static void ClearClock(MessageView self)
{
    if ((self -> timer) != 0)
    {
	TtStopTimer(self -> widget, self -> timer);
	self -> timer = 0;
    }
}



/* This gets called when the colors need to fade */
static void Tick(MessageView self, XtIntervalId *ignored)
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

    Paint(self, self -> pixmap, 0, self -> ascent);
#ifdef DEBUG    
    printf(":"); fflush(stdout);
#endif /* DEBUG */
    SetClock(self);
}

/* Answers the offset to the baseline we should use */
static unsigned int GetAscent(MessageView self)
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
static unsigned int GetDescent(MessageView self)
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
static unsigned long GetStringWidth(XFontStruct *font, char *string)
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
static void ComputeWidths(MessageView self)
{
    SANITY_CHECK(self);
    self -> groupWidth = GetStringWidth(
	TtFontForGroup(self -> widget),
	Message_getGroup(self -> message));

    self -> userWidth = GetStringWidth(
	TtFontForUser(self -> widget),
	Message_getUser(self -> message));

    self -> stringWidth = GetStringWidth(
	TtFontForString(self -> widget),
	Message_getString(self -> message));

    self -> stringWidth = GetStringWidth(
	TtFontForString(self -> widget),
	Message_getString(self -> message));

    self -> separatorWidth = GetStringWidth(
	TtFontForSeparator(self -> widget),
	SEPARATOR);
}




/* Prints debugging information */
void MessageView_debug(MessageView self)
{
    SANITY_CHECK(self);
    printf("MessageView (%p)\n", self);
#ifdef SANITY
    printf("  sanity_check = \"%s\"\n", self -> sanity_check);
#endif /* SANITY */
    printf("  refcount = %u\n", self -> refcount);
    printf("  widget = %p\n", self -> widget);
    printf("  fadeLevel = %d\n", self -> fadeLevel);
    printf("  isExpired = %s\n", (self -> isExpired) ? "true" : "false");
    printf("  message = Message[%s:%s:%s (%ld)]\n",
	   Message_getGroup(self -> message),
	   Message_getUser(self -> message),
	   Message_getString(self -> message),
	   Message_getTimeout(self -> message)
	);
    printf("  timer = %lx\n", self -> timer);
    printf("  pixmap = %lx\n", self -> pixmap);
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

#ifdef SANITY
    self -> sanity_check = sanity_value;
#endif /* SANITY */
    self -> refcount = 0;
    self -> widget = widget;
    self -> message = message;
    self -> fadeLevel = 0;
    self -> isExpired = FALSE;
    self -> ascent = GetAscent(self);
    self -> height = self -> ascent + GetDescent(self);
    ComputeWidths(self);
    self -> timer = 0;
    self -> pixmap = TtCreatePixmap(
	self -> widget,
	MessageView_getWidth(self),
	self -> height);
    Paint(self, self -> pixmap, 0, self -> ascent);
    SetClock(self);
    return self;
}

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self)
{
    ClearClock(self);

#ifdef SANITY
    self -> sanity_check = sanity_freed;
#endif /* SANITY */
    Message_free(self -> message);
    XFreePixmap(XtDisplay(self -> widget), self -> pixmap);
    free(self);
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
	printf("MessageView_free: %p\n", self); fflush(stdout);
#endif /* DEBUG */
	MessageView_free(self);
    }
}




/* Answers the receiver's Message */
Message MessageView_getMessage(MessageView self)
{
    SANITY_CHECK(self);

    return self -> message;
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
    return (self -> isExpired) || ((self -> fadeLevel) == TtGetFadeLevels(self -> widget));
}

/* MIME-decodes the receiver's message */
void MessageView_decodeMime(MessageView self)
{
    char *mimeType;
    char *mimeArgs;
    char filename[sizeof("/tmp/ticker") + 10];
    char *buffer;
    FILE *file;

    SANITY_CHECK(self);
    mimeType = Message_getMimeType(self -> message);
    mimeArgs = Message_getMimeArgs(self -> message);

    if ((mimeType == NULL) || (mimeArgs == NULL))
    {
	printf("no mime\n");
	return;
    }

#ifdef DEBUG
    printf("MIME: %s %s\n", mimeType, mimeArgs);
#endif /* DEBUG */

    /* Write the mimeArgs to a file */
    sprintf(filename, "/tmp/ticker%lu", getpid());
    file = fopen(filename, "wb");
    fputs(mimeArgs, file);
    fclose(file);

    /* Send it off to metamail to display */
    buffer = alloca(
	strlen(mimeType) + strlen(filename) +
	sizeof("metamail -B -q -b -c   > /dev/null 2>&1"));
    sprintf(buffer, "metamail -B -q -b -c %s %s > /dev/null 2>&1", mimeType, filename);
    system(buffer);

    /* Remove the temporary file */
    unlink(filename);
}


/* MIME-decodes the receiver's message */
void MessageView_expire(MessageView self)
{
    SANITY_CHECK(self);
    if (Message_getTimeout(self -> message) > 0)
    {
	self -> isExpired = TRUE;
	ClearClock(self);
	SetClock(self);
    }
}
