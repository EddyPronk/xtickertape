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
#include <stdio.h> /* printf */
#ifdef HAVE_STDLIB_H
# include <stdlib.h> /* calloc, free, malloc */
#endif
#ifdef HAVE_STRING_H
# include <string.h> /* memset, strcmp */
#endif
#ifdef HAVE_ASSERT_H
# include <assert.h> /* assert */
#endif
#include <X11/Xlib.h>
#include <X11/IntrinsicP.h>
#include <X11/Xresource.h>
#include <X11/StringDefs.h>
#include <X11/CoreP.h>
#include <X11/Xaw/SimpleP.h>
#include <Xm/XmAll.h>
#include <Xm/PrimitiveP.h>
#include <Xm/TransferT.h>
#include <Xm/TraitP.h>
#include "replace.h"
#include "globals.h"
#include "utils.h"
#include "message.h"
#include "utf8.h"
#include "message_view.h"
#include "History.h"
#include "HistoryP.h"


/*
 *
 * Resources
 *
 */

#define offset(field) XtOffsetOf(HistoryRec, field)
static XtResource resources[] =
{
    /* XtCallbackProc callback */
    {
        XtNcallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
        offset(history.callbacks), XtRPointer, (XtPointer)NULL
    },

    /* XtCallbackList attachment_callbacks */
    {
        XtNattachmentCallback, XtCCallback, XtRCallback,
        sizeof(XtCallbackList),
        offset(history.attachment_callbacks), XtRPointer, (XtPointer)NULL
    },

    /* XtCallbackList motion_callbacks */
    {
        XtNmotionCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
        offset(history.motion_callbacks), XtRPointer, (XtPointer)NULL
    },

    /* XFontStruct *font */
    {
        XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct *),
        offset(history.font), XtRString, XtDefaultFont
    },

    /* The font's code set */
    {
        XtNfontCodeSet, XtCString, XtRString, sizeof(char *),
        offset(history.code_set), XtRString, (XtPointer)NULL
    },

    /* Pixel timestamp_pixel */
    {
        XtNtimestampPixel, XtCTimestampPixel, XtRPixel, sizeof(Pixel),
        offset(history.timestamp_pixel), XtRString, "Black"
    },

    /* Pixel group_pixel */
    {
        XtNgroupPixel, XtCGroupPixel, XtRPixel, sizeof(Pixel),
        offset(history.group_pixel), XtRString, "Blue"
    },

    /* Pixel user_pixel */
    {
        XtNuserPixel, XtCUserPixel, XtRPixel, sizeof(Pixel),
        offset(history.user_pixel), XtRString, "Green"
    },

    /* Pixel string_pixel */
    {
        XtNstringPixel, XtCStringPixel, XtRPixel, sizeof(Pixel),
        offset(history.string_pixel), XtRString, "Red"
    },

    /* Pixel separator_pixel */
    {
        XtNseparatorPixel, XtCSeparatorPixel, XtRPixel, sizeof(Pixel),
        offset(history.separator_pixel), XtRString, XtDefaultForeground
    },

    /* Dimension margin_width */
    {
        XtNmarginWidth, XtCMarginWidth, XtRDimension, sizeof(Dimension),
        offset(history.margin_width), XtRImmediate, (XtPointer)5
    },

    /* Dimension margin_height */
    {
        XtNmarginHeight, XtCMarginHeight, XtRDimension, sizeof(Dimension),
        offset(history.margin_height), XtRImmediate, (XtPointer)5
    },

    /* unsigned int message_capacity */
    {
        XtNmessageCapacity, XtCMessageCapacity, XtRDimension,
        sizeof(unsigned int),
        offset(history.message_capacity), XtRImmediate, (XtPointer)32
    },

    /* Pixel selection_pixel */
    {
        XtNselectionPixel, XtCSelectionPixel, XtRPixel, sizeof(Pixel),
        offset(history.selection_pixel), XtRString, XtDefaultForeground
    },

    /* int drag_delay */
    {
        XtNdragDelay, XtCDragDelay, XtRInt, sizeof(int),
        offset(history.drag_delay), XtRImmediate, (XtPointer)100
    }
};
#undef offset


/* We try to be efficient and use XCopyArea to scroll the history
 * widget, relying on GraphicsExpose events t tell us which parts of
 * the window to repaint afterwards.  Because of the asynchronous
 * nature of this, it's possible that the area described in a
 * GraphicsExpose event will be shifted by an XCopyArea before we have
 * a chance to repaint the damaged region.  To compensate, we record
 * each pending XCopyArea request in a queue and use this information
 * to compensate. */
struct translation_queue {
    /* The next item in the queue */
    translation_queue_t next;

    /* The sequence number of the XCopyArea request */
    unsigned long request_id;

    /* The X coordinate of the left edge of the XCopyArea request */
    int left;

    /* The Y coordinate of the left edge of the XCopyArea request */
    int top;

    /* The X coordinate of the right edge of XCopyArea request */
    int right;

    /* The Y coordinate of the bottom edge of the XCopyArea request */
    int bottom;

    /* The displacement in the X direction (positive is to the right) */
    int dx;

    /* The displacement in the Y direction (positive is down) */
    int dy;
};

#if defined(DEBUG_MESSAGE)
static const char *ref_node = "node";
static const char *ref_selection = "selection";
static const char *ref_history = "history";
static const char *ref_copy = "copy";
#endif /* DEBUG_MESSAGE */

/* Allocates a translation queue item */
static translation_queue_t
translation_queue_alloc(unsigned long request_id,
                        int src_x,
                        int src_y,
                        unsigned int width,
                        unsigned int height,
                        int dest_x,
                        int dest_y)
{
    translation_queue_t self;

    /* Allocate memory for the queue item */
    self = malloc(sizeof(struct translation_queue));
    if (self == NULL) {
        return NULL;
    }

    /* Record the information */
    self->next = NULL;
    self->request_id = request_id;
    self->left = src_x;
    self->right = src_x + width;
    self->top = src_y;
    self->bottom = src_y + height;
    self->dx = dest_x - src_x;
    self->dy = dest_y - src_y;
    return self;
}

/* We store the history of messages as a tree, according to
 * message-ids and in-reply-to ids */
struct node {
    /* The message recorded by the node */
    message_t message;

    /* The node's youngest child */
    node_t child;

    /* The node's youngest elder sibling */
    node_t sibling;
};

/* Allocates and returns a new node */
static node_t
node_alloc(message_t message)
{
    node_t self;

    /* Allocate memory for the node */
    self = malloc(sizeof(struct node));
    if (self == NULL) {
        return NULL;
    }

    /* Initialize its fields to sane values */
    memset(self, 0, sizeof(struct node));

    /* Record the message */
    self->message = message;
    MESSAGE_ALLOC_REF(message, ref_node, self);
    return self;
}

/* Frees a node */
static void
node_free(node_t self)
{
    /* Make sure we decrement the message's reference count */
    if (self->message != NULL) {
        MESSAGE_FREE_REF(self->message, ref_node, self);
        self->message = NULL;
    }

    DPRINTF((5, "node_free(): %p\n", self));
    free(self);
}

/* Returns the id of the node's message */
static const char *
node_get_id(node_t self)
{
    return message_get_id(self->message);
}

/* Add a node to the tree.  WARNING: this is not a friendly function.
 * To avoid having to traverse the tree more than once when adding a
 * node, this function both finds a parent and adds a child to it and
 * also discards any nodes which no longer have visible children.
 *
 * self
 *    The pointer to the root node of the tree.  This will be updated,
 *    so it should really be the official pointer to the tree.
 *
 * parent_id
 *    The Message-Id of the parent of the node to add.
 *
 * child
 *    The child node to be added.  This will be set to NULL, so don't
 *    use a pointer you want to use later.
 *
 * index
 *    The display index of the root node.  This should initially be
 *    one less than the number of visible nodes.  Note that this value
 *    will be overwritten, so don't point to a value you wish to keep.
 *
 * index_out
 *    Will be set to the index of the newly added node.
 *
 * depth
 *    The depth (indentation level) of self.  Should initially be 0.
 *
 * depth_out
 *    Will be set to the depth of the newly added node.
 */
