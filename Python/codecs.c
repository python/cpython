/* ------------------------------------------------------------------------

   Python Codec Registry and support functions

Written by Marc-Andre Lemburg (mal@lemburg.com).

Copyright (c) Corporation for National Research Initiatives.

   ------------------------------------------------------------------------ */

#include "Python.h"
#include "pycore_call.h"          // _PyObject_CallNoArgs()
#include "pycore_interp.h"        // PyInterpreterState.codec_search_path
#include "pycore_lock.h"          // PyMutex
#include "pycore_pyerrors.h"      // _PyErr_FormatNote()
#include "pycore_pystate.h"       // _PyInterpreterState_GET()
#include "pycore_ucnhash.h"       // _PyUnicode_Name_CAPI

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
                if (!is_text_codec)
                    PyErr_Format(PyExc_LookupError,
                                 "'%.400s' is not a text encoding; "
                                 "use %s to handle arbitrary codecs",
                                 encoding, alternate_command);
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

static void wrong_exception_type(PyObject *exc)
{
    PyErr_Format(PyExc_TypeError,
                 "don't know how to handle %.200s in error callback",
                 Py_TYPE(exc)->tp_name);
}

PyObject *PyCodec_StrictErrors(PyObject *exc)
{
    if (PyExceptionInstance_Check(exc))
        PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
    else
        PyErr_SetString(PyExc_TypeError, "codec must pass exception instance");
    return NULL;
}


PyObject *PyCodec_IgnoreErrors(PyObject *exc)
{
    Py_ssize_t end;

    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeDecodeError)) {
        if (PyUnicodeDecodeError_GetEnd(exc, &end))
            return NULL;
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeTranslateError)) {
        if (PyUnicodeTranslateError_GetEnd(exc, &end))
            return NULL;
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
    return Py_BuildValue("(Nn)", PyUnicode_New(0, 0), end);
}


PyObject *PyCodec_ReplaceErrors(PyObject *exc)
{
    Py_ssize_t start, end, i, len;

    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        PyObject *res;
        Py_UCS1 *outp;
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        len = end - start;
        res = PyUnicode_New(len, '?');
        if (res == NULL)
            return NULL;
        assert(PyUnicode_KIND(res) == PyUnicode_1BYTE_KIND);
        outp = PyUnicode_1BYTE_DATA(res);
        for (i = 0; i < len; ++i)
            outp[i] = '?';
        assert(_PyUnicode_CheckConsistency(res, 1));
        return Py_BuildValue("(Nn)", res, end);
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeDecodeError)) {
        if (PyUnicodeDecodeError_GetEnd(exc, &end))
            return NULL;
        return Py_BuildValue("(Cn)",
                             (int)Py_UNICODE_REPLACEMENT_CHARACTER,
                             end);
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeTranslateError)) {
        PyObject *res;
        Py_UCS2 *outp;
        if (PyUnicodeTranslateError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeTranslateError_GetEnd(exc, &end))
            return NULL;
        len = end - start;
        res = PyUnicode_New(len, Py_UNICODE_REPLACEMENT_CHARACTER);
        if (res == NULL)
            return NULL;
        assert(PyUnicode_KIND(res) == PyUnicode_2BYTE_KIND);
        outp = PyUnicode_2BYTE_DATA(res);
        for (i = 0; i < len; i++)
            outp[i] = Py_UNICODE_REPLACEMENT_CHARACTER;
        assert(_PyUnicode_CheckConsistency(res, 1));
        return Py_BuildValue("(Nn)", res, end);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}

