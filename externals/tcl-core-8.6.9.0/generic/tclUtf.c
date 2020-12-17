/*
 * tclUtf.c --
 *
 *	Routines for manipulating UTF-8 strings.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include "tclInt.h"

/*
 * Include the static character classification tables and macros.
 */

#include "tclUniData.c"

/*
 * The following macros are used for fast character category tests. The x_BITS
 * values are shifted right by the category value to determine whether the
 * given category is included in the set.
 */

#define ALPHA_BITS ((1 << UPPERCASE_LETTER) | (1 << LOWERCASE_LETTER) \
	| (1 << TITLECASE_LETTER) | (1 << MODIFIER_LETTER) | (1<<OTHER_LETTER))

#define CONTROL_BITS ((1 << CONTROL) | (1 << FORMAT) | (1 << PRIVATE_USE))

#define DIGIT_BITS (1 << DECIMAL_DIGIT_NUMBER)

#define SPACE_BITS ((1 << SPACE_SEPARATOR) | (1 << LINE_SEPARATOR) \
	| (1 << PARAGRAPH_SEPARATOR))

#define WORD_BITS (ALPHA_BITS | DIGIT_BITS | (1 << CONNECTOR_PUNCTUATION))

#define PUNCT_BITS ((1 << CONNECTOR_PUNCTUATION) | \
	(1 << DASH_PUNCTUATION) | (1 << OPEN_PUNCTUATION) | \
	(1 << CLOSE_PUNCTUATION) | (1 << INITIAL_QUOTE_PUNCTUATION) | \
	(1 << FINAL_QUOTE_PUNCTUATION) | (1 << OTHER_PUNCTUATION))

#define GRAPH_BITS (WORD_BITS | PUNCT_BITS | \
	(1 << NON_SPACING_MARK) | (1 << ENCLOSING_MARK) | \
	(1 << COMBINING_SPACING_MARK) | (1 << LETTER_NUMBER) | \
	(1 << OTHER_NUMBER) | \
	(1 << MATH_SYMBOL) | (1 << CURRENCY_SYMBOL) | \
	(1 << MODIFIER_SYMBOL) | (1 << OTHER_SYMBOL))

/*
 * Unicode characters less than this value are represented by themselves in
 * UTF-8 strings.
 */

#define UNICODE_SELF	0x80

/*
 * The following structures are used when mapping between Unicode (UCS-2) and
 * UTF-8.
 */

static const unsigned char totalBytes[256] = {
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
#if TCL_UTF_MAX > 3
    4,4,4,4,4,4,4,4,
#else
    1,1,1,1,1,1,1,1,
#endif
    1,1,1,1,1,1,1,1
};

/*
 * Functions used only in this module.
 */

static int		UtfCount(int ch);

