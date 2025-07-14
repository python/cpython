/* ------------------------------------------------------------------------

   Python Codec Registry and support functions

Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_interp.h"        // PyInterpreterState.codec_search_path
#include "pycore_pyerrors.h"      // _PyErr_FormatNote()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI
#include "pycore_unicodeobject.h" // _PyUnicode_InternMortal()


static const char *codecs_builtin_error_handlers[] = {
    "strict", "ignore", "replace",
    "xmlcharrefreplace", "backslashreplace", "namereplace",
    "surrogatepass", "surrogateescape",
};

const char *Py_hexdigits = "0123456789abcdef";

/* --- Codec Registry ----------------------------------------------------- */

int PyCodec_Register(PyObject *search_function)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->codecs.initialized);
    if (search_function == NULL) {
        PyErr_BadArgument();
        goto onError;
    }
    if (!PyCallable_Check(search_function)) {
        PyErr_SetString(PyExc_TypeError, "argument must be callable");
        goto onError;
    }
#ifdef Py_GIL_DISABLED
    PyMutex_Lock(&interp->codecs.search_path_mutex);
#endif
    int ret = PyList_Append(interp->codecs.search_path, search_function);
#ifdef Py_GIL_DISABLED
    PyMutex_Unlock(&interp->codecs.search_path_mutex);
#endif
    return ret;

 onError:
    return -1;
}

int
PyCodec_Unregister(PyObject *search_function)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    if (interp->codecs.initialized != 1) {
        /* Do nothing if codecs state was cleared (only possible during
           interpreter shutdown). */
        return 0;
    }

    PyObject *codec_search_path = interp->codecs.search_path;
    assert(PyList_CheckExact(codec_search_path));
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(codec_search_path); i++) {
#ifdef Py_GIL_DISABLED
        PyMutex_Lock(&interp->codecs.search_path_mutex);
#endif
        PyObject *item = PyList_GetItemRef(codec_search_path, i);
        int ret = 1;
        if (item == search_function) {
            // We hold a reference to the item, so its destructor can't run
            // while we hold search_path_mutex.
            ret = PyList_SetSlice(codec_search_path, i, i+1, NULL);
        }
#ifdef Py_GIL_DISABLED
        PyMutex_Unlock(&interp->codecs.search_path_mutex);
#endif
        Py_DECREF(item);
        if (ret != 1) {
            assert(interp->codecs.search_cache != NULL);
            assert(PyDict_CheckExact(interp->codecs.search_cache));
            PyDict_Clear(interp->codecs.search_cache);
            return ret;
        }
    }
    return 0;
}

extern int _Py_normalize_encoding(const char *, char *, size_t);

/* Convert a string to a normalized Python string(decoded from UTF-8): all characters are
   converted to lower case, spaces and hyphens are replaced with underscores. */

static
PyObject *normalizestring(const char *string)
{
    size_t len = strlen(string);
    char *encoding;
    PyObject *v;

    if (len > PY_SSIZE_T_MAX) {
        PyErr_SetString(PyExc_OverflowError, "string is too large");
        return NULL;
    }

    encoding = PyMem_Malloc(len + 1);
    if (encoding == NULL)
        return PyErr_NoMemory();

    if (!_Py_normalize_encoding(string, encoding, len + 1))
    {
        PyErr_SetString(PyExc_RuntimeError, "_Py_normalize_encoding() failed");
        PyMem_Free(encoding);
        return NULL;
    }

    v = PyUnicode_FromString(encoding);
    PyMem_Free(encoding);
    return v;
}

/* Lookup the given encoding and return a tuple providing the codec
   facilities.

   The encoding string is looked up converted to all lower-case
   characters. This makes encodings looked up through this mechanism
   effectively case-insensitive.

   If no codec is found, a LookupError is set and NULL returned.

   As side effect, this tries to load the encodings package, if not
   yet done. This is part of the lazy load strategy for the encodings
   package.

*/

PyObject *_PyCodec_Lookup(const char *encoding)
{
    if (encoding == NULL) {
        PyErr_BadArgument();
        return NULL;
    }

    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->codecs.initialized);

    /* Convert the encoding to a normalized Python string: all
       characters are converted to lower case, spaces and hyphens are
       replaced with underscores. */
    PyObject *v = normalizestring(encoding);
    if (v == NULL) {
        return NULL;
    }

    /* Intern the string. We'll make it immortal later if lookup succeeds. */
    _PyUnicode_InternMortal(interp, &v);

    /* First, try to lookup the name in the registry dictionary */
    PyObject *result;
    if (PyDict_GetItemRef(interp->codecs.search_cache, v, &result) < 0) {
        goto onError;
    }
    if (result != NULL) {
        Py_DECREF(v);
        return result;
    }

    /* Next, scan the search functions in order of registration */
    const Py_ssize_t len = PyList_Size(interp->codecs.search_path);
    if (len < 0)
        goto onError;
    if (len == 0) {
        PyErr_SetString(PyExc_LookupError,
                        "no codec search functions registered: "
                        "can't find encoding");
        goto onError;
    }

    Py_ssize_t i;
    for (i = 0; i < len; i++) {
        PyObject *func;

        func = PyList_GetItemRef(interp->codecs.search_path, i);
        if (func == NULL)
            goto onError;
        result = PyObject_CallOneArg(func, v);
        Py_DECREF(func);
        if (result == NULL)
            goto onError;
        if (result == Py_None) {
            Py_CLEAR(result);
            continue;
        }
        if (!PyTuple_Check(result) || PyTuple_GET_SIZE(result) != 4) {
            PyErr_SetString(PyExc_TypeError,
                            "codec search functions must return 4-tuples");
            Py_DECREF(result);
            goto onError;
        }
        break;
    }
    if (result == NULL) {
        /* XXX Perhaps we should cache misses too ? */
        PyErr_Format(PyExc_LookupError,
                     "unknown encoding: %s", encoding);
        goto onError;
    }

    _PyUnicode_InternImmortal(interp, &v);

    /* Cache and return the result */
    if (PyDict_SetItem(interp->codecs.search_cache, v, result) < 0) {
        Py_DECREF(result);
        goto onError;
    }
    Py_DECREF(v);
    return result;

 onError:
    Py_DECREF(v);
    return NULL;
}

