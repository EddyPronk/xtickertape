/* -*- mode: c; c-file-style: "elvin" -*- */
/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdio.h> /* atoi, fprintf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* calloc, free, malloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* memset */
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h> /* assert */
#endif
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <Xm/XmAll.h>
#include <Xm/PrimitiveP.h>
#include <Xm/TransferT.h>
#include <Xm/TraitP.h>
#include "replace.h"
#include "globals.h"
#include "utils.h"
#include "utf8.h"
#include "message.h"
#include "message_view.h"
#include "ScrollerP.h"

/*
 * Resources
 */
#define offset(field) XtOffsetOf(ScrollerRec, field)

static XtResource resources[] =
{
    /* XtCallbackList callbacks */
    {
        XtNcallback, XtCCallback, XtRCallback, sizeof(XtPointer),
        offset(scroller.callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XtCallbackList attachment_callbacks */
    {
        XtNattachmentCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
        offset(scroller.attachment_callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XtCallbackList kill_callbacks */
    {
        XtNkillCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
        offset(scroller.kill_callbacks), XtRCallback, (XtPointer)NULL
    },

    /* XFontStruct *font */
    {
        XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        offset(scroller.font), XtRString, XtDefaultFont
    },

    /* The font's code set */
    {
        XtNfontCodeSet, XtCString, XtRString, sizeof(char *),
        offset(scroller.code_set), XtRString, (XtPointer)NULL
    },

    /* Pixel groupPixel */
    {
        XtNgroupPixel, XtCGroupPixel, XtRPixel, sizeof(Pixel),
        offset(scroller.group_pixel), XtRString, "Blue"
    },

    /* Pixel userPixel */
    {
        XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
        offset(scroller.user_pixel), XtRString, "Green"
    },

    /* Pixel stringPixel */
    {
        XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
        offset(scroller.string_pixel), XtRString, "Red"
    },

    /* Pixel separatorPixel */
    {
        XtNseparatorPixel, XtCSeparatorPixel, XtRPixel, sizeof(Pixel),
        offset(scroller.separator_pixel), XtRString, XtDefaultForeground
    },

    /* Dimension fadeLevels */
    {
        XtNfadeLevels, XtCFadeLevels, XtRDimension, sizeof(Dimension),
        offset(scroller.fade_levels), XtRImmediate, (XtPointer)5
    },

    /* Position drag_delta (in pixels) */
    {
        XtNdragDelta, XtCDragDelta, XtRPosition, sizeof(Position),
        offset(scroller.drag_delta), XtRImmediate, (XtPointer)3
    },

    /* Boolean use_pixmap */
    {
        XtNusePixmap, XtCUsePixmap, XtRBoolean, sizeof(Boolean),
        offset(scroller.use_pixmap), XtRImmediate, (XtPointer)False
    },

    /* Dimension frequency (in Hz) */
    {
        XtNfrequency, XtCFrequency, XtRDimension, sizeof(Dimension),
        offset(scroller.frequency), XtRImmediate, (XtPointer)24
    },

    /* Position step (in pixels) */
    {
        XtNstepSize, XtCStepSize, XtRPosition, sizeof(Position),
        offset(scroller.step), XtRImmediate, (XtPointer)1
    }
};
#undef offset


/*
 * Method declarations
 */
static void
scroll(ScrollerWidget self, int offset);

static void
redisplay(ScrollerWidget self, Region region);
static void
gexpose(Widget widget, XtPointer rock, XEvent *event, Boolean *ignored);
static void
paint(ScrollerWidget self,
      int x,
      int y,
      unsigned int width,
      unsigned int height);

#if defined(DEBUG)
typedef enum glyph_ref_type glyph_ref_type_t;
enum glyph_ref_type {
    REF_GAP,
    REF_QUEUE,
    REF_HOLDER,
    REF_REPLACE
};

static const char *ref_type_names[] = {
    "GAP",
    "QUEUE",
    "HOLDER",
    "REPLACE"
};

/* To help track memory leaks, we record the line number, reference
 * type and a pointer for each reference to a glyph in debug builds.
 * By inspecting this list, we should be able to determine why a glyph
 * hasn't been freed. */
typedef struct glyph_ref *glyph_ref_t;
struct glyph_ref {
    /* The next reference in the list. */
    glyph_ref_t next;

    /* The type of reference. */
    glyph_ref_type_t type;

    /* The name of the file where the reference was added. */
    const char *file;

    /* The line number where the reference was added. */
    int line;

    /* The pointer. */
    void* rock;
};
#endif /* DEBUG */

/* The structure of a glyph */
struct glyph {
    /* The previous glyph in the circular queue */
    glyph_t previous;

    /* The next glyph in the circular queue */
    glyph_t next;

    /* The glyph which supersedes this one */
    glyph_t successor;

    /* The widget which display's this glyph */
    ScrollerWidget widget;

#if defined(DEBUG)
    /* The references to this glyph. */
    glyph_ref_t refs;
#else /* !DEBUG */
    /* The number of external references to this glyph */
    short ref_count;
#endif /* DEBUG */

    /* The number of glyph_holders pointing to this glyph */
    short visible_count;

    /* The glyph's message_view or NULL if this is the gap */
    message_view_t message_view;

    /* The sizes of the message view */
    struct string_sizes sizes;

    /* The current degree of fading (0 is none) */
    int fade_level;

    /* Is this glyph expired? */
    Bool is_expired;

    /* Our timeout's id or None */
    XtIntervalId timeout;
};

/* Forward declaration */
static void
glyph_free(glyph_t self);
static void
glyph_set_clock(glyph_t self, int level_count);

#if defined(DEBUG)
# define GLYPH_ALLOC_REF(glyph, type, rock)     \
    glyph_alloc_ref(glyph, type, __FILE__, __LINE__, rock)
# define GLYPH_FREE_REF(glyph, type, rock)      \
    glyph_free_ref(glyph, type, __FILE__, __LINE__, rock)

static void
glyph_alloc_ref(glyph_t self, glyph_ref_type_t type, 
                const char *file, int line, void* rock)
{
    glyph_ref_t ref;

    /* Allocate memory to hold the reference. */
    ref = malloc(sizeof(struct glyph_ref));
    ASSERT(ref != NULL);

    ref->type = type;
    ref->file = file;
    ref->line = line;
    ref->rock = rock;

    ref->next = self->refs;
    self->refs = ref;

    DPRINTF((1, "%s:%d: acquired %s reference to glyph %p with rock=%p\n",
             file, line, ref_type_names[type], self, rock));
}

static void
glyph_free_ref(glyph_t self, glyph_ref_type_t type,
               const char* file, int line, void* rock)
{
    glyph_ref_t ref;
    glyph_ref_t prev;

    /* Find and remove the reference. */
    prev = NULL;
    for (ref = self->refs; ref != NULL; ref = ref->next) {
        if (ref->type == type && ref->rock == rock) {
            DPRINTF((1, "%s:%d: freeing %s reference to glyph %p acquired "
                     "at %s:%d with reference rock=%p\n",
                     file, line, ref_type_names[type], self,
                     ref->file, ref->line, rock));

            /* Update the previous link to point to the next one. */
            if (prev == NULL) {
                ASSERT(ref == self->refs);
                self->refs = ref->next;
            } else {
                prev->next = ref->next;
            }

            free(ref);
            break;
        }

        prev = ref;
    }

    /* If we don't find the reference the we have a bug. */
    ASSERT(ref != NULL);

    /* If there are no more references then discard the glyph. */
    if (self->refs == NULL) {
        glyph_free(self);
    }
}
#else /* !DEBUG */
# define GLYPH_ALLOC_REF(glyph, type, rock)     \
    glyph->ref_count++

# define GLYPH_FREE_REF(glyph, type, rock)      \
    do {                                        \
        if (--glyph->ref_count == 0) {          \
            glyph_free(glyph);                  \
        }                                       \
    } while (0)
#endif /* DEBUG */

/* Allocates and initializes a new glyph holder for the given message */
static glyph_t
glyph_alloc(ScrollerWidget widget, message_t message)
{
    glyph_t self;

    /* Allocate memory for a new glyph */
    self = malloc(sizeof(struct glyph));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize its fields to sane values */
    memset(self, 0, sizeof(struct glyph));

    /* Increment the reference count */
    self->widget = widget;

    /* Bail out now if we're the gap */
    DPRINTF((1, "allocated glyph %p\n", self));
    if (message == NULL) {
        return self;
    }

    /* Allocate a message view for display */
    self->message_view = message_view_alloc(message, 0,
                                            widget->scroller.renderer);
    if (self->message_view == NULL) {
        glyph_free(self);
        return NULL;
    }

    /* Figure out how big the glyph should be */
    message_view_get_sizes(self->message_view, False, &self->sizes);

    /* Add a little space on the end */
    /* FIX THIS: compute the per_char info for a space */
    self->sizes.width += widget->scroller.font->ascent;

    /* Start the clock */
    self->timeout = None;
    glyph_set_clock(self, widget->scroller.fade_levels);

    return self;
}

/* Frees the resources consumed by the receiver */
static void
glyph_free(glyph_t self)
{
    /* Sanity checks */
    ASSERT(self->refs == NULL);
    ASSERT(self->visible_count == 0);

    /* If the glyph has a successor then release our reference to it. */
    if (self->successor) {
	GLYPH_FREE_REF(self->successor, REF_REPLACE, self);
    }

    /* Free the message_view */
    if (self->message_view) {
        message_view_free(self->message_view);
    }

    /* Cancel any pending timeout */
    if (self->timeout != None) {
        XtRemoveTimeOut(self->timeout);
    }

    /* Free the glyph itself */
    free(self);

    DPRINTF((1, "freed glyph %p\n", self));
}

/* This is called each time the glyph should fade */
static void
glyph_tick(XtPointer closure, XtIntervalId *id)
{
    glyph_t self = (glyph_t)closure;
    int level_count = self->widget->scroller.fade_levels;

    /* We don't have a timeout */
    self->timeout = None;

    /* Have we faded through all of the levels yet? */
    if (self->fade_level + 1 >= level_count) {
        /* Don't expire more than once */
        if (!self->is_expired) {
            /* FIX THIS: we can do this ourselves */
            self->is_expired = True;
            ScGlyphExpired(self->widget, self);
        }

        return;
    }

    /* Go to the next level */
    self->fade_level++;
    glyph_set_clock(self, level_count);

    /* Redraw this glyph */
    ScRepaintGlyph(self->widget, self);
}

/* Set the clock for the next time we need to fade this widget */
static void
glyph_set_clock(glyph_t self, int level_count)
{
    int duration;

    /* Sanity check */
    ASSERT(self->timeout == None);

    /* Has the glyph expired? */
    if (self->is_expired) {
        /* Yes: fade very quickly (20 times/sec) */
        duration = 50;
    } else {
        /* No: fade according to the timeout of the message */
        message_t message = message_view_get_message(self->message_view);
        duration = 1000 * message_get_timeout(message) / level_count;
    }

    self->timeout = XtAppAddTimeOut(
        XtWidgetToApplicationContext((Widget)self->widget),
        duration, glyph_tick, self);
}

/* Returns the glyph's message */
static message_t
glyph_get_message(glyph_t self)
{
    /* Watch for the gap */
    if (self->message_view == NULL) {
        return NULL;
    }

    return message_view_get_message(self->message_view);
}

/* Returns non-zero if the glyph has been killed */
static int
glyph_is_killed(glyph_t self)
{
    message_t message;

    /* Get the glyph's message */
    message = glyph_get_message(self);
    if (message == NULL) {
        return False;
    }

    return message_is_killed(message);
}

/* Draw the glyph */
static void
glyph_paint(Display *display,
            Drawable drawable,
            GC gc,
            glyph_t self,
            int x,
            int y,
            XRectangle *bbox)
{
    /* If we're the gap then there's nothing to do */
    if (self->message_view == NULL) {
        return;
    }

    /* Delegate to the message_view */
    message_view_paint(
        self->message_view,
        display, drawable, gc,
        False, 0,
        self->widget->scroller.group_pixels[self->fade_level],
        self->widget->scroller.user_pixels[self->fade_level],
        self->widget->scroller.string_pixels[self->fade_level],
        self->widget->scroller.separator_pixels[self->fade_level],
        x - MIN(self->sizes.lbearing, 0), y,
        bbox);
}

/* Returns the total width of the glyph */
static long
glyph_get_width(glyph_t self)
{
    /* Sanity check */
    ASSERT(self->message_view != NULL);
    return MAX(self->sizes.rbearing, self->sizes.width) -
           MIN(self->sizes.lbearing, 0);
}

/* Returns the glyph which supersedes this one */
static glyph_t
glyph_get_successor(glyph_t self)
{
    /* Locate the successor with no successor */
    while (self->successor != NULL) {
        self = self->successor;
    }

    return self;
}

/* Deletes a glyph */
static void
glyph_delete(glyph_t self)
{
    /* Don't delete the gap or already expired glyphs. */
    if (self->message_view == NULL || self->is_expired) {
        return;
    }

    /* Mark the glyph as expired so that it won't get considered as a
     * replacement for glyph. */
    self->is_expired = True;
}

/* Expires the glyph */
static void
glyph_expire(glyph_t self, ScrollerWidget widget)
{
    /* Don't expire the gap and don't expire more than once */
    if (self->message_view == NULL || self->is_expired) {
        return;
    }

    /* Otherwise get gone */
    self->is_expired = True;
    ScRepaintGlyph(widget, self);

    /* Restart the timer so that we can quickly fade */
    if (self->timeout != None) {
        XtRemoveTimeOut(self->timeout);
        self->timeout = None;
    }

    glyph_set_clock(self, widget->scroller.fade_levels);
    ScGlyphExpired(widget, self);
}

/* Returns true if the queue contains no unexpired messages */
static int
queue_is_empty(glyph_t head)
{
    glyph_t glyph = head->next;

    while (glyph != head) {
        if (!glyph->is_expired) {
            return 0;
        }

        glyph = glyph->next;
    }

    return 1;
}

/* Adds an item to a circular queue of glyphs */
static void
queue_add(glyph_t tail, glyph_t glyph)
{
    glyph->previous = tail;
    glyph->next = tail->next;

    tail->next->previous = glyph;
    tail->next = glyph;

    GLYPH_ALLOC_REF(glyph, REF_QUEUE, NULL);
}

/* Locates the item in the queue with the given tag */
static glyph_t
queue_find(glyph_t head, const char *tag)
{
    glyph_t probe;

    /* Bail out now if there is no tag */
    if (tag == NULL) {
        return NULL;
    }

    /* Look for the tag in the queue */
    for (probe = head->next; probe != head; probe = probe->next) {
        message_t message;
        const char *probe_tag;

        /* Check for a match */
        message = glyph_get_message(probe);
        if (message == NULL) {
            continue;
        }

        probe_tag = message_get_tag(message);
        if (probe_tag == NULL) {
            continue;
        }

        if (strcmp(tag, probe_tag) == 0) {
            return probe;
        }
    }

    /* Not found */
    return NULL;
}

/* Replace an existing queue item with a new one with the same tag */
static void
queue_replace(glyph_t old_glyph, glyph_t new_glyph)
{
    glyph_t previous = old_glyph->previous;
    glyph_t next = old_glyph->next;

    /* Swap the message into place */
    new_glyph->previous = previous;
    previous->next = new_glyph;
    old_glyph->previous = NULL;

    new_glyph->next = next;
    next->previous = new_glyph;
    old_glyph->next = NULL;

    /* Tell the old glyph that it's been superseded; it retains a
     * reference to its successor. */
    GLYPH_ALLOC_REF(new_glyph, REF_REPLACE, old_glyph);
    old_glyph->successor = new_glyph;

    /* Clean up */
    GLYPH_FREE_REF(old_glyph, REF_QUEUE, NULL);
}

/* Removes an item from a circular queue of glyphs */
static void
queue_remove(glyph_t glyph)
{
    /* Don't dequeue if the glyph isn't queued */
    if (glyph->next == NULL) {
        ASSERT(glyph->previous == NULL);
        return;
    }

    /* Remove it from the list */
    glyph->previous->next = glyph->next;
    glyph->next->previous = glyph->previous;

    glyph->previous = NULL;
    glyph->next = NULL;

    /* Lose our reference to the glyph */
    GLYPH_FREE_REF(glyph, REF_QUEUE, NULL);
}

/* The glyph_holders are used to maintain a doubly-linked list of the
 * glyphs which are currently visible in the scroller window.  The
 * width is also recorded because under certain circumstances a single
 * glyph (usually the gap) may be displayed twice in the scroller with
 * differing widths. */
struct glyph_holder {
    /* The previous glyph_holder_t in the list */
    glyph_holder_t previous;

    /* The next glyph_holder_t in the list */
    glyph_holder_t next;

    /* The width (in pixels) of this holder's glyph */
    int width;

    /* This holder's glyph */
    glyph_t glyph;
};

/* Allocates and initializes a new glyph_holder */
static glyph_holder_t
glyph_holder_alloc(glyph_t glyph, int width)
{
    glyph_holder_t self;

    /* Allocate memory for the receiver */
    self = malloc(sizeof(struct glyph_holder));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize its contents to sane values */
    self->previous = NULL;
    self->next = NULL;
    self->width = width;

    /* Record the glyph and tell it that it's visible */
    self->glyph = glyph;
    GLYPH_ALLOC_REF(glyph, REF_HOLDER, self);
    glyph->visible_count++;
    return self;
}

/* Frees the resources consumed by the receiver */
static void
glyph_holder_free(glyph_holder_t self)
{
    glyph_t glyph = self->glyph;

    /* Dequeue the glyph if it's expired and invisible */
    if (--glyph->visible_count == 0 && glyph->is_expired) {
        queue_remove(glyph);
    }

    /* Lose our reference to the glyph */
    GLYPH_FREE_REF(glyph, REF_HOLDER, self);
    free(self);
}

/* Paints the holder's glyph */
static void
glyph_holder_paint(Display *display,
                   Drawable drawable,
                   GC gc,
                   glyph_holder_t self,
                   int x,
                   int y,
                   XRectangle *bbox)
{
    /* Delegate to the glyph */
    glyph_paint(display, drawable, gc, self->glyph, x, y, bbox);
}

/*
 * Private Methods
 */

static void
create_gc(ScrollerWidget self);
static Pixel *
create_faded_colors(Display *display,
                    Colormap colormap,
                    XColor *first,
                    XColor *last,
                    unsigned int level_count);
static void
enable_clock(ScrollerWidget self);
static void
disable_clock(ScrollerWidget self);
static void
set_clock(ScrollerWidget self);
static void
tick(XtPointer widget, XtIntervalId *interval);


/* Answers a GC with the right background color and font */
static void
create_gc(ScrollerWidget self)
{
    XGCValues values;

    values.font = self->scroller.font->fid;
    values.background = self->core.background_pixel;
    values.foreground = self->core.background_pixel;
    self->scroller.backgroundGC = XCreateGC(
        XtDisplay(self), XtWindow(
            self), GCFont | GCBackground | GCForeground, &values);
    self->scroller.gc = XCreateGC(
        XtDisplay(self), XtWindow(self), GCFont | GCBackground, &values);
}

/* Answers an array of colors fading from first to last */
static Pixel *
create_faded_colors(Display *display,
                    Colormap colormap,
                    XColor *first,
                    XColor *last,
                    unsigned int level_count)
{
    Pixel *result = calloc(level_count, sizeof(Pixel));
    long redNumerator = (long)last->red - first->red;
    long greenNumerator = (long)last->green - first->green;
    long blueNumerator = (long)last->blue - first->blue;
    long denominator = level_count;
    long index;

    for (index = 0; index < level_count; index++) {
        XColor color;

        color.red = first->red + (redNumerator * index / denominator);
        color.green = first->green + (greenNumerator * index / denominator);
        color.blue = first->blue + (blueNumerator * index / denominator);
        color.flags = DoRed | DoGreen | DoBlue;
        XAllocColor(display, colormap, &color);
        result[index] = color.pixel;
    }

    return result;
}

/* Re-enables the timer */
static void
enable_clock(ScrollerWidget self)
{
    if (self->scroller.timer == 0 && self->scroller.step != 0) {
        DPRINTF((1, "clock enabled\n"));
        set_clock(self);
    }
}

/* Temporarily disables the timer */
static void
disable_clock(ScrollerWidget self)
{
    if (self->scroller.timer != None) {
        DPRINTF((1, "clock disabled\n"));
        XtRemoveTimeOut(self->scroller.timer);
        self->scroller.timer = None;
    }
}

/* Sets the timer if the clock isn't stopped */
static void
set_clock(ScrollerWidget self)
{
    if (self->scroller.timer == None) {
        self->scroller.timer = XtAppAddTimeOut(
            XtWidgetToApplicationContext((Widget)self),
            1000L / self->scroller.frequency,
            tick, self);
    }
}

/* One interval has passed */
static void
tick(XtPointer widget, XtIntervalId *interval)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    /* Clear the timer so that set_clock() can set it again */
    ASSERT(*interval == self->scroller.timer);
    self->scroller.timer = 0;

    /* Set the clock now so that we get consistent scrolling speed */
    set_clock(self);

    /* Don't scroll if we're in the midst of a drag or if the scroller
     * is stopped */
    ASSERT(self->scroller.step != 0);
    scroll(self, self->scroller.step);
}

/* Returns the tail of the queue */
static glyph_t
get_tail(ScrollerWidget self)
{
    glyph_t glyph = self->scroller.gap->previous;

    /* Skip the left and right markers */
    while (glyph->is_expired) {
        glyph = glyph->previous;
    }

    return glyph;
}

/* Answers the width of the gap for the given scroller width and sum
  of the widths of all glyphs */
static int
gap_width(ScrollerWidget self, int last_width)
{
    return MAX((int)self->core.width - last_width,
               self->scroller.min_gap_width);
}

/*
 * Semi-private methods
 */

/* Repaints the given glyph (if visible) */
void
ScRepaintGlyph(ScrollerWidget self, glyph_t glyph)
{
    Display *display = XtDisplay((Widget)self);
    glyph_holder_t holder = self->scroller.left_holder;
    int offset = 0 - self->scroller.left_offset;
    XGCValues values;
    XRectangle bbox;

    /* Construct a clipping rectangle */
    bbox.x = 0;
    bbox.y = 0;
    bbox.width = self->core.width;
    bbox.height = self->core.height;

    /* No clip mask for the scroller */
    values.clip_mask = None;
    XChangeGC(display, self->scroller.gc, GCClipMask, &values);
    self->scroller.clip_width = 0;

    /* Go through the visible glyphs looking for the one to paint */
    while (holder != NULL) {
        if (holder->glyph == glyph) {
            if (self->scroller.use_pixmap) {
                glyph_holder_paint(
                    display, self->scroller.pixmap, self->scroller.gc,
                    holder, offset, self->scroller.font->ascent, &bbox);
                redisplay(self, NULL);
            } else {
                glyph_holder_paint(
                    display, XtWindow((Widget)self), self->scroller.gc,
                    holder, offset, self->scroller.font->ascent, &bbox);
            }
        }

        offset += holder->width;
        holder = holder->next;
    }
}

/*
 * Method Definitions
 */
static int
compute_min_gap_width(XFontStruct *font)
{
    unsigned int first = font->min_char_or_byte2;
    unsigned int last = font->max_char_or_byte2;

    /* Try to use the width of 8 'n' characters */
    if (first <= 'n' && 'n' <= last) {
        return 3 * (font->per_char + 'n' - first)->width;
    }

    /* Try 8 default character widths */
    if (first <= font->default_char && font->default_char <= last) {
        return 3 * (font->per_char + font->default_char - first)->width;
    }

    /* Ok, how about spaces? */
    if (first <= ' ' && ' ' <= last) {
        return 3 * (font->per_char + ' ' - first)->width;
    }

    /* All right, resort to 3 of some arbitrary character */
    return 3 * font->per_char->width;
}

static void
scroller_convert(Widget widget, XtPointer closure, XtPointer call_data)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    XmConvertCallbackStruct *data = (XmConvertCallbackStruct *)call_data;
    message_t message;

    /* There must be a message to copy. */
    message = self->scroller.copy_message;
    ASSERT(message != NULL);
    ASSERT(self->scroller.copy_part != MSGPART_NONE);

    /* Otherwise convert the message. */
    message_convert(widget, (XtPointer)data, self->scroller.copy_message,
                    self->scroller.copy_part);
}

/* ARGSUSED */
static void
initialize(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_holder_t holder;

    /* Try to allocate a conversion descriptor */
    self->scroller.renderer = utf8_renderer_alloc(XtDisplay(widget),
                                                  self->scroller.font,
                                                  self->scroller.code_set);
    if (self->scroller.renderer == NULL) {
        /* FIX THIS: can we fail gracefully? */
        perror("trouble");
        exit(1);
    }

    /* Record the height and width for future reference */
    self->scroller.height = self->scroller.font->ascent +
                            self->scroller.font->descent;

    /* Set the default dimensions of the widget.  These will be
     * overridden later when the widget is realized. */
    self->core.width = 400;
    self->core.height = self->scroller.height;

    /* Record the width of 8 'n' characters as the minimum gap width */
    self->scroller.min_gap_width = compute_min_gap_width(self->scroller.font);

    /* Make sure we have a height */
    if (self->core.height == 0) {
        self->core.height = self->scroller.height;
    }

    /* Allocate a glyph to represent the gap */
    self->scroller.gap = glyph_alloc(self, NULL);
    GLYPH_ALLOC_REF(self->scroller.gap, REF_GAP, self);
    self->scroller.gap->next = self->scroller.gap;
    self->scroller.gap->previous = self->scroller.gap;

    /* Allocate a glyph holder to wrap the gap */
    holder = glyph_holder_alloc(self->scroller.gap, self->core.width);

    /* Initialize the queue to only contain the gap with 0 offsets */
    self->scroller.timer = 0;
    self->scroller.is_stopped = True;
    self->scroller.is_visible = False;
    self->scroller.is_dragging = False;
    self->scroller.left_holder = holder;
    self->scroller.right_holder = holder;
    self->scroller.left_offset = 0;
    self->scroller.right_offset = 0;
    self->scroller.last_width = 0;
    self->scroller.start_drag_x = 0;
    self->scroller.last_x = 0;
    self->scroller.clip_width = 0;
    self->scroller.request_id = 0;
    self->scroller.local_delta = 0;
    self->scroller.target_delta = 0;

    self->scroller.copy_message = NULL;
    self->scroller.copy_part = MSGPART_NONE;
}

/* Realize the widget by creating a window in which to display it */
static void
realize(Widget widget,
        XtValueMask *value_mask,
        XSetWindowAttributes *attributes)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    Display *display = XtDisplay(self);
    Colormap colormap = XDefaultColormapOfScreen(XtScreen(self));
    XColor colors[5];

    /* Initialize colors */
    colors[0].pixel = self->core.background_pixel;
    colors[1].pixel = self->scroller.group_pixel;
    colors[2].pixel = self->scroller.user_pixel;
    colors[3].pixel = self->scroller.string_pixel;
    colors[4].pixel = self->scroller.separator_pixel;
    XQueryColors(display, colormap, colors, 5);

    /* Create a window and a couple of graphics contexts */
    XtCreateWindow(widget, InputOutput, CopyFromParent, *value_mask,
                   attributes);
    create_gc(self);

    if (self->scroller.use_pixmap) {
        /* Create an offscreen pixmap */
        self->scroller.pixmap = XCreatePixmap(
            XtDisplay(self), XtWindow(self),
            self->core.width, self->scroller.height,
            self->core.depth);

        /* Clear the offscreen pixmap to the background color */
        paint(self, 0, 0, self->core.width, self->core.height);
    } else {
        /* Register an event handler for GraphicsExpose events */
        XtAddEventHandler(widget, 0, True, gexpose, NULL);
    }

    /* Avoid pathological numbers of fade levels. */
    if (self -> scroller.fade_levels < 1)
    {
        self -> scroller.fade_levels = 1;
    }

    /* Allocate colors */
    self->scroller.separator_pixels = create_faded_colors(
        display, colormap, &colors[4], &colors[0], self->scroller.fade_levels);
    self->scroller.group_pixels = create_faded_colors(
        display, colormap, &colors[1], &colors[0], self->scroller.fade_levels);
    self->scroller.user_pixels = create_faded_colors(
        display, colormap, &colors[2], &colors[0], self->scroller.fade_levels);
    self->scroller.string_pixels = create_faded_colors(
        display, colormap, &colors[3], &colors[0], self->scroller.fade_levels);
}

/* Add a glyph_holder to the left edge of the Scroller */
static void
add_left_holder(ScrollerWidget self)
{
    glyph_t glyph;
    glyph_holder_t holder;
    int width;

    /* Find the first unexpired glyph to the left of the scroller */
    glyph = self->scroller.left_holder->glyph;
    glyph = glyph_get_successor(glyph)->previous;
    while (glyph->is_expired) {
        glyph = glyph->previous;
    }

    /* We need to do magic for the gap */
    if (glyph == self->scroller.gap) {
        glyph_holder_t left = self->scroller.left_holder;
        glyph_t tail = get_tail(self);

        /* Determine the width of the tail glyph */
        if (tail == self->scroller.gap) {
            width = gap_width(self, left->width);
        } else {
            width = gap_width(self, glyph_get_width(tail));
        }

        /* If the next glyph is also the gap then just expand it */
        if (left->glyph == self->scroller.gap) {
            left->width += width;
            self->scroller.left_offset += width;
            return;
        }
    } else {
        width = glyph_get_width(glyph);
    }

    /* Create a glyph holder and add it to the list */
    holder = glyph_holder_alloc(glyph, width);
    self->scroller.left_holder->previous = holder;
    holder->next = self->scroller.left_holder;
    self->scroller.left_holder = holder;

    self->scroller.left_offset += width;
}

/* Add a glyph_holder to the right edge */
static void
add_right_holder(ScrollerWidget self)
{
    glyph_t glyph;
    glyph_holder_t holder;
    int width;

    /* Find the first unexpired glyph to the right of the scroller */
    glyph = self->scroller.right_holder->glyph;
    glyph = glyph_get_successor(glyph)->next;
    while (glyph->is_expired) {
        glyph = glyph->next;
    }

    /* We need to do some magic for the gap */
    if (glyph == self->scroller.gap) {
        glyph_holder_t right = self->scroller.right_holder;
        width = gap_width(self, right->width);

        /* If the previous glyph is also the gap then just expand it */
        if (right->glyph == self->scroller.gap) {
            right->width += width;
            self->scroller.right_offset += width;
            return;
        }
    } else {
        width = glyph_get_width(glyph);
    }

    /* Create a glyph_holder and add it to the list */
    holder = glyph_holder_alloc(glyph, width);
    self->scroller.right_holder->next = holder;
    holder->previous = self->scroller.right_holder;
    self->scroller.right_holder = holder;

    self->scroller.right_offset += width;
}

/* Remove the glyph_holder at the left edge of the scroller */
static void
remove_left_holder(ScrollerWidget self)
{
    glyph_holder_t holder = self->scroller.left_holder;

    /* Clean up the linked list */
    self->scroller.left_holder = holder->next;
    self->scroller.left_holder->previous = NULL;
    self->scroller.left_offset -= holder->width;
    glyph_holder_free(holder);
}

/* Remove the glyph_holder at the right edge of the scroller */
static void
remove_right_holder(ScrollerWidget self)
{
    glyph_holder_t holder = self->scroller.right_holder;

    /* Clean up the linked list */
    self->scroller.right_holder = holder->previous;
    self->scroller.right_holder->next = NULL;
    self->scroller.right_offset -= holder->width;
    glyph_holder_free(holder);
}

/* Updates the state of the scroller after a shift of zero or more
 * pixels to the left. */
static void
adjust_left(ScrollerWidget self)
{
    int done = 0;

    /* Add and remove glyphs until we reach a stable state */
    while (!done) {
        done = 1;

        /* Add glyphs to the right if there's room */
        if (self->scroller.right_offset < 0) {
            add_right_holder(self);
            done = 0;
        }

        /* Remove glyphs from the left if they're no longer visible */
        if (self->scroller.left_offset >=
            self->scroller.left_holder->width) {
            remove_left_holder(self);
            done = 0;
        }

        /* Check for the magical stop condition */
        if (self->scroller.left_holder == self->scroller.right_holder &&
            self->scroller.left_holder->glyph == self->scroller.gap &&
            queue_is_empty(self->scroller.gap)) {
            /* Tidy up and stop */
            self->scroller.left_offset = 0;
            self->scroller.right_offset = 0;
            self->scroller.left_holder->width = self->core.width;
            self->scroller.is_stopped = True;
            disable_clock(self);
            return;
        }
    }
}

/* Updates the state of the scroller after a shift of zero or more
 * pixels to the right */
static void
adjust_right(ScrollerWidget self)
{
    int done = 0;

    /* Add and remove glyphs until we reach a stable state */
    while (!done) {
        done = 1;

        /* Add glyphs to the left it needed */
        if (self->scroller.left_offset < 0) {
            add_left_holder(self);
            done = 0;
        }

        /* Remove glyphs from the right if they're no longer visible */
        if (self->scroller.right_offset >=
            self->scroller.right_holder->width) {
            remove_right_holder(self);
            done = 0;
        }

        /* Check for the magical stop condition */
        if (self->scroller.left_holder == self->scroller.right_holder &&
            self->scroller.right_holder->glyph == self->scroller.gap &&
            queue_is_empty(self->scroller.gap)) {
            /* Tidy up and stop */
            self->scroller.left_offset = 0;
            self->scroller.right_offset = 0;
            self->scroller.right_holder->width = self->core.width;
            self->scroller.is_stopped = True;
            disable_clock(self);
            return;
        }
    }
}

/* Try to scroll to the desired position as determined by
 * target_delta.  If the X server hasn't acknowledged our last
 * CopyArea request then we leave this pending */
static void
scroll(ScrollerWidget self, int offset)
{
    Display *display = XtDisplay(self);
    int delta;

    /* FIX THIS: offsets should be positive */
    self->scroller.target_delta -= offset;

    /* Bail if we have an outstanding CopyArea request */
    if (self->scroller.local_delta != 0) {
        return;
    }

    /* Bail if we're at the target position */
    delta = self->scroller.target_delta;
    if (delta == 0) {
        return;
    }

    /* Scroll left or right as appropriate */
    self->scroller.left_offset -= delta;
    self->scroller.right_offset += delta;
    self->scroller.local_delta = delta;
    self->scroller.target_delta = 0;

    /* Update the view holders */
    if (delta < 0) {
        adjust_left(self);
    } else {
        adjust_right(self);
    }

    /* Pixmaps are easy */
    if (self->scroller.use_pixmap) {
        /* Scroll the pixmap */
        XCopyArea(display, self->scroller.pixmap, self->scroller.pixmap,
                  self->scroller.backgroundGC,
                  0, 0, self->core.width, self->core.height,
                  delta, 0);

        /* We're always in sync with the X server */
        self->scroller.local_delta = 0;

        /* Repaint the missing bits */
        paint(self,
              delta < 0 ? self->core.width + delta : 0, 0,
              delta < 0 ? -delta : delta, self->scroller.height);

        /* Copy the pixmap to the screen */
        redisplay(self, NULL);
        return;
    }

    /* If the scroller is obscured then we're done */
    if (!self->scroller.is_visible) {
        self->scroller.local_delta = 0;
        return;
    }

    /* Record the sequence number for the CopyArea request */
    self->scroller.request_id = NextRequest(display);

    /* Copy the window to itself */
    XCopyArea(display, XtWindow(self), XtWindow(self),
              self->scroller.backgroundGC,
              -delta, 0, self->core.width, self->scroller.height, 0, 0);

    /* Record that we're out of sync */
    self->scroller.local_delta = delta;
}

/* Repaints the view of each view_holder_t */
static void
paint(ScrollerWidget self,
      int x,
      int y,
      unsigned int width,
      unsigned int height)
{
    Widget widget = (Widget)self;
    Display *display = XtDisplay(widget);
    glyph_holder_t holder = self->scroller.left_holder;
    int offset = 0 - self->scroller.left_offset;
    int end = self->core.width;
    XGCValues values;
    XRectangle bbox;

    /* Compensate for unprocessed CopyArea requests */
    x += self->scroller.local_delta;

    /* Record the width an height of the bounding box */
    bbox.width = width;
    bbox.height = height;

    /* Has the width changed since our last paint call? */
    if (self->scroller.clip_width == width) {
        /* Yes.  We can reuse the clip mask and just change the origin */
        values.clip_x_origin = x;
        XChangeGC(display, self->scroller.gc, GCClipXOrigin, &values);
    } else {
        /* No.  Set up a new clip mask */
        bbox.x = 0;
        bbox.y = 0;

        /* Create the clip mask as a single rectangle */
        XSetClipRectangles(display, self->scroller.gc, x, y, &bbox,
                           1, Unsorted);
        self->scroller.clip_width = width;
    }

    /* Set the origin of the bounding box */
    bbox.x = x;
    bbox.y = y;

    /* Are we using an offscreen pixmap? */
    if (self->scroller.use_pixmap) {
        /* Reset this portion of the pixmap to the background color */
        XFillRectangle(display, self->scroller.pixmap,
                       self->scroller.backgroundGC, x, y, width, height);
    }

    /* Draw each visible glyph */
    while (offset < end) {
        if (self->scroller.use_pixmap) {
            glyph_holder_paint(display, self->scroller.pixmap,
                               self->scroller.gc, holder, offset,
                               self->scroller.font->ascent, &bbox);
        } else {
            glyph_holder_paint(display, XtWindow(self), self->scroller.gc,
                               holder, offset, self->scroller.font->ascent,
                               &bbox);
        }

        offset += holder->width;
        holder = holder->next;
    }

    /* Sanity check */
    ASSERT(offset - self->core.width == self->scroller.right_offset);
}

/* Repaints the scroller */
static void
redisplay(ScrollerWidget self, Region region)
{
    /* If we're using a pixmap then just copy it to the window */
    if (self->scroller.use_pixmap) {
        XCopyArea(XtDisplay((Widget)self), self->scroller.pixmap,
                  XtWindow((Widget)self), self->scroller.backgroundGC,
                  0, 0, self->core.width, self->scroller.height, 0, 0);
        return;
    }

    /* Repaint the region */
    if (region != NULL) {
        XRectangle rectangle;

        /* Find the smallest enclosing rectangle and repaint within it */
        XClipBox(region, &rectangle);
        paint(self, rectangle.x, 0, rectangle.width, self->scroller.height);
    }
}

/* Redisplay the portion of the scroller in the Region */
static void
expose(Widget widget, XEvent *event, Region region)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    /* The scroller is visible */
    self->scroller.is_visible = True;

    /* Should we postpone the expose event until we're ready? */
    redisplay(self, region);
}