static void
node_add1(node_t *self,
          const char *parent_id,
          node_t *child,
          int *index,
          int *index_out,
          int depth,
          int *depth_out)
{
    /* Traverse all of our siblings */
    while (*self != NULL) {
        const char *id;

        /* If no match yet, then check for one here */
        if (parent_id && *child != NULL &&
            (id = node_get_id(*self)) != NULL &&
            strcmp(id, parent_id) == 0) {
            /* Match!  Add the child to this node */
            (*child)->sibling = (*self)->child;
            (*self)->child = *child;
            *child = NULL;

            /* Kill the child node it its parent was killed */
            if (message_is_killed((*self)->message)) {
                message_set_killed((*self)->child->message, True);
            }

            /* Record output information */
            *index_out = *index;
            *depth_out = depth + 1;

            /* The next node we visit will be the new child.  We don't
             * want it to update the index to be consistent with the
             * case where we can't find its parent, so we increment
             * the index here to compensate for it being decremented
             * when we traverse the children next. */
            (*index)++;
        }

        /* Check with the descendents */
        if ((*self)->child != NULL) {
            node_add1(&(*self)->child,
                      parent_id, child,
                      index, index_out,
                      depth + 1, depth_out);
        }

        /* We've traversed a node */
        (*index)--;

        /* If this node has scrolled off the top of the history and
         * has no children then we can safely remove it from the tree.
         * Note: this check is simplistic, but it will work since the
         * new node must always be added after any nodes that have
         * scrolled off the top. */
        if (*index < 0 && (*self)->child == NULL) {
            /* If the above statement is true, then it follows that
             * the node may not have any elder siblings.  Hence this
             * is a good sanity check. */
            ASSERT((*self)->sibling == NULL);

            /* Free the node */
            node_free(*self);
            *self = NULL;
            return;
        }

        /* The pointer we want to update if this node disappears */
        self = &(*self)->sibling;
    }
}

/* Add a node to the tree.  This is just a friendly wrapper around
 * node_add1() above.
 *
 * self
 *    The pointer to the variable holding the root of the tree.  This
 *    will commonly be modified, so use the actual pointer.
 *
 * parent_id
 *    The Message-Id of the parent node; the In-Reply-To field.
 *
 * child
 *    The node to be added to the tree.
 *
 * count
 *    The number of nodes which should be visible in the tree view
 *    once the child is added to the tree.
 *
 * index_out
 *    Will be set to the index of the newly added node.
 *
 * depth_out
 *    Will be set to the depth of the newly added node.
 */
static void
node_add(node_t *self,
         const char *parent_id,
         node_t child,
         int count,
         int *index_out,
         int *depth_out)
{
    int index;

    /* The last index will be count - 1 */
    index = count - 1;

    /* Trim the older nodes in the tree, and add the node if its
     * parent is in the tree. */
    node_add1(self, parent_id, &child, &index, index_out, 0, depth_out);

    /* Add the child now if it didn't get added already */
    if (child != NULL) {
        /* Add the node to the tree */
        child->sibling = *self;
        (*self) = child;
        *index_out = count - 1;
        *depth_out = 0;
    }
}

/* Finds the node which wraps message */
node_t
node_find(node_t self, message_t message)
{
    node_t result;

    /* Traverse the siblings until an answer is found */
    while (self != NULL) {
        /* Is it this node? */
        if (self->message == message) {
            return self;
        }

        /* Is it one of our children? */
        if (self->child) {
            result = node_find(self->child, message);
            if (result != NULL) {
                return result;
            }
        }

        /* Try the next sibling */
        self = self->sibling;
    }

    /* Not here */
    return NULL;
}

/* Kills a node and its children */
void
node_kill(node_t self)
{
    node_t child;

    /* Sanity check */
    ASSERT(self != NULL);

    /* If the node has already been killed then don't kill it again */
    if (message_is_killed(self->message)) {
        return;
    }

    /* Mark the node as killed */
    message_set_killed(self->message, True);

    /* Kill its children */
    for (child = self->child; child != NULL; child = child->sibling) {
        node_kill(child);
    }
}

#if defined(DEBUG) && 0
static void
node_dump(node_t self, int depth)
{
    int i;

    /* Nothing to print... */
    if (self == NULL) {
        return;
    }

    /* Print our siblings */
    node_dump(self->sibling, depth);

    /* Indent */
    for (i = 0; i < depth; i++) {
        printf("  ");
    }

    /* Print this node's message */
    printf("%p: %s\n", self, message_get_string(self->message));

    /* Print our children */
    node_dump(self->child, depth + 1);
}
#endif


/* Populate an array with message views */
static void
node_populate(node_t self,
              utf8_renderer_t renderer,
              message_view_t *array,
              int depth,
              int *index,
              message_t selection,
              unsigned int *selection_index_out)
{
    /* Go through each of our siblings */
    while (self != NULL) {
        /* Add the children first */
        if (self->child) {
            node_populate(self->child, renderer, array, depth + 1,
                          index, selection, selection_index_out);
        }

        /* Bail if the index goes negative */
        if (*index < 0) {
            return;
        }

        /* Is this the selection? */
        if (self->message == selection) {
            *selection_index_out = *index;
        }

        /* Wrap the message in a message view */
        /* FIX THIS: use a real conversion descriptor */
        array[(*index)--] = message_view_alloc(self->message, depth,
                                               renderer);

        /* Move on to the next node */
        self = self->sibling;
    }
}

/*
 *
 * Private Methods
 *
 */

/* Copy one region of the screen to another.  This uses XCopyArea to
 * perform the actual copying, and records information in the
 * translation queue so that GraphicsExpose events can be translated
 * to the new coordinates */
static void
copy_area(HistoryWidget self,
          Display *display,
          Drawable window,
          GC gc,
          int src_x,
          int src_y,
          unsigned int width,
          unsigned int height,
          int dest_x,
          int dest_y)
{
    translation_queue_t item;

    /* Allocate a new translation queue item */
    item = translation_queue_alloc(NextRequest(display), src_x, src_y,
                                   width, height, dest_x, dest_y);
    if (item == NULL) {
        abort();
    }

    /* Append the item to the translation queue */
    if (self->history.tqueue_end == NULL) {
        ASSERT(self->history.tqueue == NULL);
        self->history.tqueue = item;
        DPRINTF((5, "first: %lu\n", item->request_id));
    } else {
        ASSERT(self->history.tqueue != NULL);
        self->history.tqueue_end->next = item;
        DPRINTF((5, "added: %lu\n", item->request_id));
    }

    self->history.tqueue_end = item;

    /* Make the request */
    XCopyArea(display, window, window, gc, src_x, src_y, width, height,
              dest_x, dest_y);
}

/* Translate the bounding box to compensate for unprocessed CopyArea
 * requests */
static void
compensate_bbox(HistoryWidget self,
                unsigned long request_id,
                XRectangle *bbox)
{
    translation_queue_t item;
    int left = bbox->x;
    int right = left + bbox->width;
    int top = bbox->y;
    int bottom = top + bbox->height;

    DPRINTF((5, "compensate_bbox() %lu\n", request_id));

    /* Go through each outstanding delta item */
    item = self->history.tqueue;
    while (item != NULL) {
        /* Has the request been processed? */
        if (item->request_id <= request_id) {
            DPRINTF((5, "removing item %lu\n", item->request_id));

            /* Yes.  We no longer require it */
            ASSERT(self->history.tqueue == item);
            self->history.tqueue = item->next;
            free(item);
            item = self->history.tqueue;
            continue;
        }

        DPRINTF((5, "compensating for item %lu; %ux%u+%d+%d to %d,%d\n",
                 item->request_id,
                 item->right - item->left, item->bottom - item->top,
                 item->left, item->top,
                 item->left + item->dx, item->top + item->dy));

        /* Adjust the x coordinate and width (there are 6 cases) */
        if (item->left < left) {
            if (item->right < left) {
                /* nothing */
            } else if (item->right < right) {
                left += item->dx;
            } else {
                left += item->dx;
                right += item->dx;
            }
        } else if (item->left < right) {
            if (item->right < right) {
                left = MIN(left, item->left + item->dx);
                right = MAX(right, item->right + item->dx);
            } else {
                right += item->dx;
            }
        } else {
            /* nothing */
        }

        /* Adjust the y coordinate and height (same 6 cases) */
        if (item->top < top) {
            if (item->bottom < top) {
                /* nothing */
            } else if (item->bottom < bottom) {
                top += item->dy;
            } else {
                top += item->dy;
                bottom += item->dy;
            }
        } else if (item->top < bottom) {
            if (item->bottom < bottom) {
                top = MIN(top, item->top + item->dy);
                bottom = MAX(bottom, item->bottom + item->dy);
            } else {
                bottom += item->dy;
            }
        } else {
            /* nothing */
        }

        item = item->next;
    }

    /* Tidy up if the queue is empty */
    if (self->history.tqueue == NULL) {
        DPRINTF((5, "empty!\n"));
        self->history.tqueue_end = NULL;
    }

    /* Update the bbox */
    bbox->x = left;
    bbox->width = right - left;
    bbox->y = top;
    bbox->height = bottom - top;
}

