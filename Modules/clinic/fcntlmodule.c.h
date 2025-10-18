/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"          // PyGC_Head
#  include "pycore_runtime.h"     // _Py_ID()
#endif
#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(fcntl_fcntl__doc__,
"fcntl($module, fd, cmd, arg=0, /)\n"
"--\n"
"\n"
"Perform the operation `cmd` on file descriptor fd.\n"
"\n"
"The values used for `cmd` are operating system dependent, and are available\n"
"as constants in the fcntl module, using the same names as used in\n"
"the relevant C header files.  The argument arg is optional, and\n"
"defaults to 0; it may be an int or a string.  If arg is given as a string,\n"
"the return value of fcntl is a string of that length, containing the\n"
"resulting value put in the arg buffer by the operating system.  The length\n"
"of the arg string is not allowed to exceed 1024 bytes.  If the arg given\n"
"is an integer or if none is specified, the result value is an integer\n"
"corresponding to the return value of the fcntl call in the C code.");

#define FCNTL_FCNTL_METHODDEF    \
    {"fcntl", _PyCFunction_CAST(fcntl_fcntl), METH_FASTCALL, fcntl_fcntl__doc__},

static PyObject *
fcntl_fcntl_impl(PyObject *module, int fd, int code, PyObject *arg);

static PyObject *
fcntl_fcntl(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int code;
    PyObject *arg = NULL;

    if (!_PyArg_CheckPositional("fcntl", nargs, 2, 3)) {
        goto exit;
    }
    fd = PyObject_AsFileDescriptor(args[0]);
    if (fd < 0) {
        goto exit;
    }
    code = PyLong_AsInt(args[1]);
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    arg = args[2];
skip_optional:
    return_value = fcntl_fcntl_impl(module, fd, code, arg);

exit:
    return return_value;
}

PyDoc_STRVAR(fcntl_ioctl__doc__,
"ioctl($module, fd, request, arg=0, mutate_flag=True, /)\n"
"--\n"
"\n"
"Perform the operation `request` on file descriptor `fd`.\n"
"\n"
"The values used for `request` are operating system dependent, and are available\n"
"as constants in the fcntl or termios library modules, using the same names as\n"
"used in the relevant C header files.\n"
"\n"
"The argument `arg` is optional, and defaults to 0; it may be an int or a\n"
"buffer containing character data (most likely a string or an array).\n"
"\n"
"If the argument is a mutable buffer (such as an array) and if the\n"
"mutate_flag argument (which is only allowed in this case) is true then the\n"
"buffer is (in effect) passed to the operating system and changes made by\n"
"the OS will be reflected in the contents of the buffer after the call has\n"
"returned.  The return value is the integer returned by the ioctl system\n"
"call.\n"
"\n"
"If the argument is a mutable buffer and the mutable_flag argument is false,\n"
"the behavior is as if a string had been passed.\n"
"\n"
"If the argument is an immutable buffer (most likely a string) then a copy\n"
"of the buffer is passed to the operating system and the return value is a\n"
"string of the same length containing whatever the operating system put in\n"
"the buffer.  The length of the arg buffer in this case is not allowed to\n"
"exceed 1024 bytes.\n"
"\n"
"If the arg given is an integer or if none is specified, the result value is\n"
"an integer corresponding to the return value of the ioctl call in the C\n"
"code.");

#define FCNTL_IOCTL_METHODDEF    \
    {"ioctl", _PyCFunction_CAST(fcntl_ioctl), METH_FASTCALL, fcntl_ioctl__doc__},

static PyObject *
fcntl_ioctl_impl(PyObject *module, int fd, unsigned long code, PyObject *arg,
                 int mutate_arg);

