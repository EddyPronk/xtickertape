# Tell aclocal to pick up our m4 macros
ACLOCAL_AMFLAGS = -I m4

# First work out the executables
bin_PROGRAMS = xtickertape show-url

# Don't forget to set up the packagers
SUBDIRS = packages m4

# Automatically generated sources
BUILT_SOURCES = errors.h XTickertape.ad

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
	errors.h replace.h replace.c \
	utf8.h utf8.c \
	globals.h main.c

# show-url only has one
show_url_SOURCES = show-url.c

# Indicate what the man pages are
man_MANS = xtickertape.1 show-url.1 groups.5 keys.5 usenet.5

# Make sure we include the app-defaults file in the distribution
# and also the show-url script
EXTRA_DIST = XTickertape.ad.in show-url $(man_MANS) index en


# Put the app-defaults somewhere...
x11libdir = $(libdir)/X11
appdefaultsdir = $(x11libdir)/app-defaults

# Constructs grammar.h from grammar.pcg
grammar.h: lisp.pcg
	@TPC@ -o $@ $<

# How to construct the error macros
errors.h: index en
	@MCGEN@ -c -i $(srcdir)/index -l $(srcdir)/en -o $@

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
