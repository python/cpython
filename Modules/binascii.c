/*
** Routines to represent binary data in ASCII and vice-versa
**
** This module currently supports the following encodings:
** uuencode:
**      each line encodes 45 bytes (except possibly the last)
**      First char encodes (binary) length, rest data
**      each char encodes 6 bits, as follows:
**      binary: 01234567 abcdefgh ijklmnop
**      ascii:  012345 67abcd efghij klmnop
**      ASCII encoding method is "excess-space": 000000 is encoded as ' ', etc.
**      short binary data is zero-extended (so the bits are always in the
**      right place), this does *not* reflect in the length.
** base64:
**      Line breaks are insignificant, but lines are at most 76 chars
**      each char encodes 6 bits, in similar order as uucode/hqx. Encoding
**      is done via a table.
**      Short binary data is filled (in ASCII) with '='.
** hqx:
**      File starts with introductory text, real data starts and ends
**      with colons.
**      Data consists of three similar parts: info, datafork, resourcefork.
**      Each part is protected (at the end) with a 16-bit crc
**      The binary data is run-length encoded, and then ascii-fied:
**      binary: 01234567 abcdefgh ijklmnop
**      ascii:  012345 67abcd efghij klmnop
**      ASCII encoding is table-driven, see the code.
**      Short binary data results in the runt ascii-byte being output with
**      the bits in the right place.
**
** While I was reading dozens of programs that encode or decode the formats
** here (documentation? hihi:-) I have formulated Jansen's Observation:
**
**      Programs that encode binary data in ASCII are written in
**      such a style that they are as unreadable as possible. Devices used
**      include unnecessary global variables, burying important tables
**      in unrelated sourcefiles, putting functions in include files,
**      using seemingly-descriptive variable names for different purposes,
**      calls to empty subroutines and a host of others.
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

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#ifdef USE_ZLIB_CRC32
#include "zlib.h"
#endif

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
/* 0*/  FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       \b    \t    \n    ^K    ^L    \r    ^N    ^O   */
/* 1*/  FAIL, FAIL, SKIP, FAIL, FAIL, SKIP, FAIL, FAIL,
/*       ^P    ^Q    ^R    ^S    ^T    ^U    ^V    ^W   */
/* 2*/  FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*       ^X    ^Y    ^Z    ^[    ^\    ^]    ^^    ^_   */
/* 3*/  FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*              !     "     #     $     %     &     '   */
/* 4*/  FAIL, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06,
/*        (     )     *     +     ,     -     .     /   */
/* 5*/  0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, FAIL, FAIL,
/*        0     1     2     3     4     5     6     7   */
/* 6*/  0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, FAIL,
/*        8     9     :     ;     <     =     >     ?   */
/* 7*/  0x14, 0x15, DONE, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        @     A     B     C     D     E     F     G   */
/* 8*/  0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D,
/*        H     I     J     K     L     M     N     O   */
/* 9*/  0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, FAIL,
/*        P     Q     R     S     T     U     V     W   */
/*10*/  0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, FAIL,
/*        X     Y     Z     [     \     ]     ^     _   */
/*11*/  0x2C, 0x2D, 0x2E, 0x2F, FAIL, FAIL, FAIL, FAIL,
/*        `     a     b     c     d     e     f     g   */
/*12*/  0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, FAIL,
/*        h     i     j     k     l     m     n     o   */
/*13*/  0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, FAIL, FAIL,
/*        p     q     r     s     t     u     v     w   */
/*14*/  0x3D, 0x3E, 0x3F, FAIL, FAIL, FAIL, FAIL, FAIL,
/*        x     y     z     {     |     }     ~    ^?   */
/*15*/  FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
/*16*/  FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL, FAIL,
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
#define BASE64_MAXBIN ((PY_SSIZE_T_MAX - 3) / 2)

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

/*[clinic input]
output preset file
module binascii
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=44c6f840ce708f0c]*/

/*[python input]

class ascii_buffer_converter(CConverter):
    type = 'Py_buffer'
    converter = 'ascii_buffer_converter'
    impl_by_reference = True
    c_default = "{NULL, NULL}"

    def cleanup(self):
        name = self.name
        return "".join(["if (", name, ".obj)\n   PyBuffer_Release(&", name, ");\n"])

[python start generated code]*/
/*[python end generated code: output=da39a3ee5e6b4b0d input=3eb7b63610da92cd]*/

