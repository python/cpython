/* ------------------------------------------------------------------------

   _codecs -- Provides access to the codec registry and the builtin
              codecs.

   This module should never be imported directly. The standard library
   module "codecs" wraps this builtin module for use within Python.

   The codec registry is accessible via:

     register(search_function) -> None

     lookup(encoding) -> CodecInfo object

   The builtin Unicode codecs use the following interface:

     <encoding>_encode(Unicode_object[,errors='strict']) ->
        (string object, bytes consumed)

     <encoding>_decode(char_buffer_obj[,errors='strict']) ->
        (Unicode object, bytes consumed)

   <encoding>_encode() interfaces also accept non-Unicode object as
   input. The objects are then converted to Unicode using
   PyUnicode_FromObject() prior to applying the conversion.

   These <encoding>s are available: utf_8, unicode_escape,
   raw_unicode_escape, unicode_internal, latin_1, ascii (7-bit),
   mbcs (on win32).


Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#define PY_SSIZE_T_CLEAN
#include "Python.h"

#ifdef MS_WINDOWS
#include <windows.h>
#endif

/* --- Registry ----------------------------------------------------------- */

PyDoc_STRVAR(register__doc__,
"register(search_function)\n\
\n\
Register a codec search function. Search functions are expected to take\n\
one argument, the encoding name in all lower case letters, and return\n\
a tuple of functions (encoder, decoder, stream_reader, stream_writer)\n\
(or a CodecInfo object).");

static
PyObject *codec_register(PyObject *self, PyObject *search_function)
{
    if (PyCodec_Register(search_function))
        return NULL;

    Py_RETURN_NONE;
}

PyDoc_STRVAR(lookup__doc__,
"lookup(encoding) -> CodecInfo\n\
\n\
Looks up a codec tuple in the Python codec registry and returns\n\
a CodecInfo object.");

static
PyObject *codec_lookup(PyObject *self, PyObject *args)
{
    char *encoding;

    if (!PyArg_ParseTuple(args, "s:lookup", &encoding))
        return NULL;

    return _PyCodec_Lookup(encoding);
}

PyDoc_STRVAR(encode__doc__,
"encode(obj, [encoding[,errors]]) -> object\n\
\n\
Encodes obj using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore', 'replace' and\n\
'xmlcharrefreplace' as well as any other name registered with\n\
codecs.register_error that can handle ValueErrors.");

static PyObject *
codec_encode(PyObject *self, PyObject *args)
{
    const char *encoding = NULL;
    const char *errors = NULL;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O|ss:encode", &v, &encoding, &errors))
        return NULL;

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    return PyCodec_Encode(v, encoding, errors);
}

PyDoc_STRVAR(decode__doc__,
"decode(obj, [encoding[,errors]]) -> object\n\
\n\
Decodes obj using the codec registered for encoding. encoding defaults\n\
to the default encoding. errors may be given to set a different error\n\
handling scheme. Default is 'strict' meaning that encoding errors raise\n\
a ValueError. Other possible values are 'ignore' and 'replace'\n\
as well as any other name registered with codecs.register_error that is\n\
able to handle ValueErrors.");

static PyObject *
codec_decode(PyObject *self, PyObject *args)
{
    const char *encoding = NULL;
    const char *errors = NULL;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O|ss:decode", &v, &encoding, &errors))
        return NULL;

    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    return PyCodec_Decode(v, encoding, errors);
}

/* --- Helpers ------------------------------------------------------------ */

static
PyObject *codec_tuple(PyObject *unicode,
                      Py_ssize_t len)
{
    PyObject *v;
    if (unicode == NULL)
        return NULL;
    v = Py_BuildValue("On", unicode, len);
    Py_DECREF(unicode);
    return v;
}

/* --- String codecs ------------------------------------------------------ */
static PyObject *
escape_decode(PyObject *self,
              PyObject *args)
{
    const char *errors = NULL;
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "s#|z:escape_decode",
                          &data, &size, &errors))
        return NULL;
    return codec_tuple(PyBytes_DecodeEscape(data, size, errors, 0, NULL),
                       size);
}

