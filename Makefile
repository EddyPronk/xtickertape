#
# Makefile for Tickertape
#

TICKERTAPE = tickertape
CONNECTION = ElvinConnection.o

ALL = $(TICKERTAPE) 

OBJS = $(CONNECTION) \
	Control.o Tickertape.o \
	MessageView.o Message.o Subscription.o List.o main.o

CDEBUGFLAGS =
#CDEBUGFLAGS = -g

#INCLUDES = -I/usr/openwin/include

CFLAGS = $(EXTRACFLAGS) $(CDEBUGFLAGS) $(INCLUDES) $(DEFINES)

LIBDIRS = -L/usr/local/lib
LIBS = -lXaw -lXt -lX11

all: $(ALL)

$(TICKERTAPE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBDIRS) $(LIBS) $(EXTRALIBS)

clean:
	rm -f $(TARGET) $(OBJS)


