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
             Helps in the construction of strings of unknown length by 
	     providing an automatically resizing buffer

****************************************************************/

#ifndef STRING_BUFFER_H
#define STRING_BUFFER_H

#ifndef lint
static const char cvs_STRING_BUFFER_H[] = "$Id: StringBuffer.h,v 1.4 1998/12/24 05:48:30 phelps Exp $";
#endif /* lint */

typedef struct StringBuffer_t *StringBuffer;

/* Allocates an empty StringBuffer */
StringBuffer StringBuffer_alloc();

/* Frees the resources used by the receiver */
void StringBuffer_free(StringBuffer self);

/* Prints out debugging information about the receiver */
void StringBuffer_debug(StringBuffer self);

/* Resets the buffer to its first character */
void StringBuffer_clear(StringBuffer self);

/* Answers the number of characters in the receiver's buffer */
unsigned long StringBuffer_length(StringBuffer self);

/* Appends a single character to the receiver */
void StringBuffer_appendChar(StringBuffer self, char ch);

/* Appends an integer to the receiver */
void StringBuffer_appendInt(StringBuffer self, int value);

/* Appends a null-terminated character array to the receiver */
void StringBuffer_append(StringBuffer self, char *string);

/* Answers a pointer to the receiver's null-terminated character array */
char *StringBuffer_getBuffer(StringBuffer self);

#endif /* STRING_BUFFER_H */
