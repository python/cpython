/*
    An implementation of the new I/O lib as defined by PEP 3116 - "New I/O"

    Classes defined here: UnsupportedOperation, BlockingIOError.
    Functions defined here: open().

    Mostly written by Amaury Forgeot d'Arc
*/

#define PY_SSIZE_T_CLEAN
#include "Python.h"
#include "structmember.h"
#include "_iomodule.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS_STAT_H */


/* Various interned strings */

PyObject *_PyIO_str_close;
PyObject *_PyIO_str_closed;
PyObject *_PyIO_str_decode;
PyObject *_PyIO_str_encode;
PyObject *_PyIO_str_fileno;
PyObject *_PyIO_str_flush;
PyObject *_PyIO_str_getstate;
PyObject *_PyIO_str_isatty;
PyObject *_PyIO_str_newlines;
PyObject *_PyIO_str_nl;
PyObject *_PyIO_str_read;
PyObject *_PyIO_str_read1;
PyObject *_PyIO_str_readable;
PyObject *_PyIO_str_readall;
PyObject *_PyIO_str_readinto;
PyObject *_PyIO_str_readline;
PyObject *_PyIO_str_reset;
PyObject *_PyIO_str_seek;
PyObject *_PyIO_str_seekable;
PyObject *_PyIO_str_setstate;
PyObject *_PyIO_str_tell;
PyObject *_PyIO_str_truncate;
PyObject *_PyIO_str_writable;
PyObject *_PyIO_str_write;

PyObject *_PyIO_empty_str;
PyObject *_PyIO_empty_bytes;
PyObject *_PyIO_zero;


PyDoc_STRVAR(module_doc,
"The io module provides the Python interfaces to stream handling. The\n"
"builtin open function is defined in this module.\n"
"\n"
"At the top of the I/O hierarchy is the abstract base class IOBase. It\n"
"defines the basic interface to a stream. Note, however, that there is no\n"
"separation between reading and writing to streams; implementations are\n"
"allowed to raise an IOError if they do not support a given operation.\n"
"\n"
"Extending IOBase is RawIOBase which deals simply with the reading and\n"
"writing of raw bytes to a stream. FileIO subclasses RawIOBase to provide\n"
"an interface to OS files.\n"
"\n"
"BufferedIOBase deals with buffering on a raw byte stream (RawIOBase). Its\n"
"subclasses, BufferedWriter, BufferedReader, and BufferedRWPair buffer\n"
"streams that are readable, writable, and both respectively.\n"
"BufferedRandom provides a buffered interface to random access\n"
"streams. BytesIO is a simple stream of in-memory bytes.\n"
"\n"
"Another IOBase subclass, TextIOBase, deals with the encoding and decoding\n"
"of streams into text. TextIOWrapper, which extends it, is a buffered text\n"
"interface to a buffered raw stream (`BufferedIOBase`). Finally, StringIO\n"
"is a in-memory stream for text.\n"
"\n"
"Argument names are not part of the specification, and only the arguments\n"
"of open() are intended to be used as keyword arguments.\n"
"\n"
"data:\n"
"\n"
"DEFAULT_BUFFER_SIZE\n"
"\n"
"   An int containing the default buffer size used by the module's buffered\n"
"   I/O classes. open() uses the file's blksize (as obtained by os.stat) if\n"
"   possible.\n"
    );


/*
 * The main open() function
 */
