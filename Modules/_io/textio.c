/*
    An implementation of Text I/O as defined by PEP 3116 - "New I/O"

    Classes defined here: TextIOBase, IncrementalNewlineDecoder, TextIOWrapper.

    Written by Amaury Forgeot d'Arc and Antoine Pitrou
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "_iomodule.h"

/* TextIOBase */

PyDoc_STRVAR(textiobase_doc,
    "Base class for text I/O.\n"
    "\n"
    "This class provides a character and line based interface to stream\n"
    "I/O. There is no readinto method because Python's character strings\n"
    "are immutable. There is no public constructor.\n"
    );

static PyObject *
_unsupported(const char *message)
{
    PyErr_SetString(IO_STATE->unsupported_operation, message);
    return NULL;
}

PyDoc_STRVAR(textiobase_detach_doc,
    "Separate the underlying buffer from the TextIOBase and return it.\n"
    "\n"
    "After the underlying buffer has been detached, the TextIO is in an\n"
    "unusable state.\n"
    );

static PyObject *
textiobase_detach(PyObject *self)
{
    return _unsupported("detach");
}

PyDoc_STRVAR(textiobase_read_doc,
    "Read at most n characters from stream.\n"
    "\n"
    "Read from underlying buffer until we have n characters or we hit EOF.\n"
    "If n is negative or omitted, read until EOF.\n"
    );

static PyObject *
textiobase_read(PyObject *self, PyObject *args)
{
    return _unsupported("read");
}

PyDoc_STRVAR(textiobase_readline_doc,
    "Read until newline or EOF.\n"
    "\n"
    "Returns an empty string if EOF is hit immediately.\n"
    );

static PyObject *
textiobase_readline(PyObject *self, PyObject *args)
{
    return _unsupported("readline");
}

PyDoc_STRVAR(textiobase_write_doc,
    "Write string to stream.\n"
    "Returns the number of characters written (which is always equal to\n"
    "the length of the string).\n"
    );

static PyObject *
textiobase_write(PyObject *self, PyObject *args)
{
    return _unsupported("write");
}

PyDoc_STRVAR(textiobase_encoding_doc,
    "Encoding of the text stream.\n"
    "\n"
    "Subclasses should override.\n"
    );

static PyObject *
textiobase_encoding_get(PyObject *self, void *context)
{
    Py_RETURN_NONE;
}

PyDoc_STRVAR(textiobase_newlines_doc,
    "Line endings translated so far.\n"
    "\n"
    "Only line endings translated during reading are considered.\n"
    "\n"
    "Subclasses should override.\n"
    );

static PyObject *
textiobase_newlines_get(PyObject *self, void *context)
{
    Py_RETURN_NONE;
}

PyDoc_STRVAR(textiobase_errors_doc,
    "The error setting of the decoder or encoder.\n"
    "\n"
    "Subclasses should override.\n"
    );

static PyObject *
textiobase_errors_get(PyObject *self, void *context)
{
    Py_RETURN_NONE;
}


static PyMethodDef textiobase_methods[] = {
    {"detach", (PyCFunction)textiobase_detach, METH_NOARGS, textiobase_detach_doc},
    {"read", textiobase_read, METH_VARARGS, textiobase_read_doc},
    {"readline", textiobase_readline, METH_VARARGS, textiobase_readline_doc},
    {"write", textiobase_write, METH_VARARGS, textiobase_write_doc},
    {NULL, NULL}
};

static PyGetSetDef textiobase_getset[] = {
    {"encoding", (getter)textiobase_encoding_get, NULL, textiobase_encoding_doc},
    {"newlines", (getter)textiobase_newlines_get, NULL, textiobase_newlines_doc},
    {"errors", (getter)textiobase_errors_get, NULL, textiobase_errors_doc},
    {NULL}
};

PyTypeObject PyTextIOBase_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io._TextIOBase",          /*tp_name*/
    0,                          /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    0,                          /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    textiobase_doc,             /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /* tp_weaklistoffset */
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    textiobase_methods,         /* tp_methods */
    0,                          /* tp_members */
    textiobase_getset,          /* tp_getset */
    &PyIOBase_Type,             /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    0,                          /* tp_init */
    0,                          /* tp_alloc */
    0,                          /* tp_new */
};


/* IncrementalNewlineDecoder */

PyDoc_STRVAR(incrementalnewlinedecoder_doc,
    "Codec used when reading a file in universal newlines mode.  It wraps\n"
    "another incremental decoder, translating \\r\\n and \\r into \\n.  It also\n"
    "records the types of newlines encountered.  When used with\n"
    "translate=False, it ensures that the newline sequence is returned in\n"
    "one piece. When used with decoder=None, it expects unicode strings as\n"
    "decode input and translates newlines without first invoking an external\n"
    "decoder.\n"
    );

typedef struct {
    PyObject_HEAD
    PyObject *decoder;
    PyObject *errors;
    signed int pendingcr: 1;
    signed int translate: 1;
    unsigned int seennl: 3;
} nldecoder_object;

static int
incrementalnewlinedecoder_init(nldecoder_object *self,
                               PyObject *args, PyObject *kwds)
{
    PyObject *decoder;
    int translate;
    PyObject *errors = NULL;
    char *kwlist[] = {"decoder", "translate", "errors", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "Oi|O:IncrementalNewlineDecoder",
                                     kwlist, &decoder, &translate, &errors))
        return -1;

    self->decoder = decoder;
    Py_INCREF(decoder);

    if (errors == NULL) {
        self->errors = PyUnicode_FromString("strict");
        if (self->errors == NULL)
            return -1;
    }
    else {
        Py_INCREF(errors);
        self->errors = errors;
    }

    self->translate = translate;
    self->seennl = 0;
    self->pendingcr = 0;

    return 0;
}

static void
incrementalnewlinedecoder_dealloc(nldecoder_object *self)
{
    Py_CLEAR(self->decoder);
    Py_CLEAR(self->errors);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

#define SEEN_CR   1
#define SEEN_LF   2
#define SEEN_CRLF 4
#define SEEN_ALL (SEEN_CR | SEEN_LF | SEEN_CRLF)

PyObject *
_PyIncrementalNewlineDecoder_decode(PyObject *_self,
                                    PyObject *input, int final)
{
    PyObject *output;
    Py_ssize_t output_len;
    nldecoder_object *self = (nldecoder_object *) _self;

    if (self->decoder == NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "IncrementalNewlineDecoder.__init__ not called");
        return NULL;
    }

    /* decode input (with the eventual \r from a previous pass) */
    if (self->decoder != Py_None) {
        output = PyObject_CallMethodObjArgs(self->decoder,
            _PyIO_str_decode, input, final ? Py_True : Py_False, NULL);
    }
    else {
        output = input;
        Py_INCREF(output);
    }

    if (output == NULL)
        return NULL;

    if (!PyUnicode_Check(output)) {
        PyErr_SetString(PyExc_TypeError,
                        "decoder should return a string result");
        goto error;
    }

    output_len = PyUnicode_GET_SIZE(output);
    if (self->pendingcr && (final || output_len > 0)) {
        Py_UNICODE *out;
        PyObject *modified = PyUnicode_FromUnicode(NULL, output_len + 1);
        if (modified == NULL)
            goto error;
        out = PyUnicode_AS_UNICODE(modified);
        out[0] = '\r';
        memcpy(out + 1, PyUnicode_AS_UNICODE(output),
               output_len * sizeof(Py_UNICODE));
        Py_DECREF(output);
        output = modified;
        self->pendingcr = 0;
        output_len++;
    }

    /* retain last \r even when not translating data:
     * then readline() is sure to get \r\n in one pass
     */
    if (!final) {
        if (output_len > 0
            && PyUnicode_AS_UNICODE(output)[output_len - 1] == '\r') {

            if (Py_REFCNT(output) == 1) {
                if (PyUnicode_Resize(&output, output_len - 1) < 0)
                    goto error;
            }
            else {
                PyObject *modified = PyUnicode_FromUnicode(
                    PyUnicode_AS_UNICODE(output),
                    output_len - 1);
                if (modified == NULL)
                    goto error;
                Py_DECREF(output);
                output = modified;
            }
            self->pendingcr = 1;
        }
    }

    /* Record which newlines are read and do newline translation if desired,
       all in one pass. */
    {
        Py_UNICODE *in_str;
        Py_ssize_t len;
        int seennl = self->seennl;
        int only_lf = 0;

        in_str = PyUnicode_AS_UNICODE(output);
        len = PyUnicode_GET_SIZE(output);

        if (len == 0)
            return output;

        /* If, up to now, newlines are consistently \n, do a quick check
           for the \r *byte* with the libc's optimized memchr.
           */
        if (seennl == SEEN_LF || seennl == 0) {
            only_lf = (memchr(in_str, '\r', len * sizeof(Py_UNICODE)) == NULL);
        }

        if (only_lf) {
            /* If not already seen, quick scan for a possible "\n" character.
               (there's nothing else to be done, even when in translation mode)
            */
            if (seennl == 0 &&
                memchr(in_str, '\n', len * sizeof(Py_UNICODE)) != NULL) {
                Py_UNICODE *s, *end;
                s = in_str;
                end = in_str + len;
                for (;;) {
                    Py_UNICODE c;
                    /* Fast loop for non-control characters */
                    while (*s > '\n')
                        s++;
                    c = *s++;
                    if (c == '\n') {
                        seennl |= SEEN_LF;
                        break;
                    }
                    if (s > end)
                        break;
                }
            }
            /* Finished: we have scanned for newlines, and none of them
               need translating */
        }
        else if (!self->translate) {
            Py_UNICODE *s, *end;
            /* We have already seen all newline types, no need to scan again */
            if (seennl == SEEN_ALL)
                goto endscan;
            s = in_str;
            end = in_str + len;
            for (;;) {
                Py_UNICODE c;
                /* Fast loop for non-control characters */
                while (*s > '\r')
                    s++;
                c = *s++;
                if (c == '\n')
                    seennl |= SEEN_LF;
                else if (c == '\r') {
                    if (*s == '\n') {
                        seennl |= SEEN_CRLF;
                        s++;
                    }
                    else
                        seennl |= SEEN_CR;
                }
                if (s > end)
                    break;
                if (seennl == SEEN_ALL)
                    break;
            }
        endscan:
            ;
        }
        else {
            PyObject *translated = NULL;
            Py_UNICODE *out_str;
            Py_UNICODE *in, *out, *end;
            if (Py_REFCNT(output) != 1) {
                /* We could try to optimize this so that we only do a copy
                   when there is something to translate. On the other hand,
                   most decoders should only output non-shared strings, i.e.
                   translation is done in place. */
                translated = PyUnicode_FromUnicode(NULL, len);
                if (translated == NULL)
                    goto error;
                assert(Py_REFCNT(translated) == 1);
                memcpy(PyUnicode_AS_UNICODE(translated),
                       PyUnicode_AS_UNICODE(output),
                       len * sizeof(Py_UNICODE));
            }
            else {
                translated = output;
            }
            out_str = PyUnicode_AS_UNICODE(translated);
            in = in_str;
            out = out_str;
            end = in_str + len;
            for (;;) {
                Py_UNICODE c;
                /* Fast loop for non-control characters */
                while ((c = *in++) > '\r')
                    *out++ = c;
                if (c == '\n') {
                    *out++ = c;
                    seennl |= SEEN_LF;
                    continue;
                }
                if (c == '\r') {
                    if (*in == '\n') {
                        in++;
                        seennl |= SEEN_CRLF;
                    }
                    else
                        seennl |= SEEN_CR;
                    *out++ = '\n';
                    continue;
                }
                if (in > end)
                    break;
                *out++ = c;
            }
            if (translated != output) {
                Py_DECREF(output);
                output = translated;
            }
            if (out - out_str != len) {
                if (PyUnicode_Resize(&output, out - out_str) < 0)
                    goto error;
            }
        }
        self->seennl |= seennl;
    }

    return output;

  error:
    Py_DECREF(output);
    return NULL;
}

