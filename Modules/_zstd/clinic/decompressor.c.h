/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_abstract.h"      // _PyNumber_Index()
#include "pycore_critical_section.h"// Py_BEGIN_CRITICAL_SECTION()
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_zstd_ZstdDecompressor_new__doc__,
"ZstdDecompressor(zstd_dict=None, options=None)\n"
"--\n"
"\n"
"Create a decompressor object for decompressing data incrementally.\n"
"\n"
"  zstd_dict\n"
"    A ZstdDict object, a pre-trained Zstandard dictionary.\n"
"  options\n"
"    A dict object that contains advanced decompression parameters.\n"
"\n"
"Thread-safe at method level. For one-shot decompression, use the decompress()\n"
"function instead.");

static PyObject *
_zstd_ZstdDecompressor_new_impl(PyTypeObject *type, PyObject *zstd_dict,
                                PyObject *options);

static PyObject *
_zstd_ZstdDecompressor_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
        .ob_item = { &_Py_ID(zstd_dict), &_Py_ID(options), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"zstd_dict", "options", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "ZstdDecompressor",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    PyObject *zstd_dict = Py_None;
    PyObject *options = Py_None;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        zstd_dict = fastargs[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    options = fastargs[1];
skip_optional_pos:
    return_value = _zstd_ZstdDecompressor_new_impl(type, zstd_dict, options);

exit:
    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdDecompressor_unused_data__doc__,
"A bytes object of un-consumed input data.\n"
"\n"
"When ZstdDecompressor object stops after a frame is\n"
"decompressed, unused input data after the frame. Otherwise this will be b\'\'.");
#if defined(_zstd_ZstdDecompressor_unused_data_DOCSTR)
#   undef _zstd_ZstdDecompressor_unused_data_DOCSTR
#endif
#define _zstd_ZstdDecompressor_unused_data_DOCSTR _zstd_ZstdDecompressor_unused_data__doc__

#if !defined(_zstd_ZstdDecompressor_unused_data_DOCSTR)
#  define _zstd_ZstdDecompressor_unused_data_DOCSTR NULL
#endif
#if defined(_ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF)
#  undef _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF
#  define _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF {"unused_data", (getter)_zstd_ZstdDecompressor_unused_data_get, (setter)_zstd_ZstdDecompressor_unused_data_set, _zstd_ZstdDecompressor_unused_data_DOCSTR},
#else
#  define _ZSTD_ZSTDDECOMPRESSOR_UNUSED_DATA_GETSETDEF {"unused_data", (getter)_zstd_ZstdDecompressor_unused_data_get, NULL, _zstd_ZstdDecompressor_unused_data_DOCSTR},
#endif

static PyObject *
_zstd_ZstdDecompressor_unused_data_get_impl(ZstdDecompressor *self);

static PyObject *
_zstd_ZstdDecompressor_unused_data_get(PyObject *self, void *Py_UNUSED(context))
{
    PyObject *return_value = NULL;

    Py_BEGIN_CRITICAL_SECTION(self);
    return_value = _zstd_ZstdDecompressor_unused_data_get_impl((ZstdDecompressor *)self);
    Py_END_CRITICAL_SECTION();

    return return_value;
}

PyDoc_STRVAR(_zstd_ZstdDecompressor_decompress__doc__,
"decompress($self, /, data, max_length=-1)\n"
"--\n"
"\n"
"Decompress *data*, returning uncompressed bytes if possible, or b\'\' otherwise.\n"
"\n"
"  data\n"
"    A bytes-like object, Zstandard data to be decompressed.\n"
"  max_length\n"
"    Maximum size of returned data. When it is negative, the size of\n"
"    output buffer is unlimited. When it is nonnegative, returns at\n"
"    most max_length bytes of decompressed data.\n"
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
"Attempting to decompress data after the end of a frame is reached raises an\n"
"EOFError. Any data found after the end of the frame is ignored and saved in\n"
"the self.unused_data attribute.");

#define _ZSTD_ZSTDDECOMPRESSOR_DECOMPRESS_METHODDEF    \
    {"decompress", _PyCFunction_CAST(_zstd_ZstdDecompressor_decompress), METH_FASTCALL|METH_KEYWORDS, _zstd_ZstdDecompressor_decompress__doc__},

static PyObject *
_zstd_ZstdDecompressor_decompress_impl(ZstdDecompressor *self,
                                       Py_buffer *data,
                                       Py_ssize_t max_length);

static PyObject *
_zstd_ZstdDecompressor_decompress(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
    return_value = _zstd_ZstdDecompressor_decompress_impl((ZstdDecompressor *)self, &data, max_length);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}
/*[clinic end generated code: output=7a4d278f9244e684 input=a9049054013a1b77]*/