PyDoc_STRVAR(open_doc,
"open(file, mode='r', buffering=-1, encoding=None,\n"
"     errors=None, newline=None, closefd=True, opener=None) -> file object\n"
"\n"
"Open file and return a stream.  Raise IOError upon failure.\n"
"\n"
"file is either a text or byte string giving the name (and the path\n"
"if the file isn't in the current working directory) of the file to\n"
"be opened or an integer file descriptor of the file to be\n"
"wrapped. (If a file descriptor is given, it is closed when the\n"
"returned I/O object is closed, unless closefd is set to False.)\n"
"\n"
"mode is an optional string that specifies the mode in which the file\n"
"is opened. It defaults to 'r' which means open for reading in text\n"
"mode.  Other common values are 'w' for writing (truncating the file if\n"
"it already exists), 'x' for creating and writing to a new file, and\n"
"'a' for appending (which on some Unix systems, means that all writes\n"
"append to the end of the file regardless of the current seek position).\n"
"In text mode, if encoding is not specified the encoding used is platform\n"
"dependent: locale.getpreferredencoding(False) is called to get the\n"
"current locale encoding. (For reading and writing raw bytes use binary\n"
"mode and leave encoding unspecified.) The available modes are:\n"
"\n"
"========= ===============================================================\n"
"Character Meaning\n"
"--------- ---------------------------------------------------------------\n"
"'r'       open for reading (default)\n"
"'w'       open for writing, truncating the file first\n"
"'x'       create a new file and open it for writing\n"
"'a'       open for writing, appending to the end of the file if it exists\n"
"'b'       binary mode\n"
"'t'       text mode (default)\n"
"'+'       open a disk file for updating (reading and writing)\n"
"'U'       universal newline mode (deprecated)\n"
"========= ===============================================================\n"
"\n"
"The default mode is 'rt' (open for reading text). For binary random\n"
"access, the mode 'w+b' opens and truncates the file to 0 bytes, while\n"
"'r+b' opens the file without truncation. The 'x' mode implies 'w' and\n"
"raises an `FileExistsError` if the file already exists.\n"
"\n"
"Python distinguishes between files opened in binary and text modes,\n"
"even when the underlying operating system doesn't. Files opened in\n"
"binary mode (appending 'b' to the mode argument) return contents as\n"
"bytes objects without any decoding. In text mode (the default, or when\n"
"'t' is appended to the mode argument), the contents of the file are\n"
"returned as strings, the bytes having been first decoded using a\n"
"platform-dependent encoding or using the specified encoding if given.\n"
"\n"
"'U' mode is deprecated and will raise an exception in future versions\n"
"of Python.  It has no effect in Python 3.  Use newline to control\n"
"universal newlines mode.\n"
"\n"
"buffering is an optional integer used to set the buffering policy.\n"
"Pass 0 to switch buffering off (only allowed in binary mode), 1 to select\n"
"line buffering (only usable in text mode), and an integer > 1 to indicate\n"
"the size of a fixed-size chunk buffer.  When no buffering argument is\n"
"given, the default buffering policy works as follows:\n"
"\n"
"* Binary files are buffered in fixed-size chunks; the size of the buffer\n"
"  is chosen using a heuristic trying to determine the underlying device's\n"
"  \"block size\" and falling back on `io.DEFAULT_BUFFER_SIZE`.\n"
"  On many systems, the buffer will typically be 4096 or 8192 bytes long.\n"
"\n"
"* \"Interactive\" text files (files for which isatty() returns True)\n"
"  use line buffering.  Other text files use the policy described above\n"
"  for binary files.\n"
"\n"
"encoding is the name of the encoding used to decode or encode the\n"
"file. This should only be used in text mode. The default encoding is\n"
"platform dependent, but any encoding supported by Python can be\n"
"passed.  See the codecs module for the list of supported encodings.\n"
"\n"
"errors is an optional string that specifies how encoding errors are to\n"
"be handled---this argument should not be used in binary mode. Pass\n"
"'strict' to raise a ValueError exception if there is an encoding error\n"
"(the default of None has the same effect), or pass 'ignore' to ignore\n"
"errors. (Note that ignoring encoding errors can lead to data loss.)\n"
"See the documentation for codecs.register or run 'help(codecs.Codec)'\n"
"for a list of the permitted encoding error strings.\n"
"\n"
"newline controls how universal newlines works (it only applies to text\n"
"mode). It can be None, '', '\\n', '\\r', and '\\r\\n'.  It works as\n"
"follows:\n"
"\n"
"* On input, if newline is None, universal newlines mode is\n"
"  enabled. Lines in the input can end in '\\n', '\\r', or '\\r\\n', and\n"
"  these are translated into '\\n' before being returned to the\n"
"  caller. If it is '', universal newline mode is enabled, but line\n"
"  endings are returned to the caller untranslated. If it has any of\n"
"  the other legal values, input lines are only terminated by the given\n"
"  string, and the line ending is returned to the caller untranslated.\n"
"\n"
"* On output, if newline is None, any '\\n' characters written are\n"
"  translated to the system default line separator, os.linesep. If\n"
"  newline is '' or '\\n', no translation takes place. If newline is any\n"
"  of the other legal values, any '\\n' characters written are translated\n"
"  to the given string.\n"
"\n"
"If closefd is False, the underlying file descriptor will be kept open\n"
"when the file is closed. This does not work when a file name is given\n"
"and must be True in that case.\n"
"\n"
"A custom opener can be used by passing a callable as *opener*. The\n"
"underlying file descriptor for the file object is then obtained by\n"
"calling *opener* with (*file*, *flags*). *opener* must return an open\n"
"file descriptor (passing os.open as *opener* results in functionality\n"
"similar to passing None).\n"
"\n"
"open() returns a file object whose type depends on the mode, and\n"
"through which the standard file operations such as reading and writing\n"
"are performed. When open() is used to open a file in a text mode ('w',\n"
"'r', 'wt', 'rt', etc.), it returns a TextIOWrapper. When used to open\n"
"a file in a binary mode, the returned class varies: in read binary\n"
"mode, it returns a BufferedReader; in write binary and append binary\n"
"modes, it returns a BufferedWriter, and in read/write mode, it returns\n"
"a BufferedRandom.\n"
"\n"
"It is also possible to use a string or bytearray as a file for both\n"
"reading and writing. For strings StringIO can be used like a file\n"
"opened in a text mode, and for bytes a BytesIO can be used like a file\n"
"opened in a binary mode.\n"
    );