/* Sets the origin of the visible portion of the widget */
static void
set_origin(HistoryWidget self, long x, long y, int update_scrollbars)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self->history.gc;
    XGCValues values;

    DPRINTF((5, "History.set_origin(x=%ld, y=%ld, update_scrollbars=%d)\n",
	     x, y, update_scrollbars));

    /* Skip this part if we're not visible */
    if (gc != None) {
        /* Remove our clip mask */
        values.clip_mask = None;
        XChangeGC(display, gc, GCClipMask, &values);

        /* Copy the area to its new location */
        copy_area(
            self, display, window, gc,
            x - self->history.x, y - self->history.y,
            self->core.width, self->core.height,
            0, 0);
    }

    /* Update the scrollbars */
    if (update_scrollbars) {
        /* Have we moved horizontally? */
        if (self -> history.x != x) {
            XtVaSetValues(self -> history.hscrollbar, XmNvalue, (int)x, NULL);
        }

        /* Have we moved vertically? */
        if (self -> history.y != y) {
            XtVaSetValues(self -> history.vscrollbar, XmNvalue, (int)y, NULL);
        }
    }

    /* Record our new location */
    self->history.x = x;
    self->history.y = y;
}

/* Callback for the vertical scrollbar */
static void
vert_scrollbar_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, self->history.x, cbs->value, 0);
}

/* Callback for the horizontal scrollbar */
static void
horiz_scrollbar_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)client_data;
    XmScrollBarCallbackStruct *cbs = (XmScrollBarCallbackStruct *)call_data;

    /* Drag to the new location */
    set_origin(self, cbs->value, self->history.y, 0);
}

static void
history_convert(Widget widget, XtPointer closure, XtPointer call_data)
{
    HistoryWidget self = (HistoryWidget)widget;
    XmConvertCallbackStruct *data = (XmConvertCallbackStruct *)call_data;
    message_t message;

    /* There must be a message to copy. */
    message = self->history.copy_message;
    ASSERT(message != NULL);
    ASSERT(self->history.copy_part != MSGPART_NONE);

    /* Otherwise convert the message. */
    message_convert(widget, (XtPointer)data, self->history.copy_message,
                    self->history.copy_part);
}

/* Initializes the History-related stuff */
/* ARGSUSED */
static void
init(Widget request, Widget widget, ArgList args, Cardinal *num_args)
{
    HistoryWidget self = (HistoryWidget)widget;
    Widget scrollbar;

    DPRINTF((5, "History.init()\n"));

    /* No GC to start with */
    self->history.gc = None;

    /* Allocate a conversion descriptor */
    self->history.renderer = utf8_renderer_alloc(XtDisplay(widget),
                                                 self->history.font,
                                                 self->history.code_set);
    if (self->history.renderer == NULL) {
        perror("trouble");
        exit(1);
    }

    /* Set the initial width/height if none are supplied */
    self->core.width = 400;
    self->core.height = 80;

    /* Set our initial dimensions to be the margin width */
    self->history.width = (long)self->history.margin_width * 2;
    self->history.height = (long)self->history.margin_height * 2;

    /* Compute the line height */
    self->history.line_height = (long)self->history.font->ascent +
        (long)self->history.font->descent + 1;

    /* Initialize the x and y coordinates of the visible region */
    self->history.x = 0;
    self->history.y = 0;
    self->history.pointer_x = 0;
    self->history.pointer_y = 0;

    /* Start with an empty delta queue */
    self->history.tqueue = NULL;
    self->history.tqueue_end = NULL;

    /* Assume we're threaded */
    self->history.is_threaded = True;

    /* We don't have any nodes yet */
    self->history.nodes = NULL;

    /* Allocate enough room for all of our message views */
    self->history.message_capacity = MAX(self->history.message_capacity, 1);
    self->history.messages = calloc(self->history.message_capacity,
                                    sizeof(message_t));
    self->history.message_count = 0;
    self->history.message_index = 0;
    self->history.message_views = calloc(self->history.message_capacity,
                                         sizeof(message_view_t));

    /* Nothing is selected yet */
    self->history.selection = NULL;
    self->history.selection_index = (unsigned int)-1;
    self->history.drag_timeout = None;
    self->history.drag_direction = DRAG_NONE;
    self->history.show_timestamps = False;

    /* Nothing has been copied yet. */
    self->history.copy_message = NULL;
    self->history.copy_part = MSGPART_NONE;

    /* Create the horizontal scrollbar */
    scrollbar = XtVaCreateManagedWidget("HorizScrollBar",
                                        xmScrollBarWidgetClass, XtParent(self),
                                        XmNincrement,
                                        self->history.line_height,
                                        XmNmaximum, self->history.width,
                                        XmNminimum, 0,
                                        XmNorientation, XmHORIZONTAL,
                                        XmNpageIncrement, self->core.width,
                                        XmNsliderSize, self->history.width,
                                        NULL);

    /* Add a bunch of callbacks */
    XtAddCallback(scrollbar, XmNdragCallback, horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNdecrementCallback, horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback,
                  horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback,
                  horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, horiz_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback,
                  horiz_scrollbar_cb, self);
    self->history.hscrollbar = scrollbar;

    /* Create the vertical scrollbar */
    scrollbar = XtVaCreateManagedWidget(
        "VertScrollBar", xmScrollBarWidgetClass,
        XtParent(self),
        XmNincrement, self->history.line_height,
        XmNmaximum, self->history.height,
        XmNminimum, 0,
        XmNorientation, XmVERTICAL,
        XmNpageIncrement, self->core.height,
        XmNsliderSize, self->history.height,
        NULL);

    /* Add a bunch of callbacks */
    XtAddCallback(scrollbar, XmNdecrementCallback, vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNdragCallback, vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNincrementCallback, vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNpageDecrementCallback,
                  vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNpageIncrementCallback,
                  vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNtoTopCallback, vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNtoBottomCallback, vert_scrollbar_cb, self);
    XtAddCallback(scrollbar, XmNvalueChangedCallback, vert_scrollbar_cb, self);
    self->history.vscrollbar = scrollbar;
}

/* Transfer traits */
static XmTransferTraitRec transfer_traits = {
    0, history_convert, NULL, NULL
};

static void
class_part_init(WidgetClass wclass)
{
    /* Install data transfer traits. */
    XmeTraitSet(wclass, XmQTtransfer, (XtPointer)&transfer_traits);
}

/* Returns the message index corresponding to the given y coordinate */
static unsigned int
index_of_y(HistoryWidget self, long y)
{
    unsigned int index;

    /* Is y in the top margin? */
    if (self->history.y + y < self->history.margin_height) {
        /* Pretend its in the first message */
        index = 0;
    } else {
        /* Otherwise compute the position */
        index = (self->history.y + y - self->history.margin_height) /
                self->history.line_height;
    }

    return index;
}

/* Updates the scrollbars after either the widget or the data it is
 * displaying is resized */
static void
update_scrollbars(Widget widget, long *x_out, long *y_out)
{
    HistoryWidget self = (HistoryWidget)widget;
    Dimension width, height;
    long x, y;

    /* Figure out how big the widget is now */
    XtVaGetValues(widget, XmNwidth, &width, XmNheight, &height, NULL);

    /* The slider can't be wider than the total */
    if (self->history.width < width) {
        width = self->history.width;
    }

    /* Keep the right edge sane */
    x = self->history.x;
    if (self->history.width - width < x) {
        x = self->history.width - width;
    }

    /* The slider can't be taller than the total */
    if (self->history.height < height) {
        height = self->history.height;
    }

    /* Keep the bottom edge sane */
    y = self->history.y;
    if (self->history.height - height < y) {
        y = self->history.height - height;
    }

    /* Update the horizontal scrollbar */
    XtVaSetValues(self->history.hscrollbar, XmNvalue, x,
                  XmNsliderSize, width, XmNmaximum, self->history.width,
                  NULL);

    /* Update the vertical scrollbar */
    XtVaSetValues(self->history.vscrollbar, XmNvalue, y,
                  XmNsliderSize, height, XmNmaximum, self->history.height,
                  NULL);

    /* Return the resulting x and y values */
    *x_out = x;
    *y_out = y;
}

