/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_codecs_register__doc__,
"register($module, search_function, /)\n"
"--\n"
"\n"
"Register a codec search function.\n"
"\n"
"Search functions are expected to take one argument, the encoding name in\n"
"all lower case letters, and either return None, or a tuple of functions\n"
"(encoder, decoder, stream_reader, stream_writer) (or a CodecInfo object).");

#define _CODECS_REGISTER_METHODDEF    \
    {"register", (PyCFunction)_codecs_register, METH_O, _codecs_register__doc__},

PyDoc_STRVAR(_codecs_lookup__doc__,
"lookup($module, encoding, /)\n"
"--\n"
"\n"
"Looks up a codec tuple in the Python codec registry and returns a CodecInfo object.");

#define _CODECS_LOOKUP_METHODDEF    \
    {"lookup", (PyCFunction)_codecs_lookup, METH_O, _codecs_lookup__doc__},

static PyObject *
_codecs_lookup_impl(PyObject *module, const char *encoding);

static PyObject *
_codecs_lookup(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *encoding;

    if (!PyArg_Parse(arg, "s:lookup", &encoding)) {
        goto exit;
    }
    return_value = _codecs_lookup_impl(module, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_encode__doc__,
"encode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Encodes obj using the codec registered for encoding.\n"
"\n"
"The default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that encoding\n"
"errors raise a ValueError.  Other possible values are \'ignore\', \'replace\'\n"
"and \'backslashreplace\' as well as any other name registered with\n"
"codecs.register_error that can handle ValueErrors.");

#define _CODECS_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)_codecs_encode, METH_FASTCALL, _codecs_encode__doc__},

static PyObject *
_codecs_encode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors);

static PyObject *
_codecs_encode(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {"O|ss:encode", _keywords, 0};
    PyObject *obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &obj, &encoding, &errors)) {
        goto exit;
    }
    return_value = _codecs_encode_impl(module, obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_decode__doc__,
"decode($module, /, obj, encoding=\'utf-8\', errors=\'strict\')\n"
"--\n"
"\n"
"Decodes obj using the codec registered for encoding.\n"
"\n"
"Default encoding is \'utf-8\'.  errors may be given to set a\n"
"different error handling scheme.  Default is \'strict\' meaning that encoding\n"
"errors raise a ValueError.  Other possible values are \'ignore\', \'replace\'\n"
"and \'backslashreplace\' as well as any other name registered with\n"
"codecs.register_error that can handle ValueErrors.");

#define _CODECS_DECODE_METHODDEF    \
    {"decode", (PyCFunction)_codecs_decode, METH_FASTCALL, _codecs_decode__doc__},

static PyObject *
_codecs_decode_impl(PyObject *module, PyObject *obj, const char *encoding,
                    const char *errors);

static PyObject *
_codecs_decode(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"obj", "encoding", "errors", NULL};
    static _PyArg_Parser _parser = {"O|ss:decode", _keywords, 0};
    PyObject *obj;
    const char *encoding = NULL;
    const char *errors = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &obj, &encoding, &errors)) {
        goto exit;
    }
    return_value = _codecs_decode_impl(module, obj, encoding, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs__forget_codec__doc__,
"_forget_codec($module, encoding, /)\n"
"--\n"
"\n"
"Purge the named codec from the internal codec lookup cache");

#define _CODECS__FORGET_CODEC_METHODDEF    \
    {"_forget_codec", (PyCFunction)_codecs__forget_codec, METH_O, _codecs__forget_codec__doc__},

static PyObject *
_codecs__forget_codec_impl(PyObject *module, const char *encoding);

static PyObject *
_codecs__forget_codec(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *encoding;

    if (!PyArg_Parse(arg, "s:_forget_codec", &encoding)) {
        goto exit;
    }
    return_value = _codecs__forget_codec_impl(module, encoding);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_escape_decode__doc__,
"escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_DECODE_METHODDEF    \
    {"escape_decode", (PyCFunction)_codecs_escape_decode, METH_VARARGS, _codecs_escape_decode__doc__},

static PyObject *
_codecs_escape_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors);

static PyObject *
_codecs_escape_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "s*|z:escape_decode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_escape_encode__doc__,
"escape_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ESCAPE_ENCODE_METHODDEF    \
    {"escape_encode", (PyCFunction)_codecs_escape_encode, METH_VARARGS, _codecs_escape_encode__doc__},

