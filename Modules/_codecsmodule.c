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

   These <encoding>s are available: utf_8, unicode_escape,
   raw_unicode_escape, latin_1, ascii (7-bit), mbcs (on win32).


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
_codecs_register(PyObject *module, PyObject *search_function)
/*[clinic end generated code: output=d1bf21e99db7d6d3 input=369578467955cae4]*/
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
_codecs_lookup_impl(PyObject *module, const char *encoding)
/*[clinic end generated code: output=9f0afa572080c36d input=3c572c0db3febe9c]*/
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
_codecs_encode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors)
/*[clinic end generated code: output=385148eb9a067c86 input=cd5b685040ff61f0]*/
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
_codecs_decode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors)
/*[clinic end generated code: output=679882417dc3a0bd input=7702c0cc2fa1add6]*/
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
_codecs__forget_codec_impl(PyObject *module, const char *encoding)
/*[clinic end generated code: output=0bde9f0a5b084aa2 input=18d5d92d0e386c38]*/
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
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_escape_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors)
/*[clinic end generated code: output=505200ba8056979a input=77298a561c90bd82]*/
{
    PyObject *decoded = PyBytes_DecodeEscape(data->buf, data->len,
                                             errors, 0, NULL);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.escape_encode
    data: object(subclass_of='&PyBytes_Type')
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_escape_encode_impl(PyObject *module, PyObject *data,
                           const char *errors)
/*[clinic end generated code: output=4af1d477834bab34 input=8f4b144799a94245]*/
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
_codecs.utf_7_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_7_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=0cd3a944a32a4089 input=22c395d357815d26]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_8_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final)
/*[clinic end generated code: output=10f74dec8d9bb8bf input=f611b3867352ba59]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=783b442abcbcc2d0 input=191d360bd7309180]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=899b9e6364379dcd input=c6904fdc27fb4724]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=49f6465ea07669c8 input=e49012400974649b]*/
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
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=0f385f251ecc1988 input=5a9c19f2e6b6cf0e]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final)
/*[clinic end generated code: output=2fc961807f7b145f input=fd7193965627eb58]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=ec8f46b67a94f3e6 input=9078ec70acfe7613]*/
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
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final)
/*[clinic end generated code: output=ff82bae862c92c4e input=f1ae1bbbb86648ff]*/
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
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final)
/*[clinic end generated code: output=6bfb177dceaf4848 input=e46a73bc859d0bd0]*/
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
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                   const char *errors)
/*[clinic end generated code: output=3ca3c917176b82ab input=8328081a3a569bd6]*/
{
    PyObject *decoded = PyUnicode_DecodeUnicodeEscape(data->buf, data->len,
                                                      errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.raw_unicode_escape_decode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_raw_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                       const char *errors)
/*[clinic end generated code: output=c98eeb56028070a6 input=d2f5159ce3b3392f]*/
{
    PyObject *decoded = PyUnicode_DecodeRawUnicodeEscape(data->buf, data->len,
                                                         errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.latin_1_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_latin_1_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors)
/*[clinic end generated code: output=07f3dfa3f72c7d8f input=76ca58fd6dcd08c7]*/
{
    PyObject *decoded = PyUnicode_DecodeLatin1(data->buf, data->len, errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.ascii_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_ascii_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors)
/*[clinic end generated code: output=2627d72058d42429 input=e428a267a04b4481]*/
{
    PyObject *decoded = PyUnicode_DecodeASCII(data->buf, data->len, errors);
    return codec_tuple(decoded, data->len);
}

/*[clinic input]
_codecs.charmap_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    mapping: object = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors, PyObject *mapping)
/*[clinic end generated code: output=2c335b09778cf895 input=15b69df43458eb40]*/
{
    PyObject *decoded;

    if (mapping == Py_None)
        mapping = NULL;

    decoded = PyUnicode_DecodeCharmap(data->buf, data->len, mapping, errors);
    return codec_tuple(decoded, data->len);
}

#ifdef MS_WINDOWS

/*[clinic input]
_codecs.mbcs_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_mbcs_decode_impl(PyObject *module, Py_buffer *data,
                         const char *errors, int final)
/*[clinic end generated code: output=39b65b8598938c4b input=1c1d50f08fa53789]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeMBCSStateful(data->buf, data->len,
            errors, final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.oem_decode
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_oem_decode_impl(PyObject *module, Py_buffer *data,
                        const char *errors, int final)
/*[clinic end generated code: output=da1617612f3fcad8 input=81b67cba811022e5]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeCodePageStateful(CP_OEMCP,
        data->buf, data->len, errors, final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

/*[clinic input]
_codecs.code_page_decode
    codepage: int
    data: Py_buffer
    errors: str(accept={str, NoneType}) = None
    final: bool(accept={int}) = False
    /
[clinic start generated code]*/

static PyObject *
_codecs_code_page_decode_impl(PyObject *module, int codepage,
                              Py_buffer *data, const char *errors, int final)
/*[clinic end generated code: output=53008ea967da3fff input=c5f58d036cb63575]*/
{
    Py_ssize_t consumed = data->len;
    PyObject *decoded = PyUnicode_DecodeCodePageStateful(codepage,
                                                         data->buf, data->len,
                                                         errors,
                                                         final ? NULL : &consumed);
    return codec_tuple(decoded, consumed);
}

#endif /* MS_WINDOWS */

/* --- Encoder ------------------------------------------------------------ */

/*[clinic input]
_codecs.readbuffer_encode
    data: Py_buffer(accept={str, buffer})
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_readbuffer_encode_impl(PyObject *module, Py_buffer *data,
                               const char *errors)
/*[clinic end generated code: output=c645ea7cdb3d6e86 input=aa10cfdf252455c5]*/
{
    PyObject *result = PyBytes_FromStringAndSize(data->buf, data->len);
    return codec_tuple(result, data->len);
}

/*[clinic input]
_codecs.utf_7_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_7_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=0feda21ffc921bc8 input=2546dbbb3fa53114]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF7(str, 0, 0, errors),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.utf_8_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_8_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=02bf47332b9c796c input=a3e71ae01c3f93f3]*/
{
    return codec_tuple(_PyUnicode_AsUTF8String(str, errors),
                       PyUnicode_GET_LENGTH(str));
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-16 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

/*[clinic input]
_codecs.utf_16_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=c654e13efa2e64e4 input=68cdc2eb8338555d]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF16(str, errors, byteorder),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.utf_16_le_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=431b01e55f2d4995 input=83d042706eed6798]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF16(str, errors, -1),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.utf_16_be_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_16_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=96886a6fd54dcae3 input=6f1e9e623b03071b]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF16(str, errors, +1),
                       PyUnicode_GET_LENGTH(str));
}

/* This version provides access to the byteorder parameter of the
   builtin UTF-32 codecs as optional third argument. It defaults to 0
   which means: use the native byte order and prepend the data with a
   BOM mark.

*/

/*[clinic input]
_codecs.utf_32_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    byteorder: int = 0
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder)
/*[clinic end generated code: output=5c760da0c09a8b83 input=8ec4c64d983bc52b]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF32(str, errors, byteorder),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.utf_32_le_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=b65cd176de8e36d6 input=f0918d41de3eb1b1]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF32(str, errors, -1),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.utf_32_be_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_utf_32_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=1d9e71a9358709e9 input=967a99a95748b557]*/
{
    return codec_tuple(_PyUnicode_EncodeUTF32(str, errors, +1),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.unicode_escape_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                   const char *errors)
/*[clinic end generated code: output=66271b30bc4f7a3c input=8c4de07597054e33]*/
{
    return codec_tuple(PyUnicode_AsUnicodeEscapeString(str),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.raw_unicode_escape_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_raw_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                       const char *errors)
/*[clinic end generated code: output=a66a806ed01c830a input=4aa6f280d78e4574]*/
{
    return codec_tuple(PyUnicode_AsRawUnicodeEscapeString(str),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.latin_1_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_latin_1_encode_impl(PyObject *module, PyObject *str,
                            const char *errors)
/*[clinic end generated code: output=2c28c83a27884e08 input=ec3ef74bf85c5c5d]*/
{
    return codec_tuple(_PyUnicode_AsLatin1String(str, errors),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.ascii_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_ascii_encode_impl(PyObject *module, PyObject *str,
                          const char *errors)
/*[clinic end generated code: output=b5e035182d33befc input=93e6e602838bd3de]*/
{
    return codec_tuple(_PyUnicode_AsASCIIString(str, errors),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.charmap_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    mapping: object = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_encode_impl(PyObject *module, PyObject *str,
                            const char *errors, PyObject *mapping)
/*[clinic end generated code: output=047476f48495a9e9 input=2a98feae73dadce8]*/
{
    if (mapping == Py_None)
        mapping = NULL;

    return codec_tuple(_PyUnicode_EncodeCharmap(str, mapping, errors),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.charmap_build
    map: unicode
    /
[clinic start generated code]*/

static PyObject *
_codecs_charmap_build_impl(PyObject *module, PyObject *map)
/*[clinic end generated code: output=bb073c27031db9ac input=d91a91d1717dbc6d]*/
{
    return PyUnicode_BuildEncodingMap(map);
}

#ifdef MS_WINDOWS

/*[clinic input]
_codecs.mbcs_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_mbcs_encode_impl(PyObject *module, PyObject *str, const char *errors)
/*[clinic end generated code: output=76e2e170c966c080 input=2e932fc289ea5a5b]*/
{
    return codec_tuple(PyUnicode_EncodeCodePage(CP_ACP, str, errors),
                       PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.oem_encode
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_oem_encode_impl(PyObject *module, PyObject *str, const char *errors)
/*[clinic end generated code: output=65d5982c737de649 input=9eac86dc21eb14f2]*/
{
    return codec_tuple(PyUnicode_EncodeCodePage(CP_OEMCP, str, errors),
        PyUnicode_GET_LENGTH(str));
}

/*[clinic input]
_codecs.code_page_encode
    code_page: int
    str: unicode
    errors: str(accept={str, NoneType}) = None
    /
[clinic start generated code]*/

static PyObject *
_codecs_code_page_encode_impl(PyObject *module, int code_page, PyObject *str,
                              const char *errors)
/*[clinic end generated code: output=45673f6085657a9e input=7d18a33bc8cd0f94]*/
{
    return codec_tuple(PyUnicode_EncodeCodePage(code_page, str, errors),
                       PyUnicode_GET_LENGTH(str));
}

#endif /* MS_WINDOWS */

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
_codecs_register_error_impl(PyObject *module, const char *errors,
                            PyObject *handler)
/*[clinic end generated code: output=fa2f7d1879b3067d input=5e6709203c2e33fe]*/
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
_codecs_lookup_error_impl(PyObject *module, const char *name)
/*[clinic end generated code: output=087f05dc0c9a98cc input=4775dd65e6235aba]*/
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
    _CODECS_OEM_ENCODE_METHODDEF
    _CODECS_OEM_DECODE_METHODDEF
    _CODECS_CODE_PAGE_ENCODE_METHODDEF
    _CODECS_CODE_PAGE_DECODE_METHODDEF
    _CODECS_REGISTER_ERROR_METHODDEF
    _CODECS_LOOKUP_ERROR_METHODDEF
    _CODECS__FORGET_CODEC_METHODDEF
    {NULL, NULL}                /* sentinel */
};

static PyModuleDef_Slot _codecs_slots[] = {
    {0, NULL}
};

static struct PyModuleDef codecsmodule = {
        PyModuleDef_HEAD_INIT,
        "_codecs",
        NULL,
        0,
        _codecs_functions,
        _codecs_slots,
        NULL,
        NULL,
        NULL
};

PyMODINIT_FUNC
PyInit__codecs(void)
{
    return PyModuleDef_Init(&codecsmodule);
}
