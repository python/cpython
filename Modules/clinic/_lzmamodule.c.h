/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_lzma_LZMACompressor_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"Returns a chunk of compressed data if possible, or b\'\' otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.");

#define _LZMA_LZMACOMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)_lzma_LZMACompressor_compress, METH_O, _lzma_LZMACompressor_compress__doc__},

static PyObject *
_lzma_LZMACompressor_compress_impl(Compressor *self, Py_buffer *data);

static PyObject *
_lzma_LZMACompressor_compress(Compressor *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("compress", "argument", "contiguous buffer", arg);
        goto exit;
    }
    return_value = _lzma_LZMACompressor_compress_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_lzma_LZMACompressor_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"Returns the compressed data left in internal buffers.\n"
"\n"
"The compressor object may not be used after this method is called.");

#define _LZMA_LZMACOMPRESSOR_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_lzma_LZMACompressor_flush, METH_NOARGS, _lzma_LZMACompressor_flush__doc__},

static PyObject *
_lzma_LZMACompressor_flush_impl(Compressor *self);

static PyObject *
_lzma_LZMACompressor_flush(Compressor *self, PyObject *Py_UNUSED(ignored))
{
    return _lzma_LZMACompressor_flush_impl(self);
}

PyDoc_STRVAR(_lzma_LZMADecompressor_decompress__doc__,
"decompress($self, /, data, max_length=-1)\n"
"--\n"
"\n"
"Decompress *data*, returning uncompressed data as bytes.\n"
"\n"
"If *max_length* is nonnegative, returns at most *max_length* bytes of\n"
"decompressed data. If this limit is reached and further output can be\n"
"produced, *self.needs_input* will be set to ``False``. In this case, the next\n"
"call to *decompress()* may provide *data* as b\'\' to obtain more of the output.\n"
"\n"
"If all of the input data was decompressed and returned (either because this\n"
"was less than *max_length* bytes, or because *max_length* was negative),\n"
"*self.needs_input* will be set to True.\n"
"\n"
"Attempting to decompress data after the end of stream is reached raises an\n"
"EOFError.  Any data found after the end of the stream is ignored and saved in\n"
"the unused_data attribute.");

#define _LZMA_LZMADECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)(void(*)(void))_lzma_LZMADecompressor_decompress, METH_FASTCALL|METH_KEYWORDS, _lzma_LZMADecompressor_decompress__doc__},

static PyObject *
_lzma_LZMADecompressor_decompress_impl(Decompressor *self, Py_buffer *data,
                                       Py_ssize_t max_length);

