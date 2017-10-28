/*[clinic input]
preserve
[clinic start generated code]*/

PyDoc_STRVAR(select_select__doc__,
"select($module, rlist, wlist, xlist, timeout=None, /)\n"
"--\n"
"\n"
"Wait until one or more file descriptors are ready for some kind of I/O.\n"
"\n"
"The first three arguments are sequences of file descriptors to be waited for:\n"
"  rlist -- wait until ready for reading\n"
"  wlist -- wait until ready for writing\n"
"  xlist -- wait for an ``exceptional condition\'\'\n"
"If only one kind of condition is required, pass [] for the other lists.\n"
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
"On Windows only sockets are supported; on Unix, all file descriptors\n"
"can be used.");

#define SELECT_SELECT_METHODDEF    \
    {"select", (PyCFunction)select_select, METH_VARARGS, select_select__doc__},

static PyObject *
select_select_impl(PyModuleDef *module, PyObject *rlist, PyObject *wlist,
                   PyObject *xlist, PyObject *timeout_obj);

static PyObject *
select_select(PyModuleDef *module, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *rlist;
    PyObject *wlist;
    PyObject *xlist;
    PyObject *timeout_obj = Py_None;

    if (!PyArg_UnpackTuple(args, "select",
        3, 4,
        &rlist, &wlist, &xlist, &timeout_obj))
        goto exit;
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
"    either an integer, or an object with a fileno() method returning\n"
"    an int\n"
"  eventmask\n"
"    an optional bitmask describing the type of events to check for");

#define SELECT_POLL_REGISTER_METHODDEF    \
    {"register", (PyCFunction)select_poll_register, METH_VARARGS, select_poll_register__doc__},

static PyObject *
select_poll_register_impl(pollObject *self, PyObject *fd,
                          unsigned short eventmask);

static PyObject *
select_poll_register(pollObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *fd;
    unsigned short eventmask = POLLIN | POLLPRI | POLLOUT;

    if (!PyArg_ParseTuple(args, "O|O&:register",
        &fd, ushort_converter, &eventmask))
        goto exit;
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
"    an optional bitmask describing the type of events to check for");

#define SELECT_POLL_MODIFY_METHODDEF    \
    {"modify", (PyCFunction)select_poll_modify, METH_VARARGS, select_poll_modify__doc__},

static PyObject *
select_poll_modify_impl(pollObject *self, PyObject *fd,
                        unsigned short eventmask);

static PyObject *
select_poll_modify(pollObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *fd;
    unsigned short eventmask;

    if (!PyArg_ParseTuple(args, "OO&:modify",
        &fd, ushort_converter, &eventmask))
        goto exit;
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

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll_poll__doc__,
"poll($self, timeout=None, /)\n"
"--\n"
"\n"
"Polls the set of registered file descriptors.\n"
"\n"
"Return value is a list containing any descriptors that have events\n"
"or errors to report, in the form of tuples (fd, event).");

#define SELECT_POLL_POLL_METHODDEF    \
    {"poll", (PyCFunction)select_poll_poll, METH_VARARGS, select_poll_poll__doc__},

static PyObject *
select_poll_poll_impl(pollObject *self, PyObject *timeout_obj);

static PyObject *
select_poll_poll(pollObject *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *timeout_obj = Py_None;

    if (!PyArg_UnpackTuple(args, "poll",
        0, 1,
        &timeout_obj))
        goto exit;
    return_value = select_poll_poll_impl(self, timeout_obj);

exit:
    return return_value;
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL))

PyDoc_STRVAR(select_poll__doc__,
"poll($module, /)\n"
"--\n"
"\n"
"Returns a polling object.\n"
"\n"
"The object supports registering and unregistering file descriptors,\n"
"and then polling them for I/O events.");

#define SELECT_POLL_METHODDEF    \
    {"poll", (PyCFunction)select_poll, METH_NOARGS, select_poll__doc__},

static PyObject *
select_poll_impl(PyModuleDef *module);

static PyObject *
select_poll(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return select_poll_impl(module);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) */

#if (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H)

PyDoc_STRVAR(select_devpoll__doc__,
"devpoll($module, /)\n"
"--\n"
"\n"
"Returns a polling object using /dev/poll.\n"
"\n"
"The object supports registering and unregistering file descriptors,\n"
"and then polling them for I/O events.");

#define SELECT_DEVPOLL_METHODDEF    \
    {"devpoll", (PyCFunction)select_devpoll, METH_NOARGS, select_devpoll__doc__},

static PyObject *
select_devpoll_impl(PyModuleDef *module);

static PyObject *
select_devpoll(PyModuleDef *module, PyObject *Py_UNUSED(ignored))
{
    return select_devpoll_impl(module);
}