/*
 *---------------------------------------------------------------------------
 *
 * UtfCount --
 *
 *	Find the number of bytes in the Utf character "ch".
 *
 * Results:
 *	The return values is the number of bytes in the Utf character "ch".
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static inline int
UtfCount(
    int ch)			/* The Unicode character whose size is returned. */
{
    if ((unsigned)(ch - 1) < (UNICODE_SELF - 1)) {
	return 1;
    }
    if (ch <= 0x7FF) {
	return 2;
    }
#if TCL_UTF_MAX > 3
    if (((unsigned)(ch - 0x10000) <= 0xFFFFF)) {
	return 4;
    }
#endif
    return 3;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UniCharToUtf --
 *
 *	Store the given Tcl_UniChar as a sequence of UTF-8 bytes in the
 *	provided buffer. Equivalent to Plan 9 runetochar().
 *
 * Results:
 *	The return values is the number of bytes in the buffer that were
 *	consumed.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_UniCharToUtf(
    int ch,			/* The Tcl_UniChar to be stored in the
				 * buffer. */
    char *buf)			/* Buffer in which the UTF-8 representation of
				 * the Tcl_UniChar is stored. Buffer must be
				 * large enough to hold the UTF-8 character
				 * (at most TCL_UTF_MAX bytes). */
{
    if ((unsigned)(ch - 1) < (UNICODE_SELF - 1)) {
	buf[0] = (char) ch;
	return 1;
    }
    if (ch >= 0) {
	if (ch <= 0x7FF) {
	    buf[1] = (char) ((ch | 0x80) & 0xBF);
	    buf[0] = (char) ((ch >> 6) | 0xC0);
	    return 2;
	}
	if (ch <= 0xFFFF) {
#if TCL_UTF_MAX > 3
	    if ((ch & 0xF800) == 0xD800) {
		if (ch & 0x0400) {
		    /* Low surrogate */
		    if (((buf[0] & 0xF8) == 0xF0) && ((buf[1] & 0xC0) == 0x80)
			    && ((buf[2] & 0xCF) == 0)) {
			/* Previous Tcl_UniChar was a High surrogate, so combine */
			buf[3] = (char) ((ch & 0x3F) | 0x80);
			buf[2] |= (char) (((ch >> 6) & 0x0F) | 0x80);
			return 4;
		    }
		    /* Previous Tcl_UniChar was not a High surrogate, so just output */
		} else {
		    /* High surrogate */
		    ch += 0x40;
		    /* Fill buffer with specific 3-byte (invalid) byte combination,
		       so following Low surrogate can recognize it and combine */
		    buf[2] = (char) ((ch << 4) & 0x30);
		    buf[1] = (char) (((ch >> 2) & 0x3F) | 0x80);
		    buf[0] = (char) (((ch >> 8) & 0x07) | 0xF0);
		    return 0;
		}
	    }
#endif
	    goto three;
	}

#if TCL_UTF_MAX > 3
	if (ch <= 0x10FFFF) {
	    buf[3] = (char) ((ch | 0x80) & 0xBF);
	    buf[2] = (char) (((ch >> 6) | 0x80) & 0xBF);
	    buf[1] = (char) (((ch >> 12) | 0x80) & 0xBF);
	    buf[0] = (char) ((ch >> 18) | 0xF0);
	    return 4;
	}
    } else if (ch == -1) {
	if (((buf[0] & 0xF8) == 0xF0) && ((buf[1] & 0xC0) == 0x80)
		&& ((buf[2] & 0xCF) == 0)) {
	    ch = 0xD7C0 + ((buf[0] & 0x07) << 8) + ((buf[1] & 0x3F) << 2)
		    + ((buf[2] & 0x30) >> 4);
	    goto three;
	}
#endif
    }

    ch = 0xFFFD;
three:
    buf[2] = (char) ((ch | 0x80) & 0xBF);
    buf[1] = (char) (((ch >> 6) | 0x80) & 0xBF);
    buf[0] = (char) ((ch >> 12) | 0xE0);
    return 3;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UniCharToUtfDString --
 *
 *	Convert the given Unicode string to UTF-8.
 *
 * Results:
 *	The return value is a pointer to the UTF-8 representation of the
 *	Unicode string. Storage for the return value is appended to the end of
 *	dsPtr.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

char *
Tcl_UniCharToUtfDString(
    const Tcl_UniChar *uniStr,	/* Unicode string to convert to UTF-8. */
    int uniLength,		/* Length of Unicode string in Tcl_UniChars
				 * (must be >= 0). */
    Tcl_DString *dsPtr)		/* UTF-8 representation of string is appended
				 * to this previously initialized DString. */
{
    const Tcl_UniChar *w, *wEnd;
    char *p, *string;
    int oldLength;

    /*
     * UTF-8 string length in bytes will be <= Unicode string length *
     * TCL_UTF_MAX.
     */

    oldLength = Tcl_DStringLength(dsPtr);
    Tcl_DStringSetLength(dsPtr, oldLength + (uniLength + 1) * TCL_UTF_MAX);
    string = Tcl_DStringValue(dsPtr) + oldLength;

    p = string;
    wEnd = uniStr + uniLength;
    for (w = uniStr; w < wEnd; ) {
	p += Tcl_UniCharToUtf(*w, p);
	w++;
    }
    Tcl_DStringSetLength(dsPtr, oldLength + (p - string));

    return string;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfToUniChar --
 *
 *	Extract the Tcl_UniChar represented by the UTF-8 string. Bad UTF-8
 *	sequences are converted to valid Tcl_UniChars and processing
 *	continues. Equivalent to Plan 9 chartorune().
 *
 *	The caller must ensure that the source buffer is long enough that this
 *	routine does not run off the end and dereference non-existent memory
 *	looking for trail bytes. If the source buffer is known to be '\0'
 *	terminated, this cannot happen. Otherwise, the caller should call
 *	Tcl_UtfCharComplete() before calling this routine to ensure that
 *	enough bytes remain in the string.
 *
 *	If TCL_UTF_MAX == 4, special handling of Surrogate pairs is done:
 *	For any UTF-8 string containing a character outside of the BMP, the
 *	first call to this function will fill *chPtr with the high surrogate
 *	and generate a return value of 0. Calling Tcl_UtfToUniChar again
 *	will produce the low surrogate and a return value of 4. Because *chPtr
 *	is used to remember whether the high surrogate is already produced, it
 *	is recommended to initialize the variable it points to as 0 before
 *	the first call to Tcl_UtfToUniChar is done.
 *
 * Results:
 *	*chPtr is filled with the Tcl_UniChar, and the return value is the
 *	number of bytes from the UTF-8 string that were consumed.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_UtfToUniChar(
    register const char *src,	/* The UTF-8 string. */
    register Tcl_UniChar *chPtr)/* Filled with the Tcl_UniChar represented by
				 * the UTF-8 string. */
{
    register int byte;

    /*
     * Unroll 1 to 3 (or 4) byte UTF-8 sequences.
     */

    byte = *((unsigned char *) src);
    if (byte < 0xC0) {
	/*
	 * Handles properly formed UTF-8 characters between 0x01 and 0x7F.
	 * Also treats \0 and naked trail bytes 0x80 to 0xBF as valid
	 * characters representing themselves.
	 */

	*chPtr = (Tcl_UniChar) byte;
	return 1;
    } else if (byte < 0xE0) {
	if ((src[1] & 0xC0) == 0x80) {
	    /*
	     * Two-byte-character lead-byte followed by a trail-byte.
	     */

	    *chPtr = (Tcl_UniChar) (((byte & 0x1F) << 6) | (src[1] & 0x3F));
	    if ((unsigned)(*chPtr - 1) >= (UNICODE_SELF - 1)) {
		return 2;
	    }
	}

	/*
	 * A two-byte-character lead-byte not followed by trail-byte
	 * represents itself.
	 */
    } else if (byte < 0xF0) {
	if (((src[1] & 0xC0) == 0x80) && ((src[2] & 0xC0) == 0x80)) {
	    /*
	     * Three-byte-character lead byte followed by two trail bytes.
	     */

	    *chPtr = (Tcl_UniChar) (((byte & 0x0F) << 12)
		    | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F));
	    if (*chPtr > 0x7FF) {
		return 3;
	    }
	}

	/*
	 * A three-byte-character lead-byte not followed by two trail-bytes
	 * represents itself.
	 */
    }
#if TCL_UTF_MAX > 3
    else if (byte < 0xF8) {
	if (((src[1] & 0xC0) == 0x80) && ((src[2] & 0xC0) == 0x80) && ((src[3] & 0xC0) == 0x80)) {
	    /*
	     * Four-byte-character lead byte followed by three trail bytes.
	     */
#if TCL_UTF_MAX == 4
	    Tcl_UniChar surrogate;

	    byte = (((byte & 0x07) << 18) | ((src[1] & 0x3F) << 12)
		    | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F)) - 0x10000;
	    surrogate = (Tcl_UniChar) (0xD800 + (byte >> 10));
	    if (byte & 0x100000) {
		/* out of range, < 0x10000 or > 0x10ffff */
	    } else if (*chPtr != surrogate) {
		/* produce high surrogate, but don't advance source pointer */
		*chPtr = surrogate;
		return 0;
	    } else {
		/* produce low surrogate, and advance source pointer */
		*chPtr = (Tcl_UniChar) (0xDC00 | (byte & 0x3FF));
		return 4;
	    }
#else
	    *chPtr = (Tcl_UniChar) (((byte & 0x07) << 18) | ((src[1] & 0x3F) << 12)
		    | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F));
	    if ((unsigned)(*chPtr - 0x10000) <= 0xFFFFF) {
		return 4;
	    }
#endif
	}

	/*
	 * A four-byte-character lead-byte not followed by two trail-bytes
	 * represents itself.
	 */
    }
#endif

    *chPtr = (Tcl_UniChar) byte;
    return 1;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfToUniCharDString --
 *
 *	Convert the UTF-8 string to Unicode.
 *
 * Results:
 *	The return value is a pointer to the Unicode representation of the
 *	UTF-8 string. Storage for the return value is appended to the end of
 *	dsPtr. The Unicode string is terminated with a Unicode NULL character.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_UniChar *
Tcl_UtfToUniCharDString(
    const char *src,		/* UTF-8 string to convert to Unicode. */
    int length,			/* Length of UTF-8 string in bytes, or -1 for
				 * strlen(). */
    Tcl_DString *dsPtr)		/* Unicode representation of string is
				 * appended to this previously initialized
				 * DString. */
{
    Tcl_UniChar ch = 0, *w, *wString;
    const char *p, *end;
    int oldLength;

    if (length < 0) {
	length = strlen(src);
    }

    /*
     * Unicode string length in Tcl_UniChars will be <= UTF-8 string length in
     * bytes.
     */

    oldLength = Tcl_DStringLength(dsPtr);

    Tcl_DStringSetLength(dsPtr,
	    oldLength + (int) ((length + 1) * sizeof(Tcl_UniChar)));
    wString = (Tcl_UniChar *) (Tcl_DStringValue(dsPtr) + oldLength);

    w = wString;
    p = src;
    end = src + length - TCL_UTF_MAX;
    while (p < end) {
	p += TclUtfToUniChar(p, &ch);
	*w++ = ch;
    }
    end += TCL_UTF_MAX;
    while (p < end) {
	if (Tcl_UtfCharComplete(p, end-p)) {
	    p += TclUtfToUniChar(p, &ch);
	} else {
	    ch = UCHAR(*p++);
	}
	*w++ = ch;
    }
    *w = '\0';
    Tcl_DStringSetLength(dsPtr,
	    (oldLength + ((char *) w - (char *) wString)));

    return wString;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfCharComplete --
 *
 *	Determine if the UTF-8 string of the given length is long enough to be
 *	decoded by Tcl_UtfToUniChar(). This does not ensure that the UTF-8
 *	string is properly formed. Equivalent to Plan 9 fullrune().
 *
 * Results:
 *	The return value is 0 if the string is not long enough, non-zero
 *	otherwise.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_UtfCharComplete(
    const char *src,		/* String to check if first few bytes contain
				 * a complete UTF-8 character. */
    int length)			/* Length of above string in bytes. */
{
    return length >= totalBytes[(unsigned char)*src];
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_NumUtfChars --
 *
 *	Returns the number of characters (not bytes) in the UTF-8 string, not
 *	including the terminating NULL byte. This is equivalent to Plan 9
 *	utflen() and utfnlen().
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_NumUtfChars(
    register const char *src,	/* The UTF-8 string to measure. */
    int length)			/* The length of the string in bytes, or -1
				 * for strlen(string). */
{
    Tcl_UniChar ch = 0;
    register int i = 0;

    /*
     * The separate implementations are faster.
     *
     * Since this is a time-sensitive function, we also do the check for the
     * single-byte char case specially.
     */

    if (length < 0) {
	while (*src != '\0') {
	    src += TclUtfToUniChar(src, &ch);
	    i++;
	}
	if (i < 0) i = INT_MAX; /* Bug [2738427] */
    } else {
	register const char *endPtr = src + length - TCL_UTF_MAX;

	while (src < endPtr) {
	    src += TclUtfToUniChar(src, &ch);
	    i++;
	}
	endPtr += TCL_UTF_MAX;
	while ((src < endPtr) && Tcl_UtfCharComplete(src, endPtr - src)) {
	    src += TclUtfToUniChar(src, &ch);
	    i++;
	}
	if (src < endPtr) {
	    i += endPtr - src;
	}
    }
    return i;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfFindFirst --
 *
 *	Returns a pointer to the first occurance of the given Unicode character
 *	in the NULL-terminated UTF-8 string. The NULL terminator is considered
 *	part of the UTF-8 string. Equivalent to Plan 9 utfrune().
 *
 * Results:
 *	As above. If the Unicode character does not exist in the given string,
 *	the return value is NULL.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_UtfFindFirst(
    const char *src,		/* The UTF-8 string to be searched. */
    int ch)			/* The Unicode character to search for. */
{
    int len, fullchar;
    Tcl_UniChar find = 0;

    while (1) {
	len = TclUtfToUniChar(src, &find);
	fullchar = find;
#if TCL_UTF_MAX == 4
	if (!len) {
	    len += TclUtfToUniChar(src, &find);
	    fullchar = (((fullchar & 0x3ff) << 10) | (find & 0x3ff)) + 0x10000;
	}
#endif
	if (fullchar == ch) {
	    return src;
	}
	if (*src == '\0') {
	    return NULL;
	}
	src += len;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfFindLast --
 *
 *	Returns a pointer to the last occurance of the given Unicode character
 *	in the NULL-terminated UTF-8 string. The NULL terminator is considered
 *	part of the UTF-8 string. Equivalent to Plan 9 utfrrune().
 *
 * Results:
 *	As above. If the Unicode character does not exist in the given string, the
 *	return value is NULL.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_UtfFindLast(
    const char *src,		/* The UTF-8 string to be searched. */
    int ch)			/* The Unicode character to search for. */
{
    int len, fullchar;
    Tcl_UniChar find = 0;
    const char *last;

    last = NULL;
    while (1) {
	len = TclUtfToUniChar(src, &find);
	fullchar = find;
#if TCL_UTF_MAX == 4
	if (!len) {
	    len += TclUtfToUniChar(src, &find);
	    fullchar = (((fullchar & 0x3ff) << 10) | (find & 0x3ff)) + 0x10000;
	}
#endif
	if (fullchar == ch) {
	    last = src;
	}
	if (*src == '\0') {
	    break;
	}
	src += len;
    }
    return last;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfNext --
 *
 *	Given a pointer to some current location in a UTF-8 string, move
 *	forward one character. The caller must ensure that they are not asking
 *	for the next character after the last character in the string.
 *
 * Results:
 *	The return value is the pointer to the next character in the UTF-8
 *	string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_UtfNext(
    const char *src)		/* The current location in the string. */
{
    Tcl_UniChar ch = 0;
    int len = TclUtfToUniChar(src, &ch);

#if TCL_UTF_MAX == 4
    if (len == 0) {
      len = TclUtfToUniChar(src, &ch);
    }
#endif
    return src + len;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfPrev --
 *
 *	Given a pointer to some current location in a UTF-8 string, move
 *	backwards one character. This works correctly when the pointer is in
 *	the middle of a UTF-8 character.
 *
 * Results:
 *	The return value is a pointer to the previous character in the UTF-8
 *	string. If the current location was already at the beginning of the
 *	string, the return value will also be a pointer to the beginning of
 *	the string.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_UtfPrev(
    const char *src,		/* The current location in the string. */
    const char *start)		/* Pointer to the beginning of the string, to
				 * avoid going backwards too far. */
{
    const char *look;
    int i, byte;

    look = --src;
    for (i = 0; i < TCL_UTF_MAX; i++) {
	if (look < start) {
	    if (src < start) {
		src = start;
	    }
	    break;
	}
	byte = *((unsigned char *) look);
	if (byte < 0x80) {
	    break;
	}
	if (byte >= 0xC0) {
	    return look;
	}
	look--;
    }
    return src;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UniCharAtIndex --
 *
 *	Returns the Tcl_UniChar represented at the specified character
 *	(not byte) position in the UTF-8 string.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_UniChar
Tcl_UniCharAtIndex(
    register const char *src,	/* The UTF-8 string to dereference. */
    register int index)		/* The position of the desired character. */
{
    Tcl_UniChar ch = 0;

    while (index-- >= 0) {
	src += TclUtfToUniChar(src, &ch);
    }
    return ch;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfAtIndex --
 *
 *	Returns a pointer to the specified character (not byte) position in
 *	the UTF-8 string.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

const char *
Tcl_UtfAtIndex(
    register const char *src,	/* The UTF-8 string. */
    register int index)		/* The position of the desired character. */
{
    Tcl_UniChar ch = 0;
    int len = 1;

    while (index-- > 0) {
	len = TclUtfToUniChar(src, &ch);
	src += len;
    }
#if TCL_UTF_MAX == 4
     if (!len) {
	/* Index points at character following High Surrogate */
	src += TclUtfToUniChar(src, &ch);
    }
#endif
    return src;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_UtfBackslash --
 *
 *	Figure out how to handle a backslash sequence.
 *
 * Results:
 *	Stores the bytes represented by the backslash sequence in dst and
 *	returns the number of bytes written to dst. At most TCL_UTF_MAX bytes
 *	are written to dst; dst must have been large enough to accept those
 *	bytes. If readPtr isn't NULL then it is filled in with a count of the
 *	number of bytes in the backslash sequence.
 *
 * Side effects:
 *	The maximum number of bytes it takes to represent a Unicode character
 *	in UTF-8 is guaranteed to be less than the number of bytes used to
 *	express the backslash sequence that represents that Unicode character.
 *	If the target buffer into which the caller is going to store the bytes
 *	that represent the Unicode character is at least as large as the
 *	source buffer from which the backslashed sequence was extracted, no
 *	buffer overruns should occur.
 *
 *---------------------------------------------------------------------------
 */

int
Tcl_UtfBackslash(
    const char *src,		/* Points to the backslash character of a
				 * backslash sequence. */
    int *readPtr,		/* Fill in with number of characters read from
				 * src, unless NULL. */
    char *dst)			/* Filled with the bytes represented by the
				 * backslash sequence. */
{
#define LINE_LENGTH 128
    int numRead;
    int result;

    result = TclParseBackslash(src, LINE_LENGTH, &numRead, dst);
    if (numRead == LINE_LENGTH) {
	/*
	 * We ate a whole line. Pay the price of a strlen()
	 */

	result = TclParseBackslash(src, (int)strlen(src), &numRead, dst);
    }
    if (readPtr != NULL) {
	*readPtr = numRead;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UtfToUpper --
 *
 *	Convert lowercase characters to uppercase characters in a UTF string
 *	in place. The conversion may shrink the UTF string.
 *
 * Results:
 *	Returns the number of bytes in the resulting string excluding the
 *	trailing null.
 *
 * Side effects:
 *	Writes a terminating null after the last converted character.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UtfToUpper(
    char *str)			/* String to convert in place. */
{
    Tcl_UniChar ch = 0, upChar;
    char *src, *dst;
    int bytes;

    /*
     * Iterate over the string until we hit the terminating null.
     */

    src = dst = str;
    while (*src) {
	bytes = TclUtfToUniChar(src, &ch);
	upChar = Tcl_UniCharToUpper(ch);

	/*
	 * To keep badly formed Utf strings from getting inflated by the
	 * conversion (thereby causing a segfault), only copy the upper case
	 * char to dst if its size is <= the original char.
	 */

	if (bytes < UtfCount(upChar)) {
	    memcpy(dst, src, (size_t) bytes);
	    dst += bytes;
	} else {
	    dst += Tcl_UniCharToUtf(upChar, dst);
	}
	src += bytes;
    }
    *dst = '\0';
    return (dst - str);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UtfToLower --
 *
 *	Convert uppercase characters to lowercase characters in a UTF string
 *	in place. The conversion may shrink the UTF string.
 *
 * Results:
 *	Returns the number of bytes in the resulting string excluding the
 *	trailing null.
 *
 * Side effects:
 *	Writes a terminating null after the last converted character.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UtfToLower(
    char *str)			/* String to convert in place. */
{
    Tcl_UniChar ch = 0, lowChar;
    char *src, *dst;
    int bytes;

    /*
     * Iterate over the string until we hit the terminating null.
     */

    src = dst = str;
    while (*src) {
	bytes = TclUtfToUniChar(src, &ch);
	lowChar = Tcl_UniCharToLower(ch);

	/*
	 * To keep badly formed Utf strings from getting inflated by the
	 * conversion (thereby causing a segfault), only copy the lower case
	 * char to dst if its size is <= the original char.
	 */

	if (bytes < UtfCount(lowChar)) {
	    memcpy(dst, src, (size_t) bytes);
	    dst += bytes;
	} else {
	    dst += Tcl_UniCharToUtf(lowChar, dst);
	}
	src += bytes;
    }
    *dst = '\0';
    return (dst - str);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UtfToTitle --
 *
 *	Changes the first character of a UTF string to title case or uppercase
 *	and the rest of the string to lowercase. The conversion happens in
 *	place and may shrink the UTF string.
 *
 * Results:
 *	Returns the number of bytes in the resulting string excluding the
 *	trailing null.
 *
 * Side effects:
 *	Writes a terminating null after the last converted character.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UtfToTitle(
    char *str)			/* String to convert in place. */
{
    Tcl_UniChar ch = 0, titleChar, lowChar;
    char *src, *dst;
    int bytes;

    /*
     * Capitalize the first character and then lowercase the rest of the
     * characters until we get to a null.
     */

    src = dst = str;

    if (*src) {
	bytes = TclUtfToUniChar(src, &ch);
	titleChar = Tcl_UniCharToTitle(ch);

	if (bytes < UtfCount(titleChar)) {
	    memcpy(dst, src, (size_t) bytes);
	    dst += bytes;
	} else {
	    dst += Tcl_UniCharToUtf(titleChar, dst);
	}
	src += bytes;
    }
    while (*src) {
	bytes = TclUtfToUniChar(src, &ch);
	lowChar = ch;
	/* Special exception for Georgian Asomtavruli chars, no titlecase. */
	if ((unsigned)(lowChar - 0x1C90) >= 0x30) {
	    lowChar = Tcl_UniCharToLower(lowChar);
	}

	if (bytes < UtfCount(lowChar)) {
	    memcpy(dst, src, (size_t) bytes);
	    dst += bytes;
	} else {
	    dst += Tcl_UniCharToUtf(lowChar, dst);
	}
	src += bytes;
    }
    *dst = '\0';
    return (dst - str);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpUtfNcmp2 --
 *
 *	Compare at most numBytes bytes of utf-8 strings cs and ct. Both cs and
 *	ct are assumed to be at least numBytes bytes long.
 *
 * Results:
 *	Return <0 if cs < ct, 0 if cs == ct, or >0 if cs > ct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpUtfNcmp2(
    const char *cs,		/* UTF string to compare to ct. */
    const char *ct,		/* UTF string cs is compared to. */
    unsigned long numBytes)	/* Number of *bytes* to compare. */
{
    /*
     * We can't simply call 'memcmp(cs, ct, numBytes);' because we need to
     * check for Tcl's \xC0\x80 non-utf-8 null encoding. Otherwise utf-8 lexes
     * fine in the strcmp manner.
     */

    register int result = 0;

    for ( ; numBytes != 0; numBytes--, cs++, ct++) {
	if (*cs != *ct) {
	    result = UCHAR(*cs) - UCHAR(*ct);
	    break;
	}
    }
    if (numBytes && ((UCHAR(*cs) == 0xC0) || (UCHAR(*ct) == 0xC0))) {
	unsigned char c1, c2;

	c1 = ((UCHAR(*cs) == 0xC0) && (UCHAR(cs[1]) == 0x80)) ? 0 : UCHAR(*cs);
	c2 = ((UCHAR(*ct) == 0xC0) && (UCHAR(ct[1]) == 0x80)) ? 0 : UCHAR(*ct);
	result = (c1 - c2);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UtfNcmp --
 *
 *	Compare at most numChars UTF chars of string cs to string ct. Both cs
 *	and ct are assumed to be at least numChars UTF chars long.
 *
 * Results:
 *	Return <0 if cs < ct, 0 if cs == ct, or >0 if cs > ct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UtfNcmp(
    const char *cs,		/* UTF string to compare to ct. */
    const char *ct,		/* UTF string cs is compared to. */
    unsigned long numChars)	/* Number of UTF chars to compare. */
{
    Tcl_UniChar ch1 = 0, ch2 = 0;

    /*
     * Cannot use 'memcmp(cs, ct, n);' as byte representation of \u0000 (the
     * pair of bytes 0xC0,0x80) is larger than byte representation of \u0001
     * (the byte 0x01.)
     */

    while (numChars-- > 0) {
	/*
	 * n must be interpreted as chars, not bytes. This should be called
	 * only when both strings are of at least n chars long (no need for \0
	 * check)
	 */

	cs += TclUtfToUniChar(cs, &ch1);
	ct += TclUtfToUniChar(ct, &ch2);
	if (ch1 != ch2) {
#if TCL_UTF_MAX == 4
	    /* Surrogates always report higher than non-surrogates */
	    if (((ch1 & 0xFC00) == 0xD800)) {
	    if ((ch2 & 0xFC00) != 0xD800) {
		return ch1;
	    }
	    } else if ((ch2 & 0xFC00) == 0xD800) {
		return -ch2;
	    }
#endif
	    return (ch1 - ch2);
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UtfNcasecmp --
 *
 *	Compare at most numChars UTF chars of string cs to string ct case
 *	insensitive. Both cs and ct are assumed to be at least numChars UTF
 *	chars long.
 *
 * Results:
 *	Return <0 if cs < ct, 0 if cs == ct, or >0 if cs > ct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UtfNcasecmp(
    const char *cs,		/* UTF string to compare to ct. */
    const char *ct,		/* UTF string cs is compared to. */
    unsigned long numChars)	/* Number of UTF chars to compare. */
{
    Tcl_UniChar ch1 = 0, ch2 = 0;

    while (numChars-- > 0) {
	/*
	 * n must be interpreted as chars, not bytes.
	 * This should be called only when both strings are of
	 * at least n chars long (no need for \0 check)
	 */
	cs += TclUtfToUniChar(cs, &ch1);
	ct += TclUtfToUniChar(ct, &ch2);
	if (ch1 != ch2) {
#if TCL_UTF_MAX == 4
	    /* Surrogates always report higher than non-surrogates */
	    if (((ch1 & 0xFC00) == 0xD800)) {
	    if ((ch2 & 0xFC00) != 0xD800) {
		return ch1;
	    }
	    } else if ((ch2 & 0xFC00) == 0xD800) {
		return -ch2;
	    }
#endif
	    ch1 = Tcl_UniCharToLower(ch1);
	    ch2 = Tcl_UniCharToLower(ch2);
	    if (ch1 != ch2) {
		return (ch1 - ch2);
	    }
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclUtfCasecmp --
 *
 *	Compare UTF chars of string cs to string ct case insensitively.
 *	Replacement for strcasecmp in Tcl core, in places where UTF-8 should
 *	be handled.
 *
 * Results:
 *	Return <0 if cs < ct, 0 if cs == ct, or >0 if cs > ct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclUtfCasecmp(
    const char *cs,		/* UTF string to compare to ct. */
    const char *ct)		/* UTF string cs is compared to. */
{
    Tcl_UniChar ch1 = 0, ch2 = 0;

    while (*cs && *ct) {
	cs += TclUtfToUniChar(cs, &ch1);
	ct += TclUtfToUniChar(ct, &ch2);
	if (ch1 != ch2) {
#if TCL_UTF_MAX == 4
	    /* Surrogates always report higher than non-surrogates */
	    if (((ch1 & 0xFC00) == 0xD800)) {
	    if ((ch2 & 0xFC00) != 0xD800) {
		return ch1;
	    }
	    } else if ((ch2 & 0xFC00) == 0xD800) {
		return -ch2;
	    }
#endif
	    ch1 = Tcl_UniCharToLower(ch1);
	    ch2 = Tcl_UniCharToLower(ch2);
	    if (ch1 != ch2) {
		return ch1 - ch2;
	    }
	}
    }
    return UCHAR(*cs) - UCHAR(*ct);
}


/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharToUpper --
 *
 *	Compute the uppercase equivalent of the given Unicode character.
 *
 * Results:
 *	Returns the uppercase Unicode character.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_UniChar
Tcl_UniCharToUpper(
    int ch)			/* Unicode character to convert. */
{
    int info = GetUniCharInfo(ch);

    if (GetCaseType(info) & 0x04) {
	ch -= GetDelta(info);
    }
    return (Tcl_UniChar) ch;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharToLower --
 *
 *	Compute the lowercase equivalent of the given Unicode character.
 *
 * Results:
 *	Returns the lowercase Unicode character.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_UniChar
Tcl_UniCharToLower(
    int ch)			/* Unicode character to convert. */
{
    int info = GetUniCharInfo(ch);
    int mode = GetCaseType(info);

    if ((mode & 0x02) && (mode != 0x7)) {
	ch += GetDelta(info);
    }
    return (Tcl_UniChar) ch;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharToTitle --
 *
 *	Compute the titlecase equivalent of the given Unicode character.
 *
 * Results:
 *	Returns the titlecase Unicode character.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_UniChar
Tcl_UniCharToTitle(
    int ch)			/* Unicode character to convert. */
{
    int info = GetUniCharInfo(ch);
    int mode = GetCaseType(info);

    if (mode & 0x1) {
	/*
	 * Subtract or add one depending on the original case.
	 */

	if (mode != 0x7) {
	    ch += ((mode & 0x4) ? -1 : 1);
	}
    } else if (mode == 0x4) {
	ch -= GetDelta(info);
    }
    return (Tcl_UniChar) ch;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharLen --
 *
 *	Find the length of a UniChar string. The str input must be null
 *	terminated.
 *
 * Results:
 *	Returns the length of str in UniChars (not bytes).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharLen(
    const Tcl_UniChar *uniStr)	/* Unicode string to find length of. */
{
    int len = 0;

    while (*uniStr != '\0') {
	len++;
	uniStr++;
    }
    return len;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharNcmp --
 *
 *	Compare at most numChars unichars of string ucs to string uct.
 *	Both ucs and uct are assumed to be at least numChars unichars long.
 *
 * Results:
 *	Return <0 if ucs < uct, 0 if ucs == uct, or >0 if ucs > uct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharNcmp(
    const Tcl_UniChar *ucs,	/* Unicode string to compare to uct. */
    const Tcl_UniChar *uct,	/* Unicode string ucs is compared to. */
    unsigned long numChars)	/* Number of unichars to compare. */
{
#ifdef WORDS_BIGENDIAN
    /*
     * We are definitely on a big-endian machine; memcmp() is safe
     */

    return memcmp(ucs, uct, numChars*sizeof(Tcl_UniChar));

#else /* !WORDS_BIGENDIAN */
    /*
     * We can't simply call memcmp() because that is not lexically correct.
     */

    for ( ; numChars != 0; ucs++, uct++, numChars--) {
	if (*ucs != *uct) {
	    return (*ucs - *uct);
	}
    }
    return 0;
#endif /* WORDS_BIGENDIAN */
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharNcasecmp --
 *
 *	Compare at most numChars unichars of string ucs to string uct case
 *	insensitive. Both ucs and uct are assumed to be at least numChars
 *	unichars long.
 *
 * Results:
 *	Return <0 if ucs < uct, 0 if ucs == uct, or >0 if ucs > uct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharNcasecmp(
    const Tcl_UniChar *ucs,	/* Unicode string to compare to uct. */
    const Tcl_UniChar *uct,	/* Unicode string ucs is compared to. */
    unsigned long numChars)	/* Number of unichars to compare. */
{
    for ( ; numChars != 0; numChars--, ucs++, uct++) {
	if (*ucs != *uct) {
	    Tcl_UniChar lcs = Tcl_UniCharToLower(*ucs);
	    Tcl_UniChar lct = Tcl_UniCharToLower(*uct);

	    if (lcs != lct) {
		return (lcs - lct);
	    }
	}
    }
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsAlnum --
 *
 *	Test if a character is an alphanumeric Unicode character.
 *
 * Results:
 *	Returns 1 if character is alphanumeric.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsAlnum(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return (((ALPHA_BITS | DIGIT_BITS) >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsAlpha --
 *
 *	Test if a character is an alphabetic Unicode character.
 *
 * Results:
 *	Returns 1 if character is alphabetic.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsAlpha(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return ((ALPHA_BITS >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsControl --
 *
 *	Test if a character is a Unicode control character.
 *
 * Results:
 *	Returns non-zero if character is a control.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsControl(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	ch &= 0x1FFFFF;
	if ((ch == 0xE0001) || ((ch >= 0xE0020) && (ch <= 0xE007f))) {
	    return 1;
	}
	if ((ch >= 0xF0000) && ((ch & 0xFFFF) <= 0xFFFD)) {
	    return 1;
	}
	return 0;
    }
#endif
    return ((CONTROL_BITS >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsDigit --
 *
 *	Test if a character is a numeric Unicode character.
 *
 * Results:
 *	Returns non-zero if character is a digit.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsDigit(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return (GetCategory(ch) == DECIMAL_DIGIT_NUMBER);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsGraph --
 *
 *	Test if a character is any Unicode print character except space.
 *
 * Results:
 *	Returns non-zero if character is printable, but not space.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsGraph(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	ch &= 0x1FFFFF;
	return (ch >= 0xE0100) && (ch <= 0xE01EF);
    }
#endif
    return ((GRAPH_BITS >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsLower --
 *
 *	Test if a character is a lowercase Unicode character.
 *
 * Results:
 *	Returns non-zero if character is lowercase.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsLower(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return (GetCategory(ch) == LOWERCASE_LETTER);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsPrint --
 *
 *	Test if a character is a Unicode print character.
 *
 * Results:
 *	Returns non-zero if character is printable.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsPrint(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	ch &= 0x1FFFFF;
	return (ch >= 0xE0100) && (ch <= 0xE01EF);
    }
#endif
    return (((GRAPH_BITS|SPACE_BITS) >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsPunct --
 *
 *	Test if a character is a Unicode punctuation character.
 *
 * Results:
 *	Returns non-zero if character is punct.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsPunct(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return ((PUNCT_BITS >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsSpace --
 *
 *	Test if a character is a whitespace Unicode character.
 *
 * Results:
 *	Returns non-zero if character is a space.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsSpace(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    /* Ignore upper 11 bits. */
    ch &= 0x1FFFFF;
#else
    /* Ignore upper 16 bits. */
    ch &= 0xFFFF;
#endif

    /*
     * If the character is within the first 127 characters, just use the
     * standard C function, otherwise consult the Unicode table.
     */

    if (ch < 0x80) {
	return TclIsSpaceProc((char) ch);
#if TCL_UTF_MAX > 3
    } else if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
#endif
    } else if (ch == 0x0085 || ch == 0x180E || ch == 0x200B
	    || ch == 0x202F || ch == 0x2060 || ch == 0xFEFF) {
	return 1;
    } else {
	return ((SPACE_BITS >> GetCategory(ch)) & 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsUpper --
 *
 *	Test if a character is a uppercase Unicode character.
 *
 * Results:
 *	Returns non-zero if character is uppercase.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsUpper(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return (GetCategory(ch) == UPPERCASE_LETTER);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharIsWordChar --
 *
 *	Test if a character is alphanumeric or a connector punctuation mark.
 *
 * Results:
 *	Returns 1 if character is a word character.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharIsWordChar(
    int ch)			/* Unicode character to test. */
{
#if TCL_UTF_MAX > 3
    if (UNICODE_OUT_OF_RANGE(ch)) {
	return 0;
    }
#endif
    return ((WORD_BITS >> GetCategory(ch)) & 1);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_UniCharCaseMatch --
 *
 *	See if a particular Unicode string matches a particular pattern.
 *	Allows case insensitivity. This is the Unicode equivalent of the char*
 *	Tcl_StringCaseMatch. The UniChar strings must be NULL-terminated.
 *	This has no provision for counted UniChar strings, thus should not be
 *	used where NULLs are expected in the UniChar string. Use
 *	TclUniCharMatch where possible.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_UniCharCaseMatch(
    const Tcl_UniChar *uniStr,	/* Unicode String. */
    const Tcl_UniChar *uniPattern,
				/* Pattern, which may contain special
				 * characters. */
    int nocase)			/* 0 for case sensitive, 1 for insensitive */
{
    Tcl_UniChar ch1 = 0, p;

    while (1) {
	p = *uniPattern;

	/*
	 * See if we're at the end of both the pattern and the string. If so,
	 * we succeeded. If we're at the end of the pattern but not at the end
	 * of the string, we failed.
	 */

	if (p == 0) {
	    return (*uniStr == 0);
	}
	if ((*uniStr == 0) && (p != '*')) {
	    return 0;
	}

	/*
	 * Check for a "*" as the next pattern character. It matches any
	 * substring. We handle this by skipping all the characters up to the
	 * next matching one in the pattern, and then calling ourselves
	 * recursively for each postfix of string, until either we match or we
	 * reach the end of the string.
	 */

	if (p == '*') {
	    /*
	     * Skip all successive *'s in the pattern
	     */

	    while (*(++uniPattern) == '*') {
		/* empty body */
	    }
	    p = *uniPattern;
	    if (p == 0) {
		return 1;
	    }
	    if (nocase) {
		p = Tcl_UniCharToLower(p);
	    }
	    while (1) {
		/*
		 * Optimization for matching - cruise through the string
		 * quickly if the next char in the pattern isn't a special
		 * character
		 */

		if ((p != '[') && (p != '?') && (p != '\\')) {
		    if (nocase) {
			while (*uniStr && (p != *uniStr)
				&& (p != Tcl_UniCharToLower(*uniStr))) {
			    uniStr++;
			}
		    } else {
			while (*uniStr && (p != *uniStr)) {
			    uniStr++;
			}
		    }
		}
		if (Tcl_UniCharCaseMatch(uniStr, uniPattern, nocase)) {
		    return 1;
		}
		if (*uniStr == 0) {
		    return 0;
		}
		uniStr++;
	    }
	}

	/*
	 * Check for a "?" as the next pattern character. It matches any
	 * single character.
	 */

	if (p == '?') {
	    uniPattern++;
	    uniStr++;
	    continue;
	}

	/*
	 * Check for a "[" as the next pattern character. It is followed by a
	 * list of characters that are acceptable, or by a range (two
	 * characters separated by "-").
	 */

	if (p == '[') {
	    Tcl_UniChar startChar, endChar;

	    uniPattern++;
	    ch1 = (nocase ? Tcl_UniCharToLower(*uniStr) : *uniStr);
	    uniStr++;
	    while (1) {
		if ((*uniPattern == ']') || (*uniPattern == 0)) {
		    return 0;
		}
		startChar = (nocase ? Tcl_UniCharToLower(*uniPattern)
			: *uniPattern);
		uniPattern++;
		if (*uniPattern == '-') {
		    uniPattern++;
		    if (*uniPattern == 0) {
			return 0;
		    }
		    endChar = (nocase ? Tcl_UniCharToLower(*uniPattern)
			    : *uniPattern);
		    uniPattern++;
		    if (((startChar <= ch1) && (ch1 <= endChar))
			    || ((endChar <= ch1) && (ch1 <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */
			break;
		    }
		} else if (startChar == ch1) {
		    break;
		}
	    }
	    while (*uniPattern != ']') {
		if (*uniPattern == 0) {
		    uniPattern--;
		    break;
		}
		uniPattern++;
	    }
	    uniPattern++;
	    continue;
	}

	/*
	 * If the next pattern character is '\', just strip off the '\' so we
	 * do exact matching on the character that follows.
	 */

	if (p == '\\') {
	    if (*(++uniPattern) == '\0') {
		return 0;
	    }
	}

	/*
	 * There's no special character. Just make sure that the next bytes of
	 * each string match.
	 */

	if (nocase) {
	    if (Tcl_UniCharToLower(*uniStr) !=
		    Tcl_UniCharToLower(*uniPattern)) {
		return 0;
	    }
	} else if (*uniStr != *uniPattern) {
	    return 0;
	}
	uniStr++;
	uniPattern++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TclUniCharMatch --
 *
 *	See if a particular Unicode string matches a particular pattern.
 *	Allows case insensitivity. This is the Unicode equivalent of the char*
 *	Tcl_StringCaseMatch. This variant of Tcl_UniCharCaseMatch uses counted
 *	Strings, so embedded NULLs are allowed.
 *
 * Results:
 *	The return value is 1 if string matches pattern, and 0 otherwise. The
 *	matching operation permits the following special characters in the
 *	pattern: *?\[] (see the manual entry for details on what these mean).
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclUniCharMatch(
    const Tcl_UniChar *string,	/* Unicode String. */
    int strLen,			/* Length of String */
    const Tcl_UniChar *pattern,	/* Pattern, which may contain special
				 * characters. */
    int ptnLen,			/* Length of Pattern */
    int nocase)			/* 0 for case sensitive, 1 for insensitive */
{
    const Tcl_UniChar *stringEnd, *patternEnd;
    Tcl_UniChar p;

    stringEnd = string + strLen;
    patternEnd = pattern + ptnLen;

    while (1) {
	/*
	 * See if we're at the end of both the pattern and the string. If so,
	 * we succeeded. If we're at the end of the pattern but not at the end
	 * of the string, we failed.
	 */

	if (pattern == patternEnd) {
	    return (string == stringEnd);
	}
	p = *pattern;
	if ((string == stringEnd) && (p != '*')) {
	    return 0;
	}

	/*
	 * Check for a "*" as the next pattern character. It matches any
	 * substring. We handle this by skipping all the characters up to the
	 * next matching one in the pattern, and then calling ourselves
	 * recursively for each postfix of string, until either we match or we
	 * reach the end of the string.
	 */

	if (p == '*') {
	    /*
	     * Skip all successive *'s in the pattern.
	     */

	    while (*(++pattern) == '*') {
		/* empty body */
	    }
	    if (pattern == patternEnd) {
		return 1;
	    }
	    p = *pattern;
	    if (nocase) {
		p = Tcl_UniCharToLower(p);
	    }
	    while (1) {
		/*
		 * Optimization for matching - cruise through the string
		 * quickly if the next char in the pattern isn't a special
		 * character.
		 */

		if ((p != '[') && (p != '?') && (p != '\\')) {
		    if (nocase) {
			while ((string < stringEnd) && (p != *string)
				&& (p != Tcl_UniCharToLower(*string))) {
			    string++;
			}
		    } else {
			while ((string < stringEnd) && (p != *string)) {
			    string++;
			}
		    }
		}
		if (TclUniCharMatch(string, stringEnd - string,
			pattern, patternEnd - pattern, nocase)) {
		    return 1;
		}
		if (string == stringEnd) {
		    return 0;
		}
		string++;
	    }
	}

	/*
	 * Check for a "?" as the next pattern character. It matches any
	 * single character.
	 */

	if (p == '?') {
	    pattern++;
	    string++;
	    continue;
	}

	/*
	 * Check for a "[" as the next pattern character. It is followed by a
	 * list of characters that are acceptable, or by a range (two
	 * characters separated by "-").
	 */

	if (p == '[') {
	    Tcl_UniChar ch1, startChar, endChar;

	    pattern++;
	    ch1 = (nocase ? Tcl_UniCharToLower(*string) : *string);
	    string++;
	    while (1) {
		if ((*pattern == ']') || (pattern == patternEnd)) {
		    return 0;
		}
		startChar = (nocase ? Tcl_UniCharToLower(*pattern) : *pattern);
		pattern++;
		if (*pattern == '-') {
		    pattern++;
		    if (pattern == patternEnd) {
			return 0;
		    }
		    endChar = (nocase ? Tcl_UniCharToLower(*pattern)
			    : *pattern);
		    pattern++;
		    if (((startChar <= ch1) && (ch1 <= endChar))
			    || ((endChar <= ch1) && (ch1 <= startChar))) {
			/*
			 * Matches ranges of form [a-z] or [z-a].
			 */
			break;
		    }
		} else if (startChar == ch1) {
		    break;
		}
	    }
	    while (*pattern != ']') {
		if (pattern == patternEnd) {
		    pattern--;
		    break;
		}
		pattern++;
	    }
	    pattern++;
	    continue;
	}

	/*
	 * If the next pattern character is '\', just strip off the '\' so we
	 * do exact matching on the character that follows.
	 */

	if (p == '\\') {
	    if (++pattern == patternEnd) {
		return 0;
	    }
	}

	/*
	 * There's no special character. Just make sure that the next bytes of
	 * each string match.
	 */

	if (nocase) {
	    if (Tcl_UniCharToLower(*string) != Tcl_UniCharToLower(*pattern)) {
		return 0;
	    }
	} else if (*string != *pattern) {
	    return 0;
	}
	string++;
	pattern++;
    }
}

/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 4
 * fill-column: 78
 * End:
 */