PyObject *PyCodec_XMLCharRefReplaceErrors(PyObject *exc)
{
    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        PyObject *restuple;
        PyObject *object;
        Py_ssize_t i;
        Py_ssize_t start;
        Py_ssize_t end;
        PyObject *res;
        Py_UCS1 *outp;
        Py_ssize_t ressize;
        Py_UCS4 ch;
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeEncodeError_GetObject(exc)))
            return NULL;
        if (end - start > PY_SSIZE_T_MAX / (2+7+1))
            end = start + PY_SSIZE_T_MAX / (2+7+1);
        for (i = start, ressize = 0; i < end; ++i) {
            /* object is guaranteed to be "ready" */
            ch = PyUnicode_READ_CHAR(object, i);
            if (ch<10)
                ressize += 2+1+1;
            else if (ch<100)
                ressize += 2+2+1;
            else if (ch<1000)
                ressize += 2+3+1;
            else if (ch<10000)
                ressize += 2+4+1;
            else if (ch<100000)
                ressize += 2+5+1;
            else if (ch<1000000)
                ressize += 2+6+1;
            else
                ressize += 2+7+1;
        }
        /* allocate replacement */
        res = PyUnicode_New(ressize, 127);
        if (res == NULL) {
            Py_DECREF(object);
            return NULL;
        }
        outp = PyUnicode_1BYTE_DATA(res);
        /* generate replacement */
        for (i = start; i < end; ++i) {
            int digits;
            int base;
            ch = PyUnicode_READ_CHAR(object, i);
            *outp++ = '&';
            *outp++ = '#';
            if (ch<10) {
                digits = 1;
                base = 1;
            }
            else if (ch<100) {
                digits = 2;
                base = 10;
            }
            else if (ch<1000) {
                digits = 3;
                base = 100;
            }
            else if (ch<10000) {
                digits = 4;
                base = 1000;
            }
            else if (ch<100000) {
                digits = 5;
                base = 10000;
            }
            else if (ch<1000000) {
                digits = 6;
                base = 100000;
            }
            else {
                digits = 7;
                base = 1000000;
            }
            while (digits-->0) {
                *outp++ = '0' + ch/base;
                ch %= base;
                base /= 10;
            }
            *outp++ = ';';
        }
        assert(_PyUnicode_CheckConsistency(res, 1));
        restuple = Py_BuildValue("(Nn)", res, end);
        Py_DECREF(object);
        return restuple;
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}

PyObject *PyCodec_BackslashReplaceErrors(PyObject *exc)
{
    PyObject *object;
    Py_ssize_t i;
    Py_ssize_t start;
    Py_ssize_t end;
    PyObject *res;
    Py_UCS1 *outp;
    int ressize;
    Py_UCS4 c;

    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeDecodeError)) {
        const unsigned char *p;
        if (PyUnicodeDecodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeDecodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeDecodeError_GetObject(exc)))
            return NULL;
        p = (const unsigned char*)PyBytes_AS_STRING(object);
        res = PyUnicode_New(4 * (end - start), 127);
        if (res == NULL) {
            Py_DECREF(object);
            return NULL;
        }
        outp = PyUnicode_1BYTE_DATA(res);
        for (i = start; i < end; i++, outp += 4) {
            unsigned char c = p[i];
            outp[0] = '\\';
            outp[1] = 'x';
            outp[2] = Py_hexdigits[(c>>4)&0xf];
            outp[3] = Py_hexdigits[c&0xf];
        }

        assert(_PyUnicode_CheckConsistency(res, 1));
        Py_DECREF(object);
        return Py_BuildValue("(Nn)", res, end);
    }
    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeEncodeError_GetObject(exc)))
            return NULL;
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeTranslateError)) {
        if (PyUnicodeTranslateError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeTranslateError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeTranslateError_GetObject(exc)))
            return NULL;
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }

    if (end - start > PY_SSIZE_T_MAX / (1+1+8))
        end = start + PY_SSIZE_T_MAX / (1+1+8);
    for (i = start, ressize = 0; i < end; ++i) {
        /* object is guaranteed to be "ready" */
        c = PyUnicode_READ_CHAR(object, i);
        if (c >= 0x10000) {
            ressize += 1+1+8;
        }
        else if (c >= 0x100) {
            ressize += 1+1+4;
        }
        else
            ressize += 1+1+2;
    }
    res = PyUnicode_New(ressize, 127);
    if (res == NULL) {
        Py_DECREF(object);
        return NULL;
    }
    outp = PyUnicode_1BYTE_DATA(res);
    for (i = start; i < end; ++i) {
        c = PyUnicode_READ_CHAR(object, i);
        *outp++ = '\\';
        if (c >= 0x00010000) {
            *outp++ = 'U';
            *outp++ = Py_hexdigits[(c>>28)&0xf];
            *outp++ = Py_hexdigits[(c>>24)&0xf];
            *outp++ = Py_hexdigits[(c>>20)&0xf];
            *outp++ = Py_hexdigits[(c>>16)&0xf];
            *outp++ = Py_hexdigits[(c>>12)&0xf];
            *outp++ = Py_hexdigits[(c>>8)&0xf];
        }
        else if (c >= 0x100) {
            *outp++ = 'u';
            *outp++ = Py_hexdigits[(c>>12)&0xf];
            *outp++ = Py_hexdigits[(c>>8)&0xf];
        }
        else
            *outp++ = 'x';
        *outp++ = Py_hexdigits[(c>>4)&0xf];
        *outp++ = Py_hexdigits[c&0xf];
    }

    assert(_PyUnicode_CheckConsistency(res, 1));
    Py_DECREF(object);
    return Py_BuildValue("(Nn)", res, end);
}

