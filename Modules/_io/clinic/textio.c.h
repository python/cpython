/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(_io_IncrementalNewlineDecoder___init____doc__,
"IncrementalNewlineDecoder(decoder, translate, errors=\'strict\')\n"
"--\n"
"\n"
"Codec used when reading a file in universal newlines mode.\n"
"\n"
"It wraps another incremental decoder, translating \\r\\n and \\r into \\n.\n"
"It also records the types of newlines encountered.  When used with\n"
"translate=False, it ensures that the newline sequence is returned in\n"
"one piece. When used with decoder=None, it expects unicode strings as\n"
"decode input and translates newlines without first invoking an external\n"
"decoder.");

static int
_io_IncrementalNewlineDecoder___init___impl(nldecoder_object *self,
                                            PyObject *decoder, int translate,
                                            PyObject *errors);

static int
_io_IncrementalNewlineDecoder___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"decoder", "translate", "errors", NULL};
    PyObject *decoder;
    int translate;
    PyObject *errors = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|O:IncrementalNewlineDecoder", _keywords,
        &decoder, &translate, &errors))
        goto exit;
    return_value = _io_IncrementalNewlineDecoder___init___impl((nldecoder_object *)self, decoder, translate, errors);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_decode__doc__,
"decode($self, /, input, final=False)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_DECODE_METHODDEF    \
    {"decode", (PyCFunction)_io_IncrementalNewlineDecoder_decode, METH_VARARGS|METH_KEYWORDS, _io_IncrementalNewlineDecoder_decode__doc__},

static PyObject *
_io_IncrementalNewlineDecoder_decode_impl(nldecoder_object *self,
                                          PyObject *input, int final);

static PyObject *
_io_IncrementalNewlineDecoder_decode(nldecoder_object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"input", "final", NULL};
    PyObject *input;
    int final = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|i:decode", _keywords,
        &input, &final))
        goto exit;
    return_value = _io_IncrementalNewlineDecoder_decode_impl(self, input, final);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_getstate__doc__,
"getstate($self, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_GETSTATE_METHODDEF    \
    {"getstate", (PyCFunction)_io_IncrementalNewlineDecoder_getstate, METH_NOARGS, _io_IncrementalNewlineDecoder_getstate__doc__},

static PyObject *
_io_IncrementalNewlineDecoder_getstate_impl(nldecoder_object *self);

static PyObject *
_io_IncrementalNewlineDecoder_getstate(nldecoder_object *self, PyObject *Py_UNUSED(ignored))
{
    return _io_IncrementalNewlineDecoder_getstate_impl(self);
}

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_setstate__doc__,
"setstate($self, state, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_SETSTATE_METHODDEF    \
    {"setstate", (PyCFunction)_io_IncrementalNewlineDecoder_setstate, METH_O, _io_IncrementalNewlineDecoder_setstate__doc__},

PyDoc_STRVAR(_io_IncrementalNewlineDecoder_reset__doc__,
"reset($self, /)\n"
"--\n"
"\n");

#define _IO_INCREMENTALNEWLINEDECODER_RESET_METHODDEF    \
    {"reset", (PyCFunction)_io_IncrementalNewlineDecoder_reset, METH_NOARGS, _io_IncrementalNewlineDecoder_reset__doc__},

static PyObject *
_io_IncrementalNewlineDecoder_reset_impl(nldecoder_object *self);

