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
             Reads tokens from a FILE, coping with whitespace,
	     linefeeds and special characters

****************************************************************/

#ifndef FILE_STREAM_TOKENIZER_H
#define FILE_STREAM_TOKENIZER_H

#ifndef lint
static const char cvs_FILE_STREAM_TOKENIZER_H[] = "$Id: FileStreamTokenizer.h,v 1.7 1998/12/24 05:48:27 phelps Exp $";
#endif /* lint */

/* The FileStreamTokenizer data type */
typedef struct FileStreamTokenizer_t *FileStreamTokenizer;

#include <stdio.h>

/* Answers a new FileStreamTokenizer */
FileStreamTokenizer FileStreamTokenizer_alloc(FILE *file, char *whitespace, char *special);

/* Frees the resources consumed by a FileStreamTokenizer */
void FileStreamTokenizer_free(FileStreamTokenizer self);

/* Prints debugging information about a FileStreamTokenizer */
void FileStreamTokenizer_debug(FileStreamTokenizer self);

/* Skip over the given whitespace characters */
void FileStreamTokenizer_skip(FileStreamTokenizer self, char *whitespace);

/* Skips over whitespace */
void FileStreamTokenizer_skipWhitespace(FileStreamTokenizer self);


/* Answers the receiver's next token or NULL if at the end of the file.
 * NB: this token exists in the receiver's internal buffer and will be
 * overwritten when the next token is read */
char *FileStreamTokenizer_nextWithSpec(
    FileStreamTokenizer self, char *whitespace, char *special);

/* Answers the receiver's next token or NULL if at the end of the file.
 * NB: this token exists in the receiver's internal buffer and will be
 * overwritten when the next token is read */
char *FileStreamTokenizer_next(FileStreamTokenizer self);

/* Skips to the end of the current line */
void FileStreamTokenizer_skipToEndOfLine(FileStreamTokenizer self);

#endif /* FILE_STREAM_TOKENIZER_H */
