#
# Makefile for Fumble
#

TARGET = go

#OBJS = Scroller.o MessageView.o FontInfo.o Graphics.o Message.o Hash.o List.o main.o
OBJS = Tickertape.o MessageView.o Message.o List.o main.o

CDEBUGFLAGS =
#CDEBUGFLAGS = -g

#INCLUDES = -I/usr/openwin/include

CFLAGS = $(EXTRACFLAGS) $(CDEBUGFLAGS) $(INCLUDES) $(DEFINES)

LIBDIRS = -L/usr/local/lib
LIBS = -lXaw3d -lXt -lX11

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBDIRS) $(LIBS) $(EXTRALIBS)

clean:
	rm -f $(TARGET) $(OBJS)
