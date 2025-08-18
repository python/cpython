/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_encode__doc__,
"encode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Return an encoded string version of \'input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme. Default is\n"
"\'strict\' meaning that encoding errors raise a UnicodeEncodeError. Other possible\n"
"values are \'ignore\', \'replace\' and \'xmlcharrefreplace\' as well as any other name\n"
"registered with codecs.register_error that can handle UnicodeEncodeErrors.");

#define _MULTIBYTECODEC_MULTIBYTECODEC_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_encode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_encode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyObject *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_encode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(errors), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *input;
    const char *errors = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    input = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("encode", "argument 'errors'", "str or None", args[1]);
        goto exit;
    }
skip_optional_pos:
    return_value = _multibytecodec_MultibyteCodec_encode_impl((MultibyteCodecObject *)self, input, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_decode__doc__,
"decode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Decodes \'input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme. Default is\n"
"\'strict\' meaning that encoding errors raise a UnicodeDecodeError. Other possible\n"
"values are \'ignore\' and \'replace\' as well as any other name registered with\n"
"codecs.register_error that is able to handle UnicodeDecodeErrors.\"");

#define _MULTIBYTECODEC_MULTIBYTECODEC_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteCodec_decode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteCodec_decode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           Py_buffer *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_decode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(errors), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer input = {NULL, NULL};
    const char *errors = NULL;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &input, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1] == Py_None) {
        errors = NULL;
    }
    else if (PyUnicode_Check(args[1])) {
        Py_ssize_t errors_length;
        errors = PyUnicode_AsUTF8AndSize(args[1], &errors_length);
        if (errors == NULL) {
            goto exit;
        }
        if (strlen(errors) != (size_t)errors_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
    }
    else {
        _PyArg_BadArgument("decode", "argument 'errors'", "str or None", args[1]);
        goto exit;
    }
skip_optional_pos:
    return_value = _multibytecodec_MultibyteCodec_decode_impl((MultibyteCodecObject *)self, &input, errors);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_encode__doc__,
"encode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_ENCODE_METHODDEF    \
    {"encode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalEncoder_encode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalEncoder_encode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyObject *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(final), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "encode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *input;
    int final = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    input = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    final = PyObject_IsTrue(args[1]);
    if (final < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _multibytecodec_MultibyteIncrementalEncoder_encode_impl((MultibyteIncrementalEncoderObject *)self, input, final);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_getstate, METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate_impl(MultibyteIncrementalEncoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_getstate_impl((MultibyteIncrementalEncoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_setstate, METH_O, _multibytecodec_MultibyteIncrementalEncoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate_impl(MultibyteIncrementalEncoderObject *self,
                                                          PyLongObject *statelong);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_setstate(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyLongObject *statelong;

    if (!PyLong_Check(arg)) {
        _PyArg_BadArgument("setstate", "argument", "int", arg);
        goto exit;
    }
    statelong = (PyLongObject *)arg;
    return_value = _multibytecodec_MultibyteIncrementalEncoder_setstate_impl((MultibyteIncrementalEncoderObject *)self, statelong);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalEncoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALENCODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_reset, METH_NOARGS, _multibytecodec_MultibyteIncrementalEncoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset_impl(MultibyteIncrementalEncoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_reset_impl((MultibyteIncrementalEncoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF    \
    {"decode", _PyCFunction_CAST(_multibytecodec_MultibyteIncrementalDecoder_decode), METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteIncrementalDecoder_decode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        Py_buffer *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode(PyObject *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .ob_item = { &_Py_ID(input), &_Py_ID(final), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "decode",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    Py_buffer input = {NULL, NULL};
    int final = 0;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 2, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (PyObject_GetBuffer(args[0], &input, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    final = PyObject_IsTrue(args[1]);
    if (final < 0) {
        goto exit;
    }
skip_optional_pos:
    return_value = _multibytecodec_MultibyteIncrementalDecoder_decode_impl((MultibyteIncrementalDecoderObject *)self, &input, final);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_getstate, METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_getstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate_impl(MultibyteIncrementalDecoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_getstate(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalDecoder_getstate_impl((MultibyteIncrementalDecoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_setstate, METH_O, _multibytecodec_MultibyteIncrementalDecoder_setstate__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate_impl(MultibyteIncrementalDecoderObject *self,
                                                          PyObject *state);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_setstate(PyObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *state;

    if (!PyTuple_Check(arg)) {
        _PyArg_BadArgument("setstate", "argument", "tuple", arg);
        goto exit;
    }
    state = arg;
    return_value = _multibytecodec_MultibyteIncrementalDecoder_setstate_impl((MultibyteIncrementalDecoderObject *)self, state);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_reset, METH_NOARGS, _multibytecodec_MultibyteIncrementalDecoder_reset__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset_impl(MultibyteIncrementalDecoderObject *self);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalDecoder_reset_impl((MultibyteIncrementalDecoderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_read__doc__,
"read($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF    \
    {"read", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_read), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_read__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_read(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sizeobj = Py_None;

    if (!_PyArg_CheckPositional("read", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = args[0];
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_read_impl((MultibyteStreamReaderObject *)self, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readline__doc__,
"readline($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF    \
    {"readline", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readline), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_readline__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readline(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sizeobj = Py_None;

    if (!_PyArg_CheckPositional("readline", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizeobj = args[0];
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_readline_impl((MultibyteStreamReaderObject *)self, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readlines__doc__,
"readlines($self, sizehintobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF    \
    {"readlines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamReader_readlines), METH_FASTCALL, _multibytecodec_MultibyteStreamReader_readlines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyObject *sizehintobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines(PyObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *sizehintobj = Py_None;

    if (!_PyArg_CheckPositional("readlines", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    sizehintobj = args[0];
skip_optional:
    return_value = _multibytecodec_MultibyteStreamReader_readlines_impl((MultibyteStreamReaderObject *)self, sizehintobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteStreamReader_reset, METH_NOARGS, _multibytecodec_MultibyteStreamReader_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_reset_impl(MultibyteStreamReaderObject *self);

static PyObject *
_multibytecodec_MultibyteStreamReader_reset(PyObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteStreamReader_reset_impl((MultibyteStreamReaderObject *)self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_write__doc__,
"write($self, strobj, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF    \
    {"write", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_write), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_write__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_write_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls,
                                                 PyObject *strobj);

static PyObject *
_multibytecodec_MultibyteStreamWriter_write(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "write",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *strobj;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    strobj = args[0];
    return_value = _multibytecodec_MultibyteStreamWriter_write_impl((MultibyteStreamWriterObject *)self, cls, strobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF    \
    {"writelines", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_writelines), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_writelines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines_impl(MultibyteStreamWriterObject *self,
                                                      PyTypeObject *cls,
                                                      PyObject *lines);

static PyObject *
_multibytecodec_MultibyteStreamWriter_writelines(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
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
        .fname = "writelines",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *lines;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser,
            /*minpos*/ 1, /*maxpos*/ 1, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!args) {
        goto exit;
    }
    lines = args[0];
    return_value = _multibytecodec_MultibyteStreamWriter_writelines_impl((MultibyteStreamWriterObject *)self, cls, lines);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF    \
    {"reset", _PyCFunction_CAST(_multibytecodec_MultibyteStreamWriter_reset), METH_METHOD|METH_FASTCALL|METH_KEYWORDS, _multibytecodec_MultibyteStreamWriter_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self,
                                                 PyTypeObject *cls);

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset(PyObject *self, PyTypeObject *cls, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    if (nargs || (kwnames && PyTuple_GET_SIZE(kwnames))) {
        PyErr_SetString(PyExc_TypeError, "reset() takes no arguments");
        return NULL;
    }
    return _multibytecodec_MultibyteStreamWriter_reset_impl((MultibyteStreamWriterObject *)self, cls);
}

PyDoc_STRVAR(_multibytecodec___create_codec__doc__,
"__create_codec($module, arg, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC___CREATE_CODEC_METHODDEF    \
    {"__create_codec", (PyCFunction)_multibytecodec___create_codec, METH_O, _multibytecodec___create_codec__doc__},
/*[clinic end generated code: output=014f4f6bb9d29594 input=a9049054013a1b77]*/