static PyObject *
_codecs_escape_encode_impl(PyObject *module, PyObject *data,
                           const char *errors);

static PyObject *
_codecs_escape_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *data;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O!|z:escape_encode",
        &PyBytes_Type, &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_escape_encode_impl(module, data, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_internal_decode__doc__,
"unicode_internal_decode($module, obj, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_INTERNAL_DECODE_METHODDEF    \
    {"unicode_internal_decode", (PyCFunction)_codecs_unicode_internal_decode, METH_VARARGS, _codecs_unicode_internal_decode__doc__},

static PyObject *
_codecs_unicode_internal_decode_impl(PyObject *module, PyObject *obj,
                                     const char *errors);

static PyObject *
_codecs_unicode_internal_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_decode",
        &obj, &errors)) {
        goto exit;
    }
    return_value = _codecs_unicode_internal_decode_impl(module, obj, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_decode__doc__,
"utf_7_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_DECODE_METHODDEF    \
    {"utf_7_decode", (PyCFunction)_codecs_utf_7_decode, METH_VARARGS, _codecs_utf_7_decode__doc__},

static PyObject *
_codecs_utf_7_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyObject *
_codecs_utf_7_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_7_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_7_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_decode__doc__,
"utf_8_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_DECODE_METHODDEF    \
    {"utf_8_decode", (PyCFunction)_codecs_utf_8_decode, METH_VARARGS, _codecs_utf_8_decode__doc__},

static PyObject *
_codecs_utf_8_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors, int final);

static PyObject *
_codecs_utf_8_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_8_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_8_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_decode__doc__,
"utf_16_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_DECODE_METHODDEF    \
    {"utf_16_decode", (PyCFunction)_codecs_utf_16_decode, METH_VARARGS, _codecs_utf_16_decode__doc__},

static PyObject *
_codecs_utf_16_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyObject *
_codecs_utf_16_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_16_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_decode__doc__,
"utf_16_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_DECODE_METHODDEF    \
    {"utf_16_le_decode", (PyCFunction)_codecs_utf_16_le_decode, METH_VARARGS, _codecs_utf_16_le_decode__doc__},

static PyObject *
_codecs_utf_16_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_16_le_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_le_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_16_le_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_decode__doc__,
"utf_16_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_DECODE_METHODDEF    \
    {"utf_16_be_decode", (PyCFunction)_codecs_utf_16_be_decode, METH_VARARGS, _codecs_utf_16_be_decode__doc__},

static PyObject *
_codecs_utf_16_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_16_be_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_16_be_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_16_be_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_ex_decode__doc__,
"utf_16_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_EX_DECODE_METHODDEF    \
    {"utf_16_ex_decode", (PyCFunction)_codecs_utf_16_ex_decode, METH_VARARGS, _codecs_utf_16_ex_decode__doc__},

static PyObject *
_codecs_utf_16_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyObject *
_codecs_utf_16_ex_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zii:utf_16_ex_decode",
        &data, &errors, &byteorder, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_16_ex_decode_impl(module, &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_decode__doc__,
"utf_32_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_DECODE_METHODDEF    \
    {"utf_32_decode", (PyCFunction)_codecs_utf_32_decode, METH_VARARGS, _codecs_utf_32_decode__doc__},

static PyObject *
_codecs_utf_32_decode_impl(PyObject *module, Py_buffer *data,
                           const char *errors, int final);

static PyObject *
_codecs_utf_32_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_32_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_decode__doc__,
"utf_32_le_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_DECODE_METHODDEF    \
    {"utf_32_le_decode", (PyCFunction)_codecs_utf_32_le_decode, METH_VARARGS, _codecs_utf_32_le_decode__doc__},

static PyObject *
_codecs_utf_32_le_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_32_le_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_le_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_32_le_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_decode__doc__,
"utf_32_be_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_DECODE_METHODDEF    \
    {"utf_32_be_decode", (PyCFunction)_codecs_utf_32_be_decode, METH_VARARGS, _codecs_utf_32_be_decode__doc__},

static PyObject *
_codecs_utf_32_be_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int final);

static PyObject *
_codecs_utf_32_be_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:utf_32_be_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_32_be_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_ex_decode__doc__,
"utf_32_ex_decode($module, data, errors=None, byteorder=0, final=False,\n"
"                 /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_EX_DECODE_METHODDEF    \
    {"utf_32_ex_decode", (PyCFunction)_codecs_utf_32_ex_decode, METH_VARARGS, _codecs_utf_32_ex_decode__doc__},