static int
ascii_buffer_converter(PyObject *arg, Py_buffer *buf)
{
    if (arg == NULL) {
        PyBuffer_Release(buf);
        return 1;
    }
    if (PyUnicode_Check(arg)) {
        if (PyUnicode_READY(arg) < 0)
            return 0;
        if (!PyUnicode_IS_ASCII(arg)) {
            PyErr_SetString(PyExc_ValueError,
                            "string argument should contain only ASCII characters");
            return 0;
        }
        assert(PyUnicode_KIND(arg) == PyUnicode_1BYTE_KIND);
        buf->buf = (void *) PyUnicode_1BYTE_DATA(arg);
        buf->len = PyUnicode_GET_LENGTH(arg);
        buf->obj = NULL;
        return 1;
    }
    if (PyObject_GetBuffer(arg, buf, PyBUF_SIMPLE) != 0) {
        PyErr_Format(PyExc_TypeError,
                     "argument should be bytes, buffer or ASCII string, "
                     "not %R", Py_TYPE(arg));
        return 0;
    }
    if (!PyBuffer_IsContiguous(buf, 'C')) {
        PyErr_Format(PyExc_TypeError,
                     "argument should be a contiguous buffer, "
                     "not %R", Py_TYPE(arg));
        PyBuffer_Release(buf);
        return 0;
    }
    return Py_CLEANUP_SUPPORTED;
}

#include "clinic/binascii.c.h"

