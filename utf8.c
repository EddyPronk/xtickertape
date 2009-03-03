/***********************************************************************

  Copyright (C) 1997-2009 by Mantara Software (ABN 17 105 665 594).
  All Rights Reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   * Redistributions of source code must retain the above
     copyright notice, this list of conditions and the following
     disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials
     provided with the distribution.

   * Neither the name of the Mantara Software nor the names
     of its contributors may be used to endorse or promote
     products derived from this software without specific prior
     written permission. 

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
   FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
   REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
   BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
   LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
   ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.

***********************************************************************/

#ifndef lint
static const char cvsid[] = "$Id: utf8.c,v 1.17 2009/03/09 05:26:27 phelps Exp $";
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
#ifdef HAVE_ICONV_H
#include <iconv.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "globals.h"
#include "utf8.h"

/* Make sure EILSEQ is defined for our switch statement */
#ifndef EILSEQ
#define EILSEQ -1
#endif /* EILSEQ */

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


/* The format of a guesses table entry */
struct guess
{
    /* The name of the font's registry-encoding */
    char *code;

    /* The guess strings/formats */
    char *guesses[8];
};


/* The guesses table */
struct guess guesses[] =
{
    { "UTF-8", { "utf8", NULL } },
    { "ISO10646-1", { "UCS-2BE", "ucs2", NULL } },
    { "ISO8859-1", { "ISO-8859-1", "iso8859_1", NULL } },
    { "ISO8859-2", { "ISO-8859-2", "iso8859_2", NULL } },
    { "ISO8859-3", { "ISO-8859-3", "iso8859_3", NULL } },
    { "ISO8859-4", { "ISO-8859-4", "iso8859_4", NULL } },
    { "ISO8859-5", { "ISO-8859-5", "iso8859_5", NULL } },
    { "ISO8859-6", { "ISO-8859-6", "iso8859_6", NULL } },
    { "ISO8859-7", { "ISO-8859-7", "iso8859_7", NULL } },
    { "ISO8859-8", { "ISO-8859-8", "iso8859_8", NULL } },
    { "ISO8859-9", { "ISO-8859-9", "iso8859_9", NULL } },
    { "ISO8859-10", { "ISO-8859-10", "iso8859_10", NULL } },
    { "ISO8859-11", { "ISO-8859-11", "iso8859_11", NULL } },
    { "ISO8859-12", { "ISO-8859-12", "iso8859_12", NULL } },
    { "ISO8859-13", { "ISO-8859-13", "iso8859_13", NULL } },
    { "ISO8859-14", { "ISO-8859-14", "iso8859_14", NULL } },
    { "ISO8859-15", { "ISO-8859-15", "iso8859_15", NULL } },
    { "ISO8859-16", { "ISO-8859-16", "iso8859_16", NULL } },
    { NULL, { NULL } }
};


/* Make sure an iconv_t type is defined */
/* FIX THIS: this should be based on iconv_t */
#ifndef HAVE_ICONV_H
typedef void * iconv_t;
#endif

/* The empty character */
static XCharStruct empty_char = { 0, 0, 0, 0, 0, 0 };

#ifdef HAVE_ICONV
/* Locates the guess with the given name in the guesses table */
static struct guess *find_guess(const char *code)
{
    struct guess *guess = guesses;

    /* Go through the guesses */
    for (guess = guesses; guess -> code != NULL; guess++)
    {
        if (strcmp(guess -> code, code) == 0)
        {
            return guess;
        }
    }

    return NULL;
}

/* Uses the from and to code set names to open a conversion
 * descriptor.  Since different implementations of iconv() may use
 * different strings to represent a given code set, we try a bunch of
 * them until one works. */
static iconv_t do_iconv_open(const char *tocode, const char *fromcode)
{
    struct guess *to_guess;
    struct guess *from_guess;
    const char *to_string;
    const char *from_string;
    iconv_t cd;
    int i, j;

    /* Look up the guesses */
    to_guess = find_guess(tocode);
    from_guess = find_guess(fromcode);

    i = 0;
    to_string = tocode;
    while (to_string != NULL)
    {
        j = 0;
        from_string = fromcode;
        while (from_string != NULL)
        {
            /* Try this combination */
            fflush(stderr);
            if ((cd = iconv_open(to_string, from_string)) != (iconv_t)-1)
            {
                return cd;
            }

            /* Get the next fromstring */
            from_string = from_guess ? from_guess -> guesses[j++] : NULL;
        }

        /* Get the next tostring */
        to_string = to_guess ? to_guess -> guesses[i++] : NULL;
    }

    /* Give up */
    fprintf(stderr, "Unable to convert from %s to %s: %s\n",
            fromcode, tocode, strerror(errno));
    return (iconv_t)-1;
}
#else /* !HAVE_ICONV */
static iconv_t do_iconv_open(const char *tocode, const char *fromcode)
{
    return (iconv_t)-1;
}
#endif /* HAVE_ICONV */

#ifndef HAVE_ICONV
size_t iconv(
    iconv_t cd,
    char **inbuf, size_t *inbytesleft,
    char **outbuf, size_t *outbytesleft)
{
    abort();
}

#endif /* HAVE_ICONV */

/* Allocates memory for and returns a string containing the font's
 * registry and encoding separated by a dash, or NULL if it can't be
 * determined */
static char *alloc_font_code_set(Display *display, XFontStruct *font)
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
    ICONV_CONST char *string = "a";
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

