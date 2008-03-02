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
**
** Added support for quoted-printable encoding, based on rfc 1521 et al
** quoted-printable encoding specifies that non printable characters (anything
** below 32 and above 126) be encoded as =XX where XX is the hexadecimal value
** of the character.  It also specifies some other behavior to enable 8bit data
** in a mail message with little difficulty (maximum line sizes, protecting
** some cases of whitespace, etc).
**
** Brandon Long, September 2001.
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

/* Max binary chunk size; limited only by available memory */
#define BASE64_MAXBIN (INT_MAX/2 - sizeof(PyStringObject) - 3)

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

PyDoc_STRVAR(doc_a2b_uu, "(ascii) -> bin. Decode a line of uuencoded data");

static PyObject *
binascii_a2b_uu(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int ascii_len, bin_len;

	if ( !PyArg_ParseTuple(args, "t#:a2b_uu", &ascii_data, &ascii_len) )
		return NULL;

	assert(ascii_len >= 0);

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
			** '`' as zero instead of space.
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
		/* Extra '`' may be written as padding in some cases */
		if ( this_ch != ' ' && this_ch != ' '+64 &&
		     this_ch != '\n' && this_ch != '\r' ) {
			PyErr_SetString(Error, "Trailing garbage");
			Py_DECREF(rv);
			return NULL;
		}
	}
	return rv;
}

PyDoc_STRVAR(doc_b2a_uu, "(bin) -> ascii. Uuencode line of data");

static PyObject *
binascii_b2a_uu(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int bin_len;

	if ( !PyArg_ParseTuple(args, "s#:b2a_uu", &bin_data, &bin_len) )
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


static int
binascii_find_valid(unsigned char *s, int slen, int num)
{
	/* Finds & returns the (num+1)th
	** valid character for base64, or -1 if none.
	*/

	int ret = -1;
	unsigned char c, b64val;

	while ((slen > 0) && (ret == -1)) {
		c = *s;
		b64val = table_a2b_base64[c & 0x7f];
		if ( ((c <= 0x7f) && (b64val != (unsigned char)-1)) ) {
			if (num == 0)
				ret = *s;
			num--;
		}

		s++;
		slen--;
	}
	return ret;
}

PyDoc_STRVAR(doc_a2b_base64, "(ascii) -> bin. Decode a line of base64 data");

static PyObject *
binascii_a2b_base64(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int ascii_len, bin_len;
	int quad_pos = 0;

	if ( !PyArg_ParseTuple(args, "t#:a2b_base64", &ascii_data, &ascii_len) )
		return NULL;

	assert(ascii_len >= 0);

	if (ascii_len > INT_MAX - 3)
		return PyErr_NoMemory();

	bin_len = ((ascii_len+3)/4)*3; /* Upper bound, corrected later */

	/* Allocate the buffer */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len)) == NULL )
		return NULL;
	bin_data = (unsigned char *)PyString_AsString(rv);
	bin_len = 0;

	for( ; ascii_len > 0; ascii_len--, ascii_data++) {
		this_ch = *ascii_data;

		if (this_ch > 0x7f ||
		    this_ch == '\r' || this_ch == '\n' || this_ch == ' ')
			continue;

		/* Check for pad sequences and ignore
		** the invalid ones.
		*/
		if (this_ch == BASE64_PAD) {
			if ( (quad_pos < 2) ||
			     ((quad_pos == 2) &&
			      (binascii_find_valid(ascii_data, ascii_len, 1)
			       != BASE64_PAD)) )
			{
				continue;
			}
			else {
				/* A pad sequence means no more input.
				** We've already interpreted the data
				** from the quad at this point.
				*/
				leftbits = 0;
				break;
			}
		}

		this_ch = table_a2b_base64[*ascii_data];
		if ( this_ch == (unsigned char) -1 )
			continue;

		/*
		** Shift it in on the low end, and see if there's
		** a byte ready for output.
		*/
		quad_pos = (quad_pos + 1) & 0x03;
		leftchar = (leftchar << 6) | (this_ch);
		leftbits += 6;

		if ( leftbits >= 8 ) {
			leftbits -= 8;
			*bin_data++ = (leftchar >> leftbits) & 0xff;
			bin_len++;
			leftchar &= ((1 << leftbits) - 1);
		}
 	}

	if (leftbits != 0) {
		PyErr_SetString(Error, "Incorrect padding");
		Py_DECREF(rv);
		return NULL;
	}

	/* And set string size correctly. If the result string is empty
	** (because the input was all invalid) return the shared empty
	** string instead; _PyString_Resize() won't do this for us.
	*/
	if (bin_len > 0)
		_PyString_Resize(&rv, bin_len);
	else {
		Py_DECREF(rv);
		rv = PyString_FromString("");
	}
	return rv;
}