#endif /* (defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)) && defined(HAVE_SYS_DEVPOLL_H) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_new__doc__,
"epoll(sizehint=-1, flags=0)\n"
"--\n"
"\n"
"Returns an epolling object.\n"
"\n"
"sizehint must be a positive integer or -1 for the default size. The\n"
"sizehint is used to optimize internal data structures. It doesn\'t limit\n"
"the maximum number of monitored events.");

static PyObject *
pyepoll_new_impl(PyTypeObject *type, int sizehint, int flags);

static PyObject *
pyepoll_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"sizehint", "flags", NULL};
    int sizehint = FD_SETSIZE - 1;
    int flags = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|ii:epoll", _keywords,
        &sizehint, &flags))
        goto exit;
    return_value = pyepoll_new_impl(type, sizehint, flags);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_close__doc__,
"close($self, /)\n"
"--\n"
"\n"
"Close the epoll control file descriptor.\n"
"\n"
"Further operations on the epoll object will raise an exception.");

#define PYEPOLL_CLOSE_METHODDEF    \
    {"close", (PyCFunction)pyepoll_close, METH_NOARGS, pyepoll_close__doc__},

static PyObject *
pyepoll_close_impl(pyEpoll_Object *self);

static PyObject *
pyepoll_close(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return pyepoll_close_impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_fileno__doc__,
"fileno($self, /)\n"
"--\n"
"\n"
"Return the epoll control file descriptor.");

#define PYEPOLL_FILENO_METHODDEF    \
    {"fileno", (PyCFunction)pyepoll_fileno, METH_NOARGS, pyepoll_fileno__doc__},

static PyObject *
pyepoll_fileno_impl(pyEpoll_Object *self);

static PyObject *
pyepoll_fileno(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return pyepoll_fileno_impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_fromfd__doc__,
"fromfd($type, fd, /)\n"
"--\n"
"\n"
"Create an epoll object from a given control fd.");

#define PYEPOLL_FROMFD_METHODDEF    \
    {"fromfd", (PyCFunction)pyepoll_fromfd, METH_O|METH_CLASS, pyepoll_fromfd__doc__},

static PyObject *
pyepoll_fromfd_impl(PyTypeObject *type, int fd);

static PyObject *
pyepoll_fromfd(PyTypeObject *type, PyObject *arg)
{
    PyObject *return_value = NULL;
    int fd;

    if (!PyArg_Parse(arg, "i:fromfd", &fd))
        goto exit;
    return_value = pyepoll_fromfd_impl(type, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_register__doc__,
"register($self, /, fd,\n"
"         eventmask=select.EPOLLIN | select.EPOLLOUT | select.EPOLLPRI)\n"
"--\n"
"\n"
"Registers a new fd or raises an OSError if the fd is already registered.\n"
"\n"
"fd is the target file descriptor of the operation.\n"
"events is a bit set composed of the various EPOLL constants; the default\n"
"is EPOLLIN | EPOLLOUT | EPOLLPRI.\n"
"\n"
"The epoll interface supports all file descriptors that support poll.");

#define PYEPOLL_REGISTER_METHODDEF    \
    {"register", (PyCFunction)pyepoll_register, METH_VARARGS|METH_KEYWORDS, pyepoll_register__doc__},

static PyObject *
pyepoll_register_impl(pyEpoll_Object *self, PyObject *fd,
                      unsigned int eventmask);

static PyObject *
pyepoll_register(pyEpoll_Object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", "eventmask", NULL};
    PyObject *fd;
    unsigned int eventmask = EPOLLIN | EPOLLOUT | EPOLLPRI;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|I:register", _keywords,
        &fd, &eventmask))
        goto exit;
    return_value = pyepoll_register_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_modify__doc__,
"modify($self, /, fd, eventmask)\n"
"--\n"
"\n"
"Modify event mask for a registered file descriptor.\n"
"\n"
"fd is the target file descriptor of the operation, and\n"
"events is a bit set composed of the various EPOLL constants.");

#define PYEPOLL_MODIFY_METHODDEF    \
    {"modify", (PyCFunction)pyepoll_modify, METH_VARARGS|METH_KEYWORDS, pyepoll_modify__doc__},

static PyObject *
pyepoll_modify_impl(pyEpoll_Object *self, PyObject *fd,
                    unsigned int eventmask);

static PyObject *
pyepoll_modify(pyEpoll_Object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", "eventmask", NULL};
    PyObject *fd;
    unsigned int eventmask;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OI:modify", _keywords,
        &fd, &eventmask))
        goto exit;
    return_value = pyepoll_modify_impl(self, fd, eventmask);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_unregister__doc__,
"unregister($self, /, fd)\n"
"--\n"
"\n"
"Remove a registered file descriptor from the epoll object.\n"
"\n"
"fd is the target file descriptor of the operation.");

