/***************************************************************

  Copyright (C) DSTC Pty Ltd (ACN 052 372 577) 2001.
  Unpublished work.  All Rights Reserved.

  The software contained on this media is the property of the
  DSTC Pty Ltd.  Use of this software is strictly in accordance
  with the license agreement in the accompanying LICENSE.DOC
  file.  If your distribution of this software does not contain
  a LICENSE.DOC file then you have no rights to use this
  software in any manner and should contact DSTC at the address
  below to determine an appropriate licensing arrangement.

     DSTC Pty Ltd
     Level 7, GP South
     University of Queensland
     St Lucia, 4072
     Australia
     Tel: +61 7 3365 4310
     Fax: +61 7 3365 4311
     Email: enquiries@dstc.edu.au

  This software is being provided "AS IS" without warranty of
  any kind.  In no event shall DSTC Pty Ltd be liable for
  damage of any kind arising out of or in connection with
  the use or performance of this software.

****************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: utf8.c,v 1.5 2003/01/17 16:00:17 phelps Exp $";
#endif /* lint */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/* #ifdef HAVE_STDIO_H */
#include <stdio.h>
/* #endif */
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif
/* #ifdef HAVE_ICONV_H */
#include <iconv.h>
/* #endif */
/* #ifdef HAVE_ERRNO_H */
#include <errno.h>
/* #endif */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "globals.h"
#include "utf8.h"

/* The CHARSET_REGISTRY atom */
#define CHARSET_REGISTRY "CHARSET_REGISTRY"
static Atom registry = None;

/* The CHARSET_ENCODING atom */
#define CHARSET_ENCODING "CHARSET_ENCODING"
static Atom encoding = None;

/* The name of the UTF-8 code set that works with iconv() */
#define UTF8_CODE "UTF-8"

/* The size of the iconv() conversion buffer used when measuring and
 * displaying strings. */
#define BUFFER_SIZE 1024

/* The maximum number of bytes per character */
#define MAX_CHAR_SIZE 2


/* The code set name we'd expect for ISO10646-1 fonts */
#define ISO10646 "ISO10646-1"

/* The code set name that works for ISO10646-1 fonts */
#define UCS2BE "UCS-2BE"


/* The empty character */
static XCharStruct empty_char = { 0, 0, 0, 0, 0, 0 };


/* Guess the name of a font's code set */
static char *guess_font_code_set(Display *display, XFontStruct *font)
{
    Atom atoms[2];
    char *names[2];
    char *string;
    size_t length;

    /* Look up the CHARSET_REGISTRY atom */
    if (registry == None)
    {
	registry = XInternAtom(display, CHARSET_REGISTRY, False);
    }

    /* Look up the font's CHARSET_REGISTRY property */
    if (! XGetFontProperty(font, registry, &atoms[0]))
    {
	return NULL;
    }

    /* Look up the CHARSET_ENCODING atom */
    if (encoding == None)
    {
	encoding = XInternAtom(display, CHARSET_ENCODING, False);
    }

    /* Look up the font's CHARSET_ENCODING property */
    if (! XGetFontProperty(font, encoding, &atoms[1]))
    {
	return NULL;
    }

    /* Stringify the names */
    if (! XGetAtomNames(display, atoms, 2, names))
    {
	return NULL;
    }

    /* Allocate some memory to hold the code set name */
    length = strlen(names[0]) + 1 + strlen(names[1]) + 1;
    if ((string = malloc(length)) == NULL)
    {
	XFree(names[0]);
	XFree(names[1]);
	return NULL;
    }

    /* Construct the code set name */
    sprintf(string, "%s-%s", names[0], names[1]);

    /* Clean up a bit */
    XFree(names[0]);
    XFree(names[1]);

    /* HACK: Replace ISO10646-1 with UCS-2BE */
    if (strcmp(string, ISO10646) == 0)
    {
	strcpy(string, UCS2BE);
    }

    return string;
}


