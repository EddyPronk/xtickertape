# First work out the executables
bin_PROGRAMS = xtickertape
# noinst_PROGRAMS = test-parser
bin_SCRIPTS = show-url

# Automatically generated sources
BUILT_SOURCES = errors.h

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
#	subscription.h subscription.c \
#	ast.h ast.c grammar.h \
#	def-config.h parser.h parser.c \

# test_parser_SOURCES = \
#	message.h message.c \
#	subscription.h subscription.c \
#	ast.h ast.c \
#	grammar.h parser.h parser.c \
#	test-parser.c

# Indicate what the man pages are
man_MANS = xtickertape.1 show-url.1 groups.5 usenet.5

# Make sure we include the app-defaults file in the distribution
# and also the show-url script
EXTRA_DIST = XTickertape.ad show-url $(man_MANS) \
	grammar.pcg index en


# Put the app-defaults somewhere...
x11libdir = $(libdir)/X11
appdefaultsdir = $(x11libdir)/app-defaults

# Constructs grammar.h from grammar.pcg
grammar.h: grammar.pcg
	@TPC@ -o $@ $<

# How to construct the error macros
errors.h: index en
	@MCGEN@ -c -i index -l en -o $@

# Construct XTickertape from XTickertape.ad
install-data-local:
	$(mkinstalldirs) $(appdefaultsdir)
	$(INSTALL_DATA) $(srcdir)/XTickertape.ad $(appdefaultsdir)/XTickertape
