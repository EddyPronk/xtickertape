#
# Makefile for Tickertape
#

TICKERTAPE = tickertape

ALL = $(TICKERTAPE) 

OBJS = BridgeConnection.o Control.o Tickertape.o \
	MessageView.o Message.o List.o main.o

CDEBUGFLAGS =
#CDEBUGFLAGS = -g

#INCLUDES = -I/usr/openwin/include

CFLAGS = $(EXTRACFLAGS) $(CDEBUGFLAGS) $(INCLUDES) $(DEFINES)

LIBDIRS = -L/usr/local/lib
LIBS = -lXaw3d -lXt -lX11

all: $(ALL)

$(TICKERTAPE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBDIRS) $(LIBS) $(EXTRALIBS)

clean:
	rm -f $(TARGET) $(OBJS)


