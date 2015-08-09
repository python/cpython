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

/*[clinic input]
module _codecs
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=e1390e3da3cb9deb]*/

#include "clinic/_codecsmodule.c.h"

/* --- Registry ----------------------------------------------------------- */

/*[clinic input]
_codecs.register
    search_function: object
    /

Register a codec search function.

Search functions are expected to take one argument, the encoding name in
all lower case letters, and either return None, or a tuple of functions
(encoder, decoder, stream_reader, stream_writer) (or a CodecInfo object).
[clinic start generated code]*/

static PyObject *
_codecs_register(PyModuleDef *module, PyObject *search_function)
/*[clinic end generated code: output=d17608b6ad380eb8 input=369578467955cae4]*/
{
    if (PyCodec_Register(search_function))
        return NULL;

    Py_RETURN_NONE;
}

/*[clinic input]
_codecs.lookup
    encoding: str
    /

Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.
[clinic start generated code]*/

static PyObject *
_codecs_lookup_impl(PyModuleDef *module, const char *encoding)
/*[clinic end generated code: output=798e41aff0c04ef6 input=3c572c0db3febe9c]*/
{
    return _PyCodec_Lookup(encoding);
}

/*[clinic input]
_codecs.encode
    obj: object
    encoding: str(c_default="NULL") = "utf-8"
    errors: str(c_default="NULL") = "strict"

Encodes obj using the codec registered for encoding.

The default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.
[clinic start generated code]*/

static PyObject *
_codecs_encode_impl(PyModuleDef *module, PyObject *obj, const char *encoding,
                    const char *errors)
/*[clinic end generated code: output=5c073f62249c8d7c input=cd5b685040ff61f0]*/
{
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Encode via the codec registry */
    return PyCodec_Encode(obj, encoding, errors);
}

/*[clinic input]
_codecs.decode
    obj: object
    encoding: str(c_default="NULL") = "utf-8"
    errors: str(c_default="NULL") = "strict"

Decodes obj using the codec registered for encoding.

Default encoding is 'utf-8'.  errors may be given to set a
different error handling scheme.  Default is 'strict' meaning that encoding
errors raise a ValueError.  Other possible values are 'ignore', 'replace'
and 'backslashreplace' as well as any other name registered with
codecs.register_error that can handle ValueErrors.
[clinic start generated code]*/

static PyObject *
_codecs_decode_impl(PyModuleDef *module, PyObject *obj, const char *encoding,
                    const char *errors)
/*[clinic end generated code: output=c81cbf6189a7f878 input=7702c0cc2fa1add6]*/
{
    if (encoding == NULL)
        encoding = PyUnicode_GetDefaultEncoding();

    /* Decode via the codec registry */
    return PyCodec_Decode(obj, encoding, errors);
}

/* --- Helpers ------------------------------------------------------------ */

/*[clinic input]
_codecs._forget_codec

    encoding: str
    /

Purge the named codec from the internal codec lookup cache
[clinic start generated code]*/

static PyObject *
_codecs__forget_codec_impl(PyModuleDef *module, const char *encoding)
/*[clinic end generated code: output=b56a9b99d2d28080 input=18d5d92d0e386c38]*/
{
    if (_PyCodec_Forget(encoding) < 0) {
        return NULL;
    };
    Py_RETURN_NONE;
}

static
PyObject *codec_tuple(PyObject *decoded,
                      Py_ssize_t len)
{
    if (decoded == NULL)
        return NULL;
    return Py_BuildValue("Nn", decoded, len);
}