/* Draws the highlight */
static void
paint_highlight(HistoryWidget self)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self->history.gc;
    XGCValues values;
    XSegment segments[4];
    long y;
    int i;

    /* Bail out if we're not highlighted */
    if (!self->primitive.highlighted) {
        return;
    }

    /* Determine the location of the selection */
    y = self->history.selection_index * self->history.line_height -
        self->history.y + self->history.margin_height;

    /* Bail if it isn't visible */
    if (y + self->history.line_height < 0 &&
        (long)self->history.height <= y) {
        return;
    }

    /* We'll always draw the top and bottom segments */
    i = 0;
    segments[i].x1 = 0;
    segments[i].y1 = y;
    segments[i].x2 = self->core.width - 1;
    segments[i].y2 = y;
    i++;

    segments[i].x1 = 0;
    segments[i].y1 = y + self->history.line_height - 1;
    segments[i].x2 = self->core.width - 1;
    segments[i].y2 = y + self->history.line_height - 1;
    i++;

    /* Draw the left edge iff the we're scrolled all the way to the left */
    if (self->history.x == 0) {
        segments[i].x1 = 0;
        segments[i].y1 = y;
        segments[i].x2 = 0;
        segments[i].y2 = y + self->history.line_height - 1;
        i++;
    }

    /* Draw the right edge iff we're scrolled all the way to the right */
    if (self->history.x + self->core.width >= self->history.width) {
        segments[i].x1 = self->core.width - 1;
        segments[i].y1 = y;
        segments[i].x2 = self->core.width - 1;
        segments[i].y2 = y + self->history.line_height - 1;
        i++;
    }

    /* Set up the graphics context */
    values.foreground = self->primitive.highlight_color;
    XChangeGC(display, gc, GCForeground, &values);

    /* Draw the segments */
    XDrawSegments(display, window, gc, segments, i);
}

/* Repaint the widget */
static void
paint(HistoryWidget self, XRectangle *bbox)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self->history.gc;
    long xmargin = (long)self->history.margin_width;
    long ymargin = (long)self->history.margin_height;
    int show_timestamps = self->history.show_timestamps;
    message_view_t view;
    XGCValues values;
    unsigned int index;
    long x, y;

    /* Set that as our bounding box */
    XSetClipRectangles(display, gc, 0, 0, bbox, 1, YXSorted);

    /* Compute our X coordinate */
    x = xmargin - self->history.x;

    /* Is the top margin visible? */
    if (self->history.y < self->history.margin_height) {
        /* Yes.  Start drawing at index 0 */
        index = 0;
        y = ymargin - self->history.y;
    } else {
        /* No.  Compute the first visible index */
        index = (self->history.y - self->history.margin_height) /
                self->history.line_height;
        y = (ymargin - self->history.y) % self->history.line_height;
    }

    /* Draw all visible message views */
    while (index < self->history.message_count) {
        /* Stop if we run out of message views. */
        view = self->history.message_views[index++];
        if (view == NULL) {
            return;
        }

        /* Is this the selected message? */
        if (message_view_get_message(view) == self->history.selection) {
            /* Yes, draw a background for it */
            values.foreground = self->history.selection_pixel;
            XChangeGC(display, gc, GCForeground, &values);

            /* FIX THIS: should we respect the margin? */
            XFillRectangle(display, window, gc, 0, y,
                           self->core.width, self->history.line_height);

            /* Draw the highlight */
            paint_highlight(self);
        }

        /* Draw the view */
        message_view_paint(
            view, display, window, gc,
            show_timestamps, self->history.timestamp_pixel,
            self->history.group_pixel, self->history.user_pixel,
            self->history.string_pixel, self->history.separator_pixel,
            x, y + self->history.font->ascent, bbox);

        /* Get ready to draw the next one */
        y += self->history.line_height;

        /* Bail out if the next line is past the end of the screen */
        if (y >= self->core.height) {
            return;
        }
    }
}

/* Redisplay the given region */
static void
redisplay(Widget widget, XEvent *event, Region region)
{
    HistoryWidget self = (HistoryWidget)widget;
    XExposeEvent *x_event = (XExposeEvent *)event;
    XRectangle bbox;

    /* Sanity check */
    if (!region) {
        DPRINTF((5, "no region\n"));
        return;
    }

    /* Find the smallest rectangle which contains the region */
    XClipBox(region, &bbox);

    /* Compensate for unprocessed CopyArea requests */
    DPRINTF((5, "redisplay() request_id=%lu\n", x_event->serial));
    DPRINTF((5, "before: %dx%d+%d+%d\n",
             bbox.width, bbox.height, bbox.x, bbox.y));
    compensate_bbox(self, x_event->serial, &bbox);
    DPRINTF((5, "after: %dx%d+%d+%d\n",
             bbox.width, bbox.height, bbox.x, bbox.y));

    /* Repaint the region */
    paint(self, &bbox);
}

/* Redraw the item at the given index */
static void
redisplay_index(HistoryWidget self, unsigned int index)
{
    XRectangle bbox;
    long y;

    /* Bail if the index is out of range */
    if (index >= self->history.message_count) {
        return;
    }

    /* Figure out where the message is */
    y = index * self->history.line_height -
        self->history.y + self->history.margin_height;

    /* Create a bounding box for the message */
    bbox.x = 0;
    bbox.y = y;
    bbox.width = self->core.width;
    bbox.height = self->history.line_height;

    /* Paint it */
    paint(self, &bbox);
}

/* This is called in response to pointer motion */
static void
motion_cb(Widget widget, XtPointer closure, XEvent *event,
          Boolean *continue_to_dispatch)
{
    HistoryWidget self = (HistoryWidget)widget;
    XMotionEvent *mevent;
    unsigned int index;
    message_view_t view;

    /* Sanity check */
    ASSERT(event->type == MotionNotify);
    mevent = (XMotionEvent *)event;

    /* Assume that nothing is under the pointer */
    view = NULL;

    /* Make sure the pointer is within the bounds of the widget */
    if (0 <= mevent->y && mevent->y < self->core.height) {
        index = index_of_y(self, mevent->y);

        /* Make sure it's over a message */
        if (index < self->history.message_count) {
            view = self->history.message_views[index];
        }
    }

    /* Call the callbacks */
    XtCallCallbackList(widget, self->history.motion_callbacks,
                       view ? message_view_get_message(view) : NULL);
}

/* Repaint the bits of the widget that didn't get copied */
static Boolean
gexpose(HistoryWidget self, XGraphicsExposeEvent *event)
{
    XRectangle bbox;

    /* Get the bounding box of the event. */
    bbox.x = event->x;
    bbox.y = event->y;
    bbox.width = event->width;
    bbox.height = event->height;

    /* Compensate for unprocessed CopyArea requests */
    DPRINTF((5, "process_events() request_id=%lu\n", event->serial));
    DPRINTF((5, "before: %dx%d+%d+%d\n",
             bbox.width, bbox.height,
             bbox.x, bbox.y));
    compensate_bbox(self, event->serial, &bbox);
    DPRINTF((5, "after: %dx%d+%d+%d\n",
             bbox.width, bbox.height,
             bbox.x, bbox.y));

    /* Update this portion of the widget. */
    paint(self, &bbox);

    /* Arrange to stop if this is the last GraphicsExpose event in the
     * batch. */
    return event->count != 0;
}

