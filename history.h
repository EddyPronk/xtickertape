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

#ifndef HISTORY_H
#define HISTORY_H

/* The history type */
typedef struct history *history_t;

#include <Xm/Xm.h>
#include "Message.h"

/* Allocates and initializes a new empty history */
history_t history_alloc();

/* Frees the history */
void history_free(history_t self);

/* Sets the history's Motif List widget */
void history_set_list(history_t self, Widget list);

/* Sets the history's threadedness */
void history_set_threaded(history_t self, int is_threaded);


/* Adds a message to the end of the history */
int history_add(history_t self, Message message);

/* Answers the Message at the given index */
Message history_get(history_t self, int index);

/* Answers the index of given Message in the history */
int history_index(history_t self, Message message);


#endif /* HISTORY_H */
