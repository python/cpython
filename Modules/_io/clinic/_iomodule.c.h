/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_UnpackKeywords()

PyDoc_STRVAR(_io_open__doc__,
"open($module, /, file, mode=\'r\', buffering=-1, encoding=None,\n"
"     errors=None, newline=None, closefd=True, opener=None)\n"
"--\n"
"\n"
"Open file and return a stream.  Raise OSError upon failure.\n"
"\n"
"file is either a text or byte string giving the name (and the path\n"
"if the file isn\'t in the current working directory) of the file to\n"
"be opened or an integer file descriptor of the file to be\n"
"wrapped. (If a file descriptor is given, it is closed when the\n"
"returned I/O object is closed, unless closefd is set to False.)\n"
"\n"
"mode is an optional string that specifies the mode in which the file\n"
"is opened. It defaults to \'r\' which means open for reading in text\n"
"mode.  Other common values are \'w\' for writing (truncating the file if\n"
"it already exists), \'x\' for creating and writing to a new file, and\n"
"\'a\' for appending (which on some Unix systems, means that all writes\n"
"append to the end of the file regardless of the current seek position).\n"
"In text mode, if encoding is not specified the encoding used is platform\n"
"dependent: locale.getencoding() is called to get the current locale encoding.\n"
"(For reading and writing raw bytes use binary mode and leave encoding\n"
"unspecified.) The available modes are:\n"
"\n"
"========= ===============================================================\n"
"Character Meaning\n"
"--------- ---------------------------------------------------------------\n"
"\'r\'       open for reading (default)\n"
"\'w\'       open for writing, truncating the file first\n"
"\'x\'       create a new file and open it for writing\n"
"\'a\'       open for writing, appending to the end of the file if it exists\n"
"\'b\'       binary mode\n"
"\'t\'       text mode (default)\n"
"\'+\'       open a disk file for updating (reading and writing)\n"
"========= ===============================================================\n"
"\n"
"The default mode is \'rt\' (open for reading text). For binary random\n"
"access, the mode \'w+b\' opens and truncates the file to 0 bytes, while\n"
"\'r+b\' opens the file without truncation. The \'x\' mode implies \'w\' and\n"
"raises an `FileExistsError` if the file already exists.\n"
"\n"
"Python distinguishes between files opened in binary and text modes,\n"
"even when the underlying operating system doesn\'t. Files opened in\n"
"binary mode (appending \'b\' to the mode argument) return contents as\n"
"bytes objects without any decoding. In text mode (the default, or when\n"
"\'t\' is appended to the mode argument), the contents of the file are\n"
"returned as strings, the bytes having been first decoded using a\n"
"platform-dependent encoding or using the specified encoding if given.\n"
"\n"
"buffering is an optional integer used to set the buffering policy.\n"
"Pass 0 to switch buffering off (only allowed in binary mode), 1 to select\n"
"line buffering (only usable in text mode), and an integer > 1 to indicate\n"
"the size of a fixed-size chunk buffer.  When no buffering argument is\n"
"given, the default buffering policy works as follows:\n"
"\n"
"* Binary files are buffered in fixed-size chunks; the size of the buffer\n"
"  is chosen using a heuristic trying to determine the underlying device\'s\n"
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
"\'strict\' to raise a ValueError exception if there is an encoding error\n"
"(the default of None has the same effect), or pass \'ignore\' to ignore\n"
"errors. (Note that ignoring encoding errors can lead to data loss.)\n"
"See the documentation for codecs.register or run \'help(codecs.Codec)\'\n"
"for a list of the permitted encoding error strings.\n"
"\n"
"newline controls how universal newlines works (it only applies to text\n"
"mode). It can be None, \'\', \'\\n\', \'\\r\', and \'\\r\\n\'.  It works as\n"
"follows:\n"
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
"are performed. When open() is used to open a file in a text mode (\'w\',\n"
"\'r\', \'wt\', \'rt\', etc.), it returns a TextIOWrapper. When used to open\n"
"a file in a binary mode, the returned class varies: in read binary\n"
"mode, it returns a BufferedReader; in write binary and append binary\n"
"modes, it returns a BufferedWriter, and in read/write mode, it returns\n"
"a BufferedRandom.\n"
"\n"
"It is also possible to use a string or bytearray as a file for both\n"
"reading and writing. For strings StringIO can be used like a file\n"
"opened in a text mode, and for bytes a BytesIO can be used like a file\n"
"opened in a binary mode.");

#define _IO_OPEN_METHODDEF    \
    {"open", _PyCFunction_CAST(_io_open), METH_FASTCALL|METH_KEYWORDS, _io_open__doc__},

static PyObject *
_io_open_impl(PyObject *module, PyObject *file, const char *mode,
              int buffering, const char *encoding, const char *errors,
              const char *newline, int closefd, PyObject *opener);