/* Repaint the bits of the scroller that didn't get copied */
static void
gexpose(Widget widget, XtPointer rock, XEvent *event, Boolean *ignored)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    Display *display = XtDisplay(widget);
    XEvent event_buffer;

    /* Sanity check */
    ASSERT(!self->scroller.use_pixmap);

    /* Process all of the GraphicsExpose events in one go */
    for (;;) {
        XGraphicsExposeEvent *g_event;

        /* See if the server has synced with our local state */
        if (LastKnownRequestProcessed(display) >= self->scroller.request_id) {
            self->scroller.local_delta = 0;
        }

        /* Stop drawing stuff if the scroller is obscured */
        if (event->type == NoExpose) {
            self->scroller.is_visible = False;
            return;
        }

        /* Ignore events other than GraphicsExpose. */
        if (event->type != GraphicsExpose) {
            ASSERT(event->type == SelectionClear ||
                   event->type == SelectionRequest ||
                   event->type == SelectionNotify);
            return;
        }

        /* Coerce the event */
        g_event = (XGraphicsExposeEvent *)event;

        /* Update this portion of the scroller */
        paint(self, g_event->x, 0, g_event->width, self->scroller.height);

        /* Bail if this is the last GraphicsExpose event */
        if (g_event->count == 0) {
            return;
        }

        /* Otherwise grab the next GraphicsExpose event */
        XNextEvent(display, &event_buffer);
        event = &event_buffer;
    }
}

