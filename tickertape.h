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
             Represents a Scroller and ControlPanel and manages the
	     connection between them, the notification service and the 
	     user

****************************************************************/

#ifndef TICKERTAPE_H
#define TICKERTAPE_H

#ifndef lint
static const char cvs_TICKERTAPE_H[] = "$Id: tickertape.h,v 1.1 1999/08/19 01:46:12 phelps Exp $";
#endif /* lint */

typedef struct Tickertape_t *Tickertape;

#include <X11/Intrinsic.h>

/* Answers a new Tickertape for the given user using the given file as
 * her groups file and connecting to the notification service
 * specified by host and port */
Tickertape Tickertape_alloc(
    char *user, char *tickerDir,
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


/* Answers the receiver's tickerDir filename */
char *Tickertape_tickerDir(Tickertape self);

/* Answers the receiver's groups file filename */
char *Tickertape_groupsFilename(Tickertape self);

/* Answers the receiver's usenet filename */
char *Tickertape_usenetFilename(Tickertape self);

/* Answers the receiver's groups file */
FILE *Tickertape_groupsFile(Tickertape self);

/* Answers the receiver's usenet file */
FILE *Tickertape_usenetFile(Tickertape self);

#endif /* TICKERTAPE_H */