static PyObject *
fcntl_ioctl(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    unsigned long code;
    PyObject *arg = NULL;
    int mutate_arg = 1;

    if (!_PyArg_CheckPositional("ioctl", nargs, 2, 4)) {
        goto exit;
    }
    fd = PyObject_AsFileDescriptor(args[0]);
    if (fd < 0) {
        goto exit;
    }
    if (!PyIndex_Check(args[1])) {
        _PyArg_BadArgument("ioctl", "argument 2", "int", args[1]);
        goto exit;
    }
    {
        Py_ssize_t _bytes = PyLong_AsNativeBytes(args[1], &code, sizeof(unsigned long),
                Py_ASNATIVEBYTES_NATIVE_ENDIAN |
                Py_ASNATIVEBYTES_ALLOW_INDEX |
                Py_ASNATIVEBYTES_UNSIGNED_BUFFER);
        if (_bytes < 0) {
            goto exit;
        }
        if ((size_t)_bytes > sizeof(unsigned long)) {
            if (PyErr_WarnEx(PyExc_DeprecationWarning,
                "integer value out of range", 1) < 0)
            {
                goto exit;
            }
        }
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    arg = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    mutate_arg = PyObject_IsTrue(args[3]);
    if (mutate_arg < 0) {
        goto exit;
    }
skip_optional:
    return_value = fcntl_ioctl_impl(module, fd, code, arg, mutate_arg);

exit:
    return return_value;
}

PyDoc_STRVAR(fcntl_flock__doc__,
"flock($module, fd, operation, /)\n"
"--\n"
"\n"
"Perform the lock operation `operation` on file descriptor `fd`.\n"
"\n"
"See the Unix manual page for flock(2) for details (On some systems, this\n"
"function is emulated using fcntl()).");

#define FCNTL_FLOCK_METHODDEF    \
    {"flock", _PyCFunction_CAST(fcntl_flock), METH_FASTCALL, fcntl_flock__doc__},

static PyObject *
fcntl_flock_impl(PyObject *module, int fd, int code);

static PyObject *
fcntl_flock(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int code;

    if (!_PyArg_CheckPositional("flock", nargs, 2, 2)) {
        goto exit;
    }
    fd = PyObject_AsFileDescriptor(args[0]);
    if (fd < 0) {
        goto exit;
    }
    code = PyLong_AsInt(args[1]);
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = fcntl_flock_impl(module, fd, code);

exit:
    return return_value;
}

PyDoc_STRVAR(fcntl_lockf__doc__,
"lockf($module, fd, cmd, len=0, start=0, whence=0, /)\n"
"--\n"
"\n"
"A wrapper around the fcntl() locking calls.\n"
"\n"
"`fd` is the file descriptor of the file to lock or unlock, and operation is one\n"
"of the following values:\n"
"\n"
"    LOCK_UN - unlock\n"
"    LOCK_SH - acquire a shared lock\n"
"    LOCK_EX - acquire an exclusive lock\n"
"\n"
"When operation is LOCK_SH or LOCK_EX, it can also be bitwise ORed with\n"
"LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and the\n"
"lock cannot be acquired, an OSError will be raised and the exception will\n"
"have an errno attribute set to EACCES or EAGAIN (depending on the operating\n"
"system -- for portability, check for either value).\n"
"\n"
"`len` is the number of bytes to lock, with the default meaning to lock to\n"
"EOF.  `start` is the byte offset, relative to `whence`, to that the lock\n"
"starts.  `whence` is as with fileobj.seek(), specifically:\n"
"\n"
"    0 - relative to the start of the file (SEEK_SET)\n"
"    1 - relative to the current buffer position (SEEK_CUR)\n"
"    2 - relative to the end of the file (SEEK_END)");

#define FCNTL_LOCKF_METHODDEF    \
    {"lockf", _PyCFunction_CAST(fcntl_lockf), METH_FASTCALL, fcntl_lockf__doc__},

static PyObject *
fcntl_lockf_impl(PyObject *module, int fd, int code, PyObject *lenobj,
                 PyObject *startobj, int whence);

static PyObject *
fcntl_lockf(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int code;
    PyObject *lenobj = NULL;
    PyObject *startobj = NULL;
    int whence = 0;

    if (!_PyArg_CheckPositional("lockf", nargs, 2, 5)) {
        goto exit;
    }
    fd = PyObject_AsFileDescriptor(args[0]);
    if (fd < 0) {
        goto exit;
    }
    code = PyLong_AsInt(args[1]);
    if (code == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    lenobj = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    startobj = args[3];
    if (nargs < 5) {
        goto skip_optional;
    }
    whence = PyLong_AsInt(args[4]);
    if (whence == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional:
    return_value = fcntl_lockf_impl(module, fd, code, lenobj, startobj, whence);

exit:
    return return_value;
}

#if defined(F_PREALLOCATE)

PyDoc_STRVAR(fcntl_fstore___init____doc__,
"fstore(flags=0, posmode=0, offset=0, length=0)\n"
"--\n"
"\n"
"Initialize an fstore structure.\n"
"\n"
"This structure is used with the F_PREALLOCATE command to preallocate\n"
"file space on macOS systems.");

static int
fcntl_fstore___init___impl(FStoreObject *self, int flags, int posmode,
                           long offset, long length);

static int
fcntl_fstore___init__(PyObject *self, PyObject *args, PyObject *kwargs)
{
    int return_value = -1;
    #if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)

    #define NUM_KEYWORDS 4
    static struct {
        PyGC_Head _this_is_not_used;
        PyObject_VAR_HEAD
        Py_hash_t ob_hash;
        PyObject *ob_item[NUM_KEYWORDS];
    } _kwtuple = {
        .ob_base = PyVarObject_HEAD_INIT(&PyTuple_Type, NUM_KEYWORDS)
        .ob_hash = -1,
        .ob_item = { &_Py_ID(flags), &_Py_ID(posmode), &_Py_ID(offset), &_Py_ID(length), },
    };
    #undef NUM_KEYWORDS
    #define KWTUPLE (&_kwtuple.ob_base.ob_base)

    #else  // !Py_BUILD_CORE
    #  define KWTUPLE NULL
    #endif  // !Py_BUILD_CORE

    static const char * const _keywords[] = {"flags", "posmode", "offset", "length", NULL};
    static _PyArg_Parser _parser = {
        .keywords = _keywords,
        .fname = "fstore",
        .kwtuple = KWTUPLE,
    };
    #undef KWTUPLE
    PyObject *argsbuf[4];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int flags = 0;
    int posmode = 0;
    long offset = 0;
    long length = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser,
            /*minpos*/ 0, /*maxpos*/ 4, /*minkw*/ 0, /*varpos*/ 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        flags = PyLong_AsInt(fastargs[0]);
        if (flags == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[1]) {
        posmode = PyLong_AsInt(fastargs[1]);
        if (posmode == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    if (fastargs[2]) {
        offset = PyLong_AsLong(fastargs[2]);
        if (offset == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    length = PyLong_AsLong(fastargs[3]);
    if (length == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = fcntl_fstore___init___impl((FStoreObject *)self, flags, posmode, offset, length);

exit:
    return return_value;
}

#endif /* defined(F_PREALLOCATE) */

#if defined(F_PREALLOCATE)

PyDoc_STRVAR(fcntl_fstore_from_buffer__doc__,
"from_buffer($type, data, /)\n"
"--\n"
"\n"
"Create an fstore instance from bytes data.\n"
"\n"
"This is useful for creating an fstore instance from the result of\n"
"fcntl.fcntl when using the F_PREALLOCATE command.\n"
"\n"
"Raises ValueError if the data is not exactly the size of struct fstore.");

#define FCNTL_FSTORE_FROM_BUFFER_METHODDEF    \
    {"from_buffer", (PyCFunction)fcntl_fstore_from_buffer, METH_O|METH_CLASS, fcntl_fstore_from_buffer__doc__},

static PyObject *
fcntl_fstore_from_buffer_impl(PyTypeObject *type, Py_buffer *data);

static PyObject *
fcntl_fstore_from_buffer(PyObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    Py_buffer data = {NULL, NULL};

    if (PyObject_GetBuffer(arg, &data, PyBUF_SIMPLE) != 0) {
        goto exit;
    }
    return_value = fcntl_fstore_from_buffer_impl((PyTypeObject *)type, &data);

exit:
    /* Cleanup for data */
    if (data.obj) {
       PyBuffer_Release(&data);
    }

    return return_value;
}

#endif /* defined(F_PREALLOCATE) */

#ifndef FCNTL_FSTORE_FROM_BUFFER_METHODDEF
    #define FCNTL_FSTORE_FROM_BUFFER_METHODDEF
#endif /* !defined(FCNTL_FSTORE_FROM_BUFFER_METHODDEF) */
/*[clinic end generated code: output=e6b4d62e7dcc1068 input=a9049054013a1b77]*/
