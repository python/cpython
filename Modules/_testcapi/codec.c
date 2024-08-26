#include "parts.h"

// === Codecs registration and un-registration ================================

static PyObject *
codec_register(PyObject *Py_UNUSED(module), PyObject *search_function)
{
    if (PyCodec_Register(search_function) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
codec_unregister(PyObject *Py_UNUSED(module), PyObject *search_function)
{
    if (PyCodec_Unregister(search_function) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
codec_known_encoding(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;   // should not be NULL
    if (!PyArg_ParseTuple(args, "z", &encoding)) {
        return NULL;
    }
    assert(encoding != NULL);
    return PyCodec_KnownEncoding(encoding) ? Py_True : Py_False;
}

// === Codecs encoding and decoding interfaces ================================

static PyObject *
codec_encode(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *input;
    const char *encoding;   // should not be NULL
    const char *errors;     // can be NULL
    if (!PyArg_ParseTuple(args, "O|zz", &input, &encoding, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    return PyCodec_Encode(input, encoding, errors);
}

static PyObject *
codec_decode(PyObject *Py_UNUSED(module), PyObject *args)
{
    PyObject *input;
    const char *encoding;   // should not be NULL
    const char *errors;     // can be NULL
    if (!PyArg_ParseTuple(args, "O|zz", &input, &encoding, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    return PyCodec_Decode(input, encoding, errors);
}

static PyObject *
codec_encoder(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;  // should not be NULL
    if (!PyArg_ParseTuple(args, "z", &encoding)) {
        return NULL;
    }
    assert(encoding != NULL);
    return PyCodec_Encoder(encoding);
}

static PyObject *
codec_decoder(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;  // should not be NULL
    if (!PyArg_ParseTuple(args, "z", &encoding)) {
        return NULL;
    }
    assert(encoding != NULL);
    return PyCodec_Decoder(encoding);
}

static PyObject *
codec_incremental_encoder(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;   // should not be NULL
    const char *errors;     // should not be NULL
    if (!PyArg_ParseTuple(args, "zz", &encoding, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    assert(errors != NULL);
    return PyCodec_IncrementalEncoder(encoding, errors);
}

static PyObject *
codec_incremental_decoder(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;   // should not be NULL
    const char *errors;     // should not be NULL
    if (!PyArg_ParseTuple(args, "zz", &encoding, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    assert(errors != NULL);
    return PyCodec_IncrementalDecoder(encoding, errors);
}

static PyObject *
codec_stream_reader(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;  // should not be NULL
    PyObject *stream;
    const char *errors;    // should not be NULL
    if (!PyArg_ParseTuple(args, "zOz", &encoding, &stream, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    assert(errors != NULL);
    return PyCodec_StreamReader(encoding, stream, errors);
}

static PyObject *
codec_stream_writer(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;  // should not be NULL
    PyObject *stream;
    const char *errors;    // should not be NULL
    if (!PyArg_ParseTuple(args, "zOz", &encoding, &stream, &errors)) {
        return NULL;
    }
    assert(encoding != NULL);
    assert(errors != NULL);
    return PyCodec_StreamWriter(encoding, stream, errors);
}

// === Codecs errors handlers =================================================

static PyObject *
codec_register_error(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;  // should not be NULL
    PyObject *error;
    if (!PyArg_ParseTuple(args, "zO", &encoding, &error)) {
        return NULL;
    }
    assert(encoding != NULL);
    if (PyCodec_RegisterError(encoding, error) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
codec_lookup_error(PyObject *Py_UNUSED(module), PyObject *args)
{
    const char *encoding;   // can be NULL
    if (!PyArg_ParseTuple(args, "z", &encoding)) {
        return NULL;
    }
    return PyCodec_LookupError(encoding);
}

static PyObject *
codec_strict_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_StrictErrors(exc);
}

static PyObject *
codec_ignore_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_IgnoreErrors(exc);
}

static PyObject *
codec_replace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_ReplaceErrors(exc);
}

static PyObject *
codec_xmlcharrefreplace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_XMLCharRefReplaceErrors(exc);
}

static PyObject *
codec_backslashreplace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_BackslashReplaceErrors(exc);
}

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
static PyObject *
codec_namereplace_errors(PyObject *Py_UNUSED(module), PyObject *exc)
{
    assert(exc != NULL);
    return PyCodec_NameReplaceErrors(exc);
}
#endif

static PyMethodDef test_methods[] = {
    /* codecs registration */
    {"codec_register", codec_register, METH_O},
    {"codec_unregister", codec_unregister, METH_O},
    {"codec_known_encoding", codec_known_encoding, METH_VARARGS},
    /* encoding and decoding interface */
    {"codec_encode", codec_encode, METH_VARARGS},
    {"codec_decode", codec_decode, METH_VARARGS},
    {"codec_encoder", codec_encoder, METH_VARARGS},
    {"codec_decoder", codec_decoder, METH_VARARGS},
    {"codec_incremental_encoder", codec_incremental_encoder, METH_VARARGS},
    {"codec_incremental_decoder", codec_incremental_decoder, METH_VARARGS},
    {"codec_stream_reader", codec_stream_reader, METH_VARARGS},
    {"codec_stream_writer", codec_stream_writer, METH_VARARGS},
    /* error handling */
    {"codec_register_error", codec_register_error, METH_VARARGS},
    {"codec_lookup_error", codec_lookup_error, METH_VARARGS},
    {"codec_strict_errors", codec_strict_errors, METH_O},
    {"codec_ignore_errors", codec_ignore_errors, METH_O},
    {"codec_replace_errors", codec_replace_errors, METH_O},
    {"codec_xmlcharrefreplace_errors", codec_xmlcharrefreplace_errors, METH_O},
    {"codec_backslashreplace_errors", codec_backslashreplace_errors, METH_O},
#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x03050000
    {"codec_namereplace_errors", codec_namereplace_errors, METH_O},
#endif
    {NULL, NULL, 0, NULL},
};

int
_PyTestCapi_Init_Codec(PyObject *module)
{
    if (PyModule_AddFunctions(module, test_methods) < 0) {
        return -1;
    }
    return 0;
}