static PyObject *
_codecs_utf_32_ex_decode_impl(PyObject *module, Py_buffer *data,
                              const char *errors, int byteorder, int final);

static PyObject *
_codecs_utf_32_ex_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int byteorder = 0;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zii:utf_32_ex_decode",
        &data, &errors, &byteorder, &final)) {
        goto exit;
    }
    return_value = _codecs_utf_32_ex_decode_impl(module, &data, errors, byteorder, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_decode__doc__,
"unicode_escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"unicode_escape_decode", (PyCFunction)_codecs_unicode_escape_decode, METH_VARARGS, _codecs_unicode_escape_decode__doc__},

static PyObject *
_codecs_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                   const char *errors);

static PyObject *
_codecs_unicode_escape_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "s*|z:unicode_escape_decode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_unicode_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_decode__doc__,
"raw_unicode_escape_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_DECODE_METHODDEF    \
    {"raw_unicode_escape_decode", (PyCFunction)_codecs_raw_unicode_escape_decode, METH_VARARGS, _codecs_raw_unicode_escape_decode__doc__},

static PyObject *
_codecs_raw_unicode_escape_decode_impl(PyObject *module, Py_buffer *data,
                                       const char *errors);

static PyObject *
_codecs_raw_unicode_escape_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "s*|z:raw_unicode_escape_decode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_raw_unicode_escape_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_decode__doc__,
"latin_1_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_DECODE_METHODDEF    \
    {"latin_1_decode", (PyCFunction)_codecs_latin_1_decode, METH_VARARGS, _codecs_latin_1_decode__doc__},

static PyObject *
_codecs_latin_1_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors);

static PyObject *
_codecs_latin_1_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y*|z:latin_1_decode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_latin_1_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_decode__doc__,
"ascii_decode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_DECODE_METHODDEF    \
    {"ascii_decode", (PyCFunction)_codecs_ascii_decode, METH_VARARGS, _codecs_ascii_decode__doc__},

static PyObject *
_codecs_ascii_decode_impl(PyObject *module, Py_buffer *data,
                          const char *errors);

static PyObject *
_codecs_ascii_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "y*|z:ascii_decode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_ascii_decode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_decode__doc__,
"charmap_decode($module, data, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_DECODE_METHODDEF    \
    {"charmap_decode", (PyCFunction)_codecs_charmap_decode, METH_VARARGS, _codecs_charmap_decode__doc__},

static PyObject *
_codecs_charmap_decode_impl(PyObject *module, Py_buffer *data,
                            const char *errors, PyObject *mapping);

static PyObject *
_codecs_charmap_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "y*|zO:charmap_decode",
        &data, &errors, &mapping)) {
        goto exit;
    }
    return_value = _codecs_charmap_decode_impl(module, &data, errors, mapping);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_decode__doc__,
"mbcs_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_DECODE_METHODDEF    \
    {"mbcs_decode", (PyCFunction)_codecs_mbcs_decode, METH_VARARGS, _codecs_mbcs_decode__doc__},

static PyObject *
_codecs_mbcs_decode_impl(PyObject *module, Py_buffer *data,
                         const char *errors, int final);

static PyObject *
_codecs_mbcs_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:mbcs_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_mbcs_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_decode__doc__,
"oem_decode($module, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_DECODE_METHODDEF    \
    {"oem_decode", (PyCFunction)_codecs_oem_decode, METH_VARARGS, _codecs_oem_decode__doc__},

static PyObject *
_codecs_oem_decode_impl(PyObject *module, Py_buffer *data,
                        const char *errors, int final);