/* Information used to display a UTF-8 string in a given font */
struct utf8_renderer
{
    /* The font to use */
    XFontStruct *font;

    /* The conversion descriptor used to transcode from UTF-8 to the
     * code set used by the font */
    iconv_t cd;

    /* If non-zero then we're skipping a troublesome UTF-8 character */
    int is_skipping;

    /* The number of bytes per character in the font's code set */
    int dimension;

    /* The thickness of an underline */
    long underline_thickness;

    /* The position of an underline */
    long underline_position;
};

/* Answers the statistics to use for a given character in the font */
static XCharStruct *per_char(
    XFontStruct *font,
    unsigned char byte1,
    unsigned char byte2)
{
    XCharStruct *info;

    /* Fixed-width font? */
    if (font -> per_char == NULL)
    {
	return &font -> max_bounds;
    }

    /* Is the character present? */
    if (font -> min_byte1 <= byte1 && byte1 <= font -> max_byte1 &&
	font -> min_char_or_byte2 <= byte2 && byte2 <= font -> max_char_or_byte2)
    {
	/* Look up the character info */
	info = font -> per_char +
	    (byte1 - font -> min_byte1) * (font -> max_char_or_byte2 - font -> min_char_or_byte2 + 1) +
	    byte2 - font -> min_char_or_byte2;

	/* If the bounding box is non-zero then use it */
	if (info -> width != 0)
	{
	    return info;
	}
    }

    /* Is the default charater present? */
    byte1 = font -> default_char >> 8;
    byte2 = font -> default_char & 0xFF;
    if (font -> min_byte1 <= byte1 && byte1 <= font -> max_byte1 &&
	font -> min_char_or_byte2 <= byte2 && byte2 <= font -> max_char_or_byte2)
    {
	/* Look up the character info */
	info = font -> per_char +
	    (byte1 - font -> min_byte1) * (font -> max_char_or_byte2 - font -> min_char_or_byte2 + 1) +
	    byte2 - font -> min_char_or_byte2;

	/* If the bounding box is non-zero then use it */
	if (info -> width != 0)
	{
	    return info;
	}
    }

    /* If all else fails then use an empty character */
    return &empty_char;
}


/* Returns the number of bytes required to encode a single character
 * in the output encoding */
static int cd_dimension(iconv_t cd)
{
    char buffer[MAX_CHAR_SIZE];
    char *string = "a";
    char *point = buffer;
    size_t in_length = 1;
    size_t out_length = MAX_CHAR_SIZE;

    /* Try to encode one character */
    if (iconv(cd, &string, &in_length, &point, &out_length) == (size_t)-1)
    {
	return -1;
    }

    /* Fail if the encoding was not complete */
    if (in_length != 0)
    {
	return -1;
    }

    /* Otherwise return the number of characters required */
    return point - buffer;
}

/* Returns an iconv conversion descriptor for converting characters to
 * be displayed in a given font from a given code set.  If tocode is
 * non-NULL then it will be used, otherwise an attempt will be made to
 * guess the font's encoding */