/* Codec registry encoding check API. */

int PyCodec_KnownEncoding(const char *encoding)
{
    PyObject *codecs;

    codecs = _PyCodec_Lookup(encoding);
    if (!codecs) {
        PyErr_Clear();
        return 0;
    }
    else {
        Py_DECREF(codecs);
        return 1;
    }
}

static
PyObject *args_tuple(PyObject *object,
                     const char *errors)
{
    PyObject *args;

    args = PyTuple_New(1 + (errors != NULL));
    if (args == NULL)
        return NULL;
    PyTuple_SET_ITEM(args, 0, Py_NewRef(object));
    if (errors) {
        PyObject *v;

        v = PyUnicode_FromString(errors);
        if (v == NULL) {
            Py_DECREF(args);
            return NULL;
        }
        PyTuple_SET_ITEM(args, 1, v);
    }
    return args;
}

/* Helper function to get a codec item */

static
PyObject *codec_getitem(const char *encoding, int index)
{
    PyObject *codecs;
    PyObject *v;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
        return NULL;
    v = PyTuple_GET_ITEM(codecs, index);
    Py_DECREF(codecs);
    return Py_NewRef(v);
}

/* Helper functions to create an incremental codec. */
static
PyObject *codec_makeincrementalcodec(PyObject *codec_info,
                                     const char *errors,
                                     const char *attrname)
{
    PyObject *ret, *inccodec;

    inccodec = PyObject_GetAttrString(codec_info, attrname);
    if (inccodec == NULL)
        return NULL;
    if (errors)
        ret = PyObject_CallFunction(inccodec, "s", errors);
    else
        ret = _PyObject_CallNoArgs(inccodec);
    Py_DECREF(inccodec);
    return ret;
}

static
PyObject *codec_getincrementalcodec(const char *encoding,
                                    const char *errors,
                                    const char *attrname)
{
    PyObject *codec_info, *ret;

    codec_info = _PyCodec_Lookup(encoding);
    if (codec_info == NULL)
        return NULL;
    ret = codec_makeincrementalcodec(codec_info, errors, attrname);
    Py_DECREF(codec_info);
    return ret;
}

/* Helper function to create a stream codec. */

static
PyObject *codec_getstreamcodec(const char *encoding,
                               PyObject *stream,
                               const char *errors,
                               const int index)
{
    PyObject *codecs, *streamcodec, *codeccls;

    codecs = _PyCodec_Lookup(encoding);
    if (codecs == NULL)
        return NULL;

    codeccls = PyTuple_GET_ITEM(codecs, index);
    if (errors != NULL)
        streamcodec = PyObject_CallFunction(codeccls, "Os", stream, errors);
    else
        streamcodec = PyObject_CallOneArg(codeccls, stream);
    Py_DECREF(codecs);
    return streamcodec;
}

/* Helpers to work with the result of _PyCodec_Lookup

 */
PyObject *_PyCodecInfo_GetIncrementalDecoder(PyObject *codec_info,
                                             const char *errors)
{
    return codec_makeincrementalcodec(codec_info, errors,
                                      "incrementaldecoder");
}

PyObject *_PyCodecInfo_GetIncrementalEncoder(PyObject *codec_info,
                                             const char *errors)
{
    return codec_makeincrementalcodec(codec_info, errors,
                                      "incrementalencoder");
}


/* Convenience APIs to query the Codec registry.

   All APIs return a codec object with incremented refcount.

 */

PyObject *PyCodec_Encoder(const char *encoding)
{
    return codec_getitem(encoding, 0);
}

PyObject *PyCodec_Decoder(const char *encoding)
{
    return codec_getitem(encoding, 1);
}

PyObject *PyCodec_IncrementalEncoder(const char *encoding,
                                     const char *errors)
{
    return codec_getincrementalcodec(encoding, errors, "incrementalencoder");
}

PyObject *PyCodec_IncrementalDecoder(const char *encoding,
                                     const char *errors)
{
    return codec_getincrementalcodec(encoding, errors, "incrementaldecoder");
}

PyObject *PyCodec_StreamReader(const char *encoding,
                               PyObject *stream,
                               const char *errors)
{
    return codec_getstreamcodec(encoding, stream, errors, 2);
}

PyObject *PyCodec_StreamWriter(const char *encoding,
                               PyObject *stream,
                               const char *errors)
{
    return codec_getstreamcodec(encoding, stream, errors, 3);
}

/* Encode an object (e.g. a Unicode object) using the given encoding
   and return the resulting encoded object (usually a Python string).

   errors is passed to the encoder factory as argument if non-NULL. */

static PyObject *
_PyCodec_EncodeInternal(PyObject *object,
                        PyObject *encoder,
                        const char *encoding,
                        const char *errors)
{
    PyObject *args = NULL, *result = NULL;
    PyObject *v = NULL;

    args = args_tuple(object, errors);
    if (args == NULL)
        goto onError;

    result = PyObject_Call(encoder, args, NULL);
    if (result == NULL) {
        _PyErr_FormatNote("%s with '%s' codec failed", "encoding", encoding);
        goto onError;
    }

    if (!PyTuple_Check(result) ||
        PyTuple_GET_SIZE(result) != 2) {
        PyErr_SetString(PyExc_TypeError,
                        "encoder must return a tuple (object, integer)");
        goto onError;
    }
    v = Py_NewRef(PyTuple_GET_ITEM(result,0));
    /* We don't check or use the second (integer) entry. */

    Py_DECREF(args);
    Py_DECREF(encoder);
    Py_DECREF(result);
    return v;

 onError:
    Py_XDECREF(result);
    Py_XDECREF(args);
    Py_XDECREF(encoder);
    return NULL;
}

/* Decode an object (usually a Python string) using the given encoding
   and return an equivalent object (e.g. a Unicode object).

   errors is passed to the decoder factory as argument if non-NULL. */

