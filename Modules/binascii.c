/***********************************************************
Copyright 1991, 1992, 1993, 1994 by Stichting Mathematisch Centrum,
Amsterdam, The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

/*
** Routines to represent binary data in ASCII and vice-versa
**
** This module currently supports the following encodings:
** uuencode:
**     	each line encodes 45 bytes (except possibly the last)
**	First char encodes (binary) length, rest data
**	each char encodes 6 bits, as follows:
**	binary: 01234567 abcdefgh ijklmnop
**	ascii:  012345 67abcd efghij klmnop
**	ASCII encoding method is "excess-space": 000000 is encoded as ' ', etc.
**	short binary data is zero-extended (so the bits are always in the
**	right place), this does *not* reflect in the length.
** base64:
**      Line breaks are insignificant, but lines are at most 76 chars
**      each char encodes 6 bits, in similar order as uucode/hqx. Encoding
**      is done via a table.
**      Short binary data is filled (in ASCII) with '='.
** hqx:
**	File starts with introductory text, real data starts and ends
**	with colons.
**	Data consists of three similar parts: info, datafork, resourcefork.
**	Each part is protected (at the end) with a 16-bit crc
**	The binary data is run-length encoded, and then ascii-fied:
**	binary: 01234567 abcdefgh ijklmnop
**	ascii:  012345 67abcd efghij klmnop
**	ASCII encoding is table-driven, see the code.
**	Short binary data results in the runt ascii-byte being output with
**	the bits in the right place.
**
** While I was reading dozens of programs that encode or decode the formats
** here (documentation? hihi:-) I have formulated Jansen's Observation:
**
**	Programs that encode binary data in ASCII are written in
**	such a style that they are as unreadable as possible. Devices used
**	include unnecessary global variables, burying important tables
**	in unrelated sourcefiles, putting functions in include files,
**	using seemingly-descriptive variable names for different purposes,
**	calls to empty subroutines and a host of others.
**
** I have attempted to break with this tradition, but I guess that that
** does make the performance sub-optimal. Oh well, too bad...
**
** Jack Jansen, CWI, July 1995.
*/


#include "Python.h"

static PyObject *Error;
static PyObject *Incomplete;

/*
** hqx lookup table, ascii->binary.
*/

#define RUNCHAR 0x90

#define DONE 0x7F
#define SKIP 0x7E
#define FAIL 0x7D

static unsigned char table_a2b_hqx[256] = {
/*       ^@    ^A    ^B    ^C    ^D    ^E    ^F    ^G   */
/* 0*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       \b    \t    \n    ^K    ^L    \r    ^N    ^O   */
/* 1*/	FAIL, FAIL, SKIP, FAIL, FAIL, SKIP, FAIL, FAIL,
/*       ^P    ^Q    ^R    ^S    ^T    ^U    ^V    ^W   */
/* 2*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       ^X    ^Y    ^Z    ^[    ^\    ^]    ^^    ^_   */
/* 3*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*              !     "     #     $     %     &     '   */
/* 4*/	FAIL, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
/*        (     )     *     +     ,     -     .     /   */
/* 5*/	0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, FAIL, FAIL,
/*        0     1     2     3     4     5     6     7   */
/* 6*/	0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, FAIL,
/*        8     9     :     ;     <     =     >     ?   */
/* 7*/	0x14, 0x15, DONE, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        @     A     B     C     D     E     F     G   */
/* 8*/	0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
/*        H     I     J     K     L     M     N     O   */
/* 9*/	0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, FAIL,
/*        P     Q     R     S     T     U     V     W   */
/*10*/	0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, FAIL,
/*        X     Y     Z     [     \     ]     ^     _   */
/*11*/	0x2C, 0x2D, 0x2E, 0x2F, FAIL, FAIL, FAIL, FAIL,
/*        `     a     b     c     d     e     f     g   */
/*12*/	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, FAIL,
/*        h     i     j     k     l     m     n     o   */
/*13*/	0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, FAIL, FAIL,
/*        p     q     r     s     t     u     v     w   */
/*14*/	0x3D, 0x3E, 0x3F, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        x     y     z     {     |     }     ~    ^?   */
/*15*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*16*/	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
	FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
};

static unsigned char table_b2a_hqx[] =
"!\"#$%&'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr";