static PyObject *
incrementalnewlinedecoder_decode(nldecoder_object *self,
                                 PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"input", "final", NULL};
    PyObject *input;
    int final = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|i:IncrementalNewlineDecoder",
                                     kwlist, &input, &final))
        return NULL;
    return _PyIncrementalNewlineDecoder_decode((PyObject *) self, input, final);
}

static PyObject *
incrementalnewlinedecoder_getstate(nldecoder_object *self, PyObject *args)
{
    PyObject *buffer;
    unsigned PY_LONG_LONG flag;

    if (self->decoder != Py_None) {
        PyObject *state = PyObject_CallMethodObjArgs(self->decoder,
           _PyIO_str_getstate, NULL);
        if (state == NULL)
            return NULL;
        if (!PyArg_Parse(state, "(OK)", &buffer, &flag)) {
            Py_DECREF(state);
            return NULL;
        }
        Py_INCREF(buffer);
        Py_DECREF(state);
    }
    else {
        buffer = PyBytes_FromString("");
        flag = 0;
    }
    flag <<= 1;
    if (self->pendingcr)
        flag |= 1;
    return Py_BuildValue("NK", buffer, flag);
}

static PyObject *
incrementalnewlinedecoder_setstate(nldecoder_object *self, PyObject *state)
{
    PyObject *buffer;
    unsigned PY_LONG_LONG flag;

    if (!PyArg_Parse(state, "(OK)", &buffer, &flag))
        return NULL;

    self->pendingcr = (int) flag & 1;
    flag >>= 1;

    if (self->decoder != Py_None)
        return PyObject_CallMethod(self->decoder,
                                   "setstate", "((OK))", buffer, flag);
    else
        Py_RETURN_NONE;
}

static PyObject *
incrementalnewlinedecoder_reset(nldecoder_object *self, PyObject *args)
{
    self->seennl = 0;
    self->pendingcr = 0;
    if (self->decoder != Py_None)
        return PyObject_CallMethodObjArgs(self->decoder, _PyIO_str_reset, NULL);
    else
        Py_RETURN_NONE;
}

static PyObject *
incrementalnewlinedecoder_newlines_get(nldecoder_object *self, void *context)
{
    switch (self->seennl) {
    case SEEN_CR:
        return PyUnicode_FromString("\r");
    case SEEN_LF:
        return PyUnicode_FromString("\n");
    case SEEN_CRLF:
        return PyUnicode_FromString("\r\n");
    case SEEN_CR | SEEN_LF:
        return Py_BuildValue("ss", "\r", "\n");
    case SEEN_CR | SEEN_CRLF:
        return Py_BuildValue("ss", "\r", "\r\n");
    case SEEN_LF | SEEN_CRLF:
        return Py_BuildValue("ss", "\n", "\r\n");
    case SEEN_CR | SEEN_LF | SEEN_CRLF:
        return Py_BuildValue("sss", "\r", "\n", "\r\n");
    default:
        Py_RETURN_NONE;
   }

}


static PyMethodDef incrementalnewlinedecoder_methods[] = {
    {"decode", (PyCFunction)incrementalnewlinedecoder_decode, METH_VARARGS|METH_KEYWORDS},
    {"getstate", (PyCFunction)incrementalnewlinedecoder_getstate, METH_NOARGS},
    {"setstate", (PyCFunction)incrementalnewlinedecoder_setstate, METH_O},
    {"reset", (PyCFunction)incrementalnewlinedecoder_reset, METH_NOARGS},
    {NULL}
};

static PyGetSetDef incrementalnewlinedecoder_getset[] = {
    {"newlines", (getter)incrementalnewlinedecoder_newlines_get, NULL, NULL},
    {NULL}
};

PyTypeObject PyIncrementalNewlineDecoder_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.IncrementalNewlineDecoder", /*tp_name*/
    sizeof(nldecoder_object), /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)incrementalnewlinedecoder_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tp_setattr*/
    0,                          /*tp_compare */
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /*tp_flags*/
    incrementalnewlinedecoder_doc,          /* tp_doc */
    0,                          /* tp_traverse */
    0,                          /* tp_clear */
    0,                          /* tp_richcompare */
    0,                          /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    0,                          /* tp_iternext */
    incrementalnewlinedecoder_methods, /* tp_methods */
    0,                          /* tp_members */
    incrementalnewlinedecoder_getset, /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    0,                          /* tp_dictoffset */
    (initproc)incrementalnewlinedecoder_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};


/* TextIOWrapper */

PyDoc_STRVAR(textiowrapper_doc,
    "Character and line based layer over a BufferedIOBase object, buffer.\n"
    "\n"
    "encoding gives the name of the encoding that the stream will be\n"
    "decoded or encoded with. It defaults to locale.getpreferredencoding.\n"
    "\n"
    "errors determines the strictness of encoding and decoding (see the\n"
    "codecs.register) and defaults to \"strict\".\n"
    "\n"
    "newline can be None, '', '\\n', '\\r', or '\\r\\n'.  It controls the\n"
    "handling of line endings. If it is None, universal newlines is\n"
    "enabled.  With this enabled, on input, the lines endings '\\n', '\\r',\n"
    "or '\\r\\n' are translated to '\\n' before being returned to the\n"
    "caller. Conversely, on output, '\\n' is translated to the system\n"
    "default line seperator, os.linesep. If newline is any other of its\n"
    "legal values, that newline becomes the newline when the file is read\n"
    "and it is returned untranslated. On output, '\\n' is converted to the\n"
    "newline.\n"
    "\n"
    "If line_buffering is True, a call to flush is implied when a call to\n"
    "write contains a newline character."
    );

typedef PyObject *
        (*encodefunc_t)(PyObject *, PyObject *);

typedef struct
{
    PyObject_HEAD
    int ok; /* initialized? */
    int detached;
    Py_ssize_t chunk_size;
    PyObject *buffer;
    PyObject *encoding;
    PyObject *encoder;
    PyObject *decoder;
    PyObject *readnl;
    PyObject *errors;
    const char *writenl; /* utf-8 encoded, NULL stands for \n */
    char line_buffering;
    char readuniversal;
    char readtranslate;
    char writetranslate;
    char seekable;
    char telling;
    char deallocating;
    /* Specialized encoding func (see below) */
    encodefunc_t encodefunc;
    /* Whether or not it's the start of the stream */
    char encoding_start_of_stream;

    /* Reads and writes are internally buffered in order to speed things up.
       However, any read will first flush the write buffer if itsn't empty.

       Please also note that text to be written is first encoded before being
       buffered. This is necessary so that encoding errors are immediately
       reported to the caller, but it unfortunately means that the
       IncrementalEncoder (whose encode() method is always written in Python)
       becomes a bottleneck for small writes.
    */
    PyObject *decoded_chars;       /* buffer for text returned from decoder */
    Py_ssize_t decoded_chars_used; /* offset into _decoded_chars for read() */
    PyObject *pending_bytes;       /* list of bytes objects waiting to be
                                      written, or NULL */
    Py_ssize_t pending_bytes_count;
    PyObject *snapshot;
    /* snapshot is either None, or a tuple (dec_flags, next_input) where
     * dec_flags is the second (integer) item of the decoder state and
     * next_input is the chunk of input bytes that comes next after the
     * snapshot point.  We use this to reconstruct decoder states in tell().
     */

    /* Cache raw object if it's a FileIO object */
    PyObject *raw;

    PyObject *weakreflist;
    PyObject *dict;
} textio;


/* A couple of specialized cases in order to bypass the slow incremental
   encoding methods for the most popular encodings. */

