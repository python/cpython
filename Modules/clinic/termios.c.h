/*[clinic input]
preserve
[clinic start generated code]*/

#if defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_MODULE)
#  include "pycore_gc.h"            // PyGC_Head
#  include "pycore_runtime.h"       // _Py_ID()
#endif


PyDoc_STRVAR(termios_tcgetattr__doc__,
"tcgetattr($module, fd, /)\n"
"--\n"
"\n"
"Get the tty attributes for file descriptor fd.\n"
"\n"
"Returns a list [iflag, oflag, cflag, lflag, ispeed, ospeed, cc]\n"
"where cc is a list of the tty special characters (each a string of\n"
"length 1, except the items with indices VMIN and VTIME, which are\n"
"integers when these fields are defined).  The interpretation of the\n"
"flags and the speeds as well as the indexing in the cc array must be\n"
"done using the symbolic constants defined in this module.");

#define TERMIOS_TCGETATTR_METHODDEF    \
    {"tcgetattr", (PyCFunction)termios_tcgetattr, METH_O, termios_tcgetattr__doc__},

static PyObject *
termios_tcgetattr_impl(PyObject *module, int fd);

static PyObject *
termios_tcgetattr(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!_PyLong_FileDescriptor_Converter(arg, &fd)) {
        goto exit;
    }
    return_value = termios_tcgetattr_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsetattr__doc__,
"tcsetattr($module, fd, when, attributes, /)\n"
"--\n"
"\n"
"Set the tty attributes for file descriptor fd.\n"
"\n"
"The attributes to be set are taken from the attributes argument, which\n"
"is a list like the one returned by tcgetattr(). The when argument\n"
"determines when the attributes are changed: termios.TCSANOW to\n"
"change immediately, termios.TCSADRAIN to change after transmitting all\n"
"queued output, or termios.TCSAFLUSH to change after transmitting all\n"
"queued output and discarding all queued input.");

#define TERMIOS_TCSETATTR_METHODDEF    \
    {"tcsetattr", _PyCFunction_CAST(termios_tcsetattr), METH_FASTCALL, termios_tcsetattr__doc__},

static PyObject *
termios_tcsetattr_impl(PyObject *module, int fd, int when, PyObject *term);

static PyObject *
termios_tcsetattr(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int when;
    PyObject *term;

    if (!_PyArg_CheckPositional("tcsetattr", nargs, 3, 3)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    when = _PyLong_AsInt(args[1]);
    if (when == -1 && PyErr_Occurred()) {
        goto exit;
    }
    term = args[2];
    return_value = termios_tcsetattr_impl(module, fd, when, term);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsendbreak__doc__,
"tcsendbreak($module, fd, duration, /)\n"
"--\n"
"\n"
"Send a break on file descriptor fd.\n"
"\n"
"A zero duration sends a break for 0.25-0.5 seconds; a nonzero duration\n"
"has a system dependent meaning.");

#define TERMIOS_TCSENDBREAK_METHODDEF    \
    {"tcsendbreak", _PyCFunction_CAST(termios_tcsendbreak), METH_FASTCALL, termios_tcsendbreak__doc__},

static PyObject *
termios_tcsendbreak_impl(PyObject *module, int fd, int duration);

static PyObject *
termios_tcsendbreak(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int duration;

    if (!_PyArg_CheckPositional("tcsendbreak", nargs, 2, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    duration = _PyLong_AsInt(args[1]);
    if (duration == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcsendbreak_impl(module, fd, duration);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcdrain__doc__,
"tcdrain($module, fd, /)\n"
"--\n"
"\n"
"Wait until all output written to file descriptor fd has been transmitted.");

#define TERMIOS_TCDRAIN_METHODDEF    \
    {"tcdrain", (PyCFunction)termios_tcdrain, METH_O, termios_tcdrain__doc__},

static PyObject *
termios_tcdrain_impl(PyObject *module, int fd);

static PyObject *
termios_tcdrain(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!_PyLong_FileDescriptor_Converter(arg, &fd)) {
        goto exit;
    }
    return_value = termios_tcdrain_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcflush__doc__,
"tcflush($module, fd, queue, /)\n"
"--\n"
"\n"
"Discard queued data on file descriptor fd.\n"
"\n"
"The queue selector specifies which queue: termios.TCIFLUSH for the input\n"
"queue, termios.TCOFLUSH for the output queue, or termios.TCIOFLUSH for\n"
"both queues.");

#define TERMIOS_TCFLUSH_METHODDEF    \
    {"tcflush", _PyCFunction_CAST(termios_tcflush), METH_FASTCALL, termios_tcflush__doc__},

static PyObject *
termios_tcflush_impl(PyObject *module, int fd, int queue);

static PyObject *
termios_tcflush(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int queue;

    if (!_PyArg_CheckPositional("tcflush", nargs, 2, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    queue = _PyLong_AsInt(args[1]);
    if (queue == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcflush_impl(module, fd, queue);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcflow__doc__,
"tcflow($module, fd, action, /)\n"
"--\n"
"\n"
"Suspend or resume input or output on file descriptor fd.\n"
"\n"
"The action argument can be termios.TCOOFF to suspend output,\n"
"termios.TCOON to restart output, termios.TCIOFF to suspend input,\n"
"or termios.TCION to restart input.");

#define TERMIOS_TCFLOW_METHODDEF    \
    {"tcflow", _PyCFunction_CAST(termios_tcflow), METH_FASTCALL, termios_tcflow__doc__},

static PyObject *
termios_tcflow_impl(PyObject *module, int fd, int action);

static PyObject *
termios_tcflow(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    int action;

    if (!_PyArg_CheckPositional("tcflow", nargs, 2, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    action = _PyLong_AsInt(args[1]);
    if (action == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = termios_tcflow_impl(module, fd, action);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcgetwinsize__doc__,
"tcgetwinsize($module, fd, /)\n"
"--\n"
"\n"
"Get the tty winsize for file descriptor fd.\n"
"\n"
"Returns a tuple (ws_row, ws_col).");

#define TERMIOS_TCGETWINSIZE_METHODDEF    \
    {"tcgetwinsize", (PyCFunction)termios_tcgetwinsize, METH_O, termios_tcgetwinsize__doc__},

static PyObject *
termios_tcgetwinsize_impl(PyObject *module, int fd);

static PyObject *
termios_tcgetwinsize(PyObject *module, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!_PyLong_FileDescriptor_Converter(arg, &fd)) {
        goto exit;
    }
    return_value = termios_tcgetwinsize_impl(module, fd);

exit:
    return return_value;
}

PyDoc_STRVAR(termios_tcsetwinsize__doc__,
"tcsetwinsize($module, fd, winsize, /)\n"
"--\n"
"\n"
"Set the tty winsize for file descriptor fd.\n"
"\n"
"The winsize to be set is taken from the winsize argument, which\n"
"is a two-item tuple (ws_row, ws_col) like the one returned by tcgetwinsize().");

#define TERMIOS_TCSETWINSIZE_METHODDEF    \
    {"tcsetwinsize", _PyCFunction_CAST(termios_tcsetwinsize), METH_FASTCALL, termios_tcsetwinsize__doc__},

static PyObject *
termios_tcsetwinsize_impl(PyObject *module, int fd, PyObject *winsz);

static PyObject *
termios_tcsetwinsize(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    PyObject *winsz;

    if (!_PyArg_CheckPositional("tcsetwinsize", nargs, 2, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    winsz = args[1];
    return_value = termios_tcsetwinsize_impl(module, fd, winsz);

exit:
    return return_value;
}
/*[clinic end generated code: output=d286a3906a051869 input=a9049054013a1b77]*/