static PyObject *
io_open(PyObject *self, PyObject *args, PyObject *kwds)
{
    char *kwlist[] = {"file", "mode", "buffering",
                      "encoding", "errors", "newline",
                      "closefd", "opener", NULL};
    PyObject *file, *opener = Py_None;
    char *mode = "r";
    int buffering = -1, closefd = 1;
    char *encoding = NULL, *errors = NULL, *newline = NULL;
    unsigned i;

    int creating = 0, reading = 0, writing = 0, appending = 0, updating = 0;
    int text = 0, binary = 0, universal = 0;

    char rawmode[6], *m;
    int line_buffering, isatty;

    PyObject *raw, *modeobj = NULL, *buffer, *wrapper, *result = NULL;

    _Py_IDENTIFIER(isatty);
    _Py_IDENTIFIER(fileno);
    _Py_IDENTIFIER(mode);
    _Py_IDENTIFIER(close);

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|sizzziO:open", kwlist,
                                     &file, &mode, &buffering,
                                     &encoding, &errors, &newline,
                                     &closefd, &opener)) {
        return NULL;
    }

    if (!PyUnicode_Check(file) &&
	!PyBytes_Check(file) &&
	!PyNumber_Check(file)) {
        PyErr_Format(PyExc_TypeError, "invalid file: %R", file);
        return NULL;
    }

    /* Decode mode */
    for (i = 0; i < strlen(mode); i++) {
        char c = mode[i];

        switch (c) {
        case 'x':
            creating = 1;
            break;
        case 'r':
            reading = 1;
            break;
        case 'w':
            writing = 1;
            break;
        case 'a':
            appending = 1;
            break;
        case '+':
            updating = 1;
            break;
        case 't':
            text = 1;
            break;
        case 'b':
            binary = 1;
            break;
        case 'U':
            universal = 1;
            reading = 1;
            break;
        default:
            goto invalid_mode;
        }

        /* c must not be duplicated */
        if (strchr(mode+i+1, c)) {
          invalid_mode:
            PyErr_Format(PyExc_ValueError, "invalid mode: '%s'", mode);
            return NULL;
        }

    }

    m = rawmode;
    if (creating)  *(m++) = 'x';
    if (reading)   *(m++) = 'r';
    if (writing)   *(m++) = 'w';
    if (appending) *(m++) = 'a';
    if (updating)  *(m++) = '+';
    *m = '\0';

    /* Parameters validation */
    if (universal) {
        if (writing || appending) {
            PyErr_SetString(PyExc_ValueError,
                            "can't use U and writing mode at once");
            return NULL;
        }
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "'U' mode is deprecated", 1) < 0)
            return NULL;
        reading = 1;
    }

    if (text && binary) {
        PyErr_SetString(PyExc_ValueError,
                        "can't have text and binary mode at once");
        return NULL;
    }

    if (creating + reading + writing + appending > 1) {
        PyErr_SetString(PyExc_ValueError,
                        "must have exactly one of create/read/write/append mode");
        return NULL;
    }

    if (binary && encoding != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an encoding argument");
        return NULL;
    }

    if (binary && errors != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an errors argument");
        return NULL;
    }

    if (binary && newline != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take a newline argument");
        return NULL;
    }

    /* Create the Raw file stream */
    raw = PyObject_CallFunction((PyObject *)&PyFileIO_Type,
                                "OsiO", file, rawmode, closefd, opener);
    if (raw == NULL)
        return NULL;
    result = raw;

    modeobj = PyUnicode_FromString(mode);
    if (modeobj == NULL)
        goto error;

    /* buffering */
    {
        PyObject *res = _PyObject_CallMethodId(raw, &PyId_isatty, NULL);
        if (res == NULL)
            goto error;
        isatty = PyLong_AsLong(res);
        Py_DECREF(res);
        if (isatty == -1 && PyErr_Occurred())
            goto error;
    }

    if (buffering == 1 || (buffering < 0 && isatty)) {
        buffering = -1;
        line_buffering = 1;
    }
    else
        line_buffering = 0;

    if (buffering < 0) {
        buffering = DEFAULT_BUFFER_SIZE;
#ifdef HAVE_STRUCT_STAT_ST_BLKSIZE
        {
            struct stat st;
            long fileno;
            PyObject *res = _PyObject_CallMethodId(raw, &PyId_fileno, NULL);
            if (res == NULL)
                goto error;

            fileno = PyLong_AsLong(res);
            Py_DECREF(res);
            if (fileno == -1 && PyErr_Occurred())
                goto error;

            if (fstat(fileno, &st) >= 0 && st.st_blksize > 1)
                buffering = st.st_blksize;
        }
#endif
    }
    if (buffering < 0) {
        PyErr_SetString(PyExc_ValueError,
                        "invalid buffering size");
        goto error;
    }

    /* if not buffering, returns the raw file object */
    if (buffering == 0) {
        if (!binary) {
            PyErr_SetString(PyExc_ValueError,
                            "can't have unbuffered text I/O");
            goto error;
        }

        Py_DECREF(modeobj);
        return result;
    }

    /* wraps into a buffered file */
    {
        PyObject *Buffered_class;

        if (updating)
            Buffered_class = (PyObject *)&PyBufferedRandom_Type;
        else if (creating || writing || appending)
            Buffered_class = (PyObject *)&PyBufferedWriter_Type;
        else if (reading)
            Buffered_class = (PyObject *)&PyBufferedReader_Type;
        else {
            PyErr_Format(PyExc_ValueError,
                         "unknown mode: '%s'", mode);
            goto error;
        }

        buffer = PyObject_CallFunction(Buffered_class, "Oi", raw, buffering);
    }
    if (buffer == NULL)
        goto error;
    result = buffer;
    Py_DECREF(raw);


    /* if binary, returns the buffered file */
    if (binary) {
        Py_DECREF(modeobj);
        return result;
    }

    /* wraps into a TextIOWrapper */
    wrapper = PyObject_CallFunction((PyObject *)&PyTextIOWrapper_Type,
				    "Osssi",
				    buffer,
				    encoding, errors, newline,
				    line_buffering);
    if (wrapper == NULL)
        goto error;
    result = wrapper;
    Py_DECREF(buffer);

    if (_PyObject_SetAttrId(wrapper, &PyId_mode, modeobj) < 0)
        goto error;
    Py_DECREF(modeobj);
    return result;

  error:
    if (result != NULL) {
        PyObject *exc, *val, *tb, *close_result;
        PyErr_Fetch(&exc, &val, &tb);
        close_result = _PyObject_CallMethodId(result, &PyId_close, NULL);
        _PyErr_ChainExceptions(exc, val, tb);
        Py_XDECREF(close_result);
        Py_DECREF(result);
    }
    Py_XDECREF(modeobj);
    return NULL;
}

