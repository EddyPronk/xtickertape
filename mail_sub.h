/***************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
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
             Transforms elvinmail notifications into tickertape
	     messages suitable for scrolling

****************************************************************/

#ifndef MAIL_SUB_H
#define MAIL_SUB_H

#ifndef lint
static const char cvs_MAIL_SUB_H[] = "$Id: mail_sub.h,v 1.9 2009/03/09 05:26:27 phelps Exp $";
#endif /* lint */

/* The mail_sub_t data type */
typedef struct mail_sub *mail_sub_t;

/* The mail_sub callback type */
typedef void (*mail_sub_callback_t)(void *rock, message_t message, int show_attachment);


/* Exported functions */

/* Answers a new mail_sub */
mail_sub_t mail_sub_alloc(char *user, mail_sub_callback_t callback, void *rock);

/* Releases the resources consumed by the receiver */
void mail_sub_free(mail_sub_t self);

/* Prints debugging information about the receiver */
void mail_sub_debug(mail_sub_t self);

/* Sets the receiver's connection */
void mail_sub_set_connection(mail_sub_t self, elvin_handle_t handle, elvin_error_t error);

#endif /* MAIL_SUB_H */
