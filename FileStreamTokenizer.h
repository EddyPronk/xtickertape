/* $Id: FileStreamTokenizer.h,v 1.2 1998/10/23 10:03:31 phelps Exp $
 * COPYRIGHT!
 *
 * Reads tokens from a FILE, coping with whitespace, linefeeds
 * and special characters
 */

#ifndef FILE_STREAM_TOKENIZER_H
#define FILE_STREAM_TOKENIZER_H

/* The FileStreamTokenizer data type */
typedef struct FileStreamTokenizer_t *FileStreamTokenizer;

#include <stdio.h>

/* Answers a new FileStreamTokenizer */
FileStreamTokenizer FileStreamTokenizer_alloc(FILE *file, char *whitespace, char *special);

/* Frees the resources consumed by a FileStreamTokenizer */
void FileStreamTokenizer_free(FileStreamTokenizer self);

/* Prints debugging information about a FileStreamTokenizer */
void FileStreamTokenizer_debug(FileStreamTokenizer self);


/* Answers the receiver's next token (dynamically allocated) or NULL
 * if at the end of the file */
char *FileStreamTokenizer_next(FileStreamTokenizer self);

/* Skips to the end of the current line */
void FileStreamTokenizer_skipToEndOfLine(FileStreamTokenizer self);

#endif /* FILE_STREAM_TOKENIZER_H */
