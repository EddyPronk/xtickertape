/***************************************************************

  Copyright (C) 2001-2004 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

  This software is the property of the Mantara Software. All use
  of this software is permitted only under the terms of a
  license agreement with Mantara Software. If you do not have
  such a license, then you have no rights to use this software
  in any manner and should contact Mantara at the address below
  to determine an appropriate licensing arrangement.
  
     Mantara Software
     PO Box 1820
     Toowong QLD 4066
     Australia
     Tel: +61 7 3876 8844
     Fax: +61 7 3876 8843
     Email: licensing@mantara.com
  
     Web: http://www.mantara.com
  
  This software is being provided "AS IS" without warranty of
  any kind. In no event shall Mantara Software be liable for
  damage of any kind arising out of or in connection with the
  use or performance of this software.
****************************************************************/

#ifndef MESSAGE_VIEW_H
#define MESSAGE_VIEW_H

#ifndef lint
static const char cvs_MESSAGE_VIEW_H[] = "$Id: message_view.h,v 2.11 2004/08/02 22:24:16 phelps Exp $";
#endif /* lint */

/* The message_view type */
typedef struct message_view *message_view_t;

/* Allocates and initializes a new message_view_t */
message_view_t message_view_alloc(
    message_t message,
    long indent,
    utf8_renderer_t renderer);

/* Frees a message_view_t */
void message_view_free(message_view_t self);

/* Returns the message view's message */
message_t message_view_get_message(message_view_t self);

/* Returns the sizes of the message view */
void message_view_get_sizes(
    message_view_t self,
    int show_timestamp,
    string_sizes_t sizes_out);

/* Draws the message_view */
void message_view_paint(
    message_view_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    int show_timestamp,
    unsigned long timestamp_pixel,
    unsigned long group_pixel,
    unsigned long user_pixel,
    unsigned long message_pixel,
    unsigned long separator_pixel,
    long x, long y,
    XRectangle *bbox);
#endif /* MESSAGE_VIEW_H */
