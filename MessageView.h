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

   Description: 
             A view (in the MVC sense) of the a Message which is
	     suitable for scrolling through across the Scroller widget 

****************************************************************/

#ifndef MESSAGEVIEW_H
#define MESSAGEVIEW_H

#ifndef lint
static const char cvs_MESSAGEVIEW_H[] = "$Id: MessageView.h,v 1.9 1998/12/24 05:48:29 phelps Exp $";
#endif /* lint */

typedef struct MessageView_t *MessageView;

#include "Message.h"

/* Prints debugging information */
void MessageView_debug(MessageView self);

/* Creates and returns a 'view' on a Message */
MessageView MessageView_alloc(ScrollerWidget widget, Message message);

/* Free the memory allocated by the receiver */
void MessageView_free(MessageView self);

/* Adds another reference to the count */
MessageView MessageView_allocReference(MessageView self);

/* Removes a reference from the count */
void MessageView_freeReference(MessageView self);


/* Answers the receiver's Message */
Message MessageView_getMessage(MessageView self);

/* Answers the width (in pixels) of the receiver */
unsigned int MessageView_getWidth(MessageView self);

/* Redisplays the receiver on the drawable */
void MessageView_redisplay(MessageView self, Drawable drawable, int x, int y);

/* Answers non-zero if the receiver has outstayed its welcome */
int MessageView_isTimedOut(MessageView self);

/* MIME-decodes the receiver's message */
void MessageView_decodeMime(MessageView self);

/* Convice a MessageView that time as run out */
void MessageView_expire(MessageView self);

#endif /* MESSAGEVIEW_H */