static PyObject *
ascii_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeASCII(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors));
}

static PyObject *
utf16be_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), 1);
}

static PyObject *
utf16le_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), -1);
}

static PyObject *
utf16_encode(textio *self, PyObject *text)
{
    if (!self->encoding_start_of_stream) {
        /* Skip the BOM and use native byte ordering */
#if defined(WORDS_BIGENDIAN)
        return utf16be_encode(self, text);
#else
        return utf16le_encode(self, text);
#endif
    }
    return PyUnicode_EncodeUTF16(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), 0);
}

static PyObject *
utf32be_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeUTF32(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), 1);
}

static PyObject *
utf32le_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeUTF32(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), -1);
}

static PyObject *
utf32_encode(textio *self, PyObject *text)
{
    if (!self->encoding_start_of_stream) {
        /* Skip the BOM and use native byte ordering */
#if defined(WORDS_BIGENDIAN)
        return utf32be_encode(self, text);
#else
        return utf32le_encode(self, text);
#endif
    }
    return PyUnicode_EncodeUTF32(PyUnicode_AS_UNICODE(text),
                                 PyUnicode_GET_SIZE(text),
                                 PyBytes_AS_STRING(self->errors), 0);
}

static PyObject *
utf8_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeUTF8(PyUnicode_AS_UNICODE(text),
                                PyUnicode_GET_SIZE(text),
                                PyBytes_AS_STRING(self->errors));
}

static PyObject *
latin1_encode(textio *self, PyObject *text)
{
    return PyUnicode_EncodeLatin1(PyUnicode_AS_UNICODE(text),
                                  PyUnicode_GET_SIZE(text),
                                  PyBytes_AS_STRING(self->errors));
}

/* Map normalized encoding names onto the specialized encoding funcs */

typedef struct {
    const char *name;
    encodefunc_t encodefunc;
} encodefuncentry;

static encodefuncentry encodefuncs[] = {
    {"ascii",       (encodefunc_t) ascii_encode},
    {"iso8859-1",   (encodefunc_t) latin1_encode},
    {"utf-8",       (encodefunc_t) utf8_encode},
    {"utf-16-be",   (encodefunc_t) utf16be_encode},
    {"utf-16-le",   (encodefunc_t) utf16le_encode},
    {"utf-16",      (encodefunc_t) utf16_encode},
    {"utf-32-be",   (encodefunc_t) utf32be_encode},
    {"utf-32-le",   (encodefunc_t) utf32le_encode},
    {"utf-32",      (encodefunc_t) utf32_encode},
    {NULL, NULL}
};


static int
textiowrapper_init(textio *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"buffer", "encoding", "errors",
                      "newline", "line_buffering",
                      NULL};
    PyObject *buffer, *raw;
    char *encoding = NULL;
    char *errors = NULL;
    char *newline = NULL;
    int line_buffering = 0;
    _PyIO_State *state = IO_STATE;

    PyObject *res;
    int r;

    self->ok = 0;
    self->detached = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|zzzi:fileio",
                                     kwlist, &buffer, &encoding, &errors,
                                     &newline, &line_buffering))
        return -1;

    if (newline && newline[0] != '\0'
        && !(newline[0] == '\n' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\0')
        && !(newline[0] == '\r' && newline[1] == '\n' && newline[2] == '\0')) {
        PyErr_Format(PyExc_ValueError,
                     "illegal newline value: %s", newline);
        return -1;
    }

    Py_CLEAR(self->buffer);
    Py_CLEAR(self->encoding);
    Py_CLEAR(self->encoder);
    Py_CLEAR(self->decoder);
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->decoded_chars);
    Py_CLEAR(self->pending_bytes);
    Py_CLEAR(self->snapshot);
    Py_CLEAR(self->errors);
    Py_CLEAR(self->raw);
    self->decoded_chars_used = 0;
    self->pending_bytes_count = 0;
    self->encodefunc = NULL;

    if (encoding == NULL) {
        /* Try os.device_encoding(fileno) */
        PyObject *fileno;
        fileno = PyObject_CallMethod(buffer, "fileno", NULL);
        /* Ignore only AttributeError and UnsupportedOperation */
        if (fileno == NULL) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError) ||
                PyErr_ExceptionMatches(state->unsupported_operation)) {
                PyErr_Clear();
            }
            else {
                goto error;
            }
        }
        else {
            self->encoding = PyObject_CallMethod(state->os_module,
                                                 "device_encoding",
                                                 "N", fileno);
            if (self->encoding == NULL)
                goto error;
            else if (!PyUnicode_Check(self->encoding))
                Py_CLEAR(self->encoding);
        }
    }
    if (encoding == NULL && self->encoding == NULL) {
        if (state->locale_module == NULL) {
            state->locale_module = PyImport_ImportModule("locale");
            if (state->locale_module == NULL)
                goto catch_ImportError;
            else
                goto use_locale;
        }
        else {
          use_locale:
            self->encoding = PyObject_CallMethod(
                state->locale_module, "getpreferredencoding", NULL);
            if (self->encoding == NULL) {
              catch_ImportError:
                /*
                 Importing locale can raise a ImportError because of
                 _functools, and locale.getpreferredencoding can raise a
                 ImportError if _locale is not available.  These will happen
                 during module building.
                */
                if (PyErr_ExceptionMatches(PyExc_ImportError)) {
                    PyErr_Clear();
                    self->encoding = PyUnicode_FromString("ascii");
                }
                else
                    goto error;
            }
            else if (!PyUnicode_Check(self->encoding))
                Py_CLEAR(self->encoding);
        }
    }
    if (self->encoding != NULL) {
        encoding = _PyUnicode_AsString(self->encoding);
        if (encoding == NULL)
            goto error;
    }
    else if (encoding != NULL) {
        self->encoding = PyUnicode_FromString(encoding);
        if (self->encoding == NULL)
            goto error;
    }
    else {
        PyErr_SetString(PyExc_IOError,
                        "could not determine default encoding");
    }

    if (errors == NULL)
        errors = "strict";
    self->errors = PyBytes_FromString(errors);
    if (self->errors == NULL)
        goto error;

    self->chunk_size = 8192;
    self->readuniversal = (newline == NULL || newline[0] == '\0');
    self->line_buffering = line_buffering;
    self->readtranslate = (newline == NULL);
    if (newline) {
        self->readnl = PyUnicode_FromString(newline);
        if (self->readnl == NULL)
            return -1;
    }
    self->writetranslate = (newline == NULL || newline[0] != '\0');
    if (!self->readuniversal && self->readnl) {
        self->writenl = _PyUnicode_AsString(self->readnl);
        if (self->writenl == NULL)
            goto error;
        if (!strcmp(self->writenl, "\n"))
            self->writenl = NULL;
    }
#ifdef MS_WINDOWS
    else
        self->writenl = "\r\n";
#endif

    /* Build the decoder object */
    res = PyObject_CallMethod(buffer, "readable", NULL);
    if (res == NULL)
        goto error;
    r = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (r == -1)
        goto error;
    if (r == 1) {
        self->decoder = PyCodec_IncrementalDecoder(
            encoding, errors);
        if (self->decoder == NULL)
            goto error;

        if (self->readuniversal) {
            PyObject *incrementalDecoder = PyObject_CallFunction(
                (PyObject *)&PyIncrementalNewlineDecoder_Type,
                "Oi", self->decoder, (int)self->readtranslate);
            if (incrementalDecoder == NULL)
                goto error;
            Py_CLEAR(self->decoder);
            self->decoder = incrementalDecoder;
        }
    }

    /* Build the encoder object */
    res = PyObject_CallMethod(buffer, "writable", NULL);
    if (res == NULL)
        goto error;
    r = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (r == -1)
        goto error;
    if (r == 1) {
        PyObject *ci;
        self->encoder = PyCodec_IncrementalEncoder(
            encoding, errors);
        if (self->encoder == NULL)
            goto error;
        /* Get the normalized named of the codec */
        ci = _PyCodec_Lookup(encoding);
        if (ci == NULL)
            goto error;
        res = PyObject_GetAttrString(ci, "name");
        Py_DECREF(ci);
        if (res == NULL) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_Clear();
            else
                goto error;
        }
        else if (PyUnicode_Check(res)) {
            encodefuncentry *e = encodefuncs;
            while (e->name != NULL) {
                if (!PyUnicode_CompareWithASCIIString(res, e->name)) {
                    self->encodefunc = e->encodefunc;
                    break;
                }
                e++;
            }
        }
        Py_XDECREF(res);
    }

    self->buffer = buffer;
    Py_INCREF(buffer);

    if (Py_TYPE(buffer) == &PyBufferedReader_Type ||
        Py_TYPE(buffer) == &PyBufferedWriter_Type ||
        Py_TYPE(buffer) == &PyBufferedRandom_Type) {
        raw = PyObject_GetAttrString(buffer, "raw");
        /* Cache the raw FileIO object to speed up 'closed' checks */
        if (raw == NULL) {
            if (PyErr_ExceptionMatches(PyExc_AttributeError))
                PyErr_Clear();
            else
                goto error;
        }
        else if (Py_TYPE(raw) == &PyFileIO_Type)
            self->raw = raw;
        else
            Py_DECREF(raw);
    }

    res = PyObject_CallMethod(buffer, "seekable", NULL);
    if (res == NULL)
        goto error;
    self->seekable = self->telling = PyObject_IsTrue(res);
    Py_DECREF(res);

    self->encoding_start_of_stream = 0;
    if (self->seekable && self->encoder) {
        PyObject *cookieObj;
        int cmp;

        self->encoding_start_of_stream = 1;

        cookieObj = PyObject_CallMethodObjArgs(buffer, _PyIO_str_tell, NULL);
        if (cookieObj == NULL)
            goto error;

        cmp = PyObject_RichCompareBool(cookieObj, _PyIO_zero, Py_EQ);
        Py_DECREF(cookieObj);
        if (cmp < 0) {
            goto error;
        }

        if (cmp == 0) {
            self->encoding_start_of_stream = 0;
            res = PyObject_CallMethodObjArgs(self->encoder, _PyIO_str_setstate,
                                             _PyIO_zero, NULL);
            if (res == NULL)
                goto error;
            Py_DECREF(res);
        }
    }

    self->ok = 1;
    return 0;

  error:
    return -1;
}