static PyObject *
_io_IncrementalNewlineDecoder_reset(nldecoder_object *self, PyObject *Py_UNUSED(ignored))
{
    return _io_IncrementalNewlineDecoder_reset_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper___init____doc__,
"TextIOWrapper(buffer, encoding=None, errors=None, newline=None,\n"
"              line_buffering=False, write_through=False)\n"
"--\n"
"\n"
"Character and line based layer over a BufferedIOBase object, buffer.\n"
"\n"
"encoding gives the name of the encoding that the stream will be\n"
"decoded or encoded with. It defaults to locale.getpreferredencoding(False).\n"
"\n"
"errors determines the strictness of encoding and decoding (see\n"
"help(codecs.Codec) or the documentation for codecs.register) and\n"
"defaults to \"strict\".\n"
"\n"
"newline controls how line endings are handled. It can be None, \'\',\n"
"\'\\n\', \'\\r\', and \'\\r\\n\'.  It works as follows:\n"
"\n"
"* On input, if newline is None, universal newlines mode is\n"
"  enabled. Lines in the input can end in \'\\n\', \'\\r\', or \'\\r\\n\', and\n"
"  these are translated into \'\\n\' before being returned to the\n"
"  caller. If it is \'\', universal newline mode is enabled, but line\n"
"  endings are returned to the caller untranslated. If it has any of\n"
"  the other legal values, input lines are only terminated by the given\n"
"  string, and the line ending is returned to the caller untranslated.\n"
"\n"
"* On output, if newline is None, any \'\\n\' characters written are\n"
"  translated to the system default line separator, os.linesep. If\n"
"  newline is \'\' or \'\\n\', no translation takes place. If newline is any\n"
"  of the other legal values, any \'\\n\' characters written are translated\n"
"  to the given string.\n"
"\n"
"If line_buffering is True, a call to flush is implied when a call to\n"
"write contains a newline character.");

static int
_io_TextIOWrapper___init___impl(textio *self, PyObject *buffer,
                                const char *encoding, const char *errors,
                                const char *newline, int line_buffering,
                                int write_through);

static int
_io_TextIOWrapper___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    static char *_keywords[] = {"buffer", "encoding", "errors", "newline", "line_buffering", "write_through", NULL};
    PyObject *buffer;
    const char *encoding = NULL;
    const char *errors = NULL;
    const char *newline = NULL;
    int line_buffering = 0;
    int write_through = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|zzzii:TextIOWrapper", _keywords,
        &buffer, &encoding, &errors, &newline, &line_buffering, &write_through))
        goto exit;
    return_value = _io_TextIOWrapper___init___impl((textio *)self, buffer, encoding, errors, newline, line_buffering, write_through);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_detach__doc__,
"detach($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_DETACH_METHODDEF    \
    {"detach", (PyCFunction)_io_TextIOWrapper_detach, METH_NOARGS, _io_TextIOWrapper_detach__doc__},

static PyObject *
_io_TextIOWrapper_detach_impl(textio *self);

