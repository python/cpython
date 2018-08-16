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

#ifdef MS_WINDOWS
#include <windows.h>
#endif

/* Various interned strings */

PyObject *_PyIO_str_close = NULL;
PyObject *_PyIO_str_closed = NULL;
PyObject *_PyIO_str_decode = NULL;
PyObject *_PyIO_str_encode = NULL;
PyObject *_PyIO_str_fileno = NULL;
PyObject *_PyIO_str_flush = NULL;
PyObject *_PyIO_str_getstate = NULL;
PyObject *_PyIO_str_isatty = NULL;
PyObject *_PyIO_str_newlines = NULL;
PyObject *_PyIO_str_nl = NULL;
PyObject *_PyIO_str_peek = NULL;
PyObject *_PyIO_str_read = NULL;
PyObject *_PyIO_str_read1 = NULL;
PyObject *_PyIO_str_readable = NULL;
PyObject *_PyIO_str_readall = NULL;
PyObject *_PyIO_str_readinto = NULL;
PyObject *_PyIO_str_readline = NULL;
PyObject *_PyIO_str_reset = NULL;
PyObject *_PyIO_str_seek = NULL;
PyObject *_PyIO_str_seekable = NULL;
PyObject *_PyIO_str_setstate = NULL;
PyObject *_PyIO_str_tell = NULL;
PyObject *_PyIO_str_truncate = NULL;
PyObject *_PyIO_str_writable = NULL;
PyObject *_PyIO_str_write = NULL;

PyObject *_PyIO_empty_str = NULL;
PyObject *_PyIO_empty_bytes = NULL;