static int
_textiowrapper_clear(textio *self)
{
    if (self->ok && _PyIOBase_finalize((PyObject *) self) < 0)
        return -1;
    self->ok = 0;
    Py_CLEAR(self->buffer);
    Py_CLEAR(self->encoding);
    Py_CLEAR(self->encoder);
    Py_CLEAR(self->decoder);
    Py_CLEAR(self->readnl);
    Py_CLEAR(self->decoded_chars);
    Py_CLEAR(self->pending_bytes);
    Py_CLEAR(self->snapshot);
    Py_CLEAR(self->errors);
    Py_CLEAR(self->raw);
    return 0;
}

static void
textiowrapper_dealloc(textio *self)
{
    self->deallocating = 1;
    if (_textiowrapper_clear(self) < 0)
        return;
    _PyObject_GC_UNTRACK(self);
    if (self->weakreflist != NULL)
        PyObject_ClearWeakRefs((PyObject *)self);
    Py_CLEAR(self->dict);
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static int
textiowrapper_traverse(textio *self, visitproc visit, void *arg)
{
    Py_VISIT(self->buffer);
    Py_VISIT(self->encoding);
    Py_VISIT(self->encoder);
    Py_VISIT(self->decoder);
    Py_VISIT(self->readnl);
    Py_VISIT(self->decoded_chars);
    Py_VISIT(self->pending_bytes);
    Py_VISIT(self->snapshot);
    Py_VISIT(self->errors);
    Py_VISIT(self->raw);

    Py_VISIT(self->dict);
    return 0;
}

static int
textiowrapper_clear(textio *self)
{
    if (_textiowrapper_clear(self) < 0)
        return -1;
    Py_CLEAR(self->dict);
    return 0;
}

static PyObject *
textiowrapper_closed_get(textio *self, void *context);

/* This macro takes some shortcuts to make the common case faster. */
#define CHECK_CLOSED(self) \
    do { \
        int r; \
        PyObject *_res; \
        if (Py_TYPE(self) == &PyTextIOWrapper_Type) { \
            if (self->raw != NULL) \
                r = _PyFileIO_closed(self->raw); \
            else { \
                _res = textiowrapper_closed_get(self, NULL); \
                if (_res == NULL) \
                    return NULL; \
                r = PyObject_IsTrue(_res); \
                Py_DECREF(_res); \
                if (r < 0) \
                    return NULL; \
            } \
            if (r > 0) { \
                PyErr_SetString(PyExc_ValueError, \
                                "I/O operation on closed file."); \
                return NULL; \
            } \
        } \
        else if (_PyIOBase_check_closed((PyObject *)self, Py_True) == NULL) \
            return NULL; \
    } while (0)

#define CHECK_INITIALIZED(self) \
    if (self->ok <= 0) { \
        if (self->detached) { \
            PyErr_SetString(PyExc_ValueError, \
                 "underlying buffer has been detached"); \
        } else {                                   \
            PyErr_SetString(PyExc_ValueError, \
                "I/O operation on uninitialized object"); \
        } \
        return NULL; \
    }

#define CHECK_INITIALIZED_INT(self) \
    if (self->ok <= 0) { \
        if (self->detached) { \
            PyErr_SetString(PyExc_ValueError, \
                 "underlying buffer has been detached"); \
        } else {                                   \
            PyErr_SetString(PyExc_ValueError, \
                "I/O operation on uninitialized object"); \
        } \
        return -1; \
    }


static PyObject *
textiowrapper_detach(textio *self)
{
    PyObject *buffer, *res;
    CHECK_INITIALIZED(self);
    res = PyObject_CallMethodObjArgs((PyObject *)self, _PyIO_str_flush, NULL);
    if (res == NULL)
        return NULL;
    Py_DECREF(res);
    buffer = self->buffer;
    self->buffer = NULL;
    self->detached = 1;
    self->ok = 0;
    return buffer;
}

Py_LOCAL_INLINE(const Py_UNICODE *)
findchar(const Py_UNICODE *s, Py_ssize_t size, Py_UNICODE ch)
{
    /* like wcschr, but doesn't stop at NULL characters */
    while (size-- > 0) {
        if (*s == ch)
            return s;
        s++;
    }
    return NULL;
}

/* Flush the internal write buffer. This doesn't explicitly flush the
   underlying buffered object, though. */
static int
_textiowrapper_writeflush(textio *self)
{
    PyObject *pending, *b, *ret;

    if (self->pending_bytes == NULL)
        return 0;

    pending = self->pending_bytes;
    Py_INCREF(pending);
    self->pending_bytes_count = 0;
    Py_CLEAR(self->pending_bytes);

    b = _PyBytes_Join(_PyIO_empty_bytes, pending);
    Py_DECREF(pending);
    if (b == NULL)
        return -1;
    ret = PyObject_CallMethodObjArgs(self->buffer,
                                     _PyIO_str_write, b, NULL);
    Py_DECREF(b);
    if (ret == NULL)
        return -1;
    Py_DECREF(ret);
    return 0;
}

static PyObject *
textiowrapper_write(textio *self, PyObject *args)
{
    PyObject *ret;
    PyObject *text; /* owned reference */
    PyObject *b;
    Py_ssize_t textlen;
    int haslf = 0;
    int needflush = 0;

    CHECK_INITIALIZED(self);

    if (!PyArg_ParseTuple(args, "U:write", &text)) {
        return NULL;
    }

    CHECK_CLOSED(self);

    if (self->encoder == NULL)
        return _unsupported("not writable");

    Py_INCREF(text);

    textlen = PyUnicode_GetSize(text);

    if ((self->writetranslate && self->writenl != NULL) || self->line_buffering)
        if (findchar(PyUnicode_AS_UNICODE(text),
                     PyUnicode_GET_SIZE(text), '\n'))
            haslf = 1;

    if (haslf && self->writetranslate && self->writenl != NULL) {
        PyObject *newtext = PyObject_CallMethod(
            text, "replace", "ss", "\n", self->writenl);
        Py_DECREF(text);
        if (newtext == NULL)
            return NULL;
        text = newtext;
    }

    if (self->line_buffering &&
        (haslf ||
         findchar(PyUnicode_AS_UNICODE(text),
                  PyUnicode_GET_SIZE(text), '\r')))
        needflush = 1;

    /* XXX What if we were just reading? */
    if (self->encodefunc != NULL) {
        b = (*self->encodefunc)((PyObject *) self, text);
        self->encoding_start_of_stream = 0;
    }
    else
        b = PyObject_CallMethodObjArgs(self->encoder,
                                       _PyIO_str_encode, text, NULL);
    Py_DECREF(text);
    if (b == NULL)
        return NULL;

    if (self->pending_bytes == NULL) {
        self->pending_bytes = PyList_New(0);
        if (self->pending_bytes == NULL) {
            Py_DECREF(b);
            return NULL;
        }
        self->pending_bytes_count = 0;
    }
    if (PyList_Append(self->pending_bytes, b) < 0) {
        Py_DECREF(b);
        return NULL;
    }
    self->pending_bytes_count += PyBytes_GET_SIZE(b);
    Py_DECREF(b);
    if (self->pending_bytes_count > self->chunk_size || needflush) {
        if (_textiowrapper_writeflush(self) < 0)
            return NULL;
    }

    if (needflush) {
        ret = PyObject_CallMethodObjArgs(self->buffer, _PyIO_str_flush, NULL);
        if (ret == NULL)
            return NULL;
        Py_DECREF(ret);
    }

    Py_CLEAR(self->snapshot);

    if (self->decoder) {
        ret = PyObject_CallMethod(self->decoder, "reset", NULL);
        if (ret == NULL)
            return NULL;
        Py_DECREF(ret);
    }

    return PyLong_FromSsize_t(textlen);
}

/* Steal a reference to chars and store it in the decoded_char buffer;
 */
static void
textiowrapper_set_decoded_chars(textio *self, PyObject *chars)
{
    Py_CLEAR(self->decoded_chars);
    self->decoded_chars = chars;
    self->decoded_chars_used = 0;
}

static PyObject *
textiowrapper_get_decoded_chars(textio *self, Py_ssize_t n)
{
    PyObject *chars;
    Py_ssize_t avail;

    if (self->decoded_chars == NULL)
        return PyUnicode_FromStringAndSize(NULL, 0);

    avail = (PyUnicode_GET_SIZE(self->decoded_chars)
             - self->decoded_chars_used);

    assert(avail >= 0);

    if (n < 0 || n > avail)
        n = avail;

    if (self->decoded_chars_used > 0 || n < avail) {
        chars = PyUnicode_FromUnicode(
            PyUnicode_AS_UNICODE(self->decoded_chars)
            + self->decoded_chars_used, n);
        if (chars == NULL)
            return NULL;
    }
    else {
        chars = self->decoded_chars;
        Py_INCREF(chars);
    }

    self->decoded_chars_used += n;
    return chars;
}

/* Read and decode the next chunk of data from the BufferedReader.
 */
static int
textiowrapper_read_chunk(textio *self)
{
    PyObject *dec_buffer = NULL;
    PyObject *dec_flags = NULL;
    PyObject *input_chunk = NULL;
    PyObject *decoded_chars, *chunk_size;
    int eof;

    /* The return value is True unless EOF was reached.  The decoded string is
     * placed in self._decoded_chars (replacing its previous value).  The
     * entire input chunk is sent to the decoder, though some of it may remain
     * buffered in the decoder, yet to be converted.
     */

    if (self->decoder == NULL) {
        _unsupported("not readable");
        return -1;
    }

    if (self->telling) {
        /* To prepare for tell(), we need to snapshot a point in the file
         * where the decoder's input buffer is empty.
         */

        PyObject *state = PyObject_CallMethodObjArgs(self->decoder,
                                                     _PyIO_str_getstate, NULL);
        if (state == NULL)
            return -1;
        /* Given this, we know there was a valid snapshot point
         * len(dec_buffer) bytes ago with decoder state (b'', dec_flags).
         */
        if (PyArg_Parse(state, "(OO)", &dec_buffer, &dec_flags) < 0) {
            Py_DECREF(state);
            return -1;
        }
        Py_INCREF(dec_buffer);
        Py_INCREF(dec_flags);
        Py_DECREF(state);
    }

    /* Read a chunk, decode it, and put the result in self._decoded_chars. */
    chunk_size = PyLong_FromSsize_t(self->chunk_size);
    if (chunk_size == NULL)
        goto fail;
    input_chunk = PyObject_CallMethodObjArgs(self->buffer,
        _PyIO_str_read1, chunk_size, NULL);
    Py_DECREF(chunk_size);
    if (input_chunk == NULL)
        goto fail;
    assert(PyBytes_Check(input_chunk));

    eof = (PyBytes_Size(input_chunk) == 0);

    if (Py_TYPE(self->decoder) == &PyIncrementalNewlineDecoder_Type) {
        decoded_chars = _PyIncrementalNewlineDecoder_decode(
            self->decoder, input_chunk, eof);
    }
    else {
        decoded_chars = PyObject_CallMethodObjArgs(self->decoder,
            _PyIO_str_decode, input_chunk, eof ? Py_True : Py_False, NULL);
    }

    /* TODO sanity check: isinstance(decoded_chars, unicode) */
    if (decoded_chars == NULL)
        goto fail;
    textiowrapper_set_decoded_chars(self, decoded_chars);
    if (PyUnicode_GET_SIZE(decoded_chars) > 0)
        eof = 0;

    if (self->telling) {
        /* At the snapshot point, len(dec_buffer) bytes before the read, the
         * next input to be decoded is dec_buffer + input_chunk.
         */
        PyObject *next_input = PyNumber_Add(dec_buffer, input_chunk);
        if (next_input == NULL)
            goto fail;
        assert (PyBytes_Check(next_input));
        Py_DECREF(dec_buffer);
        Py_CLEAR(self->snapshot);
        self->snapshot = Py_BuildValue("NN", dec_flags, next_input);
    }
    Py_DECREF(input_chunk);

    return (eof == 0);

  fail:
    Py_XDECREF(dec_buffer);
    Py_XDECREF(dec_flags);
    Py_XDECREF(input_chunk);
    return -1;
}

static PyObject *
textiowrapper_read(textio *self, PyObject *args)
{
    Py_ssize_t n = -1;
    PyObject *result = NULL, *chunks = NULL;

    CHECK_INITIALIZED(self);

    if (!PyArg_ParseTuple(args, "|O&:read", &_PyIO_ConvertSsize_t, &n))
        return NULL;

    CHECK_CLOSED(self);

    if (self->decoder == NULL)
        return _unsupported("not readable");

    if (_textiowrapper_writeflush(self) < 0)
        return NULL;

    if (n < 0) {
        /* Read everything */
        PyObject *bytes = PyObject_CallMethod(self->buffer, "read", NULL);
        PyObject *decoded;
        if (bytes == NULL)
            goto fail;
        decoded = PyObject_CallMethodObjArgs(self->decoder, _PyIO_str_decode,
                                             bytes, Py_True, NULL);
        Py_DECREF(bytes);
        if (decoded == NULL)
            goto fail;

        result = textiowrapper_get_decoded_chars(self, -1);

        if (result == NULL) {
            Py_DECREF(decoded);
            return NULL;
        }

        PyUnicode_AppendAndDel(&result, decoded);
        if (result == NULL)
            goto fail;

        Py_CLEAR(self->snapshot);
        return result;
    }
    else {
        int res = 1;
        Py_ssize_t remaining = n;

        result = textiowrapper_get_decoded_chars(self, n);
        if (result == NULL)
            goto fail;
        remaining -= PyUnicode_GET_SIZE(result);

        /* Keep reading chunks until we have n characters to return */
        while (remaining > 0) {
            res = textiowrapper_read_chunk(self);
            if (res < 0)
                goto fail;
            if (res == 0)  /* EOF */
                break;
            if (chunks == NULL) {
                chunks = PyList_New(0);
                if (chunks == NULL)
                    goto fail;
            }
            if (PyList_Append(chunks, result) < 0)
                goto fail;
            Py_DECREF(result);
            result = textiowrapper_get_decoded_chars(self, remaining);
            if (result == NULL)
                goto fail;
            remaining -= PyUnicode_GET_SIZE(result);
        }
        if (chunks != NULL) {
            if (result != NULL && PyList_Append(chunks, result) < 0)
                goto fail;
            Py_CLEAR(result);
            result = PyUnicode_Join(_PyIO_empty_str, chunks);
            if (result == NULL)
                goto fail;
            Py_CLEAR(chunks);
        }
        return result;
    }
  fail:
    Py_XDECREF(result);
    Py_XDECREF(chunks);
    return NULL;
}


/* NOTE: `end` must point to the real end of the Py_UNICODE storage,
   that is to the NUL character. Otherwise the function will produce
   incorrect results. */
static Py_UNICODE *
find_control_char(Py_UNICODE *start, Py_UNICODE *end, Py_UNICODE ch)
{
    Py_UNICODE *s = start;
    for (;;) {
        while (*s > ch)
            s++;
        if (*s == ch)
            return s;
        if (s == end)
            return NULL;
        s++;
    }
}

Py_ssize_t
_PyIO_find_line_ending(
    int translated, int universal, PyObject *readnl,
    Py_UNICODE *start, Py_UNICODE *end, Py_ssize_t *consumed)
{
    Py_ssize_t len = end - start;

    if (translated) {
        /* Newlines are already translated, only search for \n */
        Py_UNICODE *pos = find_control_char(start, end, '\n');
        if (pos != NULL)
            return pos - start + 1;
        else {
            *consumed = len;
            return -1;
        }
    }
    else if (universal) {
        /* Universal newline search. Find any of \r, \r\n, \n
         * The decoder ensures that \r\n are not split in two pieces
         */
        Py_UNICODE *s = start;
        for (;;) {
            Py_UNICODE ch;
            /* Fast path for non-control chars. The loop always ends
               since the Py_UNICODE storage is NUL-terminated. */
            while (*s > '\r')
                s++;
            if (s >= end) {
                *consumed = len;
                return -1;
            }
            ch = *s++;
            if (ch == '\n')
                return s - start;
            if (ch == '\r') {
                if (*s == '\n')
                    return s - start + 1;
                else
                    return s - start;
            }
        }
    }
    else {
        /* Non-universal mode. */
        Py_ssize_t readnl_len = PyUnicode_GET_SIZE(readnl);
        Py_UNICODE *nl = PyUnicode_AS_UNICODE(readnl);
        if (readnl_len == 1) {
            Py_UNICODE *pos = find_control_char(start, end, nl[0]);
            if (pos != NULL)
                return pos - start + 1;
            *consumed = len;
            return -1;
        }
        else {
            Py_UNICODE *s = start;
            Py_UNICODE *e = end - readnl_len + 1;
            Py_UNICODE *pos;
            if (e < s)
                e = s;
            while (s < e) {
                Py_ssize_t i;
                Py_UNICODE *pos = find_control_char(s, end, nl[0]);
                if (pos == NULL || pos >= e)
                    break;
                for (i = 1; i < readnl_len; i++) {
                    if (pos[i] != nl[i])
                        break;
                }
                if (i == readnl_len)
                    return pos - start + readnl_len;
                s = pos + 1;
            }
            pos = find_control_char(e, end, nl[0]);
            if (pos == NULL)
                *consumed = len;
            else
                *consumed = pos - start;
            return -1;
        }
    }
}

static PyObject *
_textiowrapper_readline(textio *self, Py_ssize_t limit)
{
    PyObject *line = NULL, *chunks = NULL, *remaining = NULL;
    Py_ssize_t start, endpos, chunked, offset_to_buffer;
    int res;

    CHECK_CLOSED(self);

    if (_textiowrapper_writeflush(self) < 0)
        return NULL;

    chunked = 0;

    while (1) {
        Py_UNICODE *ptr;
        Py_ssize_t line_len;
        Py_ssize_t consumed = 0;

        /* First, get some data if necessary */
        res = 1;
        while (!self->decoded_chars ||
               !PyUnicode_GET_SIZE(self->decoded_chars)) {
            res = textiowrapper_read_chunk(self);
            if (res < 0)
                goto error;
            if (res == 0)
                break;
        }
        if (res == 0) {
            /* end of file */
            textiowrapper_set_decoded_chars(self, NULL);
            Py_CLEAR(self->snapshot);
            start = endpos = offset_to_buffer = 0;
            break;
        }

        if (remaining == NULL) {
            line = self->decoded_chars;
            start = self->decoded_chars_used;
            offset_to_buffer = 0;
            Py_INCREF(line);
        }
        else {
            assert(self->decoded_chars_used == 0);
            line = PyUnicode_Concat(remaining, self->decoded_chars);
            start = 0;
            offset_to_buffer = PyUnicode_GET_SIZE(remaining);
            Py_CLEAR(remaining);
            if (line == NULL)
                goto error;
        }

        ptr = PyUnicode_AS_UNICODE(line);
        line_len = PyUnicode_GET_SIZE(line);

        endpos = _PyIO_find_line_ending(
            self->readtranslate, self->readuniversal, self->readnl,
            ptr + start, ptr + line_len, &consumed);
        if (endpos >= 0) {
            endpos += start;
            if (limit >= 0 && (endpos - start) + chunked >= limit)
                endpos = start + limit - chunked;
            break;
        }

        /* We can put aside up to `endpos` */
        endpos = consumed + start;
        if (limit >= 0 && (endpos - start) + chunked >= limit) {
            /* Didn't find line ending, but reached length limit */
            endpos = start + limit - chunked;
            break;
        }

        if (endpos > start) {
            /* No line ending seen yet - put aside current data */
            PyObject *s;
            if (chunks == NULL) {
                chunks = PyList_New(0);
                if (chunks == NULL)
                    goto error;
            }
            s = PyUnicode_FromUnicode(ptr + start, endpos - start);
            if (s == NULL)
                goto error;
            if (PyList_Append(chunks, s) < 0) {
                Py_DECREF(s);
                goto error;
            }
            chunked += PyUnicode_GET_SIZE(s);
            Py_DECREF(s);
        }
        /* There may be some remaining bytes we'll have to prepend to the
           next chunk of data */
        if (endpos < line_len) {
            remaining = PyUnicode_FromUnicode(
                    ptr + endpos, line_len - endpos);
            if (remaining == NULL)
                goto error;
        }
        Py_CLEAR(line);
        /* We have consumed the buffer */
        textiowrapper_set_decoded_chars(self, NULL);
    }

    if (line != NULL) {
        /* Our line ends in the current buffer */
        self->decoded_chars_used = endpos - offset_to_buffer;
        if (start > 0 || endpos < PyUnicode_GET_SIZE(line)) {
            if (start == 0 && Py_REFCNT(line) == 1) {
                if (PyUnicode_Resize(&line, endpos) < 0)
                    goto error;
            }
            else {
                PyObject *s = PyUnicode_FromUnicode(
                        PyUnicode_AS_UNICODE(line) + start, endpos - start);
                Py_CLEAR(line);
                if (s == NULL)
                    goto error;
                line = s;
            }
        }
    }
    if (remaining != NULL) {
        if (chunks == NULL) {
            chunks = PyList_New(0);
            if (chunks == NULL)
                goto error;
        }
        if (PyList_Append(chunks, remaining) < 0)
            goto error;
        Py_CLEAR(remaining);
    }
    if (chunks != NULL) {
        if (line != NULL && PyList_Append(chunks, line) < 0)
            goto error;
        Py_CLEAR(line);
        line = PyUnicode_Join(_PyIO_empty_str, chunks);
        if (line == NULL)
            goto error;
        Py_DECREF(chunks);
    }
    if (line == NULL)
        line = PyUnicode_FromStringAndSize(NULL, 0);

    return line;

  error:
    Py_XDECREF(chunks);
    Py_XDECREF(remaining);
    Py_XDECREF(line);
    return NULL;
}

static PyObject *
textiowrapper_readline(textio *self, PyObject *args)
{
    Py_ssize_t limit = -1;

    CHECK_INITIALIZED(self);
    if (!PyArg_ParseTuple(args, "|n:readline", &limit)) {
        return NULL;
    }
    return _textiowrapper_readline(self, limit);
}

/* Seek and Tell */

typedef struct {
    Py_off_t start_pos;
    int dec_flags;
    int bytes_to_feed;
    int chars_to_skip;
    char need_eof;
} cookie_type;

/*
   To speed up cookie packing/unpacking, we store the fields in a temporary
   string and call _PyLong_FromByteArray() or _PyLong_AsByteArray (resp.).
   The following macros define at which offsets in the intermediary byte
   string the various CookieStruct fields will be stored.
 */

#define COOKIE_BUF_LEN      (sizeof(Py_off_t) + 3 * sizeof(int) + sizeof(char))

#if defined(WORDS_BIGENDIAN)

# define IS_LITTLE_ENDIAN   0

/* We want the least significant byte of start_pos to also be the least
   significant byte of the cookie, which means that in big-endian mode we
   must copy the fields in reverse order. */

# define OFF_START_POS      (sizeof(char) + 3 * sizeof(int))
# define OFF_DEC_FLAGS      (sizeof(char) + 2 * sizeof(int))
# define OFF_BYTES_TO_FEED  (sizeof(char) + sizeof(int))
# define OFF_CHARS_TO_SKIP  (sizeof(char))
# define OFF_NEED_EOF       0

#else

# define IS_LITTLE_ENDIAN   1

/* Little-endian mode: the least significant byte of start_pos will
   naturally end up the least significant byte of the cookie. */

# define OFF_START_POS      0
# define OFF_DEC_FLAGS      (sizeof(Py_off_t))
# define OFF_BYTES_TO_FEED  (sizeof(Py_off_t) + sizeof(int))
# define OFF_CHARS_TO_SKIP  (sizeof(Py_off_t) + 2 * sizeof(int))
# define OFF_NEED_EOF       (sizeof(Py_off_t) + 3 * sizeof(int))

#endif

static int
textiowrapper_parse_cookie(cookie_type *cookie, PyObject *cookieObj)
{
    unsigned char buffer[COOKIE_BUF_LEN];
    PyLongObject *cookieLong = (PyLongObject *)PyNumber_Long(cookieObj);
    if (cookieLong == NULL)
        return -1;

    if (_PyLong_AsByteArray(cookieLong, buffer, sizeof(buffer),
                            IS_LITTLE_ENDIAN, 0) < 0) {
        Py_DECREF(cookieLong);
        return -1;
    }
    Py_DECREF(cookieLong);

    memcpy(&cookie->start_pos, buffer + OFF_START_POS, sizeof(cookie->start_pos));
    memcpy(&cookie->dec_flags, buffer + OFF_DEC_FLAGS, sizeof(cookie->dec_flags));
    memcpy(&cookie->bytes_to_feed, buffer + OFF_BYTES_TO_FEED, sizeof(cookie->bytes_to_feed));
    memcpy(&cookie->chars_to_skip, buffer + OFF_CHARS_TO_SKIP, sizeof(cookie->chars_to_skip));
    memcpy(&cookie->need_eof, buffer + OFF_NEED_EOF, sizeof(cookie->need_eof));

    return 0;
}

static PyObject *
textiowrapper_build_cookie(cookie_type *cookie)
{
    unsigned char buffer[COOKIE_BUF_LEN];

    memcpy(buffer + OFF_START_POS, &cookie->start_pos, sizeof(cookie->start_pos));
    memcpy(buffer + OFF_DEC_FLAGS, &cookie->dec_flags, sizeof(cookie->dec_flags));
    memcpy(buffer + OFF_BYTES_TO_FEED, &cookie->bytes_to_feed, sizeof(cookie->bytes_to_feed));
    memcpy(buffer + OFF_CHARS_TO_SKIP, &cookie->chars_to_skip, sizeof(cookie->chars_to_skip));
    memcpy(buffer + OFF_NEED_EOF, &cookie->need_eof, sizeof(cookie->need_eof));

    return _PyLong_FromByteArray(buffer, sizeof(buffer), IS_LITTLE_ENDIAN, 0);
}
#undef IS_LITTLE_ENDIAN

static int
_textiowrapper_decoder_setstate(textio *self, cookie_type *cookie)
{
    PyObject *res;
    /* When seeking to the start of the stream, we call decoder.reset()
       rather than decoder.getstate().
       This is for a few decoders such as utf-16 for which the state value
       at start is not (b"", 0) but e.g. (b"", 2) (meaning, in the case of
       utf-16, that we are expecting a BOM).
    */
    if (cookie->start_pos == 0 && cookie->dec_flags == 0)
        res = PyObject_CallMethodObjArgs(self->decoder, _PyIO_str_reset, NULL);
    else
        res = PyObject_CallMethod(self->decoder, "setstate",
                                  "((yi))", "", cookie->dec_flags);
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

static int
_textiowrapper_encoder_setstate(textio *self, cookie_type *cookie)
{
    PyObject *res;
    /* Same as _textiowrapper_decoder_setstate() above. */
    if (cookie->start_pos == 0 && cookie->dec_flags == 0) {
        res = PyObject_CallMethodObjArgs(self->encoder, _PyIO_str_reset, NULL);
        self->encoding_start_of_stream = 1;
    }
    else {
        res = PyObject_CallMethodObjArgs(self->encoder, _PyIO_str_setstate,
                                         _PyIO_zero, NULL);
        self->encoding_start_of_stream = 0;
    }
    if (res == NULL)
        return -1;
    Py_DECREF(res);
    return 0;
}

static PyObject *
textiowrapper_seek(textio *self, PyObject *args)
{
    PyObject *cookieObj, *posobj;
    cookie_type cookie;
    int whence = 0;
    PyObject *res;
    int cmp;

    CHECK_INITIALIZED(self);

    if (!PyArg_ParseTuple(args, "O|i:seek", &cookieObj, &whence))
        return NULL;
    CHECK_CLOSED(self);

    Py_INCREF(cookieObj);

    if (!self->seekable) {
        _unsupported("underlying stream is not seekable");
        goto fail;
    }

    if (whence == 1) {
        /* seek relative to current position */
        cmp = PyObject_RichCompareBool(cookieObj, _PyIO_zero, Py_EQ);
        if (cmp < 0)
            goto fail;

        if (cmp == 0) {
            _unsupported("can't do nonzero cur-relative seeks");
            goto fail;
        }

        /* Seeking to the current position should attempt to
         * sync the underlying buffer with the current position.
         */
        Py_DECREF(cookieObj);
        cookieObj = PyObject_CallMethod((PyObject *)self, "tell", NULL);
        if (cookieObj == NULL)
            goto fail;
    }
    else if (whence == 2) {
        /* seek relative to end of file */

        cmp = PyObject_RichCompareBool(cookieObj, _PyIO_zero, Py_EQ);
        if (cmp < 0)
            goto fail;

        if (cmp == 0) {
            _unsupported("can't do nonzero end-relative seeks");
            goto fail;
        }

        res = PyObject_CallMethod((PyObject *)self, "flush", NULL);
        if (res == NULL)
            goto fail;
        Py_DECREF(res);

        textiowrapper_set_decoded_chars(self, NULL);
        Py_CLEAR(self->snapshot);
        if (self->decoder) {
            res = PyObject_CallMethod(self->decoder, "reset", NULL);
            if (res == NULL)
                goto fail;
            Py_DECREF(res);
        }

        res = PyObject_CallMethod(self->buffer, "seek", "ii", 0, 2);
        Py_XDECREF(cookieObj);
        return res;
    }
    else if (whence != 0) {
        PyErr_Format(PyExc_ValueError,
                     "invalid whence (%d, should be 0, 1 or 2)", whence);
        goto fail;
    }

    cmp = PyObject_RichCompareBool(cookieObj, _PyIO_zero, Py_LT);
    if (cmp < 0)
        goto fail;

    if (cmp == 1) {
        PyErr_Format(PyExc_ValueError,
                     "negative seek position %R", cookieObj);
        goto fail;
    }

    res = PyObject_CallMethodObjArgs((PyObject *)self, _PyIO_str_flush, NULL);
    if (res == NULL)
        goto fail;
    Py_DECREF(res);

    /* The strategy of seek() is to go back to the safe start point
     * and replay the effect of read(chars_to_skip) from there.
     */
    if (textiowrapper_parse_cookie(&cookie, cookieObj) < 0)
        goto fail;

    /* Seek back to the safe start point. */
    posobj = PyLong_FromOff_t(cookie.start_pos);
    if (posobj == NULL)
        goto fail;
    res = PyObject_CallMethodObjArgs(self->buffer,
                                     _PyIO_str_seek, posobj, NULL);
    Py_DECREF(posobj);
    if (res == NULL)
        goto fail;
    Py_DECREF(res);

    textiowrapper_set_decoded_chars(self, NULL);
    Py_CLEAR(self->snapshot);

    /* Restore the decoder to its state from the safe start point. */
    if (self->decoder) {
        if (_textiowrapper_decoder_setstate(self, &cookie) < 0)
            goto fail;
    }

    if (cookie.chars_to_skip) {
        /* Just like _read_chunk, feed the decoder and save a snapshot. */
        PyObject *input_chunk = PyObject_CallMethod(
            self->buffer, "read", "i", cookie.bytes_to_feed);
        PyObject *decoded;

        if (input_chunk == NULL)
            goto fail;

        assert (PyBytes_Check(input_chunk));

        self->snapshot = Py_BuildValue("iN", cookie.dec_flags, input_chunk);
        if (self->snapshot == NULL) {
            Py_DECREF(input_chunk);
            goto fail;
        }

        decoded = PyObject_CallMethod(self->decoder, "decode",
                                      "Oi", input_chunk, (int)cookie.need_eof);

        if (decoded == NULL)
            goto fail;

        textiowrapper_set_decoded_chars(self, decoded);

        /* Skip chars_to_skip of the decoded characters. */
        if (PyUnicode_GetSize(self->decoded_chars) < cookie.chars_to_skip) {
            PyErr_SetString(PyExc_IOError, "can't restore logical file position");
            goto fail;
        }
        self->decoded_chars_used = cookie.chars_to_skip;
    }
    else {
        self->snapshot = Py_BuildValue("iy", cookie.dec_flags, "");
        if (self->snapshot == NULL)
            goto fail;
    }

    /* Finally, reset the encoder (merely useful for proper BOM handling) */
    if (self->encoder) {
        if (_textiowrapper_encoder_setstate(self, &cookie) < 0)
            goto fail;
    }
    return cookieObj;
  fail:
    Py_XDECREF(cookieObj);
    return NULL;

}

static PyObject *
textiowrapper_tell(textio *self, PyObject *args)
{
    PyObject *res;
    PyObject *posobj = NULL;
    cookie_type cookie = {0,0,0,0,0};
    PyObject *next_input;
    Py_ssize_t chars_to_skip, chars_decoded;
    PyObject *saved_state = NULL;
    char *input, *input_end;

    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);

    if (!self->seekable) {
        _unsupported("underlying stream is not seekable");
        goto fail;
    }
    if (!self->telling) {
        PyErr_SetString(PyExc_IOError,
                        "telling position disabled by next() call");
        goto fail;
    }

    if (_textiowrapper_writeflush(self) < 0)
        return NULL;
    res = PyObject_CallMethod((PyObject *)self, "flush", NULL);
    if (res == NULL)
        goto fail;
    Py_DECREF(res);

    posobj = PyObject_CallMethod(self->buffer, "tell", NULL);
    if (posobj == NULL)
        goto fail;

    if (self->decoder == NULL || self->snapshot == NULL) {
        assert (self->decoded_chars == NULL || PyUnicode_GetSize(self->decoded_chars) == 0);
        return posobj;
    }

#if defined(HAVE_LARGEFILE_SUPPORT)
    cookie.start_pos = PyLong_AsLongLong(posobj);
#else
    cookie.start_pos = PyLong_AsLong(posobj);
#endif
    if (PyErr_Occurred())
        goto fail;

    /* Skip backward to the snapshot point (see _read_chunk). */
    if (!PyArg_Parse(self->snapshot, "(iO)", &cookie.dec_flags, &next_input))
        goto fail;

    assert (PyBytes_Check(next_input));

    cookie.start_pos -= PyBytes_GET_SIZE(next_input);

    /* How many decoded characters have been used up since the snapshot? */
    if (self->decoded_chars_used == 0)  {
        /* We haven't moved from the snapshot point. */
        Py_DECREF(posobj);
        return textiowrapper_build_cookie(&cookie);
    }

    chars_to_skip = self->decoded_chars_used;

    /* Starting from the snapshot position, we will walk the decoder
     * forward until it gives us enough decoded characters.
     */
    saved_state = PyObject_CallMethodObjArgs(self->decoder,
                                             _PyIO_str_getstate, NULL);
    if (saved_state == NULL)
        goto fail;

    /* Note our initial start point. */
    if (_textiowrapper_decoder_setstate(self, &cookie) < 0)
        goto fail;

    /* Feed the decoder one byte at a time.  As we go, note the
     * nearest "safe start point" before the current location
     * (a point where the decoder has nothing buffered, so seek()
     * can safely start from there and advance to this location).
     */
    chars_decoded = 0;
    input = PyBytes_AS_STRING(next_input);
    input_end = input + PyBytes_GET_SIZE(next_input);
    while (input < input_end) {
        PyObject *state;
        char *dec_buffer;
        Py_ssize_t dec_buffer_len;
        int dec_flags;

        PyObject *decoded = PyObject_CallMethod(
            self->decoder, "decode", "y#", input, 1);
        if (decoded == NULL)
            goto fail;
        assert (PyUnicode_Check(decoded));
        chars_decoded += PyUnicode_GET_SIZE(decoded);
        Py_DECREF(decoded);

        cookie.bytes_to_feed += 1;

        state = PyObject_CallMethodObjArgs(self->decoder,
                                           _PyIO_str_getstate, NULL);
        if (state == NULL)
            goto fail;
        if (!PyArg_Parse(state, "(y#i)", &dec_buffer, &dec_buffer_len, &dec_flags)) {
            Py_DECREF(state);
            goto fail;
        }
        Py_DECREF(state);

        if (dec_buffer_len == 0 && chars_decoded <= chars_to_skip) {
            /* Decoder buffer is empty, so this is a safe start point. */
            cookie.start_pos += cookie.bytes_to_feed;
            chars_to_skip -= chars_decoded;
            cookie.dec_flags = dec_flags;
            cookie.bytes_to_feed = 0;
            chars_decoded = 0;
        }
        if (chars_decoded >= chars_to_skip)
            break;
        input++;
    }
    if (input == input_end) {
        /* We didn't get enough decoded data; signal EOF to get more. */
        PyObject *decoded = PyObject_CallMethod(
            self->decoder, "decode", "yi", "", /* final = */ 1);
        if (decoded == NULL)
            goto fail;
        assert (PyUnicode_Check(decoded));
        chars_decoded += PyUnicode_GET_SIZE(decoded);
        Py_DECREF(decoded);
        cookie.need_eof = 1;

        if (chars_decoded < chars_to_skip) {
            PyErr_SetString(PyExc_IOError,
                            "can't reconstruct logical file position");
            goto fail;
        }
    }

    /* finally */
    Py_XDECREF(posobj);
    res = PyObject_CallMethod(self->decoder, "setstate", "(O)", saved_state);
    Py_DECREF(saved_state);
    if (res == NULL)
        return NULL;
    Py_DECREF(res);

    /* The returned cookie corresponds to the last safe start point. */
    cookie.chars_to_skip = Py_SAFE_DOWNCAST(chars_to_skip, Py_ssize_t, int);
    return textiowrapper_build_cookie(&cookie);

  fail:
    Py_XDECREF(posobj);
    if (saved_state) {
        PyObject *type, *value, *traceback;
        PyErr_Fetch(&type, &value, &traceback);

        res = PyObject_CallMethod(self->decoder, "setstate", "(O)", saved_state);
        Py_DECREF(saved_state);
        if (res == NULL)
            return NULL;
        Py_DECREF(res);

        PyErr_Restore(type, value, traceback);
    }
    return NULL;
}

static PyObject *
textiowrapper_truncate(textio *self, PyObject *args)
{
    PyObject *pos = Py_None;
    PyObject *res;

    CHECK_INITIALIZED(self)
    if (!PyArg_ParseTuple(args, "|O:truncate", &pos)) {
        return NULL;
    }

    res = PyObject_CallMethodObjArgs((PyObject *) self, _PyIO_str_flush, NULL);
    if (res == NULL)
        return NULL;
    Py_DECREF(res);

    return PyObject_CallMethodObjArgs(self->buffer, _PyIO_str_truncate, pos, NULL);
}

static PyObject *
textiowrapper_repr(textio *self)
{
    PyObject *nameobj, *res;

    CHECK_INITIALIZED(self);

    nameobj = PyObject_GetAttrString((PyObject *) self, "name");
    if (nameobj == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError))
            PyErr_Clear();
        else
            return NULL;
        res = PyUnicode_FromFormat("<_io.TextIOWrapper encoding=%R>",
                                   self->encoding);
    }
    else {
        res = PyUnicode_FromFormat("<_io.TextIOWrapper name=%R encoding=%R>",
                                   nameobj, self->encoding);
        Py_DECREF(nameobj);
    }
    return res;
}