static char table_a2b_base64[] = {
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
	52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, 0,-1,-1, /* Note PAD->0 */
	-1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
	15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
	-1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
	41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

#define BASE64_PAD '='
#define BASE64_MAXBIN 57	/* Max binary chunk size (76 char line) */

static unsigned char table_b2a_base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";



static unsigned short crctab_hqx[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

static char doc_a2b_uu[] = "(ascii) -> bin. Decode a line of uuencoded data";

static PyObject *
binascii_a2b_uu(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int ascii_len, bin_len;
	
	if ( !PyArg_ParseTuple(args, "s#", &ascii_data, &ascii_len) )
		return NULL;

	/* First byte: binary data length (in bytes) */
	bin_len = (*ascii_data++ - ' ') & 077;
	ascii_len--;

	/* Allocate the buffer */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len)) == NULL )
		return NULL;
	bin_data = (unsigned char *)PyString_AsString(rv);
	
	for( ; bin_len > 0 ; ascii_len--, ascii_data++ ) {
		this_ch = *ascii_data;
		if ( this_ch == '\n' || this_ch == '\r' || ascii_len <= 0) {
			/*
			** Whitespace. Assume some spaces got eaten at
			** end-of-line. (We check this later)
			*/
			this_ch = 0;
	        } else {
			/* Check the character for legality
			** The 64 in stead of the expected 63 is because
			** there are a few uuencodes out there that use
			** '@' as zero instead of space.
			*/
			if ( this_ch < ' ' || this_ch > (' ' + 64)) {
				PyErr_SetString(Error, "Illegal char");
				Py_DECREF(rv);
				return NULL;
			}
			this_ch = (this_ch - ' ') & 077;
		}
		/*
		** Shift it in on the low end, and see if there's
		** a byte ready for output.
		*/
		leftchar = (leftchar << 6) | (this_ch);
		leftbits += 6;
		if ( leftbits >= 8 ) {
			leftbits -= 8;
			*bin_data++ = (leftchar >> leftbits) & 0xff;
			leftchar &= ((1 << leftbits) - 1);
			bin_len--;
		}
	}
	/*
	** Finally, check that if there's anything left on the line
	** that it's whitespace only.
	*/
	while( ascii_len-- > 0 ) {
		this_ch = *ascii_data++;
		/* Extra '@' may be written as padding in some cases */
		if ( this_ch != ' ' && this_ch != '@' &&
		     this_ch != '\n' && this_ch != '\r' ) {
			PyErr_SetString(Error, "Trailing garbage");
			Py_DECREF(rv);
			return NULL;
		}
	}
	return rv;
}

static char doc_b2a_uu[] = "(bin) -> ascii. Uuencode line of data";
	
static PyObject *
binascii_b2a_uu(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int bin_len;
	
	if ( !PyArg_ParseTuple(args, "s#", &bin_data, &bin_len) )
		return NULL;
	if ( bin_len > 45 ) {
		/* The 45 is a limit that appears in all uuencode's */
		PyErr_SetString(Error, "At most 45 bytes at once");
		return NULL;
	}

	/* We're lazy and allocate to much (fixed up later) */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len*2)) == NULL )
		return NULL;
	ascii_data = (unsigned char *)PyString_AsString(rv);

	/* Store the length */
	*ascii_data++ = ' ' + (bin_len & 077);
	
	for( ; bin_len > 0 || leftbits != 0 ; bin_len--, bin_data++ ) {
		/* Shift the data (or padding) into our buffer */
		if ( bin_len > 0 )	/* Data */
			leftchar = (leftchar << 8) | *bin_data;
		else			/* Padding */
			leftchar <<= 8;
		leftbits += 8;

		/* See if there are 6-bit groups ready */
		while ( leftbits >= 6 ) {
			this_ch = (leftchar >> (leftbits-6)) & 0x3f;
			leftbits -= 6;
			*ascii_data++ = this_ch + ' ';
		}
	}
	*ascii_data++ = '\n';	/* Append a courtesy newline */
	
	_PyString_Resize(&rv, (ascii_data -
			       (unsigned char *)PyString_AsString(rv)));
	return rv;
}

static char doc_a2b_base64[] = "(ascii) -> bin. Decode a line of base64 data";