static void
process_events(Widget widget, XtPointer closure, XEvent *event,
               Boolean *continue_to_dispatch)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(widget);
    XEvent event_buffer;

    /* Process all of the GraphicsExpose events in one hit */
    for (;;) {
        switch (event->type) {
        case GraphicsExpose:
            /* Continue processing graphics expose evens until we've
             * processed the entire batch. */
            if (!gexpose(self, &event->xgraphicsexpose)) {
                *continue_to_dispatch = False;
                return;
            }

        case NoExpose:
            /* Stop drawing if the widget is obscured. */
            DPRINTF((5, "History: NoExpose event\n"));
            *continue_to_dispatch = False;
            return;

        case SelectionClear:
            DPRINTF((5, "History: SelectionClear\n"));
            *continue_to_dispatch = True;
            return;

        case SelectionRequest:
            DPRINTF((5, "History: SelectionRequest\n"));
            *continue_to_dispatch = True;
            return;

        case SelectionNotify:
            DPRINTF((5, "History: electionNotify\n"));
            *continue_to_dispatch = True;
            return;

        case ClientMessage:
        case MappingNotify:
            *continue_to_dispatch = True;
            return;

        default:
            DPRINTF((5, "ignoring unknown event type: %d\n", event->type));
            *continue_to_dispatch = True;
            return;
        }

        /* Otherwise grab the next event */
        XNextEvent(display, &event_buffer);
        event = &event_buffer;
    }
}

/* Recompute the dimensions of the widget and update the scrollbars */
static void
recompute_dimensions(HistoryWidget self)
{
    int show_timestamps = self->history.show_timestamps;
    long width = 0;
    long height = 0;
    unsigned int i;
    long x, y;

    /* Measure each message */
    for (i = 0; i < self->history.message_count; i++) {
        struct string_sizes sizes;

        message_view_get_sizes(self->history.message_views[i],
                               show_timestamps,
                               &sizes);
        width = MAX(width, sizes.width);
        height += self->history.line_height;
    }

    /* Update our dimensions */
    self->history.width = width + (long)self->history.margin_width * 2;
    self->history.height = height + (long)self->history.margin_height * 2;

    /* And update the scrollbars */
    update_scrollbars((Widget)self, &x, &y);
    set_origin(self, x, y, False);
}

/* Realize the widget by creating a window in which to display it */
static void
realize(Widget widget,
        XtValueMask *value_mask,
        XSetWindowAttributes *attributes)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(self);
    XGCValues values;

    DPRINTF((5, "History.realize()\n"));

    /* Create our window */
    XtCreateWindow(widget, InputOutput, CopyFromParent,
                   *value_mask, attributes);

    /* Create a GC for our own nefarious purposes */
    values.background = self->core.background_pixel;
    values.font = self->history.font->fid;
    self->history.gc = XCreateGC(display, XtWindow(widget),
                                 GCBackground | GCFont, &values);

    /* Register to receive GraphicsExpose events */
    XtAddEventHandler(widget, 0, True, process_events, NULL);

    /* Register to receive motion callbacks */
    XtAddEventHandler(widget, PointerMotionMask, False, motion_cb, NULL);
}

/* Insert a message before the given index */
static void
insert_message(HistoryWidget self,
               unsigned int index,
               unsigned int indent,
               message_t message)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    message_view_t view;
    struct string_sizes sizes;
    long y = 0;
    long delta_y;
    long width = 0;
    long height = 0;
    XRectangle bbox;
    unsigned int i;
    XGCValues values;
    GC gc = self->history.gc;
    int show_timestamps = self->history.show_timestamps;
    long xpos, ypos;

    /* Sanity check */
    ASSERT(index <= self->history.message_count);

    /* Cancel our clip mask */
    if (gc != None) {
        values.clip_mask = None;
        values.foreground = self->core.background_pixel;
        XChangeGC(display, gc, GCClipMask | GCForeground, &values);
    }

    /* If there's still room then we'll have to move stuff down */
    if (self->history.message_count < self->history.message_capacity) {
        /* Measure the nodes before the index */
        for (i = 0; i < index; i++) {
            view = self->history.message_views[i];

            /* Measure the view */
            message_view_get_sizes(view, show_timestamps, &sizes);
            width = MAX(width, sizes.width);
            y += self->history.line_height;
            height += self->history.line_height;
        }

        /* Measure the nodes after the index, moving them down to make
         * room for the new message. */
        for (i = self->history.message_count; i > index; i--) {
            view = self->history.message_views[i - 1];

            /* Measure the view */
            message_view_get_sizes(view, show_timestamps, &sizes);
            width = MAX(width, sizes.width);
            height += self->history.line_height;

            /* Move it */
            self->history.message_views[i] = view;
            if (self->history.selection_index == i - 1) {
                self->history.selection_index = i;
            }
        }

        /* Move stuff down to make room */
        if (gc != None) {
            copy_area(self, display, window, gc,
                      0, self->history.margin_height - self->history.y + y,
                      self->core.width, height - y,
                      0, self->history.margin_height -
                      self->history.y + y + self->history.line_height);
        }

        /* We've got another node */
        self->history.message_count++;
    } else {
        /* Discard the first message view */
        message_view_free(self->history.message_views[0]);
        if (self->history.selection_index == 0) {
            self->history.selection_index = (unsigned int)-1;
        }

        /* Measure the nodes before the index, moving them up to make
         * room for the new message. */
        for (i = 0; i < index; i++) {
            view = self->history.message_views[i + 1];

            /* Measure the view */
            message_view_get_sizes(view, show_timestamps, &sizes);
            width = MAX(width, sizes.width);
            y += self->history.line_height;
            height += self->history.line_height;

            /* Move it up */
            self->history.message_views[i] = view;

            /* Update the selection index */
            if (self->history.selection_index == i + 1) {
                self->history.selection_index = i;
            }
        }

        /* Measure the nodes after the index */
        for (i = index + 1; i < self->history.message_count; i++) {
            view = self->history.message_views[i];

            /* Measure the view */
            message_view_get_sizes(view, show_timestamps, &sizes);
            width = MAX(width, sizes.width);
            height += self->history.line_height;
        }

        /* Move stuff up to make room */
        if (gc != None) {
            copy_area(self, display, window, gc,
                      0, self->history.margin_height -
                      self->history.y + self->history.line_height,
                      self->core.width, y,
                      0, self->history.margin_height - self->history.y);
        }
    }

    /* Create a new message view */
    /* FIX THIS: use a real conversion descriptor! */
    view = message_view_alloc(message, indent, self->history.renderer);
    self->history.message_views[index] = view;

    /* Measure it */
    message_view_get_sizes(view, show_timestamps, &sizes);
    width = MAX(width, sizes.width);
    height += self->history.line_height;

    /* Paint it */
    if (gc != None) {
        /* Remove any remains of the previous message */
        XFillRectangle(display, window, gc,
                       0, self->history.margin_height -
                       self->history.y + y,
                       self->core.width, self->history.line_height);

        /* Make a bounding box */
        bbox.x = 0;
        bbox.y = self->history.margin_height - self->history.y + y;
        bbox.width = self->core.width;
        bbox.height = self->history.line_height;

        /* Use it to draw the new message */
        message_view_paint(view, display, window, gc,
                           self->history.show_timestamps,
                           self->history.timestamp_pixel,
                           self->history.group_pixel,
                           self->history.user_pixel,
                           self->history.string_pixel,
                           self->history.separator_pixel,
                           self->history.margin_width - self->history.x,
                           self->history.margin_height - self->history.y +
                           y + self->history.font->ascent,
                           &bbox);
    }

    /* Add the margins to our height and width */
    width += (long)self->history.margin_width * 2;
    height += (long)self->history.margin_height * 2;

    /* Has the width changed? */
    if (self->history.width != width) {
        self->history.width = width;

        /* Repaint the selection so that the right edge is drawn properly */
        redisplay_index(self, self->history.selection_index);
    }

    /* Update our height */
    if (self->core.height < height) {
        delta_y =
            MAX(0, height -
                MAX((long)self->history.height, (long)self->core.height));
    } else {
        delta_y = 0;
    }

    self->history.height = height;

    /* Update the scrollbars */
    update_scrollbars((Widget)self, &xpos, &ypos);
    set_origin(self, xpos, ypos + delta_y, True);
}

/* Make sure the given index is visible */
static void
make_index_visible(HistoryWidget self, unsigned int index)
{
    long y;

    /* Sanity check */
    ASSERT(index < self->history.message_count);

    /* Figure out where the index would appear */
    y = self->history.selection_index * self->history.line_height;

    /* If it's above then scroll up to it */
    if (y + self->history.margin_height < self->history.y) {
        set_origin(self, self->history.x, y + self->history.margin_height, 1);
        return;
    }

    /* If it's below then scroll down to it */
    if (self->history.y + self->core.height - self->history.margin_height <
        y + self->history.line_height) {
        set_origin(self, self->history.x,
                   y + self->history.line_height - self->core.height +
                   self->history.margin_height, 1);
        return;
    }
}