/* Inquiries */

static PyObject *
textiowrapper_fileno(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    return PyObject_CallMethod(self->buffer, "fileno", NULL);
}

static PyObject *
textiowrapper_seekable(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    return PyObject_CallMethod(self->buffer, "seekable", NULL);
}

static PyObject *
textiowrapper_readable(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    return PyObject_CallMethod(self->buffer, "readable", NULL);
}

static PyObject *
textiowrapper_writable(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    return PyObject_CallMethod(self->buffer, "writable", NULL);
}

static PyObject *
textiowrapper_isatty(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    return PyObject_CallMethod(self->buffer, "isatty", NULL);
}

static PyObject *
textiowrapper_getstate(textio *self, PyObject *args)
{
    PyErr_Format(PyExc_TypeError,
                 "cannot serialize '%s' object", Py_TYPE(self)->tp_name);
    return NULL;
}

static PyObject *
textiowrapper_flush(textio *self, PyObject *args)
{
    CHECK_INITIALIZED(self);
    CHECK_CLOSED(self);
    self->telling = self->seekable;
    if (_textiowrapper_writeflush(self) < 0)
        return NULL;
    return PyObject_CallMethod(self->buffer, "flush", NULL);
}

static PyObject *
textiowrapper_close(textio *self, PyObject *args)
{
    PyObject *res;
    int r;
    CHECK_INITIALIZED(self);

    res = textiowrapper_closed_get(self, NULL);
    if (res == NULL)
        return NULL;
    r = PyObject_IsTrue(res);
    Py_DECREF(res);
    if (r < 0)
        return NULL;

    if (r > 0) {
        Py_RETURN_NONE; /* stream already closed */
    }
    else {
        if (self->deallocating) {
            res = PyObject_CallMethod(self->buffer, "_dealloc_warn", "O", self);
            if (res)
                Py_DECREF(res);
            else
                PyErr_Clear();
        }
        res = PyObject_CallMethod((PyObject *)self, "flush", NULL);
        if (res == NULL) {
            return NULL;
        }
        else
            Py_DECREF(res);

        return PyObject_CallMethod(self->buffer, "close", NULL);
    }
}