static PyObject *
_io_TextIOWrapper_detach(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_detach_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_write__doc__,
"write($self, text, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_WRITE_METHODDEF    \
    {"write", (PyCFunction)_io_TextIOWrapper_write, METH_O, _io_TextIOWrapper_write__doc__},

static PyObject *
_io_TextIOWrapper_write_impl(textio *self, PyObject *text);

static PyObject *
_io_TextIOWrapper_write(textio *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    PyObject *text;

    if (!PyArg_Parse(arg, "U:write", &text))
        goto exit;
    return_value = _io_TextIOWrapper_write_impl(self, text);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_read__doc__,
"read($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READ_METHODDEF    \
    {"read", (PyCFunction)_io_TextIOWrapper_read, METH_VARARGS, _io_TextIOWrapper_read__doc__},

static PyObject *
_io_TextIOWrapper_read_impl(textio *self, Py_ssize_t n);

static PyObject *
_io_TextIOWrapper_read(textio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t n = -1;

    if (!PyArg_ParseTuple(args, "|O&:read",
        _PyIO_ConvertSsize_t, &n))
        goto exit;
    return_value = _io_TextIOWrapper_read_impl(self, n);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_readline__doc__,
"readline($self, size=-1, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READLINE_METHODDEF    \
    {"readline", (PyCFunction)_io_TextIOWrapper_readline, METH_VARARGS, _io_TextIOWrapper_readline__doc__},

static PyObject *
_io_TextIOWrapper_readline_impl(textio *self, Py_ssize_t size);

static PyObject *
_io_TextIOWrapper_readline(textio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    Py_ssize_t size = -1;

    if (!PyArg_ParseTuple(args, "|n:readline",
        &size))
        goto exit;
    return_value = _io_TextIOWrapper_readline_impl(self, size);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_seek__doc__,
"seek($self, cookie, whence=0, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_SEEK_METHODDEF    \
    {"seek", (PyCFunction)_io_TextIOWrapper_seek, METH_VARARGS, _io_TextIOWrapper_seek__doc__},

static PyObject *
_io_TextIOWrapper_seek_impl(textio *self, PyObject *cookieObj, int whence);

static PyObject *
_io_TextIOWrapper_seek(textio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *cookieObj;
    int whence = 0;

    if (!PyArg_ParseTuple(args, "O|i:seek",
        &cookieObj, &whence))
        goto exit;
    return_value = _io_TextIOWrapper_seek_impl(self, cookieObj, whence);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_tell__doc__,
"tell($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_TELL_METHODDEF    \
    {"tell", (PyCFunction)_io_TextIOWrapper_tell, METH_NOARGS, _io_TextIOWrapper_tell__doc__},

static PyObject *
_io_TextIOWrapper_tell_impl(textio *self);

static PyObject *
_io_TextIOWrapper_tell(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_tell_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_truncate__doc__,
"truncate($self, pos=None, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_TRUNCATE_METHODDEF    \
    {"truncate", (PyCFunction)_io_TextIOWrapper_truncate, METH_VARARGS, _io_TextIOWrapper_truncate__doc__},

static PyObject *
_io_TextIOWrapper_truncate_impl(textio *self, PyObject *pos);

static PyObject *
_io_TextIOWrapper_truncate(textio *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *pos = Py_None;

    if (!PyArg_UnpackTuple(args, "truncate",
        0, 1,
        &pos))
        goto exit;
    return_value = _io_TextIOWrapper_truncate_impl(self, pos);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_TextIOWrapper_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)_io_TextIOWrapper_fileno, METH_NOARGS, _io_TextIOWrapper_fileno__doc__},

static PyObject *
_io_TextIOWrapper_fileno_impl(textio *self);

static PyObject *
_io_TextIOWrapper_fileno(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_fileno_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_seekable__doc__,
"seekable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_SEEKABLE_METHODDEF    \
    {"seekable", (PyCFunction)_io_TextIOWrapper_seekable, METH_NOARGS, _io_TextIOWrapper_seekable__doc__},

static PyObject *
_io_TextIOWrapper_seekable_impl(textio *self);

static PyObject *
_io_TextIOWrapper_seekable(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_seekable_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_readable__doc__,
"readable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_READABLE_METHODDEF    \
    {"readable", (PyCFunction)_io_TextIOWrapper_readable, METH_NOARGS, _io_TextIOWrapper_readable__doc__},

static PyObject *
_io_TextIOWrapper_readable_impl(textio *self);

static PyObject *
_io_TextIOWrapper_readable(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_readable_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_writable__doc__,
"writable($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_WRITABLE_METHODDEF    \
    {"writable", (PyCFunction)_io_TextIOWrapper_writable, METH_NOARGS, _io_TextIOWrapper_writable__doc__},

static PyObject *
_io_TextIOWrapper_writable_impl(textio *self);

static PyObject *
_io_TextIOWrapper_writable(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_writable_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_isatty__doc__,
"isatty($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_ISATTY_METHODDEF    \
    {"isatty", (PyCFunction)_io_TextIOWrapper_isatty, METH_NOARGS, _io_TextIOWrapper_isatty__doc__},

static PyObject *
_io_TextIOWrapper_isatty_impl(textio *self);

static PyObject *
_io_TextIOWrapper_isatty(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_isatty_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_flush__doc__,
"flush($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_FLUSH_METHODDEF    \
    {"flush", (PyCFunction)_io_TextIOWrapper_flush, METH_NOARGS, _io_TextIOWrapper_flush__doc__},

static PyObject *
_io_TextIOWrapper_flush_impl(textio *self);

static PyObject *
_io_TextIOWrapper_flush(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_flush_impl(self);
}

PyDoc_STRVAR(_io_TextIOWrapper_close__doc__,
"close($self, /)\n"
"--\n"
"\n");

#define _IO_TEXTIOWRAPPER_CLOSE_METHODDEF    \
    {"close", (PyCFunction)_io_TextIOWrapper_close, METH_NOARGS, _io_TextIOWrapper_close__doc__},

static PyObject *
_io_TextIOWrapper_close_impl(textio *self);

static PyObject *
_io_TextIOWrapper_close(textio *self, PyObject *Py_UNUSED(ignored))
{
    return _io_TextIOWrapper_close_impl(self);
}
/*[clinic end generated code: output=690608f85aab8ba5 input=a9049054013a1b77]*/