/*[clinic input]
binascii.a2b_uu

    data: ascii_buffer
    /

Decode a line of uuencoded data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_uu_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=5779f39b0b48459f input=7cafeaf73df63d1c]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t ascii_len, bin_len;

    ascii_data = data->buf;
    ascii_len = data->len;

    assert(ascii_len >= 0);

    /* First byte: binary data length (in bytes) */
    bin_len = (*ascii_data++ - ' ') & 077;
    ascii_len--;

    /* Allocate the buffer */
    if ( (rv=PyBytes_FromStringAndSize(NULL, bin_len)) == NULL )
        return NULL;
    bin_data = (unsigned char *)PyBytes_AS_STRING(rv);

    for( ; bin_len > 0 ; ascii_len--, ascii_data++ ) {
        /* XXX is it really best to add NULs if there's no more data */
        this_ch = (ascii_len > 0) ? *ascii_data : 0;
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

/*[clinic input]
binascii.b2a_uu

    data: Py_buffer
    /

Uuencode line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_uu_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=181021b69bb9a414 input=00fdf458ce8b465b]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t bin_len;

    bin_data = data->buf;
    bin_len = data->len;
    if ( bin_len > 45 ) {
        /* The 45 is a limit that appears in all uuencode's */
        PyErr_SetString(Error, "At most 45 bytes at once");
        return NULL;
    }

    /* We're lazy and allocate to much (fixed up later) */
    if ( (rv=PyBytes_FromStringAndSize(NULL, 2 + (bin_len+2)/3*4)) == NULL )
        return NULL;
    ascii_data = (unsigned char *)PyBytes_AS_STRING(rv);

    /* Store the length */
    *ascii_data++ = ' ' + (bin_len & 077);

    for( ; bin_len > 0 || leftbits != 0 ; bin_len--, bin_data++ ) {
        /* Shift the data (or padding) into our buffer */
        if ( bin_len > 0 )              /* Data */
            leftchar = (leftchar << 8) | *bin_data;
        else                            /* Padding */
            leftchar <<= 8;
        leftbits += 8;

        /* See if there are 6-bit groups ready */
        while ( leftbits >= 6 ) {
            this_ch = (leftchar >> (leftbits-6)) & 0x3f;
            leftbits -= 6;
            *ascii_data++ = this_ch + ' ';
        }
    }
    *ascii_data++ = '\n';       /* Append a courtesy newline */

    if (_PyBytes_Resize(&rv,
                       (ascii_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    return rv;
}


static int
binascii_find_valid(unsigned char *s, Py_ssize_t slen, int num)
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

/*[clinic input]
binascii.a2b_base64

    data: ascii_buffer
    /

Decode a line of base64 data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_base64_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=3e351b702bed56d2 input=5872acf6e1cac243]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t ascii_len, bin_len;
    int quad_pos = 0;

    ascii_data = data->buf;
    ascii_len = data->len;

    assert(ascii_len >= 0);

    if (ascii_len > PY_SSIZE_T_MAX - 3)
        return PyErr_NoMemory();

    bin_len = ((ascii_len+3)/4)*3; /* Upper bound, corrected later */

    /* Allocate the buffer */
    if ( (rv=PyBytes_FromStringAndSize(NULL, bin_len)) == NULL )
        return NULL;
    bin_data = (unsigned char *)PyBytes_AS_STRING(rv);
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
    ** string instead; _PyBytes_Resize() won't do this for us.
    */
    if (bin_len > 0) {
        if (_PyBytes_Resize(&rv, bin_len) < 0) {
            Py_CLEAR(rv);
        }
    }
    else {
        Py_DECREF(rv);
        rv = PyBytes_FromStringAndSize("", 0);
    }
    return rv;
}


/*[clinic input]
binascii.b2a_base64

    data: Py_buffer
    /

Base64-code line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_base64_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=3cd61fbee2913285 input=14ec4e47371174a9]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t bin_len;

    bin_data = data->buf;
    bin_len = data->len;

    assert(bin_len >= 0);

    if ( bin_len > BASE64_MAXBIN ) {
        PyErr_SetString(Error, "Too much data for base64 line");
        return NULL;
    }

    /* We're lazy and allocate too much (fixed up later).
       "+3" leaves room for up to two pad characters and a trailing
       newline.  Note that 'b' gets encoded as 'Yg==\n' (1 in, 5 out). */
    if ( (rv=PyBytes_FromStringAndSize(NULL, bin_len*2 + 3)) == NULL )
        return NULL;
    ascii_data = (unsigned char *)PyBytes_AS_STRING(rv);

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
    *ascii_data++ = '\n';       /* Append a courtesy newline */

    if (_PyBytes_Resize(&rv,
                       (ascii_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    return rv;
}

/*[clinic input]
binascii.a2b_hqx

    data: ascii_buffer
    /

Decode .hqx coding.
[clinic start generated code]*/

static PyObject *
binascii_a2b_hqx_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=60bcdbbd28b105cd input=0d914c680e0eed55]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t len;
    int done = 0;

    ascii_data = data->buf;
    len = data->len;

    assert(len >= 0);

    if (len > PY_SSIZE_T_MAX - 2)
        return PyErr_NoMemory();

    /* Allocate a string that is too big (fixed later)
       Add two to the initial length to prevent interning which
       would preclude subsequent resizing.  */
    if ( (rv=PyBytes_FromStringAndSize(NULL, len+2)) == NULL )
        return NULL;
    bin_data = (unsigned char *)PyBytes_AS_STRING(rv);

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
    if (_PyBytes_Resize(&rv,
                       (bin_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    if (rv) {
        PyObject *rrv = Py_BuildValue("Oi", rv, done);
        Py_DECREF(rv);
        return rrv;
    }

    return NULL;
}


/*[clinic input]
binascii.rlecode_hqx

    data: Py_buffer
    /

Binhex RLE-code binary data.
[clinic start generated code]*/

static PyObject *
binascii_rlecode_hqx_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=0905da344dbf0648 input=e1f1712447a82b09]*/
{
    unsigned char *in_data, *out_data;
    PyObject *rv;
    unsigned char ch;
    Py_ssize_t in, inend, len;

    in_data = data->buf;
    len = data->len;

    assert(len >= 0);

    if (len > PY_SSIZE_T_MAX / 2 - 2)
        return PyErr_NoMemory();

    /* Worst case: output is twice as big as input (fixed later) */
    if ( (rv=PyBytes_FromStringAndSize(NULL, len*2+2)) == NULL )
        return NULL;
    out_data = (unsigned char *)PyBytes_AS_STRING(rv);

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
                *out_data++ = (unsigned char) (inend-in);
                in = inend-1;
            } else {
                /* Less than 3. Output the byte itself */
                *out_data++ = ch;
            }
        }
    }
    if (_PyBytes_Resize(&rv,
                       (out_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    return rv;
}


/*[clinic input]
binascii.b2a_hqx

    data: Py_buffer
    /

Encode .hqx data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_hqx_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=5a987810d5e3cdbb input=9596ebe019fe12ba]*/
{
    unsigned char *ascii_data, *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t len;

    bin_data = data->buf;
    len = data->len;

    assert(len >= 0);

    if (len > PY_SSIZE_T_MAX / 2 - 2)
        return PyErr_NoMemory();

    /* Allocate a buffer that is at least large enough */
    if ( (rv=PyBytes_FromStringAndSize(NULL, len*2+2)) == NULL )
        return NULL;
    ascii_data = (unsigned char *)PyBytes_AS_STRING(rv);

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
    if (_PyBytes_Resize(&rv,
                       (ascii_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    return rv;
}


/*[clinic input]
binascii.rledecode_hqx

    data: Py_buffer
    /

Decode hexbin RLE-coded string.
[clinic start generated code]*/

static PyObject *
binascii_rledecode_hqx_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=f7afd89b789946ab input=54cdd49fc014402c]*/
{
    unsigned char *in_data, *out_data;
    unsigned char in_byte, in_repeat;
    PyObject *rv;
    Py_ssize_t in_len, out_len, out_len_left;

    in_data = data->buf;
    in_len = data->len;

    assert(in_len >= 0);

    /* Empty string is a special case */
    if ( in_len == 0 )
        return PyBytes_FromStringAndSize("", 0);
    else if (in_len > PY_SSIZE_T_MAX / 2)
        return PyErr_NoMemory();

    /* Allocate a buffer of reasonable size. Resized when needed */
    out_len = in_len*2;
    if ( (rv=PyBytes_FromStringAndSize(NULL, out_len)) == NULL )
        return NULL;
    out_len_left = out_len;
    out_data = (unsigned char *)PyBytes_AS_STRING(rv);

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
                      if ( out_len > PY_SSIZE_T_MAX / 2) return PyErr_NoMemory(); \
                      if (_PyBytes_Resize(&rv, 2*out_len) < 0) \
                        { Py_XDECREF(rv); return NULL; } \
                      out_data = (unsigned char *)PyBytes_AS_STRING(rv) \
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
    if (_PyBytes_Resize(&rv,
                       (out_data -
                        (unsigned char *)PyBytes_AS_STRING(rv))) < 0) {
        Py_CLEAR(rv);
    }
    return rv;
}


/*[clinic input]
binascii.crc_hqx -> int

    data: Py_buffer
    crc: int
    /

Compute hqx CRC incrementally.
[clinic start generated code]*/

static int
binascii_crc_hqx_impl(PyModuleDef *module, Py_buffer *data, int crc)
/*[clinic end generated code: output=634dac18dfa863d7 input=68060931b2f51c8a]*/
{
    unsigned char *bin_data;
    unsigned int ucrc = (unsigned int)crc;
    Py_ssize_t len;

    bin_data = data->buf;
    len = data->len;

    while(len-- > 0) {
        ucrc=((ucrc<<8)&0xff00)^crctab_hqx[((ucrc>>8)&0xff)^*bin_data++];
    }

    return (int)ucrc;
}

#ifndef USE_ZLIB_CRC32
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

static unsigned int crc_32_tab[256] = {
0x00000000U, 0x77073096U, 0xee0e612cU, 0x990951baU, 0x076dc419U,
0x706af48fU, 0xe963a535U, 0x9e6495a3U, 0x0edb8832U, 0x79dcb8a4U,
0xe0d5e91eU, 0x97d2d988U, 0x09b64c2bU, 0x7eb17cbdU, 0xe7b82d07U,
0x90bf1d91U, 0x1db71064U, 0x6ab020f2U, 0xf3b97148U, 0x84be41deU,
0x1adad47dU, 0x6ddde4ebU, 0xf4d4b551U, 0x83d385c7U, 0x136c9856U,
0x646ba8c0U, 0xfd62f97aU, 0x8a65c9ecU, 0x14015c4fU, 0x63066cd9U,
0xfa0f3d63U, 0x8d080df5U, 0x3b6e20c8U, 0x4c69105eU, 0xd56041e4U,
0xa2677172U, 0x3c03e4d1U, 0x4b04d447U, 0xd20d85fdU, 0xa50ab56bU,
0x35b5a8faU, 0x42b2986cU, 0xdbbbc9d6U, 0xacbcf940U, 0x32d86ce3U,
0x45df5c75U, 0xdcd60dcfU, 0xabd13d59U, 0x26d930acU, 0x51de003aU,
0xc8d75180U, 0xbfd06116U, 0x21b4f4b5U, 0x56b3c423U, 0xcfba9599U,
0xb8bda50fU, 0x2802b89eU, 0x5f058808U, 0xc60cd9b2U, 0xb10be924U,
0x2f6f7c87U, 0x58684c11U, 0xc1611dabU, 0xb6662d3dU, 0x76dc4190U,
0x01db7106U, 0x98d220bcU, 0xefd5102aU, 0x71b18589U, 0x06b6b51fU,
0x9fbfe4a5U, 0xe8b8d433U, 0x7807c9a2U, 0x0f00f934U, 0x9609a88eU,
0xe10e9818U, 0x7f6a0dbbU, 0x086d3d2dU, 0x91646c97U, 0xe6635c01U,
0x6b6b51f4U, 0x1c6c6162U, 0x856530d8U, 0xf262004eU, 0x6c0695edU,
0x1b01a57bU, 0x8208f4c1U, 0xf50fc457U, 0x65b0d9c6U, 0x12b7e950U,
0x8bbeb8eaU, 0xfcb9887cU, 0x62dd1ddfU, 0x15da2d49U, 0x8cd37cf3U,
0xfbd44c65U, 0x4db26158U, 0x3ab551ceU, 0xa3bc0074U, 0xd4bb30e2U,
0x4adfa541U, 0x3dd895d7U, 0xa4d1c46dU, 0xd3d6f4fbU, 0x4369e96aU,
0x346ed9fcU, 0xad678846U, 0xda60b8d0U, 0x44042d73U, 0x33031de5U,
0xaa0a4c5fU, 0xdd0d7cc9U, 0x5005713cU, 0x270241aaU, 0xbe0b1010U,
0xc90c2086U, 0x5768b525U, 0x206f85b3U, 0xb966d409U, 0xce61e49fU,
0x5edef90eU, 0x29d9c998U, 0xb0d09822U, 0xc7d7a8b4U, 0x59b33d17U,
0x2eb40d81U, 0xb7bd5c3bU, 0xc0ba6cadU, 0xedb88320U, 0x9abfb3b6U,
0x03b6e20cU, 0x74b1d29aU, 0xead54739U, 0x9dd277afU, 0x04db2615U,
0x73dc1683U, 0xe3630b12U, 0x94643b84U, 0x0d6d6a3eU, 0x7a6a5aa8U,
0xe40ecf0bU, 0x9309ff9dU, 0x0a00ae27U, 0x7d079eb1U, 0xf00f9344U,
0x8708a3d2U, 0x1e01f268U, 0x6906c2feU, 0xf762575dU, 0x806567cbU,
0x196c3671U, 0x6e6b06e7U, 0xfed41b76U, 0x89d32be0U, 0x10da7a5aU,
0x67dd4accU, 0xf9b9df6fU, 0x8ebeeff9U, 0x17b7be43U, 0x60b08ed5U,
0xd6d6a3e8U, 0xa1d1937eU, 0x38d8c2c4U, 0x4fdff252U, 0xd1bb67f1U,
0xa6bc5767U, 0x3fb506ddU, 0x48b2364bU, 0xd80d2bdaU, 0xaf0a1b4cU,
0x36034af6U, 0x41047a60U, 0xdf60efc3U, 0xa867df55U, 0x316e8eefU,
0x4669be79U, 0xcb61b38cU, 0xbc66831aU, 0x256fd2a0U, 0x5268e236U,
0xcc0c7795U, 0xbb0b4703U, 0x220216b9U, 0x5505262fU, 0xc5ba3bbeU,
0xb2bd0b28U, 0x2bb45a92U, 0x5cb36a04U, 0xc2d7ffa7U, 0xb5d0cf31U,
0x2cd99e8bU, 0x5bdeae1dU, 0x9b64c2b0U, 0xec63f226U, 0x756aa39cU,
0x026d930aU, 0x9c0906a9U, 0xeb0e363fU, 0x72076785U, 0x05005713U,
0x95bf4a82U, 0xe2b87a14U, 0x7bb12baeU, 0x0cb61b38U, 0x92d28e9bU,
0xe5d5be0dU, 0x7cdcefb7U, 0x0bdbdf21U, 0x86d3d2d4U, 0xf1d4e242U,
0x68ddb3f8U, 0x1fda836eU, 0x81be16cdU, 0xf6b9265bU, 0x6fb077e1U,
0x18b74777U, 0x88085ae6U, 0xff0f6a70U, 0x66063bcaU, 0x11010b5cU,
0x8f659effU, 0xf862ae69U, 0x616bffd3U, 0x166ccf45U, 0xa00ae278U,
0xd70dd2eeU, 0x4e048354U, 0x3903b3c2U, 0xa7672661U, 0xd06016f7U,
0x4969474dU, 0x3e6e77dbU, 0xaed16a4aU, 0xd9d65adcU, 0x40df0b66U,
0x37d83bf0U, 0xa9bcae53U, 0xdebb9ec5U, 0x47b2cf7fU, 0x30b5ffe9U,
0xbdbdf21cU, 0xcabac28aU, 0x53b39330U, 0x24b4a3a6U, 0xbad03605U,
0xcdd70693U, 0x54de5729U, 0x23d967bfU, 0xb3667a2eU, 0xc4614ab8U,
0x5d681b02U, 0x2a6f2b94U, 0xb40bbe37U, 0xc30c8ea1U, 0x5a05df1bU,
0x2d02ef8dU
};
#endif  /* USE_ZLIB_CRC32 */

/*[clinic input]
binascii.crc32 -> unsigned_int

    data: Py_buffer
    crc: unsigned_int(bitwise=True) = 0
    /

Compute CRC-32 incrementally.
[clinic start generated code]*/

static unsigned int
binascii_crc32_impl(PyModuleDef *module, Py_buffer *data, unsigned int crc)
/*[clinic end generated code: output=620a961643393c4f input=bbe340bc99d25aa8]*/

#ifdef USE_ZLIB_CRC32
/* This was taken from zlibmodule.c PyZlib_crc32 (but is PY_SSIZE_T_CLEAN) */
{
    Byte *buf;
    Py_ssize_t len;
    int signed_val;

    buf = (Byte*)data->buf;
    len = data->len;
    signed_val = crc32(crc, buf, len);
    return (unsigned int)signed_val & 0xffffffffU;
}
#else  /* USE_ZLIB_CRC32 */
{ /* By Jim Ahlstrom; All rights transferred to CNRI */
    unsigned char *bin_data;
    Py_ssize_t len;
    unsigned int result;

    bin_data = data->buf;
    len = data->len;

    crc = ~ crc;
    while (len-- > 0) {
        crc = crc_32_tab[(crc ^ *bin_data++) & 0xff] ^ (crc >> 8);
        /* Note:  (crc >> 8) MUST zero fill on left */
    }

    result = (crc ^ 0xFFFFFFFF);
    return result & 0xffffffff;
}
#endif  /* USE_ZLIB_CRC32 */

/*[clinic input]
binascii.b2a_hex

    data: Py_buffer
    /

Hexadecimal representation of binary data.

The return value is a bytes object.  This function is also
available as "hexlify()".
[clinic start generated code]*/

static PyObject *
binascii_b2a_hex_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=179318922c2f8fda input=96423cfa299ff3b1]*/
{
    char* argbuf;
    Py_ssize_t arglen;
    PyObject *retval;
    char* retbuf;
    Py_ssize_t i, j;

    argbuf = data->buf;
    arglen = data->len;

    assert(arglen >= 0);
    if (arglen > PY_SSIZE_T_MAX / 2)
        return PyErr_NoMemory();

    retval = PyBytes_FromStringAndSize(NULL, arglen*2);
    if (!retval)
        return NULL;
    retbuf = PyBytes_AS_STRING(retval);

    /* make hex version of string, taken from shamodule.c */
    for (i=j=0; i < arglen; i++) {
        unsigned char c;
        c = (argbuf[i] >> 4) & 0xf;
        retbuf[j++] = Py_hexdigits[c];
        c = argbuf[i] & 0xf;
        retbuf[j++] = Py_hexdigits[c];
    }
    return retval;
}

/*[clinic input]
binascii.hexlify = binascii.b2a_hex

Hexadecimal representation of binary data.

The return value is a bytes object.
[clinic start generated code]*/

static PyObject *
binascii_hexlify_impl(PyModuleDef *module, Py_buffer *data)
/*[clinic end generated code: output=6098440091fb61dc input=2e3afae7f083f061]*/
{
    return binascii_b2a_hex_impl(module, data);
}

static int
to_int(int c)
{
    if (Py_ISDIGIT(c))
        return c - '0';
    else {
        if (Py_ISUPPER(c))
            c = Py_TOLOWER(c);
        if (c >= 'a' && c <= 'f')
            return c - 'a' + 10;
    }
    return -1;
}


/*[clinic input]
binascii.a2b_hex

    hexstr: ascii_buffer
    /

Binary data of hexadecimal representation.

hexstr must contain an even number of hex digits (upper or lower case).
This function is also available as "unhexlify()".
[clinic start generated code]*/

static PyObject *
binascii_a2b_hex_impl(PyModuleDef *module, Py_buffer *hexstr)
/*[clinic end generated code: output=d61da452b5c6d290 input=9e1e7f2f94db24fd]*/
{
    char* argbuf;
    Py_ssize_t arglen;
    PyObject *retval;
    char* retbuf;
    Py_ssize_t i, j;

    argbuf = hexstr->buf;
    arglen = hexstr->len;

    assert(arglen >= 0);

    /* XXX What should we do about strings with an odd length?  Should
     * we add an implicit leading zero, or a trailing zero?  For now,
     * raise an exception.
     */
    if (arglen % 2) {
        PyErr_SetString(Error, "Odd-length string");
        return NULL;
    }

    retval = PyBytes_FromStringAndSize(NULL, (arglen/2));
    if (!retval)
        return NULL;
    retbuf = PyBytes_AS_STRING(retval);

    for (i=j=0; i < arglen; i += 2) {
        int top = to_int(Py_CHARMASK(argbuf[i]));
        int bot = to_int(Py_CHARMASK(argbuf[i+1]));
        if (top == -1 || bot == -1) {
            PyErr_SetString(Error,
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

/*[clinic input]
binascii.unhexlify = binascii.a2b_hex

Binary data of hexadecimal representation.

hexstr must contain an even number of hex digits (upper or lower case).
[clinic start generated code]*/

static PyObject *
binascii_unhexlify_impl(PyModuleDef *module, Py_buffer *hexstr)
/*[clinic end generated code: output=17cec7544499803e input=dd8c012725f462da]*/
{
    return binascii_a2b_hex_impl(module, hexstr);
}

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


/*[clinic input]
binascii.a2b_qp

    data: ascii_buffer
    header: int(c_default="0") = False

Decode a string of qp-encoded data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_qp_impl(PyModuleDef *module, Py_buffer *data, int header)
/*[clinic end generated code: output=a44ef88270352114 input=5187a0d3d8e54f3b]*/
{
    Py_ssize_t in, out;
    char ch;
    unsigned char *ascii_data, *odata;
    Py_ssize_t datalen = 0;
    PyObject *rv;

    ascii_data = data->buf;
    datalen = data->len;

    /* We allocate the output same size as input, this is overkill.
     * The previous implementation used calloc() so we'll zero out the
     * memory here too, since PyMem_Malloc() does not guarantee that.
     */
    odata = (unsigned char *) PyMem_Malloc(datalen);
    if (odata == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    memset(odata, 0, datalen);

    in = out = 0;
    while (in < datalen) {
        if (ascii_data[in] == '=') {
            in++;
            if (in >= datalen) break;
            /* Soft line breaks */
            if ((ascii_data[in] == '\n') || (ascii_data[in] == '\r')) {
                if (ascii_data[in] != '\n') {
                    while (in < datalen && ascii_data[in] != '\n') in++;
                }
                if (in < datalen) in++;
            }
            else if (ascii_data[in] == '=') {
                /* broken case from broken python qp */
                odata[out++] = '=';
                in++;
            }
            else if (((ascii_data[in] >= 'A' && ascii_data[in] <= 'F') ||
                      (ascii_data[in] >= 'a' && ascii_data[in] <= 'f') ||
                      (ascii_data[in] >= '0' && ascii_data[in] <= '9')) &&
                     ((ascii_data[in+1] >= 'A' && ascii_data[in+1] <= 'F') ||
                      (ascii_data[in+1] >= 'a' && ascii_data[in+1] <= 'f') ||
                      (ascii_data[in+1] >= '0' && ascii_data[in+1] <= '9'))) {
                /* hexval */
                ch = hexval(ascii_data[in]) << 4;
                in++;
                ch |= hexval(ascii_data[in]);
                in++;
                odata[out++] = ch;
            }
            else {
              odata[out++] = '=';
            }
        }
        else if (header && ascii_data[in] == '_') {
            odata[out++] = ' ';
            in++;
        }
        else {
            odata[out] = ascii_data[in];
            in++;
            out++;
        }
    }
    if ((rv = PyBytes_FromStringAndSize((char *)odata, out)) == NULL) {
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

/* XXX: This is ridiculously complicated to be backward compatible
 * (mostly) with the quopri module.  It doesn't re-create the quopri
 * module bug where text ending in CRLF has the CR encoded */

/*[clinic input]
binascii.b2a_qp

    data: Py_buffer
    quotetabs: int(c_default="0") = False
    istext: int(c_default="1") = True
    header: int(c_default="0") = False

Encode a string using quoted-printable encoding.

On encoding, when istext is set, newlines are not encoded, and white
space at end of lines is.  When istext is not set, \r and \n (CR/LF)
are both encoded.  When quotetabs is set, space and tabs are encoded.
[clinic start generated code]*/

static PyObject *
binascii_b2a_qp_impl(PyModuleDef *module, Py_buffer *data, int quotetabs, int istext, int header)
/*[clinic end generated code: output=ff2991ba640fff3e input=7f2a9aaa008e92b2]*/
{
    Py_ssize_t in, out;
    unsigned char *databuf, *odata;
    Py_ssize_t datalen = 0, odatalen = 0;
    PyObject *rv;
    unsigned int linelen = 0;
    unsigned char ch;
    int crlf = 0;
    unsigned char *p;

    databuf = data->buf;
    datalen = data->len;

    /* See if this string is using CRLF line ends */
    /* XXX: this function has the side effect of converting all of
     * the end of lines to be the same depending on this detection
     * here */
    p = (unsigned char *) memchr(databuf, '\n', datalen);
    if ((p != NULL) && (p > databuf) && (*(p-1) == '\r'))
        crlf = 1;

    /* First, scan to see how many characters need to be encoded */
    in = 0;
    while (in < datalen) {
        if ((databuf[in] > 126) ||
            (databuf[in] == '=') ||
            (header && databuf[in] == '_') ||
            ((databuf[in] == '.') && (linelen == 0) &&
             (databuf[in+1] == '\n' || databuf[in+1] == '\r' || databuf[in+1] == 0)) ||
            (!istext && ((databuf[in] == '\r') || (databuf[in] == '\n'))) ||
            ((databuf[in] == '\t' || databuf[in] == ' ') && (in + 1 == datalen)) ||
            ((databuf[in] < 33) &&
             (databuf[in] != '\r') && (databuf[in] != '\n') &&
             (quotetabs || ((databuf[in] != '\t') && (databuf[in] != ' ')))))
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
                ((databuf[in] == '\n') ||
                 ((in+1 < datalen) && (databuf[in] == '\r') &&
                 (databuf[in+1] == '\n'))))
            {
                linelen = 0;
                /* Protect against whitespace on end of line */
                if (in && ((databuf[in-1] == ' ') || (databuf[in-1] == '\t')))
                    odatalen += 2;
                if (crlf)
                    odatalen += 2;
                else
                    odatalen += 1;
                if (databuf[in] == '\r')
                    in += 2;
                else
                    in++;
            }
            else {
                if ((in + 1 != datalen) &&
                    (databuf[in+1] != '\n') &&
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
    memset(odata, 0, odatalen);

    in = out = linelen = 0;
    while (in < datalen) {
        if ((databuf[in] > 126) ||
            (databuf[in] == '=') ||
            (header && databuf[in] == '_') ||
            ((databuf[in] == '.') && (linelen == 0) &&
             (databuf[in+1] == '\n' || databuf[in+1] == '\r' || databuf[in+1] == 0)) ||
            (!istext && ((databuf[in] == '\r') || (databuf[in] == '\n'))) ||
            ((databuf[in] == '\t' || databuf[in] == ' ') && (in + 1 == datalen)) ||
            ((databuf[in] < 33) &&
             (databuf[in] != '\r') && (databuf[in] != '\n') &&
             (quotetabs ||
            (!quotetabs && ((databuf[in] != '\t') && (databuf[in] != ' '))))))
        {
            if ((linelen + 3 )>= MAXLINESIZE) {
                odata[out++] = '=';
                if (crlf) odata[out++] = '\r';
                odata[out++] = '\n';
                linelen = 0;
            }
            odata[out++] = '=';
            to_hex(databuf[in], &odata[out]);
            out += 2;
            in++;
            linelen += 3;
        }
        else {
            if (istext &&
                ((databuf[in] == '\n') ||
                 ((in+1 < datalen) && (databuf[in] == '\r') &&
                 (databuf[in+1] == '\n'))))
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
                if (databuf[in] == '\r')
                    in += 2;
                else
                    in++;
            }
            else {
                if ((in + 1 != datalen) &&
                    (databuf[in+1] != '\n') &&
                    (linelen + 1) >= MAXLINESIZE) {
                    odata[out++] = '=';
                    if (crlf) odata[out++] = '\r';
                    odata[out++] = '\n';
                    linelen = 0;
                }
                linelen++;
                if (header && databuf[in] == ' ') {
                    odata[out++] = '_';
                    in++;
                }
                else {
                    odata[out++] = databuf[in++];
                }
            }
        }
    }
    if ((rv = PyBytes_FromStringAndSize((char *)odata, out)) == NULL) {
        PyMem_Free(odata);
        return NULL;
    }
    PyMem_Free(odata);
    return rv;
}

/* List of functions defined in the module */

static struct PyMethodDef binascii_module_methods[] = {
    BINASCII_A2B_UU_METHODDEF
    BINASCII_B2A_UU_METHODDEF
    BINASCII_A2B_BASE64_METHODDEF
    BINASCII_B2A_BASE64_METHODDEF
    BINASCII_A2B_HQX_METHODDEF
    BINASCII_B2A_HQX_METHODDEF
    BINASCII_A2B_HEX_METHODDEF
    BINASCII_B2A_HEX_METHODDEF
    BINASCII_HEXLIFY_METHODDEF
    BINASCII_UNHEXLIFY_METHODDEF
    BINASCII_RLECODE_HQX_METHODDEF
    BINASCII_RLEDECODE_HQX_METHODDEF
    BINASCII_CRC_HQX_METHODDEF
    BINASCII_CRC32_METHODDEF
    BINASCII_A2B_QP_METHODDEF
    BINASCII_B2A_QP_METHODDEF
    {NULL, NULL}                             /* sentinel */
};


/* Initialization function for the module (*must* be called PyInit_binascii) */
PyDoc_STRVAR(doc_binascii, "Conversion between binary data and ASCII");


static struct PyModuleDef binasciimodule = {
    PyModuleDef_HEAD_INIT,
    "binascii",
    doc_binascii,
    -1,
    binascii_module_methods,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit_binascii(void)
{
    PyObject *m, *d;

    /* Create the module and add the functions */
    m = PyModule_Create(&binasciimodule);
    if (m == NULL)
        return NULL;

    d = PyModule_GetDict(m);

    Error = PyErr_NewException("binascii.Error", PyExc_ValueError, NULL);
    PyDict_SetItemString(d, "Error", Error);
    Incomplete = PyErr_NewException("binascii.Incomplete", NULL, NULL);
    PyDict_SetItemString(d, "Incomplete", Incomplete);
    if (PyErr_Occurred()) {
        Py_DECREF(m);
        m = NULL;
    }
    return m;
}
