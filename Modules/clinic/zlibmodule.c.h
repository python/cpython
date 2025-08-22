/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(zlib_compress__doc__,
"compress($module, data, /, level=Z_DEFAULT_COMPRESSION, wbits=MAX_WBITS)\n"
"--\n"
"\n"
"Returns a bytes object containing compressed data.\n"
"\n"
"  data\n"
"    Binary data to be compressed.\n"
"  level\n"
"    Compression level, in 0-9 or -1.\n"
"  wbits\n"
"    The window buffer size and container format.");

#define ZLIB_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(zlib_compress), METH_FASTCALL|METH_KEYWORDS, zlib_compress__doc__},

static PyObject *
zlib_compress_impl(PyObject *module, Py_buffer *data, int level, int wbits);

static PyObject *
zlib_compress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(level), &_Py_ID(wbits), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "level", "wbits", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int level = Z_DEFAULT_COMPRESSION;
    int wbits = MAX_WBITS;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        level = PyLong_AsInt(args[1]);
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    wbits = PyLong_AsInt(args[2]);
    if (wbits == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = zlib_compress_impl(module, &data, level, wbits);

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
    {"decompress", _PyCFunction_CAST(zlib_decompress), METH_FASTCALL|METH_KEYWORDS, zlib_decompress__doc__},

static PyObject *
zlib_decompress_impl(PyObject *module, Py_buffer *data, int wbits,
                     Py_ssize_t bufsize);

static PyObject *
zlib_decompress(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(wbits), &_Py_ID(bufsize), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "wbits", "bufsize", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int wbits = MAX_WBITS;
    Py_ssize_t bufsize = DEF_BUF_SIZE;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        wbits = PyLong_AsInt(args[1]);
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
    {"compressobj", _PyCFunction_CAST(zlib_compressobj), METH_FASTCALL|METH_KEYWORDS, zlib_compressobj__doc__},

static PyObject *
zlib_compressobj_impl(PyObject *module, int level, int method, int wbits,
                      int memLevel, int strategy, Py_buffer *zdict);

static PyObject *
zlib_compressobj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 6
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(level), &_Py_ID(method), &_Py_ID(wbits), &_Py_ID(memLevel), &_Py_ID(strategy), &_Py_ID(zdict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"level", "method", "wbits", "memLevel", "strategy", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compressobj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[6];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int level = Z_DEFAULT_COMPRESSION;
    int method = DEFLATED;
    int wbits = MAX_WBITS;
    int memLevel = DEF_MEM_LEVEL;
    int strategy = Z_DEFAULT_STRATEGY;
    Py_buffer zdict = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 6, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        level = PyLong_AsInt(args[0]);
        if (level == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[1]) {
        method = PyLong_AsInt(args[1]);
        if (method == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        wbits = PyLong_AsInt(args[2]);
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        memLevel = PyLong_AsInt(args[3]);
        if (memLevel == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        strategy = PyLong_AsInt(args[4]);
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
    {"decompressobj", _PyCFunction_CAST(zlib_decompressobj), METH_FASTCALL|METH_KEYWORDS, zlib_decompressobj__doc__},

static PyObject *
zlib_decompressobj_impl(PyObject *module, int wbits, PyObject *zdict);

static PyObject *
zlib_decompressobj(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(wbits), &_Py_ID(zdict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompressobj",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int wbits = MAX_WBITS;
    PyObject *zdict = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        wbits = PyLong_AsInt(args[0]);
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
    {"compress", _PyCFunction_CAST(zlib_Compress_compress), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_compress__doc__},

static PyObject *
zlib_Compress_compress_impl(compobject *self, PyTypeObject *cls,
                            Py_buffer *data);

static PyObject *
zlib_Compress_compress(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_buffer data = {NULL, NULL};

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = zlib_Compress_compress_impl((compobject *)self, cls, &data);

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
    {"decompress", _PyCFunction_CAST(zlib_Decompress_decompress), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_decompress__doc__},

static PyObject *
zlib_Decompress_decompress_impl(compobject *self, PyTypeObject *cls,
                                Py_buffer *data, Py_ssize_t max_length);

static PyObject *
zlib_Decompress_decompress(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(max_length), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"", "max_length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
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
    return_value = zlib_Decompress_decompress_impl((compobject *)self, cls, &data, max_length);

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
    {"flush", _PyCFunction_CAST(zlib_Compress_flush), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_flush__doc__},

static PyObject *
zlib_Compress_flush_impl(compobject *self, PyTypeObject *cls, int mode);

static PyObject *
zlib_Compress_flush(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "flush",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    int mode = Z_FINISH;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional_posonly;
    }
    mode = PyLong_AsInt(args[0]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_posonly:
    return_value = zlib_Compress_flush_impl((compobject *)self, cls, mode);

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
    {"copy", _PyCFunction_CAST(zlib_Compress_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress_copy__doc__},

static PyObject *
zlib_Compress_copy_impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Compress_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return zlib_Compress_copy_impl((compobject *)self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(zlib_Compress___copy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress___copy____doc__},

static PyObject *
zlib_Compress___copy___impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Compress___copy__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return zlib_Compress___copy___impl((compobject *)self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Compress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_COMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(zlib_Compress___deepcopy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Compress___deepcopy____doc__},

static PyObject *
zlib_Compress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                PyObject *memo);

static PyObject *
zlib_Compress___deepcopy__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__deepcopy__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *memo;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    memo = args[0];
    return_value = zlib_Compress___deepcopy___impl((compobject *)self, cls, memo);

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
    {"copy", _PyCFunction_CAST(zlib_Decompress_copy), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_copy__doc__},

static PyObject *
zlib_Decompress_copy_impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Decompress_copy(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "copy() takes no arguments");
        return NULL;
    }
    return zlib_Decompress_copy_impl((compobject *)self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___copy____doc__,
"__copy__($self, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___COPY___METHODDEF    \
    {"__copy__", _PyCFunction_CAST(zlib_Decompress___copy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress___copy____doc__},

static PyObject *
zlib_Decompress___copy___impl(compobject *self, PyTypeObject *cls);

static PyObject *
zlib_Decompress___copy__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "__copy__() takes no arguments");
        return NULL;
    }
    return zlib_Decompress___copy___impl((compobject *)self, cls);
}

#endif /* defined(HAVE_ZLIB_COPY) */

#if defined(HAVE_ZLIB_COPY)

PyDoc_STRVAR(zlib_Decompress___deepcopy____doc__,
"__deepcopy__($self, memo, /)\n"
"--\n"
"\n");

#define ZLIB_DECOMPRESS___DEEPCOPY___METHODDEF    \
    {"__deepcopy__", _PyCFunction_CAST(zlib_Decompress___deepcopy__), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress___deepcopy____doc__},

static PyObject *
zlib_Decompress___deepcopy___impl(compobject *self, PyTypeObject *cls,
                                  PyObject *memo);

static PyObject *
zlib_Decompress___deepcopy__(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "__deepcopy__",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *memo;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    memo = args[0];
    return_value = zlib_Decompress___deepcopy___impl((compobject *)self, cls, memo);

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
    {"flush", _PyCFunction_CAST(zlib_Decompress_flush), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, zlib_Decompress_flush__doc__},

static PyObject *
zlib_Decompress_flush_impl(compobject *self, PyTypeObject *cls,
                           Py_ssize_t length);

static PyObject *
zlib_Decompress_flush(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
    #  define KWTUPLE (PyObject *)&_Py_SINGLETON(tuple_empty)
    #else
    #  define KWTUPLE NULL
    #endif

    static const char * const _keywords[] = {"", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "flush",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t length = DEF_BUF_SIZE;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
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
    return_value = zlib_Decompress_flush_impl((compobject *)self, cls, length);

exit:
    return return_value;
}

PyDoc_STRVAR(zlib__ZlibDecompressor_decompress__doc__,
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

#define ZLIB__ZLIBDECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(zlib__ZlibDecompressor_decompress), METH_FASTCALL|METH_KEYWORDS, zlib__ZlibDecompressor_decompress__doc__},

static PyObject *
zlib__ZlibDecompressor_decompress_impl(ZlibDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length);

static PyObject *
zlib__ZlibDecompressor_decompress(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(data), &_Py_ID(max_length), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "max_length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decompress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    Py_ssize_t max_length = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
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
    return_value = zlib__ZlibDecompressor_decompress_impl((ZlibDecompressor *)self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib__ZlibDecompressor__doc__,
"_ZlibDecompressor(wbits=MAX_WBITS, zdict=b\'\')\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  zdict\n"
"    The predefined compression dictionary. This is a sequence of bytes\n"
"    (such as a bytes object) containing subsequences that are expected\n"
"    to occur frequently in the data that is to be compressed. Those\n"
"    subsequences that are expected to be most common should come at the\n"
"    end of the dictionary. This must be the same dictionary as used by the\n"
"    compressor that produced the input data.");

static PyObject *
zlib__ZlibDecompressor_impl(PyTypeObject *type, int wbits, PyObject *zdict);

static PyObject *
zlib__ZlibDecompressor(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 2
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(wbits), &_Py_ID(zdict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"wbits", "zdict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "_ZlibDecompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int wbits = MAX_WBITS;
    PyObject *zdict = NULL;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        wbits = PyLong_AsInt(fastargs[0]);
        if (wbits == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    zdict = fastargs[1];
skip_optional_pos:
    return_value = zlib__ZlibDecompressor_impl(type, wbits, zdict);

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
    {"adler32", _PyCFunction_CAST(zlib_adler32), METH_FASTCALL, zlib_adler32__doc__},

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
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[1], &value, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
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

PyDoc_STRVAR(zlib_adler32_combine__doc__,
"adler32_combine($module, adler1, adler2, len2, /)\n"
"--\n"
"\n"
"Combine two Adler-32 checksums into one.\n"
"\n"
"  adler1\n"
"    Adler-32 checksum for sequence A\n"
"  adler2\n"
"    Adler-32 checksum for sequence B\n"
"  len2\n"
"    Length of sequence B\n"
"\n"
"Given the Adler-32 checksum \'adler1\' of a sequence A and the\n"
"Adler-32 checksum \'adler2\' of a sequence B of length \'len2\',\n"
"return the Adler-32 checksum of A and B concatenated.");

#define ZLIB_ADLER32_COMBINE_METHODDEF    \
    {"adler32_combine", _PyCFunction_CAST(zlib_adler32_combine), METH_FASTCALL, zlib_adler32_combine__doc__},

static unsigned int
zlib_adler32_combine_impl(PyObject *module, unsigned int adler1,
                          unsigned int adler2, PyObject *len2);

static PyObject *
zlib_adler32_combine(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int adler1;
    unsigned int adler2;
    PyObject *len2;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("adler32_combine", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[0], &adler1, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[1], &adler2, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    if (!PyLong_Check(args[2])) {
        _PyArg_BadArgument("adler32_combine", "argument 3", "int", args[2]);
        goto exit;
    }
    len2 = args[2];
    _return_value = zlib_adler32_combine_impl(module, adler1, adler2, len2);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
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
    {"crc32", _PyCFunction_CAST(zlib_crc32), METH_FASTCALL, zlib_crc32__doc__},

static unsigned int
zlib_crc32_impl(PyObject *module, Py_buffer *data, unsigned int value);

static PyObject *
zlib_crc32(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};
    unsigned int value = 0;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("crc32", nargs, 1, 2)) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[1], &value, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
skip_optional:
    _return_value = zlib_crc32_impl(module, &data, value);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(zlib_crc32_combine__doc__,
"crc32_combine($module, crc1, crc2, len2, /)\n"
"--\n"
"\n"
"Combine two CRC-32 checksums into one.\n"
"\n"
"  crc1\n"
"    CRC-32 checksum for sequence A\n"
"  crc2\n"
"    CRC-32 checksum for sequence B\n"
"  len2\n"
"    Length of sequence B\n"
"\n"
"Given the CRC-32 checksum \'crc1\' of a sequence A and the\n"
"CRC-32 checksum \'crc2\' of a sequence B of length \'len2\',\n"
"return the CRC-32 checksum of A and B concatenated.");

#define ZLIB_CRC32_COMBINE_METHODDEF    \
    {"crc32_combine", _PyCFunction_CAST(zlib_crc32_combine), METH_FASTCALL, zlib_crc32_combine__doc__},

static unsigned int
zlib_crc32_combine_impl(PyObject *module, unsigned int crc1,
                        unsigned int crc2, PyObject *len2);

static PyObject *
zlib_crc32_combine(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    unsigned int crc1;
    unsigned int crc2;
    PyObject *len2;
    unsigned int _return_value;

    if (!_PyArg_CheckPositional("crc32_combine", nargs, 3, 3)) {
        goto exit;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[0], &crc1, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[1], &crc2, sizeof(unsigned int),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned int)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    if (!PyLong_Check(args[2])) {
        _PyArg_BadArgument("crc32_combine", "argument 3", "int", args[2]);
        goto exit;
    }
    len2 = args[2];
    _return_value = zlib_crc32_combine_impl(module, crc1, crc2, len2);
    if ((_return_value == (unsigned int)-1) && PyErr_Occurred()) {
        goto exit;
    }
    return_value = PyLong_FromUnsignedLong((unsigned long)_return_value);

exit:
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
/*[clinic end generated code: output=59184b81fea41d3d input=a9049054013a1b77]*/