utf8_renderer_t utf8_renderer_alloc(
    Display *display,
    XFontStruct *font,
    const char *tocode)
{
    utf8_renderer_t self;
    iconv_t cd;
    int dimension;
    char *string;
    unsigned long value;

    /* Allocate room for the new utf8_renderer */
    if ((self = (utf8_renderer_t)malloc(sizeof(struct utf8_renderer))) == NULL)
    {
	return NULL;
    }

    /* Set its fields to sane values */
    self -> font = font;
    self -> cd = (iconv_t)-1;
    self -> is_skipping = 0;
    self -> dimension = 1;

    /* Is there a font property for underline thickness? */
    if (! XGetFontProperty(font, XA_UNDERLINE_THICKNESS, &value))
    {
	/* Make something up */
	self -> underline_thickness = MAX((font -> ascent + font -> descent + 10L) / 20, 1);
    }
    else
    {
	/* Yes: use it */
	self -> underline_thickness = MAX(value, 1);
    }

    /* Is there a font property for the underline position? */
    if (! XGetFontProperty(font, XA_UNDERLINE_POSITION, &value))
    {
	/* Make up something plausible */
	self -> underline_position = MAX((font -> descent + 4L) / 8 , 1);
    }
    else
    {
	/* Yes!  Use it. */
	self -> underline_position = MAX(value, 2);
    }

    /* Was an encoding provided? */
    if (tocode != NULL)
    {
	/* Yes.  Use it to create a conversion descriptor */
	if ((cd = iconv_open(tocode, UTF8_CODE)) == (iconv_t)-1)
	{
	    fprintf(stderr, "Unable convert from %s to %s: %s\n",
		    tocode, UTF8_CODE, strerror(errno));
	    return self;
	}

	/* Encode a single character to get the dimension */
	if ((dimension = cd_dimension(cd)) < 0)
	{
	    iconv_close(cd);
	    return self;
	}

	self -> cd = cd;
	self -> dimension = dimension;
	return self;
    }


    /* Guess the font's code set */
    if ((string = guess_font_code_set(display, font)) == NULL)
    {
	return self;
    }

    /* Open a conversion descriptor */
    if ((cd = iconv_open(string, UTF8_CODE)) == (iconv_t)-1)
    {
	self -> dimension = 1;
	free(string);
	return self;
    }

    /* Clean up some more */
    free(string);

    /* Try to encode a single character */
    if ((dimension = cd_dimension(cd)) == 0)
    {
	iconv_close(cd);
	return self;
    }

    /* Successful guess! */
    self -> cd = cd;
    self -> dimension = dimension;
    return self;
}

/* Wrapper around iconv() to catch most of the nasty gotchas */
static size_t utf8_renderer_iconv(
    utf8_renderer_t self,
    char **inbuf, size_t *inbytesleft,
    char **outbuf, size_t *outbytesleft)
{
    size_t count;

    /* An unsupported conversion becomse UTF-8 to ASCII */
    if (self -> cd == (iconv_t)-1)
    {
	/* This is a stateless translation */
	if (inbytesleft == NULL || outbytesleft == NULL)
	{
	    return 0;
	}

	/* Keep going until we're out of room */
	while (*inbytesleft && *outbytesleft)
	{
	    int ch;

	    /* Get the next character */
	    ch = *(*inbuf)++;
	    (*inbytesleft)--;

	    /* If it's the beginning of a multibyte character then
	     * replace it with the default char */
	    if ((ch & 0xc0) == 0xc0)
	    {
		*(*outbuf)++ = self -> font -> default_char;
		(*outbytesleft)--;
	    }
	    else if ((ch & 0xc0) != 0x80)
	    {
		*(*outbuf)++ = ch;
		(*outbytesleft)--;
	    }
	}
	
	return 0;
    }

    /* Watch for a NULL transformation */
    if (inbytesleft == NULL || outbytesleft == NULL)
    {
	self -> is_skipping = 0;
	return iconv(self -> cd, inbuf, inbytesleft, outbuf, outbytesleft);
    }

    /* Keep going even when we encounter invalid or unrepresentable
     * characters (this assumes UTF-8 as the input code set */
    count = 0;
    while (*inbytesleft && *outbytesleft)
    {
	size_t n;

	/* Skip the rest of an untranslatable character */
	if (self -> is_skipping)
	{
	    while (*inbytesleft > 0 && (*(*inbuf) & 0xc0) == 0x80)
	    {
		(*inbuf)++;
		(*inbytesleft)--;
	    }

	    /* Bail if we're out of input */
	    if (*inbytesleft == 0)
	    {
		break;
	    }

	    self -> is_skipping = 0;
	}

	/* Try to convert the sequence */
	if ((n = iconv(self -> cd, inbuf, inbytesleft, outbuf, outbytesleft)) == (size_t)-1)
	{
	    switch (errno)
	    {
		case E2BIG:
		{
		    return count;
		}

		case EILSEQ:
		{
		    errno = 0;

		    /* Skip the first byte of the untranslatable character */
		    (*inbuf)++;
		    (*inbytesleft)--;

		    /* Insert the default character */
		    if (self -> dimension == 1)
		    {
			*(*outbuf)++ = self -> font -> default_char;
			(*outbytesleft)--;
		    }
		    else
		    {
			*(*outbuf)++ = self -> font -> default_char >> 8;
			*(*outbuf)++ = self -> font -> default_char & 0xFF;
			(*outbytesleft) += 2;
		    }

		    count++;

		    /* Reset the conversion descriptor */
		    iconv(self -> cd, NULL, NULL, NULL, NULL);

		    /* Arrange to skip the rest of the character */
		    self -> is_skipping = 1;
		    break;
		}

		default:
		{			    
		    perror("iconv() failed");
		    return (size_t)-1;
		}
	    }
	}
	else
	{
	    count += n;
	}
    }

    return count;
}