/* Selects the given message at the given index */
static void
set_selection(HistoryWidget self, unsigned int index, message_t message)
{
    Display *display = XtDisplay((Widget)self);
    Window window = XtWindow((Widget)self);
    GC gc = self->history.gc;
    XGCValues values;
    XRectangle bbox;
    long y;

    /* Is this the same selection as before? */
    if (self->history.selection == message) {
        /* Just make sure it's visible */
        if (gc != None &&
            self->history.selection_index != (unsigned int)-1) {
            make_index_visible(self, index);
        }

        return;
    }

    /* Set up a bounding box */
    bbox.x = 0;
    bbox.y = 0;
    bbox.width = self->core.width;
    bbox.height = self->core.height;

    /* Drop our extra reference to the old selection */
    if (self->history.selection != NULL) {
        MESSAGE_FREE_REF(self->history.selection, ref_selection, self);
    }

    /* Redraw the old selection if appropriate */
    if (gc != None && self->history.selection_index != (unsigned int)-1) {
        /* Determine the location of the old selection */
        y = self->history.selection_index * self->history.line_height -
            self->history.y + self->history.margin_height;

        /* Is it visible? */
        if (0 <= y + self->history.line_height &&
            y < (long)self->history.height) {
            /* Clear the clip mask */
            values.clip_mask = None;
            values.foreground = self->core.background_pixel;
            XChangeGC(display, gc, GCClipMask | GCForeground, &values);

            /* Erase the rectangle previously occupied by the message */
            XFillRectangle(
                display, window, gc,
                0, y, self->core.width, self->history.line_height);

            /* And then draw it again */
            message_view_paint(
                self->history.message_views[self->history.selection_index],
                display, window, gc,
                self->history.show_timestamps,
                self->history.timestamp_pixel,
                self->history.group_pixel, self->history.user_pixel,
                self->history.string_pixel, self->history.separator_pixel,
                self->history.margin_width - self->history.x,
                y + self->history.font->ascent,
                &bbox);
        }
    }

    /* Record the new selection */
    if (message == NULL) {
        self->history.selection = NULL;
    } else {
        self->history.selection = message;
        MESSAGE_ALLOC_REF(message, ref_selection, self);
    }

    /* And record its index */
    self->history.selection_index = index;

    /* Draw the new selection if appropriate */
    if (gc != None && self->history.selection_index != (unsigned int)-1) {
        /* Determine its location */
        y = self->history.selection_index * self->history.line_height -
            self->history.y + self->history.margin_height;

        /* Is it visible? */
        if (0 <= y + self->history.line_height &&
            y < (long)self->history.height) {
            /* Clear the clip mask */
            values.clip_mask = None;
            values.foreground = self->history.selection_pixel;
            XChangeGC(display, gc, GCClipMask | GCForeground, &values);

            /* Draw the selection rectangle */
            XFillRectangle(display, window, gc, 0, y,
                           self->core.width, self->history.line_height);

            /* Draw the highlight */
            paint_highlight(self);

            /* And then draw the message view on top of it */
            message_view_paint(
                self->history.message_views[self->history.selection_index],
                display, window, gc,
                self->history.show_timestamps,
                self->history.timestamp_pixel,
                self->history.group_pixel, self->history.user_pixel,
                self->history.string_pixel, self->history.separator_pixel,
                self->history.margin_width - self->history.x,
                y + self->history.font->ascent,
                &bbox);
        }

        /* Try to make the entire selection is visible */
        make_index_visible(self, index);
    }

    /* Call the callback with our new selection */
    XtCallCallbackList((Widget)self, self->history.callbacks, message);
}

/* Selects the message at the given index */
static void
set_selection_index(HistoryWidget self, unsigned int index)
{
    message_view_t view;

    /* Locate the message view at that index */
    if (index < self->history.message_count) {
        view = self->history.message_views[index];
    } else {
        index = (unsigned int)-1;
        view = NULL;
    }

    /* Select it */
    set_selection(self, index, view ? message_view_get_message(view) : NULL);
}

/* Destroy the widget */
static void
destroy(Widget self)
{
    DPRINTF((3, "History.destroy()\n"));
}

/* Resize the widget */
static void
resize(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;
    int page_inc;
    long x, y;

    DPRINTF((3, "History.resize(w=%d, h=%d)\n",
             self->core.width, self->core.height));

    /* Update the page increment of the horizontal scrollbar */
    page_inc = self->core.height;
    XtVaSetValues(self->history.hscrollbar,
                  XmNpageIncrement, self->core.width,
                  NULL);

    /* Update the page increment of the vertical scrollbar */
    page_inc = self->core.height;
    XtVaSetValues(self->history.vscrollbar,
                  XmNpageIncrement, page_inc,
                  NULL);

    /* Update the scrollbar sizes */
    update_scrollbars(widget, &x, &y);

    /* The entire widget will be redisplayed in a moment, so simply
     * set up the correct location */
    self->history.x = x;
    self->history.y = y;
}

/* What should this do? */
static Boolean
set_values(Widget current,
           Widget request,
           Widget new,
           ArgList args,
           Cardinal *num_args)
{
    DPRINTF((3, "History.set_values()\n"));
    return False;
}

/* We're always happy */
static XtGeometryResult
query_geometry(Widget widget,
               XtWidgetGeometry *intended,
               XtWidgetGeometry *preferred)
{
    DPRINTF((3, "History.query_geometry()\n"));
    return XtGeometryYes;
}

/* Hightlight the border */
static void
border_highlight(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;

    DPRINTF((3, "History.border_highlight()\n"));

    /* Skip out if we're already highlighted */
    if (self->primitive.highlighted) {
        return;
    }

    /* We're now highlighted */
    self->primitive.highlighted = True;

    /* Repaint the selection */
    redisplay_index(self, self->history.selection_index);
}

/* Unhighlight the border */
static void
border_unhighlight(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;

    DPRINTF((3, "History.border_unhighlight()\n"));

    /* No longer highlighting */
    self->primitive.highlighted = False;

    /* Repaint the selection */
    redisplay_index(self, self->history.selection_index);
}

/*
 *
 *  Action definitions
 *
 */

/* Scroll up or down, wait and repeat */
static void
drag_timeout_cb(XtPointer closure, XtIntervalId *id)
{
    HistoryWidget self = (HistoryWidget)closure;
    unsigned int index;
    int x, y;

    /* Sanity check */
    ASSERT(self->history.drag_timeout == *id);

    /* Clear our status */
    self->history.drag_direction = DRAG_NONE;
    self->history.drag_timeout = None;

    /* Determine the position of the pointer */
    x = self->history.pointer_x;
    y = self->history.pointer_y;

    /* Figure out what's happening */
    if (y < 0) {
        /* We're being dragged up. */
        self->history.drag_direction = DRAG_UP;

        /* If we've already selected at the first message then make
         * the top margin visible */
        if (self->history.selection_index == 0) {
            set_origin(self, self->history.x, 0, 1);
        } else {
            /* Otherwise select the item before the first completely
             * visible one */
            index = index_of_y(self, -1);
            set_selection_index(self, index);
        }
    } else if (self->core.height < y) {
        /* We're being dragged down */
        self->history.drag_direction = DRAG_DOWN;

        /* If we've already select the last message then make the
         * bottom margin visible */
        if (self->history.selection_index ==
            self->history.message_count - 1) {
            if (self->history.height >= self->core.height) {
                set_origin(self, self->history.x,
                           self->history.height - self->core.height, 1);
            }
        } else {
            /* Otherwise select the item below the last completely
             * visible one */
            index = index_of_y(self, self->core.height + 1);
            set_selection_index(self, index);
        }
    } else {
        /* We're within the widget's bounds */
        index = index_of_y(self, y);

        /* Don't go past the last message */
        if (index >= self->history.message_count) {
            index = self->history.message_count - 1;
        }

        /* Select */
        set_selection_index(self, index);
    }

    /* Arrange to scroll again after a judicious pause */
    self->history.drag_timeout =
        XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)self),
                        self->history.drag_delay,
                        drag_timeout_cb, self);
}

