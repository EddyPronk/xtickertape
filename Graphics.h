/* $Id: Graphics.h,v 1.1 1997/02/05 06:24:13 phelps Exp $ */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <X11/Xlib.h>

typedef struct Graphics_t *Graphics;

#include "FontInfo.h"


/* Answers a new Graphics */
Graphics Graphics_alloc(char *displayName, int screen);

/* Releases the receiver's memory */
void Graphics_free(Graphics self);


/* Answers the receiver's file descriptor */
int Graphics_getFD(Graphics self);

/* Answers the receiver's X Display */
Display *Graphics_getDisplay(Graphics self);

/* Answers the receiver's screen number */
int Graphics_getScreen(Graphics self);

/* Answers the receiver's root window */
Window Graphics_getRootWindow(Graphics self);

/* Answers the receiver's default graphics context */
GC Graphics_getGC(Graphics self);

/* Answers the receiver's Black pixel */
unsigned long Graphics_getBlackPixel(Graphics self);

/* Answers the receiver's White pixel */
unsigned long Graphics_getWhitePixel(Graphics self);



/* Sets the foreground color of the receiver */
void Graphics_setForeground(Graphics self, unsigned long pixel);

/* Sets the background color of the receiver */
void Graphics_setBackground(Graphics self, unsigned long pixel);

/* Sets the receiver's font */
void Graphics_setFont(Graphics self, FontInfo fontInfo);



/* Answers a new FontInfo on the named Font */
FontInfo Graphics_allocFontInfo(Graphics self, char *name);

/* Answers a new Color */
unsigned long Graphics_allocPixel(Graphics self, unsigned int red, unsigned int green, unsigned int blue);

/* Answers a new Window */
Window Graphics_allocWindow(Graphics self, Window parent,
			    char *class, char *name, char *title,
			    int x, int y, unsigned int width, unsigned int height,
			    unsigned long background, long inputMask);

/* Answers a new Pixmap */
Pixmap Graphics_allocPixmap(Graphics self, unsigned int width, unsigned int height);

/* Flush the output stream */
void Graphics_flush(Graphics self);

/* Releases a Pixmap */
void Graphics_freePixmap(Graphics self, Pixmap pixmap);

/* Maps a window */
void Graphics_mapWindow(Graphics self, Window window);

#endif /* GRAPHICS_H */