/*
 * Private helpers for the io module.
 */

Py_off_t
PyNumber_AsOff_t(PyObject *item, PyObject *err)
{
    Py_off_t result;
    PyObject *runerr;
    PyObject *value = PyNumber_Index(item);
    if (value == NULL)
        return -1;

    /* We're done if PyLong_AsSsize_t() returns without error. */
    result = PyLong_AsOff_t(value);
    if (result != -1 || !(runerr = PyErr_Occurred()))
        goto finish;

    /* Error handling code -- only manage OverflowError differently */
    if (!PyErr_GivenExceptionMatches(runerr, PyExc_OverflowError))
        goto finish;

    PyErr_Clear();
    /* If no error-handling desired then the default clipping
       is sufficient.
     */
    if (!err) {
        assert(PyLong_Check(value));
        /* Whether or not it is less than or equal to
           zero is determined by the sign of ob_size
        */
        if (_PyLong_Sign(value) < 0)
            result = PY_OFF_T_MIN;
        else
            result = PY_OFF_T_MAX;
    }
    else {
        /* Otherwise replace the error with caller's error object. */
        PyErr_Format(err,
                     "cannot fit '%.200s' into an offset-sized integer",
                     item->ob_type->tp_name);
    }

 finish:
    Py_DECREF(value);
    return result;
}