/* Measures all of the characters in a string */
void utf8_renderer_measure_string(
    utf8_renderer_t self,
    char *string,
    string_sizes_t sizes)
{
    char buffer[BUFFER_SIZE];
    XCharStruct *info;
    long lbearing = 0;
    long rbearing = 0;
    long width = 0;
    char *out_point;
    size_t in_length;
    size_t out_length;
    int is_first;

    /* Count the number of bytes in the string */
    in_length = strlen(string);

    /* Keep going until we get the whole string */
    is_first = True;
    while (in_length != 0)
    {
	/* Convert the string into the display code set */
	out_length = BUFFER_SIZE;
	out_point = buffer;
	if (utf8_renderer_iconv(
		self,
		&string, &in_length,
		&out_point, &out_length) == (size_t)-1 &&
	    errno != E2BIG)
	{
	    /* This shouldn't fail */
	    abort();
	}

	/* Measure the characters */
	if (self -> dimension == 1)
	{
	    unsigned char *point = (unsigned char *)buffer;

	    /* Set the initial measurements */
	    if (is_first)
	    {
		is_first = False;
		info = per_char(self -> font, 0, *point);
		lbearing = info -> lbearing;
		rbearing = info -> rbearing;
		width = info -> width;
		point++;
	    }

	    /* Adjust for the rest of the string */
	    while (point < (unsigned char *)out_point)
	    {
		info = per_char(self -> font, 0, *point);
		lbearing = MIN(lbearing, width + (long)info -> lbearing);
		rbearing = MAX(rbearing, width + (long)info -> rbearing);
		width += (long)info -> width;
		point++;
	    }
	}
	else
	{
	    XChar2b *point = (XChar2b *)buffer;

	    /* Set the initial measurements */
	    if (is_first)
	    {
		is_first = False;
		info = per_char(self -> font, point -> byte1, point -> byte2);
		lbearing = info -> lbearing;
		rbearing = info -> rbearing;
		width = info -> width;
		point++;
	    }

	    /* Adjust for the rest of the string */
	    while (point < (XChar2b *)out_point)
	    {
		info = per_char(self -> font, point -> byte1, point -> byte2);
		lbearing = MIN(lbearing, width + (long)info -> lbearing);
		rbearing = MAX(rbearing, width + (long)info -> rbearing);
		width += (long)info -> width;
		point++;
	    }
	}
    }

    /* Record our findings */
    sizes -> lbearing = lbearing;
    sizes -> rbearing = rbearing;
    sizes -> width = width;
    sizes -> ascent = self -> font -> ascent;
    sizes -> descent = 
	MAX(
	    self -> font -> descent,
	    self -> underline_position + self -> underline_thickness);
}