static PyObject *
_PyCodec_DecodeInternal(PyObject *object,
                        PyObject *decoder,
                        const char *encoding,
                        const char *errors)
{
    PyObject *args = NULL, *result = NULL;
    PyObject *v;

    args = args_tuple(object, errors);
    if (args == NULL)
        goto onError;

    result = PyObject_Call(decoder, args, NULL);
    if (result == NULL) {
        _PyErr_FormatNote("%s with '%s' codec failed", "decoding", encoding);
        goto onError;
    }
    if (!PyTuple_Check(result) ||
        PyTuple_GET_SIZE(result) != 2) {
        PyErr_SetString(PyExc_TypeError,
                        "decoder must return a tuple (object,integer)");
        goto onError;
    }
    v = Py_NewRef(PyTuple_GET_ITEM(result,0));
    /* We don't check or use the second (integer) entry. */

    Py_DECREF(args);
    Py_DECREF(decoder);
    Py_DECREF(result);
    return v;

 onError:
    Py_XDECREF(args);
    Py_XDECREF(decoder);
    Py_XDECREF(result);
    return NULL;
}

/* Generic encoding/decoding API */
PyObject *PyCodec_Encode(PyObject *object,
                         const char *encoding,
                         const char *errors)
{
    PyObject *encoder;

    encoder = PyCodec_Encoder(encoding);
    if (encoder == NULL)
        return NULL;

    return _PyCodec_EncodeInternal(object, encoder, encoding, errors);
}

PyObject *PyCodec_Decode(PyObject *object,
                         const char *encoding,
                         const char *errors)
{
    PyObject *decoder;

    decoder = PyCodec_Decoder(encoding);
    if (decoder == NULL)
        return NULL;

    return _PyCodec_DecodeInternal(object, decoder, encoding, errors);
}

/* Text encoding/decoding API */
PyObject * _PyCodec_LookupTextEncoding(const char *encoding,
                                       const char *alternate_command)
{
    PyObject *codec;
    PyObject *attr;
    int is_text_codec;

    codec = _PyCodec_Lookup(encoding);
    if (codec == NULL)
        return NULL;

    /* Backwards compatibility: assume any raw tuple describes a text
     * encoding, and the same for anything lacking the private
     * attribute.
     */
    if (!PyTuple_CheckExact(codec)) {
        if (PyObject_GetOptionalAttr(codec, &_Py_ID(_is_text_encoding), &attr) < 0) {
            Py_DECREF(codec);
            return NULL;
        }
        if (attr != NULL) {
            is_text_codec = PyObject_IsTrue(attr);
            Py_DECREF(attr);
            if (is_text_codec <= 0) {
                Py_DECREF(codec);
                if (!is_text_codec) {
                    if (alternate_command != NULL) {
                        PyErr_Format(PyExc_LookupError,
                                     "'%.400s' is not a text encoding; "
                                     "use %s to handle arbitrary codecs",
                                     encoding, alternate_command);
                    }
                    else {
                        PyErr_Format(PyExc_LookupError,
                                     "'%.400s' is not a text encoding",
                                     encoding);
                    }
                }
                return NULL;
            }
        }
    }

    /* This appears to be a valid text encoding */
    return codec;
}


static
PyObject *codec_getitem_checked(const char *encoding,
                                const char *alternate_command,
                                int index)
{
    PyObject *codec;
    PyObject *v;

    codec = _PyCodec_LookupTextEncoding(encoding, alternate_command);
    if (codec == NULL)
        return NULL;

    v = Py_NewRef(PyTuple_GET_ITEM(codec, index));
    Py_DECREF(codec);
    return v;
}

static PyObject * _PyCodec_TextEncoder(const char *encoding)
{
    return codec_getitem_checked(encoding, "codecs.encode()", 0);
}

static PyObject * _PyCodec_TextDecoder(const char *encoding)
{
    return codec_getitem_checked(encoding, "codecs.decode()", 1);
}

PyObject *_PyCodec_EncodeText(PyObject *object,
                              const char *encoding,
                              const char *errors)
{
    PyObject *encoder;

    encoder = _PyCodec_TextEncoder(encoding);
    if (encoder == NULL)
        return NULL;

    return _PyCodec_EncodeInternal(object, encoder, encoding, errors);
}

PyObject *_PyCodec_DecodeText(PyObject *object,
                              const char *encoding,
                              const char *errors)
{
    PyObject *decoder;

    decoder = _PyCodec_TextDecoder(encoding);
    if (decoder == NULL)
        return NULL;

    return _PyCodec_DecodeInternal(object, decoder, encoding, errors);
}

/* Register the error handling callback function error under the name
   name. This function will be called by the codec when it encounters
   an unencodable characters/undecodable bytes and doesn't know the
   callback name, when name is specified as the error parameter
   in the call to the encode/decode function.
   Return 0 on success, -1 on error */
int PyCodec_RegisterError(const char *name, PyObject *error)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->codecs.initialized);
    if (!PyCallable_Check(error)) {
        PyErr_SetString(PyExc_TypeError, "handler must be callable");
        return -1;
    }
    return PyDict_SetItemString(interp->codecs.error_registry,
                                name, error);
}

int _PyCodec_UnregisterError(const char *name)
{
    for (size_t i = 0; i < Py_ARRAY_LENGTH(codecs_builtin_error_handlers); ++i) {
        if (strcmp(name, codecs_builtin_error_handlers[i]) == 0) {
            PyErr_Format(PyExc_ValueError,
                         "cannot un-register built-in error handler '%s'", name);
            return -1;
        }
    }
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->codecs.initialized);
    return PyDict_PopString(interp->codecs.error_registry, name, NULL);
}

/* Lookup the error handling callback function registered under the
   name error. As a special case NULL can be passed, in which case
   the error handling callback for strict encoding will be returned. */
