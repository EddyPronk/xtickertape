# First work out the executables
bin_PROGRAMS = xtickertape
bin_SCRIPTS = show-url

# xtickertape has many sources
xtickertape_SOURCES = \
	config.h \
	sanity.h \
	Tickertape.h Tickertape.c \
	Control.h Control.c \
	Scroller.h ScrollerP.h Scroller.c \
	ElvinConnection.h ElvinConnection.c \
	MessageView.h MessageView.c \
	Message.h Message.c \
	Subscription.h Subscription.c \
	UsenetSubscription.h UsenetSubscription.c \
	MailSubscription.h MailSubscription.c \
	OrbitSubscription.h OrbitSubscription.c \
	StringBuffer.h StringBuffer.c \
	FileStreamTokenizer.h FileStreamTokenizer.c \
	Hash.h Hash.c \
	List.h List.c \
	mask.xbm red.xbm white.xbm \
	getopt.h getopt.c getopt1.c \
	main.c

# Indicate what the man pages are
man_MANS = xtickertape.1 show-url.1 groups.5 usenet.5

# Make sure we include the app-defaults file in the distribution
# and also the show-url script
EXTRA_DIST = Tickertape.ad show-url $(man_MANS)


# Put the app-defaults somewhere...
x11libdir = $(libdir)/X11
appdefaultsdir = $(x11libdir)/app-defaults

# Construct Tickertape from Tickertape.ad
install-data-local:
	$(mkinstalldirs) $(appdefaultsdir)
	$(INSTALL_DATA) $(srcdir)/Tickertape.ad $(appdefaultsdir)/Tickertape