static PyObject *
binascii_a2b_base64(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	int npad = 0;
	PyObject *rv;
	int ascii_len, bin_len;
	
	if ( !PyArg_ParseTuple(args, "s#", &ascii_data, &ascii_len) )
		return NULL;

	bin_len = ((ascii_len+3)/4)*3; /* Upper bound, corrected later */

	/* Allocate the buffer */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len)) == NULL )
		return NULL;
	bin_data = (unsigned char *)PyString_AsString(rv);
	bin_len = 0;
	for( ; ascii_len > 0 ; ascii_len--, ascii_data++ ) {
		/* Skip some punctuation */
		this_ch = (*ascii_data & 0x7f);
		if ( this_ch == '\r' || this_ch == '\n' || this_ch == ' ' )
			continue;
		
		if ( this_ch == BASE64_PAD )
			npad++;
		this_ch = table_a2b_base64[(*ascii_data) & 0x7f];
		if ( this_ch == (unsigned char) -1 ) continue;
		/*
		** Shift it in on the low end, and see if there's
		** a byte ready for output.
		*/
		leftchar = (leftchar << 6) | (this_ch);
		leftbits += 6;
		if ( leftbits >= 8 ) {
			leftbits -= 8;
			*bin_data++ = (leftchar >> leftbits) & 0xff;
			leftchar &= ((1 << leftbits) - 1);
			bin_len++;
		}
	}
	/* Check that no bits are left */
	if ( leftbits ) {
		PyErr_SetString(Error, "Incorrect padding");
		Py_DECREF(rv);
		return NULL;
	}
	/* and remove any padding */
	bin_len -= npad;
	/* and set string size correctly */
	_PyString_Resize(&rv, bin_len);
	return rv;
}

static char doc_b2a_base64[] = "(bin) -> ascii. Base64-code line of data";
	
static PyObject *
binascii_b2a_base64(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int bin_len;
	
	if ( !PyArg_ParseTuple(args, "s#", &bin_data, &bin_len) )
		return NULL;
	if ( bin_len > BASE64_MAXBIN ) {
		PyErr_SetString(Error, "Too much data for base64 line");
		return NULL;
	}
	
	/* We're lazy and allocate to much (fixed up later) */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len*2)) == NULL )
		return NULL;
	ascii_data = (unsigned char *)PyString_AsString(rv);

	for( ; bin_len > 0 ; bin_len--, bin_data++ ) {
		/* Shift the data into our buffer */
		leftchar = (leftchar << 8) | *bin_data;
		leftbits += 8;

		/* See if there are 6-bit groups ready */
		while ( leftbits >= 6 ) {
			this_ch = (leftchar >> (leftbits-6)) & 0x3f;
			leftbits -= 6;
			*ascii_data++ = table_b2a_base64[this_ch];
		}
	}
	if ( leftbits == 2 ) {
		*ascii_data++ = table_b2a_base64[(leftchar&3) << 4];
		*ascii_data++ = BASE64_PAD;
		*ascii_data++ = BASE64_PAD;
	} else if ( leftbits == 4 ) {
		*ascii_data++ = table_b2a_base64[(leftchar&0xf) << 2];
		*ascii_data++ = BASE64_PAD;
	} 
	*ascii_data++ = '\n';	/* Append a courtesy newline */
	
	_PyString_Resize(&rv, (ascii_data -
			       (unsigned char *)PyString_AsString(rv)));
	return rv;
}

static char doc_a2b_hqx[] = "ascii -> bin, done. Decode .hqx coding";

static PyObject *
binascii_a2b_hqx(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int len;
	int done = 0;
	
	if ( !PyArg_ParseTuple(args, "s#", &ascii_data, &len) )
		return NULL;

	/* Allocate a string that is too big (fixed later) */
	if ( (rv=PyString_FromStringAndSize(NULL, len)) == NULL )
		return NULL;
	bin_data = (unsigned char *)PyString_AsString(rv);

	for( ; len > 0 ; len--, ascii_data++ ) {
		/* Get the byte and look it up */
		this_ch = table_a2b_hqx[*ascii_data];
		if ( this_ch == SKIP )
			continue;
		if ( this_ch == FAIL ) {
			PyErr_SetString(Error, "Illegal char");
			Py_DECREF(rv);
			return NULL;
		}
		if ( this_ch == DONE ) {
			/* The terminating colon */
			done = 1;
			break;
		}

		/* Shift it into the buffer and see if any bytes are ready */
		leftchar = (leftchar << 6) | (this_ch);
		leftbits += 6;
		if ( leftbits >= 8 ) {
			leftbits -= 8;
			*bin_data++ = (leftchar >> leftbits) & 0xff;
			leftchar &= ((1 << leftbits) - 1);
		}
	}
	
	if ( leftbits && !done ) {
		PyErr_SetString(Incomplete,
				"String has incomplete number of bytes");
		Py_DECREF(rv);
		return NULL;
	}
	_PyString_Resize(
		&rv, (bin_data - (unsigned char *)PyString_AsString(rv)));
	if (rv) {
		PyObject *rrv = Py_BuildValue("Oi", rv, done);
		Py_DECREF(rv);
		return rrv;
	}

	return NULL;
}