PyObject *PyCodec_LookupError(const char *name)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    assert(interp->codecs.initialized);

    if (name==NULL)
        name = "strict";
    PyObject *handler;
    if (PyDict_GetItemStringRef(interp->codecs.error_registry, name, &handler) < 0) {
        return NULL;
    }
    if (handler == NULL) {
        PyErr_Format(PyExc_LookupError, "unknown error handler name '%.400s'", name);
        return NULL;
    }
    return handler;
}


static inline void
wrong_exception_type(PyObject *exc)
{
    PyErr_Format(PyExc_TypeError,
                 "don't know how to handle %T in error callback", exc);
}


#define _PyIsUnicodeEncodeError(EXC)    \
    PyObject_TypeCheck(EXC, (PyTypeObject *)PyExc_UnicodeEncodeError)
#define _PyIsUnicodeDecodeError(EXC)    \
    PyObject_TypeCheck(EXC, (PyTypeObject *)PyExc_UnicodeDecodeError)
#define _PyIsUnicodeTranslateError(EXC) \
    PyObject_TypeCheck(EXC, (PyTypeObject *)PyExc_UnicodeTranslateError)


// --- codecs handlers: utilities ---------------------------------------------

/*
 * Return the number of characters (including special prefixes)
 * needed to represent 'ch' by codec_handler_write_unicode_hex().
 */
static inline Py_ssize_t
codec_handler_unicode_hex_width(Py_UCS4 ch)
{
    if (ch >= 0x10000) {
        // format: '\\' + 'U' + 8 hex digits
        return 1 + 1 + 8;
    }
    else if (ch >= 0x100) {
        // format: '\\' + 'u' + 4 hex digits
        return 1 + 1 + 4;
    }
    else {
        // format: '\\' + 'x' + 2 hex digits
        return 1 + 1 + 2;
    }
}


/*
 * Write the hexadecimal representation of 'ch' to the buffer pointed by 'p'
 * using 2, 4, or 8 characters prefixed by '\x', '\u', or '\U' respectively.
 */
static inline void
codec_handler_write_unicode_hex(Py_UCS1 **p, Py_UCS4 ch)
{
    *(*p)++ = '\\';
    if (ch >= 0x10000) {
        *(*p)++ = 'U';
        *(*p)++ = Py_hexdigits[(ch >> 28) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 24) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 20) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 16) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 12) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 8) & 0xf];
    }
    else if (ch >= 0x100) {
        *(*p)++ = 'u';
        *(*p)++ = Py_hexdigits[(ch >> 12) & 0xf];
        *(*p)++ = Py_hexdigits[(ch >> 8) & 0xf];
    }
    else {
        *(*p)++ = 'x';
    }
    *(*p)++ = Py_hexdigits[(ch >> 4) & 0xf];
    *(*p)++ = Py_hexdigits[ch & 0xf];
}


/*
 * Determine the number of digits for a decimal representation of Unicode
 * codepoint 'ch' (by design, Unicode codepoints are limited to 7 digits).
 */
static inline int
n_decimal_digits_for_codepoint(Py_UCS4 ch)
{
    if (ch < 10) return 1;
    if (ch < 100) return 2;
    if (ch < 1000) return 3;
    if (ch < 10000) return 4;
    if (ch < 100000) return 5;
    if (ch < 1000000) return 6;
    if (ch < 10000000) return 7;
    // Unicode codepoints are limited to 1114111 (7 decimal digits)
    Py_UNREACHABLE();
}


/*
 * Create a Unicode string containing 'count' copies of the official
 * Unicode REPLACEMENT CHARACTER (0xFFFD).
 */