/* FIX THIS: should actually do something? */
static void
destroy(Widget widget)
{
    DPRINTF((2, "destroy %p\n", widget));
}

/* Find the empty view and update its width */
static void
resize(Widget widget)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    /* If the widget isn't realized then just update the gap size */
    if (!XtIsRealized(widget)) {
        self->scroller.left_holder->width = self->core.width;
        return;
    }

    /* If we're using an offscreen pixmap then we'll need a new one */
    if (self->scroller.use_pixmap) {
        /* Otherwise we need a new offscreen pixmap */
        XFreePixmap(XtDisplay(widget), self->scroller.pixmap);
        self->scroller.pixmap = XCreatePixmap(
            XtDisplay(widget), XtWindow(widget),
            self->core.width, self->scroller.height,
            self->core.depth);
    }

    /* If the scroller is stalled, then we simply need to expand the gap */
    if (self->scroller.is_stopped) {
        self->scroller.left_holder->width = self->core.width;
        if (self->scroller.use_pixmap) {
            paint(self, 0, 0, self->core.width, self->scroller.height);
            redisplay(self, NULL);
        }

        return;
    }

    /* Adjust the glyph_holders and offsets to compensate */
    if (self->scroller.step < 0) {
        glyph_holder_t holder = self->scroller.right_holder;
        int offset = holder->width - self->scroller.right_offset;

        /* Look for a gap (but not the leading glyph) that we can adjust */
        holder = holder->previous;
        while (holder != NULL) {
            /* Did we find one? */
            if (holder->glyph == self->scroller.gap) {
                /* Determine how wide the gap *should* be */
                if (holder->previous != NULL) {
                    holder->width = gap_width(self, holder->previous->width);
                } else {
                    glyph_t glyph = holder->glyph->previous;
                    while (glyph->is_expired) {
                        glyph = glyph->previous;
                    }

                    /* Watch for the gap */
                    if (glyph == self->scroller.gap) {
                        holder->width = self->core.width;
                    } else {
                        holder->width =
                            gap_width(self, glyph_get_width(glyph));
                    }
                }
            }

            offset += holder->width;
            holder = holder->previous;
        }

        /* Adjust the left offset and update the edges */
        self->scroller.left_offset = offset - self->core.width;
        adjust_left(self);
        adjust_right(self);
    } else {
        glyph_holder_t holder = self->scroller.left_holder;
        int offset = holder->width - self->scroller.left_offset;

        /* Look for a gap (but not the leading glyph) that we can adjust */
        holder = holder->next;
        while (holder != NULL) {
            /* Adjust the width of the gap */
            if (holder->glyph == self->scroller.gap) {
                holder->width = gap_width(self, holder->previous->width);
            }

            offset += holder->width;
            holder = holder->next;
        }

        /* Adjust the right offset and update the edges */
        self->scroller.right_offset = offset - self->core.width;
        adjust_right(self);
        adjust_left(self);
    }

    if (self->scroller.use_pixmap) {
        /* Update the display on that pixmap */
        paint(self, 0, 0, self->core.width, self->scroller.height);
        redisplay(self, NULL);
    }
}