static PyObject *
textiowrapper_iternext(textio *self)
{
    PyObject *line;

    CHECK_INITIALIZED(self);

    self->telling = 0;
    if (Py_TYPE(self) == &PyTextIOWrapper_Type) {
        /* Skip method call overhead for speed */
        line = _textiowrapper_readline(self, -1);
    }
    else {
        line = PyObject_CallMethodObjArgs((PyObject *)self,
                                           _PyIO_str_readline, NULL);
        if (line && !PyUnicode_Check(line)) {
            PyErr_Format(PyExc_IOError,
                         "readline() should have returned an str object, "
                         "not '%.200s'", Py_TYPE(line)->tp_name);
            Py_DECREF(line);
            return NULL;
        }
    }

    if (line == NULL)
        return NULL;

    if (PyUnicode_GET_SIZE(line) == 0) {
        /* Reached EOF or would have blocked */
        Py_DECREF(line);
        Py_CLEAR(self->snapshot);
        self->telling = self->seekable;
        return NULL;
    }

    return line;
}

static PyObject *
textiowrapper_name_get(textio *self, void *context)
{
    CHECK_INITIALIZED(self);
    return PyObject_GetAttrString(self->buffer, "name");
}

static PyObject *
textiowrapper_closed_get(textio *self, void *context)
{
    CHECK_INITIALIZED(self);
    return PyObject_GetAttr(self->buffer, _PyIO_str_closed);
}