static PyObject *
codec_handler_unicode_replacement_character(Py_ssize_t count)
{
    PyObject *res = PyUnicode_New(count, Py_UNICODE_REPLACEMENT_CHARACTER);
    if (res == NULL) {
        return NULL;
    }
    assert(count == 0 || PyUnicode_KIND(res) == PyUnicode_2BYTE_KIND);
    Py_UCS2 *outp = PyUnicode_2BYTE_DATA(res);
    for (Py_ssize_t i = 0; i < count; ++i) {
        outp[i] = Py_UNICODE_REPLACEMENT_CHARACTER;
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    return res;
}


// --- handler: 'strict' ------------------------------------------------------

PyObject *PyCodec_StrictErrors(PyObject *exc)
{
    if (PyExceptionInstance_Check(exc)) {
        PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
    }
    else {
        PyErr_SetString(PyExc_TypeError, "codec must pass exception instance");
    }
    return NULL;
}


// --- handler: 'ignore' ------------------------------------------------------

static PyObject *
_PyCodec_IgnoreError(PyObject *exc, int as_bytes)
{
    Py_ssize_t end;
    if (_PyUnicodeError_GetParams(exc, NULL, NULL, NULL,
                                  &end, NULL, as_bytes) < 0)
    {
        return NULL;
    }
    return Py_BuildValue("(Nn)", Py_GetConstant(Py_CONSTANT_EMPTY_STR), end);
}


PyObject *PyCodec_IgnoreErrors(PyObject *exc)
{
    if (_PyIsUnicodeEncodeError(exc) || _PyIsUnicodeTranslateError(exc)) {
        return _PyCodec_IgnoreError(exc, false);
    }
    else if (_PyIsUnicodeDecodeError(exc)) {
        return _PyCodec_IgnoreError(exc, true);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


// --- handler: 'replace' -----------------------------------------------------

static PyObject *
_PyCodec_ReplaceUnicodeEncodeError(PyObject *exc)
{
    Py_ssize_t start, end, slen;
    if (_PyUnicodeError_GetParams(exc, NULL, NULL,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }
    PyObject *res = PyUnicode_New(slen, '?');
    if (res == NULL) {
        return NULL;
    }
    assert(PyUnicode_KIND(res) == PyUnicode_1BYTE_KIND);
    Py_UCS1 *outp = PyUnicode_1BYTE_DATA(res);
    memset(outp, '?', sizeof(Py_UCS1) * slen);
    assert(_PyUnicode_CheckConsistency(res, 1));
    return Py_BuildValue("(Nn)", res, end);
}


static PyObject *
_PyCodec_ReplaceUnicodeDecodeError(PyObject *exc)
{
    Py_ssize_t end;
    if (PyUnicodeDecodeError_GetEnd(exc, &end) < 0) {
        return NULL;
    }
    PyObject *res = codec_handler_unicode_replacement_character(1);
    if (res == NULL) {
        return NULL;
    }
    return Py_BuildValue("(Nn)", res, end);
}


static PyObject *
_PyCodec_ReplaceUnicodeTranslateError(PyObject *exc)
{
    Py_ssize_t start, end, slen;
    if (_PyUnicodeError_GetParams(exc, NULL, NULL,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }
    PyObject *res = codec_handler_unicode_replacement_character(slen);
    if (res == NULL) {
        return NULL;
    }
    return Py_BuildValue("(Nn)", res, end);
}


PyObject *PyCodec_ReplaceErrors(PyObject *exc)
{
    if (_PyIsUnicodeEncodeError(exc)) {
        return _PyCodec_ReplaceUnicodeEncodeError(exc);
    }
    else if (_PyIsUnicodeDecodeError(exc)) {
        return _PyCodec_ReplaceUnicodeDecodeError(exc);
    }
    else if (_PyIsUnicodeTranslateError(exc)) {
        return _PyCodec_ReplaceUnicodeTranslateError(exc);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


// --- handler: 'xmlcharrefreplace' -------------------------------------------

PyObject *PyCodec_XMLCharRefReplaceErrors(PyObject *exc)
{
    if (!_PyIsUnicodeEncodeError(exc)) {
        wrong_exception_type(exc);
        return NULL;
    }

    PyObject *obj;
    Py_ssize_t objlen, start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, &objlen,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }

    // The number of characters that each character 'ch' contributes
    // in the result is 2 + k + 1, where k = min{t >= 1 | 10^t > ch}
    // and will be formatted as "&#" + DIGITS + ";". Since the Unicode
    // range is below 10^7, each "block" requires at most 2 + 7 + 1
    // characters.
    if (slen > PY_SSIZE_T_MAX / (2 + 7 + 1)) {
        end = start + PY_SSIZE_T_MAX / (2 + 7 + 1);
        end = Py_MIN(end, objlen);
        slen = Py_MAX(0, end - start);
    }

    Py_ssize_t ressize = 0;
    for (Py_ssize_t i = start; i < end; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(obj, i);
        int k = n_decimal_digits_for_codepoint(ch);
        assert(k != 0);
        assert(k <= 7);
        ressize += 2 + k + 1;
    }

    /* allocate replacement */
    PyObject *res = PyUnicode_New(ressize, 127);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }
    Py_UCS1 *outp = PyUnicode_1BYTE_DATA(res);
    /* generate replacement */
    for (Py_ssize_t i = start; i < end; ++i) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(obj, i);
        /*
         * Write the decimal representation of 'ch' to the buffer pointed by 'p'
         * using at most 7 characters prefixed by '&#' and suffixed by ';'.
         */
        *outp++ = '&';
        *outp++ = '#';
        Py_UCS1 *digit_end = outp + n_decimal_digits_for_codepoint(ch);
        for (Py_UCS1 *p_digit = digit_end - 1; p_digit >= outp; --p_digit) {
            *p_digit = '0' + (ch % 10);
            ch /= 10;
        }
        assert(ch == 0);
        outp = digit_end;
        *outp++ = ';';
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    PyObject *restuple = Py_BuildValue("(Nn)", res, end);
    Py_DECREF(obj);
    return restuple;
}


// --- handler: 'backslashreplace' --------------------------------------------

static PyObject *
_PyCodec_BackslashReplaceUnicodeEncodeError(PyObject *exc)
{
    PyObject *obj;
    Py_ssize_t objlen, start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, &objlen,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }

    // The number of characters that each character 'ch' contributes
    // in the result is 1 + 1 + k, where k >= min{t >= 1 | 16^t > ch}
    // and will be formatted as "\\" + ('U'|'u'|'x') + HEXDIGITS,
    // where the number of hexdigits is either 2, 4, or 8 (not 6).
    // Since the Unicode range is below 10^7, we choose k = 8 whence
    // each "block" requires at most 1 + 1 + 8 characters.
    if (slen > PY_SSIZE_T_MAX / (1 + 1 + 8)) {
        end = start + PY_SSIZE_T_MAX / (1 + 1 + 8);
        end = Py_MIN(end, objlen);
        slen = Py_MAX(0, end - start);
    }

    Py_ssize_t ressize = 0;
    for (Py_ssize_t i = start; i < end; ++i) {
        Py_UCS4 c = PyUnicode_READ_CHAR(obj, i);
        ressize += codec_handler_unicode_hex_width(c);
    }
    PyObject *res = PyUnicode_New(ressize, 127);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }
    Py_UCS1 *outp = PyUnicode_1BYTE_DATA(res);
    for (Py_ssize_t i = start; i < end; ++i) {
        Py_UCS4 c = PyUnicode_READ_CHAR(obj, i);
        codec_handler_write_unicode_hex(&outp, c);
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    Py_DECREF(obj);
    return Py_BuildValue("(Nn)", res, end);
}


static PyObject *
_PyCodec_BackslashReplaceUnicodeDecodeError(PyObject *exc)
{
    PyObject *obj;
    Py_ssize_t objlen, start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, &objlen,
                                  &start, &end, &slen, true) < 0)
    {
        return NULL;
    }

    PyObject *res = PyUnicode_New(4 * slen, 127);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    Py_UCS1 *outp = PyUnicode_1BYTE_DATA(res);
    const unsigned char *p = (const unsigned char *)PyBytes_AS_STRING(obj);
    for (Py_ssize_t i = start; i < end; i++, outp += 4) {
        const unsigned char ch = p[i];
        outp[0] = '\\';
        outp[1] = 'x';
        outp[2] = Py_hexdigits[(ch >> 4) & 0xf];
        outp[3] = Py_hexdigits[ch & 0xf];
    }
    assert(_PyUnicode_CheckConsistency(res, 1));
    Py_DECREF(obj);
    return Py_BuildValue("(Nn)", res, end);
}


static inline PyObject *
_PyCodec_BackslashReplaceUnicodeTranslateError(PyObject *exc)
{
    // Same implementation as for UnicodeEncodeError objects.
    return _PyCodec_BackslashReplaceUnicodeEncodeError(exc);
}


PyObject *PyCodec_BackslashReplaceErrors(PyObject *exc)
{
    if (_PyIsUnicodeEncodeError(exc)) {
        return _PyCodec_BackslashReplaceUnicodeEncodeError(exc);
    }
    else if (_PyIsUnicodeDecodeError(exc)) {
        return _PyCodec_BackslashReplaceUnicodeDecodeError(exc);
    }
    else if (_PyIsUnicodeTranslateError(exc)) {
        return _PyCodec_BackslashReplaceUnicodeTranslateError(exc);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


// --- handler: 'namereplace' -------------------------------------------------

PyObject *PyCodec_NameReplaceErrors(PyObject *exc)
{
    if (!_PyIsUnicodeEncodeError(exc)) {
        wrong_exception_type(exc);
        return NULL;
    }

    _PyUnicode_Name_CAPI *ucnhash_capi = _PyUnicode_GetNameCAPI();
    if (ucnhash_capi == NULL) {
        return NULL;
    }

    PyObject *obj;
    Py_ssize_t start, end;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, NULL,
                                  &start, &end, NULL, false) < 0)
    {
        return NULL;
    }

    char buffer[256]; /* NAME_MAXLEN in unicodename_db.h */
    Py_ssize_t imax = start, ressize = 0, replsize;
    for (; imax < end; ++imax) {
        Py_UCS4 c = PyUnicode_READ_CHAR(obj, imax);
        if (ucnhash_capi->getname(c, buffer, sizeof(buffer), 1)) {
            // If 'c' is recognized by getname(), the corresponding replacement
            // is '\\' + 'N' + '{' + NAME + '}', i.e. 1 + 1 + 1 + len(NAME) + 1
            // characters. Failures of getname() are ignored by the handler.
            replsize = 1 + 1 + 1 + strlen(buffer) + 1;
        }
        else {
            replsize = codec_handler_unicode_hex_width(c);
        }
        if (ressize > PY_SSIZE_T_MAX - replsize) {
            break;
        }
        ressize += replsize;
    }

    PyObject *res = PyUnicode_New(ressize, 127);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    Py_UCS1 *outp = PyUnicode_1BYTE_DATA(res);
    for (Py_ssize_t i = start; i < imax; ++i) {
        Py_UCS4 c = PyUnicode_READ_CHAR(obj, i);
        if (ucnhash_capi->getname(c, buffer, sizeof(buffer), 1)) {
            *outp++ = '\\';
            *outp++ = 'N';
            *outp++ = '{';
            (void)strcpy((char *)outp, buffer);
            outp += strlen(buffer);
            *outp++ = '}';
        }
        else {
            codec_handler_write_unicode_hex(&outp, c);
        }
    }

    assert(outp == PyUnicode_1BYTE_DATA(res) + ressize);
    assert(_PyUnicode_CheckConsistency(res, 1));
    PyObject *restuple = Py_BuildValue("(Nn)", res, imax);
    Py_DECREF(obj);
    return restuple;
}


#define ENC_UNKNOWN     -1
#define ENC_UTF8        0
#define ENC_UTF16BE     1
#define ENC_UTF16LE     2
#define ENC_UTF32BE     3
#define ENC_UTF32LE     4

static int
get_standard_encoding_impl(const char *encoding, int *bytelength)
{
    if (Py_TOLOWER(encoding[0]) == 'u' &&
        Py_TOLOWER(encoding[1]) == 't' &&
        Py_TOLOWER(encoding[2]) == 'f') {
        encoding += 3;
        if (*encoding == '-' || *encoding == '_' )
            encoding++;
        if (encoding[0] == '8' && encoding[1] == '\0') {
            *bytelength = 3;
            return ENC_UTF8;
        }
        else if (encoding[0] == '1' && encoding[1] == '6') {
            encoding += 2;
            *bytelength = 2;
            if (*encoding == '\0') {
#ifdef WORDS_BIGENDIAN
                return ENC_UTF16BE;
#else
                return ENC_UTF16LE;
#endif
            }
            if (*encoding == '-' || *encoding == '_' )
                encoding++;
            if (Py_TOLOWER(encoding[1]) == 'e' && encoding[2] == '\0') {
                if (Py_TOLOWER(encoding[0]) == 'b')
                    return ENC_UTF16BE;
                if (Py_TOLOWER(encoding[0]) == 'l')
                    return ENC_UTF16LE;
            }
        }
        else if (encoding[0] == '3' && encoding[1] == '2') {
            encoding += 2;
            *bytelength = 4;
            if (*encoding == '\0') {
#ifdef WORDS_BIGENDIAN
                return ENC_UTF32BE;
#else
                return ENC_UTF32LE;
#endif
            }
            if (*encoding == '-' || *encoding == '_' )
                encoding++;
            if (Py_TOLOWER(encoding[1]) == 'e' && encoding[2] == '\0') {
                if (Py_TOLOWER(encoding[0]) == 'b')
                    return ENC_UTF32BE;
                if (Py_TOLOWER(encoding[0]) == 'l')
                    return ENC_UTF32LE;
            }
        }
    }
    else if (strcmp(encoding, "CP_UTF8") == 0) {
        *bytelength = 3;
        return ENC_UTF8;
    }
    return ENC_UNKNOWN;
}


static int
get_standard_encoding(PyObject *encoding, int *code, int *bytelength)
{
    const char *encoding_cstr = PyUnicode_AsUTF8(encoding);
    if (encoding_cstr == NULL) {
        return -1;
    }
    *code = get_standard_encoding_impl(encoding_cstr, bytelength);
    return 0;
}


// --- handler: 'surrogatepass' -----------------------------------------------

static PyObject *
_PyCodec_SurrogatePassUnicodeEncodeError(PyObject *exc)
{
    PyObject *encoding = PyUnicodeEncodeError_GetEncoding(exc);
    if (encoding == NULL) {
        return NULL;
    }
    int code, bytelength;
    int rc = get_standard_encoding(encoding, &code, &bytelength);
    Py_DECREF(encoding);
    if (rc < 0) {
        return NULL;
    }
    if (code == ENC_UNKNOWN) {
        goto bail;
    }

    PyObject *obj;
    Py_ssize_t objlen, start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, &objlen,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }

    if (slen > PY_SSIZE_T_MAX / bytelength) {
        end = start + PY_SSIZE_T_MAX / bytelength;
        end = Py_MIN(end, objlen);
        slen = Py_MAX(0, end - start);
    }

    PyObject *res = PyBytes_FromStringAndSize(NULL, bytelength * slen);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    unsigned char *outp = (unsigned char *)PyBytes_AsString(res);
    for (Py_ssize_t i = start; i < end; i++) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(obj, i);
        if (!Py_UNICODE_IS_SURROGATE(ch)) {
            /* Not a surrogate, fail with original exception */
            Py_DECREF(obj);
            Py_DECREF(res);
            goto bail;
        }
        switch (code) {
            case ENC_UTF8: {
                *outp++ = (unsigned char)(0xe0 | (ch >> 12));
                *outp++ = (unsigned char)(0x80 | ((ch >> 6) & 0x3f));
                *outp++ = (unsigned char)(0x80 | (ch & 0x3f));
                break;
            }
            case ENC_UTF16LE: {
                *outp++ = (unsigned char)ch;
                *outp++ = (unsigned char)(ch >> 8);
                break;
            }
            case ENC_UTF16BE: {
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char)ch;
                break;
            }
            case ENC_UTF32LE: {
                *outp++ = (unsigned char)ch;
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char)(ch >> 16);
                *outp++ = (unsigned char)(ch >> 24);
                break;
            }
            case ENC_UTF32BE: {
                *outp++ = (unsigned char)(ch >> 24);
                *outp++ = (unsigned char)(ch >> 16);
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char)ch;
                break;
            }
        }
    }

    Py_DECREF(obj);
    PyObject *restuple = Py_BuildValue("(Nn)", res, end);
    return restuple;