/* Basically the "n" format code with the ability to turn None into -1. */
int
_PyIO_ConvertSsize_t(PyObject *obj, void *result) {
    Py_ssize_t limit;
    if (obj == Py_None) {
        limit = -1;
    }
    else if (PyNumber_Check(obj)) {
        limit = PyNumber_AsSsize_t(obj, PyExc_OverflowError);
        if (limit == -1 && PyErr_Occurred())
            return 0;
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "integer argument expected, got '%.200s'",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
    *((Py_ssize_t *)result) = limit;
    return 1;
}


_PyIO_State *
_PyIO_get_module_state(void)
{
    PyObject *mod = PyState_FindModule(&_PyIO_Module);
    _PyIO_State *state;
    if (mod == NULL || (state = IO_MOD_STATE(mod)) == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "could not find io module state "
                        "(interpreter shutdown?)");
        return NULL;
    }
    return state;
}

PyObject *
_PyIO_get_locale_module(_PyIO_State *state)
{
    PyObject *mod;
    if (state->locale_module != NULL) {
        assert(PyWeakref_CheckRef(state->locale_module));
        mod = PyWeakref_GET_OBJECT(state->locale_module);
        if (mod != Py_None) {
            Py_INCREF(mod);
            return mod;
        }
        Py_CLEAR(state->locale_module);
    }
    mod = PyImport_ImportModule("_bootlocale");
    if (mod == NULL)
        return NULL;
    state->locale_module = PyWeakref_NewRef(mod, NULL);
    if (state->locale_module == NULL) {
        Py_DECREF(mod);
        return NULL;
    }
    return mod;
}


static int
iomodule_traverse(PyObject *mod, visitproc visit, void *arg) {
    _PyIO_State *state = IO_MOD_STATE(mod);
    if (!state->initialized)
        return 0;
    if (state->locale_module != NULL) {
        Py_VISIT(state->locale_module);
    }
    Py_VISIT(state->unsupported_operation);
    return 0;
}


static int
iomodule_clear(PyObject *mod) {
    _PyIO_State *state = IO_MOD_STATE(mod);
    if (!state->initialized)
        return 0;
    if (state->locale_module != NULL)
        Py_CLEAR(state->locale_module);
    Py_CLEAR(state->unsupported_operation);
    return 0;
}

static void
iomodule_free(PyObject *mod) {
    iomodule_clear(mod);
}


/*
 * Module definition
 */

static PyMethodDef module_methods[] = {
    {"open", (PyCFunction)io_open, METH_VARARGS|METH_KEYWORDS, open_doc},
    {NULL, NULL}
};

struct PyModuleDef _PyIO_Module = {
    PyModuleDef_HEAD_INIT,
    "io",
    module_doc,
    sizeof(_PyIO_State),
    module_methods,
    NULL,
    iomodule_traverse,
    iomodule_clear,
    (freefunc)iomodule_free,
};