static PyObject *
_codecs_oem_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "y*|zi:oem_decode",
        &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_oem_decode_impl(module, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_decode__doc__,
"code_page_decode($module, codepage, data, errors=None, final=False, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_DECODE_METHODDEF    \
    {"code_page_decode", (PyCFunction)_codecs_code_page_decode, METH_VARARGS, _codecs_code_page_decode__doc__},

static PyObject *
_codecs_code_page_decode_impl(PyObject *module, int codepage,
                              Py_buffer *data, const char *errors, int final);

static PyObject *
_codecs_code_page_decode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int codepage;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;
    int final = 0;

    if (!PyArg_ParseTuple(args, "iy*|zi:code_page_decode",
        &codepage, &data, &errors, &final)) {
        goto exit;
    }
    return_value = _codecs_code_page_decode_impl(module, codepage, &data, errors, final);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(_codecs_readbuffer_encode__doc__,
"readbuffer_encode($module, data, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_READBUFFER_ENCODE_METHODDEF    \
    {"readbuffer_encode", (PyCFunction)_codecs_readbuffer_encode, METH_VARARGS, _codecs_readbuffer_encode__doc__},

static PyObject *
_codecs_readbuffer_encode_impl(PyObject *module, Py_buffer *data,
                               const char *errors);

static PyObject *
_codecs_readbuffer_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "s*|z:readbuffer_encode",
        &data, &errors)) {
        goto exit;
    }
    return_value = _codecs_readbuffer_encode_impl(module, &data, errors);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_internal_encode__doc__,
"unicode_internal_encode($module, obj, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_INTERNAL_ENCODE_METHODDEF    \
    {"unicode_internal_encode", (PyCFunction)_codecs_unicode_internal_encode, METH_VARARGS, _codecs_unicode_internal_encode__doc__},

static PyObject *
_codecs_unicode_internal_encode_impl(PyObject *module, PyObject *obj,
                                     const char *errors);