bail:
    PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
    return NULL;
}


static PyObject *
_PyCodec_SurrogatePassUnicodeDecodeError(PyObject *exc)
{
    PyObject *encoding = PyUnicodeDecodeError_GetEncoding(exc);
    if (encoding == NULL) {
        return NULL;
    }
    int code, bytelength;
    int rc = get_standard_encoding(encoding, &code, &bytelength);
    Py_DECREF(encoding);
    if (rc < 0) {
        return NULL;
    }
    if (code == ENC_UNKNOWN) {
        goto bail;
    }

    PyObject *obj;
    Py_ssize_t objlen, start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, &objlen,
                                  &start, &end, &slen, true) < 0)
    {
        return NULL;
    }

    /* Try decoding a single surrogate character. If
       there are more, let the codec call us again. */
    Py_UCS4 ch = 0;
    const unsigned char *p = (const unsigned char *)PyBytes_AS_STRING(obj);
    p += start;

    if (objlen - start >= bytelength) {
        switch (code) {
            case ENC_UTF8: {
                if ((p[0] & 0xf0) == 0xe0 &&
                    (p[1] & 0xc0) == 0x80 &&
                    (p[2] & 0xc0) == 0x80)
                {
                    /* it's a three-byte code */
                    ch = ((p[0] & 0x0f) << 12) +
                         ((p[1] & 0x3f) << 6)  +
                          (p[2] & 0x3f);
                }
                break;
            }
            case ENC_UTF16LE: {
                ch = p[1] << 8 | p[0];
                break;
            }
            case ENC_UTF16BE: {
                ch = p[0] << 8 | p[1];
                break;
            }
            case ENC_UTF32LE: {
                ch = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
                break;
            }
            case ENC_UTF32BE: {
                ch = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
                break;
            }
        }
    }
    Py_DECREF(obj);
    if (!Py_UNICODE_IS_SURROGATE(ch)) {
        goto bail;
    }

    PyObject *res = PyUnicode_FromOrdinal(ch);
    if (res == NULL) {
        return NULL;
    }
    return Py_BuildValue("(Nn)", res, start + bytelength);