/* Dragging selects */
static void
drag(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XMotionEvent *button_event = (XMotionEvent *)event;

    /* This widget now gets keyboard input */
    XmProcessTraversal(widget, XmTRAVERSE_CURRENT);

    /* Record the current mouse position */
    self->history.pointer_x = button_event->x;
    self->history.pointer_y = button_event->y;

    /* FIgure out what's going on */
    if (button_event->y < 0) {
        /* We should be dragging up.  Is a timeout already registered? */
        if (self->history.drag_timeout != None) {
            /* Bail if we're already scrolling up */
            if (self->history.drag_direction == DRAG_UP) {
                return;
            }

            /* Otherwise we're scrolling the wrong way! */
            XtRemoveTimeOut(self->history.drag_timeout);
            self->history.drag_timeout = None;
        }

        /* We're dragging up */
        self->history.drag_direction = DRAG_UP;
    } else if (self->core.height < button_event->y) {
        /* We should be dragging down.  Is a timeout already registered? */
        if (self->history.drag_timeout != None) {
            /* Bail if we're already scrolling down */
            if (self->history.drag_direction == DRAG_DOWN) {
                return;
            }

            /* Otherwise we're scrolling the wrong way! */
            XtRemoveTimeOut(self->history.drag_timeout);
            self->history.drag_timeout = None;
        }

        /* Call the timeout callback to scroll down and set a timeout */
        self->history.drag_direction = DRAG_DOWN;
    } else {
        /* We're within the bounds of the widget.  Is a timeout registered? */
        if (self->history.drag_timeout != None) {
            /* Cancel the timeout */
            XtRemoveTimeOut(self->history.drag_timeout);
            self->history.drag_timeout = None;
        }

        /* We're not dragging the window around */
        self->history.drag_direction = DRAG_NONE;
    }

    /* Call the drag timeout callback to update the selection and set
     * a timeout */
    drag_timeout_cb(self, &self->history.drag_timeout);
}

/* End of the dragging */
static void
drag_done(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;

    /* Cancel any pending timeout */
    if (self->history.drag_timeout != None) {
        XtRemoveTimeOut(self->history.drag_timeout);
        self->history.drag_timeout = None;
    }
}

/* Select the item under the mouse */
static void
do_select(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;
    unsigned int index;

    /* This widget now gets keyboard input */
    XmProcessTraversal(widget, XmTRAVERSE_CURRENT);

    /* Convert the y coordinate into an index */
    index = index_of_y(self, button_event->y);

    /* Anything past the last message is considered part of that
     * message for our purposes here */
    if (index >= self->history.message_count) {
        index = self->history.message_count - 1;
    }

    set_selection_index(self, index);
}

/* Select or deselect the item under the pointer */
static void
toggle_selection(Widget widget,
                 XEvent *event,
                 String *params,
                 Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    XButtonEvent *button_event = (XButtonEvent *)event;
    unsigned int index;

    /* This widget now gets keyboard input */
    XmProcessTraversal(widget, XmTRAVERSE_CURRENT);

    /* Convert the y coordinate into an index */
    index = (self->history.y + button_event->y -
             self->history.margin_height) / self->history.line_height;

    /* Are we selecting past the end of the list? */
    if (index >= self->history.message_count) {
        index = self->history.message_count - 1;
    }

    /* Is this our current selection? */
    if (self->history.selection_index == index) {
        /* Yes.  Toggle the selection off */
        index = (unsigned int)-1;
    }

    /* Set the selection */
    set_selection_index(self, index);
}

/* Call the attachment callbacks */
static void
show_attachment(Widget widget,
                XEvent *event,
                String *params,
                Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;

    /* This widget now gets keyboard input */
    XmProcessTraversal(widget, XmTRAVERSE_CURRENT);

    /* Finish the dragging */
    if (self->history.drag_timeout != None) {
        XtRemoveTimeOut(self->history.drag_timeout);
        self->history.drag_timeout = None;
    }

    /* Call the attachment callbacks */
    if (self->history.selection) {
        XtCallCallbackList(widget, self->history.attachment_callbacks,
                           self->history.selection);
    }
}

/* Select the previous item in the history */
static void
select_previous(Widget widget,
                XEvent *event,
                String *params,
                Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int index;

    /* Is anything selected? */
    if (self->history.selection_index == (unsigned int)-1) {
        /* No.  Prepare to select the last item in the list */
        index = self->history.message_count;
    } else {
        /* Yes.  Prepare to select the one before it */
        index = self->history.selection_index;
    }

    /* Bail if there is no previous item */
    if (index == 0) {
        return;
    }

    /* Select the previous item */
    set_selection_index(self, index - 1);
}

/* Select the next item in the history */
static void
select_next(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int index;

    /* Prepare to select the next item */
    index = self->history.selection_index + 1;

    /* Bail if there is no next item */
    if (index >= self->history.message_count) {
        return;
    }

    /* Select the next item */
    set_selection_index(self, index);
}

/* Scroll the window up if we can */
static void
scroll_up(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    int increment;

    /* Get the vertical scrollbar's increment */
    XtVaGetValues(self->history.vscrollbar, XmNincrement, &increment, NULL);

    /* Move that far to the up */
    set_origin(self, self->history.x, MAX(self->history.y - increment, 0), 1);
}

/* Scroll the window down if we can */
static void
scroll_down(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    int increment;

    /* Get the vertical scrollbar's increment */
    XtVaGetValues(self->history.vscrollbar, XmNincrement, &increment, NULL);

    /* Move that far down */
    set_origin(self, self->history.x,
               MIN(self->history.y + increment,
                   self->history.height < self->core.height ? 0 :
                   self->history.height - self->core.height), 1);
}

/* Scroll the window left if we can */
static void
scroll_left(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    int increment;

    /* Get the horizontal scrollbar's increment */
    XtVaGetValues(self->history.hscrollbar, XmNincrement, &increment, NULL);

    /* Move that far to the left */
    set_origin(self, MAX(self->history.x - increment, 0),
               self->history.y, 1);
}

/* Scroll the window right if we can */
static void
scroll_right(Widget widget, XEvent *event, String *params, Cardinal *nparams)
{
    HistoryWidget self = (HistoryWidget)widget;
    int increment;

    /* Get the scrollbar's increment */
    XtVaGetValues(self->history.hscrollbar, XmNincrement, &increment, NULL);

    /* Move that far to the right */
    set_origin(self, MIN(self->history.x + increment,
                         (self->history.width - self->core.width)),
               self->history.y, 1);
}

static void
copy_selection(Widget widget, XEvent* event, char **params,
               Cardinal *num_params)
{
    HistoryWidget self = (HistoryWidget)widget;
    Boolean res;
    message_t message;
    message_part_t part;
    const char *target;

    /* Bail if there's no selection. */
    message = self->history.selection;
    if (message == NULL) {
        DPRINTF((3, "No selection to copy.\n"));
        return;
    }

    /* Decide what to copy. */
    part = message_part_from_string(*num_params < 1 ? NULL : params[0]);

    /* Bail if the relevant part of the message doesn't exist. */
    if (message_part_size(message, part) == 0) {
        DPRINTF((1, "Message has no %s\n", message_part_to_string(part)));
    }

    /* Record the selected message and part. */
    if (self->history.copy_message != NULL) {
        MESSAGE_FREE_REF(self->history.copy_message, ref_copy, self);
        self->history.copy_message = NULL;
    }
    self->history.copy_message = self->history.selection;
    MESSAGE_ALLOC_REF(self->history.copy_message, ref_copy, self);
    self->history.copy_part = part;

    /* Decide where we're copying to. */
    target = (*num_params < 2) ? "CLIPBOARD" : params[1];
    DPRINTF((2, "BEFORE Xme[%s]Source\n", target));

    if (strcmp(target, "PRIMARY") == 0) {
        res = XmePrimarySource(widget, 0);
    } else if (strcmp(target, "SECONDARY") == 0) {
        res = XmeSecondarySource(widget, 0);
    } else /* if (strcmp(target, "CLIPBOARD") == 0) */ {
        res = XmeClipboardSource(widget, XmCOPY, 0);
    }

    DPRINTF((2, "AFTER Xme[%s]Source: %s\n", target, res ? "true" : "false"));
}