static char doc_rlecode_hqx[] = "Binhex RLE-code binary data";

static PyObject *
binascii_rlecode_hqx(self, args)
	PyObject *self;
PyObject *args;
{
	unsigned char *in_data, *out_data;
	PyObject *rv;
	unsigned char ch;
	int in, inend, len;
	
	if ( !PyArg_ParseTuple(args, "s#", &in_data, &len) )
		return NULL;

	/* Worst case: output is twice as big as input (fixed later) */
	if ( (rv=PyString_FromStringAndSize(NULL, len*2)) == NULL )
		return NULL;
	out_data = (unsigned char *)PyString_AsString(rv);
	
	for( in=0; in<len; in++) {
		ch = in_data[in];
		if ( ch == RUNCHAR ) {
			/* RUNCHAR. Escape it. */
			*out_data++ = RUNCHAR;
			*out_data++ = 0;
		} else {
			/* Check how many following are the same */
			for(inend=in+1;
			    inend<len && in_data[inend] == ch &&
				    inend < in+255;
			    inend++) ;
			if ( inend - in > 3 ) {
				/* More than 3 in a row. Output RLE. */
				*out_data++ = ch;
				*out_data++ = RUNCHAR;
				*out_data++ = inend-in;
				in = inend-1;
			} else {
				/* Less than 3. Output the byte itself */
				*out_data++ = ch;
			}
		}
	}
	_PyString_Resize(&rv, (out_data -
			       (unsigned char *)PyString_AsString(rv)));
	return rv;
}

static char doc_b2a_hqx[] = "Encode .hqx data";
	
static PyObject *
binascii_b2a_hqx(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int len;
	
	if ( !PyArg_ParseTuple(args, "s#", &bin_data, &len) )
		return NULL;

	/* Allocate a buffer that is at least large enough */
	if ( (rv=PyString_FromStringAndSize(NULL, len*2)) == NULL )
		return NULL;
	ascii_data = (unsigned char *)PyString_AsString(rv);
	
	for( ; len > 0 ; len--, bin_data++ ) {
		/* Shift into our buffer, and output any 6bits ready */
		leftchar = (leftchar << 8) | *bin_data;
		leftbits += 8;
		while ( leftbits >= 6 ) {
			this_ch = (leftchar >> (leftbits-6)) & 0x3f;
			leftbits -= 6;
			*ascii_data++ = table_b2a_hqx[this_ch];
		}
	}
	/* Output a possible runt byte */
	if ( leftbits ) {
		leftchar <<= (6-leftbits);
		*ascii_data++ = table_b2a_hqx[leftchar & 0x3f];
	}
	_PyString_Resize(&rv, (ascii_data -
			       (unsigned char *)PyString_AsString(rv)));
	return rv;
}

static char doc_rledecode_hqx[] = "Decode hexbin RLE-coded string";
	
