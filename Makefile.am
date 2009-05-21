#
#   Copyright (C) 1997-2009 Mantara Software (ABN 17 105 665 594).
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions
#   are met:
#
#   * Redistributions of source code must retain the above
#     copyright notice, this list of conditions and the following
#     disclaimer.
#
#   * Redistributions in binary form must reproduce the above
#     copyright notice, this list of conditions and the following
#     disclaimer in the documentation and/or other materials
#     provided with the distribution.
#
#   * Neither the name of the Mantara Software nor the names
#     of its contributors may be used to endorse or promote
#     products derived from this software without specific prior
#     written permission. 
#
#   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
#   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.
#

# Tell aclocal to pick up our m4 macros
ACLOCAL_AMFLAGS = -I m4

# First work out the executables
bin_PROGRAMS = xtickertape show-url elvinmail

# Don't forget to set up the packagers
SUBDIRS = packages m4

# Automatically generated sources
BUILT_SOURCES = XTickertape.ad

# xtickertape has many sources
xtickertape_SOURCES = \
	message_view.h message_view.c \
	tickertape.h tickertape.c \
	panel.h panel.c \
	Scroller.h ScrollerP.h Scroller.c \
	message.h message.c \
	groups.h groups_parser.h groups_parser.c \
	group_sub.h group_sub.c \
	History.h HistoryP.h History.c \
	usenet.h usenet_parser.h usenet_parser.c \
	usenet_sub.h usenet_sub.c \
	keys.h keys_parser.h keys_parser.c \
	key_table.h key_table.c \
	mbox_parser.h mbox_parser.c mail_sub.h mail_sub.c \
	mask.xbm red.xbm white.xbm \
	ref.h ref.c \
	replace.h replace.c \
	utf8.h utf8.c \
	utils.h utils.c \
	globals.h main.c

# show-url only has one
show_url_SOURCES = show-url.c

elvinmail_SOURCES = elvinmail.c parse_mail.h parse_mail.c

# Indicate what the man pages are
man_MANS = xtickertape.1 show-url.1 groups.5 keys.5 usenet.5

# Make sure we include the app-defaults file in the distribution
EXTRA_DIST = XTickertape.ad.in $(man_MANS)


# Put the app-defaults somewhere...
x11libdir = $(libdir)/X11
appdefaultsdir = $(x11libdir)/app-defaults

# Constructs grammar.h from grammar.pcg
grammar.h: lisp.pcg
	@TPC@ -o $@ $<

# How to construct the error macros
#errors.h: index en
#	@MCGEN@ -c -i $(srcdir)/index -l $(srcdir)/en -o $@

# Construct XTickertape from XTickertape.ad
install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(appdefaultsdir)
	$(INSTALL_DATA) XTickertape.ad $(DESTDIR)$(appdefaultsdir)/XTickertape

# Use fake targets for the various packages
.PSEUDO: deb rpm

# Build an RPM
rpm: dist
	(cd packages/rpm; make rpm)

# Build a Debian package
deb: dist
	(cd packages/deb; make deb)
