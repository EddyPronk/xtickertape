# First work out the executables
bin_PROGRAMS = xtickertape
bin_SCRIPTS = show-url

# xtickertape has many sources
xtickertape_SOURCES = \
	glyph.h gap.c message_glyph.c \
	tickertape.h tickertape.c \
	history.h history.c \
	panel.h panel.c \
	Scroller.h ScrollerP.h Scroller.c \
	message.h message.c \
	groups.h groups_parser.h groups_parser.c \
	group_sub.h group_sub.c \
	usenet.h usenet_parser.h usenet_parser.c \
	usenet_sub.h usenet_sub.c \
	mbox_parser.h mbox_parser.c mail_sub.h mail_sub.c \
	orbit_sub.h orbit_sub.c \
	mask.xbm red.xbm white.xbm \
	getopt.h getopt.c getopt1.c \
	main.c

# Indicate what the man pages are
man_MANS = xtickertape.1 show-url.1 groups.5 usenet.5

# Make sure we include the app-defaults file in the distribution
# and also the show-url script
EXTRA_DIST = XTickertape.ad show-url $(man_MANS)


# Put the app-defaults somewhere...
x11libdir = $(libdir)/X11
appdefaultsdir = $(x11libdir)/app-defaults

# Construct XTickertape from XTickertape.ad
install-data-local:
	$(mkinstalldirs) $(appdefaultsdir)
	$(INSTALL_DATA) $(srcdir)/XTickertape.ad $(appdefaultsdir)/XTickertape
