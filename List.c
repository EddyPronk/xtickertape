/* $Id */

#include <stdio.h>
#include <stdlib.h>
#include "List.h"

/* The List links */
typedef struct ListLink_t *ListLink;

struct List_t
{
    ListLink head;
    ListLink tail;
};

struct ListLink_t
{
    ListLink next;
    void *value;
};




/* Allocates a new ListLink */
ListLink ListLink_alloc(void *value)
{
    ListLink link = (ListLink) malloc(sizeof(struct ListLink_t));
    link -> next = NULL;
    link -> value = value;
    return link;
}

/* Frees a ListLink */
void ListLink_free(ListLink self)
{
    free(self);
}


/* Allocates a new List */
List List_alloc()
{
    List list;

    list = (List) malloc(sizeof(struct List_t));
    list -> head = NULL;
    list -> tail = NULL;
    return list;
}

/* Releases a List [Doesn't release its contents] */
void List_free(List self)
{
    ListLink link = self -> head;

    while (link != NULL)
    {
	ListLink next = link -> next;
	ListLink_free(link);
	link = next;
    }

    free(self);
}



/* Adds an value to the beginning of the list */
void List_addFirst(List self, void *value)
{
    ListLink link = ListLink_alloc(value);

    if (self -> tail == NULL)
    {
	self -> tail = link;
    }

    link -> next = self -> head;
    self -> head = link;
}

/* Removes the first item from the list */
void *List_dequeue(List self)
{
    void *value = List_first(self);
    List_remove(self, value);
    return value;
}


/* Adds an value to the end of the list */
void List_addLast(List self, void *value)
{
    ListLink link = ListLink_alloc(value);

    if (self -> head == NULL)
    {
	self -> head = link;
    }

    if (self -> tail != NULL)
    {
	self -> tail -> next = link;
    }

    self -> tail = link;
}


/* Removes an value from the list */
void *List_remove(List self, void *value)
{
    ListLink link = self -> head;
    ListLink previous = NULL;

    while (link != NULL)
    {
	if (link -> value == value)
	{
	    if (previous == NULL)
	    {
		self -> head = link -> next;
	    }
	    else
	    {
		previous -> next = link -> next;
	    }

	    if (link -> next == NULL)
	    {
		self -> tail = previous;
	    }

	    ListLink_free(link);
	    return value;
	}

	previous = link;
	link = link -> next;
    }

    return NULL;
}


/* Answers the indexed element of the receiver */
void *List_get(List self, unsigned long index)
{
    ListLink link;
    int count = 0;

    for (link = self -> head; link != NULL; link = link -> next)
    {
	if (count++ == index)
	{
	    return link -> value;
	}
    }

    return NULL;
}


/* Answers the first element of the receiver */
void *List_first(List self)
{
    if (self -> head)
    {
	return self -> head -> value;
    }
    else
    {
	return NULL;
    }
}

/* Answers the last element of the receiver */
void *List_last(List self)
{
    if (self -> tail)
    {
	return self -> tail -> value;
    }
    else
    {
	return NULL;
    }
}


/* Answers the number of elements in the receiver */
unsigned long List_size(List self)
{
    unsigned long count = 0;
    ListLink link;

    for (link = self -> head; link != NULL; link = link -> next)
    {
	count++;
    }

    return count;
}


/* Answers 0 if the receiver contains the value, 1 otherwise */
int List_includes(List self, void *value)
{
    ListLink link;

    for (link = self -> head; link != NULL; link = link -> next)
    {
	if (link -> value == value)
	{
	    return 1;
	}
    }

    return 0;
}



/* Enumeration */
void List_do(List self, void (*function)())
{
    ListLink link;

    for (link = self -> head; link != NULL; link = link -> next)
    {
	(*function)(link -> value);
    }
}


/* Enumeration with context */
void List_doWith(List self, void (*function)(), void *context)
{
    ListLink link;

    for (link = self -> head; link != NULL; link = link -> next)
    {
	(*function)(link -> value, context);
    }
}





/* Debugging */
void List_debug(List self)
{
    ListLink link;

    printf("\nList (0x%p)\n", self);
    printf("  head = 0x%p\n", self -> head);
    printf("  tail = 0x%p\n", self -> tail);
    printf("  elements:\n");

    for (link = self -> head; link != NULL; link = link -> next)
    {
	printf("    ListLink (0x%p)\n", link);
	printf("      next = 0x%p\n", link -> next);
	printf("      value = 0x%p (\"%s\")\n", link -> value, (char *)link -> value);
    }
}
