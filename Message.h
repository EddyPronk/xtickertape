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
             The model (in the MVC sense) of an elvin notification
	     with xtickertape

****************************************************************/

#ifndef MESSAGE_H
#define MESSAGE_H

#ifndef lint
static const char cvs_MESSAGE_H[] = "$Id: Message.h,v 1.12 1999/05/21 05:30:44 phelps Exp $";
#endif /* lint */

/* The Message pointer type */
typedef struct Message_t *Message;

/* Creates and returns a new message */
Message Message_alloc(
    void *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mimeType,
    char *mimeArgs,
    long id,
    long replyId);

/* Frees the memory used by the receiver */
void Message_free(Message self);

/* Prints debugging information */
void Message_debug(Message self);


/* Answers the Subscription info for the receiver's subscription */
void *Message_getInfo(Message self);

/* Answers the receiver's group */
char *Message_getGroup(Message self);

/* Answers the receiver's user */
char *Message_getUser(Message self);

/* Answers the receiver's string */
char *Message_getString(Message self);

/* Answers the receiver's timout in minutes */
unsigned long Message_getTimeout(Message self);

/* Sets the receiver's timeout in minutes*/
void Message_setTimeout(Message self, unsigned long timeout);

/* Answers non-zero if the receiver has a MIME attachment */
int Message_hasAttachment(Message self);

/* Answers the receiver's MIME-type string */
char *Message_getMimeType(Message self);

/* Answers the receiver's MIME arguments */
char *Message_getMimeArgs(Message self);

/* Answers the receiver's id */
unsigned long Message_getId(Message self);

/* Answers the id of the message for which this is a reply */
unsigned long Message_getReplyId(Message self);

#endif /* MESSAGE_H */