static PyObject *
escape_encode(PyObject *self,
              PyObject *args)
{
    PyObject *str;
    Py_ssize_t size;
    Py_ssize_t newsize;
    const char *errors = NULL;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "O!|z:escape_encode",
                          &PyBytes_Type, &str, &errors))
        return NULL;

    size = PyBytes_GET_SIZE(str);
    if (size > PY_SSIZE_T_MAX / 4) {
        PyErr_SetString(PyExc_OverflowError,
            "string is too large to encode");
            return NULL;
    }
    newsize = 4*size;
    v = PyBytes_FromStringAndSize(NULL, newsize);

    if (v == NULL) {
        return NULL;
    }
    else {
        Py_ssize_t i;
        char c;
        char *p = PyBytes_AS_STRING(v);

        for (i = 0; i < size; i++) {
            /* There's at least enough room for a hex escape */
            assert(newsize - (p - PyBytes_AS_STRING(v)) >= 4);
            c = PyBytes_AS_STRING(str)[i];
            if (c == '\'' || c == '\\')
                *p++ = '\\', *p++ = c;
            else if (c == '\t')
                *p++ = '\\', *p++ = 't';
            else if (c == '\n')
                *p++ = '\\', *p++ = 'n';
            else if (c == '\r')
                *p++ = '\\', *p++ = 'r';
            else if (c < ' ' || c >= 0x7f) {
                *p++ = '\\';
                *p++ = 'x';
                *p++ = Py_hexdigits[(c & 0xf0) >> 4];
                *p++ = Py_hexdigits[c & 0xf];
            }
            else
                *p++ = c;
        }
        *p = '\0';
        if (_PyBytes_Resize(&v, (p - PyBytes_AS_STRING(v)))) {
            return NULL;
        }
    }

    return codec_tuple(v, size);
}

/* --- Decoder ------------------------------------------------------------ */

static PyObject *
unicode_internal_decode(PyObject *self,
                        PyObject *args)
{
    PyObject *obj;
    const char *errors = NULL;
    const char *data;
    Py_ssize_t size;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_decode",
                          &obj, &errors))
        return NULL;

    if (PyUnicode_Check(obj)) {
        if (PyUnicode_READY(obj) < 0)
            return NULL;
        Py_INCREF(obj);
        return codec_tuple(obj, PyUnicode_GET_LENGTH(obj));
    }
    else {
        if (PyObject_AsReadBuffer(obj, (const void **)&data, &size))
            return NULL;

        return codec_tuple(_PyUnicode_DecodeUnicodeInternal(data, size, errors),
                           size);
    }
}

static PyObject *
utf_7_decode(PyObject *self,
             PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_7_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len;

    decoded = PyUnicode_DecodeUTF7Stateful(pbuf.buf, pbuf.len, errors,
                                           final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_8_decode(PyObject *self,
            PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_8_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len;

    decoded = PyUnicode_DecodeUTF8Stateful(pbuf.buf, pbuf.len, errors,
                                           final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_16_decode(PyObject *self,
            PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_16_le_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = -1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_le_decode",
                          &pbuf, &errors, &final))
        return NULL;

    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(pbuf.buf, pbuf.len, errors,
        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_16_be_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_be_decode",
                          &pbuf, &errors, &final))
        return NULL;

    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF16Stateful(pbuf.buf, pbuf.len, errors,
        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-16 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/

static PyObject *
utf_16_ex_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 0;
    PyObject *unicode, *tuple;
    int final = 0;
    Py_ssize_t consumed;

    if (!PyArg_ParseTuple(args, "y*|zii:utf_16_ex_decode",
                          &pbuf, &errors, &byteorder, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    unicode = PyUnicode_DecodeUTF16Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (unicode == NULL)
        return NULL;
    tuple = Py_BuildValue("Oni", unicode, consumed, byteorder);
    Py_DECREF(unicode);
    return tuple;
}

static PyObject *
utf_32_decode(PyObject *self,
            PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF32Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_32_le_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = -1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_le_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF32Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
utf_32_be_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 1;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_be_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    decoded = PyUnicode_DecodeUTF32Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-32 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/

static PyObject *
utf_32_ex_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int byteorder = 0;
    PyObject *unicode, *tuple;
    int final = 0;
    Py_ssize_t consumed;

    if (!PyArg_ParseTuple(args, "y*|zii:utf_32_ex_decode",
                          &pbuf, &errors, &byteorder, &final))
        return NULL;
    consumed = pbuf.len; /* This is overwritten unless final is true. */
    unicode = PyUnicode_DecodeUTF32Stateful(pbuf.buf, pbuf.len, errors,
                                        &byteorder, final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (unicode == NULL)
        return NULL;
    tuple = Py_BuildValue("Oni", unicode, consumed, byteorder);
    Py_DECREF(unicode);
    return tuple;
}

static PyObject *
unicode_escape_decode(PyObject *self,
                     PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
        PyObject *unicode;

    if (!PyArg_ParseTuple(args, "s*|z:unicode_escape_decode",
                          &pbuf, &errors))
        return NULL;

    unicode = PyUnicode_DecodeUnicodeEscape(pbuf.buf, pbuf.len, errors);
    PyBuffer_Release(&pbuf);
    return codec_tuple(unicode, pbuf.len);
}

static PyObject *
raw_unicode_escape_decode(PyObject *self,
                        PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    PyObject *unicode;

    if (!PyArg_ParseTuple(args, "s*|z:raw_unicode_escape_decode",
                          &pbuf, &errors))
        return NULL;

    unicode = PyUnicode_DecodeRawUnicodeEscape(pbuf.buf, pbuf.len, errors);
    PyBuffer_Release(&pbuf);
    return codec_tuple(unicode, pbuf.len);
}

static PyObject *
latin_1_decode(PyObject *self,
               PyObject *args)
{
    Py_buffer pbuf;
    PyObject *unicode;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y*|z:latin_1_decode",
                          &pbuf, &errors))
        return NULL;

    unicode = PyUnicode_DecodeLatin1(pbuf.buf, pbuf.len, errors);
    PyBuffer_Release(&pbuf);
    return codec_tuple(unicode, pbuf.len);
}

static PyObject *
ascii_decode(PyObject *self,
             PyObject *args)
{
    Py_buffer pbuf;
    PyObject *unicode;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y*|z:ascii_decode",
                          &pbuf, &errors))
        return NULL;

    unicode = PyUnicode_DecodeASCII(pbuf.buf, pbuf.len, errors);
    PyBuffer_Release(&pbuf);
    return codec_tuple(unicode, pbuf.len);
}

static PyObject *
charmap_decode(PyObject *self,
               PyObject *args)
{
    Py_buffer pbuf;
    PyObject *unicode;
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "y*|zO:charmap_decode",
                          &pbuf, &errors, &mapping))
        return NULL;
    if (mapping == Py_None)
        mapping = NULL;

    unicode = PyUnicode_DecodeCharmap(pbuf.buf, pbuf.len, mapping, errors);
    PyBuffer_Release(&pbuf);
    return codec_tuple(unicode, pbuf.len);
}