PyMODINIT_FUNC
PyInit__io(void)
{
    PyObject *m = PyModule_Create(&_PyIO_Module);
    _PyIO_State *state = NULL;
    if (m == NULL)
        return NULL;
    state = IO_MOD_STATE(m);
    state->initialized = 0;

#define ADD_TYPE(type, name) \
    if (PyType_Ready(type) < 0) \
        goto fail; \
    Py_INCREF(type); \
    if (PyModule_AddObject(m, name, (PyObject *)type) < 0) {  \
        Py_DECREF(type); \
        goto fail; \
    }

    /* DEFAULT_BUFFER_SIZE */
    if (PyModule_AddIntMacro(m, DEFAULT_BUFFER_SIZE) < 0)
        goto fail;

    /* UnsupportedOperation inherits from ValueError and IOError */
    state->unsupported_operation = PyObject_CallFunction(
        (PyObject *)&PyType_Type, "s(OO){}",
        "UnsupportedOperation", PyExc_ValueError, PyExc_IOError);
    if (state->unsupported_operation == NULL)
        goto fail;
    Py_INCREF(state->unsupported_operation);
    if (PyModule_AddObject(m, "UnsupportedOperation",
                           state->unsupported_operation) < 0)
        goto fail;

    /* BlockingIOError, for compatibility */
    Py_INCREF(PyExc_BlockingIOError);
    if (PyModule_AddObject(m, "BlockingIOError",
                           (PyObject *) PyExc_BlockingIOError) < 0)
        goto fail;

    /* Concrete base types of the IO ABCs.
       (the ABCs themselves are declared through inheritance in io.py)
    */
    ADD_TYPE(&PyIOBase_Type, "_IOBase");
    ADD_TYPE(&PyRawIOBase_Type, "_RawIOBase");
    ADD_TYPE(&PyBufferedIOBase_Type, "_BufferedIOBase");
    ADD_TYPE(&PyTextIOBase_Type, "_TextIOBase");

    /* Implementation of concrete IO objects. */
    /* FileIO */
    PyFileIO_Type.tp_base = &PyRawIOBase_Type;
    ADD_TYPE(&PyFileIO_Type, "FileIO");

    /* BytesIO */
    PyBytesIO_Type.tp_base = &PyBufferedIOBase_Type;
    ADD_TYPE(&PyBytesIO_Type, "BytesIO");
    if (PyType_Ready(&_PyBytesIOBuffer_Type) < 0)
        goto fail;

    /* StringIO */
    PyStringIO_Type.tp_base = &PyTextIOBase_Type;
    ADD_TYPE(&PyStringIO_Type, "StringIO");

    /* BufferedReader */
    PyBufferedReader_Type.tp_base = &PyBufferedIOBase_Type;
    ADD_TYPE(&PyBufferedReader_Type, "BufferedReader");

    /* BufferedWriter */
    PyBufferedWriter_Type.tp_base = &PyBufferedIOBase_Type;
    ADD_TYPE(&PyBufferedWriter_Type, "BufferedWriter");

    /* BufferedRWPair */
    PyBufferedRWPair_Type.tp_base = &PyBufferedIOBase_Type;
    ADD_TYPE(&PyBufferedRWPair_Type, "BufferedRWPair");

    /* BufferedRandom */
    PyBufferedRandom_Type.tp_base = &PyBufferedIOBase_Type;
    ADD_TYPE(&PyBufferedRandom_Type, "BufferedRandom");

    /* TextIOWrapper */
    PyTextIOWrapper_Type.tp_base = &PyTextIOBase_Type;
    ADD_TYPE(&PyTextIOWrapper_Type, "TextIOWrapper");

    /* IncrementalNewlineDecoder */
    ADD_TYPE(&PyIncrementalNewlineDecoder_Type, "IncrementalNewlineDecoder");

    /* Interned strings */
#define ADD_INTERNED(name) \
    if (!_PyIO_str_ ## name && \
        !(_PyIO_str_ ## name = PyUnicode_InternFromString(# name))) \
        goto fail;

    ADD_INTERNED(close)
    ADD_INTERNED(closed)
    ADD_INTERNED(decode)
    ADD_INTERNED(encode)
    ADD_INTERNED(fileno)
    ADD_INTERNED(flush)
    ADD_INTERNED(getstate)
    ADD_INTERNED(isatty)
    ADD_INTERNED(newlines)
    ADD_INTERNED(read)
    ADD_INTERNED(read1)
    ADD_INTERNED(readable)
    ADD_INTERNED(readall)
    ADD_INTERNED(readinto)
    ADD_INTERNED(readline)
    ADD_INTERNED(reset)
    ADD_INTERNED(seek)
    ADD_INTERNED(seekable)
    ADD_INTERNED(setstate)
    ADD_INTERNED(tell)
    ADD_INTERNED(truncate)
    ADD_INTERNED(write)
    ADD_INTERNED(writable)

    if (!_PyIO_str_nl &&
        !(_PyIO_str_nl = PyUnicode_InternFromString("\n")))
        goto fail;

    if (!_PyIO_empty_str &&
        !(_PyIO_empty_str = PyUnicode_FromStringAndSize(NULL, 0)))
        goto fail;
    if (!_PyIO_empty_bytes &&
        !(_PyIO_empty_bytes = PyBytes_FromStringAndSize(NULL, 0)))
        goto fail;
    if (!_PyIO_zero &&
        !(_PyIO_zero = PyLong_FromLong(0L)))
        goto fail;

    state->initialized = 1;

    return m;

  fail:
    Py_XDECREF(state->unsupported_operation);
    Py_DECREF(m);
    return NULL;
}