/* --- String codecs ------------------------------------------------------ */
/*[clinic input]
_codecs.escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_escape_decode_impl(PyModuleDef *module, Py_buffer *data,
                           const char *errors)
/*[clinic end generated code: output=648fa3e78d03e658 input=0018edfd99db714d]*/
{
    PyObject *decoded = PyBytes_DecodeEscape(data->buf, data->len,
                                             errors, 0, NULL);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.escape_encode
    data: object(subclass_of='&PyBytes_Type')
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_escape_encode_impl(PyModuleDef *module, PyObject *data,
                           const char *errors)
/*[clinic end generated code: output=fcd6f34fe4111c50 input=da9ded00992f32f2]*/
{
    Py_ssize_t size;
    Py_ssize_t newsize;
    PyObject *v;

    size = PyBytes_GET_SIZE(data);
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
            c = PyBytes_AS_STRING(data)[i];
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
/*[clinic input]
_codecs.unicode_internal_decode
    obj: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_internal_decode_impl(PyModuleDef *module, PyObject *obj,
                                     const char *errors)
/*[clinic end generated code: output=9fe47c2cd8807d92 input=8d57930aeda170c6]*/
{
    if (PyUnicode_Check(obj)) {
        if (PyUnicode_READY(obj) < 0)
            return NULL;
        Py_INCREF(obj);
        return codec_tuple(obj, PyUnicode_GET_LENGTH(obj));
    }
    else {
        Py_buffer view;
        PyObject *result;
        if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) != 0)
            return NULL;

        result = codec_tuple(
                _PyUnicode_DecodeUnicodeInternal(view.buf, view.len, errors),
                view.len);
        PyBuffer_Release(&view);
        return result;
    }
}

