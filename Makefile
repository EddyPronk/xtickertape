#
# Makefile for Fumble
#

TARGET = go

OBJS = Scroller.o MessageView.o FontInfo.o Graphics.o Message.o Hash.o List.o main.o

#CDEBUGFLAGS =
CDEBUGFLAGS = -g

INCLUDES = -I/usr/openwin/include

CFLAGS = $(CCFLAGS) $(CDEBUGFLAGS) $(INCLUDES) $(DEFINES)

#LIBS = -L/usr/local/lib -lX11 
LIBS = -lX11

$(TARGET): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

clean:
	rm -f $(TARGET) $(OBJS)