/* What should this do? */
static Boolean
set_values(Widget current,
           Widget request,
           Widget new,
           ArgList args,
           Cardinal *num_args)
{
    DPRINTF((5, "set_values\n"));
    return False;
}

/* What should this do? */
static XtGeometryResult
query_geometry(Widget widget,
               XtWidgetGeometry *intended,
               XtWidgetGeometry *preferred)
{
    DPRINTF((5, "query_geometry\n"));
    return XtGeometryYes;
}

/* Answers the glyph_holder corresponding to the given point */
static glyph_holder_t
holder_at_point(ScrollerWidget self, int x, int y)
{
    glyph_holder_t holder;
    int offset;

    /* Points outside the scroller bounds return NULL */
    if (x < 0 || self->core.width <= x || y < 0 || self->core.height <= y) {
        DPRINTF((1, "out of bounds! (x=%d, y=%d)\n", x, y));
        return NULL;
    }

    /* Work from left-to-right looking for the glyph */
    offset = -self->scroller.left_offset;
    for (holder = self->scroller.left_holder;
         holder != NULL;
         holder = holder->next) {
        offset += holder->width;
        if (x < offset) {
            return holder;
        }
    }

    /* Didn't find the point (!) so we return NULL */
    return NULL;
}

/* Answers the glyph_holder corresponding to the location of the given event */
static glyph_holder_t
holder_at_event(ScrollerWidget self, XEvent *event)
{
    XKeyEvent *key_event;
    XButtonEvent *button_event;
    XMotionEvent *motion_event;

    switch (event->type) {
    case KeyPress:
    case KeyRelease:
        key_event = (XKeyEvent *)event;
        return holder_at_point(self, key_event->x, key_event->y);

    case ButtonPress:
    case ButtonRelease:
        button_event = (XButtonEvent *)event;
        return holder_at_point(self, button_event->x, button_event->y);

    case MotionNotify:
        motion_event = (XMotionEvent *)event;
        return holder_at_point(self, motion_event->x, motion_event->y);

    default:
        return NULL;
    }
}