/*[clinic input]
_codecs.utf_7_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_7_decode_impl(PyModuleDef *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=ca945e907e72e827 input=bc4d6247ecdb01e6]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF7Stateful(data->buf, data->len,
                                                     errors,
                                                     final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_8_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_8_decode_impl(PyModuleDef *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=7309f9ff4ef5c9b6 input=39161d71e7422ee2]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF8Stateful(data->buf, data->len,
                                                     errors,
                                                     final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_16_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_decode_impl(PyModuleDef *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=8d2fa0507d9bef2c input=f3cf01d1461007ce]*/
{
    int byteorder = 0;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_16_le_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_le_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=4fd621515ef4ce18 input=a77e3bf97335d94e]*/
{
    int byteorder = -1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_16_be_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_be_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=792f4eacb3e1fa05 input=606f69fae91b5563]*/
{
    int byteorder = 1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-16 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/
/*[clinic input]
_codecs.utf_16_ex_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    byteorder: int = 0
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_ex_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=f136a186dc2defa0 input=f6e7f697658c013e]*/
{
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;

    PyObject *decoded = PyUnicode_DecodeUTF16Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    if (decoded == NULL)
        return NULL;
    return Py_BuildValue("Nni", decoded, consumed, byteorder);
}

/*[clinic input]
_codecs.utf_32_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_decode_impl(PyModuleDef *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=b7635e55857e8efb input=86d4f41c6c2e763d]*/
{
    int byteorder = 0;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_32_le_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_le_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=a79d1787d8ddf988 input=d18b650772d188ba]*/
{
    int byteorder = -1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.utf_32_be_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_be_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=a8356b0f36779981 input=19c271b5d34926d8]*/
{
    int byteorder = 1;
    /* This is overwritten unless final is true. */
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/* This non-standard version also provides access to the byteorder
   parameter of the builtin UTF-32 codec.

   It returns a tuple (unicode, bytesread, byteorder) with byteorder
   being the value in effect at the end of data.

*/
/*[clinic input]
_codecs.utf_32_ex_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    byteorder: int = 0
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_ex_decode_impl(PyModuleDef *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=ab8c70977c1992f5 input=4af3e6ccfe34a076]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeUTF32Stateful(data->buf, data->len,
                                                      errors, &byteorder,
                                                      final ? NULL : &consumed);
    if (decoded == NULL)
        return NULL;
    return Py_BuildValue("Nni", decoded, consumed, byteorder);
}

/*[clinic input]
_codecs.unicode_escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_escape_decode_impl(PyModuleDef *module, Py_buffer *data,
                                   const char *errors)
/*[clinic end generated code: output=d1aa63f2620c4999 input=49fd27d06813a7f5]*/
{
    PyObject *decoded = PyUnicode_DecodeUnicodeEscape(data->buf, data->len,
                                                      errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.raw_unicode_escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_raw_unicode_escape_decode_impl(PyModuleDef *module, Py_buffer *data,
                                       const char *errors)
/*[clinic end generated code: output=0bf96cc182d81379 input=770903a211434ebc]*/
{
    PyObject *decoded = PyUnicode_DecodeRawUnicodeEscape(data->buf, data->len,
                                                         errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.latin_1_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_latin_1_decode_impl(PyModuleDef *module, Py_buffer *data,
                            const char *errors)
/*[clinic end generated code: output=66b916f5055aaf13 input=5cad0f1759c618ec]*/
{
    PyObject *decoded = PyUnicode_DecodeLatin1(data->buf, data->len, errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.ascii_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_ascii_decode_impl(PyModuleDef *module, Py_buffer *data,
                          const char *errors)
/*[clinic end generated code: output=7f213a1b5cdafc65 input=ad1106f64037bd16]*/
{
    PyObject *decoded = PyUnicode_DecodeASCII(data->buf, data->len, errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.charmap_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    mapping: object = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_decode_impl(PyModuleDef *module, Py_buffer *data,
                            const char *errors, PyObject *mapping)
/*[clinic end generated code: output=87d27f365098bbae input=19712ca35c5a80e2]*/
{
    PyObject *decoded;

    if (mapping == Py_None)
        mapping = NULL;

    decoded = PyUnicode_DecodeCharmap(data->buf, data->len, mapping, errors);
    return codec_tuple(decoded, data->len);
}

#ifdef HAVE_MBCS

/*[clinic input]
_codecs.mbcs_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_mbcs_decode_impl(PyModuleDef *module, Py_buffer *data,
                         const char *errors, int final)
/*[clinic end generated code: output=0ebaf3a5b20e53fa input=d492c1ca64f4fa8a]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeMBCSStateful(data->buf, data->len,
            errors, final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.code_page_decode
    codepage: int
    data: Py_buffer
    errors: str(accept={str, NoneType}) = NULL
    final: int(c_default="0") = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_code_page_decode_impl(PyModuleDef *module, int codepage,
                              Py_buffer *data, const char *errors, int final)
/*[clinic end generated code: output=4318e3d9971e31ba input=4f3152a304e21d51]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeCodePageStateful(codepage,
                                                         data->buf, data->len,
                                                         errors,
                                                         final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

#endif /* HAVE_MBCS */

/* --- Encoder ------------------------------------------------------------ */

/*[clinic input]
_codecs.readbuffer_encode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_readbuffer_encode_impl(PyModuleDef *module, Py_buffer *data,
                               const char *errors)
/*[clinic end generated code: output=319cc24083299859 input=b7c322b89d4ab923]*/
{
    PyObject *result = PyBytes_FromStringAndSize(data->buf, data->len);
    return codec_tuple(result, data->len);
}

/*[clinic input]
_codecs.unicode_internal_encode
    obj: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_internal_encode_impl(PyModuleDef *module, PyObject *obj,
                                     const char *errors)
/*[clinic end generated code: output=be08457068ad503b input=8628f0280cf5ba61]*/
{
    if (PyErr_WarnEx(PyExc_DeprecationWarning,
                     "unicode_internal codec has been deprecated",
                     1))
        return NULL;

    if (PyUnicode_Check(obj)) {
        Py_UNICODE *u;
        Py_ssize_t len, size;

        if (PyUnicode_READY(obj) < 0)
            return NULL;

        u = PyUnicode_AsUnicodeAndSize(obj, &len);
        if (u == NULL)
            return NULL;
        if ((size_t)len > (size_t)PY_SSIZE_T_MAX / sizeof(Py_UNICODE))
            return PyErr_NoMemory();
        size = len * sizeof(Py_UNICODE);
        return codec_tuple(PyBytes_FromStringAndSize((const char*)u, size),
                           PyUnicode_GET_LENGTH(obj));
    }
    else {
        Py_buffer view;
        PyObject *result;
        if (PyObject_GetBuffer(obj, &view, PyBUF_SIMPLE) != 0)
            return NULL;
        result = codec_tuple(PyBytes_FromStringAndSize(view.buf, view.len),
                             view.len);
        PyBuffer_Release(&view);
        return result;
    }
}

/*[clinic input]
_codecs.utf_7_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_7_encode_impl(PyModuleDef *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=a7accc496a32b759 input=fd91a78f103b0421]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_8_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_8_encode_impl(PyModuleDef *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=ec831d80e7aedede input=2c22d40532f071f3]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_16_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_encode_impl(PyModuleDef *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=93ac58e960a9ee4d input=3935a489b2d5385e]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_16_le_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_le_encode_impl(PyModuleDef *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=422bedb8da34fb66 input=bc27df05d1d20dfe]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_16_be_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_be_encode_impl(PyModuleDef *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=3aa7ee9502acdd77 input=5a69d4112763462b]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_32_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_encode_impl(PyModuleDef *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=3e7d5a003b02baed input=434a1efa492b8d58]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_32_le_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_le_encode_impl(PyModuleDef *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=5dda641cd33dbfc2 input=dfa2d7dc78b99422]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.utf_32_be_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_be_encode_impl(PyModuleDef *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=ccca8b44d91a7c7a input=4595617b18169002]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.unicode_escape_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_escape_encode_impl(PyModuleDef *module, PyObject *str,
                                   const char *errors)
/*[clinic end generated code: output=389f23d2b8f8d80b input=8273506f14076912]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.raw_unicode_escape_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_raw_unicode_escape_encode_impl(PyModuleDef *module, PyObject *str,
                                       const char *errors)
/*[clinic end generated code: output=fec4e39d6ec37a62 input=181755d5dfacef3c]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.latin_1_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_latin_1_encode_impl(PyModuleDef *module, PyObject *str,
                            const char *errors)
/*[clinic end generated code: output=ecf00eb8e48c889c input=f03f6dcf1d84bee4]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.ascii_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_ascii_encode_impl(PyModuleDef *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=a9d18fc6b6b91cfb input=d87e25a10a593fee]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.charmap_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    mapping: object = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_encode_impl(PyModuleDef *module, PyObject *str,
                            const char *errors, PyObject *mapping)
/*[clinic end generated code: output=14ca42b83853c643 input=85f4172661e8dad9]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.charmap_build
    map: unicode
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_build_impl(PyModuleDef *module, PyObject *map)
/*[clinic end generated code: output=9485b58fa44afa6a input=d91a91d1717dbc6d]*/
{
    return PyUnicode_BuildEncodingMap(map);
}

#ifdef HAVE_MBCS

/*[clinic input]
_codecs.mbcs_encode
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_mbcs_encode_impl(PyModuleDef *module, PyObject *str,
                         const char *errors)
/*[clinic end generated code: output=d1a013bc68798bd7 input=65c09ee1e4203263]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.code_page_encode
    code_page: int
    str: object
    errors: str(accept={str, NoneType}) = NULL
    /
[clinic start generated code]*/

static PyObject *
_codecs_code_page_encode_impl(PyModuleDef *module, int code_page,
                              PyObject *str, const char *errors)
/*[clinic end generated code: output=3b406618dbfbce25 input=c8562ec460c2e309]*/
{
    PyObject *v;

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

/*[clinic input]
_codecs.register_error
    errors: str
    handler: object
    /

Register the specified error handler under the name errors.

handler must be a callable object, that will be called with an exception
instance containing information about the location of the encoding/decoding
error and must return a (replacement, new position) tuple.
[clinic start generated code]*/

static PyObject *
_codecs_register_error_impl(PyModuleDef *module, const char *errors,
                            PyObject *handler)
/*[clinic end generated code: output=be00d3b1849ce68a input=5e6709203c2e33fe]*/
{
    if (PyCodec_RegisterError(errors, handler))
        return NULL;
    Py_RETURN_NONE;
}

/*[clinic input]
_codecs.lookup_error
    name: str
    /

lookup_error(errors) -> handler

Return the error handler for the specified error handling name or raise a
LookupError, if no handler exists under this name.
[clinic start generated code]*/

static PyObject *
_codecs_lookup_error_impl(PyModuleDef *module, const char *name)
/*[clinic end generated code: output=731e6df8c83c6158 input=4775dd65e6235aba]*/
{
    return PyCodec_LookupError(name);
}

/* --- Module API --------------------------------------------------------- */

static PyMethodDef _codecs_functions[] = {
    _CODECS_REGISTER_METHODDEF
    _CODECS_LOOKUP_METHODDEF
    _CODECS_ENCODE_METHODDEF
    _CODECS_DECODE_METHODDEF
    _CODECS_ESCAPE_ENCODE_METHODDEF
    _CODECS_ESCAPE_DECODE_METHODDEF
    _CODECS_UTF_8_ENCODE_METHODDEF
    _CODECS_UTF_8_DECODE_METHODDEF
    _CODECS_UTF_7_ENCODE_METHODDEF
    _CODECS_UTF_7_DECODE_METHODDEF
    _CODECS_UTF_16_ENCODE_METHODDEF
    _CODECS_UTF_16_LE_ENCODE_METHODDEF
    _CODECS_UTF_16_BE_ENCODE_METHODDEF
    _CODECS_UTF_16_DECODE_METHODDEF
    _CODECS_UTF_16_LE_DECODE_METHODDEF
    _CODECS_UTF_16_BE_DECODE_METHODDEF
    _CODECS_UTF_16_EX_DECODE_METHODDEF
    _CODECS_UTF_32_ENCODE_METHODDEF
    _CODECS_UTF_32_LE_ENCODE_METHODDEF
    _CODECS_UTF_32_BE_ENCODE_METHODDEF
    _CODECS_UTF_32_DECODE_METHODDEF
    _CODECS_UTF_32_LE_DECODE_METHODDEF
    _CODECS_UTF_32_BE_DECODE_METHODDEF
    _CODECS_UTF_32_EX_DECODE_METHODDEF
    _CODECS_UNICODE_ESCAPE_ENCODE_METHODDEF
    _CODECS_UNICODE_ESCAPE_DECODE_METHODDEF
    _CODECS_UNICODE_INTERNAL_ENCODE_METHODDEF
    _CODECS_UNICODE_INTERNAL_DECODE_METHODDEF
    _CODECS_RAW_UNICODE_ESCAPE_ENCODE_METHODDEF
    _CODECS_RAW_UNICODE_ESCAPE_DECODE_METHODDEF
    _CODECS_LATIN_1_ENCODE_METHODDEF
    _CODECS_LATIN_1_DECODE_METHODDEF
    _CODECS_ASCII_ENCODE_METHODDEF
    _CODECS_ASCII_DECODE_METHODDEF
    _CODECS_CHARMAP_ENCODE_METHODDEF
    _CODECS_CHARMAP_DECODE_METHODDEF
    _CODECS_CHARMAP_BUILD_METHODDEF
    _CODECS_READBUFFER_ENCODE_METHODDEF
    _CODECS_MBCS_ENCODE_METHODDEF
    _CODECS_MBCS_DECODE_METHODDEF
    _CODECS_CODE_PAGE_ENCODE_METHODDEF
    _CODECS_CODE_PAGE_DECODE_METHODDEF
    _CODECS_REGISTER_ERROR_METHODDEF
    _CODECS_LOOKUP_ERROR_METHODDEF
    _CODECS__FORGET_CODEC_METHODDEF
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