#define PYEPOLL_UNREGISTER_METHODDEF    \
    {"unregister", (PyCFunction)pyepoll_unregister, METH_VARARGS|METH_KEYWORDS, pyepoll_unregister__doc__},

static PyObject *
pyepoll_unregister_impl(pyEpoll_Object *self, PyObject *fd);

static PyObject *
pyepoll_unregister(pyEpoll_Object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"fd", NULL};
    PyObject *fd;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O:unregister", _keywords,
        &fd))
        goto exit;
    return_value = pyepoll_unregister_impl(self, fd);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_poll__doc__,
"poll($self, /, timeout=-1.0, maxevents=-1)\n"
"--\n"
"\n"
"Wait for events on the epoll file descriptor.\n"
"\n"
"timeout gives the maximum time to wait in seconds (as float).\n"
"A timeout of -1 makes poll wait indefinitely.\n"
"Up to maxevents are returned to the caller.\n"
"\n"
"The return value is a list of tuples of the form (fd, events).");

#define PYEPOLL_POLL_METHODDEF    \
    {"poll", (PyCFunction)pyepoll_poll, METH_VARARGS|METH_KEYWORDS, pyepoll_poll__doc__},

static PyObject *
pyepoll_poll_impl(pyEpoll_Object *self, PyObject *timeout_obj, int maxevents);

static PyObject *
pyepoll_poll(pyEpoll_Object *self, PyObject *args, PyObject *kwargs)
{
    PyObject *return_value = NULL;
    static char *_keywords[] = {"timeout", "maxevents", NULL};
    PyObject *timeout_obj = Py_None;
    int maxevents = -1;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|Oi:poll", _keywords,
        &timeout_obj, &maxevents))
        goto exit;
    return_value = pyepoll_poll_impl(self, timeout_obj, maxevents);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_enter__doc__,
"__enter__($self, /)\n"
"--\n"
"\n");

#define PYEPOLL_ENTER_METHODDEF    \
    {"__enter__", (PyCFunction)pyepoll_enter, METH_NOARGS, pyepoll_enter__doc__},

static PyObject *
pyepoll_enter_impl(pyEpoll_Object *self);

static PyObject *
pyepoll_enter(pyEpoll_Object *self, PyObject *Py_UNUSED(ignored))
{
    return pyepoll_enter_impl(self);
}

#endif /* defined(HAVE_EPOLL) */

#if defined(HAVE_EPOLL)

PyDoc_STRVAR(pyepoll_exit__doc__,
"__exit__($self, exc_type=None, exc_value=None, exc_tb=None, /)\n"
"--\n"
"\n");

#define PYEPOLL_EXIT_METHODDEF    \
    {"__exit__", (PyCFunction)pyepoll_exit, METH_VARARGS, pyepoll_exit__doc__},

static PyObject *
pyepoll_exit_impl(pyEpoll_Object *self, PyObject *exc_type,
                  PyObject *exc_value, PyObject *exc_tb);

static PyObject *
pyepoll_exit(pyEpoll_Object *self, PyObject *args)
{
    PyObject *return_value = NULL;
    PyObject *exc_type = Py_None;
    PyObject *exc_value = Py_None;
    PyObject *exc_tb = Py_None;

    if (!PyArg_UnpackTuple(args, "__exit__",
        0, 3,
        &exc_type, &exc_value, &exc_tb))
        goto exit;
    return_value = pyepoll_exit_impl(self, exc_type, exc_value, exc_tb);

exit:
    return return_value;
}

#endif /* defined(HAVE_EPOLL) */

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

#ifndef SELECT_DEVPOLL_METHODDEF
    #define SELECT_DEVPOLL_METHODDEF
#endif /* !defined(SELECT_DEVPOLL_METHODDEF) */

#ifndef PYEPOLL_CLOSE_METHODDEF
    #define PYEPOLL_CLOSE_METHODDEF
#endif /* !defined(PYEPOLL_CLOSE_METHODDEF) */

#ifndef PYEPOLL_FILENO_METHODDEF
    #define PYEPOLL_FILENO_METHODDEF
#endif /* !defined(PYEPOLL_FILENO_METHODDEF) */

#ifndef PYEPOLL_FROMFD_METHODDEF
    #define PYEPOLL_FROMFD_METHODDEF
#endif /* !defined(PYEPOLL_FROMFD_METHODDEF) */

#ifndef PYEPOLL_ENTER_METHODDEF
    #define PYEPOLL_ENTER_METHODDEF
#endif /* !defined(PYEPOLL_ENTER_METHODDEF) */

#ifndef PYEPOLL_EXIT_METHODDEF
    #define PYEPOLL_EXIT_METHODDEF
#endif /* !defined(PYEPOLL_EXIT_METHODDEF) */
/*[clinic end generated code: output=8c67fe108dc8d539 input=a9049054013a1b77]*/