/* Answers the glyph corresponding to the location of the given event */
static glyph_t
glyph_at_event(ScrollerWidget self, XEvent *event)
{
    glyph_holder_t holder = holder_at_event(self, event);

    /* If no holder found then return the gap */
    if (holder == NULL) {
        return self->scroller.gap;
    }

    return holder->glyph;
}

static void
copy_at_event(Widget widget, XEvent* event, char **params,
              Cardinal *num_params)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;
    message_t message;
    message_part_t part;
    const char* target;
    int res;

    /* Bail if there's no glyph. */
    glyph = glyph_at_event(self, event);
    if (glyph == NULL) {
        return;
    }

    /* Bail if there's no associated message. */
    message = glyph_get_message(glyph);
    if (message == NULL) {
        return;
    }

    /* Decide what part of the message to copy. */
    part = message_part_from_string(*num_params < 1 ? NULL : params[0]);

    /* Bail if the relevant part doesn't exist. */
    if (message_part_size(message, part) == 0) {
        DPRINTF((1, "Message has no %s\n", message_part_to_string(part)));
    }

    /* Record the message and part. */
    ASSERT(self->scroller.copy_message == NULL);
    self->scroller.copy_message = glyph_get_message(glyph);
    ASSERT(self->scroller.copy_part == MSGPART_NONE);
    self->scroller.copy_part = part;

    /* Decide where we're copying to. */
    target = (*num_params < 2) ? "CLIPBOARD" : params[1];
    if (strcmp(target, "PRIMARY") == 0) {
        res = XmePrimarySource(widget, 0);
    } else if (strcmp(target, "SECONDARY") == 0) {
        res = XmeSecondarySource(widget, 0);
    } else /* if (strcmp(target, "CLIPBOARD") == 0) */ {
        res = XmeClipboardSource(widget, XmCOPY, 0);
    }

    DPRINTF((2, "Xme[%s]Source: %s\n", target, res ? "true" : "false"));

    self->scroller.copy_message = NULL;
    self->scroller.copy_part = MSGPART_NONE;
}