#ifdef HAVE_ICONV
    /* Was an encoding provided? */
    if (tocode != NULL)
    {
        /* Yes.  Use it to create a conversion descriptor */
        if ((cd = do_iconv_open(tocode, UTF8_CODE)) == (iconv_t)-1)
        {
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


    /* Look up the font's code set */
    if ((string = alloc_font_code_set(display, font)) == NULL)
    {
        return self;
    }

    /* Open a conversion descriptor */
    if ((cd = do_iconv_open(string, UTF8_CODE)) == (iconv_t)-1)
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
#endif /* HAVE_ICONV */

    return self;
}

/* Wrapper around iconv() to catch most of the nasty gotchas */
static size_t utf8_renderer_iconv(
    utf8_renderer_t self,
    ICONV_CONST char **inbuf, size_t *inbytesleft,
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
    ICONV_CONST char *string,
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
            char *point = buffer;

            /* Set the initial measurements */
            if (is_first)
            {
                is_first = False;
                info = per_char(self -> font, 0, *(unsigned char *)point);
                lbearing = info -> lbearing;
                rbearing = info -> rbearing;
                width = info -> width;
                point++;
            }

            /* Adjust for the rest of the string */
            while (point < out_point)
            {
                info = per_char(self -> font, 0, *(unsigned char *)point);
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
    ICONV_CONST char *string)
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
            char *first;
            char *last;

            /* Look for visible characters in the buffer */
            first = buffer;
            while (first < out_point)
            {
                info = per_char(renderer -> font, 0, *(unsigned char *)first);

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
                    while (last < out_point)
                    {
                        info = per_char(renderer -> font, 0, *(unsigned char*)last);

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
    /* The conversion descriptor for encoding (xxx to UTF-8) */
    iconv_t encoder_cd;

    /* The conversion descriptor for decoding (UTF-8 to xxx) */
    iconv_t decoder_cd;
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
    self -> encoder_cd = (iconv_t)-1;
    self -> decoder_cd = (iconv_t)-1;

    /* If a code set was provided then use it */
    if (code_set != NULL)
    {
        self -> encoder_cd = do_iconv_open(UTF8_CODE, code_set);
        self -> decoder_cd = do_iconv_open(code_set, UTF8_CODE);
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

        /* We don't use the charset, so free it now */
        XtFree(charset);

        /* Look up the font's code set */
        if ((string = alloc_font_code_set(display, font)) == NULL)
        {
            continue;
        }

        /* Open a conversion descriptor */
        self -> encoder_cd = do_iconv_open(UTF8_CODE, string);

        /* Open the reverse conversion descriptor */
        self -> decoder_cd = do_iconv_open(string, UTF8_CODE);
        free(string);

        XmFontListFreeFontContext(context);
        return self;
    }
}

/* Encodes a string */
char *utf8_encoder_encode(utf8_encoder_t self, ICONV_CONST char *input)
{
    char *buffer;
    char *output;
    size_t in_length;
    size_t out_length;
    size_t length;

    /* Measure the input string */
    in_length = strlen(input) + 1;

    /* Special case for empty strings */
    if (in_length == 1)
    {
        return strdup("");
    }

    /* If we have no encoder then do a manual ASCII to UTF-8 and
     * replace any characters with the high bit set with a `?' */
    if (self -> encoder_cd == (iconv_t)-1)
    {
        int i;

        /* The output will be the same size as the input */
        if ((output = (char *)malloc(in_length)) == NULL)
        {
            perror("malloc() failed");
            return NULL;
        }

        /* Copy, replacing any UTF-8 unfriendly characters */
        for (i = 0; i < in_length; i++)
        {
            int ch = input[i];

            if (ch & 0x80)
            {
                output[i] = '?';
            }
            else
            {
                output[i] = ch;
            }
        }

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
        if (iconv(self -> encoder_cd, &input, &in_length, &output, &out_length) == (size_t)-1)
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
                    return NULL;
                }

                out_length += new_length - length;
                output = new_buffer + (output - buffer);

                buffer = new_buffer;
                length = new_length;
            }
            else
            {
                perror("iconv() failed");
                return NULL;
            }
        }
    }

    return buffer;
}

/* Decodes a string */
char *utf8_encoder_decode(utf8_encoder_t self, ICONV_CONST char *input)
{
    char *buffer;
    char *output;
    size_t in_length;
    size_t out_length;
    size_t length;

    /* Measure the input string */
    in_length = strlen(input) + 1;

    /* Special case for empty strings */
    if (in_length == 1)
    {
        return strdup("");
    }

    /* If we have no decoder then do a manual UTF-8 to ASCII and
     * replace any characters with the high bit set with `?' */
    if (self -> decoder_cd == (iconv_t)-1)
    {
        int ch;
        int i;

        /* The output will be no longer than the input */
        if ((output = (char *)malloc(in_length)) == NULL)
        {
            perror("malloc() failed");
            return NULL;
        }

        /* Copy, replacing any non-ASCII characters */
        i = 0;
        while ((ch = *input++) != 0)
        {
            if ((ch & 0xc0) == 0xc0)
            {
                output[i++] = '?';
            }
            else if ((ch & 0xc0) != 0x80)
            {
                output[i++] = ch;
            }
        }

        /* Null-terminate the output string */
        output[i] = 0;
        return output;
    }

    /* Guess the length of the UTF-8 string */
    length = in_length;
    if ((buffer = (char *)malloc(length)) == NULL)
    {
        perror("malloc() failed");
        return NULL;
    }

    output = buffer;
    out_length = length;

    /* Encode away */
    while (in_length != 0)
    {
        if (iconv(self -> decoder_cd, &input, &in_length, &output, &out_length) == (size_t)-1)
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
                    return NULL;
                }

                out_length += new_length - length;
                output = new_buffer + (output - buffer);

                buffer = new_buffer;
                length = new_length;
            }
            else
            {
                perror("iconv() failed");
                return NULL;
            }
        }
    }

    return buffer;
}

