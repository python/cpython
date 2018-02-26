/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(zlib_compress__doc__,
"compress($module, data, /, level=Z_DEFAULT_COMPRESSION)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"  level\n"
"    Compression level, in 0-9 or -1.");

#define ZLIB_COMPRESS_METHODDEF    \
    {"compress", (PyCFunction)zlib_compress, METH_FASTCALL, zlib_compress__doc__},

static PyObject *
zlib_compress_impl(PyObject *module, Py_buffer *data, int level);

static PyObject *
zlib_compress(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "level", NULL};
    static _PyArg_Parser _parser = {"y*|i:compress", _keywords, 0};
    Py_buffer data = {NULL, NULL};
    int level = Z_DEFAULT_COMPRESSION;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &data, &level)) {
        goto exit;
    }
    return_value = zlib_compress_impl(module, &data, level);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_decompress__doc__,
"decompress($module, data, /, wbits=MAX_WBITS, bufsize=DEF_BUF_SIZE)\n"
"--\n"
"\n"
"Returns a bytes object containing the uncompressed data.\n"
"\n"
"  data\n"
"    Compressed data.\n"
"  wbits\n"
"    The window buffer size and container format.\n"
"  bufsize\n"
"    The initial output buffer size.");

#define ZLIB_DECOMPRESS_METHODDEF    \
    {"decompress", (PyCFunction)zlib_decompress, METH_FASTCALL, zlib_decompress__doc__},

static PyObject *
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize);

static PyObject *
zlib_decompress(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "wbits", "bufsize", NULL};
    static _PyArg_Parser _parser = {"y*|iO&:decompress", _keywords, 0};
    Py_buffer data = {NULL, NULL};
    int wbits = MAX_WBITS;
    Py_ssize_t bufsize = DEF_BUF_SIZE;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &data, &wbits, ssize_t_converter, &bufsize)) {
        goto exit;
    }
    return_value = zlib_decompress_impl(module, &data, wbits, bufsize);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

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
"    The compression level (an integer in the range 0-9 or -1; default is\n"
"    currently equivalent to 6).  Higher compression levels are slower,\n"
"    but produce smaller results.\n"
"  method\n"
"    The compression algorithm.  If given, this must be DEFLATED.\n"
"  wbits\n"
"    +9 to +15: The base-two logarithm of the window size.  Include a zlib\n"
"        container.\n"
"    -9 to -15: Generate a raw stream.\n"
"    +25 to +31: Include a gzip container.\n"
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
    {"compressobj", (PyCFunction)zlib_compressobj, METH_FASTCALL, zlib_compressobj__doc__},

static PyObject *
zlib_compressobj_impl(PyObject *module, int level, int method, int wbits,
                      int memLevel, int strategy, Py_buffer *zdict);

static PyObject *
zlib_compressobj(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"level", "method", "wbits", "memLevel", "strategy", "zdict", NULL};
    static _PyArg_Parser _parser = {"|iiiiiy*:compressobj", _keywords, 0};
    int level = Z_DEFAULT_COMPRESSION;
    int method = DEFLATED;
    int wbits = MAX_WBITS;
    int memLevel = DEF_MEM_LEVEL;
    int strategy = Z_DEFAULT_STRATEGY;
    Py_buffer zdict = {NULL, NULL};

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &level, &method, &wbits, &memLevel, &strategy, &zdict)) {
        goto exit;
    }
    return_value = zlib_compressobj_impl(module, level, method, wbits, memLevel, strategy, &zdict);

exit:
    /* Cleanup for zdict */
    if (zdict.obj) {
       PyBuffer_Release(&zdict);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_decompressobj__doc__,
"decompressobj($module, /, wbits=MAX_WBITS, zdict=b\'\')\n"
"--\n"
"\n"
"Return a decompressor object.\n"
"\n"
"  wbits\n"
"    The window buffer size and container format.\n"
"  zdict\n"
"    The predefined compression dictionary.  This must be the same\n"
"    dictionary as used by the compressor that produced the input data.");

#define ZLIB_DECOMPRESSOBJ_METHODDEF    \
    {"decompressobj", (PyCFunction)zlib_decompressobj, METH_FASTCALL, zlib_decompressobj__doc__},

static PyObject *
zlib_decompressobj_impl(PyObject *module, int wbits, PyObject *zdict);

static PyObject *
zlib_decompressobj(PyObject *module, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {"|iO:decompressobj", _keywords, 0};
    int wbits = MAX_WBITS;
    PyObject *zdict = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &wbits, &zdict)) {
        goto exit;
    }
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
    {"compress", (PyCFunction)zlib_Compress_compress, METH_O, zlib_Compress_compress__doc__},

static PyObject *
zlib_Compress_compress_impl(compobject *self, Py_buffer *data);

static PyObject *
zlib_Compress_compress(compobject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (!PyArg_Parse(arg, "y*:compress", &data)) {
        goto exit;
    }
    return_value = zlib_Compress_compress_impl(self, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_Decompress_decompress__doc__,
"decompress($self, data, /, max_length=0)\n"
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
    {"decompress", (PyCFunction)zlib_Decompress_decompress, METH_FASTCALL, zlib_Decompress_decompress__doc__},

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, Py_buffer *data,
                                Py_ssize_t max_length);

static PyObject *
zlib_Decompress_decompress(compobject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "max_length", NULL};
    static _PyArg_Parser _parser = {"y*|O&:decompress", _keywords, 0};
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &data, ssize_t_converter, &max_length)) {
        goto exit;
    }
    return_value = zlib_Decompress_decompress_impl(self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

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

    if (!PyArg_ParseTuple(args, "|i:flush",
        &mode)) {
        goto exit;
    }
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
zlib_Decompress_flush_impl(compobject *self, Py_ssize_t length);

static PyObject *
zlib_Decompress_flush(compobject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t length = DEF_BUF_SIZE;

    if (!PyArg_ParseTuple(args, "|O&:flush",
        ssize_t_converter, &length)) {
        goto exit;
    }
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
zlib_adler32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_adler32(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 1;

    if (!PyArg_ParseTuple(args, "y*|I:adler32",
        &data, &value)) {
        goto exit;
    }
    return_value = zlib_adler32_impl(module, &data, value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

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
zlib_crc32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_crc32(PyObject *module, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 0;

    if (!PyArg_ParseTuple(args, "y*|I:crc32",
        &data, &value)) {
        goto exit;
    }
    return_value = zlib_crc32_impl(module, &data, value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#ifndef ZLIB_COMPRESS_COPY_METHODDEF
    #define ZLIB_COMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_COMPRESS_COPY_METHODDEF) */

#ifndef ZLIB_DECOMPRESS_COPY_METHODDEF
    #define ZLIB_DECOMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS_COPY_METHODDEF) */
/*[clinic end generated code: output=497dad1132c962e2 input=a9049054013a1b77]*/
