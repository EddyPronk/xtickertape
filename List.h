/*
 * $Id: List.h,v 1.4 1997/02/13 08:13:43 phelps Exp $
 *
 * Generic support for singly-linked lists
 */


#ifndef LIST_H
#define LIST_H

/* The List pointer type */
typedef struct List_t *List;

/* Allocates a new List */
List List_alloc();

/* Releases a List [Doesn't release its contents] */
void List_free(List self);


/* Adds an value to the beginning of the list */
void List_addFirst(List self, void *value);

/* Removes the first item from the list */
void *List_dequeue(List self);

/* Adds an item to the end of the list */
void List_addLast(List self, void *value);

/* Removes an item from the list */
void *List_remove(List self, void *item);


/* Answers the indexed element of the receiver */
void *List_get(List self, unsigned long index);

/* Answers the first element of the receiver */
void *List_first(List self);

/* Answers the last element of the receiver */
void *List_last(List self);

/* Answers the number of elements in the receiver */
unsigned long List_size(List self);

/* Answers non-zero if the list has no elements */
int List_isEmpty(List self);

/* Answers 0 if the receiver contains the value, 1 otherwise */
int List_includes(List self, void *value);


/* Enumeration */
void List_do(List self, void (*function)());

/* Enumeration with context */
void List_doWith(List self, void (*function)(), void *context);


/* Debugging */
void List_debug(List self);

#endif /* LIST_H */
