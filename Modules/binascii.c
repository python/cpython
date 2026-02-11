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

#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_long.h"          // _PyLong_DigitValue
#include "pycore_strhex.h"        // _Py_strhex_bytes_with_sep()
#ifdef USE_ZLIB_CRC32
#  include "zlib.h"
#endif

typedef struct binascii_state {
    PyObject *Error;
    PyObject *Incomplete;
} binascii_state;

static inline binascii_state *
get_binascii_state(PyObject *module)
{
    return (binascii_state *)PyModule_GetState(module);
}


/* Align to 64 bytes for L1 cache line friendliness */
static const unsigned char table_a2b_base64[] Py_ALIGNED(64) = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,64,-1,-1, /* PAD->64 detected by fast path */
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1,

    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
};

#define BASE64_PAD '='

/* Max binary chunk size; limited only by available memory */
#define BASE64_MAXBIN ((PY_SSIZE_T_MAX - 3) / 2)

/*
 * Fast base64 encoding/decoding helpers.
 *
 * Process complete groups without loop-carried dependencies.
 */

/* Align to 64 bytes for L1 cache line friendliness */
static const unsigned char table_b2a_base64[] Py_ALIGNED(64) =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Encode 3 bytes into 4 base64 characters. */
static inline void
base64_encode_trio(const unsigned char *in, unsigned char *out,
                   const unsigned char *table)
{
    unsigned int combined = ((unsigned int)in[0] << 16) |
                            ((unsigned int)in[1] << 8) |
                            (unsigned int)in[2];
    out[0] = table[(combined >> 18) & 0x3f];
    out[1] = table[(combined >> 12) & 0x3f];
    out[2] = table[(combined >> 6) & 0x3f];
    out[3] = table[combined & 0x3f];
}

/* Encode multiple complete 3-byte groups.
 * Returns the number of input bytes processed (always a multiple of 3).
 */
static inline Py_ssize_t
base64_encode_fast(const unsigned char *in, Py_ssize_t in_len,
                   unsigned char *out, const unsigned char *table)
{
    Py_ssize_t n_trios = in_len / 3;
    const unsigned char *in_end = in + n_trios * 3;

    while (in < in_end) {
        base64_encode_trio(in, out, table);
        in += 3;
        out += 4;
    }

    return n_trios * 3;
}

/* Decode 4 base64 characters into 3 bytes.
 * Returns 1 on success, 0 if any character is invalid.
 */
static inline int
base64_decode_quad(const unsigned char *in, unsigned char *out,
                   const unsigned char *table)
{
    unsigned char v0 = table[in[0]];
    unsigned char v1 = table[in[1]];
    unsigned char v2 = table[in[2]];
    unsigned char v3 = table[in[3]];

    if ((v0 | v1 | v2 | v3) & 0xc0) {
        return 0;
    }

    out[0] = (v0 << 2) | (v1 >> 4);
    out[1] = (v1 << 4) | (v2 >> 2);
    out[2] = (v2 << 6) | v3;
    return 1;
}

/* Decode multiple complete 4-character groups (no padding allowed).
 * Returns the number of input characters processed.
 * Stops at the first invalid character, padding, or incomplete group.
 */
static inline Py_ssize_t
base64_decode_fast(const unsigned char *in, Py_ssize_t in_len,
                   unsigned char *out, const unsigned char *table)
{
    Py_ssize_t n_quads = in_len / 4;
    Py_ssize_t i;

    for (i = 0; i < n_quads; i++) {
        if (!base64_decode_quad(in + i * 4, out + i * 3, table)) {
            break;
        }
    }

    return i * 4;
}


static const unsigned char table_a2b_base85[] Py_ALIGNED(64) = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,62,-1,63, 64,65,66,-1, 67,68,69,70, -1,71,-1,-1,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,72, 73,74,75,76,
    77,10,11,12, 13,14,15,16, 17,18,19,20, 21,22,23,24,
    25,26,27,28, 29,30,31,32, 33,34,35,-1, -1,-1,78,79,
    80,36,37,38, 39,40,41,42, 43,44,45,46, 47,48,49,50,
    51,52,53,54, 55,56,57,58, 59,60,61,81, 82,83,84,-1,

    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
};

static const unsigned char table_a2b_base85_a85[] Py_ALIGNED(64) = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,26, 27,28,29,30,
    31,32,33,34, 35,36,37,38, 39,40,41,42, 43,44,45,46,
    47,48,49,50, 51,52,53,54, 55,56,57,58, 59,60,61,62,
    63,64,65,66, 67,68,69,70, 71,72,73,74, 75,76,77,78,
    79,80,81,82, 83,84,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,

    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
};

static const unsigned char table_a2b_base85_z85[] Py_ALIGNED(64) = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,68,-1,84, 83,82,72,-1, 75,76,70,65, -1,63,62,69,
     0, 1, 2, 3,  4, 5, 6, 7,  8, 9,64,-1, 73,66,74,71,
    81,36,37,38, 39,40,41,42, 43,44,45,46, 47,48,49,50,
    51,52,53,54, 55,56,57,58, 59,60,61,77, -1,78,67,-1,
    -1,10,11,12, 13,14,15,16, 17,18,19,20, 21,22,23,24,
    25,26,27,28, 29,30,31,32, 33,34,35,79, -1,80,-1,-1,

    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
};

static const unsigned char table_b2a_base85[] Py_ALIGNED(64) =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
    "abcdefghijklmnopqrstuvwxyz!#$%&()*+-;<=>?@^_`{|}~";

static const unsigned char table_b2a_base85_a85[] Py_ALIGNED(64) =
    "!\"#$%&\'()*+,-./0123456789:;<=>?@" \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstu";

static const unsigned char table_b2a_base85_z85[] Py_ALIGNED(64) =
    "0123456789abcdefghijklmnopqrstuvwxyz" \
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/\x2a?&<>()[]{}@%$#"; /* clinic doesn't like '/' followed by '*' */

#define BASE85_A85_PREFIX '<'
#define BASE85_A85_AFFIX '~'
#define BASE85_A85_SUFFIX '>'
#define BASE85_A85_Z 0x00000000
#define BASE85_A85_Y 0x20202020