static PyObject *
textiowrapper_newlines_get(textio *self, void *context)
{
    PyObject *res;
    CHECK_INITIALIZED(self);
    if (self->decoder == NULL)
        Py_RETURN_NONE;
    res = PyObject_GetAttr(self->decoder, _PyIO_str_newlines);
    if (res == NULL) {
        if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
            PyErr_Clear();
            Py_RETURN_NONE;
        }
        else {
            return NULL;
        }
    }
    return res;
}

static PyObject *
textiowrapper_errors_get(textio *self, void *context)
{
    CHECK_INITIALIZED(self);
    return PyUnicode_FromString(PyBytes_AS_STRING(self->errors));
}

static PyObject *
textiowrapper_chunk_size_get(textio *self, void *context)
{
    CHECK_INITIALIZED(self);
    return PyLong_FromSsize_t(self->chunk_size);
}

static int
textiowrapper_chunk_size_set(textio *self, PyObject *arg, void *context)
{
    Py_ssize_t n;
    CHECK_INITIALIZED_INT(self);
    n = PyNumber_AsSsize_t(arg, PyExc_TypeError);
    if (n == -1 && PyErr_Occurred())
        return -1;
    if (n <= 0) {
        PyErr_SetString(PyExc_ValueError,
                        "a strictly positive integer is required");
        return -1;
    }
    self->chunk_size = n;
    return 0;
}

