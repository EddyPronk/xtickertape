/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1995.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, Gehrmann Labs
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: MessageView.c,v 1.39 1999/05/18 00:11:36 phelps Exp $";
#endif /* lint */

#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif /* HAVE_ALLOCA_H */
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include "sanity.h"
#include "ScrollerP.h"
#include "MessageView.h"
#include "StringBuffer.h"

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
    ScrollerWidget widget;
    Message message;
    unsigned int fadeLevel;
    char isExpired;

    /* Cache */
    XtIntervalId timer;
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
    Display *display;
    int hasAttachment;
    int xpos;
    int level;
    char *string;
    GC gc;

    SANITY_CHECK(self);
    display = XtDisplay(self -> widget);
    hasAttachment = Message_hasAttachment(self -> message);
    xpos = x;
    level = self -> fadeLevel;

    /* Draw the group string */
    gc = ScGCForGroup(self -> widget, level);
    string = Message_getGroup(self -> message);
    XDrawString(display, drawable, gc, xpos, y, string, strlen(string));

    /* Underline the group string if there's a MIME attachment */
    if (hasAttachment)
    {
	XDrawLine(display, drawable, gc, xpos, y + 1, xpos + self -> groupWidth, y + 1);
    }

    xpos += self -> groupWidth;

    /* Draw the separator */
    gc = ScGCForSeparator(self -> widget, level);
    string = SEPARATOR;
    XDrawString(display, drawable, gc, xpos, y, string, strlen(string));

    /* Underline the separator if there's a MIME attachment */
    if (hasAttachment)
    {
	XDrawLine(display, drawable, gc, xpos, y + 1, xpos + self -> separatorWidth, y + 1);
    }

    xpos += self -> separatorWidth;

    /* Draw the user string */
    gc = ScGCForUser(self -> widget, level);
    string = Message_getUser(self -> message);
    XDrawString(display, drawable, gc, xpos, y, string, strlen(string));

    /* Underline the user if there's a MIME attachment */
    if (hasAttachment)
    {
	XDrawLine(display, drawable, gc, xpos, y + 1, xpos + self -> userWidth, y + 1);
    }

    xpos += self -> userWidth;

    /* Draw the separator */
    gc = ScGCForSeparator(self -> widget, level);
    string = SEPARATOR;
    XDrawString(display, drawable, gc, xpos, y, string, strlen(string));

    /* Underline the separator if there's a MIME attachment */
    if (hasAttachment)
    {
	XDrawLine(display, drawable, gc, xpos, y + 1, xpos + self -> separatorWidth, y + 1);
    }

    xpos += self -> separatorWidth;

    /* Draw the text string */
    gc = ScGCForString(self -> widget, level);
    string = Message_getString(self -> message);
    XDrawString(display, drawable, gc, xpos, y, string, strlen(string));

    /* Underline the text string if it's got a MIME attachment */
    if (hasAttachment)
    {
	XDrawLine(display, drawable, gc, xpos, y + 1, xpos + self -> stringWidth, y + 1);
    }
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
	    Message_getTimeout(self -> message) / ScGetFadeLevels(self -> widget);
    }

    self -> timer = ScStartTimer(
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
	ScStopTimer(self -> widget, self -> timer);
	self -> timer = 0;
    }
}



/* This gets called when the colors need to fade */
static void Tick(MessageView self, XtIntervalId *ignored)
{
    unsigned int maxLevels;

    SANITY_CHECK(self);
    self -> timer = 0;
    maxLevels = ScGetFadeLevels(self -> widget);

    /* Don't get older than we have to */
    if (self -> fadeLevel + 1 < maxLevels)
    {
	self -> fadeLevel++;
	SetClock(self);
#ifdef DEBUG    
	printf(":"); fflush(stdout);
#endif /* DEBUG */
    }
    else
    {
	self -> isExpired = 1;
    }
}

