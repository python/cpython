/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_multibytecodec_MultibyteCodec_encode__doc__,
"encode($self, /, input, errors=None)\n"
"--\n"
"\n"
"Return an encoded string version of `input\'.\n"
"\n"
"\'errors\' may be given to set a different error handling scheme. Default is\n"
"\'strict\' meaning that encoding errors raise a UnicodeEncodeError. Other possible\n"
"values are \'ignore\', \'replace\' and \'xmlcharrefreplace\' as well as any other name\n"
"registered with codecs.register_error that can handle UnicodeEncodeErrors.");

#define _MULTIBYTECODEC_MULTIBYTECODEC_ENCODE_METHODDEF    \
    {"encode", (PyCFunction)_multibytecodec_MultibyteCodec_encode, METH_FASTCALL, _multibytecodec_MultibyteCodec_encode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_encode_impl(MultibyteCodecObject *self,
                                           PyObject *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_encode(MultibyteCodecObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {"O|z:encode", _keywords, 0};
    PyObject *input;
    const char *errors = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &input, &errors)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteCodec_encode_impl(self, input, errors);

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
    {"decode", (PyCFunction)_multibytecodec_MultibyteCodec_decode, METH_FASTCALL, _multibytecodec_MultibyteCodec_decode__doc__},

static PyObject *
_multibytecodec_MultibyteCodec_decode_impl(MultibyteCodecObject *self,
                                           Py_buffer *input,
                                           const char *errors);

static PyObject *
_multibytecodec_MultibyteCodec_decode(MultibyteCodecObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "errors", NULL};
    static _PyArg_Parser _parser = {"y*|z:decode", _keywords, 0};
    Py_buffer input = {NULL, NULL};
    const char *errors = NULL;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &input, &errors)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteCodec_decode_impl(self, &input, errors);

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
    {"encode", (PyCFunction)_multibytecodec_MultibyteIncrementalEncoder_encode, METH_FASTCALL, _multibytecodec_MultibyteIncrementalEncoder_encode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode_impl(MultibyteIncrementalEncoderObject *self,
                                                        PyObject *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalEncoder_encode(MultibyteIncrementalEncoderObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {"O|i:encode", _keywords, 0};
    PyObject *input;
    int final = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &input, &final)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalEncoder_encode_impl(self, input, final);

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
_multibytecodec_MultibyteIncrementalEncoder_reset(MultibyteIncrementalEncoderObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalEncoder_reset_impl(self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteIncrementalDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTEINCREMENTALDECODER_DECODE_METHODDEF    \
    {"decode", (PyCFunction)_multibytecodec_MultibyteIncrementalDecoder_decode, METH_FASTCALL, _multibytecodec_MultibyteIncrementalDecoder_decode__doc__},

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode_impl(MultibyteIncrementalDecoderObject *self,
                                                        Py_buffer *input,
                                                        int final);

static PyObject *
_multibytecodec_MultibyteIncrementalDecoder_decode(MultibyteIncrementalDecoderObject *self, PyObject **args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"input", "final", NULL};
    static _PyArg_Parser _parser = {"y*|i:decode", _keywords, 0};
    Py_buffer input = {NULL, NULL};
    int final = 0;

    if (!_PyArg_ParseStack(args, nargs, kwnames, &_parser,
        &input, &final)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteIncrementalDecoder_decode_impl(self, &input, final);

exit:
    /* Cleanup for input */
    if (input.obj) {
       PyBuffer_Release(&input);
    }

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
_multibytecodec_MultibyteIncrementalDecoder_reset(MultibyteIncrementalDecoderObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteIncrementalDecoder_reset_impl(self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_read__doc__,
"read($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READ_METHODDEF    \
    {"read", (PyCFunction)_multibytecodec_MultibyteStreamReader_read, METH_VARARGS, _multibytecodec_MultibyteStreamReader_read__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_read_impl(MultibyteStreamReaderObject *self,
                                                PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_read(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *sizeobj = Py_None;

    if (!PyArg_UnpackTuple(args, "read",
        0, 1,
        &sizeobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_read_impl(self, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readline__doc__,
"readline($self, sizeobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINE_METHODDEF    \
    {"readline", (PyCFunction)_multibytecodec_MultibyteStreamReader_readline, METH_VARARGS, _multibytecodec_MultibyteStreamReader_readline__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readline_impl(MultibyteStreamReaderObject *self,
                                                    PyObject *sizeobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readline(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *sizeobj = Py_None;

    if (!PyArg_UnpackTuple(args, "readline",
        0, 1,
        &sizeobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_readline_impl(self, sizeobj);

exit:
    return return_value;
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamReader_readlines__doc__,
"readlines($self, sizehintobj=None, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMREADER_READLINES_METHODDEF    \
    {"readlines", (PyCFunction)_multibytecodec_MultibyteStreamReader_readlines, METH_VARARGS, _multibytecodec_MultibyteStreamReader_readlines__doc__},

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines_impl(MultibyteStreamReaderObject *self,
                                                     PyObject *sizehintobj);

static PyObject *
_multibytecodec_MultibyteStreamReader_readlines(MultibyteStreamReaderObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *sizehintobj = Py_None;

    if (!PyArg_UnpackTuple(args, "readlines",
        0, 1,
        &sizehintobj)) {
        goto exit;
    }
    return_value = _multibytecodec_MultibyteStreamReader_readlines_impl(self, sizehintobj);

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
_multibytecodec_MultibyteStreamReader_reset(MultibyteStreamReaderObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteStreamReader_reset_impl(self);
}

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_write__doc__,
"write($self, strobj, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITE_METHODDEF    \
    {"write", (PyCFunction)_multibytecodec_MultibyteStreamWriter_write, METH_O, _multibytecodec_MultibyteStreamWriter_write__doc__},

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_writelines__doc__,
"writelines($self, lines, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_WRITELINES_METHODDEF    \
    {"writelines", (PyCFunction)_multibytecodec_MultibyteStreamWriter_writelines, METH_O, _multibytecodec_MultibyteStreamWriter_writelines__doc__},

PyDoc_STRVAR(_multibytecodec_MultibyteStreamWriter_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC_MULTIBYTESTREAMWRITER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_multibytecodec_MultibyteStreamWriter_reset, METH_NOARGS, _multibytecodec_MultibyteStreamWriter_reset__doc__},

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset_impl(MultibyteStreamWriterObject *self);

static PyObject *
_multibytecodec_MultibyteStreamWriter_reset(MultibyteStreamWriterObject *self, PyObject *Py_UNUSED(ignored))
{
    return _multibytecodec_MultibyteStreamWriter_reset_impl(self);
}

PyDoc_STRVAR(_multibytecodec___create_codec__doc__,
"__create_codec($module, arg, /)\n"
"--\n"
"\n");

#define _MULTIBYTECODEC___CREATE_CODEC_METHODDEF    \
    {"__create_codec", (PyCFunction)_multibytecodec___create_codec, METH_O, _multibytecodec___create_codec__doc__},
/*[clinic end generated code: output=134b9e36cb985939 input=a9049054013a1b77]*/