static PyObject *
_lzma_LZMADecompressor_decompress(Decompressor *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "max_length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompress", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("decompress", "argument 'data'", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (PyFloat_Check(args[1])) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = PyNumber_Index(args[1]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        max_length = ival;
    }
skip_optional_pos:
    return_value = _lzma_LZMADecompressor_decompress_impl(self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_lzma_LZMADecompressor___init____doc__,
"LZMADecompressor(format=FORMAT_AUTO, memlimit=None, filters=None)\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  format\n"
"    Specifies the container format of the input stream.  If this is\n"
"    FORMAT_AUTO (the default), the decompressor will automatically detect\n"
"    whether the input is FORMAT_XZ or FORMAT_ALONE.  Streams created with\n"
"    FORMAT_RAW cannot be autodetected.\n"
"  memlimit\n"
"    Limit the amount of memory used by the decompressor.  This will cause\n"
"    decompression to fail if the input cannot be decompressed within the\n"
"    given limit.\n"
"  filters\n"
"    A custom filter chain.  This argument is required for FORMAT_RAW, and\n"
"    not accepted with any other format.  When provided, this should be a\n"
"    sequence of dicts, each indicating the ID and options for a single\n"
"    filter.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.");

static int
_lzma_LZMADecompressor___init___impl(Decompressor *self, int format,
                                     PyObject *memlimit, PyObject *filters);

static int
_lzma_LZMADecompressor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static const char * const _keywords[] = {"format", "memlimit", "filters", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "LZMADecompressor", 0};
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int format = FORMAT_AUTO;
    PyObject *memlimit = Py_None;
    PyObject *filters = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 3, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        if (PyFloat_Check(fastargs[0])) {
            PyErr_SetString(PyExc_TypeError,
                            "integer argument expected, got float" );
            goto exit;
        }
        format = _PyLong_AsInt(fastargs[0]);
        if (format == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        memlimit = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    filters = fastargs[2];
skip_optional_pos:
    return_value = _lzma_LZMADecompressor___init___impl((Decompressor *)self, format, memlimit, filters);

exit:
    return return_value;
}

PyDoc_STRVAR(_lzma_is_check_supported__doc__,
"is_check_supported($module, check_id, /)\n"
"--\n"
"\n"
"Test whether the given integrity check is supported.\n"
"\n"
"Always returns True for CHECK_NONE and CHECK_CRC32.");

#define _LZMA_IS_CHECK_SUPPORTED_METHODDEF    \
    {"is_check_supported", (PyCFunction)_lzma_is_check_supported, METH_O, _lzma_is_check_supported__doc__},

static PyObject *
_lzma_is_check_supported_impl(PyObject *module, int check_id);

static PyObject *
_lzma_is_check_supported(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int check_id;

    if (PyFloat_Check(arg)) {
        PyErr_SetString(PyExc_TypeError,
                        "integer argument expected, got float" );
        goto exit;
    }
    check_id = _PyLong_AsInt(arg);
    if (check_id == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = _lzma_is_check_supported_impl(module, check_id);

exit:
    return return_value;
}

PyDoc_STRVAR(_lzma__encode_filter_properties__doc__,
"_encode_filter_properties($module, filter, /)\n"
"--\n"
"\n"
"Return a bytes object encoding the options (properties) of the filter specified by *filter* (a dict).\n"
"\n"
"The result does not include the filter ID itself, only the options.");

#define _LZMA__ENCODE_FILTER_PROPERTIES_METHODDEF    \
    {"_encode_filter_properties", (PyCFunction)_lzma__encode_filter_properties, METH_O, _lzma__encode_filter_properties__doc__},

static PyObject *
_lzma__encode_filter_properties_impl(PyObject *module, lzma_filter filter);

static PyObject *
_lzma__encode_filter_properties(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    lzma_filter filter = {LZMA_VLI_UNKNOWN, NULL};

    if (!lzma_filter_converter(arg, &filter)) {
        goto exit;
    }
    return_value = _lzma__encode_filter_properties_impl(module, filter);

exit:
    /* Cleanup for filter */
    if (filter.id != LZMA_VLI_UNKNOWN)
       PyMem_Free(filter.options);

    return return_value;
}

PyDoc_STRVAR(_lzma__decode_filter_properties__doc__,
"_decode_filter_properties($module, filter_id, encoded_props, /)\n"
"--\n"
"\n"
"Return a bytes object encoding the options (properties) of the filter specified by *filter* (a dict).\n"
"\n"
"The result does not include the filter ID itself, only the options.");

#define _LZMA__DECODE_FILTER_PROPERTIES_METHODDEF    \
    {"_decode_filter_properties", (PyCFunction)(void(*)(void))_lzma__decode_filter_properties, METH_FASTCALL, _lzma__decode_filter_properties__doc__},

static PyObject *
_lzma__decode_filter_properties_impl(PyObject *module, lzma_vli filter_id,
                                     Py_buffer *encoded_props);

static PyObject *
_lzma__decode_filter_properties(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    lzma_vli filter_id;
    Py_buffer encoded_props = {NULL, NULL};

    if (!_PyArg_CheckPositional("_decode_filter_properties", nargs, 2, 2)) {
        goto exit;
    }
    if (!lzma_vli_converter(args[0], &filter_id)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[1], &encoded_props, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&encoded_props, 'C')) {
        _PyArg_BadArgument("_decode_filter_properties", "argument 2", "contiguous buffer", args[1]);
        goto exit;
    }
    return_value = _lzma__decode_filter_properties_impl(module, filter_id, &encoded_props);

exit:
    /* Cleanup for encoded_props */
    if (encoded_props.obj) {
       PyBuffer_Release(&encoded_props);
    }

    return return_value;
}
/*[clinic end generated code: output=f7477a10e86a717d input=a9049054013a1b77]*/
