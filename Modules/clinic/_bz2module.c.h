/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_bz2_BZ2Compressor_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"Returns a chunk of compressed data if possible, or b\'\' otherwise.\n"
"\n"
"When you have finished providing data to the compressor, call the\n"
"flush() method to finish the compression process.");

#define _BZ2_BZ2COMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)_bz2_BZ2Compressor_compress, METH_O, _bz2_BZ2Compressor_compress__doc__},

static PyObject *
_bz2_BZ2Compressor_compress_impl(BZ2Compressor *self, Py_buffer *data);

static PyObject *
_bz2_BZ2Compressor_compress(BZ2Compressor *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!PyArg_Parse(arg, "y*:compress", &data)) {
        goto exit;
    }
    return_value = _bz2_BZ2Compressor_compress_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Compressor_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"Returns the compressed data left in internal buffers.\n"
"\n"
"The compressor object may not be used after this method is called.");

#define _BZ2_BZ2COMPRESSOR_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_bz2_BZ2Compressor_flush, METH_NOARGS, _bz2_BZ2Compressor_flush__doc__},

static PyObject *
_bz2_BZ2Compressor_flush_impl(BZ2Compressor *self);

static PyObject *
_bz2_BZ2Compressor_flush(BZ2Compressor *self, PyObject *Py_UNUSED(ignored))
{
    return _bz2_BZ2Compressor_flush_impl(self);
}

PyDoc_STRVAR(_bz2_BZ2Compressor___init____doc__,
"BZ2Compressor(compresslevel=9, /)\n"
"--\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"  compresslevel\n"
"    Compression level, as a number between 1 and 9.\n"
"\n"
"For one-shot compression, use the compress() function instead.");

static int
_bz2_BZ2Compressor___init___impl(BZ2Compressor *self, int compresslevel);

static int
_bz2_BZ2Compressor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    int compresslevel = 9;

    if ((Py_TYPE(self) == &BZ2Compressor_Type) &&
        !_PyArg_NoKeywords("BZ2Compressor", kwargs)) {
        goto exit;
    }
    if (!PyArg_ParseTuple(args, "|i:BZ2Compressor",
        &compresslevel)) {
        goto exit;
    }
    return_value = _bz2_BZ2Compressor___init___impl((BZ2Compressor *)self, compresslevel);

exit:
    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Decompressor_decompress__doc__,
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

#define _BZ2_BZ2DECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)_bz2_BZ2Decompressor_decompress, METH_FASTCALL|METH_KEYWORDS, _bz2_BZ2Decompressor_decompress__doc__},

static PyObject *
_bz2_BZ2Decompressor_decompress_impl(BZ2Decompressor *self, Py_buffer *data,
                                     Py_ssize_t max_length);

static PyObject *
_bz2_BZ2Decompressor_decompress(BZ2Decompressor *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"data", "max_length", NULL};
    static _PyArg_Parser _parser = {"y*|n:decompress", _keywords, 0};
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = -1;

    if (!_PyArg_ParseStackAndKeywords(args, nargs, kwnames, &_parser,
        &data, &max_length)) {
        goto exit;
    }
    return_value = _bz2_BZ2Decompressor_decompress_impl(self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_bz2_BZ2Decompressor___init____doc__,
"BZ2Decompressor()\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"For one-shot decompression, use the decompress() function instead.");

static int
_bz2_BZ2Decompressor___init___impl(BZ2Decompressor *self);

static int
_bz2_BZ2Decompressor___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;

    if ((Py_TYPE(self) == &BZ2Decompressor_Type) &&
        !_PyArg_NoPositional("BZ2Decompressor", args)) {
        goto exit;
    }
    if ((Py_TYPE(self) == &BZ2Decompressor_Type) &&
        !_PyArg_NoKeywords("BZ2Decompressor", kwargs)) {
        goto exit;
    }
    return_value = _bz2_BZ2Decompressor___init___impl((BZ2Decompressor *)self);

exit:
    return return_value;
}
/*[clinic end generated code: output=564a0313177fff63 input=a9049054013a1b77]*/
