#
# Makefile for Tickertape
#

TICKERTAPE = tickertape
BRIDGE = bridge

ALL = $(TICKERTAPE) $(BRIDGE)

#OBJS = Scroller.o MessageView.o FontInfo.o Graphics.o Message.o Hash.o List.o main.o
TOBJS = BridgeConnection.o Tickertape.o MessageView.o Message.o List.o tickertape.o
BOBJS = ElvinConnection.o List.o Message.o bridge.o

CDEBUGFLAGS =
#CDEBUGFLAGS = -g

#INCLUDES = -I/usr/openwin/include

CFLAGS = $(EXTRACFLAGS) $(CDEBUGFLAGS) $(INCLUDES) $(DEFINES)

LIBDIRS = -L/usr/local/lib
LIBS = -lXaw3d -lXt -lX11

all: $(ALL)

$(TICKERTAPE): $(TOBJS)
	$(CC) -o $@ $(TOBJS) $(LIBDIRS) $(LIBS) $(EXTRALIBS)

$(BRIDGE): $(BOBJS)
	$(CC) -o $@ $(BOBJS) $(LIBDIRS) $(LIBS) $(EXTRALIBS)

clean:
	rm -f $(TARGET) $(OBJS)