/* Redraw the widget */
static void
redraw_all(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;
    Display *display = XtDisplay(widget);
    Window window = XtWindow(widget);
    GC gc = self->history.gc;
    XGCValues values;
    XRectangle bbox;

    /* Bail if we don't have a valid GC */
    if (gc == None) {
        return;
    }

    /* Set up the graphics context */
    values.clip_mask = None;
    values.foreground = self->core.background_pixel;
    XChangeGC(display, gc, GCClipMask | GCForeground, &values);

    /* And erase the contents of the widget */
    XFillRectangle(display, window, gc, 0, 0,
                   self->core.width, self->core.height);

    /* Get a bounding box that contains the entire visible portion of
     * the widget */
    bbox.x = 0;
    bbox.y = 0;
    bbox.width = self->core.width;
    bbox.height = self->core.height;

    /* And repaint it */
    paint(self, &bbox);
}

static XtActionsRec actions[] =
{
    { "drag", drag },
    { "drag-done", drag_done },
    { "select", do_select },
    { "toggle-selection", toggle_selection },
    { "show-attachment", show_attachment },
    { "select-previous", select_previous },
    { "select-next", select_next },
    { "scroll-up", scroll_up },
    { "scroll-down", scroll_down },
    { "scroll-left", scroll_left },
    { "scroll-right", scroll_right },
    { "copy-selection", copy_selection }
};

/*
 *
 * Method definitions
 *
 */

/* Set the widget's display to be threaded or not */
void
HistorySetThreaded(Widget widget, Boolean is_threaded)
{
    HistoryWidget self = (HistoryWidget)widget;
    int i, index;
    message_t message;

    /* Don't do anything if there's no change */
    if (self->history.is_threaded == is_threaded) {
        return;
    }

    /* Change threaded status */
    self->history.is_threaded = is_threaded;

    /* Get rid of all of the old message views */
    for (i = 0; i < self->history.message_count; i++) {
        message_view_free(self->history.message_views[i]);
    }

    /* Create a bunch of new message views accordingly */
    if (is_threaded) {
        index = self->history.message_count - 1;

        /* Clear the selection in case it doesn't show up */
        self->history.selection_index = (unsigned int)-1;

        /* Traverse the history tree */
        node_populate(self->history.nodes,
                      self->history.renderer,
                      self->history.message_views,
                      0, &index,
                      self->history.selection,
                      &self->history.selection_index);
    } else {
        /* Create a bunch of new ones */
        index = self->history.message_index;
        for (i = 0; i < self->history.message_count; i++) {
            /* Look up the message */
            message = self->history.messages[index];

            /* Wrap it in a message view */
            self->history.message_views[i] =
                message_view_alloc(message, 0, self->history.renderer);

            /* Update the selection index */
            if (self->history.selection == message) {
                self->history.selection_index = i;
            }

            index = (index + 1) % self->history.message_count;
        }
    }

    /* Recompute the dimensions of the widget */
    recompute_dimensions(self);

    /* Redraw the contents of the widget */
    redraw_all(widget);
}

/* Returns whether or not the history is threaded */
Boolean
HistoryIsThreaded(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;

    return self->history.is_threaded;
}

/* Set the widget's display to show timestamps or not */
void
HistorySetShowTimestamps(Widget widget, Boolean show_timestamps)
{
    HistoryWidget self = (HistoryWidget)widget;

    /* Update the flag */
    self->history.show_timestamps = show_timestamps;

    /* Recompute the dimensions of the widget */
    recompute_dimensions(self);

    /* Redraw the whole thing */
    redraw_all(widget);
}

/* Returns whether or not the timestamps are visible */
Boolean
HistoryIsShowingTimestamps(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;

    return self->history.show_timestamps;
}

/* Adds a new message to the History */
void
HistoryAddMessage(Widget widget, message_t message)
{
    HistoryWidget self = (HistoryWidget)widget;
    node_t node;
    int index;
    int depth;

    /* Wrap the message in a node */
    node = node_alloc(message);
    if (node == NULL) {
        return;
    }

    /* Add the node to the threaded history tree */
    node_add(&self->history.nodes,
             message_get_reply_id(message),
             node,
             MIN(self->history.message_count + 1,
                 self->history.message_capacity),
             &index,
             &depth);

    /* If we're not at capacity then just add a reference to the end
     * of the array of messages */
    if (self->history.message_count < self->history.message_capacity) {
        self->history.messages[self->history.message_count] = message;
        MESSAGE_ALLOC_REF(message, ref_history, self);
    } else {
        /* Free the old message */
        MESSAGE_FREE_REF(self->history.messages[self->history.message_index],
                         ref_history, self);

        /* Replace it with this message */
        self->history.messages[self->history.message_index] = message;
        MESSAGE_ALLOC_REF(message, ref_history, self);

        self->history.message_index = (self->history.message_index + 1) %
            self->history.message_capacity;
    }

    /* Add the node according to our threadedness */
    if (HistoryIsThreaded(widget)) {
        insert_message(self, index, depth, message);
    } else {
        insert_message(
            self,
            self->history.message_count < self->history.message_capacity ?
            self->history.message_count : self->history.message_capacity - 1,
            0,
            message);
    }
}

/* Kills the thread of the given message */
void
HistoryKillThread(Widget widget, message_t message)
{
    HistoryWidget self = (HistoryWidget)widget;
    node_t node;

    /* Bail if the message has already been killed */
    if (message_is_killed(message)) {
        return;
    }

    /* Look for the node which wraps the message */
    node = node_find(self->history.nodes, message);
    if (node == NULL) {
        /* The message is killed now */
        message_set_killed(message, True);
    } else {
        /* Kill the node and its children */
        node_kill(node);
    }
}

/* Selects a message in the history */
void
HistorySelect(Widget widget, message_t message)
{
    HistoryWidget self = (HistoryWidget)widget;
    unsigned int i;

    /* Save the effort if we're just cancelling a selection */
    if (message != NULL) {
        /* Find the index of the message */
        for (i = 0; i < self->history.message_count; i++) {
            if (message_view_get_message(self->history.message_views[i]) ==
                message) {
                set_selection(self, i, message);
                return;
            }
        }
    }

    /* Not found -- select at -1 */
    set_selection(self, (unsigned int)-1, message);
}

/* Selects the parent of the message in the history */
void
HistorySelectId(Widget widget, const char *message_id)
{
    HistoryWidget self = (HistoryWidget)widget;
    message_t message;
    unsigned int i;
    const char *string;

    /* Save the effort if there's no id */
    if (message_id != NULL) {
        /* Find the index of the message */
        for (i = self->history.message_count; i > 0; --i) {
            message =
                message_view_get_message(self->history.message_views[i - 1]);
            if (message == NULL) {
                continue;
            }

            string = message_get_id(message);
            if (string == NULL) {
                continue;
            }

            if (strcmp(string, message_id) == 0) {
                set_selection(self, i - 1, message);
                return;
            }
        }
    }

    /* Not found -- select at -1 */
    set_selection(self, (unsigned int)-1, NULL);
}

/* Returns the selected message or NULL if none is selected */
message_t
HistoryGetSelection(Widget widget)
{
    HistoryWidget self = (HistoryWidget)widget;

    return self->history.selection;
}

/*
 *
 * Class record initializations
 *
 */

HistoryClassRec historyClassRec =
{
    /* core_class fields */
    {
        (WidgetClass)&xmPrimitiveClassRec, /* superclass */
        "History", /* class_name */
        sizeof(HistoryRec), /* widget_size */
        NULL, /* class_initialize */
        class_part_init, /* class_part_initialize */
        False, /* class_inited */
        init, /* initialize */
        NULL, /* initialize_hook */
        realize, /* realize */
        actions, /* actions */
        XtNumber(actions), /* num_actions */
        resources, /* resources */
        XtNumber(resources), /* num_resources */
        NULLQUARK, /* xrm_class */
        True, /* compress_motion */
        XtExposeCompressMaximal, /* compress_exposure */
        True, /* compress_enterleave */
        False, /* visible_interest */
        destroy, /* destroy */
        resize, /* resize */
        redisplay, /* expose */
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
        border_highlight, /* border_highlight */
        border_unhighlight, /* border_unhighlight */
        NULL, /* translations */
        NULL, /* arm_and_activate_proc */
        NULL, /* synthetic resources */
        0, /* num syn res */
        NULL, /* extension */
    },

    /* History class fields initialization */
    {
        0 /* foo */
    }
};

WidgetClass historyWidgetClass = (WidgetClass)&historyClassRec;

/**********************************************************************/
