/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(select_select__doc__,
"select($module, rlist, wlist, xlist, timeout=None, /)\n"
"--\n"
"\n"
"Wait until one or more file descriptors are ready for some kind of I/O.\n"
"\n"
"The first three arguments are iterables of file descriptors to be waited for:\n"
"rlist -- wait until ready for reading\n"
"wlist -- wait until ready for writing\n"
"xlist -- wait for an \"exceptional condition\"\n"
"If only one kind of condition is required, pass [] for the other lists.\n"
"\n"
"A file descriptor is either a socket or file object, or a small integer\n"
"gotten from a fileno() method call on one of those.\n"
"\n"
"The optional 4th argument specifies a timeout in seconds; it may be\n"
"a floating point number to specify fractions of seconds.  If it is absent\n"
"or None, the call will never time out.\n"
"\n"
"The return value is a tuple of three lists corresponding to the first three\n"
"arguments; each contains the subset of the corresponding file descriptors\n"
"that are ready.\n"
"\n"
"*** IMPORTANT NOTICE ***\n"
"On Windows, only sockets are supported; on Unix, all file\n"
"descriptors can be used.");

#define SELECT_SELECT_METHODDEF    \
    {"select", _PyCFunction_CAST(select_select), METH_FASTCALL, select_select__doc__},

static PyObject *
select_select_impl(PyObject *module, PyObject *rlist, PyObject *wlist,
                   PyObject *xlist, PyObject *timeout_obj);

static PyObject *
select_select(PyObject *module, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *rlist;
    PyObject *wlist;
    PyObject *xlist;
    PyObject *timeout_obj = Py_None;

    if (!_PyArg_CheckPositional("select", nargs, 3, 4)) {
        goto exit;
    }
    rlist = args[0];
    wlist = args[1];
    xlist = args[2];
    if (nargs < 4) {
        goto skip_optional;
    }
    timeout_obj = args[3];
skip_optional:
    return_value = select_select_impl(module, rlist, wlist, xlist, timeout_obj);

exit:
    return return_value;
}

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_register__doc__,
"register($self, fd,\n"
"         eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Register a file descriptor with the polling object.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_POLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_poll_register), METH_FASTCALL, select_poll_register__doc__},

static PyObject *
select_poll_register_impl(pollObject *self, int fd, unsigned short eventmask);

static PyObject *
select_poll_register(pollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("register", nargs, 1, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedShort_Converter(args[1], &eventmask)) {
        goto exit;
    }
skip_optional:
    return_value = select_poll_register_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_modify__doc__,
"modify($self, fd, eventmask, /)\n"
"--\n"
"\n"
"Modify an already registered file descriptor.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    a bitmask describing the type of events to check for");

#define SELECT_POLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_poll_modify), METH_FASTCALL, select_poll_modify__doc__},

static PyObject *
select_poll_modify_impl(pollObject *self, int fd, unsigned short eventmask);