bail:
    PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
    return NULL;
}


/* This handler is declared static until someone demonstrates
   a need to call it directly. */
static PyObject *
PyCodec_SurrogatePassErrors(PyObject *exc)
{
    if (_PyIsUnicodeEncodeError(exc)) {
        return _PyCodec_SurrogatePassUnicodeEncodeError(exc);
    }
    else if (_PyIsUnicodeDecodeError(exc)) {
        return _PyCodec_SurrogatePassUnicodeDecodeError(exc);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


// --- handler: 'surrogateescape' ---------------------------------------------

static PyObject *
_PyCodec_SurrogateEscapeUnicodeEncodeError(PyObject *exc)
{
    PyObject *obj;
    Py_ssize_t start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, NULL,
                                  &start, &end, &slen, false) < 0)
    {
        return NULL;
    }

    PyObject *res = PyBytes_FromStringAndSize(NULL, slen);
    if (res == NULL) {
        Py_DECREF(obj);
        return NULL;
    }

    char *outp = PyBytes_AsString(res);
    for (Py_ssize_t i = start; i < end; i++) {
        Py_UCS4 ch = PyUnicode_READ_CHAR(obj, i);
        if (ch < 0xdc80 || ch > 0xdcff) {
            /* Not a UTF-8b surrogate, fail with original exception. */
            Py_DECREF(obj);
            Py_DECREF(res);
            PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
            return NULL;
        }
        *outp++ = ch - 0xdc00;
    }
    Py_DECREF(obj);

    return Py_BuildValue("(Nn)", res, end);
}