static PyMethodDef textiowrapper_methods[] = {
    {"detach", (PyCFunction)textiowrapper_detach, METH_NOARGS},
    {"write", (PyCFunction)textiowrapper_write, METH_VARARGS},
    {"read", (PyCFunction)textiowrapper_read, METH_VARARGS},
    {"readline", (PyCFunction)textiowrapper_readline, METH_VARARGS},
    {"flush", (PyCFunction)textiowrapper_flush, METH_NOARGS},
    {"close", (PyCFunction)textiowrapper_close, METH_NOARGS},

    {"fileno", (PyCFunction)textiowrapper_fileno, METH_NOARGS},
    {"seekable", (PyCFunction)textiowrapper_seekable, METH_NOARGS},
    {"readable", (PyCFunction)textiowrapper_readable, METH_NOARGS},
    {"writable", (PyCFunction)textiowrapper_writable, METH_NOARGS},
    {"isatty", (PyCFunction)textiowrapper_isatty, METH_NOARGS},
    {"__getstate__", (PyCFunction)textiowrapper_getstate, METH_NOARGS},

    {"seek", (PyCFunction)textiowrapper_seek, METH_VARARGS},
    {"tell", (PyCFunction)textiowrapper_tell, METH_NOARGS},
    {"truncate", (PyCFunction)textiowrapper_truncate, METH_VARARGS},
    {NULL, NULL}
};

static PyMemberDef textiowrapper_members[] = {
    {"encoding", T_OBJECT, offsetof(textio, encoding), READONLY},
    {"buffer", T_OBJECT, offsetof(textio, buffer), READONLY},
    {"line_buffering", T_BOOL, offsetof(textio, line_buffering), READONLY},
    {NULL}
};

static PyGetSetDef textiowrapper_getset[] = {
    {"name", (getter)textiowrapper_name_get, NULL, NULL},
    {"closed", (getter)textiowrapper_closed_get, NULL, NULL},
/*    {"mode", (getter)TextIOWrapper_mode_get, NULL, NULL},
*/
    {"newlines", (getter)textiowrapper_newlines_get, NULL, NULL},
    {"errors", (getter)textiowrapper_errors_get, NULL, NULL},
    {"_CHUNK_SIZE", (getter)textiowrapper_chunk_size_get,
                    (setter)textiowrapper_chunk_size_set, NULL},
    {NULL}
};

PyTypeObject PyTextIOWrapper_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_io.TextIOWrapper",        /*tp_name*/
    sizeof(textio), /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    (destructor)textiowrapper_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    0,                          /*tp_getattr*/
    0,                          /*tps_etattr*/
    0,                          /*tp_compare */
    (reprfunc)textiowrapper_repr,/*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash */
    0,                          /*tp_call*/
    0,                          /*tp_str*/
    0,                          /*tp_getattro*/
    0,                          /*tp_setattro*/
    0,                          /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE
            | Py_TPFLAGS_HAVE_GC, /*tp_flags*/
    textiowrapper_doc,          /* tp_doc */
    (traverseproc)textiowrapper_traverse, /* tp_traverse */
    (inquiry)textiowrapper_clear, /* tp_clear */
    0,                          /* tp_richcompare */
    offsetof(textio, weakreflist), /*tp_weaklistoffset*/
    0,                          /* tp_iter */
    (iternextfunc)textiowrapper_iternext, /* tp_iternext */
    textiowrapper_methods,      /* tp_methods */
    textiowrapper_members,      /* tp_members */
    textiowrapper_getset,       /* tp_getset */
    0,                          /* tp_base */
    0,                          /* tp_dict */
    0,                          /* tp_descr_get */
    0,                          /* tp_descr_set */
    offsetof(textio, dict), /*tp_dictoffset*/
    (initproc)textiowrapper_init, /* tp_init */
    0,                          /* tp_alloc */
    PyType_GenericNew,          /* tp_new */
};