static PyObject *
_codecs_unicode_internal_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *obj;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "O|z:unicode_internal_encode",
        &obj, &errors)) {
        goto exit;
    }
    return_value = _codecs_unicode_internal_encode_impl(module, obj, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_7_encode__doc__,
"utf_7_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_7_ENCODE_METHODDEF    \
    {"utf_7_encode", (PyCFunction)_codecs_utf_7_encode, METH_VARARGS, _codecs_utf_7_encode__doc__},

static PyObject *
_codecs_utf_7_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_utf_7_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_7_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_7_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_8_encode__doc__,
"utf_8_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_8_ENCODE_METHODDEF    \
    {"utf_8_encode", (PyCFunction)_codecs_utf_8_encode, METH_VARARGS, _codecs_utf_8_encode__doc__},

static PyObject *
_codecs_utf_8_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_utf_8_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_8_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_8_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_encode__doc__,
"utf_16_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_ENCODE_METHODDEF    \
    {"utf_16_encode", (PyCFunction)_codecs_utf_16_encode, METH_VARARGS, _codecs_utf_16_encode__doc__},

static PyObject *
_codecs_utf_16_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyObject *
_codecs_utf_16_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!PyArg_ParseTuple(args, "U|zi:utf_16_encode",
        &str, &errors, &byteorder)) {
        goto exit;
    }
    return_value = _codecs_utf_16_encode_impl(module, str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_le_encode__doc__,
"utf_16_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_LE_ENCODE_METHODDEF    \
    {"utf_16_le_encode", (PyCFunction)_codecs_utf_16_le_encode, METH_VARARGS, _codecs_utf_16_le_encode__doc__},

static PyObject *
_codecs_utf_16_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_16_le_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_16_le_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_16_le_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_16_be_encode__doc__,
"utf_16_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_16_BE_ENCODE_METHODDEF    \
    {"utf_16_be_encode", (PyCFunction)_codecs_utf_16_be_encode, METH_VARARGS, _codecs_utf_16_be_encode__doc__},

static PyObject *
_codecs_utf_16_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_16_be_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_16_be_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_16_be_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_encode__doc__,
"utf_32_encode($module, str, errors=None, byteorder=0, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_ENCODE_METHODDEF    \
    {"utf_32_encode", (PyCFunction)_codecs_utf_32_encode, METH_VARARGS, _codecs_utf_32_encode__doc__},

static PyObject *
_codecs_utf_32_encode_impl(PyObject *module, PyObject *str,
                           const char *errors, int byteorder);

static PyObject *
_codecs_utf_32_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    int byteorder = 0;

    if (!PyArg_ParseTuple(args, "U|zi:utf_32_encode",
        &str, &errors, &byteorder)) {
        goto exit;
    }
    return_value = _codecs_utf_32_encode_impl(module, str, errors, byteorder);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_le_encode__doc__,
"utf_32_le_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_LE_ENCODE_METHODDEF    \
    {"utf_32_le_encode", (PyCFunction)_codecs_utf_32_le_encode, METH_VARARGS, _codecs_utf_32_le_encode__doc__},

static PyObject *
_codecs_utf_32_le_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_32_le_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_32_le_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_32_le_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_utf_32_be_encode__doc__,
"utf_32_be_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UTF_32_BE_ENCODE_METHODDEF    \
    {"utf_32_be_encode", (PyCFunction)_codecs_utf_32_be_encode, METH_VARARGS, _codecs_utf_32_be_encode__doc__},

static PyObject *
_codecs_utf_32_be_encode_impl(PyObject *module, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_utf_32_be_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:utf_32_be_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_utf_32_be_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_unicode_escape_encode__doc__,
"unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"unicode_escape_encode", (PyCFunction)_codecs_unicode_escape_encode, METH_VARARGS, _codecs_unicode_escape_encode__doc__},

static PyObject *
_codecs_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                   const char *errors);

static PyObject *
_codecs_unicode_escape_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:unicode_escape_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_unicode_escape_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_raw_unicode_escape_encode__doc__,
"raw_unicode_escape_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_RAW_UNICODE_ESCAPE_ENCODE_METHODDEF    \
    {"raw_unicode_escape_encode", (PyCFunction)_codecs_raw_unicode_escape_encode, METH_VARARGS, _codecs_raw_unicode_escape_encode__doc__},

static PyObject *
_codecs_raw_unicode_escape_encode_impl(PyObject *module, PyObject *str,
                                       const char *errors);

static PyObject *
_codecs_raw_unicode_escape_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:raw_unicode_escape_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_raw_unicode_escape_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_latin_1_encode__doc__,
"latin_1_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_LATIN_1_ENCODE_METHODDEF    \
    {"latin_1_encode", (PyCFunction)_codecs_latin_1_encode, METH_VARARGS, _codecs_latin_1_encode__doc__},

static PyObject *
_codecs_latin_1_encode_impl(PyObject *module, PyObject *str,
                            const char *errors);

static PyObject *
_codecs_latin_1_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:latin_1_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_latin_1_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_ascii_encode__doc__,
"ascii_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_ASCII_ENCODE_METHODDEF    \
    {"ascii_encode", (PyCFunction)_codecs_ascii_encode, METH_VARARGS, _codecs_ascii_encode__doc__},

static PyObject *
_codecs_ascii_encode_impl(PyObject *module, PyObject *str,
                          const char *errors);

static PyObject *
_codecs_ascii_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:ascii_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_ascii_encode_impl(module, str, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_encode__doc__,
"charmap_encode($module, str, errors=None, mapping=None, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_ENCODE_METHODDEF    \
    {"charmap_encode", (PyCFunction)_codecs_charmap_encode, METH_VARARGS, _codecs_charmap_encode__doc__},

static PyObject *
_codecs_charmap_encode_impl(PyObject *module, PyObject *str,
                            const char *errors, PyObject *mapping);

static PyObject *
_codecs_charmap_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;
    PyObject *mapping = NULL;

    if (!PyArg_ParseTuple(args, "U|zO:charmap_encode",
        &str, &errors, &mapping)) {
        goto exit;
    }
    return_value = _codecs_charmap_encode_impl(module, str, errors, mapping);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_charmap_build__doc__,
"charmap_build($module, map, /)\n"
"--\n"
"\n");

#define _CODECS_CHARMAP_BUILD_METHODDEF    \
    {"charmap_build", (PyCFunction)_codecs_charmap_build, METH_O, _codecs_charmap_build__doc__},

static PyObject *
_codecs_charmap_build_impl(PyObject *module, PyObject *map);

static PyObject *
_codecs_charmap_build(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *map;

    if (!PyArg_Parse(arg, "U:charmap_build", &map)) {
        goto exit;
    }
    return_value = _codecs_charmap_build_impl(module, map);

exit:
    return return_value;
}

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_mbcs_encode__doc__,
"mbcs_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_MBCS_ENCODE_METHODDEF    \
    {"mbcs_encode", (PyCFunction)_codecs_mbcs_encode, METH_VARARGS, _codecs_mbcs_encode__doc__},

static PyObject *
_codecs_mbcs_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyObject *
_codecs_mbcs_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:mbcs_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_mbcs_encode_impl(module, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_oem_encode__doc__,
"oem_encode($module, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_OEM_ENCODE_METHODDEF    \
    {"oem_encode", (PyCFunction)_codecs_oem_encode, METH_VARARGS, _codecs_oem_encode__doc__},

static PyObject *
_codecs_oem_encode_impl(PyObject *module, PyObject *str, const char *errors);

static PyObject *
_codecs_oem_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "U|z:oem_encode",
        &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_oem_encode_impl(module, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

#if defined(MS_WINDOWS)

PyDoc_STRVAR(_codecs_code_page_encode__doc__,
"code_page_encode($module, code_page, str, errors=None, /)\n"
"--\n"
"\n");

#define _CODECS_CODE_PAGE_ENCODE_METHODDEF    \
    {"code_page_encode", (PyCFunction)_codecs_code_page_encode, METH_VARARGS, _codecs_code_page_encode__doc__},

static PyObject *
_codecs_code_page_encode_impl(PyObject *module, int code_page, PyObject *str,
                              const char *errors);

static PyObject *
_codecs_code_page_encode(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    int code_page;
    PyObject *str;
    const char *errors = NULL;

    if (!PyArg_ParseTuple(args, "iU|z:code_page_encode",
        &code_page, &str, &errors)) {
        goto exit;
    }
    return_value = _codecs_code_page_encode_impl(module, code_page, str, errors);

exit:
    return return_value;
}

#endif /* defined(MS_WINDOWS) */

PyDoc_STRVAR(_codecs_register_error__doc__,
"register_error($module, errors, handler, /)\n"
"--\n"
"\n"
"Register the specified error handler under the name errors.\n"
"\n"
"handler must be a callable object, that will be called with an exception\n"
"instance containing information about the location of the encoding/decoding\n"
"error and must return a (replacement, new position) tuple.");

#define _CODECS_REGISTER_ERROR_METHODDEF    \
    {"register_error", (PyCFunction)_codecs_register_error, METH_VARARGS, _codecs_register_error__doc__},

static PyObject *
_codecs_register_error_impl(PyObject *module, const char *errors,
                            PyObject *handler);

static PyObject *
_codecs_register_error(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    const char *errors;
    PyObject *handler;

    if (!PyArg_ParseTuple(args, "sO:register_error",
        &errors, &handler)) {
        goto exit;
    }
    return_value = _codecs_register_error_impl(module, errors, handler);

exit:
    return return_value;
}

PyDoc_STRVAR(_codecs_lookup_error__doc__,
"lookup_error($module, name, /)\n"
"--\n"
"\n"
"lookup_error(errors) -> handler\n"
"\n"
"Return the error handler for the specified error handling name or raise a\n"
"LookupError, if no handler exists under this name.");

#define _CODECS_LOOKUP_ERROR_METHODDEF    \
    {"lookup_error", (PyCFunction)_codecs_lookup_error, METH_O, _codecs_lookup_error__doc__},

static PyObject *
_codecs_lookup_error_impl(PyObject *module, const char *name);

static PyObject *
_codecs_lookup_error(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    const char *name;

    if (!PyArg_Parse(arg, "s:lookup_error", &name)) {
        goto exit;
    }
    return_value = _codecs_lookup_error_impl(module, name);

exit:
    return return_value;
}

#ifndef _CODECS_MBCS_DECODE_METHODDEF
    #define _CODECS_MBCS_DECODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_DECODE_METHODDEF) */

#ifndef _CODECS_OEM_DECODE_METHODDEF
    #define _CODECS_OEM_DECODE_METHODDEF
#endif /* !defined(_CODECS_OEM_DECODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_DECODE_METHODDEF
    #define _CODECS_CODE_PAGE_DECODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_DECODE_METHODDEF) */

#ifndef _CODECS_MBCS_ENCODE_METHODDEF
    #define _CODECS_MBCS_ENCODE_METHODDEF
#endif /* !defined(_CODECS_MBCS_ENCODE_METHODDEF) */

#ifndef _CODECS_OEM_ENCODE_METHODDEF
    #define _CODECS_OEM_ENCODE_METHODDEF
#endif /* !defined(_CODECS_OEM_ENCODE_METHODDEF) */

#ifndef _CODECS_CODE_PAGE_ENCODE_METHODDEF
    #define _CODECS_CODE_PAGE_ENCODE_METHODDEF
#endif /* !defined(_CODECS_CODE_PAGE_ENCODE_METHODDEF) */
/*[clinic end generated code: output=6d6afcabde10ed79 input=a9049054013a1b77]*/