static PyObject *
_PyCodec_SurrogateEscapeUnicodeDecodeError(PyObject *exc)
{
    PyObject *obj;
    Py_ssize_t start, end, slen;
    if (_PyUnicodeError_GetParams(exc,
                                  &obj, NULL,
                                  &start, &end, &slen, true) < 0)
    {
        return NULL;
    }

    Py_UCS2 ch[4]; /* decode up to 4 bad bytes. */
    int consumed = 0;
    const unsigned char *p = (const unsigned char *)PyBytes_AS_STRING(obj);
    while (consumed < 4 && consumed < slen) {
        /* Refuse to escape ASCII bytes. */
        if (p[start + consumed] < 128) {
            break;
        }
        ch[consumed] = 0xdc00 + p[start + consumed];
        consumed++;
    }
    Py_DECREF(obj);

    if (consumed == 0) {
        /* Codec complained about ASCII byte. */
        PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
        return NULL;
    }

    PyObject *str = PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, ch, consumed);
    if (str == NULL) {
        return NULL;
    }
    return Py_BuildValue("(Nn)", str, start + consumed);
}


static PyObject *
PyCodec_SurrogateEscapeErrors(PyObject *exc)
{
    if (_PyIsUnicodeEncodeError(exc)) {
        return _PyCodec_SurrogateEscapeUnicodeEncodeError(exc);
    }
    else if (_PyIsUnicodeDecodeError(exc)) {
        return _PyCodec_SurrogateEscapeUnicodeDecodeError(exc);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


// --- Codecs registry handlers -----------------------------------------------

static inline PyObject *
strict_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_StrictErrors(exc);
}


static inline PyObject *
ignore_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_IgnoreErrors(exc);
}


static inline PyObject *
replace_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_ReplaceErrors(exc);
}


static inline PyObject *
xmlcharrefreplace_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_XMLCharRefReplaceErrors(exc);
}


static inline PyObject *
backslashreplace_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_BackslashReplaceErrors(exc);
}


static inline PyObject *
namereplace_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_NameReplaceErrors(exc);
}


static inline PyObject *
surrogatepass_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_SurrogatePassErrors(exc);
}


static inline PyObject *
surrogateescape_errors(PyObject *Py_UNUSED(self), PyObject *exc)
{
    return PyCodec_SurrogateEscapeErrors(exc);
}


PyStatus
_PyCodec_InitRegistry(PyInterpreterState *interp)
{
    static struct {
        const char *name;
        PyMethodDef def;
    } methods[] =
    {
        {
            "strict",
            {
                "strict_errors",
                strict_errors,
                METH_O,
                PyDoc_STR("Implements the 'strict' error handling, which "
                          "raises a UnicodeError on coding errors.")
            }
        },
        {
            "ignore",
            {
                "ignore_errors",
                ignore_errors,
                METH_O,
                PyDoc_STR("Implements the 'ignore' error handling, which "
                          "ignores malformed data and continues.")
            }
        },
        {
            "replace",
            {
                "replace_errors",
                replace_errors,
                METH_O,
                PyDoc_STR("Implements the 'replace' error handling, which "
                          "replaces malformed data with a replacement marker.")
            }
        },
        {
            "xmlcharrefreplace",
            {
                "xmlcharrefreplace_errors",
                xmlcharrefreplace_errors,
                METH_O,
                PyDoc_STR("Implements the 'xmlcharrefreplace' error handling, "
                          "which replaces an unencodable character with the "
                          "appropriate XML character reference.")
            }
        },
        {
            "backslashreplace",
            {
                "backslashreplace_errors",
                backslashreplace_errors,
                METH_O,
                PyDoc_STR("Implements the 'backslashreplace' error handling, "
                          "which replaces malformed data with a backslashed "
                          "escape sequence.")
            }
        },
        {
            "namereplace",
            {
                "namereplace_errors",
                namereplace_errors,
                METH_O,
                PyDoc_STR("Implements the 'namereplace' error handling, "
                          "which replaces an unencodable character with a "
                          "\\N{...} escape sequence.")
            }
        },
        {
            "surrogatepass",
            {
                "surrogatepass",
                surrogatepass_errors,
                METH_O
            }
        },
        {
            "surrogateescape",
            {
                "surrogateescape",
                surrogateescape_errors,
                METH_O
            }
        }
    };
    // ensure that the built-in error handlers' names are kept in sync
    assert(Py_ARRAY_LENGTH(methods) == Py_ARRAY_LENGTH(codecs_builtin_error_handlers));

    assert(interp->codecs.initialized == 0);
    interp->codecs.search_path = PyList_New(0);
    if (interp->codecs.search_path == NULL) {
        return PyStatus_NoMemory();
    }
    interp->codecs.search_cache = PyDict_New();
    if (interp->codecs.search_cache == NULL) {
        return PyStatus_NoMemory();
    }
    interp->codecs.error_registry = PyDict_New();
    if (interp->codecs.error_registry == NULL) {
        return PyStatus_NoMemory();
    }
    for (size_t i = 0; i < Py_ARRAY_LENGTH(methods); ++i) {
        PyObject *func = PyCFunction_NewEx(&methods[i].def, NULL, NULL);
        if (func == NULL) {
            return PyStatus_NoMemory();
        }

        int res = PyDict_SetItemString(interp->codecs.error_registry,
                                       methods[i].name, func);
        Py_DECREF(func);
        if (res < 0) {
            return PyStatus_Error("Failed to insert into codec error registry");
        }
    }

    interp->codecs.initialized = 1;

    // Importing `encodings' will call back into this module to register codec
    // search functions, so this is done after everything else is initialized.
    PyObject *mod = PyImport_ImportModule("encodings");
    if (mod == NULL) {
        return PyStatus_Error("Failed to import encodings module");
    }
    Py_DECREF(mod);

    return PyStatus_Ok();
}

void
_PyCodec_Fini(PyInterpreterState *interp)
{
    Py_CLEAR(interp->codecs.search_path);
    Py_CLEAR(interp->codecs.search_cache);
    Py_CLEAR(interp->codecs.error_registry);
    interp->codecs.initialized = 0;
}