/*
 * Action definitions
 */

/* Called when the button is pressed */
void
show_menu(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    DPRINTF((1, "show-menu()\n"));

    /* Pop up the menu and select the message that was clicked on */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(widget, self->scroller.callbacks,
                       glyph_get_message(glyph));
}

/* Spawn metamail to decode the message's attachment */
static void
show_attachment(Widget widget,
                XEvent *event,
                String *params,
                Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    DPRINTF((1, "show-attachment()\n"));

    /* Deliver the chosen message to the callbacks */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(widget, self->scroller.attachment_callbacks,
                       glyph_get_message(glyph));
}

/* Expires a message in short order */
static void
expire(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    DPRINTF((1, "expire()\n"));

    /* Expire the glyph under the pointer */
    glyph = glyph_at_event(self, event);
    glyph_expire(glyph, self);
}

/* Delete a message when scrolling left to right (backwards) */
static void
delete_left_to_right(ScrollerWidget self, glyph_t glyph)
{
    glyph_holder_t holder = self->scroller.right_holder;
    int offset = self->core.width + self->scroller.right_offset;
    int missing_width = 0;

    /* If we're deleting the leftmost glyph then we add another now.
     * We can then safely assume that the last glyph won't get deleted
     * out from under us. */
    if (self->scroller.left_holder->glyph == glyph) {
        add_left_holder(self);
        ASSERT(self->scroller.left_holder->glyph != glyph);
    }

    /* Go through the glyphs and compensate */
    while (holder != NULL) {
        /* Remember the previous glyph in case we delete the current one */
        glyph_holder_t previous = holder->previous;

        /* If we've found the gap then insert any lost width into it */
        if (holder->glyph == self->scroller.gap) {
            holder->width += missing_width;
            missing_width = 0;
        }

        /* If we've found a holder for the deleted glyph then extract it now */
        if (holder->glyph == glyph) {
            missing_width += holder->width;

            /* Previous can never be NULL */
            ASSERT(previous != NULL);
            previous->next = holder->next;

            /* Remove the holder from the list */
            if (holder->next == NULL) {
                self->scroller.right_holder = previous;
            } else {
                holder->next->previous = previous;

                /* If the glyph was surrounded by gaps then join the
                 * gaps into one */
                if (previous->glyph == self->scroller.gap &&
                    holder->next->glyph == self->scroller.gap) {
                    glyph_holder_t right_gap = holder->next;
                    glyph_holder_t left_gap = previous;

                    /* Absorb the width of the left gap into the right one */
                    offset -= left_gap->width;
                    right_gap->width += left_gap->width;

                    /* Remove the left gap from the list */
                    right_gap->previous = left_gap->previous;
                    if (left_gap->previous == NULL) {
                        self->scroller.left_holder = right_gap;
                    } else {
                        left_gap->previous->next = right_gap;
                    }

                    /* Move along */
                    previous = left_gap->previous;
                    glyph_holder_free(left_gap);
                }
            }

            /* Lose the glyph holder */
            glyph_holder_free(holder);
        } else {
            offset -= holder->width;
        }

        holder = previous;
    }

    self->scroller.left_offset = -offset;
    adjust_right(self);
}