static const unsigned short crctab_hqx[256] = {
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
module binascii
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=de89fb46bcaf3fec]*/

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
                     "not '%.100s'", Py_TYPE(arg)->tp_name);
        return 0;
    }
    assert(PyBuffer_IsContiguous(buf, 'C'));
    return Py_CLEANUP_SUPPORTED;
}

/* The function inserts '\n' each width characters, moving the data right.
 * It assumes that we allocated enough space for all of the newlines in data.
 * Returns the size of the data including the newlines.
 */
static Py_ssize_t
wraplines(unsigned char *data, Py_ssize_t size, size_t width)
{
    if ((size_t)size <= width) {
        return size;
    }
    unsigned char *src = data + size;
    Py_ssize_t newlines = (size - 1) / width;
    Py_ssize_t line_len = size - newlines * width;
    size += newlines;
    unsigned char *dst = data + size;

    while ((src -= line_len) != data) {
        dst -= line_len;
        memmove(dst, src, line_len);
        *--dst = '\n';
        line_len = width;
    }
    assert(dst == data + width);
    return size;
}

#include "clinic/binascii.c.h"

/*[clinic input]
binascii.a2b_uu

    data: ascii_buffer
    /

Decode a line of uuencoded data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_uu_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=e027f8e0b0598742 input=7cafeaf73df63d1c]*/
{
    const unsigned char *ascii_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    Py_ssize_t ascii_len, bin_len;
    binascii_state *state;

    ascii_data = data->buf;
    ascii_len = data->len;

    assert(ascii_len >= 0);

    /* First byte: binary data length (in bytes) */
    bin_len = (*ascii_data++ - ' ') & 077;
    ascii_len--;

    /* Allocate the buffer */
    PyBytesWriter *writer = PyBytesWriter_Create(bin_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *bin_data = PyBytesWriter_GetData(writer);

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
                state = get_binascii_state(module);
                if (state == NULL) {
                    goto error;
                }
                PyErr_SetString(state->Error, "Illegal char");
                goto error;
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
            state = get_binascii_state(module);
            if (state == NULL) {
                goto error;
            }
            PyErr_SetString(state->Error, "Trailing garbage");
            goto error;
        }
    }
    return PyBytesWriter_Finish(writer);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}

/*[clinic input]
binascii.b2a_uu

    data: Py_buffer
    /
    *
    backtick: bool = False

Uuencode line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_uu_impl(PyObject *module, Py_buffer *data, int backtick)
/*[clinic end generated code: output=b1b99de62d9bbeb8 input=beb27822241095cd]*/
{
    const unsigned char *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    binascii_state *state;
    Py_ssize_t bin_len;

    bin_data = data->buf;
    bin_len = data->len;
    if ( bin_len > 45 ) {
        /* The 45 is a limit that appears in all uuencode's */
        state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_SetString(state->Error, "At most 45 bytes at once");
        return NULL;
    }

    /* We're lazy and allocate to much (fixed up later) */
    Py_ssize_t out_len = 2 + (bin_len + 2) / 3 * 4;
    PyBytesWriter *writer = PyBytesWriter_Create(out_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *ascii_data = PyBytesWriter_GetData(writer);

    /* Store the length */
    if (backtick && !bin_len)
        *ascii_data++ = '`';
    else
        *ascii_data++ = ' ' + (unsigned char)bin_len;

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
            if (backtick && !this_ch)
                *ascii_data++ = '`';
            else
                *ascii_data++ = this_ch + ' ';
        }
    }
    *ascii_data++ = '\n';       /* Append a courtesy newline */

    return PyBytesWriter_FinishWithPointer(writer, ascii_data);
}

typedef unsigned char ignorecache_t[32];

static int
ignorechar(unsigned char c, const Py_buffer *ignorechars,
           ignorecache_t ignorecache)
{
    if (ignorechars == NULL) {
        return 0;
    }
    if (ignorecache[c >> 3] & (1 << (c & 7))) {
        return 1;
    }
    if (memchr(ignorechars->buf, c, ignorechars->len)) {
        ignorecache[c >> 3] |= 1 << (c & 7);
        return 1;
    }
    return 0;
}

