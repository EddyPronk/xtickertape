/* $Id: Graphics.c,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#include <stdio.h>
#include <stdlib.h>
#include "Graphics.h"
#include <X11/Xutil.h>

struct Graphics_t
{
    Display *display;
    int screen;
    GC gc;
};



/* Answers a new Graphics */
Graphics Graphics_alloc(char *displayName, int screen)
{
    Graphics self = (Graphics) malloc(sizeof(struct Graphics_t));

    /* Connect to the display */
    if ((self -> display = XOpenDisplay(displayName)) == NULL)
    {
	fprintf(stderr, "*** Unable to connect to display \"%s\"\n", displayName);
	exit(1);
    }
    self -> screen = screen;
    self -> gc = DefaultGC(self -> display, self -> screen);

    return self;
}

/* Releases the receiver's memory */
void Graphics_free(Graphics self)
{
    XCloseDisplay(self -> display);
    free(self);
}


/* Answers the receiver's file descriptor */
int Graphics_getFD(Graphics self)
{
    return ConnectionNumber(self -> display);
}

/* Answers the receiver's X Display */
Display *Graphics_getDisplay(Graphics self)
{
    return self -> display;
}

/* Answers the receiver's screen number */
int Graphics_getScreen(Graphics self)
{
    return self -> screen;
}

/* Answers the receiver's root window */
Window Graphics_getRootWindow(Graphics self)
{
    return RootWindow(self -> display, self -> screen);
}

/* Answers the receiver's default graphics context */
GC Graphics_getGC(Graphics self)
{
    return self -> gc;
}

/* Answers the receiver's Black pixel */
unsigned long Graphics_getBlackPixel(Graphics self)
{
    return BlackPixel(self -> display, self -> screen);
}

/* Answers the receiver's White pixel */
unsigned long Graphics_getWhitePixel(Graphics self)
{
    return WhitePixel(self -> display, self -> screen);
}


/* Sets the foreground color of the receiver */
void Graphics_setForeground(Graphics self, unsigned long pixel)
{
    GC gc = Graphics_getGC(self);
    XGCValues values;

    values.foreground = pixel;
    XChangeGC(self -> display, gc, GCForeground, &values);
}

/* Sets the background color of the receiver */
void Graphics_setBackground(Graphics self, unsigned long pixel)
{
    GC gc = Graphics_getGC(self);
    XGCValues values;

    values.background = pixel;
    XChangeGC(self -> display, gc, GCBackground, &values);
}



/* Sets the receiver's font */
void Graphics_setFont(Graphics self, FontInfo fontInfo)
{
    GC gc = Graphics_getGC(self);
    XGCValues values;

    values.font = FontInfo_getFont(fontInfo);
    XChangeGC(self -> display, gc, GCFont, &values);
}



/* Answers a new FontInfo on the named Font */
FontInfo Graphics_allocFontInfo(Graphics self, char *name)
{
    XFontStruct *fs;

    if ((fs = XLoadQueryFont(self -> display, name)) == NULL)
    {
	return NULL;
    }
    else
    {
	return FontInfo_alloc(fs);
    }
}



/* Answers a new Color */
unsigned long Graphics_allocPixel(Graphics self, unsigned int red, unsigned int green, unsigned int blue)
{
    XColor color;

    color.red = red;
    color.green = green;
    color.blue = blue;
    color.flags = DoRed | DoGreen | DoBlue;
    XAllocColor(self -> display, DefaultColormap(self -> display, self -> screen), &color);
    return color.pixel;
}


/* Answers a new Window */
Window Graphics_allocWindow(Graphics self, Window parent,
			    char *class, char *name, char *title,
			    int x, int y, unsigned int width, unsigned int height,
			    unsigned long background, long inputMask)
{
    Window p, window;
    XSizeHints *sizeHints = XAllocSizeHints();
    XWMHints *wmHints = XAllocWMHints();
    XClassHint *classHints = XAllocClassHint();
    XTextProperty winName;

    p = (parent == None) ? Graphics_getRootWindow(self) : parent;
    window = XCreateSimpleWindow(self -> display, p, x, y, width, height, 0, 0, background);
    XSelectInput(self -> display, window, inputMask);

    /* Set WM hints */
    sizeHints -> flags = 0;
    wmHints -> flags = 0;
    classHints -> res_name = name;
    classHints -> res_class = class;

    /* Set title */
    if (XStringListToTextProperty(&title, 1, &winName) == 0)
    {
	fprintf(stderr, "*** horribly broken\n");
	exit(1);
    }

    XSetWMProperties(self -> display, window, &winName, &winName, NULL, 0, 
		     sizeHints, wmHints, classHints);
    
    return window;
}

/* Answers a new Pixmap */
Pixmap Graphics_allocPixmap(Graphics self, unsigned int width, unsigned int height)
{
    return XCreatePixmap(
	self -> display,
	Graphics_getRootWindow(self),
	width, height,
	DefaultDepth(self -> display, self -> screen));
}


/* Releases a Pixmap */
void Graphics_freePixmap(Graphics self, Pixmap pixmap)
{
    XFreePixmap(self -> display, pixmap);
}


/* Flush the output stream */
void Graphics_flush(Graphics self)
{
    XFlush(self -> display);
}



/* Maps a window */
void Graphics_mapWindow(Graphics self, Window window)
{
    XMapWindow(self -> display, window);
}