PyObject *PyCodec_NameReplaceErrors(PyObject *exc)
{
    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        PyObject *restuple;
        PyObject *object;
        Py_ssize_t i;
        Py_ssize_t start;
        Py_ssize_t end;
        PyObject *res;
        Py_UCS1 *outp;
        Py_ssize_t ressize;
        int replsize;
        Py_UCS4 c;
        char buffer[256]; /* NAME_MAXLEN */
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeEncodeError_GetObject(exc)))
            return NULL;
        _PyUnicode_Name_CAPI *ucnhash_capi = _PyUnicode_GetNameCAPI();
        if (ucnhash_capi == NULL) {
            return NULL;
        }
        for (i = start, ressize = 0; i < end; ++i) {
            /* object is guaranteed to be "ready" */
            c = PyUnicode_READ_CHAR(object, i);
            if (ucnhash_capi->getname(c, buffer, sizeof(buffer), 1)) {
                replsize = 1+1+1+(int)strlen(buffer)+1;
            }
            else if (c >= 0x10000) {
                replsize = 1+1+8;
            }
            else if (c >= 0x100) {
                replsize = 1+1+4;
            }
            else
                replsize = 1+1+2;
            if (ressize > PY_SSIZE_T_MAX - replsize)
                break;
            ressize += replsize;
        }
        end = i;
        res = PyUnicode_New(ressize, 127);
        if (res==NULL)
            return NULL;
        for (i = start, outp = PyUnicode_1BYTE_DATA(res);
            i < end; ++i) {
            c = PyUnicode_READ_CHAR(object, i);
            *outp++ = '\\';
            if (ucnhash_capi->getname(c, buffer, sizeof(buffer), 1)) {
                *outp++ = 'N';
                *outp++ = '{';
                strcpy((char *)outp, buffer);
                outp += strlen(buffer);
                *outp++ = '}';
                continue;
            }
            if (c >= 0x00010000) {
                *outp++ = 'U';
                *outp++ = Py_hexdigits[(c>>28)&0xf];
                *outp++ = Py_hexdigits[(c>>24)&0xf];
                *outp++ = Py_hexdigits[(c>>20)&0xf];
                *outp++ = Py_hexdigits[(c>>16)&0xf];
                *outp++ = Py_hexdigits[(c>>12)&0xf];
                *outp++ = Py_hexdigits[(c>>8)&0xf];
            }
            else if (c >= 0x100) {
                *outp++ = 'u';
                *outp++ = Py_hexdigits[(c>>12)&0xf];
                *outp++ = Py_hexdigits[(c>>8)&0xf];
            }
            else
                *outp++ = 'x';
            *outp++ = Py_hexdigits[(c>>4)&0xf];
            *outp++ = Py_hexdigits[c&0xf];
        }

        assert(outp == PyUnicode_1BYTE_DATA(res) + ressize);
        assert(_PyUnicode_CheckConsistency(res, 1));
        restuple = Py_BuildValue("(Nn)", res, end);
        Py_DECREF(object);
        return restuple;
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}

#define ENC_UNKNOWN     -1
#define ENC_UTF8        0
#define ENC_UTF16BE     1
#define ENC_UTF16LE     2
#define ENC_UTF32BE     3
#define ENC_UTF32LE     4