PyDoc_STRVAR(module_doc,
"The io module provides the Python interfaces to stream handling. The\n"
"builtin open function is defined in this module.\n"
"\n"
"At the top of the I/O hierarchy is the abstract base class IOBase. It\n"
"defines the basic interface to a stream. Note, however, that there is no\n"
"separation between reading and writing to streams; implementations are\n"
"allowed to raise an OSError if they do not support a given operation.\n"
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
"is an in-memory stream for text.\n"
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
/*[clinic input]
module _io

_io.open
    file: object
    mode: str = "r"
    buffering: int = -1
    encoding: str(accept={str, NoneType}) = NULL
    errors: str(accept={str, NoneType}) = NULL
    newline: str(accept={str, NoneType}) = NULL
    closefd: bool(accept={int}) = True
    opener: object = None

Open file and return a stream.  Raise OSError upon failure.

file is either a text or byte string giving the name (and the path
if the file isn't in the current working directory) of the file to
be opened or an integer file descriptor of the file to be
wrapped. (If a file descriptor is given, it is closed when the
returned I/O object is closed, unless closefd is set to False.)

mode is an optional string that specifies the mode in which the file
is opened. It defaults to 'r' which means open for reading in text
mode.  Other common values are 'w' for writing (truncating the file if
it already exists), 'x' for creating and writing to a new file, and
'a' for appending (which on some Unix systems, means that all writes
append to the end of the file regardless of the current seek position).
In text mode, if encoding is not specified the encoding used is platform
dependent: locale.getpreferredencoding(False) is called to get the
current locale encoding. (For reading and writing raw bytes use binary
mode and leave encoding unspecified.) The available modes are:

========= ===============================================================
Character Meaning
--------- ---------------------------------------------------------------
'r'       open for reading (default)
'w'       open for writing, truncating the file first
'x'       create a new file and open it for writing
'a'       open for writing, appending to the end of the file if it exists
'b'       binary mode
't'       text mode (default)
'+'       open a disk file for updating (reading and writing)
'U'       universal newline mode (deprecated)
========= ===============================================================

The default mode is 'rt' (open for reading text). For binary random
access, the mode 'w+b' opens and truncates the file to 0 bytes, while
'r+b' opens the file without truncation. The 'x' mode implies 'w' and
raises an `FileExistsError` if the file already exists.

Python distinguishes between files opened in binary and text modes,
even when the underlying operating system doesn't. Files opened in
binary mode (appending 'b' to the mode argument) return contents as
bytes objects without any decoding. In text mode (the default, or when
't' is appended to the mode argument), the contents of the file are
returned as strings, the bytes having been first decoded using a
platform-dependent encoding or using the specified encoding if given.

'U' mode is deprecated and will raise an exception in future versions
of Python.  It has no effect in Python 3.  Use newline to control
universal newlines mode.

buffering is an optional integer used to set the buffering policy.
Pass 0 to switch buffering off (only allowed in binary mode), 1 to select
line buffering (only usable in text mode), and an integer > 1 to indicate
the size of a fixed-size chunk buffer.  When no buffering argument is
given, the default buffering policy works as follows:

* Binary files are buffered in fixed-size chunks; the size of the buffer
  is chosen using a heuristic trying to determine the underlying device's
  "block size" and falling back on `io.DEFAULT_BUFFER_SIZE`.
  On many systems, the buffer will typically be 4096 or 8192 bytes long.

* "Interactive" text files (files for which isatty() returns True)
  use line buffering.  Other text files use the policy described above
  for binary files.

encoding is the name of the encoding used to decode or encode the
file. This should only be used in text mode. The default encoding is
platform dependent, but any encoding supported by Python can be
passed.  See the codecs module for the list of supported encodings.

errors is an optional string that specifies how encoding errors are to
be handled---this argument should not be used in binary mode. Pass
'strict' to raise a ValueError exception if there is an encoding error
(the default of None has the same effect), or pass 'ignore' to ignore
errors. (Note that ignoring encoding errors can lead to data loss.)
See the documentation for codecs.register or run 'help(codecs.Codec)'
for a list of the permitted encoding error strings.

newline controls how universal newlines works (it only applies to text
mode). It can be None, '', '\n', '\r', and '\r\n'.  It works as
follows:

* On input, if newline is None, universal newlines mode is
  enabled. Lines in the input can end in '\n', '\r', or '\r\n', and
  these are translated into '\n' before being returned to the
  caller. If it is '', universal newline mode is enabled, but line
  endings are returned to the caller untranslated. If it has any of
  the other legal values, input lines are only terminated by the given
  string, and the line ending is returned to the caller untranslated.

* On output, if newline is None, any '\n' characters written are
  translated to the system default line separator, os.linesep. If
  newline is '' or '\n', no translation takes place. If newline is any
  of the other legal values, any '\n' characters written are translated
  to the given string.

If closefd is False, the underlying file descriptor will be kept open
when the file is closed. This does not work when a file name is given
and must be True in that case.

A custom opener can be used by passing a callable as *opener*. The
underlying file descriptor for the file object is then obtained by
calling *opener* with (*file*, *flags*). *opener* must return an open
file descriptor (passing os.open as *opener* results in functionality
similar to passing None).

open() returns a file object whose type depends on the mode, and
through which the standard file operations such as reading and writing
are performed. When open() is used to open a file in a text mode ('w',
'r', 'wt', 'rt', etc.), it returns a TextIOWrapper. When used to open
a file in a binary mode, the returned class varies: in read binary
mode, it returns a BufferedReader; in write binary and append binary
modes, it returns a BufferedWriter, and in read/write mode, it returns
a BufferedRandom.

It is also possible to use a string or bytearray as a file for both
reading and writing. For strings StringIO can be used like a file
opened in a text mode, and for bytes a BytesIO can be used like a file
opened in a binary mode.
[clinic start generated code]*/

static PyObject *
_io_open_impl(PyObject *module, PyObject *file, const char *mode,
              int buffering, const char *encoding, const char *errors,
              const char *newline, int closefd, PyObject *opener)
/*[clinic end generated code: output=aefafc4ce2b46dc0 input=03da2940c8a65871]*/
{
    unsigned i;

    int creating = 0, reading = 0, writing = 0, appending = 0, updating = 0;
    int text = 0, binary = 0, universal = 0;

    char rawmode[6], *m;
    int line_buffering, is_number;
    long isatty;

    PyObject *raw, *modeobj = NULL, *buffer, *wrapper, *result = NULL, *path_or_fd = NULL;

    _Py_IDENTIFIER(_blksize);
    _Py_IDENTIFIER(isatty);
    _Py_IDENTIFIER(mode);
    _Py_IDENTIFIER(close);

    is_number = PyNumber_Check(file);

    if (is_number) {
        path_or_fd = file;
        Py_INCREF(path_or_fd);
    } else {
        path_or_fd = PyOS_FSPath(file);
        if (path_or_fd == NULL) {
            return NULL;
        }
    }

    if (!is_number &&
        !PyUnicode_Check(path_or_fd) &&
        !PyBytes_Check(path_or_fd)) {
        PyErr_Format(PyExc_TypeError, "invalid file: %R", file);
        goto error;
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
            goto error;
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
        if (creating || writing || appending || updating) {
            PyErr_SetString(PyExc_ValueError,
                            "mode U cannot be combined with x', 'w', 'a', or '+'");
            goto error;
        }
        if (PyErr_WarnEx(PyExc_DeprecationWarning,
                         "'U' mode is deprecated", 1) < 0)
            goto error;
        reading = 1;
    }

    if (text && binary) {
        PyErr_SetString(PyExc_ValueError,
                        "can't have text and binary mode at once");
        goto error;
    }

    if (creating + reading + writing + appending > 1) {
        PyErr_SetString(PyExc_ValueError,
                        "must have exactly one of create/read/write/append mode");
        goto error;
    }

    if (binary && encoding != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an encoding argument");
        goto error;
    }

    if (binary && errors != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take an errors argument");
        goto error;
    }

    if (binary && newline != NULL) {
        PyErr_SetString(PyExc_ValueError,
                        "binary mode doesn't take a newline argument");
        goto error;
    }

    /* Create the Raw file stream */
    {
        PyObject *RawIO_class = (PyObject *)&PyFileIO_Type;
#ifdef MS_WINDOWS
        if (!Py_LegacyWindowsStdioFlag && _PyIO_get_console_type(path_or_fd) != '\0') {
            RawIO_class = (PyObject *)&PyWindowsConsoleIO_Type;
            encoding = "utf-8";
        }
#endif
        raw = PyObject_CallFunction(RawIO_class,
                                    "OsiO", path_or_fd, rawmode, closefd, opener);
    }

    if (raw == NULL)
        goto error;
    result = raw;

    Py_DECREF(path_or_fd);
    path_or_fd = NULL;

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
        PyObject *blksize_obj;
        blksize_obj = _PyObject_GetAttrId(raw, &PyId__blksize);
        if (blksize_obj == NULL)
            goto error;
        buffering = PyLong_AsLong(blksize_obj);
        Py_DECREF(blksize_obj);
        if (buffering == -1 && PyErr_Occurred())
            goto error;
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
    Py_XDECREF(path_or_fd);
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

#include "clinic/_iomodule.c.h"

static PyMethodDef module_methods[] = {
    _IO_OPEN_METHODDEF
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

    /* UnsupportedOperation inherits from ValueError and OSError */
    state->unsupported_operation = PyObject_CallFunction(
        (PyObject *)&PyType_Type, "s(OO){}",
        "UnsupportedOperation", PyExc_OSError, PyExc_ValueError);
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

#ifdef MS_WINDOWS
    /* WindowsConsoleIO */
    PyWindowsConsoleIO_Type.tp_base = &PyRawIOBase_Type;
    ADD_TYPE(&PyWindowsConsoleIO_Type, "_WindowsConsoleIO");
#endif

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
    ADD_INTERNED(peek)
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

    state->initialized = 1;

    return m;

  fail:
    Py_XDECREF(state->unsupported_operation);
    Py_DECREF(m);
    return NULL;
}