static PyObject *
_io_open(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 8
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(file), &_Py_ID(mode), &_Py_ID(buffering), &_Py_ID(encoding), &_Py_ID(errors), &_Py_ID(newline), &_Py_ID(closefd), &_Py_ID(opener), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"file", "mode", "buffering", "encoding", "errors", "newline", "closefd", "opener", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "open",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[8];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    PyObject *file;
    const char *mode = "r";
    int buffering = -1;
    const char *encoding = NULL;
    const char *errors = NULL;
    const char *newline = NULL;
    int closefd = 1;
    PyObject *opener = Py_None;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 8, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    file = args[0];
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[1]) {
        if (!PyUnicode_Check(args[1])) {
            _PyArg_BadArgument("open", "argument 'mode'", "str", args[1]);
            goto exit;
        }
        Py_ssize_t mode_length;
        mode = PyUnicode_AsUTF8AndSize(args[1], &mode_length);
        if (mode == NULL) {
            goto exit;
        }
        if (strlen(mode) != (size_t)mode_length) {
            PyErr_SetString(PyExc_ValueError, "embedded null character");
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[2]) {
        buffering = PyLong_AsInt(args[2]);
        if (buffering == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[3]) {
        if (args[3] == Py_None) {
            encoding = NULL;
        }
        else if (PyUnicode_Check(args[3])) {
            Py_ssize_t encoding_length;
            encoding = PyUnicode_AsUTF8AndSize(args[3], &encoding_length);
            if (encoding == NULL) {
                goto exit;
            }
            if (strlen(encoding) != (size_t)encoding_length) {
                PyErr_SetString(PyExc_ValueError, "embedded null character");
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("open", "argument 'encoding'", "str or None", args[3]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[4]) {
        if (args[4] == Py_None) {
            errors = NULL;
        }
        else if (PyUnicode_Check(args[4])) {
            Py_ssize_t errors_length;
            errors = PyUnicode_AsUTF8AndSize(args[4], &errors_length);
            if (errors == NULL) {
                goto exit;
            }
            if (strlen(errors) != (size_t)errors_length) {
                PyErr_SetString(PyExc_ValueError, "embedded null character");
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("open", "argument 'errors'", "str or None", args[4]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[5]) {
        if (args[5] == Py_None) {
            newline = NULL;
        }
        else if (PyUnicode_Check(args[5])) {
            Py_ssize_t newline_length;
            newline = PyUnicode_AsUTF8AndSize(args[5], &newline_length);
            if (newline == NULL) {
                goto exit;
            }
            if (strlen(newline) != (size_t)newline_length) {
                PyErr_SetString(PyExc_ValueError, "embedded null character");
                goto exit;
            }
        }
        else {
            _PyArg_BadArgument("open", "argument 'newline'", "str or None", args[5]);
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (args[6]) {
        closefd = PyObject_IsTrue(args[6]);
        if (closefd < 0) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    opener = args[7];
skip_optional_pos:
    return_value = _io_open_impl(module, file, mode, buffering, encoding, errors, newline, closefd, opener);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_text_encoding__doc__,
"text_encoding($module, encoding, stacklevel=2, /)\n"
"--\n"
"\n"
"A helper function to choose the text encoding.\n"
"\n"
"When encoding is not None, this function returns it.\n"
"Otherwise, this function returns the default text encoding\n"
"(i.e. \"locale\" or \"utf-8\" depends on UTF-8 mode).\n"
"\n"
"This function emits an EncodingWarning if encoding is None and\n"
"sys.flags.warn_default_encoding is true.\n"
"\n"
"This can be used in APIs with an encoding=None parameter.\n"
"However, please consider using encoding=\"utf-8\" for new APIs.");

#define _IO_TEXT_ENCODING_METHODDEF    \
    {"text_encoding", _PyCFunction_CAST(_io_text_encoding), METH_FASTCALL, _io_text_encoding__doc__},

static PyObject *
_io_text_encoding_impl(PyObject *module, PyObject *encoding, int stacklevel);

static PyObject *
_io_text_encoding(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *encoding;
    int stacklevel = 2;

    if (!_PyArg_CheckPositional("text_encoding", nargs, 1, 2)) {
        goto exit;
    }
    encoding = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    stacklevel = PyLong_AsInt(args[1]);
    if (stacklevel == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = _io_text_encoding_impl(module, encoding, stacklevel);

exit:
    return return_value;
}

PyDoc_STRVAR(_io_open_code__doc__,
"open_code($module, /, path)\n"
"--\n"
"\n"
"Opens the provided file with the intent to import the contents.\n"
"\n"
"This may perform extra validation beyond open(), but is otherwise interchangeable\n"
"with calling open(path, \'rb\').");

#define _IO_OPEN_CODE_METHODDEF    \
    {"open_code", _PyCFunction_CAST(_io_open_code), METH_FASTCALL|METH_KEYWORDS, _io_open_code__doc__},

static PyObject *
_io_open_code_impl(PyObject *module, PyObject *path);

static PyObject *
_io_open_code(PyObject *module, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 1
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_item = { &_Py_ID(path), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"path", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "open_code",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[1];
    PyObject *path;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!PyUnicode_Check(args[0])) {
        _PyArg_BadArgument("open_code", "argument 'path'", "str", args[0]);
        goto exit;
    }
    path = args[0];
    return_value = _io_open_code_impl(module, path);

exit:
    return return_value;
}
/*[clinic end generated code: output=5d60f4e778a600a4 input=a9049054013a1b77]*/
