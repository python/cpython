/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_zstd_ZstdCompressor_new__doc__,
"ZstdCompressor(level=None, options=None, zstd_dict=None)\n"
"--\n"
"\n"
"Create a compressor object for compressing data incrementally.\n"
"\n"
"  level\n"
"    The compression level to use. Defaults to COMPRESSION_LEVEL_DEFAULT.\n"
"  options\n"
"    A dict object that contains advanced compression parameters.\n"
"  zstd_dict\n"
"    A ZstdDict object, a pre-trained Zstandard dictionary.\n"
"\n"
"Thread-safe at method level. For one-shot compression, use the compress()\n"
"function instead.");

static PyObject *
_zstd_ZstdCompressor_new_impl(PyTypeObject *type, PyObject *level,
                              PyObject *options, PyObject *zstd_dict);

static PyObject *
_zstd_ZstdCompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 3
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(level), &_Py_ID(options), &_Py_ID(zstd_dict), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"level", "options", "zstd_dict", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZstdCompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[3];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *level = Py_None;
    PyObject *options = Py_None;
    PyObject *zstd_dict = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 3, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        level = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        options = fastargs[1];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    zstd_dict = fastargs[2];
skip_optional_pos:
    return_value = _zstd_ZstdCompressor_new_impl(type, level, options, zstd_dict);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdCompressor_compress__doc__,
"compress($self, /, data, mode=ZstdCompressor.CONTINUE)\n"
"--\n"
"\n"
"Provide data to the compressor object.\n"
"\n"
"  mode\n"
"    Can be these 3 values ZstdCompressor.CONTINUE,\n"
"    ZstdCompressor.FLUSH_BLOCK, ZstdCompressor.FLUSH_FRAME\n"
"\n"
"Return a chunk of compressed data if possible, or b\'\' otherwise. When you have\n"
"finished providing data to the compressor, call the flush() method to finish\n"
"the compression process.");

#define _ZSTD_ZSTDCOMPRESSOR_COMPRESS_METHODDEF    \
    {"compress", _PyCFunction_CAST(_zstd_ZstdCompressor_compress), METH_FASTCALL|METH_KEYWORDS, _zstd_ZstdCompressor_compress__doc__},

static PyObject *
_zstd_ZstdCompressor_compress_impl(ZstdCompressor *self, Py_buffer *data,
                                   int mode);

static PyObject *
_zstd_ZstdCompressor_compress(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(data), &_Py_ID(mode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"data", "mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "compress",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer data = {NULL, NULL};
    int mode = ZSTD_e_continue;

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
    mode = PyLong_AsInt(args[1]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _zstd_ZstdCompressor_compress_impl((ZstdCompressor *)self, &data, mode);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdCompressor_flush__doc__,
"flush($self, /, mode=ZstdCompressor.FLUSH_FRAME)\n"
"--\n"
"\n"
"Finish the compression process.\n"
"\n"
"  mode\n"
"    Can be these 2 values ZstdCompressor.FLUSH_FRAME,\n"
"    ZstdCompressor.FLUSH_BLOCK\n"
"\n"
"Flush any remaining data left in internal buffers. Since Zstandard data\n"
"consists of one or more independent frames, the compressor object can still\n"
"be used after this method is called.");

#define _ZSTD_ZSTDCOMPRESSOR_FLUSH_METHODDEF    \
    {"flush", _PyCFunction_CAST(_zstd_ZstdCompressor_flush), METH_FASTCALL|METH_KEYWORDS, _zstd_ZstdCompressor_flush__doc__},

static PyObject *
_zstd_ZstdCompressor_flush_impl(ZstdCompressor *self, int mode);

static PyObject *
_zstd_ZstdCompressor_flush(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(mode), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"mode", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "flush",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    int mode = ZSTD_e_end;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 0, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    mode = PyLong_AsInt(args[0]);
    if (mode == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = _zstd_ZstdCompressor_flush_impl((ZstdCompressor *)self, mode);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdCompressor_set_pledged_input_size__doc__,
"set_pledged_input_size($self, size, /)\n"
"--\n"
"\n"
"Set the uncompressed content size to be written into the frame header.\n"
"\n"
"  size\n"
"    The size of the uncompressed data to be provided to the compressor.\n"
"\n"
"This method can be used to ensure the header of the frame about to be written\n"
"includes the size of the data, unless the CompressionParameter.content_size_flag\n"
"is set to False. If last_mode != FLUSH_FRAME, then a RuntimeError is raised.\n"
"\n"
"It is important to ensure that the pledged data size matches the actual data\n"
"size. If they do not match the compressed output data may be corrupted and the\n"
"final chunk written may be lost.");

#define _ZSTD_ZSTDCOMPRESSOR_SET_PLEDGED_INPUT_SIZE_METHODDEF    \
    {"set_pledged_input_size", (PyCFunction)_zstd_ZstdCompressor_set_pledged_input_size, METH_O, _zstd_ZstdCompressor_set_pledged_input_size__doc__},

static PyObject *
_zstd_ZstdCompressor_set_pledged_input_size_impl(ZstdCompressor *self,
                                                 unsigned long long size);

static PyObject *
_zstd_ZstdCompressor_set_pledged_input_size(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    unsigned long long size;

    if (!zstd_contentsize_converter(arg, &size)) {
        goto exit;
    }
    return_value = _zstd_ZstdCompressor_set_pledged_input_size_impl((ZstdCompressor *)self, size);

exit:
    return return_value;
}
/*[clinic end generated code: output=c1d5c2cf06a8becd input=a9049054013a1b77]*/
