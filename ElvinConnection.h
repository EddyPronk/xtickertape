/* $Id: ElvinConnection.h,v 1.1 1997/02/07 08:48:43 phelps Exp $ */

#ifndef ELVINCONNECTION_H
#define ELVINCONNECTION_H

typedef struct ElvinConnection_t *ElvinConnection;

/* Answers a new ElvinConnection */
ElvinConnection ElvinConnection_alloc(char *hostname, int port, List subscriptions);

/* Releases the resources used by the ElvinConnection */
void ElvinConnection_free(ElvinConnection self);

#endif /* ELVINCONNECTION_H */
