/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(zlib_compress__doc__,
"compress($module, bytes, level=Z_DEFAULT_COMPRESSION, /)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  bytes\n"
"    Binary data to be compressed.\n"
"  level\n"
"    Compression level, in 0-9.");

#define ZLIB_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)zlib_compress, METH_VARARGS, zlib_compress__doc__},

static PyObject *
zlib_compress_impl(PyModuleDef *module, Py_buffer *bytes, int level);

static PyObject *
zlib_compress(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer bytes = {NULL, NULL};
    int level = Z_DEFAULT_COMPRESSION;

    if (!PyArg_ParseTuple(args,
        "y*|i:compress",
        &bytes, &level))
        goto exit;
    return_value = zlib_compress_impl(module, &bytes, level);

exit:
    /* Cleanup for bytes */
    if (bytes.obj)
       PyBuffer_Release(&bytes);

    return return_value;
}

PyDoc_STRVAR(zlib_decompress__doc__,
"decompress($module, data, wbits=MAX_WBITS, bufsize=DEF_BUF_SIZE, /)\n"
"--\n"
"\n"
"Returns a bytes object containing the uncompressed data.\n"
"\n"
"  data\n"
"    Compressed data.\n"
"  wbits\n"
"    The window buffer size.\n"
"  bufsize\n"
"    The initial output buffer size.");

#define ZLIB_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)zlib_decompress, METH_VARARGS, zlib_decompress__doc__},

static PyObject *
zlib_decompress_impl(PyModuleDef *module, Py_buffer *data, int wbits, unsigned int bufsize);

static PyObject *
zlib_decompress(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    int wbits = MAX_WBITS;
    unsigned int bufsize = DEF_BUF_SIZE;

    if (!PyArg_ParseTuple(args,
        "y*|iO&:decompress",
        &data, &wbits, uint_converter, &bufsize))
        goto exit;
    return_value = zlib_decompress_impl(module, &data, wbits, bufsize);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(zlib_compressobj__doc__,
"compressobj($module, /, level=Z_DEFAULT_COMPRESSION, method=DEFLATED,\n"
"            wbits=MAX_WBITS, memLevel=DEF_MEM_LEVEL,\n"
"            strategy=Z_DEFAULT_STRATEGY, zdict=None)\n"
"--\n"
"\n"
"Return a compressor object.\n"
"\n"
"  level\n"
"    The compression level (an integer in the range 0-9; default is 6).\n"
"    Higher compression levels are slower, but produce smaller results.\n"
"  method\n"
"    The compression algorithm.  If given, this must be DEFLATED.\n"
"  wbits\n"
"    The base two logarithm of the window size (range: 8..15).\n"
"  memLevel\n"
"    Controls the amount of memory used for internal compression state.\n"
"    Valid values range from 1 to 9.  Higher values result in higher memory\n"
"    usage, faster compression, and smaller output.\n"
"  strategy\n"
"    Used to tune the compression algorithm.  Possible values are\n"
"    Z_DEFAULT_STRATEGY, Z_FILTERED, and Z_HUFFMAN_ONLY.\n"
"  zdict\n"
"    The predefined compression dictionary - a sequence of bytes\n"
"    containing subsequences that are likely to occur in the input data.");

#define ZLIB_COMPRESSOBJ_METHODDEF    \
    {"compressobj", (PyCFunction)zlib_compressobj, METH_VARARGS|METH_KEYWORDS, zlib_compressobj__doc__},

static PyObject *
zlib_compressobj_impl(PyModuleDef *module, int level, int method, int wbits, int memLevel, int strategy, Py_buffer *zdict);

static PyObject *
zlib_compressobj(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"level", "method", "wbits", "memLevel", "strategy", "zdict", NULL};
    int level = Z_DEFAULT_COMPRESSION;
    int method = DEFLATED;
    int wbits = MAX_WBITS;
    int memLevel = DEF_MEM_LEVEL;
    int strategy = Z_DEFAULT_STRATEGY;
    Py_buffer zdict = {NULL, NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|iiiiiy*:compressobj", _keywords,
        &level, &method, &wbits, &memLevel, &strategy, &zdict))
        goto exit;
    return_value = zlib_compressobj_impl(module, level, method, wbits, memLevel, strategy, &zdict);

