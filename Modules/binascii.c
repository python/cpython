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

#define PY_SSIZE_T_CLEAN

#include "Python.h"
#include "pycore_long.h"          // _PyLong_DigitValue
#include "pycore_strhex.h"        // _Py_strhex_bytes_with_sep()
#include "zlib.h"                 // crc32()

typedef struct binascii_state {
    PyObject *Error;
    PyObject *Incomplete;
} binascii_state;

static inline binascii_state *
get_binascii_state(PyObject *module)
{
    return (binascii_state *)PyModule_GetState(module);
}


static const unsigned char table_a2b_base64[] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1, 0,-1,-1, /* Note PAD->0 */
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

static const unsigned char table_b2a_base64[] =
"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


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
                     "not '%.100s'", Py_TYPE(arg)->tp_name);
        return 0;
    }
    if (!PyBuffer_IsContiguous(buf, 'C')) {
        PyErr_Format(PyExc_TypeError,
                     "argument should be a contiguous buffer, "
                     "not '%.100s'", Py_TYPE(arg)->tp_name);
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
binascii_a2b_uu_impl(PyObject *module, Py_buffer *data)
/*[clinic end generated code: output=e027f8e0b0598742 input=7cafeaf73df63d1c]*/
{
    const unsigned char *ascii_data;
    unsigned char *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    PyObject *rv;
    Py_ssize_t ascii_len, bin_len;
    binascii_state *state;

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
                state = get_binascii_state(module);
                if (state == NULL) {
                    return NULL;
                }
                PyErr_SetString(state->Error, "Illegal char");
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
            state = get_binascii_state(module);
            if (state == NULL) {
                return NULL;
            }
            PyErr_SetString(state->Error, "Trailing garbage");
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
    *
    backtick: bool(accept={int}) = False

Uuencode line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_uu_impl(PyObject *module, Py_buffer *data, int backtick)
/*[clinic end generated code: output=b1b99de62d9bbeb8 input=b26bc8d32b6ed2f6]*/
{
    unsigned char *ascii_data;
    const unsigned char *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    binascii_state *state;
    Py_ssize_t bin_len, out_len;
    _PyBytesWriter writer;

    _PyBytesWriter_Init(&writer);
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
    out_len = 2 + (bin_len + 2) / 3 * 4;
    ascii_data = _PyBytesWriter_Alloc(&writer, out_len);
    if (ascii_data == NULL)
        return NULL;

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

    return _PyBytesWriter_Finish(&writer, ascii_data);
}

/*[clinic input]
binascii.a2b_base64

    data: ascii_buffer
    /
    *
    strict_mode: bool(accept={int}) = False

Decode a line of base64 data.

  strict_mode
    When set to True, bytes that are not part of the base64 standard are not allowed.
    The same applies to excess data after padding (= / ==).
[clinic start generated code]*/

static PyObject *
binascii_a2b_base64_impl(PyObject *module, Py_buffer *data, int strict_mode)
/*[clinic end generated code: output=5409557788d4f975 input=3a30c4e3528317c6]*/
{
    assert(data->len >= 0);

    const unsigned char *ascii_data = data->buf;
    size_t ascii_len = data->len;
    binascii_state *state = NULL;
    char padding_started = 0;

    /* Allocate the buffer */
    Py_ssize_t bin_len = ((ascii_len+3)/4)*3; /* Upper bound, corrected later */
    _PyBytesWriter writer;
    _PyBytesWriter_Init(&writer);
    unsigned char *bin_data = _PyBytesWriter_Alloc(&writer, bin_len);
    if (bin_data == NULL)
        return NULL;
    unsigned char *bin_data_start = bin_data;

    if (strict_mode && ascii_len > 0 && ascii_data[0] == '=') {
        state = get_binascii_state(module);
        if (state) {
            PyErr_SetString(state->Error, "Leading padding not allowed");
        }
        goto error_end;
    }

    int quad_pos = 0;
    unsigned char leftchar = 0;
    int pads = 0;
    for (size_t i = 0; i < ascii_len; i++) {
        unsigned char this_ch = ascii_data[i];

        /* Check for pad sequences and ignore
        ** the invalid ones.
        */
        if (this_ch == BASE64_PAD) {
            padding_started = 1;

            if (quad_pos >= 2 && quad_pos + ++pads >= 4) {
                /* A pad sequence means we should not parse more input.
                ** We've already interpreted the data from the quad at this point.
                ** in strict mode, an error should raise if there's excess data after the padding.
                */
                if (strict_mode && i + 1 < ascii_len) {
                    state = get_binascii_state(module);
                    if (state) {
                        PyErr_SetString(state->Error, "Excess data after padding");
                    }
                    goto error_end;
                }

                goto done;
            }
            continue;
        }

        this_ch = table_a2b_base64[this_ch];
        if (this_ch >= 64) {
            if (strict_mode) {
                state = get_binascii_state(module);
                if (state) {
                    PyErr_SetString(state->Error, "Only base64 data is allowed");
                }
                goto error_end;
            }
            continue;
        }

        // Characters that are not '=', in the middle of the padding, are not allowed
        if (strict_mode && padding_started) {
            state = get_binascii_state(module);
            if (state) {
                PyErr_SetString(state->Error, "Discontinuous padding not allowed");
            }
            goto error_end;
        }
        pads = 0;

        switch (quad_pos) {
            case 0:
                quad_pos = 1;
                leftchar = this_ch;
                break;
            case 1:
                quad_pos = 2;
                *bin_data++ = (leftchar << 2) | (this_ch >> 4);
                leftchar = this_ch & 0x0f;
                break;
            case 2:
                quad_pos = 3;
                *bin_data++ = (leftchar << 4) | (this_ch >> 2);
                leftchar = this_ch & 0x03;
                break;
            case 3:
                quad_pos = 0;
                *bin_data++ = (leftchar << 6) | (this_ch);
                leftchar = 0;
                break;
        }
    }

    if (quad_pos != 0) {
        state = get_binascii_state(module);
        if (state == NULL) {
            /* error already set, from get_binascii_state */
        } else if (quad_pos == 1) {
            /*
            ** There is exactly one extra valid, non-padding, base64 character.
            ** This is an invalid length, as there is no possible input that
            ** could encoded into such a base64 string.
            */
            PyErr_Format(state->Error,
                         "Invalid base64-encoded string: "
                         "number of data characters (%zd) cannot be 1 more "
                         "than a multiple of 4",
                         (bin_data - bin_data_start) / 3 * 4 + 1);
        } else {
            PyErr_SetString(state->Error, "Incorrect padding");
        }
        error_end:
        _PyBytesWriter_Dealloc(&writer);
        return NULL;
    }

done:
    return _PyBytesWriter_Finish(&writer, bin_data);
}


/*[clinic input]
binascii.b2a_base64

    data: Py_buffer
    /
    *
    newline: bool(accept={int}) = True

Base64-code line of data.
[clinic start generated code]*/

static PyObject *
binascii_b2a_base64_impl(PyObject *module, Py_buffer *data, int newline)
/*[clinic end generated code: output=4ad62c8e8485d3b3 input=6083dac5777fa45d]*/
{
    unsigned char *ascii_data;
    const unsigned char *bin_data;
    int leftbits = 0;
    unsigned char this_ch;
    unsigned int leftchar = 0;
    Py_ssize_t bin_len, out_len;
    _PyBytesWriter writer;
    binascii_state *state;

    bin_data = data->buf;
    bin_len = data->len;
    _PyBytesWriter_Init(&writer);

    assert(bin_len >= 0);

    if ( bin_len > BASE64_MAXBIN ) {
        state = get_binascii_state(module);
        if (state == NULL) {
            return NULL;
        }
        PyErr_SetString(state->Error, "Too much data for base64 line");
        return NULL;
    }

    /* We're lazy and allocate too much (fixed up later).
       "+2" leaves room for up to two pad characters.
       Note that 'b' gets encoded as 'Yg==\n' (1 in, 5 out). */
    out_len = bin_len*2 + 2;
    if (newline)
        out_len++;
    ascii_data = _PyBytesWriter_Alloc(&writer, out_len);
    if (ascii_data == NULL)
        return NULL;

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
    if (newline)
        *ascii_data++ = '\n';       /* Append a courtesy newline */

    return _PyBytesWriter_Finish(&writer, ascii_data);
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
/* This is the same as zlibmodule.c zlib_crc32_impl. It exists in two
 * modules for historical reasons. We should consolidate the two. */
{
    /* Releasing the GIL for very small buffers is inefficient
       and may lower performance */
    if (data->len > 1024*5) {
        unsigned char *buf = data->buf;
        Py_ssize_t len = data->len;

        Py_BEGIN_ALLOW_THREADS
        /* Avoid truncation of length for very large buffers. crc32() takes
           length as an unsigned int, which may be narrower than Py_ssize_t. */
        while ((size_t)len > UINT_MAX) {
            crc = crc32(crc, buf, UINT_MAX);
            buf += (size_t) UINT_MAX;
            len -= (size_t) UINT_MAX;
        }
        crc = crc32(crc, buf, (unsigned int)len);
        Py_END_ALLOW_THREADS
    } else {
        crc = crc32(crc, data->buf, (unsigned int)data->len);
    }
    return crc & 0xffffffff;
}

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
    PyObject *retval;
    char* retbuf;
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

    retval = PyBytes_FromStringAndSize(NULL, (arglen/2));
    if (!retval)
        return NULL;
    retbuf = PyBytes_AS_STRING(retval);

    for (i=j=0; i < arglen; i += 2) {
        unsigned int top = _PyLong_DigitValue[Py_CHARMASK(argbuf[i])];
        unsigned int bot = _PyLong_DigitValue[Py_CHARMASK(argbuf[i+1])];
        if (top >= 16 || bot >= 16) {
            state = get_binascii_state(module);
            if (state == NULL) {
                return NULL;
            }
            PyErr_SetString(state->Error,
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
binascii_unhexlify_impl(PyObject *module, Py_buffer *hexstr)
/*[clinic end generated code: output=51a64c06c79629e3 input=dd8c012725f462da]*/
{
    return binascii_a2b_hex_impl(module, hexstr);
}

#define MAXLINESIZE 76


/*[clinic input]
binascii.a2b_qp

    data: ascii_buffer
    header: bool(accept={int}) = False

Decode a string of qp-encoded data.
[clinic start generated code]*/

static PyObject *
binascii_a2b_qp_impl(PyObject *module, Py_buffer *data, int header)
/*[clinic end generated code: output=e99f7846cfb9bc53 input=bf6766fea76cce8f]*/
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
    quotetabs: bool(accept={int}) = False
    istext: bool(accept={int}) = True
    header: bool(accept={int}) = False

Encode a string using quoted-printable encoding.

On encoding, when istext is set, newlines are not encoded, and white
space at end of lines is.  When istext is not set, \r and \n (CR/LF)
are both encoded.  When quotetabs is set, space and tabs are encoded.
[clinic start generated code]*/

static PyObject *
binascii_b2a_qp_impl(PyObject *module, Py_buffer *data, int quotetabs,
                     int istext, int header)
/*[clinic end generated code: output=e9884472ebb1a94c input=21fb7eea4a184ba6]*/
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
binascii_exec(PyObject *module) {
    int result;
    binascii_state *state = PyModule_GetState(module);
    if (state == NULL) {
        return -1;
    }

    state->Error = PyErr_NewException("binascii.Error", PyExc_ValueError, NULL);
    if (state->Error == NULL) {
        return -1;
    }
    Py_INCREF(state->Error);
    result = PyModule_AddObject(module, "Error", state->Error);
    if (result == -1) {
        Py_DECREF(state->Error);
        return -1;
    }

    state->Incomplete = PyErr_NewException("binascii.Incomplete", NULL, NULL);
    if (state->Incomplete == NULL) {
        return -1;
    }
    Py_INCREF(state->Incomplete);
    result = PyModule_AddObject(module, "Incomplete", state->Incomplete);
    if (result == -1) {
        Py_DECREF(state->Incomplete);
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot binascii_slots[] = {
    {Py_mod_exec, binascii_exec},
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
