#############################################################################
# Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 1993, 1994, 1995.
# Unpublished work.  All Rights Reserved.
#
# The software contained on this media is the property of the
# DSTC Pty Ltd.  Use of this software is strictly in accordance
# with the license agreement in the accompanying LICENSE.DOC file.
# If your distribution of this software does not contain a
# LICENSE.DOC file then you have no rights to use this software
# in any manner and should contact DSTC at the address below
# to determine an appropriate licensing arrangement.
# 
#      DSTC Pty Ltd
#      Level 7, Gehrmann Labs
#      University of Queensland
#      St Lucia, 4072
#      Australia
#      Tel: +61 7 3365 4310
#      Fax: +61 7 3365 4311
#      Email: enquiries@dstc.edu.au
# 
# This software is being provided "AS IS" without warranty of
# any kind.  In no event shall DSTC Pty Ltd be liable for
# damage of any kind arising out of or in connection with
# the use or performance of this software.
#
# Project:  Elvin: event notification
#
# File:     $Source: /data/ticker/CVS/xtickertape/Attic/Makefile,v $
#
# History:
#           08-Feb-1996  Bill Segall (bill) : created
#
# "@(#)$RCSfile: Makefile,v $ $Revision: 1.13 $"
#
#############################################################################

# The DSTC Makefile.base steals the primary target so I snitch it back
# first
all: build

# Right to the bottom where it fails
ifdef DSTCDIR
include $(DSTCDIR)/install/Makefile.base

# So we can make locally
ifndef PROJDIR
PROJDIR = units/arch/elvin3.11
endif
include $(DSTCDIR)/$(PROJDIR)/install/Makefile.elvin

.PHONY:	config build install depend unconfig clean all unbuild \
uninstall clobber

C_SOURCE = Control.c ElvinConnection.c Hash.c List.c Message.c \
MessageView.c Subscription.c Tickertape.c main.c

HEADERS = Control.h ElvinConnection.h Hash.h List.h Message.h MessageView.h \
Subscription.h Tickertape.h TickertapeP.h sanity.h

OBJECTS =  $(patsubst %.c,$(ARCHITECTURE)/obj/%.o,$(C_SOURCE))
BINARIES = $(ELVIN_BIN_DIR)/xtickertape$(.EXE)
MAN_FILES = ""

# Dependency stuff. The include file needs to exist first
depend: config
	makedepend -f $(ARCHITECTURE)/include/depend.inc $(MAKEDEPFLAGS) $(C_SOURCE)

undepend:
	@$(RM) $(ARCHITECTURE)/include/depend.inc

config: elvin-arch-dirs
	@touch $(ARCHITECTURE)/include/depend.inc

unconfig:
	@$(RM) -rf $(ARCHITECTURE)

build: local-build config $(BINARIES)

doc:

unbuild clean: local-unbuild
	@-$(RM) -f $(ARCHITECTURE)/obj/*.o a.out *~ tca.log tca.map .inslog TAGS core

uninstall clobber: clean
	@-$(RM) -f $(BINARIES)

pure: $(ARCHITECTURE)/bin/producer-pure

$(ELVIN_BIN_DIR)/xtickertape$(.EXE): $(OBJECTS)
	$(LD) -o $@ $(OBJECTS) $(LDFLAGS) $(XLIBS)

$(ARCHITECTURE)/obj/%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(XINCLUDES) -c $< -o $@

################ Dependency information ################
foo:=$(shell if test ! -d $(ARCHITECTURE); then mkdir $(ARCHITECTURE); fi;\
if test ! -d $(ARCHITECTURE)/include; then mkdir $(ARCHITECTURE)/include; fi;\
touch $(ARCHITECTURE)/include/depend.inc)
include $(ARCHITECTURE)/include/depend.inc
########################################################
else  #-- DSTCDIR not set!

all config depend build install uninstall unbuild unconfig clean clobber \
dist:
	@echo "Error: environment variable DSTCDIR not set."

endif  #-- DSTCDIR