static PyObject *
binascii_rledecode_hqx(self, args)
	PyObject *self;
        PyObject *args;
{
	unsigned char *in_data, *out_data;
	unsigned char in_byte, in_repeat;
	PyObject *rv;
	int in_len, out_len, out_len_left;

	if ( !PyArg_ParseTuple(args, "s#", &in_data, &in_len) )
		return NULL;

	/* Empty string is a special case */
	if ( in_len == 0 )
		return Py_BuildValue("s", "");

	/* Allocate a buffer of reasonable size. Resized when needed */
	out_len = in_len*2;
	if ( (rv=PyString_FromStringAndSize(NULL, out_len)) == NULL )
		return NULL;
	out_len_left = out_len;
	out_data = (unsigned char *)PyString_AsString(rv);

	/*
	** We need two macros here to get/put bytes and handle
	** end-of-buffer for input and output strings.
	*/
#define INBYTE(b) \
	do { \
	         if ( --in_len < 0 ) { \
			   PyErr_SetString(Incomplete, ""); \
			   Py_DECREF(rv); \
			   return NULL; \
		 } \
		 b = *in_data++; \
	} while(0)
	    
#define OUTBYTE(b) \
	do { \
		 if ( --out_len_left < 0 ) { \
			  _PyString_Resize(&rv, 2*out_len); \
			  if ( rv == NULL ) return NULL; \
			  out_data = (unsigned char *)PyString_AsString(rv) \
								 + out_len; \
			  out_len_left = out_len-1; \
			  out_len = out_len * 2; \
		 } \
		 *out_data++ = b; \
	} while(0)

		/*
		** Handle first byte separately (since we have to get angry
		** in case of an orphaned RLE code).
		*/
		INBYTE(in_byte);

	if (in_byte == RUNCHAR) {
		INBYTE(in_repeat);
		if (in_repeat != 0) {
			/* Note Error, not Incomplete (which is at the end
			** of the string only). This is a programmer error.
			*/
			PyErr_SetString(Error, "Orphaned RLE code at start");
			Py_DECREF(rv);
			return NULL;
		}
		OUTBYTE(RUNCHAR);
	} else {
		OUTBYTE(in_byte);
	}
	
	while( in_len > 0 ) {
		INBYTE(in_byte);

		if (in_byte == RUNCHAR) {
			INBYTE(in_repeat);
			if ( in_repeat == 0 ) {
				/* Just an escaped RUNCHAR value */
				OUTBYTE(RUNCHAR);
			} else {
				/* Pick up value and output a sequence of it */
				in_byte = out_data[-1];
				while ( --in_repeat > 0 )
					OUTBYTE(in_byte);
			}
		} else {
			/* Normal byte */
			OUTBYTE(in_byte);
		}
	}
	_PyString_Resize(&rv, (out_data -
			       (unsigned char *)PyString_AsString(rv)));
	return rv;
}

static char doc_crc_hqx[] =
"(data, oldcrc) -> newcrc. Compute hqx CRC incrementally";

static PyObject *
binascii_crc_hqx(self, args)
	PyObject *self;
PyObject *args;
{
	unsigned char *bin_data;
	unsigned int crc;
	int len;
	
	if ( !PyArg_ParseTuple(args, "s#i", &bin_data, &len, &crc) )
		return NULL;

	while(len--) {
		crc=((crc<<8)&0xff00)^crctab_hqx[((crc>>8)&0xff)^*bin_data++];
	}

	return Py_BuildValue("i", crc);
}

/* List of functions defined in the module */

static struct PyMethodDef binascii_module_methods[] = {
	{"a2b_uu",		binascii_a2b_uu,	1,	doc_a2b_uu},
	{"b2a_uu",		binascii_b2a_uu,	1,	doc_b2a_uu},
	{"a2b_base64",		binascii_a2b_base64,	1,
	 doc_a2b_base64},
	{"b2a_base64",		binascii_b2a_base64,	1,
	 doc_b2a_base64},
	{"a2b_hqx",		binascii_a2b_hqx,	1,	doc_a2b_hqx},
	{"b2a_hqx",		binascii_b2a_hqx,	1,	doc_b2a_hqx},
	{"rlecode_hqx",		binascii_rlecode_hqx,	1,
	 doc_rlecode_hqx},
	{"rledecode_hqx",	binascii_rledecode_hqx,	1,
	 doc_rledecode_hqx},
	{"crc_hqx",		binascii_crc_hqx,	1,	doc_crc_hqx},
	{NULL,			NULL}		/* sentinel */
};


/* Initialization function for the module (*must* be called initbinascii) */
static char doc_binascii[] = "Conversion between binary data and ASCII";

void
initbinascii()
{
	PyObject *m, *d, *x;

	/* Create the module and add the functions */
	m = Py_InitModule("binascii", binascii_module_methods);

	d = PyModule_GetDict(m);
	x = PyString_FromString(doc_binascii);
	PyDict_SetItemString(d, "__doc__", x);

	Error = PyString_FromString("binascii.Error");
	PyDict_SetItemString(d, "Error", Error);
	Incomplete = PyString_FromString("binascii.Incomplete");
	PyDict_SetItemString(d, "Incomplete", Incomplete);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module binascii");
}