#if 1
/* Delete a message when scrolling right to left (forwards) */
static void
delete_right_to_left(ScrollerWidget self, glyph_t glyph)
{
    glyph_holder_t holder = self->scroller.left_holder;
    int offset = -self->scroller.left_offset;
    int missing_width = 0;

    /* If we're deleting the rightmost glyph then we add another now.
     * We can then safely assume that the last glyph won't get deleted
     * out from under us. */
    if (self->scroller.right_holder->glyph == glyph) {
        add_right_holder(self);
        ASSERT(self->scroller.right_holder->glyph != glyph);
    }

    /* Go through the glyphs and compensate */
    while (holder != NULL) {
        glyph_holder_t next = holder->next;

        /* If we've found the gap then insert any lost width into it */
        if (holder->glyph == self->scroller.gap) {
            holder->width += missing_width;
            missing_width = 0;
        }

        /* If we've found a holder for the deleted glyph, then extract
         * it now */
        if (holder->glyph == glyph) {
            missing_width += holder->width;

            /* We'll always have a next glyph */
            ASSERT(next != NULL);
            next->previous = holder->previous;

            /* Remove the holder from the list */
            if (holder->previous == NULL) {
                self->scroller.left_holder = next;
            } else {
                holder->previous->next = next;

                /* If the glyph was surrounded by gaps, then join the
                 * gaps into one */
                if (next->glyph == self->scroller.gap &&
                    holder->previous->glyph == self->scroller.gap) {
                    glyph_holder_t left_gap = holder->previous;
                    glyph_holder_t right_gap = next;

                    /* Absorb the width of the right gap into the left
                     * one */
                    offset += right_gap->width;
                    left_gap->width += right_gap->width;

                    /* Remove the right gap from the list */
                    left_gap->next = right_gap->next;
                    if (right_gap->next == NULL) {
                        self->scroller.right_holder = left_gap;
                    } else {
                        right_gap->next->previous = left_gap;
                    }

                    /* Move along */
                    next = right_gap->next;
                    glyph_holder_free(right_gap);
                }
            }

            /* Lose the glyph holder */
            glyph_holder_free(holder);
        } else {
            offset += holder->width;
        }

        holder = next;
    }

    self->scroller.right_offset = offset - self->core.width;
    adjust_left(self);
}
#else
/* Deletes a message when scrolling right to left */
static void
delete_right_to_left(ScrollerWidget self, glyph_t glyph)
{
    glyph_holder_t holder = self->scroller.right_holder;
    int left_of_leading = 0;

    /* If we're deleting the rightmost glyph then we add another now
     * so that we don't lose our place in the circular queue if we
     * delete the entire contents of the scroller */
    if (self->scroller.right_holder->glyph == glyph) {
        add_right_holder(self);
        ASSERT(self->scroller.right_holder->glyph != glyph);
    }

    /* Go through the visible glyphs and remove the deleted one */
    while (holder != NULL) {
        glyph_holder_t previous = holder->previous;

        /* If this is the gap then we've seen the leading edge */
        if (holder->glyph == self->scroller.gap) {
            left_of_leading = 1;
        } else {
            /* Does the holder point to the glyph we're deleting? */
            if (holder->glyph == glyph) {
                /* If we've seen the leading edge then pull things in
                 * from the left */
                if (left_of_leading) {
                    self->scroller.left_offset -= holder->width;
                } else {
                    self->scroller.right_offset -= holder->width;
                }

                /* Remove the holder from the list */
                ASSERT(holder->next != NULL);
                holder->next->previous = previous;

                if (previous == NULL) {
                    self->scroller.left_holder = holder->next;
                } else {
                    glyph_holder_t next = holder->next;

                    /* Update the pointer */
                    previous->next = next;

                    /* If the glyph was surrounded by gaps then join
                     * them into a single big one */
                    if (next->glyph == self->scroller.gap) &&
                        (previous->glyph == self->scroller.gap) {
                        /* Remove the right gap from the list */
                        previous->next = next->next;
                        if (next->next == NULL) {
                            self->scroller.right_holder = previous;
                        } else {
                            next->next->previous = previous;
                        }

                        /* Free it */
                        glyph_holder_free(next);

                        /* Update the width of the left gap */
                        previous->width = self->core.width;
                    }
                }

                /* Free the glyph holder */
                glyph_holder_free(holder);
            }
        }

        holder = previous;
    }

    adjust_left(self);
    adjust_right(self);
}
#endif

/* Delete the given glyph from the scroller */
static void
delete_glyph(ScrollerWidget self, glyph_t glyph)
{
    /* Refuse to delete the gap */
    ASSERT(glyph != self->scroller.gap);
    if (glyph == self->scroller.gap) {
        return;
    }

    /* If the glyph isn't visible then simply remove it from the
     * queue. */
    DPRINTF((1, "[delete_glyph %c]\n",
             glyph->visible_count == 0 ? 'f' : 't'));
    if (glyph->visible_count == 0) {
        queue_remove(glyph);
        return;
    }

    /* Otherwise attempt to keep the leading edge of the text in
     * place.  Mark the glyph as deleted so that it doesn't get
     * re-added when looking for replacement glyphs. */
    glyph_delete(glyph);

    /* Fill in the space of the deleted glyph. */
    if (self->scroller.step < 0) {
        delete_left_to_right(self, glyph);
    } else {
        delete_right_to_left(self, glyph);
    }

    /* Repaint the scroller. */
    if (self->scroller.use_pixmap) {
        paint(self, 0, 0, self->core.width, self->scroller.height);
        redisplay(self, NULL);
    } else {
        XFillRectangle(XtDisplay((Widget)self), XtWindow((Widget)self),
                       self->scroller.backgroundGC,
                       0, 0, self->core.width, self->scroller.height);
        paint(self, 0, 0, self->core.width, self->scroller.height);
    }
}

/* Simply remove the message from the scroller NOW */
static void
delete(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    DPRINTF((1, "delete()\n"));

    /* Figure out which glyph to delete */
    glyph = glyph_at_event(self, event);

    /* Ignore attempts to delete the gap */
    if (glyph == self->scroller.gap) {
        return;
    }

    delete_glyph(self, glyph);
}

/* Kill a message and any replies */
static void
do_kill(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;

    DPRINTF((1, "kill()\n"));

    /* Figure out which glyph to kill */
    glyph = glyph_at_event(self, event);
    XtCallCallbackList(widget, self->scroller.kill_callbacks,
                       glyph_get_message(glyph));
}