exit:
    /* Cleanup for zdict */
    if (zdict.obj)
       PyBuffer_Release(&zdict);

    return return_value;
}

PyDoc_STRVAR(zlib_decompressobj__doc__,
"decompressobj($module, /, wbits=MAX_WBITS, zdict=b\'\')\n"
"--\n"
"\n"
"Return a decompressor object.\n"
"\n"
"  wbits\n"
"    The window buffer size.\n"
"  zdict\n"
"    The predefined compression dictionary.  This must be the same\n"
"    dictionary as used by the compressor that produced the input data.");

#define ZLIB_DECOMPRESSOBJ_METHODDEF    \
    {"decompressobj", (PyCFunction)zlib_decompressobj, METH_VARARGS|METH_KEYWORDS, zlib_decompressobj__doc__},

static PyObject *
zlib_decompressobj_impl(PyModuleDef *module, int wbits, PyObject *zdict);

static PyObject *
zlib_decompressobj(PyModuleDef *module, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"wbits", "zdict", NULL};
    int wbits = MAX_WBITS;
    PyObject *zdict = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,
        "|iO:decompressobj", _keywords,
        &wbits, &zdict))
        goto exit;
    return_value = zlib_decompressobj_impl(module, wbits, zdict);

exit:
    return return_value;
}

PyDoc_STRVAR(zlib_Compress_compress__doc__,
"compress($self, data, /)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"\n"
"After calling this function, some of the input data may still\n"
"be stored in internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");

#define ZLIB_COMPRESS_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)zlib_Compress_compress, METH_VARARGS, zlib_Compress_compress__doc__},

static PyObject *
zlib_Compress_compress_impl(compobject *self, Py_buffer *data);

static PyObject *
zlib_Compress_compress(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!PyArg_ParseTuple(args,
        "y*:compress",
        &data))
        goto exit;
    return_value = zlib_Compress_compress_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(zlib_Decompress_decompress__doc__,
"decompress($self, data, max_length=0, /)\n"
"--\n"
"\n"
"Return a bytes object containing the decompressed version of the data.\n"
"\n"
"  data\n"
"    The binary data to decompress.\n"
"  max_length\n"
"    The maximum allowable length of the decompressed data.\n"
"    Unconsumed input data will be stored in\n"
"    the unconsumed_tail attribute.\n"
"\n"
"After calling this function, some of the input data may still be stored in\n"
"internal buffers for later processing.\n"
"Call the flush() method to clear these buffers.");

#define ZLIB_DECOMPRESS_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)zlib_Decompress_decompress, METH_VARARGS, zlib_Decompress_decompress__doc__},

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data, unsigned int max_length);