PyDoc_STRVAR(doc_b2a_base64, "(bin) -> ascii. Base64-code line of data");

static PyObject *
binascii_b2a_base64(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int bin_len;

	if ( !PyArg_ParseTuple(args, "s#:b2a_base64", &bin_data, &bin_len) )
		return NULL;

	assert(bin_len >= 0);

	if ( bin_len > BASE64_MAXBIN ) {
		PyErr_SetString(Error, "Too much data for base64 line");
		return NULL;
	}

	/* We're lazy and allocate too much (fixed up later).
	   "+3" leaves room for up to two pad characters and a trailing
	   newline.  Note that 'b' gets encoded as 'Yg==\n' (1 in, 5 out). */
	if ( (rv=PyString_FromStringAndSize(NULL, bin_len*2 + 3)) == NULL )
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

PyDoc_STRVAR(doc_a2b_hqx, "ascii -> bin, done. Decode .hqx coding");

static PyObject *
binascii_a2b_hqx(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int len;
	int done = 0;

	if ( !PyArg_ParseTuple(args, "t#:a2b_hqx", &ascii_data, &len) )
		return NULL;

	assert(len >= 0);

	if (len > INT_MAX - 2)
		return PyErr_NoMemory();

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

PyDoc_STRVAR(doc_rlecode_hqx, "Binhex RLE-code binary data");

static PyObject *
binascii_rlecode_hqx(PyObject *self, PyObject *args)
{
	unsigned char *in_data, *out_data;
	PyObject *rv;
	unsigned char ch;
	int in, inend, len;

	if ( !PyArg_ParseTuple(args, "s#:rlecode_hqx", &in_data, &len) )
		return NULL;

	assert(len >= 0);

	if (len > INT_MAX / 2 - 2)
		return PyErr_NoMemory();

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

PyDoc_STRVAR(doc_b2a_hqx, "Encode .hqx data");

static PyObject *
binascii_b2a_hqx(PyObject *self, PyObject *args)
{
	unsigned char *ascii_data, *bin_data;
	int leftbits = 0;
	unsigned char this_ch;
	unsigned int leftchar = 0;
	PyObject *rv;
	int len;

	if ( !PyArg_ParseTuple(args, "s#:b2a_hqx", &bin_data, &len) )
		return NULL;

	assert(len >= 0);

	if (len > INT_MAX / 2 - 2)
		return PyErr_NoMemory();

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

PyDoc_STRVAR(doc_rledecode_hqx, "Decode hexbin RLE-coded string");

static PyObject *
binascii_rledecode_hqx(PyObject *self, PyObject *args)
{
	unsigned char *in_data, *out_data;
	unsigned char in_byte, in_repeat;
	PyObject *rv;
	int in_len, out_len, out_len_left;

	if ( !PyArg_ParseTuple(args, "s#:rledecode_hqx", &in_data, &in_len) )
		return NULL;

	assert(in_len >= 0);

	/* Empty string is a special case */
	if ( in_len == 0 )
		return Py_BuildValue("s", "");
	else if (in_len > INT_MAX / 2)
		return PyErr_NoMemory();

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
			  if ( out_len > INT_MAX / 2) return PyErr_NoMemory(); \
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

PyDoc_STRVAR(doc_crc_hqx,
"(data, oldcrc) -> newcrc. Compute hqx CRC incrementally");

static PyObject *
binascii_crc_hqx(PyObject *self, PyObject *args)
{
	unsigned char *bin_data;
	unsigned int crc;
	int len;

	if ( !PyArg_ParseTuple(args, "s#i:crc_hqx", &bin_data, &len, &crc) )
		return NULL;

	while(len-- > 0) {
		crc=((crc<<8)&0xff00)^crctab_hqx[((crc>>8)&0xff)^*bin_data++];
	}

	return Py_BuildValue("i", crc);
}

PyDoc_STRVAR(doc_crc32,
"(data, oldcrc = 0) -> newcrc. Compute CRC-32 incrementally");

/*  Crc - 32 BIT ANSI X3.66 CRC checksum files
    Also known as: ISO 3307
**********************************************************************|
*                                                                    *|
* Demonstration program to compute the 32-bit CRC used as the frame  *|
* check sequence in ADCCP (ANSI X3.66, also known as FIPS PUB 71     *|
* and FED-STD-1003, the U.S. versions of CCITT's X.25 link-level     *|
* protocol).  The 32-bit FCS was added via the Federal Register,     *|
* 1 June 1982, p.23798.  I presume but don't know for certain that   *|
* this polynomial is or will be included in CCITT V.41, which        *|
* defines the 16-bit CRC (often called CRC-CCITT) polynomial.  FIPS  *|
* PUB 78 says that the 32-bit FCS reduces otherwise undetected       *|
* errors by a factor of 10^-5 over 16-bit FCS.                       *|
*                                                                    *|
**********************************************************************|

 Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 code or tables extracted from it, as desired without restriction.

 First, the polynomial itself and its table of feedback terms.  The
 polynomial is
 X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 Note that we take it "backwards" and put the highest-order term in
 the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 the MSB being 1.

 Note that the usual hardware shift register implementation, which
 is what we're using (we're merely optimizing it by doing eight-bit
 chunks at a time) shifts bits into the lowest-order term.  In our
 implementation, that means shifting towards the right.  Why do we
 do it this way?  Because the calculated CRC must be transmitted in
 order from highest-order term to lowest-order term.  UARTs transmit
 characters in order from LSB to MSB.  By storing the CRC this way,
 we hand it to the UART in the order low-byte to high-byte; the UART
 sends each low-bit to hight-bit; and the result is transmission bit
 by bit from highest- to lowest-order term without requiring any bit
 shuffling on our part.  Reception works similarly.

 The feedback terms table consists of 256, 32-bit entries.  Notes:

  1. The table can be generated at runtime if desired; code to do so
     is shown later.  It might not be obvious, but the feedback
     terms simply represent the results of eight shift/xor opera-
     tions for all combinations of data and CRC register values.

  2. The CRC accumulation logic is the same for all CRC polynomials,
     be they sixteen or thirty-two bits wide.  You simply choose the
     appropriate table.  Alternatively, because the table can be
     generated at runtime, you can start by generating the table for
     the polynomial in question and use exactly the same "updcrc",
     if your application needn't simultaneously handle two CRC
     polynomials.  (Note, however, that XMODEM is strange.)

  3. For 16-bit CRCs, the table entries need be only 16 bits wide;
     of course, 32-bit entries work OK if the high 16 bits are zero.

  4. The values must be right-shifted by eight bits by the "updcrc"
     logic; the shift must be unsigned (bring in zeroes).  On some
     hardware you could probably optimize the shift in assembler by
     using byte-swap instructions.
********************************************************************/

static unsigned long crc_32_tab[256] = {
0x00000000UL, 0x77073096UL, 0xee0e612cUL, 0x990951baUL, 0x076dc419UL,
0x706af48fUL, 0xe963a535UL, 0x9e6495a3UL, 0x0edb8832UL, 0x79dcb8a4UL,
0xe0d5e91eUL, 0x97d2d988UL, 0x09b64c2bUL, 0x7eb17cbdUL, 0xe7b82d07UL,
0x90bf1d91UL, 0x1db71064UL, 0x6ab020f2UL, 0xf3b97148UL, 0x84be41deUL,
0x1adad47dUL, 0x6ddde4ebUL, 0xf4d4b551UL, 0x83d385c7UL, 0x136c9856UL,
0x646ba8c0UL, 0xfd62f97aUL, 0x8a65c9ecUL, 0x14015c4fUL, 0x63066cd9UL,
0xfa0f3d63UL, 0x8d080df5UL, 0x3b6e20c8UL, 0x4c69105eUL, 0xd56041e4UL,
0xa2677172UL, 0x3c03e4d1UL, 0x4b04d447UL, 0xd20d85fdUL, 0xa50ab56bUL,
0x35b5a8faUL, 0x42b2986cUL, 0xdbbbc9d6UL, 0xacbcf940UL, 0x32d86ce3UL,
0x45df5c75UL, 0xdcd60dcfUL, 0xabd13d59UL, 0x26d930acUL, 0x51de003aUL,
0xc8d75180UL, 0xbfd06116UL, 0x21b4f4b5UL, 0x56b3c423UL, 0xcfba9599UL,
0xb8bda50fUL, 0x2802b89eUL, 0x5f058808UL, 0xc60cd9b2UL, 0xb10be924UL,
0x2f6f7c87UL, 0x58684c11UL, 0xc1611dabUL, 0xb6662d3dUL, 0x76dc4190UL,
0x01db7106UL, 0x98d220bcUL, 0xefd5102aUL, 0x71b18589UL, 0x06b6b51fUL,
0x9fbfe4a5UL, 0xe8b8d433UL, 0x7807c9a2UL, 0x0f00f934UL, 0x9609a88eUL,
0xe10e9818UL, 0x7f6a0dbbUL, 0x086d3d2dUL, 0x91646c97UL, 0xe6635c01UL,
0x6b6b51f4UL, 0x1c6c6162UL, 0x856530d8UL, 0xf262004eUL, 0x6c0695edUL,
0x1b01a57bUL, 0x8208f4c1UL, 0xf50fc457UL, 0x65b0d9c6UL, 0x12b7e950UL,
0x8bbeb8eaUL, 0xfcb9887cUL, 0x62dd1ddfUL, 0x15da2d49UL, 0x8cd37cf3UL,
0xfbd44c65UL, 0x4db26158UL, 0x3ab551ceUL, 0xa3bc0074UL, 0xd4bb30e2UL,
0x4adfa541UL, 0x3dd895d7UL, 0xa4d1c46dUL, 0xd3d6f4fbUL, 0x4369e96aUL,
0x346ed9fcUL, 0xad678846UL, 0xda60b8d0UL, 0x44042d73UL, 0x33031de5UL,
0xaa0a4c5fUL, 0xdd0d7cc9UL, 0x5005713cUL, 0x270241aaUL, 0xbe0b1010UL,
0xc90c2086UL, 0x5768b525UL, 0x206f85b3UL, 0xb966d409UL, 0xce61e49fUL,
0x5edef90eUL, 0x29d9c998UL, 0xb0d09822UL, 0xc7d7a8b4UL, 0x59b33d17UL,
0x2eb40d81UL, 0xb7bd5c3bUL, 0xc0ba6cadUL, 0xedb88320UL, 0x9abfb3b6UL,
0x03b6e20cUL, 0x74b1d29aUL, 0xead54739UL, 0x9dd277afUL, 0x04db2615UL,
0x73dc1683UL, 0xe3630b12UL, 0x94643b84UL, 0x0d6d6a3eUL, 0x7a6a5aa8UL,
0xe40ecf0bUL, 0x9309ff9dUL, 0x0a00ae27UL, 0x7d079eb1UL, 0xf00f9344UL,
0x8708a3d2UL, 0x1e01f268UL, 0x6906c2feUL, 0xf762575dUL, 0x806567cbUL,
0x196c3671UL, 0x6e6b06e7UL, 0xfed41b76UL, 0x89d32be0UL, 0x10da7a5aUL,
0x67dd4accUL, 0xf9b9df6fUL, 0x8ebeeff9UL, 0x17b7be43UL, 0x60b08ed5UL,
0xd6d6a3e8UL, 0xa1d1937eUL, 0x38d8c2c4UL, 0x4fdff252UL, 0xd1bb67f1UL,
0xa6bc5767UL, 0x3fb506ddUL, 0x48b2364bUL, 0xd80d2bdaUL, 0xaf0a1b4cUL,
0x36034af6UL, 0x41047a60UL, 0xdf60efc3UL, 0xa867df55UL, 0x316e8eefUL,
0x4669be79UL, 0xcb61b38cUL, 0xbc66831aUL, 0x256fd2a0UL, 0x5268e236UL,
0xcc0c7795UL, 0xbb0b4703UL, 0x220216b9UL, 0x5505262fUL, 0xc5ba3bbeUL,
0xb2bd0b28UL, 0x2bb45a92UL, 0x5cb36a04UL, 0xc2d7ffa7UL, 0xb5d0cf31UL,
0x2cd99e8bUL, 0x5bdeae1dUL, 0x9b64c2b0UL, 0xec63f226UL, 0x756aa39cUL,
0x026d930aUL, 0x9c0906a9UL, 0xeb0e363fUL, 0x72076785UL, 0x05005713UL,
0x95bf4a82UL, 0xe2b87a14UL, 0x7bb12baeUL, 0x0cb61b38UL, 0x92d28e9bUL,
0xe5d5be0dUL, 0x7cdcefb7UL, 0x0bdbdf21UL, 0x86d3d2d4UL, 0xf1d4e242UL,
0x68ddb3f8UL, 0x1fda836eUL, 0x81be16cdUL, 0xf6b9265bUL, 0x6fb077e1UL,
0x18b74777UL, 0x88085ae6UL, 0xff0f6a70UL, 0x66063bcaUL, 0x11010b5cUL,
0x8f659effUL, 0xf862ae69UL, 0x616bffd3UL, 0x166ccf45UL, 0xa00ae278UL,
0xd70dd2eeUL, 0x4e048354UL, 0x3903b3c2UL, 0xa7672661UL, 0xd06016f7UL,
0x4969474dUL, 0x3e6e77dbUL, 0xaed16a4aUL, 0xd9d65adcUL, 0x40df0b66UL,
0x37d83bf0UL, 0xa9bcae53UL, 0xdebb9ec5UL, 0x47b2cf7fUL, 0x30b5ffe9UL,
0xbdbdf21cUL, 0xcabac28aUL, 0x53b39330UL, 0x24b4a3a6UL, 0xbad03605UL,
0xcdd70693UL, 0x54de5729UL, 0x23d967bfUL, 0xb3667a2eUL, 0xc4614ab8UL,
0x5d681b02UL, 0x2a6f2b94UL, 0xb40bbe37UL, 0xc30c8ea1UL, 0x5a05df1bUL,
0x2d02ef8dUL
};

static PyObject *
binascii_crc32(PyObject *self, PyObject *args)
{ /* By Jim Ahlstrom; All rights transferred to CNRI */
	unsigned char *bin_data;
	unsigned long crc = 0UL;	/* initial value of CRC */
	int len;
	long result;

	if ( !PyArg_ParseTuple(args, "s#|l:crc32", &bin_data, &len, &crc) )
		return NULL;

	crc = ~ crc;
#if SIZEOF_LONG > 4
	/* only want the trailing 32 bits */
	crc &= 0xFFFFFFFFUL;
#endif
	while (len-- > 0)
		crc = crc_32_tab[(crc ^ *bin_data++) & 0xffUL] ^ (crc >> 8);
		/* Note:  (crc >> 8) MUST zero fill on left */

	result = (long)(crc ^ 0xFFFFFFFFUL);
#if SIZEOF_LONG > 4
	/* Extend the sign bit.  This is one way to ensure the result is the
	 * same across platforms.  The other way would be to return an
	 * unbounded unsigned long, but the evidence suggests that lots of
	 * code outside this treats the result as if it were a signed 4-byte
	 * integer.
	 */
	result |= -(result & (1L << 31));
#endif
	return PyInt_FromLong(result);
}


static PyObject *
binascii_hexlify(PyObject *self, PyObject *args)
{
	char* argbuf;
	int arglen;
	PyObject *retval;
	char* retbuf;
	int i, j;

	if (!PyArg_ParseTuple(args, "t#:b2a_hex", &argbuf, &arglen))
		return NULL;

	assert(arglen >= 0);
	if (arglen > INT_MAX / 2)
		return PyErr_NoMemory();

	retval = PyString_FromStringAndSize(NULL, arglen*2);
	if (!retval)
		return NULL;
	retbuf = PyString_AsString(retval);
	if (!retbuf)
		goto finally;

	/* make hex version of string, taken from shamodule.c */
	for (i=j=0; i < arglen; i++) {
		char c;
		c = (argbuf[i] >> 4) & 0xf;
		c = (c>9) ? c+'a'-10 : c + '0';
		retbuf[j++] = c;
		c = argbuf[i] & 0xf;
		c = (c>9) ? c+'a'-10 : c + '0';
		retbuf[j++] = c;
	}
	return retval;

  finally:
	Py_DECREF(retval);
	return NULL;
}

PyDoc_STRVAR(doc_hexlify,
"b2a_hex(data) -> s; Hexadecimal representation of binary data.\n\
\n\
This function is also available as \"hexlify()\".");


static int
to_int(int c)
{
	if (isdigit(c))
		return c - '0';
	else {
		if (isupper(c))
			c = tolower(c);
		if (c >= 'a' && c <= 'f')
			return c - 'a' + 10;
	}
	return -1;
}


static PyObject *
binascii_unhexlify(PyObject *self, PyObject *args)
{
	char* argbuf;
	int arglen;
	PyObject *retval;
	char* retbuf;
	int i, j;

	if (!PyArg_ParseTuple(args, "s#:a2b_hex", &argbuf, &arglen))
		return NULL;

	assert(arglen >= 0);

	/* XXX What should we do about strings with an odd length?  Should
	 * we add an implicit leading zero, or a trailing zero?  For now,
	 * raise an exception.
	 */
	if (arglen % 2) {
		PyErr_SetString(PyExc_TypeError, "Odd-length string");
		return NULL;
	}

	retval = PyString_FromStringAndSize(NULL, (arglen/2));
	if (!retval)
		return NULL;
	retbuf = PyString_AsString(retval);
	if (!retbuf)
		goto finally;

	for (i=j=0; i < arglen; i += 2) {
		int top = to_int(Py_CHARMASK(argbuf[i]));
		int bot = to_int(Py_CHARMASK(argbuf[i+1]));
		if (top == -1 || bot == -1) {
			PyErr_SetString(PyExc_TypeError,
					"Non-hexadecimal digit found");
			goto finally;
		}
		retbuf[j++] = (top << 4) + bot;
	}
	return retval;

  finally:
	Py_DECREF(retval);
	return NULL;
}

PyDoc_STRVAR(doc_unhexlify,
"a2b_hex(hexstr) -> s; Binary data of hexadecimal representation.\n\
\n\
hexstr must contain an even number of hex digits (upper or lower case).\n\
This function is also available as \"unhexlify()\"");

static int table_hex[128] = {
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
   0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
  -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

#define hexval(c) table_hex[(unsigned int)(c)]

#define MAXLINESIZE 76

PyDoc_STRVAR(doc_a2b_qp, "Decode a string of qp-encoded data");

static PyObject*
binascii_a2b_qp(PyObject *self, PyObject *args, PyObject *kwargs)
{
	unsigned int in, out;
	char ch;
	unsigned char *data, *odata;
	unsigned int datalen = 0;
	PyObject *rv;
	static char *kwlist[] = {"data", "header", NULL};
	int header = 0;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|i", kwlist, &data,
	      &datalen, &header))
		return NULL;

	/* We allocate the output same size as input, this is overkill.
	 * The previous implementation used calloc() so we'll zero out the
	 * memory here too, since PyMem_Malloc() does not guarantee that.
	 */
	odata = (unsigned char *) PyMem_Malloc(datalen);
	if (odata == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(odata, datalen, 0);

	in = out = 0;
	while (in < datalen) {
		if (data[in] == '=') {
			in++;
			if (in >= datalen) break;
			/* Soft line breaks */
			if ((data[in] == '\n') || (data[in] == '\r') ||
			    (data[in] == ' ') || (data[in] == '\t')) {
				if (data[in] != '\n') {
					while (in < datalen && data[in] != '\n') in++;
				}
				if (in < datalen) in++;
			}
			else if (data[in] == '=') {
				/* broken case from broken python qp */
				odata[out++] = '=';
				in++;
			}
			else if (((data[in] >= 'A' && data[in] <= 'F') ||
			          (data[in] >= 'a' && data[in] <= 'f') ||
				  (data[in] >= '0' && data[in] <= '9')) &&
			         ((data[in+1] >= 'A' && data[in+1] <= 'F') ||
				  (data[in+1] >= 'a' && data[in+1] <= 'f') ||
				  (data[in+1] >= '0' && data[in+1] <= '9'))) {
				/* hexval */
				ch = hexval(data[in]) << 4;
				in++;
				ch |= hexval(data[in]);
				in++;
				odata[out++] = ch;
			}
			else {
			  odata[out++] = '=';
			}
		}
		else if (header && data[in] == '_') {
			odata[out++] = ' ';
			in++;
		}
		else {
			odata[out] = data[in];
			in++;
			out++;
		}
	}
	if ((rv = PyString_FromStringAndSize((char *)odata, out)) == NULL) {
		PyMem_Free(odata);
		return NULL;
	}
	PyMem_Free(odata);
	return rv;
}

static int
to_hex (unsigned char ch, unsigned char *s)
{
	unsigned int uvalue = ch;

	s[1] = "0123456789ABCDEF"[uvalue % 16];
	uvalue = (uvalue / 16);
	s[0] = "0123456789ABCDEF"[uvalue % 16];
	return 0;
}

PyDoc_STRVAR(doc_b2a_qp,
"b2a_qp(data, quotetabs=0, istext=1, header=0) -> s; \n\
 Encode a string using quoted-printable encoding. \n\
\n\
On encoding, when istext is set, newlines are not encoded, and white \n\
space at end of lines is.  When istext is not set, \\r and \\n (CR/LF) are \n\
both encoded.  When quotetabs is set, space and tabs are encoded.");

/* XXX: This is ridiculously complicated to be backward compatible
 * (mostly) with the quopri module.  It doesn't re-create the quopri
 * module bug where text ending in CRLF has the CR encoded */
static PyObject*
binascii_b2a_qp (PyObject *self, PyObject *args, PyObject *kwargs)
{
	unsigned int in, out;
	unsigned char *data, *odata;
	unsigned int datalen = 0, odatalen = 0;
	PyObject *rv;
	unsigned int linelen = 0;
	static char *kwlist[] = {"data", "quotetabs", "istext", "header", NULL};
	int istext = 1;
	int quotetabs = 0;
	int header = 0;
	unsigned char ch;
	int crlf = 0;
	unsigned char *p;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s#|iii", kwlist, &data,
	      &datalen, &quotetabs, &istext, &header))
		return NULL;

	/* See if this string is using CRLF line ends */
	/* XXX: this function has the side effect of converting all of
	 * the end of lines to be the same depending on this detection
	 * here */
	p = (unsigned char *) strchr((char *)data, '\n');
	if ((p != NULL) && (p > data) && (*(p-1) == '\r'))
		crlf = 1;

	/* First, scan to see how many characters need to be encoded */
	in = 0;
	while (in < datalen) {
		if ((data[in] > 126) ||
		    (data[in] == '=') ||
		    (header && data[in] == '_') ||
		    ((data[in] == '.') && (linelen == 1)) ||
		    (!istext && ((data[in] == '\r') || (data[in] == '\n'))) ||
		    ((data[in] == '\t' || data[in] == ' ') && (in + 1 == datalen)) ||
		    ((data[in] < 33) &&
		     (data[in] != '\r') && (data[in] != '\n') &&
		     (quotetabs && ((data[in] != '\t') || (data[in] != ' ')))))
		{
			if ((linelen + 3) >= MAXLINESIZE) {
				linelen = 0;
				if (crlf)
					odatalen += 3;
				else
					odatalen += 2;
			}
			linelen += 3;
			odatalen += 3;
			in++;
		}
		else {
		  	if (istext &&
			    ((data[in] == '\n') ||
			     ((in+1 < datalen) && (data[in] == '\r') &&
			     (data[in+1] == '\n'))))
			{
			  	linelen = 0;
				/* Protect against whitespace on end of line */
				if (in && ((data[in-1] == ' ') || (data[in-1] == '\t')))
					odatalen += 2;
				if (crlf)
					odatalen += 2;
				else
					odatalen += 1;
				if (data[in] == '\r')
					in += 2;
				else
					in++;
			}
			else {
				if ((in + 1 != datalen) &&
				    (data[in+1] != '\n') &&
				    (linelen + 1) >= MAXLINESIZE) {
					linelen = 0;
					if (crlf)
						odatalen += 3;
					else
						odatalen += 2;
				}
				linelen++;
				odatalen++;
				in++;
			}
		}
	}

	/* We allocate the output same size as input, this is overkill.
	 * The previous implementation used calloc() so we'll zero out the
	 * memory here too, since PyMem_Malloc() does not guarantee that.
	 */
	odata = (unsigned char *) PyMem_Malloc(odatalen);
	if (odata == NULL) {
		PyErr_NoMemory();
		return NULL;
	}
	memset(odata, odatalen, 0);

	in = out = linelen = 0;
	while (in < datalen) {
		if ((data[in] > 126) ||
		    (data[in] == '=') ||
		    (header && data[in] == '_') ||
		    ((data[in] == '.') && (linelen == 1)) ||
		    (!istext && ((data[in] == '\r') || (data[in] == '\n'))) ||
		    ((data[in] == '\t' || data[in] == ' ') && (in + 1 == datalen)) ||
		    ((data[in] < 33) &&
		     (data[in] != '\r') && (data[in] != '\n') &&
		     (quotetabs && ((data[in] != '\t') || (data[in] != ' ')))))
		{
			if ((linelen + 3 )>= MAXLINESIZE) {
				odata[out++] = '=';
				if (crlf) odata[out++] = '\r';
				odata[out++] = '\n';
				linelen = 0;
			}
			odata[out++] = '=';
			to_hex(data[in], &odata[out]);
			out += 2;
			in++;
			linelen += 3;
		}
		else {
		  	if (istext &&
			    ((data[in] == '\n') ||
			     ((in+1 < datalen) && (data[in] == '\r') &&
			     (data[in+1] == '\n'))))
			{
			  	linelen = 0;
				/* Protect against whitespace on end of line */
				if (out && ((odata[out-1] == ' ') || (odata[out-1] == '\t'))) {
					ch = odata[out-1];
					odata[out-1] = '=';
					to_hex(ch, &odata[out]);
					out += 2;
				}

				if (crlf) odata[out++] = '\r';
				odata[out++] = '\n';
				if (data[in] == '\r')
					in += 2;
				else
					in++;
			}
			else {
				if ((in + 1 != datalen) &&
				    (data[in+1] != '\n') &&
				    (linelen + 1) >= MAXLINESIZE) {
					odata[out++] = '=';
					if (crlf) odata[out++] = '\r';
					odata[out++] = '\n';
					linelen = 0;
				}
				linelen++;
				if (header && data[in] == ' ') {
					odata[out++] = '_';
					in++;
				}
				else {
					odata[out++] = data[in++];
				}
			}
		}
	}
	if ((rv = PyString_FromStringAndSize((char *)odata, out)) == NULL) {
		PyMem_Free(odata);
		return NULL;
	}
	PyMem_Free(odata);
	return rv;
}

/* List of functions defined in the module */

static struct PyMethodDef binascii_module_methods[] = {
	{"a2b_uu",     binascii_a2b_uu,     METH_VARARGS, doc_a2b_uu},
	{"b2a_uu",     binascii_b2a_uu,     METH_VARARGS, doc_b2a_uu},
	{"a2b_base64", binascii_a2b_base64, METH_VARARGS, doc_a2b_base64},
	{"b2a_base64", binascii_b2a_base64, METH_VARARGS, doc_b2a_base64},
	{"a2b_hqx",    binascii_a2b_hqx,    METH_VARARGS, doc_a2b_hqx},
	{"b2a_hqx",    binascii_b2a_hqx,    METH_VARARGS, doc_b2a_hqx},
	{"b2a_hex",    binascii_hexlify,    METH_VARARGS, doc_hexlify},
	{"a2b_hex",    binascii_unhexlify,  METH_VARARGS, doc_unhexlify},
	{"hexlify",    binascii_hexlify,    METH_VARARGS, doc_hexlify},
	{"unhexlify",  binascii_unhexlify,  METH_VARARGS, doc_unhexlify},
	{"rlecode_hqx",   binascii_rlecode_hqx, METH_VARARGS, doc_rlecode_hqx},
	{"rledecode_hqx", binascii_rledecode_hqx, METH_VARARGS,
	 doc_rledecode_hqx},
	{"crc_hqx",    binascii_crc_hqx,    METH_VARARGS, doc_crc_hqx},
	{"crc32",      binascii_crc32,      METH_VARARGS, doc_crc32},
	{"a2b_qp", (PyCFunction)binascii_a2b_qp, METH_VARARGS | METH_KEYWORDS,
	  doc_a2b_qp},
	{"b2a_qp", (PyCFunction)binascii_b2a_qp, METH_VARARGS | METH_KEYWORDS,
          doc_b2a_qp},
	{NULL, NULL}			     /* sentinel */
};


/* Initialization function for the module (*must* be called initbinascii) */
PyDoc_STRVAR(doc_binascii, "Conversion between binary data and ASCII");

PyMODINIT_FUNC
initbinascii(void)
{
	PyObject *m, *d, *x;

	/* Create the module and add the functions */
	m = Py_InitModule("binascii", binascii_module_methods);

	d = PyModule_GetDict(m);
	x = PyString_FromString(doc_binascii);
	PyDict_SetItemString(d, "__doc__", x);
	Py_XDECREF(x);

	Error = PyErr_NewException("binascii.Error", NULL, NULL);
	PyDict_SetItemString(d, "Error", Error);
	Incomplete = PyErr_NewException("binascii.Incomplete", NULL, NULL);
	PyDict_SetItemString(d, "Incomplete", Incomplete);
}
