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

#ifndef MESSAGE_THREAD_H
#define MESSAGE_THREAD_H

#ifndef lint
static const char cvs_MESSAGE_THREAD_H[] = "$Id: message_thread.h,v 1.1 1999/08/01 06:44:46 phelps Exp $";
#endif /* lint */

/* The message_thread type */
typedef struct message_thread *message_thread_t;


#include "Message.h"

/* Adds a Message to a thread */
message_thread_t message_thread_add(message_thread_t thread, Message message);

/* Prints debugging information */
void message_thread_debug(message_thread_t self);

#endif /* MESSAGE_THREAD_H */