/*[clinic input]
binascii.a2b_base64

    data: ascii_buffer
    /
    *
    strict_mode: bool(c_default="-1", py_default="<unrepresentable>") = False
        When set to true, bytes that are not part of the base64 standard are
        not allowed.  The same applies to excess data after padding (= / ==).
        Set to True by default if ignorechars is specified, False otherwise.
    ignorechars: Py_buffer(py_default="<unrepresentable>") = None
        A byte string containing characters to ignore from the input when
        strict_mode is true.

Decode a line of base64 data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_base64_impl(PyObject *module, Py_buffer *data, int strict_mode,
                         Py_buffer *ignorechars)
/*[clinic end generated code: output=eab37aea4cfa6daa input=3be4937d72943835]*/
{
    assert(data->len >= 0);

    const unsigned char *ascii_data = data->buf;
    size_t ascii_len = data->len;
    binascii_state *state = NULL;

    if (strict_mode == -1) {
        strict_mode = (ignorechars->buf != NULL);
    }
    if (!strict_mode || ignorechars->buf == NULL || ignorechars->len == 0) {
        ignorechars = NULL;
    }
    ignorecache_t ignorecache;
    if (ignorechars != NULL) {
        memset(ignorecache, 0, sizeof(ignorecache));
    }

    /* Allocate the buffer */
    Py_ssize_t bin_len = ((ascii_len+3)/4)*3; /* Upper bound, corrected later */
    PyBytesWriter *writer = PyBytesWriter_Create(bin_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *bin_data = PyBytesWriter_GetData(writer);

fastpath:
    /* Fast path: use optimized decoder for complete quads.
     * This works for both strict and non-strict mode for valid input.
     * The fast path stops at padding, invalid chars, or incomplete groups.
     */
    if (ascii_len >= 4) {
        Py_ssize_t fast_chars = base64_decode_fast(ascii_data, (Py_ssize_t)ascii_len,
                                                   bin_data, table_a2b_base64);
        if (fast_chars > 0) {
            ascii_data += fast_chars;
            ascii_len -= fast_chars;
            bin_data += (fast_chars / 4) * 3;
        }
    }

    /* Slow path: handle remaining input (padding, invalid chars, partial groups) */
    int quad_pos = 0;
    unsigned char leftchar = 0;
    int pads = 0;
    for (; ascii_len; ascii_data++, ascii_len--) {
        unsigned char this_ch = *ascii_data;

        /* Check for pad sequences and ignore
        ** the invalid ones.
        */
        if (this_ch == BASE64_PAD) {
            pads++;

            if (strict_mode) {
                if (quad_pos >= 2 && quad_pos + pads <= 4) {
                    continue;
                }
                if (ignorechar(BASE64_PAD, ignorechars, ignorecache)) {
                    continue;
                }
                if (quad_pos == 1) {
                    /* Set an error below. */
                    break;
                }
                state = get_binascii_state(module);
                if (state) {
                    PyErr_SetString(state->Error,
                                    (quad_pos == 0 && ascii_data == data->buf)
                                    ? "Leading padding not allowed"
                                    : "Excess padding not allowed");
                }
                goto error_end;
            }
            else {
                if (quad_pos >= 2 && quad_pos + pads >= 4) {
                    /* A pad sequence means we should not parse more input.
                    ** We've already interpreted the data from the quad at this point.
                    */
                    goto done;
                }
                continue;
            }
        }

        unsigned char v = table_a2b_base64[this_ch];
        if (v >= 64) {
            if (strict_mode && !ignorechar(this_ch, ignorechars, ignorecache)) {
                state = get_binascii_state(module);
                if (state) {
                    PyErr_SetString(state->Error, "Only base64 data is allowed");
                }
                goto error_end;
            }
            continue;
        }

        // Characters that are not '=', in the middle of the padding, are not allowed
        if (pads && strict_mode &&
            !ignorechar(BASE64_PAD, ignorechars, ignorecache))
        {
            state = get_binascii_state(module);
            if (state) {
                PyErr_SetString(state->Error, (quad_pos + pads == 4)
                    ? "Excess data after padding"
                    : "Discontinuous padding not allowed");
            }
            goto error_end;
        }
        pads = 0;

        switch (quad_pos) {
            case 0:
                quad_pos = 1;
                leftchar = v;
                break;
            case 1:
                quad_pos = 2;
                *bin_data++ = (leftchar << 2) | (v >> 4);
                leftchar = v & 0x0f;
                break;
            case 2:
                quad_pos = 3;
                *bin_data++ = (leftchar << 4) | (v >> 2);
                leftchar = v & 0x03;
                break;
            case 3:
                quad_pos = 0;
                *bin_data++ = (leftchar << 6) | (v);
                leftchar = 0;
                ascii_data++;
                ascii_len--;
                goto fastpath;
        }
    }

    if (quad_pos == 1) {
        /* There is exactly one extra valid, non-padding, base64 character.
        ** This is an invalid length, as there is no possible input that
        ** could encoded into such a base64 string.
        */
        state = get_binascii_state(module);
        if (state) {
            unsigned char *bin_data_start = PyBytesWriter_GetData(writer);
            PyErr_Format(state->Error,
                         "Invalid base64-encoded string: "
                         "number of data characters (%zd) cannot be 1 more "
                         "than a multiple of 4",
                         (bin_data - bin_data_start) / 3 * 4 + 1);
        }
        goto error_end;
    }

    if (quad_pos != 0 && quad_pos + pads < 4) {
        state = get_binascii_state(module);
        if (state) {
            PyErr_SetString(state->Error, "Incorrect padding");
        }
        goto error_end;
    }

done:
    return PyBytesWriter_FinishWithPointer(writer, bin_data);

error_end:
    PyBytesWriter_Discard(writer);
    return NULL;
}


/*[clinic input]
binascii.b2a_base64

    data: Py_buffer
    /
    *
    wrapcol: size_t = 0
    newline: bool = True

Base64-code line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_base64_impl(PyObject *module, Py_buffer *data, size_t wrapcol,
                         int newline)
/*[clinic end generated code: output=2edc7311a9515eac input=2ee4214e6d489e2e]*/
{
    const unsigned char *bin_data = data->buf;
    Py_ssize_t bin_len = data->len;
    assert(bin_len >= 0);

    /* Each group of 3 bytes (rounded up) gets encoded as 4 characters,
     * not counting newlines.
     * Note that 'b' gets encoded as 'Yg==' (1 in, 4 out).
     *
     * Use unsigned integer arithmetic to avoid signed integer overflow.
     */
    size_t out_len = ((size_t)bin_len + 2u) / 3u * 4u;
    if (out_len > PY_SSIZE_T_MAX) {
        goto toolong;
    }
    if (wrapcol && out_len) {
        /* Each line should encode a whole number of bytes. */
        wrapcol = wrapcol < 4 ? 4 : wrapcol / 4 * 4;
        out_len += (out_len - 1u) / wrapcol;
        if (out_len > PY_SSIZE_T_MAX) {
            goto toolong;
        }
    }
    if (newline) {
        out_len++;
        if (out_len > PY_SSIZE_T_MAX) {
            goto toolong;
        }
    }
    PyBytesWriter *writer = PyBytesWriter_Create(out_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *ascii_data = PyBytesWriter_GetData(writer);

    /* Use the optimized fast path for complete 3-byte groups */
    Py_ssize_t fast_bytes = base64_encode_fast(bin_data, bin_len, ascii_data,
                                               table_b2a_base64);
    bin_data += fast_bytes;
    ascii_data += (fast_bytes / 3) * 4;
    bin_len -= fast_bytes;

    /* Handle remaining 0-2 bytes */
    if (bin_len == 1) {
        /* 1 byte remaining: produces 2 base64 chars + 2 padding */
        unsigned int val = bin_data[0];
        *ascii_data++ = table_b2a_base64[(val >> 2) & 0x3f];
        *ascii_data++ = table_b2a_base64[(val << 4) & 0x3f];
        *ascii_data++ = BASE64_PAD;
        *ascii_data++ = BASE64_PAD;
    }
    else if (bin_len == 2) {
        /* 2 bytes remaining: produces 3 base64 chars + 1 padding */
        unsigned int val = ((unsigned int)bin_data[0] << 8) | bin_data[1];
        *ascii_data++ = table_b2a_base64[(val >> 10) & 0x3f];
        *ascii_data++ = table_b2a_base64[(val >> 4) & 0x3f];
        *ascii_data++ = table_b2a_base64[(val << 2) & 0x3f];
        *ascii_data++ = BASE64_PAD;
    }

    if (wrapcol) {
        unsigned char *start = PyBytesWriter_GetData(writer);
        ascii_data = start + wraplines(start, ascii_data - start, wrapcol);
    }
    if (newline)
        *ascii_data++ = '\n';       /* Append a courtesy newline */

    return PyBytesWriter_FinishWithPointer(writer, ascii_data);

toolong:;
    binascii_state *state = get_binascii_state(module);
    if (state == NULL) {
        return NULL;
    }
    PyErr_SetString(state->Error, "Too much data for base64");
    return NULL;
}

/*[clinic input]
binascii.a2b_ascii85

    data: ascii_buffer
    /
    *
    foldspaces: bool = False
        Allow 'y' as a short form encoding four spaces.
    adobe: bool = False
        Expect data to be wrapped in '<~' and '~>' as in Adobe Ascii85.
    ignorechars: Py_buffer(c_default="NULL", py_default="b''") = None
        A byte string containing characters to ignore from the input.

Decode Ascii85 data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_ascii85_impl(PyObject *module, Py_buffer *data, int foldspaces,
                          int adobe, Py_buffer *ignorechars)
/*[clinic end generated code: output=599aa3e41095a651 input=20796c9b23cec213]*/
{
    const unsigned char *ascii_data = data->buf;
    Py_ssize_t ascii_len = data->len;
    binascii_state *state = NULL;

    assert(ascii_len >= 0);

    /* Consume Ascii85 prefix and suffix if present. */
    if (adobe) {
        if (ascii_len < 2
            || ascii_data[ascii_len - 2] != BASE85_A85_AFFIX
            || ascii_data[ascii_len - 1] != BASE85_A85_SUFFIX)
        {
            state = get_binascii_state(module);
            if (state != NULL) {
                PyErr_SetString(state->Error,
                                "Ascii85 encoded byte sequences must end with b'~>'");
            }
            return NULL;
        }
        ascii_len -= 2;
        if (ascii_len >= 2
            && ascii_data[0] == BASE85_A85_PREFIX
            && ascii_data[1] == BASE85_A85_AFFIX)
        {
            ascii_data += 2;
            ascii_len -= 2;
        }
    }

    ignorecache_t ignorecache;
    if (ignorechars != NULL) {
        memset(ignorecache, 0, sizeof(ignorecache));
    }

    /* Allocate output buffer. */
    size_t bin_len = ascii_len;
    unsigned char this_ch = 0;
    size_t count_yz = 0;
    for (Py_ssize_t i = 0; i < ascii_len; i++) {
        this_ch = ascii_data[i];
        if (this_ch == 'y' || this_ch == 'z') {
            count_yz++;
        }
    }
    bin_len = (bin_len - count_yz + 4) / 5 * 4;
    if (count_yz > (PY_SSIZE_T_MAX - bin_len) / 4) {
        binascii_state *state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_SetString(state->Error, "Too much Ascii85 data");
        return NULL;
    }
    bin_len += count_yz * 4;

    PyBytesWriter *writer = PyBytesWriter_Create(bin_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *bin_data = PyBytesWriter_GetData(writer);
    if (bin_data == NULL) {
        return NULL;
    }

    uint32_t leftchar = 0;
    int group_pos = 0;
    for (; ascii_len > 0 || group_pos != 0; ascii_len--, ascii_data++) {
        /* Shift (in radix-85) data or padding into our buffer. */
        unsigned char this_digit;
        if (ascii_len > 0) {
            this_ch = *ascii_data;
            this_digit = table_a2b_base85_a85[this_ch];
        }
        else {
            /* Pad with largest radix-85 digit when decoding. */
            this_digit = 84;
        }
        if (this_digit < 85) {
            if (group_pos == 4
                && (leftchar > UINT32_MAX / 85
                    || leftchar * 85 > UINT32_MAX - this_digit))
            {
                state = get_binascii_state(module);
                if (state != NULL) {
                    PyErr_SetString(state->Error, "Ascii85 overflow");
                }
                goto error;
            }
            leftchar = leftchar * 85 + this_digit;
            group_pos++;
        }
        else if ((this_ch == 'y' && foldspaces) || this_ch == 'z') {
            if (group_pos != 0) {
                state = get_binascii_state(module);
                if (state != NULL) {
                    PyErr_Format(state->Error,
                                 "'%c' inside Ascii85 5-tuple", this_ch);
                }
                goto error;
            }
            leftchar = this_ch == 'y' ? BASE85_A85_Y : BASE85_A85_Z;
            group_pos = 5;
        }
        else if (!ignorechar(this_ch, ignorechars, ignorecache)) {
            state = get_binascii_state(module);
            if (state != NULL) {
                PyErr_Format(state->Error,
                             "Non-Ascii85 digit found: %c", this_ch);
            }
            goto error;
        }

        /* Wait until buffer is full. */
        if (group_pos != 5) {
            continue;
        }

        /* Write current chunk. */
        Py_ssize_t chunk_len = ascii_len < 1 ? 3 + ascii_len : 4;
        for (Py_ssize_t i = 0; i < chunk_len; i++) {
            *bin_data++ = (leftchar >> (24 - 8 * i)) & 0xff;
        }

        group_pos = 0;
        leftchar = 0;
    }

    return PyBytesWriter_FinishWithPointer(writer, bin_data);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}

/*[clinic input]
binascii.b2a_ascii85

    data: Py_buffer
    /
    *
    foldspaces: bool = False
        Emit 'y' as a short form encoding four spaces.
    wrapcol: size_t = 0
        Split result into lines of provided width.
    pad: bool = False
        Pad input to a multiple of 4 before encoding.
    adobe: bool = False
        Wrap result in '<~' and '~>' as in Adobe Ascii85.

Ascii85-encode data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_ascii85_impl(PyObject *module, Py_buffer *data, int foldspaces,
                          size_t wrapcol, int pad, int adobe)
/*[clinic end generated code: output=5ce8fdee843073f4 input=791da754508c7d17]*/
{
    const unsigned char *bin_data = data->buf;
    Py_ssize_t bin_len = data->len;

    assert(bin_len >= 0);

    if (adobe && wrapcol == 1) {
        wrapcol = 2;
    }

    /* Allocate output buffer. */
    size_t out_len = ((size_t)bin_len + 3) / 4 * 5;
    if (adobe) {
        out_len += 4;
    }
    if (!pad && (bin_len % 4)) {
        out_len -= 4 - (bin_len % 4);
    }
    if (wrapcol && out_len) {
        out_len += (out_len - 1) / wrapcol;
    }
    if (out_len > PY_SSIZE_T_MAX) {
        binascii_state *state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_SetString(state->Error, "Too much data for Ascii85");
        return NULL;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(out_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *ascii_data = PyBytesWriter_GetData(writer);

    if (adobe) {
        *ascii_data++ = BASE85_A85_PREFIX;
        *ascii_data++ = BASE85_A85_AFFIX;
    }

    /* Encode all full-length chunks. */
    for (; bin_len >= 4; bin_len -= 4, bin_data += 4) {
        uint32_t leftchar = (bin_data[0] << 24) | (bin_data[1] << 16) |
                            (bin_data[2] << 8)  |  bin_data[3];
        if (leftchar == BASE85_A85_Z) {
            *ascii_data++ = 'z';
        }
        else if (foldspaces && leftchar == BASE85_A85_Y) {
            *ascii_data++ = 'y';
        }
        else {
            ascii_data[4] = table_b2a_base85_a85[leftchar % 85];
            leftchar /= 85;
            ascii_data[3] = table_b2a_base85_a85[leftchar % 85];
            leftchar /= 85;
            ascii_data[2] = table_b2a_base85_a85[leftchar % 85];
            leftchar /= 85;
            ascii_data[1] = table_b2a_base85_a85[leftchar % 85];
            leftchar /= 85;
            ascii_data[0] = table_b2a_base85_a85[leftchar];

            ascii_data += 5;
        }
    }

    /* Encode partial-length final chunk. */
    if (bin_len > 0) {
        uint32_t leftchar = 0;
        for (Py_ssize_t i = 0; i < 4; i++) {
            leftchar <<= 8;     /* Pad with zero when encoding. */
            if (i < bin_len) {
                leftchar |= *bin_data++;
            }
        }
        if (pad && leftchar == BASE85_A85_Z) {
            *ascii_data++ = 'z';
        }
        else {
            Py_ssize_t group_len = pad ? 5 : bin_len + 1;
            for (Py_ssize_t i = 4; i >= 0; i--) {
                if (i < group_len) {
                    ascii_data[i] = table_b2a_base85_a85[leftchar % 85];
                }
                leftchar /= 85;
            }
            ascii_data += group_len;
        }
    }

    if (adobe) {
        *ascii_data++ = BASE85_A85_AFFIX;
        *ascii_data++ = BASE85_A85_SUFFIX;
    }

    if (wrapcol && out_len) {
        unsigned char *start = PyBytesWriter_GetData(writer);
        ascii_data = start + wraplines(start, ascii_data - start, wrapcol);
        if (adobe && ascii_data[-2] == '\n') {
            assert(ascii_data[-1] == BASE85_A85_SUFFIX);
            assert(ascii_data[-3] == BASE85_A85_AFFIX);
            ascii_data[-3] = '\n';
            ascii_data[-2] = BASE85_A85_AFFIX;
        }
    }

    return PyBytesWriter_FinishWithPointer(writer, ascii_data);
}

static PyObject *
base85_decode_impl(PyObject *module, Py_buffer *data,
                   const unsigned char table_a2b[], const char *name)
{
    const unsigned char *ascii_data = data->buf;
    Py_ssize_t ascii_len = data->len;
    binascii_state *state = NULL;

    assert(ascii_len >= 0);

    /* Allocate output buffer. */
    size_t bin_len = ((size_t)ascii_len + 4) / 5 * 4;
    PyBytesWriter *writer = PyBytesWriter_Create(bin_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *bin_data = PyBytesWriter_GetData(writer);

    uint32_t leftchar = 0;
    int group_pos = 0;
    for (; ascii_len > 0 || group_pos != 0; ascii_len--, ascii_data++) {
        /* Shift (in radix-85) data or padding into our buffer. */
        unsigned char this_digit;
        if (ascii_len > 0) {
            unsigned char this_ch = *ascii_data;
            this_digit = table_a2b[this_ch];
        }
        else {
            /* Pad with largest radix-85 digit when decoding. */
            this_digit = 84;
        }
        if (this_digit < 85) {
            if (group_pos == 4
                && (leftchar > UINT32_MAX / 85
                    || leftchar * 85 > UINT32_MAX - this_digit))
            {
                state = get_binascii_state(module);
                if (state != NULL) {
                    PyErr_Format(state->Error,
                                 "%s overflow in hunk starting at byte %d",
                                 name, (data->len - ascii_len) / 5 * 5);
                }
                goto error;
            }
            leftchar = leftchar * 85 + this_digit;
            group_pos++;
        }
        else {
            state = get_binascii_state(module);
            if (state != NULL) {
                PyErr_Format(state->Error, "bad %s character at position %d",
                             name, data->len - ascii_len);
            }
            goto error;
        }

        /* Wait until buffer is full. */
        if (group_pos != 5) {
            continue;
        }

        /* Write current chunk. */
        Py_ssize_t chunk_len = ascii_len < 1 ? 3 + ascii_len : 4;
        for (Py_ssize_t i = 0; i < chunk_len; i++) {
            *bin_data++ = (leftchar >> (24 - 8 * i)) & 0xff;
        }

        group_pos = 0;
        leftchar = 0;
    }

    return PyBytesWriter_FinishWithPointer(writer, bin_data);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}

static PyObject *
base85_encode_impl(PyObject *module, Py_buffer *data, int pad,
                   const unsigned char table_b2a[], const char *name)
{
    const unsigned char *bin_data = data->buf;
    Py_ssize_t bin_len = data->len;

    assert(bin_len >= 0);

    /* Allocate output buffer. */
    size_t out_len = ((size_t)bin_len + 3) / 4 * 5;
    if (!pad && (bin_len % 4)) {
        out_len -= 4 - (bin_len % 4);
    }
    if (out_len > PY_SSIZE_T_MAX) {
        binascii_state *state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_Format(state->Error, "Too much data for %s", name);
        return NULL;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(out_len);
    if (writer == NULL) {
        return NULL;
    }
    unsigned char *ascii_data = PyBytesWriter_GetData(writer);

    /* Encode all full-length chunks. */
    for (; bin_len >= 4; bin_len -= 4, bin_data += 4) {
        uint32_t leftchar = (bin_data[0] << 24) | (bin_data[1] << 16) |
                            (bin_data[2] << 8)  |  bin_data[3];

        ascii_data[4] = table_b2a[leftchar % 85];
        leftchar /= 85;
        ascii_data[3] = table_b2a[leftchar % 85];
        leftchar /= 85;
        ascii_data[2] = table_b2a[leftchar % 85];
        leftchar /= 85;
        ascii_data[1] = table_b2a[leftchar % 85];
        leftchar /= 85;
        ascii_data[0] = table_b2a[leftchar];

        ascii_data += 5;
    }

    /* Encode partial-length final chunk. */
    if (bin_len > 0) {
        uint32_t leftchar = 0;
        for (Py_ssize_t i = 0; i < 4; i++) {
            leftchar <<= 8;     /* Pad with zero when encoding. */
            if (i < bin_len) {
                leftchar |= *bin_data++;
            }
        }
        Py_ssize_t group_len = pad ? 5 : bin_len + 1;
        for (Py_ssize_t i = 4; i >= 0; i--) {
            if (i < group_len) {
                ascii_data[i] = table_b2a[leftchar % 85];
            }
            leftchar /= 85;
        }
        ascii_data += group_len;
    }

    return PyBytesWriter_FinishWithPointer(writer, ascii_data);
}

/*[clinic input]
binascii.a2b_base85

    data: ascii_buffer
    /

Decode a line of Base85 data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_base85_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=c2db6ab9181b0089 input=06c9d595352b5a2b]*/
{
    return base85_decode_impl(module, data, table_a2b_base85, "Base85");
}

/*[clinic input]
binascii.b2a_base85

    data: Py_buffer
    /
    *
    pad: bool = False
        Pad input to a multiple of 4 before encoding.

Base85-code line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_base85_impl(PyObject *module, Py_buffer *data, int pad)
/*[clinic end generated code: output=b317adb36a57740d input=89fde81b96dcec06]*/
{
    return base85_encode_impl(module, data, pad, table_b2a_base85, "Base85");
}

/*[clinic input]
binascii.a2b_z85

    data: ascii_buffer
    /

Decode a line of Z85 data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_z85_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=57d8260bb5267a98 input=c54baff4d81510a4]*/
{
    return base85_decode_impl(module, data, table_a2b_base85_z85, "Z85");
}

/*[clinic input]
binascii.b2a_z85

    data: Py_buffer
    /
    *
    pad: bool = False
        Pad input to a multiple of 4 before encoding.

Z85-code line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_z85_impl(PyObject *module, Py_buffer *data, int pad)
/*[clinic end generated code: output=88284835e332c9cf input=51d070a5a6cf82d8]*/
{
    return base85_encode_impl(module, data, pad, table_b2a_base85_z85, "Z85");
}

/*[clinic input]
binascii.crc_hqx

    data: Py_buffer
    crc: unsigned_int(bitwise=True)
    /

Compute CRC-CCITT incrementally.
[clinic start generated code]*/

static PyObject *
binascii_crc_hqx_impl(PyObject *module, Py_buffer *data, unsigned int crc)
/*[clinic end generated code: output=2fde213d0f547a98 input=56237755370a951c]*/
{
    const unsigned char *bin_data;
    Py_ssize_t len;

    crc &= 0xffff;
    bin_data = data->buf;
    len = data->len;

    while(len-- > 0) {
        crc = ((crc<<8)&0xff00) ^ crctab_hqx[(crc>>8)^*bin_data++];
    }

    return PyLong_FromUnsignedLong(crc);
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

static const unsigned int crc_32_tab[256] = {
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

static unsigned int
internal_crc32(const unsigned char *bin_data, Py_ssize_t len, unsigned int crc)
{ /* By Jim Ahlstrom; All rights transferred to CNRI */
    unsigned int result;

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
binascii.crc32 -> unsigned_int

    data: Py_buffer
    crc: unsigned_int(bitwise=True) = 0
    /

Compute CRC-32 incrementally.
[clinic start generated code]*/

static unsigned int
binascii_crc32_impl(PyObject *module, Py_buffer *data, unsigned int crc)
/*[clinic end generated code: output=52cf59056a78593b input=bbe340bc99d25aa8]*/

#ifdef USE_ZLIB_CRC32
/* This is the same as zlibmodule.c zlib_crc32_impl. It exists in two
 * modules for historical reasons. */
{
    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (data->len > 1024*5) {
        unsigned char *buf = data->buf;
        Py_ssize_t len = data->len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. crc32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t.
           We further limit size due to bugs in Apple's macOS zlib.
           See https://github.com/python/cpython/issues/105967
         */
#define ZLIB_CRC_CHUNK_SIZE 0x40000000
#if ZLIB_CRC_CHUNK_SIZE > INT_MAX
# error "unsupported less than 32-bit platform?"
#endif
        while ((size_t)len > ZLIB_CRC_CHUNK_SIZE) {
            crc = crc32(crc, buf, ZLIB_CRC_CHUNK_SIZE);
            buf += (size_t) ZLIB_CRC_CHUNK_SIZE;
            len -= (size_t) ZLIB_CRC_CHUNK_SIZE;
        }
#undef ZLIB_CRC_CHUNK_SIZE
        crc = crc32(crc, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        crc = crc32(crc, data->buf, (unsigned int)data->len);
    }
    return crc & 0xffffffff;
}
#else  /* USE_ZLIB_CRC32 */
{
    const unsigned char *bin_data = data->buf;
    Py_ssize_t len = data->len;

    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (len > 1024*5) {
        unsigned int result;
        Py_BEGIN_ALLOW_THREADS
        result = internal_crc32(bin_data, len, crc);
        Py_END_ALLOW_THREADS
        return result;
    } else {
        return internal_crc32(bin_data, len, crc);
    }
}
#endif  /* USE_ZLIB_CRC32 */

/*[clinic input]
binascii.b2a_hex

    data: Py_buffer
    sep: object = NULL
        An optional single character or byte to separate hex bytes.
    bytes_per_sep: int = 1
        How many bytes between separators.  Positive values count from the
        right, negative values count from the left.

Hexadecimal representation of binary data.

The return value is a bytes object.  This function is also
available as "hexlify()".

Example:
>>> binascii.b2a_hex(b'\xb9\x01\xef')
b'b901ef'
>>> binascii.hexlify(b'\xb9\x01\xef', ':')
b'b9:01:ef'
>>> binascii.b2a_hex(b'\xb9\x01\xef', b'_', 2)
b'b9_01ef'
[clinic start generated code]*/

static PyObject *
binascii_b2a_hex_impl(PyObject *module, Py_buffer *data, PyObject *sep,
                      int bytes_per_sep)
/*[clinic end generated code: output=a26937946a81d2c7 input=ec0ade6ba2e43543]*/
{
    return _Py_strhex_bytes_with_sep((const char *)data->buf, data->len,
                                     sep, bytes_per_sep);
}

/*[clinic input]
binascii.hexlify = binascii.b2a_hex

Hexadecimal representation of binary data.

The return value is a bytes object.  This function is also
available as "b2a_hex()".
[clinic start generated code]*/

static PyObject *
binascii_hexlify_impl(PyObject *module, Py_buffer *data, PyObject *sep,
                      int bytes_per_sep)
/*[clinic end generated code: output=d12aa1b001b15199 input=bc317bd4e241f76b]*/
{
    return _Py_strhex_bytes_with_sep((const char *)data->buf, data->len,
                                     sep, bytes_per_sep);
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
binascii_a2b_hex_impl(PyObject *module, Py_buffer *hexstr)
/*[clinic end generated code: output=0cc1a139af0eeecb input=9e1e7f2f94db24fd]*/
{
    const char* argbuf;
    Py_ssize_t arglen;
    Py_ssize_t i, j;
    binascii_state *state;

    argbuf = hexstr->buf;
    arglen = hexstr->len;

    assert(arglen >= 0);

    /* XXX What should we do about strings with an odd length?  Should
     * we add an implicit leading zero, or a trailing zero?  For now,
     * raise an exception.
     */
    if (arglen % 2) {
        state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_SetString(state->Error, "Odd-length string");
        return NULL;
    }

    PyBytesWriter *writer = PyBytesWriter_Create(arglen/2);
    if (writer == NULL) {
        return NULL;
    }
    char *retbuf = PyBytesWriter_GetData(writer);

    for (i=j=0; i < arglen; i += 2) {
        unsigned int top = _PyLong_DigitValue[Py_CHARMASK(argbuf[i])];
        unsigned int bot = _PyLong_DigitValue[Py_CHARMASK(argbuf[i+1])];
        if (top >= 16 || bot >= 16) {
            state = get_binascii_state(module);
            if (state == NULL) {
                goto error;
            }
            PyErr_SetString(state->Error,
                            "Non-hexadecimal digit found");
            goto error;
        }
        retbuf[j++] = (top << 4) + bot;
    }
    return PyBytesWriter_Finish(writer);

error:
    PyBytesWriter_Discard(writer);
    return NULL;
}

/*[clinic input]
binascii.unhexlify = binascii.a2b_hex

Binary data of hexadecimal representation.

hexstr must contain an even number of hex digits (upper or lower case).
[clinic start generated code]*/

static PyObject *
binascii_unhexlify_impl(PyObject *module, Py_buffer *hexstr)
/*[clinic end generated code: output=51a64c06c79629e3 input=dd8c012725f462da]*/
{
    return binascii_a2b_hex_impl(module, hexstr);
}

#define MAXLINESIZE 76


/*[clinic input]
binascii.a2b_qp

    data: ascii_buffer
    header: bool = False

Decode a string of qp-encoded data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_qp_impl(PyObject *module, Py_buffer *data, int header)
/*[clinic end generated code: output=e99f7846cfb9bc53 input=bdfb31598d4e47b9]*/
{
    Py_ssize_t in, out;
    char ch;
    const unsigned char *ascii_data;
    unsigned char *odata;
    Py_ssize_t datalen = 0;
    PyObject *rv;

    ascii_data = data->buf;
    datalen = data->len;

    /* We allocate the output same size as input, this is overkill.
     */
    odata = (unsigned char *) PyMem_Calloc(1, datalen);
    if (odata == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

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
            else if ((in + 1 < datalen) &&
                     ((ascii_data[in] >= 'A' && ascii_data[in] <= 'F') ||
                      (ascii_data[in] >= 'a' && ascii_data[in] <= 'f') ||
                      (ascii_data[in] >= '0' && ascii_data[in] <= '9')) &&
                     ((ascii_data[in+1] >= 'A' && ascii_data[in+1] <= 'F') ||
                      (ascii_data[in+1] >= 'a' && ascii_data[in+1] <= 'f') ||
                      (ascii_data[in+1] >= '0' && ascii_data[in+1] <= '9'))) {
                /* hexval */
                ch = _PyLong_DigitValue[ascii_data[in]] << 4;
                in++;
                ch |= _PyLong_DigitValue[ascii_data[in]];
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
    rv = PyBytes_FromStringAndSize((char *)odata, out);
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
    quotetabs: bool = False
    istext: bool = True
    header: bool = False

Encode a string using quoted-printable encoding.

On encoding, when istext is set, newlines are not encoded, and white
space at end of lines is.  When istext is not set, \r and \n (CR/LF)
are both encoded.  When quotetabs is set, space and tabs are encoded.
[clinic start generated code]*/

static PyObject *
binascii_b2a_qp_impl(PyObject *module, Py_buffer *data, int quotetabs,
                     int istext, int header)
/*[clinic end generated code: output=e9884472ebb1a94c input=e9102879afb0defd]*/
{
    Py_ssize_t in, out;
    const unsigned char *databuf;
    unsigned char *odata;
    Py_ssize_t datalen = 0, odatalen = 0;
    PyObject *rv;
    unsigned int linelen = 0;
    unsigned char ch;
    int crlf = 0;
    const unsigned char *p;

    databuf = data->buf;
    datalen = data->len;

    /* See if this string is using CRLF line ends */
    /* XXX: this function has the side effect of converting all of
     * the end of lines to be the same depending on this detection
     * here */
    p = (const unsigned char *) memchr(databuf, '\n', datalen);
    if ((p != NULL) && (p > databuf) && (*(p-1) == '\r'))
        crlf = 1;

    /* First, scan to see how many characters need to be encoded */
    in = 0;
    while (in < datalen) {
        Py_ssize_t delta = 0;
        if ((databuf[in] > 126) ||
            (databuf[in] == '=') ||
            (header && databuf[in] == '_') ||
            ((databuf[in] == '.') && (linelen == 0) &&
             (in + 1 == datalen || databuf[in+1] == '\n' ||
              databuf[in+1] == '\r' || databuf[in+1] == 0)) ||
            (!istext && ((databuf[in] == '\r') || (databuf[in] == '\n'))) ||
            ((databuf[in] == '\t' || databuf[in] == ' ') && (in + 1 == datalen)) ||
            ((databuf[in] < 33) &&
             (databuf[in] != '\r') && (databuf[in] != '\n') &&
             (quotetabs || ((databuf[in] != '\t') && (databuf[in] != ' ')))))
        {
            if ((linelen + 3) >= MAXLINESIZE) {
                linelen = 0;
                if (crlf)
                    delta += 3;
                else
                    delta += 2;
            }
            linelen += 3;
            delta += 3;
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
                    delta += 2;
                if (crlf)
                    delta += 2;
                else
                    delta += 1;
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
                        delta += 3;
                    else
                        delta += 2;
                }
                linelen++;
                delta++;
                in++;
            }
        }
        if (PY_SSIZE_T_MAX - delta < odatalen) {
            PyErr_NoMemory();
            return NULL;
        }
        odatalen += delta;
    }

    /* We allocate the output same size as input, this is overkill.
     */
    odata = (unsigned char *) PyMem_Calloc(1, odatalen);
    if (odata == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    in = out = linelen = 0;
    while (in < datalen) {
        if ((databuf[in] > 126) ||
            (databuf[in] == '=') ||
            (header && databuf[in] == '_') ||
            ((databuf[in] == '.') && (linelen == 0) &&
             (in + 1 == datalen || databuf[in+1] == '\n' ||
              databuf[in+1] == '\r' || databuf[in+1] == 0)) ||
            (!istext && ((databuf[in] == '\r') || (databuf[in] == '\n'))) ||
            ((databuf[in] == '\t' || databuf[in] == ' ') && (in + 1 == datalen)) ||
            ((databuf[in] < 33) &&
             (databuf[in] != '\r') && (databuf[in] != '\n') &&
             (quotetabs || ((databuf[in] != '\t') && (databuf[in] != ' ')))))
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
    rv = PyBytes_FromStringAndSize((char *)odata, out);
    PyMem_Free(odata);
    return rv;
}

/* List of functions defined in the module */

static struct PyMethodDef binascii_module_methods[] = {
    BINASCII_A2B_UU_METHODDEF
    BINASCII_B2A_UU_METHODDEF
    BINASCII_A2B_BASE64_METHODDEF
    BINASCII_B2A_BASE64_METHODDEF
    BINASCII_B2A_ASCII85_METHODDEF
    BINASCII_A2B_ASCII85_METHODDEF
    BINASCII_A2B_BASE85_METHODDEF
    BINASCII_B2A_BASE85_METHODDEF
    BINASCII_A2B_Z85_METHODDEF
    BINASCII_B2A_Z85_METHODDEF
    BINASCII_A2B_HEX_METHODDEF
    BINASCII_B2A_HEX_METHODDEF
    BINASCII_HEXLIFY_METHODDEF
    BINASCII_UNHEXLIFY_METHODDEF
    BINASCII_CRC_HQX_METHODDEF
    BINASCII_CRC32_METHODDEF
    BINASCII_A2B_QP_METHODDEF
    BINASCII_B2A_QP_METHODDEF
    {NULL, NULL}                             /* sentinel */
};


/* Initialization function for the module (*must* be called PyInit_binascii) */
PyDoc_STRVAR(doc_binascii, "Conversion between binary data and ASCII");

static int
binascii_exec(PyObject *module)
{
    binascii_state *state = PyModule_GetState(module);
    if (state == NULL) {
        return -1;
    }

    state->Error = PyErr_NewException("binascii.Error", PyExc_ValueError, NULL);
    if (PyModule_AddObjectRef(module, "Error", state->Error) < 0) {
        return -1;
    }

    state->Incomplete = PyErr_NewException("binascii.Incomplete", NULL, NULL);
    if (PyModule_AddObjectRef(module, "Incomplete", state->Incomplete) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot binascii_slots[] = {
    {Py_mod_exec, binascii_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
    {0, NULL}
};

static int
binascii_traverse(PyObject *module, visitproc visit, void *arg)
{
    binascii_state *state = get_binascii_state(module);
    Py_VISIT(state->Error);
    Py_VISIT(state->Incomplete);
    return 0;
}

static int
binascii_clear(PyObject *module)
{
    binascii_state *state = get_binascii_state(module);
    Py_CLEAR(state->Error);
    Py_CLEAR(state->Incomplete);
    return 0;
}

static void
binascii_free(void *module)
{
    binascii_clear((PyObject *)module);
}

static struct PyModuleDef binasciimodule = {
    PyModuleDef_HEAD_INIT,
    "binascii",
    doc_binascii,
    sizeof(binascii_state),
    binascii_module_methods,
    binascii_slots,
    binascii_traverse,
    binascii_clear,
    binascii_free
};

PyMODINIT_FUNC
PyInit_binascii(void)
{
    return PyModuleDef_Init(&binasciimodule);
}
