/*
 * $Id: Ticker.h,v 1.2 1998/10/21 01:58:09 phelps Exp $
 * COPYRIGHT!
 *
 * Tickertape represents a scroller and control panel and manages the
 * connection between them, the notification service and the user.
 */

#ifndef TICKER_H
#define TICKER_H

typedef struct Tickertape_t *Tickertape;

#include <X11/Intrinsic.h>

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service
 * specified by host and port */
Tickertape Tickertape_alloc(char *user, char *file, char *host, int port, Widget top);

/* Frees the resources used by the Tickertape */
void Tickertape_free(Tickertape self);

/* Prints out debugging information about the Tickertape */
void Tickertape_debug(Tickertape self);

/* Handles the notify action */
void Tickertape_handleNotify(Tickertape self, Widget widget);

/* Handles the quit action */
void Tickertape_handleQuit(Tickertape self, Widget widget);

/* Prints out debugging information about the receiver */
void Tickertape_debug(Tickertape self);

#endif /* TICKER_H */