/* Answers the offset to the baseline we should use */
static unsigned int GetAscent(MessageView self)
{
    XFontStruct *font;
    unsigned int ascent;

    SANITY_CHECK(self);
    font = ScFontForGroup(self -> widget);
    ascent = font -> ascent;

    font = ScFontForUser(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    font = ScFontForString(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;
    
    font = ScFontForSeparator(self -> widget);
    ascent = (ascent > font -> ascent) ? ascent : font -> ascent;

    return ascent;
}

/* Answers the number of pixels below the baseline we need */
static unsigned int GetDescent(MessageView self)
{
    XFontStruct *font;
    unsigned int descent;

    SANITY_CHECK(self);
    font = ScFontForGroup(self -> widget);
    descent = font -> descent;

    font = ScFontForUser(self -> widget);
    descent = (descent > font -> descent) ? descent : font -> descent;

    font = ScFontForString(self -> widget);
    descent = (descent > font -> descent) ? descent : font -> descent;
    
    font = ScFontForSeparator(self -> widget);
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
	ScFontForGroup(self -> widget),
	Message_getGroup(self -> message));

    self -> userWidth = GetStringWidth(
	ScFontForUser(self -> widget),
	Message_getUser(self -> message));

    self -> stringWidth = GetStringWidth(
	ScFontForString(self -> widget),
	Message_getString(self -> message));

    self -> stringWidth = GetStringWidth(
	ScFontForString(self -> widget),
	Message_getString(self -> message));

    self -> separatorWidth = GetStringWidth(
	ScFontForSeparator(self -> widget),
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
    printf("  ascent = %u\n", self -> ascent);
    printf("  height = %u\n", self -> height);
    printf("  groupWidth = %u\n", self -> groupWidth);
    printf("  userWidth = %u\n", self -> userWidth);
    printf("  stringWidth = %u\n", self -> stringWidth);
    printf("  separatorWidth = %u\n", self -> separatorWidth);
}

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(ScrollerWidget widget, Message message)
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
    SetClock(self);
    return self;
}

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self)
{
    ClearClock(self);

    Message_free(self -> message);
#ifdef SANITY
    self -> sanity_check = sanity_freed;
#else /* SANITY */
    free(self);
#endif /* SANITY */
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

    return 
	self -> groupWidth +
	self -> separatorWidth +
	self -> userWidth +
	self -> separatorWidth +
	self -> stringWidth +
	SPACING;
}

/* Redisplays the receiver on the drawable */
void MessageView_redisplay(MessageView self, Drawable drawable, int x, int y)
{
    SANITY_CHECK(self);
    Paint(self, drawable, x, y + self -> ascent);
}


/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self)
{
    SANITY_CHECK(self);
    return (self -> isExpired) || ((self -> fadeLevel) == ScGetFadeLevels(self -> widget));
}

/* MIME-decodes the receiver's message */
void MessageView_decodeMime(MessageView self)
{
    char *mimeType;
    char *mimeArgs;
    StringBuffer buffer;
    char *filename;
    FILE *file;

    SANITY_CHECK(self);

#ifdef METAMAIL
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
    buffer = StringBuffer_alloc();
    StringBuffer_append(buffer, "/tmp/ticker");
    StringBuffer_appendInt(buffer, getpid());
#ifdef HAVE_ALLOCA
    filename = (char *)alloca(StringBuffer_length(buffer) + 1);
    strcpy(filename, StringBuffer_getBuffer(buffer));
#else /* HAVE_ALLOCA */
    filename = strdup(StringBuffer_getBuffer(buffer));
#endif /* HAVE_ALLOCA */

    file = fopen(filename, "wb");
    fputs(mimeArgs, file);
    fclose(file);

    /* Send it off to metamail to display */
    StringBuffer_clear(buffer);
    StringBuffer_append(buffer, METAMAIL);
    StringBuffer_append(buffer, " -B -q -b -c ");
    StringBuffer_append(buffer, mimeType);
    StringBuffer_appendChar(buffer, ' ');
    StringBuffer_append(buffer, filename);
    StringBuffer_append(buffer, " > /dev/null 2>&1");
    system(StringBuffer_getBuffer(buffer));

    /* Remove the temporary file */
    unlink(filename);
#ifndef HAVE_ALLOCA
    free(filename);
#endif /* HAVE_ALLOCA */
    StringBuffer_free(buffer);
#endif /* HAVE_METAMAIL */
}


/* Convince a MessageView that time has run out */
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


/* Convince a MessageView that it's time is up */
void MessageView_expireNow(MessageView self)
{
    SANITY_CHECK(self);
    if (Message_getTimeout(self -> message) > 0)
    {
	self -> isExpired = TRUE;
    }
}