static int
get_standard_encoding(const char *encoding, int *bytelength)
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
    else if (strcmp(encoding, "cp65001") == 0) {
        *bytelength = 3;
        return ENC_UTF8;
    }
    return ENC_UNKNOWN;
}

/* This handler is declared static until someone demonstrates
   a need to call it directly. */
static PyObject *
PyCodec_SurrogatePassErrors(PyObject *exc)
{
    PyObject *restuple;
    PyObject *object;
    PyObject *encode;
    const char *encoding;
    int code;
    int bytelength;
    Py_ssize_t i;
    Py_ssize_t start;
    Py_ssize_t end;
    PyObject *res;

    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        unsigned char *outp;
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeEncodeError_GetObject(exc)))
            return NULL;
        if (!(encode = PyUnicodeEncodeError_GetEncoding(exc))) {
            Py_DECREF(object);
            return NULL;
        }
        if (!(encoding = PyUnicode_AsUTF8(encode))) {
            Py_DECREF(object);
            Py_DECREF(encode);
            return NULL;
        }
        code = get_standard_encoding(encoding, &bytelength);
        Py_DECREF(encode);
        if (code == ENC_UNKNOWN) {
            /* Not supported, fail with original exception */
            PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
            Py_DECREF(object);
            return NULL;
        }

        if (end - start > PY_SSIZE_T_MAX / bytelength)
            end = start + PY_SSIZE_T_MAX / bytelength;
        res = PyBytes_FromStringAndSize(NULL, bytelength*(end-start));
        if (!res) {
            Py_DECREF(object);
            return NULL;
        }
        outp = (unsigned char*)PyBytes_AsString(res);
        for (i = start; i < end; i++) {
            /* object is guaranteed to be "ready" */
            Py_UCS4 ch = PyUnicode_READ_CHAR(object, i);
            if (!Py_UNICODE_IS_SURROGATE(ch)) {
                /* Not a surrogate, fail with original exception */
                PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
                Py_DECREF(res);
                Py_DECREF(object);
                return NULL;
            }
            switch (code) {
            case ENC_UTF8:
                *outp++ = (unsigned char)(0xe0 | (ch >> 12));
                *outp++ = (unsigned char)(0x80 | ((ch >> 6) & 0x3f));
                *outp++ = (unsigned char)(0x80 | (ch & 0x3f));
                break;
            case ENC_UTF16LE:
                *outp++ = (unsigned char) ch;
                *outp++ = (unsigned char)(ch >> 8);
                break;
            case ENC_UTF16BE:
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char) ch;
                break;
            case ENC_UTF32LE:
                *outp++ = (unsigned char) ch;
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char)(ch >> 16);
                *outp++ = (unsigned char)(ch >> 24);
                break;
            case ENC_UTF32BE:
                *outp++ = (unsigned char)(ch >> 24);
                *outp++ = (unsigned char)(ch >> 16);
                *outp++ = (unsigned char)(ch >> 8);
                *outp++ = (unsigned char) ch;
                break;
            }
        }
        restuple = Py_BuildValue("(On)", res, end);
        Py_DECREF(res);
        Py_DECREF(object);
        return restuple;
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeDecodeError)) {
        const unsigned char *p;
        Py_UCS4 ch = 0;
        if (PyUnicodeDecodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeDecodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeDecodeError_GetObject(exc)))
            return NULL;
        p = (const unsigned char*)PyBytes_AS_STRING(object);
        if (!(encode = PyUnicodeDecodeError_GetEncoding(exc))) {
            Py_DECREF(object);
            return NULL;
        }
        if (!(encoding = PyUnicode_AsUTF8(encode))) {
            Py_DECREF(object);
            Py_DECREF(encode);
            return NULL;
        }
        code = get_standard_encoding(encoding, &bytelength);
        Py_DECREF(encode);
        if (code == ENC_UNKNOWN) {
            /* Not supported, fail with original exception */
            PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
            Py_DECREF(object);
            return NULL;
        }

        /* Try decoding a single surrogate character. If
           there are more, let the codec call us again. */
        p += start;
        if (PyBytes_GET_SIZE(object) - start >= bytelength) {
            switch (code) {
            case ENC_UTF8:
                if ((p[0] & 0xf0) == 0xe0 &&
                    (p[1] & 0xc0) == 0x80 &&
                    (p[2] & 0xc0) == 0x80) {
                    /* it's a three-byte code */
                    ch = ((p[0] & 0x0f) << 12) + ((p[1] & 0x3f) << 6) + (p[2] & 0x3f);
                }
                break;
            case ENC_UTF16LE:
                ch = p[1] << 8 | p[0];
                break;
            case ENC_UTF16BE:
                ch = p[0] << 8 | p[1];
                break;
            case ENC_UTF32LE:
                ch = (p[3] << 24) | (p[2] << 16) | (p[1] << 8) | p[0];
                break;
            case ENC_UTF32BE:
                ch = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
                break;
            }
        }

        Py_DECREF(object);
        if (!Py_UNICODE_IS_SURROGATE(ch)) {
            /* it's not a surrogate - fail */
            PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
            return NULL;
        }
        res = PyUnicode_FromOrdinal(ch);
        if (res == NULL)
            return NULL;
        return Py_BuildValue("(Nn)", res, start + bytelength);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}

