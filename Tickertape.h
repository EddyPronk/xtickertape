/*
 * $Id: Tickertape.h,v 1.7 1998/12/17 01:26:13 phelps Exp $
 * COPYRIGHT!
 *
 * Tickertape represents a scroller and control panel and manages the
 * connection between them, the notification service and the user.
 */

#ifndef TICKERTAPE_H
#define TICKERTAPE_H

typedef struct Tickertape_t *Tickertape;

#include <X11/Intrinsic.h>

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service
 * specified by host and port */
Tickertape Tickertape_alloc(
    char *user,
    char *groupsFile, char *usenetFile,
    char *host, int port,
    Widget top);

/* Frees the resources used by the Tickertape */
void Tickertape_free(Tickertape self);

/* Prints out debugging information about the Tickertape */
void Tickertape_debug(Tickertape self);

/* Handles the notify action */
void Tickertape_handleNotify(Tickertape self, Widget widget);

/* Handles the quit action */
void Tickertape_handleQuit(Tickertape self, Widget widget);

#endif /* TICKERTAPE_H */