#ifdef HAVE_MBCS

static PyObject *
mbcs_decode(PyObject *self,
            PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;

    if (!PyArg_ParseTuple(args, "y*|zi:mbcs_decode",
                          &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len;

    decoded = PyUnicode_DecodeMBCSStateful(pbuf.buf, pbuf.len, errors,
                                           final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

static PyObject *
code_page_decode(PyObject *self,
                 PyObject *args)
{
    Py_buffer pbuf;
    const char *errors = NULL;
    int final = 0;
    Py_ssize_t consumed;
    PyObject *decoded = NULL;
    int code_page;

    if (!PyArg_ParseTuple(args, "iy*|zi:code_page_decode",
                          &code_page, &pbuf, &errors, &final))
        return NULL;
    consumed = pbuf.len;

    decoded = PyUnicode_DecodeCodePageStateful(code_page,
                                               pbuf.buf, pbuf.len, errors,
                                               final ? NULL : &consumed);
    PyBuffer_Release(&pbuf);
    if (decoded == NULL)
        return NULL;
    return codec_tuple(decoded, consumed);
}

#endif /* HAVE_MBCS */

/* --- Encoder ------------------------------------------------------------ */

static PyObject *
readbuffer_encode(PyObject *self,
                  PyObject *args)
{
    Py_buffer pdata;
    const char *data;
    Py_ssize_t size;
    const char *errors = NULL;
    PyObject *result;

    if (!PyArg_ParseTuple(args, "s*|z:readbuffer_encode",
                          &pdata, &errors))
        return NULL;
    data = pdata.buf;
    size = pdata.len;

    result = PyBytes_FromStringAndSize(data, size);
    PyBuffer_Release(&pdata);
    return codec_tuple(result, size);
}

static PyObject *
unicode_internal_encode(PyObject *self,
                        PyObject *args)
{
    PyObject *obj;
    const char *errors = NULL;
    const char *data;
    Py_ssize_t len, size;

    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "unicode_internal codec has been deprecated",
                     1))
        return NULL;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_encode",
                          &obj, &errors))
        return NULL;

    if (PyUnicode_Check(obj)) {
        Py_UNICODE *u;

        if (PyUnicode_READY(obj) < 0)
            return NULL;

        u = PyUnicode_AsUnicodeAndSize(obj, &len);
        if (u == NULL)
            return NULL;
        if (len > PY_SSIZE_T_MAX / sizeof(Py_UNICODE))
            return PyErr_NoMemory();
        size = len * sizeof(Py_UNICODE);
        return codec_tuple(PyBytes_FromStringAndSize((const char*)u, size),
                           PyUnicode_GET_LENGTH(obj));
    }
    else {
        if (PyObject_AsReadBuffer(obj, (const void **)&data, &size))
            return NULL;
        return codec_tuple(PyBytes_FromStringAndSize(data, size), size);
    }
}