static PyObject *
PyCodec_SurrogateEscapeErrors(PyObject *exc)
{
    PyObject *restuple;
    PyObject *object;
    Py_ssize_t i;
    Py_ssize_t start;
    Py_ssize_t end;
    PyObject *res;

    if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeEncodeError)) {
        char *outp;
        if (PyUnicodeEncodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeEncodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeEncodeError_GetObject(exc)))
            return NULL;
        res = PyBytes_FromStringAndSize(NULL, end-start);
        if (!res) {
            Py_DECREF(object);
            return NULL;
        }
        outp = PyBytes_AsString(res);
        for (i = start; i < end; i++) {
            /* object is guaranteed to be "ready" */
            Py_UCS4 ch = PyUnicode_READ_CHAR(object, i);
            if (ch < 0xdc80 || ch > 0xdcff) {
                /* Not a UTF-8b surrogate, fail with original exception */
                PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
                Py_DECREF(res);
                Py_DECREF(object);
                return NULL;
            }
            *outp++ = ch - 0xdc00;
        }
        restuple = Py_BuildValue("(On)", res, end);
        Py_DECREF(res);
        Py_DECREF(object);
        return restuple;
    }
    else if (PyObject_TypeCheck(exc, (PyTypeObject *)PyExc_UnicodeDecodeError)) {
        PyObject *str;
        const unsigned char *p;
        Py_UCS2 ch[4]; /* decode up to 4 bad bytes. */
        int consumed = 0;
        if (PyUnicodeDecodeError_GetStart(exc, &start))
            return NULL;
        if (PyUnicodeDecodeError_GetEnd(exc, &end))
            return NULL;
        if (!(object = PyUnicodeDecodeError_GetObject(exc)))
            return NULL;
        p = (const unsigned char*)PyBytes_AS_STRING(object);
        while (consumed < 4 && consumed < end-start) {
            /* Refuse to escape ASCII bytes. */
            if (p[start+consumed] < 128)
                break;
            ch[consumed] = 0xdc00 + p[start+consumed];
            consumed++;
        }
        Py_DECREF(object);
        if (!consumed) {
            /* codec complained about ASCII byte. */
            PyErr_SetObject(PyExceptionInstance_Class(exc), exc);
            return NULL;
        }
        str = PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, ch, consumed);
        if (str == NULL)
            return NULL;
        return Py_BuildValue("(Nn)", str, start+consumed);
    }
    else {
        wrong_exception_type(exc);
        return NULL;
    }
}


static PyObject *strict_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_StrictErrors(exc);
}


static PyObject *ignore_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_IgnoreErrors(exc);
}


static PyObject *replace_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_ReplaceErrors(exc);
}


static PyObject *xmlcharrefreplace_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_XMLCharRefReplaceErrors(exc);
}


static PyObject *backslashreplace_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_BackslashReplaceErrors(exc);
}

static PyObject *namereplace_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_NameReplaceErrors(exc);
}

static PyObject *surrogatepass_errors(PyObject *self, PyObject *exc)
{
    return PyCodec_SurrogatePassErrors(exc);
}

static PyObject *surrogateescape_errors(PyObject *self, PyObject *exc)
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
