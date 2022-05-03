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
    {"compress", (PyCFunction)(void(*)(void))zlib_compress, METH_FASTCALL|METH_KEYWORDS, zlib_compress__doc__},

static PyObject *
zlib_compress_impl(PyObject *module, Py_buffer *data, int level);

static PyObject *
zlib_compress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "level", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "compress", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int level = Z_DEFAULT_COMPRESSION;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("compress", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    level = _PyLong_AsInt(args[1]);
    if (level == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
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
    {"decompress", (PyCFunction)(void(*)(void))zlib_decompress, METH_FASTCALL|METH_KEYWORDS, zlib_decompress__doc__},

static PyObject *
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize);

static PyObject *
zlib_decompress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "wbits", "bufsize", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompress", 0};
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int wbits = MAX_WBITS;
    Py_ssize_t bufsize = DEF_BUF_SIZE;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 3, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("decompress", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        wbits = _PyLong_AsInt(args[1]);
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[2]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        bufsize = ival;
    }
skip_optional_pos:
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
    {"compressobj", (PyCFunction)(void(*)(void))zlib_compressobj, METH_FASTCALL|METH_KEYWORDS, zlib_compressobj__doc__},

static PyObject *
zlib_compressobj_impl(PyObject *module, int level, int method, int wbits,
                      int memLevel, int strategy, Py_buffer *zdict);

static PyObject *
zlib_compressobj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"level", "method", "wbits", "memLevel", "strategy", "zdict", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "compressobj", 0};
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int level = Z_DEFAULT_COMPRESSION;
    int method = DEFLATED;
    int wbits = MAX_WBITS;
    int memLevel = DEF_MEM_LEVEL;
    int strategy = Z_DEFAULT_STRATEGY;
    Py_buffer zdict = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 6, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        level = _PyLong_AsInt(args[0]);
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        method = _PyLong_AsInt(args[1]);
        if (method == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        wbits = _PyLong_AsInt(args[2]);
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        memLevel = _PyLong_AsInt(args[3]);
        if (memLevel == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        strategy = _PyLong_AsInt(args[4]);
        if (strategy == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (PyObject_GetBuffer(args[5], &zdict, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&zdict, 'C')) {
        _PyArg_BadArgument("compressobj", "argument 'zdict'", "contiguous buffer", args[5]);
        goto exit;
    }
skip_optional_pos:
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
    {"decompressobj", (PyCFunction)(void(*)(void))zlib_decompressobj, METH_FASTCALL|METH_KEYWORDS, zlib_decompressobj__doc__},

static PyObject *
zlib_decompressobj_impl(PyObject *module, int wbits, PyObject *zdict);

static PyObject *
zlib_decompressobj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompressobj", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int wbits = MAX_WBITS;
    PyObject *zdict = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        wbits = _PyLong_AsInt(args[0]);
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    zdict = args[1];
skip_optional_pos:
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
    {"compress", (PyCFunction)(void(*)(void))zlib_Compress_compress, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_compress__doc__},

static PyObject *
zlib_Compress_compress_impl(compobject *self, PyTypeObject *cls,
                            Py_buffer *data);

static PyObject *
zlib_Compress_compress(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "compress", 0};
    PyObject *argsbuf[1];
    Py_buffer data = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("compress", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    return_value = zlib_Compress_compress_impl(self, cls, &data);

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
    {"decompress", (PyCFunction)(void(*)(void))zlib_Decompress_decompress, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_decompress__doc__},

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, PyTypeObject *cls,
                                Py_buffer *data, Py_ssize_t max_length);

static PyObject *
zlib_Decompress_decompress(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", "max_length", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "decompress", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("decompress", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[1]);
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
    return_value = zlib_Decompress_decompress_impl(self, cls, &data, max_length);

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
    {"flush", (PyCFunction)(void(*)(void))zlib_Compress_flush, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_flush__doc__},

static PyObject *
zlib_Compress_flush_impl(compobject *self, PyTypeObject *cls, int mode);

static PyObject *
zlib_Compress_flush(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "flush", 0};
    PyObject *argsbuf[1];
    int mode = Z_FINISH;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    mode = _PyLong_AsInt(args[0]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_posonly:
    return_value = zlib_Compress_flush_impl(self, cls, mode);

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
    {"copy", (PyCFunction)(void(*)(void))zlib_Compress_copy, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_copy__doc__},

static PyObject *
zlib_Compress_copy_impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Compress_copy(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return zlib_Compress_copy_impl(self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)(void(*)(void))zlib_Compress___copy__, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress___copy____doc__},

static PyObject *
zlib_Compress___copy___impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Compress___copy__(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return zlib_Compress___copy___impl(self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)(void(*)(void))zlib_Compress___deepcopy__, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress___deepcopy____doc__},

static PyObject *
zlib_Compress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                PyObject *memo);

static PyObject *
zlib_Compress___deepcopy__(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "__deepcopy__", 0};
    PyObject *argsbuf[1];
    PyObject *memo;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    memo = args[0];
    return_value = zlib_Compress___deepcopy___impl(self, cls, memo);

exit:
    return return_value;
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress_copy__doc__,
"copy($self, /)\n"
"--\n"
"\n"
"Return a copy of the decompression object.");

#define ZLIB_DECOMPRESS_COPY_METHODDEF    \
    {"copy", (PyCFunction)(void(*)(void))zlib_Decompress_copy, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_copy__doc__},

static PyObject *
zlib_Decompress_copy_impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Decompress_copy(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return zlib_Decompress_copy_impl(self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___COPY___METHODDEF    \
    {"__copy__", (PyCFunction)(void(*)(void))zlib_Decompress___copy__, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress___copy____doc__},

static PyObject *
zlib_Decompress___copy___impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Decompress___copy__(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return zlib_Decompress___copy___impl(self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", (PyCFunction)(void(*)(void))zlib_Decompress___deepcopy__, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress___deepcopy____doc__},

static PyObject *
zlib_Decompress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                  PyObject *memo);

static PyObject *
zlib_Decompress___deepcopy__(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "__deepcopy__", 0};
    PyObject *argsbuf[1];
    PyObject *memo;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    memo = args[0];
    return_value = zlib_Decompress___deepcopy___impl(self, cls, memo);

exit:
    return return_value;
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
    {"flush", (PyCFunction)(void(*)(void))zlib_Decompress_flush, METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_flush__doc__},

static PyObject *
zlib_Decompress_flush_impl(compobject *self, PyTypeObject *cls,
                           Py_ssize_t length);

static PyObject *
zlib_Decompress_flush(compobject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "flush", 0};
    PyObject *argsbuf[1];
    Py_ssize_t length = DEF_BUF_SIZE;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    {
        Py_ssize_t ival = -1;
        PyObject *iobj = _PyNumber_Index(args[0]);
        if (iobj != NULL) {
            ival = PyLong_AsSsize_t(iobj);
            Py_DECREF(iobj);
        }
        if (ival == -1 && PyErr_Occurred()) {
            goto exit;
        }
        length = ival;
    }
skip_optional_posonly:
    return_value = zlib_Decompress_flush_impl(self, cls, length);

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
    {"adler32", (PyCFunction)(void(*)(void))zlib_adler32, METH_FASTCALL, zlib_adler32__doc__},

static PyObject *
zlib_adler32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_adler32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 1;

    if (!_PyArg_CheckPositional("adler32", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("adler32", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    value = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (value == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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
    {"crc32", (PyCFunction)(void(*)(void))zlib_crc32, METH_FASTCALL, zlib_crc32__doc__},

static PyObject *
zlib_crc32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_crc32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 0;

    if (!_PyArg_CheckPositional("crc32", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!PyBuffer_IsContiguous(&data, 'C')) {
        _PyArg_BadArgument("crc32", "argument 1", "contiguous buffer", args[0]);
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    value = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (value == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
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

#ifndef ZLIB_COMPRESS___COPY___METHODDEF
    #define ZLIB_COMPRESS___COPY___METHODDEF
#endif /* !defined(ZLIB_COMPRESS___COPY___METHODDEF) */

#ifndef ZLIB_COMPRESS___DEEPCOPY___METHODDEF
    #define ZLIB_COMPRESS___DEEPCOPY___METHODDEF
#endif /* !defined(ZLIB_COMPRESS___DEEPCOPY___METHODDEF) */

#ifndef ZLIB_DECOMPRESS_COPY_METHODDEF
    #define ZLIB_DECOMPRESS_COPY_METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS_COPY_METHODDEF) */

#ifndef ZLIB_DECOMPRESS___COPY___METHODDEF
    #define ZLIB_DECOMPRESS___COPY___METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS___COPY___METHODDEF) */

#ifndef ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF
    #define ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF
#endif /* !defined(ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF) */
/*[clinic end generated code: output=1bda0d996fb51269 input=a9049054013a1b77]*/
