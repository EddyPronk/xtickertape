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
static const char cvs_MESSAGE_H[] = "$Id: message.h,v 1.5 2001/02/22 01:31:30 phelps Exp $";
#endif /* lint */

/* The message_t type */
typedef struct message *message_t;

/* Creates and returns a new message */
message_t message_alloc(
    char *info,
    char *group,
    char *user,
    char *string,
    unsigned int timeout,
    char *mime_type,
    char *mime_args,
    size_t mime_length,
    char *tag,
    char *id,
    char *reply_id);

/* Allocates another reference to the message_t */
message_t message_alloc_reference(message_t self);

/* Frees the memory used by the receiver */
void message_free(message_t self);

/* Prints debugging information */
void message_debug(message_t self);


/* Answers the Subscription info for the receiver's subscription */
char *message_get_info(message_t self);

/* Answers the receiver's creation time */
time_t *message_get_creation_time(message_t self);

/* Answers the receiver's group */
char *message_get_group(message_t self);

/* Answers the receiver's user */
char *message_get_user(message_t self);

/* Answers the receiver's string */
char *message_get_string(message_t self);

/* Answers the receiver's timout in minutes */
unsigned long message_get_timeout(message_t self);

/* Sets the receiver's timeout in minutes*/
void message_set_timeout(message_t self, unsigned long timeout);

/* Answers non-zero if the receiver has a MIME attachment */
int message_has_attachment(message_t self);

/* Answers the receiver's MIME-type string */
char *message_get_mime_type(message_t self);

/* Answers the receiver's MIME arguments */
size_t message_get_mime_args(message_t self, char **mime_args_out);

/* Answers the receiver's tag */
char *message_get_tag(message_t self);

/* Answers the receiver's id */
char *message_get_id(message_t self);

/* Answers the id of the message for which this is a reply */
char *message_get_reply_id(message_t self);

#endif /* MESSAGE_H */