/* Scroll more quickly */
static void
faster(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    int value = 1;

    DPRINTF((1, "faster()\n"));

    /* If there's an argument then try to convert it to a number */
    if (*nparams > 0) {
        value = atoi(params[0]);
        if (value == 0) {
            fprintf(stderr, "Unable to convert argument %s to a number\n",
                    params[0]);
            value = 1;
        }
    }

    /* Increase the step size */
    self->scroller.step += value;

    /* Make sure the clock runs if we're scrolling and not if we're
     * stationary */
    if (self->scroller.step != 0) {
        enable_clock(self);
    } else {
        disable_clock(self);
    }
}

/* Scroll more slowly */
static void
slower(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    int value = 1;

    DPRINTF((1, "slower()\n"));

    /* If there's an argument then try to convert it to a number */
    if (*nparams > 0) {
        value = atoi(params[0]);
        if (value == 0) {
            fprintf(stderr, "Unable to convert argument %s to a number\n",
                    params[0]);
            value = 1;
        }
    }

    /* Decrease the step size */
    self->scroller.step -= value;

    /* Make sure the clock runs if we're scrolling and not if we're
     * stationary */
    if (self->scroller.step != 0) {
        enable_clock(self);
    } else {
        disable_clock(self);
    }
}

/* Scroll at a specific speed */
static void
set_speed(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    /* set-speed only accepts one parameter */
    if (*nparams != 1) {
        return;
    }

    DPRINTF((1, "set-speed()\n"));

    /* Set the speed.  Disable the clock for a speed of 0 */
    self->scroller.step = atoi(params[0]);
    if (self->scroller.step == 0) {
        disable_clock(self);
        return;
    }

    /* Bail if we're being dragged */
    if (self->scroller.is_dragging) {
        return;
    }

    /* Otherwise make sure the clock is running */
    enable_clock(self);
}

/* Prepare to drag */
static void
start_drag(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;

    DPRINTF((1, "start-drag()\n"));

    /* Record the postion of the click for future reference */
    self->scroller.start_drag_x = button_event->x;
    self->scroller.last_x = button_event->x;
}

/* Someone is dragging the mouse */
static void
drag(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    XMotionEvent *motion_event = (XMotionEvent *)event;

    DPRINTF((1, "drag()\n"));

    /* Aren't we officially dragging yet? */
    if (!self->scroller.is_dragging) {
        /* Give a little leeway so that a wobbly click doesn't become
         * a drag */
        if (self->scroller.start_drag_x - self->scroller.drag_delta <
            motion_event->x &&
            motion_event->x < self->scroller.start_drag_x +
            self->scroller.drag_delta) {
            return;
        }

        /* Otherwise we're definitely dragging */
        self->scroller.is_dragging = True;

        /* Disable the timer until the drag is done */
        disable_clock(self);
    }

    /* Drag the scroller to the right place */
    scroll(self, self->scroller.last_x - motion_event->x);
    self->scroller.last_x = motion_event->x;
}

/* Stop draggin' my heart around */
static void
stop_drag(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    ScrollerWidget self = (ScrollerWidget)widget;

    DPRINTF((1, "stop-drag()\n"));

    /* If we were dragging then stop */
    if (self->scroller.is_dragging) {
        self->scroller.is_dragging = False;

        /* Set the timer so that we can start scrolling */
        enable_clock(self);
        return;
    }

    /* We weren't dragging: optionally invoke a different action.  If
     * we have parameters then invoke the first as an action and hand
     * the rest to it as parameters */
    if (*nparams != 0) {
        XtCallActionProc(widget, params[0], event, params + 1, *nparams - 1);
    }
}

/*
 * Actions table
 */
static XtActionsRec actions[] =
{
    { "start-drag", start_drag },
    { "stop-drag", stop_drag },
    { "drag", drag },
    { "show-menu", show_menu },
    { "show-attachment", show_attachment },
    { "expire", expire },
    { "delete", delete },
    { "kill", do_kill },
    { "faster", faster },
    { "slower", slower },
    { "set-speed", set_speed },
    { "copy", copy_at_event }
};

/*
 *Public methods
 */

/* Adds a message to the receiver */
void
ScAddMessage(Widget widget, message_t message)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph;
    glyph_t probe;
    glyph_holder_t holder;

    /* Create a glyph for the message */
    glyph = glyph_alloc(self, message);
    if (glyph == NULL) {
        return;
    }

    /* Try replacing an existing queue element with the same tag as this one */
    probe = queue_find(self->scroller.gap, message_get_tag(message));
    if (probe != NULL) {
        queue_replace(probe, glyph);
    } else {
        queue_add(self->scroller.gap->previous, glyph);
    }

    /* Adjust the gap width if possible and appropriate */
    holder = self->scroller.left_holder;
    if (self->scroller.step < 0 && holder->glyph == self->scroller.gap) {
        int width = gap_width(self, glyph_get_width(glyph));

        /* If the effect is invisible, then just do it */
        if (holder->width - self->scroller.left_offset < width) {
            self->scroller.left_offset -= holder->width - width;
            holder->width = width;
        } else {
            /* We have to compromise */
            holder->width -= self->scroller.left_offset;
            self->scroller.left_offset = 0;
        }
    }

    /* Make sure the clock is running */
    if (self->scroller.is_stopped) {
        self->scroller.is_stopped = False;
        enable_clock(self);
    }
}

/* Purge any killed messages */
void
ScPurgeKilled(Widget widget)
{
    ScrollerWidget self = (ScrollerWidget)widget;
    glyph_t glyph, next;

    /* Go through and delete the killed nodes */
    glyph = self->scroller.gap->next;
    while (glyph != self->scroller.gap) {
        next = glyph->next;

        /* Get the glyph's message */
        if (glyph_is_killed(glyph)) {
            delete_glyph(self, glyph);
        }

        glyph = next;
    }
}

/* Callback for expiring glyphs */
void
ScGlyphExpired(ScrollerWidget self, glyph_t glyph)
{
    DPRINTF((1, "[ScGlyphExpired %c]\n",
             glyph->visible_count == 0 ? 'f' : 't'));

    /* Dequeue the glyph if it's not visible */
    if (glyph->visible_count == 0) {
        queue_remove(glyph);
    }
}

/* Transfer traits. */
static XmTransferTraitRec transfer_traits = {
    0, scroller_convert, NULL, NULL
};

static void
class_part_init(WidgetClass wclass)
{
    /* Initialize data transfer traits. */
    XmeTraitSet(wclass, XmQTtransfer, (XtPointer)&transfer_traits);
}

/*
 * Class record initialization
 */
ScrollerClassRec scrollerClassRec =
{
    /* core_class fields */
    {
        (WidgetClass)&xmPrimitiveClassRec, /* superclass */
        "Scroller", /* class_name */
        sizeof(ScrollerRec), /* widget_size */
        NULL, /* class_initialize */
        class_part_init, /* class_part_initialize */
        False, /* class_inited */
        initialize, /* initialize */
        NULL, /* initialize_hook */
        realize, /* realize */
        actions, /* actions */
        XtNumber(actions), /* num_actions */
        resources, /* resources */
        XtNumber(resources), /* num_resources */
        NULLQUARK, /* xrm_class */
        True, /* compress_motion */
        True, /* compress_exposure */
        True, /* compress_enterleave */
        False, /* visible_interest */
        destroy, /* destroy */
        resize, /* resize */
        expose, /* expose */
        set_values, /* set_values */
        NULL, /* set_values_hook */
        XtInheritSetValuesAlmost, /* set_values_almost */
        NULL, /* get_values_hook */
        NULL, /* accept_focus */
        XtVersion, /* version */
        NULL, /* callback_private */
        NULL, /* tm_table */
        query_geometry, /* query_geometry */
        XtInheritDisplayAccelerator, /* display_accelerator */
        NULL /* extension */
    },

    /* Primitive class fields initialization */
    {
        NULL, /* border_highlight */
        NULL, /* border_unhighlight */
        NULL, /* translations */
        NULL, /* arm_and_activate_proc */
        NULL, /* synthetic resources */
        0, /* num syn res */
        NULL, /* extension */
    },

    /* Scroller class fields initialization */
    {
        0 /* ignored */
    }
};

WidgetClass scrollerWidgetClass = (WidgetClass)&scrollerClassRec;

/**********************************************************************/