/* Draw a string within the bounding box, measuring the characters so
 * as to minimize bandwidth requirements */
void utf8_renderer_draw_string(
    Display *display,
    Drawable drawable,
    GC gc,
    utf8_renderer_t renderer,
    int x, int y,
    XRectangle *bbox,
    char *string)
{
    char buffer[BUFFER_SIZE];
    XCharStruct *info;
    long left, right;
    char *out_point;
    size_t in_length;
    size_t out_length;

    /* Count the number of bytes in the string */
    in_length = strlen(string);

    /* Skip over characters outside the bounding box */
    left = x;
    while (in_length != 0)
    {
	/* Convert the string into the font's code set */
	out_length = BUFFER_SIZE;
	out_point = buffer;
	if (utf8_renderer_iconv(
		renderer,
		&string, &in_length,
		&out_point, &out_length) == (size_t)-1 &&
	    errno != E2BIG)
	{
	    /* This shouldn't fail */
	    abort();
	}

	/* Are characters in this code set one byte wide? */
	if (renderer -> dimension == 1)
	{
	    unsigned char *first;
	    unsigned char *last;

	    /* Look for visible characters in the buffer */
	    first = (unsigned char *)buffer;
	    while (first < (unsigned char *)out_point)
	    {
		info = per_char(renderer -> font, 0, *first);

		/* Skip anything to the left of the bounding box */
		if (left + info -> rbearing < bbox -> x)
		{
		    left = left + (long)info -> width;
		    first++;
		}
		else
		{
		    /* Look for the last visible character */
		    last = first;
		    right = left;
		    while (last < (unsigned char *)out_point)
		    {
			info = per_char(renderer -> font, 0, *last);

			if (right + info -> lbearing < bbox -> x + bbox -> width)
			{
			    right = right + (long)info -> width;
			    last++;
			}
			else
			{
			    /* Reset the conversion descriptor */
			    if (utf8_renderer_iconv(
				    renderer,
				    NULL, NULL,
				    NULL, NULL) == (size_t)-1)
			    {
				abort();
			    }

			    XDrawString(display, drawable, gc, left, y, first, last - first);
			    return;
			}
		    }

		    /* Still visible at the end of the buffer */
		    XDrawString(display, drawable, gc, left, y, first, last - first);
		    first = last;
		    left = right;
		}
	    }
	}
	else
	{
	    XChar2b *first;
	    XChar2b *last;

	    /* Look for visible characters in the buffer */
	    first = (XChar2b *)buffer;
	    while (first < (XChar2b *)out_point)
	    {
		info = per_char(renderer -> font, first -> byte1, first -> byte2);

		if (left + info -> rbearing < bbox -> x)
		{
		    left = left + (long)info -> width;
		    first++;
		}
		else
		{
		    /* Look for the last visible character */
		    last = first;
		    right = left;
		    while (last < (XChar2b *)out_point)
		    {
			info = per_char(renderer -> font, last -> byte1, last -> byte2);

			if (right + info -> lbearing < bbox -> x + bbox -> width)
			{
			    right = right + (long)info -> width;
			    last++;
			}
			else
			{
			    /* Reset the conversion descriptor */
			    if (utf8_renderer_iconv(
				    renderer,
				    NULL, NULL,
				    NULL, NULL) == (size_t)-1)
			    {
				abort();
			    }

			    XDrawString16(display, drawable, gc, left, y, first, last - first);
			    return;
			}
		    }

		    /* Still visible at the end of the buffer */
		    XDrawString16(display, drawable, gc, left, y, first, last - first);
		    first = last;
		    left = right;
		}
	    }
	}
    }
}

/* Underline a string within the bounding box, measuring the
 * characters so as to minimize bandwidth requirements */