static PyObject *
utf_7_encode(PyObject *self,
            PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_7_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF7(str, 0, 0, errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_8_encode(PyObject *self,
            PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_8_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(PyUnicode_AsEncodedString(str, "utf-8", errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-16 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

static PyObject *
utf_16_encode(PyObject *self,
            PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    int byteorder = 0;

    if (!PyArg_ParseTuple(args, "O|zi:utf_16_encode",
                          &str, &errors, &byteorder))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF16(str, errors, byteorder),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_16_le_encode(PyObject *self,
                 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_16_le_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF16(str, errors, -1),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_16_be_encode(PyObject *self,
                 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_16_be_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF16(str, errors, +1),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-32 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

static PyObject *
utf_32_encode(PyObject *self,
            PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    int byteorder = 0;

    if (!PyArg_ParseTuple(args, "O|zi:utf_32_encode",
                          &str, &errors, &byteorder))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF32(str, errors, byteorder),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_32_le_encode(PyObject *self,
                 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_32_le_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF32(str, errors, -1),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
utf_32_be_encode(PyObject *self,
                 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:utf_32_be_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeUTF32(str, errors, +1),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
unicode_escape_encode(PyObject *self,
                     PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:unicode_escape_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(PyUnicode_AsUnicodeEscapeString(str),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
raw_unicode_escape_encode(PyObject *self,
                        PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:raw_unicode_escape_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(PyUnicode_AsRawUnicodeEscapeString(str),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
latin_1_encode(PyObject *self,
               PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:latin_1_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_AsLatin1String(str, errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
ascii_encode(PyObject *self,
             PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:ascii_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_AsASCIIString(str, errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
charmap_encode(PyObject *self,
             PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "O|zO:charmap_encode",
                          &str, &errors, &mapping))
        return NULL;
    if (mapping == Py_None)
        mapping = NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(_PyUnicode_EncodeCharmap(str, mapping, errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject*
charmap_build(PyObject *self, PyObject *args)
{
    PyObject *map;
    if (!PyArg_ParseTuple(args, "U:charmap_build", &map))
        return NULL;
    return PyUnicode_BuildEncodingMap(map);
}

#ifdef HAVE_MBCS

static PyObject *
mbcs_encode(PyObject *self,
            PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:mbcs_encode",
                          &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(PyUnicode_EncodeCodePage(CP_ACP, str, errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

static PyObject *
code_page_encode(PyObject *self,
                 PyObject *args)
{
    PyObject *str, *v;
    const char *errors = NULL;
    int code_page;

    if (!PyArg_ParseTuple(args, "iO|z:code_page_encode",
                          &code_page, &str, &errors))
        return NULL;

    str = PyUnicode_FromObject(str);
    if (str == NULL || PyUnicode_READY(str) < 0) {
        Py_XDECREF(str);
        return NULL;
    }
    v = codec_tuple(PyUnicode_EncodeCodePage(code_page,
                                             str,
                                             errors),
                    PyUnicode_GET_LENGTH(str));
    Py_DECREF(str);
    return v;
}

#endif /* HAVE_MBCS */

/* --- Error handler registry --------------------------------------------- */

PyDoc_STRVAR(register_error__doc__,
"register_error(errors, handler)\n\
\n\
Register the specified error handler under the name\n\
errors. handler must be a callable object, that\n\
will be called with an exception instance containing\n\
information about the location of the encoding/decoding\n\
error and must return a (replacement, new position) tuple.");

static PyObject *register_error(PyObject *self, PyObject *args)
{
    const char *name;
    PyObject *handler;

    if (!PyArg_ParseTuple(args, "sO:register_error",
                          &name, &handler))
        return NULL;
    if (PyCodec_RegisterError(name, handler))
        return NULL;
    Py_RETURN_NONE;
}

PyDoc_STRVAR(lookup_error__doc__,
"lookup_error(errors) -> handler\n\
\n\
Return the error handler for the specified error handling name\n\
or raise a LookupError, if no handler exists under this name.");

static PyObject *lookup_error(PyObject *self, PyObject *args)
{
    const char *name;

    if (!PyArg_ParseTuple(args, "s:lookup_error",
                          &name))
        return NULL;
    return PyCodec_LookupError(name);
}

/* --- Module API --------------------------------------------------------- */

static PyMethodDef _codecs_functions[] = {
    {"register",                codec_register,                 METH_O,
        register__doc__},
    {"lookup",                  codec_lookup,                   METH_VARARGS,
        lookup__doc__},
    {"encode",                  codec_encode,                   METH_VARARGS,
        encode__doc__},
    {"decode",                  codec_decode,                   METH_VARARGS,
        decode__doc__},
    {"escape_encode",           escape_encode,                  METH_VARARGS},
    {"escape_decode",           escape_decode,                  METH_VARARGS},
    {"utf_8_encode",            utf_8_encode,                   METH_VARARGS},
    {"utf_8_decode",            utf_8_decode,                   METH_VARARGS},
    {"utf_7_encode",            utf_7_encode,                   METH_VARARGS},
    {"utf_7_decode",            utf_7_decode,                   METH_VARARGS},
    {"utf_16_encode",           utf_16_encode,                  METH_VARARGS},
    {"utf_16_le_encode",        utf_16_le_encode,               METH_VARARGS},
    {"utf_16_be_encode",        utf_16_be_encode,               METH_VARARGS},
    {"utf_16_decode",           utf_16_decode,                  METH_VARARGS},
    {"utf_16_le_decode",        utf_16_le_decode,               METH_VARARGS},
    {"utf_16_be_decode",        utf_16_be_decode,               METH_VARARGS},
    {"utf_16_ex_decode",        utf_16_ex_decode,               METH_VARARGS},
    {"utf_32_encode",           utf_32_encode,                  METH_VARARGS},
    {"utf_32_le_encode",        utf_32_le_encode,               METH_VARARGS},
    {"utf_32_be_encode",        utf_32_be_encode,               METH_VARARGS},
    {"utf_32_decode",           utf_32_decode,                  METH_VARARGS},
    {"utf_32_le_decode",        utf_32_le_decode,               METH_VARARGS},
    {"utf_32_be_decode",        utf_32_be_decode,               METH_VARARGS},
    {"utf_32_ex_decode",        utf_32_ex_decode,               METH_VARARGS},
    {"unicode_escape_encode",   unicode_escape_encode,          METH_VARARGS},
    {"unicode_escape_decode",   unicode_escape_decode,          METH_VARARGS},
    {"unicode_internal_encode", unicode_internal_encode,        METH_VARARGS},
    {"unicode_internal_decode", unicode_internal_decode,        METH_VARARGS},
    {"raw_unicode_escape_encode", raw_unicode_escape_encode,    METH_VARARGS},
    {"raw_unicode_escape_decode", raw_unicode_escape_decode,    METH_VARARGS},
    {"latin_1_encode",          latin_1_encode,                 METH_VARARGS},
    {"latin_1_decode",          latin_1_decode,                 METH_VARARGS},
    {"ascii_encode",            ascii_encode,                   METH_VARARGS},
    {"ascii_decode",            ascii_decode,                   METH_VARARGS},
    {"charmap_encode",          charmap_encode,                 METH_VARARGS},
    {"charmap_decode",          charmap_decode,                 METH_VARARGS},
    {"charmap_build",           charmap_build,                  METH_VARARGS},
    {"readbuffer_encode",       readbuffer_encode,              METH_VARARGS},
#ifdef HAVE_MBCS
    {"mbcs_encode",             mbcs_encode,                    METH_VARARGS},
    {"mbcs_decode",             mbcs_decode,                    METH_VARARGS},
    {"code_page_encode",        code_page_encode,               METH_VARARGS},
    {"code_page_decode",        code_page_decode,               METH_VARARGS},
#endif
    {"register_error",          register_error,                 METH_VARARGS,
        register_error__doc__},
    {"lookup_error",            lookup_error,                   METH_VARARGS,
        lookup_error__doc__},
    {NULL, NULL}                /* sentinel */
};

static struct PyModuleDef codecsmodule = {
        PyModuleDef_HEAD_INIT,
        "_codecs",
        NULL,
        -1,
        _codecs_functions,
        NULL,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__codecs(void)
{
        return PyModule_Create(&codecsmodule);
}