static PyObject *
zlib_Decompress_decompress(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int max_length = 0;

    if (!PyArg_ParseTuple(args,
        "y*|O&:decompress",
        &data, uint_converter, &max_length))
        goto exit;
    return_value = zlib_Decompress_decompress_impl(self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(zlib_Compress_flush__doc__,
"flush($self, mode=zlib.Z_FINISH, /)\n"
"--\n"
"\n"
"Return a bytes object containing any remaining compressed data.\n"
"\n"
"  mode\n"
"    One of the constants Z_SYNC_FLUSH, Z_FULL_FLUSH, Z_FINISH.\n"
"    If mode == Z_FINISH, the compressor object can no longer be\n"
"    used after calling the flush() method.  Otherwise, more data\n"
"    can still be compressed.");

#define ZLIB_COMPRESS_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)zlib_Compress_flush, METH_VARARGS, zlib_Compress_flush__doc__},

static PyObject *
zlib_Compress_flush_impl(compobject *self, int mode);

static PyObject *
zlib_Compress_flush(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    int mode = Z_FINISH;

    if (!PyArg_ParseTuple(args,
        "|i:flush",
        &mode))
        goto exit;
    return_value = zlib_Compress_flush_impl(self, mode);

exit:
    return return_value;
}

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the compression object.");

#define ZLIB_COMPRESS_COPY_METHODDEF    \
    {"copy", (PyCFunction)zlib_Compress_copy, METH_NOARGS, zlib_Compress_copy__doc__},

static PyObject *
zlib_Compress_copy_impl(compobject *self);

static PyObject *
zlib_Compress_copy(compobject *self, PyObject *Py_UNUSED(ignored))
{
    return zlib_Compress_copy_impl(self);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#ifndef ZLIB_COMPRESS_COPY_METHODDEF
    #define ZLIB_COMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_COMPRESS_COPY_METHODDEF) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the decompression object.");

#define ZLIB_DECOMPRESS_COPY_METHODDEF    \
    {"copy", (PyCFunction)zlib_Decompress_copy, METH_NOARGS, zlib_Decompress_copy__doc__},

static PyObject *
zlib_Decompress_copy_impl(compobject *self);

static PyObject *
zlib_Decompress_copy(compobject *self, PyObject *Py_UNUSED(ignored))
{
    return zlib_Decompress_copy_impl(self);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#ifndef ZLIB_DECOMPRESS_COPY_METHODDEF
    #define ZLIB_DECOMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS_COPY_METHODDEF) */

PyDoc_STRVAR(zlib_Decompress_flush__doc__,
"flush($self, length=zlib.DEF_BUF_SIZE, /)\n"
"--\n"
"\n"
"Return a bytes object containing any remaining decompressed data.\n"
"\n"
"  length\n"
"    the initial size of the output buffer.");

#define ZLIB_DECOMPRESS_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)zlib_Decompress_flush, METH_VARARGS, zlib_Decompress_flush__doc__},

static PyObject *
zlib_Decompress_flush_impl(compobject *self, unsigned int length);

static PyObject *
zlib_Decompress_flush(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    unsigned int length = DEF_BUF_SIZE;

    if (!PyArg_ParseTuple(args,
        "|O&:flush",
        uint_converter, &length))
        goto exit;
    return_value = zlib_Decompress_flush_impl(self, length);

exit:
    return return_value;
}

PyDoc_STRVAR(zlib_adler32__doc__,
"adler32($module, data, value=1, /)\n"
"--\n"
"\n"
"Compute an Adler-32 checksum of data.\n"
"\n"
"  value\n"
"    Starting value of the checksum.\n"
"\n"
"The returned checksum is an integer.");

#define ZLIB_ADLER32_METHODDEF    \
    {"adler32", (PyCFunction)zlib_adler32, METH_VARARGS, zlib_adler32__doc__},

static PyObject *
zlib_adler32_impl(PyModuleDef *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_adler32(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 1;

    if (!PyArg_ParseTuple(args,
        "y*|I:adler32",
        &data, &value))
        goto exit;
    return_value = zlib_adler32_impl(module, &data, value);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}

PyDoc_STRVAR(zlib_crc32__doc__,
"crc32($module, data, value=0, /)\n"
"--\n"
"\n"
"Compute a CRC-32 checksum of data.\n"
"\n"
"  value\n"
"    Starting value of the checksum.\n"
"\n"
"The returned checksum is an integer.");

#define ZLIB_CRC32_METHODDEF    \
    {"crc32", (PyCFunction)zlib_crc32, METH_VARARGS, zlib_crc32__doc__},

static PyObject *
zlib_crc32_impl(PyModuleDef *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_crc32(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 0;

    if (!PyArg_ParseTuple(args,
        "y*|I:crc32",
        &data, &value))
        goto exit;
    return_value = zlib_crc32_impl(module, &data, value);

exit:
    /* Cleanup for data */
    if (data.obj)
       PyBuffer_Release(&data);

    return return_value;
}
/*[clinic end generated code: output=bc9473721ca7c962 input=a9049054013a1b77]*/