void utf8_renderer_draw_underline(
    utf8_renderer_t self,
    Display *display,
    Drawable drawable,
    GC gc,
    int x, int y,
    XRectangle *bbox,
    long width)
{
    XFillRectangle(
	display, drawable, gc,
	MAX(x, bbox -> x),
	y + self -> underline_position,
	MIN(x + width, (long)bbox -> x + (long)bbox -> width) - MAX(x, bbox -> x),
	self -> underline_thickness);
}




/* The structure of the UTF-8 encoder */
struct utf8_encoder
{
    /* The conversion descriptor */
    iconv_t cd;
};

/* Allocate and initialize a utf8_encoder */
utf8_encoder_t utf8_encoder_alloc(
    Display *display,
    XmFontList font_list,
    char *code_set)
{
    utf8_encoder_t self;
    XmFontContext context;
    XmStringCharSet charset;
    XFontStruct *font = NULL;
    char *string;

    /* Allocate memory for a utf8_encoder */
    if ((self = (utf8_encoder_t)malloc(sizeof(struct utf8_encoder))) == NULL)
    {
	return NULL;
    }

    /* Initialize with a sane value */
    self -> cd = (iconv_t)-1;

    /* If a code set was provided then use it */
    if (code_set != NULL)
    {
	if ((self -> cd = iconv_open(UTF8_CODE, code_set)) == (iconv_t)-1)
	{
	    perror("iconv_open() failed\n");
	}

	return self;
    }

    /* Go through the font list looking for a font */
    if (! XmFontListInitFontContext(&context, font_list))
    {
	perror("XmFontListInitFontContext() failed");
	return self;
    }

    /* Look for a font */
    while (1)
    {
	/* Get the next font from the font list */
	if (! XmFontListGetNextFont(context, &charset, &font))
	{
	    perror("XmFontListGetNextFont() failed");
	    XmFontListFreeFontContext(context);
	    return self;
	}

	/* Guess it's code set name */
	if ((string = guess_font_code_set(display, font)) == NULL)
	{
	    continue;
	}

	/* Open a conversion descriptor */
	if ((self -> cd = iconv_open(UTF8_CODE, string)) == (iconv_t)-1)
	{
	    perror("iconv_open() failed");
	}

	free(string);

	XmFontListFreeFontContext(context);
	return self;
    }
}

/* Encodes a string */
char *utf8_encoder_encode(utf8_encoder_t self, char *input)
{
    char *buffer;
    char *output;
    size_t in_length;
    size_t out_length;
    size_t length;

    /* Measure the input string */
    in_length = strlen(input) + 1;

    /* Special case for empty strings */
    if (in_length == 0)
    {
	return strdup("");
    }

    /* If we have no encoder then try a simple memcpy and hope for the best */
    if (self -> cd == (iconv_t)-1)
    {
	if ((output = (char *)malloc(length)) == NULL)
	{
	    perror("malloc() failed");
	    return NULL;
	}

	memcpy(output, input, in_length);
	return output;
    }

    /* Guess twice as much should suffice for the output */
    length = 2 * in_length;
    if ((buffer = (char *)malloc(length)) == NULL)
    {
	perror("malloc() failed");
	return NULL;
    }

    output = buffer;
    out_length = length;

    /* Otherwise encode away! */
    while (in_length != 0)
    {
	if (iconv(self -> cd, &input, &in_length, &output, &out_length) == (size_t)-1)
	{
	    /* If we're out of space then allocate some more */
	    if (errno == E2BIG)
	    {
		char *new_buffer;
		size_t new_length;

		new_length = length * 2;
		if ((new_buffer = realloc(buffer, new_length)) == NULL)
		{
		    perror("realloc() failed");
		    free(output);
		    return strdup("");
		}

		out_length += new_length - length;
		output = new_buffer + (output - buffer);

		buffer = new_buffer;
		length = new_length;
	    }
	    else
	    {
		perror("iconv() failed");
		return strdup("");
	    }
	}

	*output = '\0';
    }

    return buffer;
}


