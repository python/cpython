/*[clinic input]
preserve
[clinic start generated code]*/

#include "pycore_modsupport.h"    // _PyArg_CheckPositional()

PyDoc_STRVAR(fcntl_fcntl__doc__,
"fcntl($module, fd, cmd, arg=0, /)\n"
"--\n"
"\n"
"Perform the operation cmd on file descriptor fd.\n"
"\n"
"The values used for cmd are operating system dependent, and are\n"
"available as constants in the fcntl module, using the same names as used\n"
"in the relevant C header files.  The argument arg is optional, and\n"
"defaults to 0; it may be an integer, a bytes-like object or a string.\n"
"If arg is given as a string, it will be encoded to binary using the\n"
"UTF-8 encoding.\n"
"\n"
"If the arg given is an integer or if none is specified, the result value\n"
"is an integer corresponding to the return value of the fcntl() call in\n"
"the C code.\n"
"\n"
"If arg is given as a bytes-like object, the return value of fcntl() is a\n"
"bytes object of that length, containing the resulting value put in the\n"
"arg buffer by the operating system.");

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
"Perform the operation request on file descriptor fd.\n"
"\n"
"The values used for request are operating system dependent, and are\n"
"available as constants in the fcntl or termios library modules, using\n"
"the same names as used in the relevant C header files.\n"
"\n"
"The argument arg is optional, and defaults to 0; it may be an integer, a\n"
"bytes-like object or a string.  If arg is given as a string, it will be\n"
"encoded to binary using the UTF-8 encoding.\n"
"\n"
"If the arg given is an integer or if none is specified, the result value\n"
"is an integer corresponding to the return value of the ioctl() call in\n"
"the C code.\n"
"\n"
"If the argument is a mutable buffer (such as a bytearray) and the\n"
"mutate_flag argument is true (default) then the buffer is (in effect)\n"
"passed to the operating system and changes made by the OS will be\n"
"reflected in the contents of the buffer after the call has returned.\n"
"The return value is the integer returned by the ioctl() system call.\n"
"\n"
"If the argument is a mutable buffer and the mutable_flag argument is\n"
"false, the behavior is as if an immutable buffer had been passed.\n"
"\n"
"If the argument is an immutable buffer then a copy of the buffer is\n"
"passed to the operating system and the return value is a bytes object of\n"
"the same length containing whatever the operating system put in the\n"
"buffer.");

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
"Perform the lock operation on file descriptor fd.\n"
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
"fd is the file descriptor of the file to lock or unlock, and operation\n"
"is one of the following values:\n"
"\n"
"    LOCK_UN - unlock\n"
"    LOCK_SH - acquire a shared lock\n"
"    LOCK_EX - acquire an exclusive lock\n"
"\n"
"When operation is LOCK_SH or LOCK_EX, it can also be bitwise ORed with\n"
"LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and\n"
"the lock cannot be acquired, an OSError will be raised and the exception\n"
"will have an errno attribute set to EACCES or EAGAIN (depending on the\n"
"operating system -- for portability, check for either value).\n"
"\n"
"len is the number of bytes to lock, with the default meaning to lock to\n"
"EOF.  start is the byte offset, relative to whence, to that the lock\n"
"starts.  whence is as with fileobj.seek(), specifically:\n"
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
/*[clinic end generated code: output=c782fcf9dd6690e0 input=a9049054013a1b77]*/