static PyObject *
select_poll_modify(pollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    unsigned short eventmask;

    if (!_PyArg_CheckPositional("modify", nargs, 2, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    if (!_PyLong_UnsignedShort_Converter(args[1], &eventmask)) {
        goto exit;
    }
    return_value = select_poll_modify_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_unregister__doc__,
"unregister($self, fd, /)\n"
"--\n"
"\n"
"Remove a file descriptor being tracked by the polling object.");

#define SELECT_POLL_UNREGISTER_METHODDEF    \
    {"unregister", (PyCFunction)select_poll_unregister, METH_O, select_poll_unregister__doc__},

static PyObject *
select_poll_unregister_impl(pollObject *self, int fd);

static PyObject *
select_poll_unregister(pollObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!_PyLong_FileDescriptor_Converter(arg, &fd)) {
        goto exit;
    }
    return_value = select_poll_unregister_impl(self, fd);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_poll__doc__,
"poll($self, timeout=None, /)\n"
"--\n"
"\n"
"Polls the set of registered file descriptors.\n"
"\n"
"  timeout\n"
"    The maximum time to wait in milliseconds, or else None (or a negative\n"
"    value) to wait indefinitely.\n"
"\n"
"Returns a list containing any descriptors that have events or errors to\n"
"report, as a list of (fd, event) 2-tuples.");

#define SELECT_POLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_poll_poll), METH_FASTCALL, select_poll_poll__doc__},

static PyObject *
select_poll_poll_impl(pollObject *self, PyObject *timeout_obj);

static PyObject *
select_poll_poll(pollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *timeout_obj = Py_None;

    if (!_PyArg_CheckPositional("poll", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = args[0];
skip_optional:
    return_value = select_poll_poll_impl(self, timeout_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_register__doc__,
"register($self, fd,\n"
"         eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Register a file descriptor with the polling object.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_DEVPOLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_devpoll_register), METH_FASTCALL, select_devpoll_register__doc__},

static PyObject *
select_devpoll_register_impl(devpollObject *self, int fd,
                             unsigned short eventmask);

static PyObject *
select_devpoll_register(devpollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("register", nargs, 1, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedShort_Converter(args[1], &eventmask)) {
        goto exit;
    }
skip_optional:
    return_value = select_devpoll_register_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_modify__doc__,
"modify($self, fd,\n"
"       eventmask=select.POLLIN | select.POLLPRI | select.POLLOUT, /)\n"
"--\n"
"\n"
"Modify a possible already registered file descriptor.\n"
"\n"
"  fd\n"
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_DEVPOLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_devpoll_modify), METH_FASTCALL, select_devpoll_modify__doc__},

static PyObject *
select_devpoll_modify_impl(devpollObject *self, int fd,
                           unsigned short eventmask);

static PyObject *
select_devpoll_modify(devpollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    int fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!_PyArg_CheckPositional("modify", nargs, 1, 2)) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    if (nargs < 2) {
        goto skip_optional;
    }
    if (!_PyLong_UnsignedShort_Converter(args[1], &eventmask)) {
        goto exit;
    }
skip_optional:
    return_value = select_devpoll_modify_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_unregister__doc__,
"unregister($self, fd, /)\n"
"--\n"
"\n"
"Remove a file descriptor being tracked by the polling object.");

#define SELECT_DEVPOLL_UNREGISTER_METHODDEF    \
    {"unregister", (PyCFunction)select_devpoll_unregister, METH_O, select_devpoll_unregister__doc__},

static PyObject *
select_devpoll_unregister_impl(devpollObject *self, int fd);

static PyObject *
select_devpoll_unregister(devpollObject *self, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!_PyLong_FileDescriptor_Converter(arg, &fd)) {
        goto exit;
    }
    return_value = select_devpoll_unregister_impl(self, fd);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_poll__doc__,
"poll($self, timeout=None, /)\n"
"--\n"
"\n"
"Polls the set of registered file descriptors.\n"
"\n"
"  timeout\n"
"    The maximum time to wait in milliseconds, or else None (or a negative\n"
"    value) to wait indefinitely.\n"
"\n"
"Returns a list containing any descriptors that have events or errors to\n"
"report, as a list of (fd, event) 2-tuples.");

#define SELECT_DEVPOLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_devpoll_poll), METH_FASTCALL, select_devpoll_poll__doc__},

static PyObject *
select_devpoll_poll_impl(devpollObject *self, PyObject *timeout_obj);

static PyObject *
select_devpoll_poll(devpollObject *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *timeout_obj = Py_None;

    if (!_PyArg_CheckPositional("poll", nargs, 0, 1)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    timeout_obj = args[0];
skip_optional:
    return_value = select_devpoll_poll_impl(self, timeout_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the devpoll file descriptor.\n"
"\n"
"Further operations on the devpoll object will raise an exception.");

#define SELECT_DEVPOLL_CLOSE_METHODDEF    \
    {"close", (PyCFunction)select_devpoll_close, METH_NOARGS, select_devpoll_close__doc__},

static PyObject *
select_devpoll_close_impl(devpollObject *self);

static PyObject *
select_devpoll_close(devpollObject *self, PyObject *Py_UNUSED(ignored))
{
    return select_devpoll_close_impl(self);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the file descriptor.");

#define SELECT_DEVPOLL_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)select_devpoll_fileno, METH_NOARGS, select_devpoll_fileno__doc__},

static PyObject *
select_devpoll_fileno_impl(devpollObject *self);

static PyObject *
select_devpoll_fileno(devpollObject *self, PyObject *Py_UNUSED(ignored))
{
    return select_devpoll_fileno_impl(self);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll__doc__,
"poll($module, /)\n"
"--\n"
"\n"
"Returns a polling object.\n"
"\n"
"This object supports registering and unregistering file descriptors, and then\n"
"polling them for I/O events.");

#define SELECT_POLL_METHODDEF    \
    {"poll", (PyCFunction)select_poll, METH_NOARGS, select_poll__doc__},

static PyObject *
select_poll_impl(PyObject *module);

static PyObject *
select_poll(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return select_poll_impl(module);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll__doc__,
"devpoll($module, /)\n"
"--\n"
"\n"
"Returns a polling object.\n"
"\n"
"This object supports registering and unregistering file descriptors, and then\n"
"polling them for I/O events.");

#define SELECT_DEVPOLL_METHODDEF    \
    {"devpoll", (PyCFunction)select_devpoll, METH_NOARGS, select_devpoll__doc__},

static PyObject *
select_devpoll_impl(PyObject *module);

static PyObject *
select_devpoll(PyObject *module, PyObject *Py_UNUSED(ignored))
{
    return select_devpoll_impl(module);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll__doc__,
"epoll(sizehint=-1, flags=0)\n"
"--\n"
"\n"
"Returns an epolling object.\n"
"\n"
"  sizehint\n"
"    The expected number of events to be registered.  It must be positive,\n"
"    or -1 to use the default.  It is only used on older systems where\n"
"    epoll_create1() is not available; otherwise it has no effect (though its\n"
"    value is still checked).\n"
"  flags\n"
"    Deprecated and completely ignored.  However, when supplied, its value\n"
"    must be 0 or select.EPOLL_CLOEXEC, otherwise OSError is raised.");

static PyObject *
select_epoll_impl(PyTypeObject *type, int sizehint, int flags);

static PyObject *
select_epoll(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"sizehint", "flags", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "epoll", 0};
    PyObject *argsbuf[2];
    PyObject * const *fastargs;
    Py_ssize_t nargs = PyTuple_GET_SIZE(args);
    Py_ssize_t noptargs = nargs + (kwargs ? PyDict_GET_SIZE(kwargs) : 0) - 0;
    int sizehint = -1;
    int flags = 0;

    fastargs = _PyArg_UnpackKeywords(_PyTuple_CAST(args)->ob_item, nargs, kwargs, NULL, &_parser, 0, 2, 0, argsbuf);
    if (!fastargs) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (fastargs[0]) {
        sizehint = _PyLong_AsInt(fastargs[0]);
        if (sizehint == -1 && PyErr_Occurred()) {
            goto exit;
        }
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    flags = _PyLong_AsInt(fastargs[1]);
    if (flags == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = select_epoll_impl(type, sizehint, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the epoll control file descriptor.\n"
"\n"
"Further operations on the epoll object will raise an exception.");

#define SELECT_EPOLL_CLOSE_METHODDEF    \
    {"close", (PyCFunction)select_epoll_close, METH_NOARGS, select_epoll_close__doc__},

static PyObject *
select_epoll_close_impl(pyEpoll_Object *self);

static PyObject *
select_epoll_close(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return select_epoll_close_impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the epoll control file descriptor.");

#define SELECT_EPOLL_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)select_epoll_fileno, METH_NOARGS, select_epoll_fileno__doc__},

static PyObject *
select_epoll_fileno_impl(pyEpoll_Object *self);

static PyObject *
select_epoll_fileno(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return select_epoll_fileno_impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_fromfd__doc__,
"fromfd($type, fd, /)\n"
"--\n"
"\n"
"Create an epoll object from a given control fd.");

#define SELECT_EPOLL_FROMFD_METHODDEF    \
    {"fromfd", (PyCFunction)select_epoll_fromfd, METH_O|METH_CLASS, select_epoll_fromfd__doc__},

static PyObject *
select_epoll_fromfd_impl(PyTypeObject *type, int fd);

static PyObject *
select_epoll_fromfd(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = select_epoll_fromfd_impl(type, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_register__doc__,
"register($self, /, fd,\n"
"         eventmask=select.EPOLLIN | select.EPOLLPRI | select.EPOLLOUT)\n"
"--\n"
"\n"
"Registers a new fd or raises an OSError if the fd is already registered.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation\n"
"  eventmask\n"
"    a bit set composed of the various EPOLL constants\n"
"\n"
"The epoll interface supports all file descriptors that support poll.");

#define SELECT_EPOLL_REGISTER_METHODDEF    \
    {"register", _PyCFunction_CAST(select_epoll_register), METH_FASTCALL|METH_KEYWORDS, select_epoll_register__doc__},

static PyObject *
select_epoll_register_impl(pyEpoll_Object *self, int fd,
                           unsigned int eventmask);

static PyObject *
select_epoll_register(pyEpoll_Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "eventmask", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "register", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 1;
    int fd;
    unsigned int eventmask = EPOLLIN | EPOLLPRI | EPOLLOUT;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    eventmask = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (eventmask == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = select_epoll_register_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_modify__doc__,
"modify($self, /, fd, eventmask)\n"
"--\n"
"\n"
"Modify event mask for a registered file descriptor.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation\n"
"  eventmask\n"
"    a bit set composed of the various EPOLL constants");

#define SELECT_EPOLL_MODIFY_METHODDEF    \
    {"modify", _PyCFunction_CAST(select_epoll_modify), METH_FASTCALL|METH_KEYWORDS, select_epoll_modify__doc__},

static PyObject *
select_epoll_modify_impl(pyEpoll_Object *self, int fd,
                         unsigned int eventmask);

static PyObject *
select_epoll_modify(pyEpoll_Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", "eventmask", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "modify", 0};
    PyObject *argsbuf[2];
    int fd;
    unsigned int eventmask;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 2, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    eventmask = (unsigned int)PyLong_AsUnsignedLongMask(args[1]);
    if (eventmask == (unsigned int)-1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = select_epoll_modify_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_unregister__doc__,
"unregister($self, /, fd)\n"
"--\n"
"\n"
"Remove a registered file descriptor from the epoll object.\n"
"\n"
"  fd\n"
"    the target file descriptor of the operation");

#define SELECT_EPOLL_UNREGISTER_METHODDEF    \
    {"unregister", _PyCFunction_CAST(select_epoll_unregister), METH_FASTCALL|METH_KEYWORDS, select_epoll_unregister__doc__},

static PyObject *
select_epoll_unregister_impl(pyEpoll_Object *self, int fd);

static PyObject *
select_epoll_unregister(pyEpoll_Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"fd", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "unregister", 0};
    PyObject *argsbuf[1];
    int fd;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 1, 1, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!_PyLong_FileDescriptor_Converter(args[0], &fd)) {
        goto exit;
    }
    return_value = select_epoll_unregister_impl(self, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll_poll__doc__,
"poll($self, /, timeout=None, maxevents=-1)\n"
"--\n"
"\n"
"Wait for events on the epoll file descriptor.\n"
"\n"
"  timeout\n"
"    the maximum time to wait in seconds (as float);\n"
"    a timeout of None or -1 makes poll wait indefinitely\n"
"  maxevents\n"
"    the maximum number of events returned; -1 means no limit\n"
"\n"
"Returns a list containing any descriptors that have events to report,\n"
"as a list of (fd, events) 2-tuples.");

#define SELECT_EPOLL_POLL_METHODDEF    \
    {"poll", _PyCFunction_CAST(select_epoll_poll), METH_FASTCALL|METH_KEYWORDS, select_epoll_poll__doc__},

static PyObject *
select_epoll_poll_impl(pyEpoll_Object *self, PyObject *timeout_obj,
                       int maxevents);

static PyObject *
select_epoll_poll(pyEpoll_Object *self, PyObject *const *args, Py_ssize_t nargs, PyObject *kwnames)
{
    PyObject *return_value = NULL;
    static const char * const _keywords[] = {"timeout", "maxevents", NULL};
    static _PyArg_Parser _parser = {NULL, _keywords, "poll", 0};
    PyObject *argsbuf[2];
    Py_ssize_t noptargs = nargs + (kwnames ? PyTuple_GET_SIZE(kwnames) : 0) - 0;
    PyObject *timeout_obj = Py_None;
    int maxevents = -1;

    args = _PyArg_UnpackKeywords(args, nargs, NULL, kwnames, &_parser, 0, 2, 0, argsbuf);
    if (!args) {
        goto exit;
    }
    if (!noptargs) {
        goto skip_optional_pos;
    }
    if (args[0]) {
        timeout_obj = args[0];
        if (!--noptargs) {
            goto skip_optional_pos;
        }
    }
    maxevents = _PyLong_AsInt(args[1]);
    if (maxevents == -1 && PyErr_Occurred()) {
        goto exit;
    }
skip_optional_pos:
    return_value = select_epoll_poll_impl(self, timeout_obj, maxevents);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll___enter____doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define SELECT_EPOLL___ENTER___METHODDEF    \
    {"__enter__", (PyCFunction)select_epoll___enter__, METH_NOARGS, select_epoll___enter____doc__},

static PyObject *
select_epoll___enter___impl(pyEpoll_Object *self);

static PyObject *
select_epoll___enter__(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return select_epoll___enter___impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(select_epoll___exit____doc__,
"__exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)\n"
"--\n"
"\n");

#define SELECT_EPOLL___EXIT___METHODDEF    \
    {"__exit__", _PyCFunction_CAST(select_epoll___exit__), METH_FASTCALL, select_epoll___exit____doc__},

static PyObject *
select_epoll___exit___impl(pyEpoll_Object *self, PyObject *exc_type,
                           PyObject *exc_value, PyObject *exc_tb);

static PyObject *
select_epoll___exit__(pyEpoll_Object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *exc_type = Py_None;
    PyObject *exc_value = Py_None;
    PyObject *exc_tb = Py_None;

    if (!_PyArg_CheckPositional("__exit__", nargs, 0, 3)) {
        goto exit;
    }
    if (nargs < 1) {
        goto skip_optional;
    }
    exc_type = args[0];
    if (nargs < 2) {
        goto skip_optional;
    }
    exc_value = args[1];
    if (nargs < 3) {
        goto skip_optional;
    }
    exc_tb = args[2];
skip_optional:
    return_value = select_epoll___exit___impl(self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue__doc__,
"kqueue()\n"
"--\n"
"\n"
"Kqueue syscall wrapper.\n"
"\n"
"For example, to start watching a socket for input:\n"
">>> kq = kqueue()\n"
">>> sock = socket()\n"
">>> sock.connect((host, port))\n"
">>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_ADD)], 0)\n"
"\n"
"To wait one second for it to become writeable:\n"
">>> kq.control(None, 1, 1000)\n"
"\n"
"To stop listening:\n"
">>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_DELETE)], 0)");

static PyObject *
select_kqueue_impl(PyTypeObject *type);

static PyObject *
select_kqueue(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;

    if ((type == _selectstate_by_type(type)->kqueue_queue_Type ||
         type->tp_init == _selectstate_by_type(type)->kqueue_queue_Type->tp_init) &&
        !_PyArg_NoPositional("kqueue", args)) {
        goto exit;
    }
    if ((type == _selectstate_by_type(type)->kqueue_queue_Type ||
         type->tp_init == _selectstate_by_type(type)->kqueue_queue_Type->tp_init) &&
        !_PyArg_NoKeywords("kqueue", kwargs)) {
        goto exit;
    }
    return_value = select_kqueue_impl(type);

exit:
    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the kqueue control file descriptor.\n"
"\n"
"Further operations on the kqueue object will raise an exception.");

#define SELECT_KQUEUE_CLOSE_METHODDEF    \
    {"close", (PyCFunction)select_kqueue_close, METH_NOARGS, select_kqueue_close__doc__},

static PyObject *
select_kqueue_close_impl(kqueue_queue_Object *self);

static PyObject *
select_kqueue_close(kqueue_queue_Object *self, PyObject *Py_UNUSED(ignored))
{
    return select_kqueue_close_impl(self);
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the kqueue control file descriptor.");

#define SELECT_KQUEUE_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)select_kqueue_fileno, METH_NOARGS, select_kqueue_fileno__doc__},

static PyObject *
select_kqueue_fileno_impl(kqueue_queue_Object *self);

static PyObject *
select_kqueue_fileno(kqueue_queue_Object *self, PyObject *Py_UNUSED(ignored))
{
    return select_kqueue_fileno_impl(self);
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_fromfd__doc__,
"fromfd($type, fd, /)\n"
"--\n"
"\n"
"Create a kqueue object from a given control fd.");

#define SELECT_KQUEUE_FROMFD_METHODDEF    \
    {"fromfd", (PyCFunction)select_kqueue_fromfd, METH_O|METH_CLASS, select_kqueue_fromfd__doc__},

static PyObject *
select_kqueue_fromfd_impl(PyTypeObject *type, int fd);

static PyObject *
select_kqueue_fromfd(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    fd = _PyLong_AsInt(arg);
    if (fd == -1 && PyErr_Occurred()) {
        goto exit;
    }
    return_value = select_kqueue_fromfd_impl(type, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#if defined(HAVE_KQUEUE)

PyDoc_STRVAR(select_kqueue_control__doc__,
"control($self, changelist, maxevents, timeout=None, /)\n"
"--\n"
"\n"
"Calls the kernel kevent function.\n"
"\n"
"  changelist\n"
"    Must be an iterable of kevent objects describing the changes to be made\n"
"    to the kernel\'s watch list or None.\n"
"  maxevents\n"
"    The maximum number of events that the kernel will return.\n"
"  timeout\n"
"    The maximum time to wait in seconds, or else None to wait forever.\n"
"    This accepts floats for smaller timeouts, too.");

#define SELECT_KQUEUE_CONTROL_METHODDEF    \
    {"control", _PyCFunction_CAST(select_kqueue_control), METH_FASTCALL, select_kqueue_control__doc__},

static PyObject *
select_kqueue_control_impl(kqueue_queue_Object *self, PyObject *changelist,
                           int maxevents, PyObject *otimeout);

static PyObject *
select_kqueue_control(kqueue_queue_Object *self, PyObject *const *args, Py_ssize_t nargs)
{
    PyObject *return_value = NULL;
    PyObject *changelist;
    int maxevents;
    PyObject *otimeout = Py_None;

    if (!_PyArg_CheckPositional("control", nargs, 2, 3)) {
        goto exit;
    }
    changelist = args[0];
    maxevents = _PyLong_AsInt(args[1]);
    if (maxevents == -1 && PyErr_Occurred()) {
        goto exit;
    }
    if (nargs < 3) {
        goto skip_optional;
    }
    otimeout = args[2];
skip_optional:
    return_value = select_kqueue_control_impl(self, changelist, maxevents, otimeout);

exit:
    return return_value;
}

#endif /* defined(HAVE_KQUEUE) */

#ifndef SELECT_POLL_REGISTER_METHODDEF
    #define SELECT_POLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_POLL_REGISTER_METHODDEF) */

#ifndef SELECT_POLL_MODIFY_METHODDEF
    #define SELECT_POLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_POLL_MODIFY_METHODDEF) */

#ifndef SELECT_POLL_UNREGISTER_METHODDEF
    #define SELECT_POLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_POLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_POLL_POLL_METHODDEF
    #define SELECT_POLL_POLL_METHODDEF
#endif /* !defined(SELECT_POLL_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_REGISTER_METHODDEF
    #define SELECT_DEVPOLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_REGISTER_METHODDEF) */

#ifndef SELECT_DEVPOLL_MODIFY_METHODDEF
    #define SELECT_DEVPOLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_MODIFY_METHODDEF) */

#ifndef SELECT_DEVPOLL_UNREGISTER_METHODDEF
    #define SELECT_DEVPOLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_DEVPOLL_POLL_METHODDEF
    #define SELECT_DEVPOLL_POLL_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_CLOSE_METHODDEF
    #define SELECT_DEVPOLL_CLOSE_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_CLOSE_METHODDEF) */

#ifndef SELECT_DEVPOLL_FILENO_METHODDEF
    #define SELECT_DEVPOLL_FILENO_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_FILENO_METHODDEF) */

#ifndef SELECT_POLL_METHODDEF
    #define SELECT_POLL_METHODDEF
#endif /* !defined(SELECT_POLL_METHODDEF) */

#ifndef SELECT_DEVPOLL_METHODDEF
    #define SELECT_DEVPOLL_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_METHODDEF) */

#ifndef SELECT_EPOLL_CLOSE_METHODDEF
    #define SELECT_EPOLL_CLOSE_METHODDEF
#endif /* !defined(SELECT_EPOLL_CLOSE_METHODDEF) */

#ifndef SELECT_EPOLL_FILENO_METHODDEF
    #define SELECT_EPOLL_FILENO_METHODDEF
#endif /* !defined(SELECT_EPOLL_FILENO_METHODDEF) */

#ifndef SELECT_EPOLL_FROMFD_METHODDEF
    #define SELECT_EPOLL_FROMFD_METHODDEF
#endif /* !defined(SELECT_EPOLL_FROMFD_METHODDEF) */

#ifndef SELECT_EPOLL_REGISTER_METHODDEF
    #define SELECT_EPOLL_REGISTER_METHODDEF
#endif /* !defined(SELECT_EPOLL_REGISTER_METHODDEF) */

#ifndef SELECT_EPOLL_MODIFY_METHODDEF
    #define SELECT_EPOLL_MODIFY_METHODDEF
#endif /* !defined(SELECT_EPOLL_MODIFY_METHODDEF) */

#ifndef SELECT_EPOLL_UNREGISTER_METHODDEF
    #define SELECT_EPOLL_UNREGISTER_METHODDEF
#endif /* !defined(SELECT_EPOLL_UNREGISTER_METHODDEF) */

#ifndef SELECT_EPOLL_POLL_METHODDEF
    #define SELECT_EPOLL_POLL_METHODDEF
#endif /* !defined(SELECT_EPOLL_POLL_METHODDEF) */

#ifndef SELECT_EPOLL___ENTER___METHODDEF
    #define SELECT_EPOLL___ENTER___METHODDEF
#endif /* !defined(SELECT_EPOLL___ENTER___METHODDEF) */

#ifndef SELECT_EPOLL___EXIT___METHODDEF
    #define SELECT_EPOLL___EXIT___METHODDEF
#endif /* !defined(SELECT_EPOLL___EXIT___METHODDEF) */

#ifndef SELECT_KQUEUE_CLOSE_METHODDEF
    #define SELECT_KQUEUE_CLOSE_METHODDEF
#endif /* !defined(SELECT_KQUEUE_CLOSE_METHODDEF) */

#ifndef SELECT_KQUEUE_FILENO_METHODDEF
    #define SELECT_KQUEUE_FILENO_METHODDEF
#endif /* !defined(SELECT_KQUEUE_FILENO_METHODDEF) */

#ifndef SELECT_KQUEUE_FROMFD_METHODDEF
    #define SELECT_KQUEUE_FROMFD_METHODDEF
#endif /* !defined(SELECT_KQUEUE_FROMFD_METHODDEF) */

#ifndef SELECT_KQUEUE_CONTROL_METHODDEF
    #define SELECT_KQUEUE_CONTROL_METHODDEF
#endif /* !defined(SELECT_KQUEUE_CONTROL_METHODDEF) */
/*[clinic end generated code: output=e77cc5c8a6c77860 input=a9049054013a1b77]*/
