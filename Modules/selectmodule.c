/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
   Under BeOS, we suffer the same dichotomy as Win32; sockets can be anything
   >= 0.
*/

#include "Python.h"
#include <structmember.h>

#ifdef __APPLE__
    /* Perform runtime testing for a broken poll on OSX to make it easier
     * to use the same binary on multiple releases of the OS.
     */
#undef HAVE_BROKEN_POLL
#endif

/* Windows #defines FD_SETSIZE to 64 if FD_SETSIZE isn't already defined.
   64 is too small (too many people have bumped into that limit).
   Here we boost it.
   Users who want even more than the boosted limit should #define
   FD_SETSIZE higher before this; e.g., via compiler /D switch.
*/
#if defined(MS_WINDOWS) && !defined(FD_SETSIZE)
#define FD_SETSIZE 512
#endif

#if defined(HAVE_POLL_H)
#include <poll.h>
#elif defined(HAVE_SYS_POLL_H)
#include <sys/poll.h>
#endif

#ifdef __sgi
/* This is missing from unistd.h */
extern void bzero(void *, int);
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#if defined(PYOS_OS2) && !defined(PYCC_GCC)
#include <sys/time.h>
#include <utils.h>
#endif

#ifdef MS_WINDOWS
#  include <winsock2.h>
#else
#  define SOCKET int
#  ifdef __BEOS__
#    include <net/socket.h>
#  elif defined(__VMS)
#    include <socket.h>
#  endif
#endif

static PyObject *SelectError;

/* list of Python objects and their file descriptor */
typedef struct {
    PyObject *obj;                           /* owned reference */
    SOCKET fd;
    int sentinel;                            /* -1 == sentinel */
} pylist;

static void
reap_obj(pylist fd2obj[FD_SETSIZE + 1])
{
    int i;
    for (i = 0; i < FD_SETSIZE + 1 && fd2obj[i].sentinel >= 0; i++) {
        Py_CLEAR(fd2obj[i].obj);
    }
    fd2obj[0].sentinel = -1;
}


/* returns -1 and sets the Python exception if an error occurred, otherwise
   returns a number >= 0
*/
static int
seq2set(PyObject *seq, fd_set *set, pylist fd2obj[FD_SETSIZE + 1])
{
    int i;
    int max = -1;
    int index = 0;
    PyObject* fast_seq = NULL;
    PyObject* o = NULL;

    fd2obj[0].obj = (PyObject*)0;            /* set list to zero size */
    FD_ZERO(set);

    fast_seq = PySequence_Fast(seq, "arguments 1-3 must be sequences");
    if (!fast_seq)
        return -1;

    for (i = 0; i < PySequence_Fast_GET_SIZE(fast_seq); i++)  {
        SOCKET v;

        /* any intervening fileno() calls could decr this refcnt */
        if (!(o = PySequence_Fast_GET_ITEM(fast_seq, i)))
            return -1;

        Py_INCREF(o);
        v = PyObject_AsFileDescriptor( o );
        if (v == -1) goto finally;

#if defined(_MSC_VER)
        max = 0;                             /* not used for Win32 */
#else  /* !_MSC_VER */
        if (!_PyIsSelectable_fd(v)) {
            PyErr_SetString(PyExc_ValueError,
                        "filedescriptor out of range in select()");
            goto finally;
        }
        if (v > max)
            max = v;
#endif /* _MSC_VER */
        FD_SET(v, set);

        /* add object and its file descriptor to the list */
        if (index >= FD_SETSIZE) {
            PyErr_SetString(PyExc_ValueError,
                          "too many file descriptors in select()");
            goto finally;
        }
        fd2obj[index].obj = o;
        fd2obj[index].fd = v;
        fd2obj[index].sentinel = 0;
        fd2obj[++index].sentinel = -1;
    }
    Py_DECREF(fast_seq);
    return max+1;

  finally:
    Py_XDECREF(o);
    Py_DECREF(fast_seq);
    return -1;
}

/* returns NULL and sets the Python exception if an error occurred */
static PyObject *
set2list(fd_set *set, pylist fd2obj[FD_SETSIZE + 1])
{
    int i, j, count=0;
    PyObject *list, *o;
    SOCKET fd;

    for (j = 0; fd2obj[j].sentinel >= 0; j++) {
        if (FD_ISSET(fd2obj[j].fd, set))
            count++;
    }
    list = PyList_New(count);
    if (!list)
        return NULL;

    i = 0;
    for (j = 0; fd2obj[j].sentinel >= 0; j++) {
        fd = fd2obj[j].fd;
        if (FD_ISSET(fd, set)) {
            o = fd2obj[j].obj;
            fd2obj[j].obj = NULL;
            /* transfer ownership */
            if (PyList_SetItem(list, i, o) < 0)
                goto finally;

            i++;
        }
    }
    return list;
  finally:
    Py_DECREF(list);
    return NULL;
}

#undef SELECT_USES_HEAP
#if FD_SETSIZE > 1024
#define SELECT_USES_HEAP
#endif /* FD_SETSIZE > 1024 */

static PyObject *
select_select(PyObject *self, PyObject *args)
{
#ifdef SELECT_USES_HEAP
    pylist *rfd2obj, *wfd2obj, *efd2obj;
#else  /* !SELECT_USES_HEAP */
    /* XXX: All this should probably be implemented as follows:
     * - find the highest descriptor we're interested in
     * - add one
     * - that's the size
     * See: Stevens, APitUE, $12.5.1
     */
    pylist rfd2obj[FD_SETSIZE + 1];
    pylist wfd2obj[FD_SETSIZE + 1];
    pylist efd2obj[FD_SETSIZE + 1];
#endif /* SELECT_USES_HEAP */
    PyObject *ifdlist, *ofdlist, *efdlist;
    PyObject *ret = NULL;
    PyObject *tout = Py_None;
    fd_set ifdset, ofdset, efdset;
    double timeout;
    struct timeval tv, *tvp;
    long seconds;
    int imax, omax, emax, max;
    int n;

    /* convert arguments */
    if (!PyArg_UnpackTuple(args, "select", 3, 4,
                          &ifdlist, &ofdlist, &efdlist, &tout))
        return NULL;

    if (tout == Py_None)
        tvp = (struct timeval *)0;
    else if (!PyNumber_Check(tout)) {
        PyErr_SetString(PyExc_TypeError,
                        "timeout must be a float or None");
        return NULL;
    }
    else {
        timeout = PyFloat_AsDouble(tout);
        if (timeout == -1 && PyErr_Occurred())
            return NULL;
        if (timeout > (double)LONG_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "timeout period too long");
            return NULL;
        }
        seconds = (long)timeout;
        timeout = timeout - (double)seconds;
        tv.tv_sec = seconds;
        tv.tv_usec = (long)(timeout * 1E6);
        tvp = &tv;
    }


#ifdef SELECT_USES_HEAP
    /* Allocate memory for the lists */
    rfd2obj = PyMem_NEW(pylist, FD_SETSIZE + 1);
    wfd2obj = PyMem_NEW(pylist, FD_SETSIZE + 1);
    efd2obj = PyMem_NEW(pylist, FD_SETSIZE + 1);
    if (rfd2obj == NULL || wfd2obj == NULL || efd2obj == NULL) {
        if (rfd2obj) PyMem_DEL(rfd2obj);
        if (wfd2obj) PyMem_DEL(wfd2obj);
        if (efd2obj) PyMem_DEL(efd2obj);
        return PyErr_NoMemory();
    }
#endif /* SELECT_USES_HEAP */
    /* Convert sequences to fd_sets, and get maximum fd number
     * propagates the Python exception set in seq2set()
     */
    rfd2obj[0].sentinel = -1;
    wfd2obj[0].sentinel = -1;
    efd2obj[0].sentinel = -1;
    if ((imax=seq2set(ifdlist, &ifdset, rfd2obj)) < 0)
        goto finally;
    if ((omax=seq2set(ofdlist, &ofdset, wfd2obj)) < 0)
        goto finally;
    if ((emax=seq2set(efdlist, &efdset, efd2obj)) < 0)
        goto finally;
    max = imax;
    if (omax > max) max = omax;
    if (emax > max) max = emax;

    Py_BEGIN_ALLOW_THREADS
    n = select(max, &ifdset, &ofdset, &efdset, tvp);
    Py_END_ALLOW_THREADS

#ifdef MS_WINDOWS
    if (n == SOCKET_ERROR) {
        PyErr_SetExcFromWindowsErr(SelectError, WSAGetLastError());
    }
#else
    if (n < 0) {
        PyErr_SetFromErrno(SelectError);
    }
#endif
    else {
        /* any of these three calls can raise an exception.  it's more
           convenient to test for this after all three calls... but
           is that acceptable?
        */
        ifdlist = set2list(&ifdset, rfd2obj);
        ofdlist = set2list(&ofdset, wfd2obj);
        efdlist = set2list(&efdset, efd2obj);
        if (PyErr_Occurred())
            ret = NULL;
        else
            ret = PyTuple_Pack(3, ifdlist, ofdlist, efdlist);

        Py_XDECREF(ifdlist);
        Py_XDECREF(ofdlist);
        Py_XDECREF(efdlist);
    }

  finally:
    reap_obj(rfd2obj);
    reap_obj(wfd2obj);
    reap_obj(efd2obj);
#ifdef SELECT_USES_HEAP
    PyMem_DEL(rfd2obj);
    PyMem_DEL(wfd2obj);
    PyMem_DEL(efd2obj);
#endif /* SELECT_USES_HEAP */
    return ret;
}

#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)
/*
 * poll() support
 */

typedef struct {
    PyObject_HEAD
    PyObject *dict;
    int ufd_uptodate;
    int ufd_len;
    struct pollfd *ufds;
    int poll_running;
} pollObject;

static PyTypeObject poll_Type;

/* Update the malloc'ed array of pollfds to match the dictionary
   contained within a pollObject.  Return 1 on success, 0 on an error.
*/

static int
update_ufd_array(pollObject *self)
{
    Py_ssize_t i, pos;
    PyObject *key, *value;
    struct pollfd *old_ufds = self->ufds;

    self->ufd_len = PyDict_Size(self->dict);
    PyMem_RESIZE(self->ufds, struct pollfd, self->ufd_len);
    if (self->ufds == NULL) {
        self->ufds = old_ufds;
        PyErr_NoMemory();
        return 0;
    }

    i = pos = 0;
    while (PyDict_Next(self->dict, &pos, &key, &value)) {
        assert(i < self->ufd_len);
        /* Never overflow */
        self->ufds[i].fd = (int)PyInt_AsLong(key);
        self->ufds[i].events = (short)(unsigned short)PyInt_AsLong(value);
        i++;
    }
    assert(i == self->ufd_len);
    self->ufd_uptodate = 1;
    return 1;
}

static int
ushort_converter(PyObject *obj, void *ptr)
{
    unsigned long uval;

    uval = PyLong_AsUnsignedLong(obj);
    if (uval == (unsigned long)-1 && PyErr_Occurred())
        return 0;
    if (uval > USHRT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "Python int too large for C unsigned short");
        return 0;
    }

    *(unsigned short *)ptr = Py_SAFE_DOWNCAST(uval, unsigned long, unsigned short);
    return 1;
}

PyDoc_STRVAR(poll_register_doc,
"register(fd [, eventmask] ) -> None\n\n\
Register a file descriptor with the polling object.\n\
fd -- either an integer, or an object with a fileno() method returning an\n\
      int.\n\
events -- an optional bitmask describing the type of events to check for");

static PyObject *
poll_register(pollObject *self, PyObject *args)
{
    PyObject *o, *key, *value;
    int fd;
    unsigned short events = POLLIN | POLLPRI | POLLOUT;
    int err;

    if (!PyArg_ParseTuple(args, "O|O&:register", &o, ushort_converter, &events))
        return NULL;

    fd = PyObject_AsFileDescriptor(o);
    if (fd == -1) return NULL;

    /* Add entry to the internal dictionary: the key is the
       file descriptor, and the value is the event mask. */
    key = PyInt_FromLong(fd);
    if (key == NULL)
        return NULL;
    value = PyInt_FromLong(events);
    if (value == NULL) {
        Py_DECREF(key);
        return NULL;
    }
    err = PyDict_SetItem(self->dict, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
    if (err < 0)
        return NULL;

    self->ufd_uptodate = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(poll_modify_doc,
"modify(fd, eventmask) -> None\n\n\
Modify an already registered file descriptor.\n\
fd -- either an integer, or an object with a fileno() method returning an\n\
      int.\n\
events -- an optional bitmask describing the type of events to check for");

static PyObject *
poll_modify(pollObject *self, PyObject *args)
{
    PyObject *o, *key, *value;
    int fd;
    unsigned short events;
    int err;

    if (!PyArg_ParseTuple(args, "OO&:modify", &o, ushort_converter, &events))
        return NULL;

    fd = PyObject_AsFileDescriptor(o);
    if (fd == -1) return NULL;

    /* Modify registered fd */
    key = PyInt_FromLong(fd);
    if (key == NULL)
        return NULL;
    if (PyDict_GetItem(self->dict, key) == NULL) {
        errno = ENOENT;
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    value = PyInt_FromLong(events);
    if (value == NULL) {
        Py_DECREF(key);
        return NULL;
    }
    err = PyDict_SetItem(self->dict, key, value);
    Py_DECREF(key);
    Py_DECREF(value);
    if (err < 0)
        return NULL;

    self->ufd_uptodate = 0;

    Py_INCREF(Py_None);
    return Py_None;
}


PyDoc_STRVAR(poll_unregister_doc,
"unregister(fd) -> None\n\n\
Remove a file descriptor being tracked by the polling object.");

static PyObject *
poll_unregister(pollObject *self, PyObject *o)
{
    PyObject *key;
    int fd;

    fd = PyObject_AsFileDescriptor( o );
    if (fd == -1)
        return NULL;

    /* Check whether the fd is already in the array */
    key = PyInt_FromLong(fd);
    if (key == NULL)
        return NULL;

    if (PyDict_DelItem(self->dict, key) == -1) {
        Py_DECREF(key);
        /* This will simply raise the KeyError set by PyDict_DelItem
           if the file descriptor isn't registered. */
        return NULL;
    }

    Py_DECREF(key);
    self->ufd_uptodate = 0;

    Py_INCREF(Py_None);
    return Py_None;
}

PyDoc_STRVAR(poll_poll_doc,
"poll( [timeout] ) -> list of (fd, event) 2-tuples\n\n\
Polls the set of registered file descriptors, returning a list containing \n\
any descriptors that have events or errors to report.");

static PyObject *
poll_poll(pollObject *self, PyObject *args)
{
    PyObject *result_list = NULL, *tout = NULL;
    int timeout = 0, poll_result, i, j;
    PyObject *value = NULL, *num = NULL;

    if (!PyArg_UnpackTuple(args, "poll", 0, 1, &tout)) {
        return NULL;
    }

    /* Check values for timeout */
    if (tout == NULL || tout == Py_None)
        timeout = -1;
    else if (!PyNumber_Check(tout)) {
        PyErr_SetString(PyExc_TypeError,
                        "timeout must be an integer or None");
        return NULL;
    }
    else {
        tout = PyNumber_Int(tout);
        if (!tout)
            return NULL;
        timeout = _PyInt_AsInt(tout);
        Py_DECREF(tout);
        if (timeout == -1 && PyErr_Occurred())
            return NULL;
    }

    /* On some OSes, typically BSD-based ones, the timeout parameter of the
       poll() syscall, when negative, must be exactly INFTIM, where defined,
       or -1. See issue 31334. */
    if (timeout < 0) {
#ifdef INFTIM
        timeout = INFTIM;
#else
        timeout = -1;
#endif
    }

    /* Avoid concurrent poll() invocation, issue 8865 */
    if (self->poll_running) {
        PyErr_SetString(PyExc_RuntimeError,
                        "concurrent poll() invocation");
        return NULL;
    }

    /* Ensure the ufd array is up to date */
    if (!self->ufd_uptodate)
        if (update_ufd_array(self) == 0)
            return NULL;

    self->poll_running = 1;

    /* call poll() */
    Py_BEGIN_ALLOW_THREADS
    poll_result = poll(self->ufds, self->ufd_len, timeout);
    Py_END_ALLOW_THREADS

    self->poll_running = 0;

    if (poll_result < 0) {
        PyErr_SetFromErrno(SelectError);
        return NULL;
    }

    /* build the result list */

    result_list = PyList_New(poll_result);
    if (!result_list)
        return NULL;
    else {
        for (i = 0, j = 0; j < poll_result; j++) {
            /* skip to the next fired descriptor */
            while (!self->ufds[i].revents) {
                i++;
            }
            /* if we hit a NULL return, set value to NULL
               and break out of loop; code at end will
               clean up result_list */
            value = PyTuple_New(2);
            if (value == NULL)
                goto error;
            num = PyInt_FromLong(self->ufds[i].fd);
            if (num == NULL) {
                Py_DECREF(value);
                goto error;
            }
            PyTuple_SET_ITEM(value, 0, num);

            /* The &0xffff is a workaround for AIX.  'revents'
               is a 16-bit short, and IBM assigned POLLNVAL
               to be 0x8000, so the conversion to int results
               in a negative number. See SF bug #923315. */
            num = PyInt_FromLong(self->ufds[i].revents & 0xffff);
            if (num == NULL) {
                Py_DECREF(value);
                goto error;
            }
            PyTuple_SET_ITEM(value, 1, num);
            PyList_SET_ITEM(result_list, j, value);
            i++;
        }
    }
    return result_list;

  error:
    Py_DECREF(result_list);
    return NULL;
}

static PyMethodDef poll_methods[] = {
    {"register",        (PyCFunction)poll_register,
     METH_VARARGS,  poll_register_doc},
    {"modify",          (PyCFunction)poll_modify,
     METH_VARARGS,  poll_modify_doc},
    {"unregister",      (PyCFunction)poll_unregister,
     METH_O,        poll_unregister_doc},
    {"poll",            (PyCFunction)poll_poll,
     METH_VARARGS,  poll_poll_doc},
    {NULL,              NULL}           /* sentinel */
};

static pollObject *
newPollObject(void)
{
    pollObject *self;
    self = PyObject_New(pollObject, &poll_Type);
    if (self == NULL)
        return NULL;
    /* ufd_uptodate is a Boolean, denoting whether the
       array pointed to by ufds matches the contents of the dictionary. */
    self->ufd_uptodate = 0;
    self->ufds = NULL;
    self->poll_running = 0;
    self->dict = PyDict_New();
    if (self->dict == NULL) {
        Py_DECREF(self);
        return NULL;
    }
    return self;
}

static void
poll_dealloc(pollObject *self)
{
    if (self->ufds != NULL)
        PyMem_DEL(self->ufds);
    Py_XDECREF(self->dict);
    PyObject_Del(self);
}

static PyObject *
poll_getattr(pollObject *self, char *name)
{
    return Py_FindMethod(poll_methods, (PyObject *)self, name);
}

static PyTypeObject poll_Type = {
    /* The ob_type field must be initialized in the module init function
     * to be portable to Windows without using C++. */
    PyVarObject_HEAD_INIT(NULL, 0)
    "select.poll",              /*tp_name*/
    sizeof(pollObject),         /*tp_basicsize*/
    0,                          /*tp_itemsize*/
    /* methods */
    (destructor)poll_dealloc, /*tp_dealloc*/
    0,                          /*tp_print*/
    (getattrfunc)poll_getattr, /*tp_getattr*/
    0,                      /*tp_setattr*/
    0,                          /*tp_compare*/
    0,                          /*tp_repr*/
    0,                          /*tp_as_number*/
    0,                          /*tp_as_sequence*/
    0,                          /*tp_as_mapping*/
    0,                          /*tp_hash*/
};

PyDoc_STRVAR(poll_doc,
"Returns a polling object, which supports registering and\n\
unregistering file descriptors, and then polling them for I/O events.");

static PyObject *
select_poll(PyObject *self, PyObject *unused)
{
    return (PyObject *)newPollObject();
}

#ifdef __APPLE__
/*
 * On some systems poll() sets errno on invalid file descriptors. We test
 * for this at runtime because this bug may be fixed or introduced between
 * OS releases.
 */
static int select_have_broken_poll(void)
{
    int poll_test;
    int filedes[2];

    struct pollfd poll_struct = { 0, POLLIN|POLLPRI|POLLOUT, 0 };

    /* Create a file descriptor to make invalid */
    if (pipe(filedes) < 0) {
        return 1;
    }
    poll_struct.fd = filedes[0];
    close(filedes[0]);
    close(filedes[1]);
    poll_test = poll(&poll_struct, 1, 0);
    if (poll_test < 0) {
        return 1;
    } else if (poll_test == 0 && poll_struct.revents != POLLNVAL) {
        return 1;
    }
    return 0;
}
#endif /* __APPLE__ */

#endif /* HAVE_POLL */

#ifdef HAVE_EPOLL
/* **************************************************************************
 *                      epoll interface for Linux 2.6
 *
 * Written by Christian Heimes
 * Inspired by Twisted's _epoll.pyx and select.poll()
 */

#ifdef HAVE_SYS_EPOLL_H
#include <sys/epoll.h>
#endif

typedef struct {
    PyObject_HEAD
    SOCKET epfd;                        /* epoll control file descriptor */
} pyEpoll_Object;

static PyTypeObject pyEpoll_Type;
#define pyepoll_CHECK(op) (PyObject_TypeCheck((op), &pyEpoll_Type))

static PyObject *
pyepoll_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed epoll fd");
    return NULL;
}

static int
pyepoll_internal_close(pyEpoll_Object *self)
{
    int save_errno = 0;
    if (self->epfd >= 0) {
        int epfd = self->epfd;
        self->epfd = -1;
        Py_BEGIN_ALLOW_THREADS
        if (close(epfd) < 0)
            save_errno = errno;
        Py_END_ALLOW_THREADS
    }
    return save_errno;
}

static PyObject *
newPyEpoll_Object(PyTypeObject *type, int sizehint, SOCKET fd)
{
    pyEpoll_Object *self;

    if (sizehint == -1) {
        sizehint = FD_SETSIZE-1;
    }
    else if (sizehint < 1) {
        PyErr_Format(PyExc_ValueError,
                     "sizehint must be greater zero, got %d",
                     sizehint);
        return NULL;
    }

    assert(type != NULL && type->tp_alloc != NULL);
    self = (pyEpoll_Object *) type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;

    if (fd == -1) {
        Py_BEGIN_ALLOW_THREADS
        self->epfd = epoll_create(sizehint);
        Py_END_ALLOW_THREADS
    }
    else {
        self->epfd = fd;
    }
    if (self->epfd < 0) {
        Py_DECREF(self);
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return (PyObject *)self;
}


static PyObject *
pyepoll_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    int sizehint = -1;
    static char *kwlist[] = {"sizehint", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i:epoll", kwlist,
                                     &sizehint))
        return NULL;

    return newPyEpoll_Object(type, sizehint, -1);
}


static void
pyepoll_dealloc(pyEpoll_Object *self)
{
    (void)pyepoll_internal_close(self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
pyepoll_close(pyEpoll_Object *self)
{
    errno = pyepoll_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(pyepoll_close_doc,
"close() -> None\n\
\n\
Close the epoll control file descriptor. Further operations on the epoll\n\
object will raise an exception.");

static PyObject*
pyepoll_get_closed(pyEpoll_Object *self)
{
    if (self->epfd < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject*
pyepoll_fileno(pyEpoll_Object *self)
{
    if (self->epfd < 0)
        return pyepoll_err_closed();
    return PyInt_FromLong(self->epfd);
}

PyDoc_STRVAR(pyepoll_fileno_doc,
"fileno() -> int\n\
\n\
Return the epoll control file descriptor.");

static PyObject*
pyepoll_fromfd(PyObject *cls, PyObject *args)
{
    SOCKET fd;

    if (!PyArg_ParseTuple(args, "i:fromfd", &fd))
        return NULL;

    return newPyEpoll_Object((PyTypeObject*)cls, -1, fd);
}

PyDoc_STRVAR(pyepoll_fromfd_doc,
"fromfd(fd) -> epoll\n\
\n\
Create an epoll object from a given control fd.");

static PyObject *
pyepoll_internal_ctl(int epfd, int op, PyObject *pfd, unsigned int events)
{
    struct epoll_event ev;
    int result;
    int fd;

    if (epfd < 0)
        return pyepoll_err_closed();

    fd = PyObject_AsFileDescriptor(pfd);
    if (fd == -1) {
        return NULL;
    }

    switch(op) {
        case EPOLL_CTL_ADD:
        case EPOLL_CTL_MOD:
        ev.events = events;
        ev.data.fd = fd;
        Py_BEGIN_ALLOW_THREADS
        result = epoll_ctl(epfd, op, fd, &ev);
        Py_END_ALLOW_THREADS
        break;
        case EPOLL_CTL_DEL:
        /* In kernel versions before 2.6.9, the EPOLL_CTL_DEL
         * operation required a non-NULL pointer in event, even
         * though this argument is ignored. */
        Py_BEGIN_ALLOW_THREADS
        result = epoll_ctl(epfd, op, fd, &ev);
        if (errno == EBADF) {
            /* fd already closed */
            result = 0;
            errno = 0;
        }
        Py_END_ALLOW_THREADS
        break;
        default:
        result = -1;
        errno = EINVAL;
    }

    if (result < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
pyepoll_register(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    unsigned int events = EPOLLIN | EPOLLOUT | EPOLLPRI;
    static char *kwlist[] = {"fd", "eventmask", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|I:register", kwlist,
                                     &pfd, &events)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_ADD, pfd, events);
}

PyDoc_STRVAR(pyepoll_register_doc,
"register(fd[, eventmask]) -> None\n\
\n\
Registers a new fd or raises an IOError if the fd is already registered.\n\
fd is the target file descriptor of the operation.\n\
events is a bit set composed of the various EPOLL constants; the default\n\
is EPOLL_IN | EPOLL_OUT | EPOLL_PRI.\n\
\n\
The epoll interface supports all file descriptors that support poll.");

static PyObject *
pyepoll_modify(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    unsigned int events;
    static char *kwlist[] = {"fd", "eventmask", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OI:modify", kwlist,
                                     &pfd, &events)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_MOD, pfd, events);
}

PyDoc_STRVAR(pyepoll_modify_doc,
"modify(fd, eventmask) -> None\n\
\n\
fd is the target file descriptor of the operation\n\
events is a bit set composed of the various EPOLL constants");

static PyObject *
pyepoll_unregister(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    static char *kwlist[] = {"fd", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O:unregister", kwlist,
                                     &pfd)) {
        return NULL;
    }

    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_DEL, pfd, 0);
}

PyDoc_STRVAR(pyepoll_unregister_doc,
"unregister(fd) -> None\n\
\n\
fd is the target file descriptor of the operation.");

static PyObject *
pyepoll_poll(pyEpoll_Object *self, PyObject *args, PyObject *kwds)
{
    double dtimeout = -1.;
    int timeout;
    int maxevents = -1;
    int nfds, i;
    PyObject *elist = NULL, *etuple = NULL;
    struct epoll_event *evs = NULL;
    static char *kwlist[] = {"timeout", "maxevents", NULL};

    if (self->epfd < 0)
        return pyepoll_err_closed();

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|di:poll", kwlist,
                                     &dtimeout, &maxevents)) {
        return NULL;
    }

    if (dtimeout < 0) {
        timeout = -1;
    }
    else if (dtimeout * 1000.0 > INT_MAX) {
        PyErr_SetString(PyExc_OverflowError,
                        "timeout is too large");
        return NULL;
    }
    else {
        timeout = (int)(dtimeout * 1000.0);
    }

    if (maxevents == -1) {
        maxevents = FD_SETSIZE-1;
    }
    else if (maxevents < 1) {
        PyErr_Format(PyExc_ValueError,
                     "maxevents must be greater than 0, got %d",
                     maxevents);
        return NULL;
    }

    evs = PyMem_New(struct epoll_event, maxevents);
    if (evs == NULL) {
        Py_DECREF(self);
        PyErr_NoMemory();
        return NULL;
    }

    Py_BEGIN_ALLOW_THREADS
    nfds = epoll_wait(self->epfd, evs, maxevents, timeout);
    Py_END_ALLOW_THREADS
    if (nfds < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        goto error;
    }

    elist = PyList_New(nfds);
    if (elist == NULL) {
        goto error;
    }

    for (i = 0; i < nfds; i++) {
        etuple = Py_BuildValue("iI", evs[i].data.fd, evs[i].events);
        if (etuple == NULL) {
            Py_CLEAR(elist);
            goto error;
        }
        PyList_SET_ITEM(elist, i, etuple);
    }

    error:
    PyMem_Free(evs);
    return elist;
}

PyDoc_STRVAR(pyepoll_poll_doc,
"poll([timeout=-1[, maxevents=-1]]) -> [(fd, events), (...)]\n\
\n\
Wait for events on the epoll file descriptor for a maximum time of timeout\n\
in seconds (as float). -1 makes poll wait indefinitely.\n\
Up to maxevents are returned to the caller.");

static PyMethodDef pyepoll_methods[] = {
    {"fromfd",          (PyCFunction)pyepoll_fromfd,
     METH_VARARGS | METH_CLASS, pyepoll_fromfd_doc},
    {"close",           (PyCFunction)pyepoll_close,     METH_NOARGS,
     pyepoll_close_doc},
    {"fileno",          (PyCFunction)pyepoll_fileno,    METH_NOARGS,
     pyepoll_fileno_doc},
    {"modify",          (PyCFunction)pyepoll_modify,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_modify_doc},
    {"register",        (PyCFunction)pyepoll_register,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_register_doc},
    {"unregister",      (PyCFunction)pyepoll_unregister,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_unregister_doc},
    {"poll",            (PyCFunction)pyepoll_poll,
     METH_VARARGS | METH_KEYWORDS,      pyepoll_poll_doc},
    {NULL,      NULL},
};

static PyGetSetDef pyepoll_getsetlist[] = {
    {"closed", (getter)pyepoll_get_closed, NULL,
     "True if the epoll handler is closed"},
    {0},
};

PyDoc_STRVAR(pyepoll_doc,
"select.epoll([sizehint=-1])\n\
\n\
Returns an epolling object\n\
\n\
sizehint must be a positive integer or -1 for the default size. The\n\
sizehint is used to optimize internal data structures. It doesn't limit\n\
the maximum number of monitored events.");

static PyTypeObject pyEpoll_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "select.epoll",                                     /* tp_name */
    sizeof(pyEpoll_Object),                             /* tp_basicsize */
    0,                                                  /* tp_itemsize */
    (destructor)pyepoll_dealloc,                        /* tp_dealloc */
    0,                                                  /* tp_print */
    0,                                                  /* tp_getattr */
    0,                                                  /* tp_setattr */
    0,                                                  /* tp_compare */
    0,                                                  /* tp_repr */
    0,                                                  /* tp_as_number */
    0,                                                  /* tp_as_sequence */
    0,                                                  /* tp_as_mapping */
    0,                                                  /* tp_hash */
    0,                                                  /* tp_call */
    0,                                                  /* tp_str */
    PyObject_GenericGetAttr,                            /* tp_getattro */
    0,                                                  /* tp_setattro */
    0,                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                 /* tp_flags */
    pyepoll_doc,                                        /* tp_doc */
    0,                                                  /* tp_traverse */
    0,                                                  /* tp_clear */
    0,                                                  /* tp_richcompare */
    0,                                                  /* tp_weaklistoffset */
    0,                                                  /* tp_iter */
    0,                                                  /* tp_iternext */
    pyepoll_methods,                                    /* tp_methods */
    0,                                                  /* tp_members */
    pyepoll_getsetlist,                                 /* tp_getset */
    0,                                                  /* tp_base */
    0,                                                  /* tp_dict */
    0,                                                  /* tp_descr_get */
    0,                                                  /* tp_descr_set */
    0,                                                  /* tp_dictoffset */
    0,                                                  /* tp_init */
    0,                                                  /* tp_alloc */
    pyepoll_new,                                        /* tp_new */
    0,                                                  /* tp_free */
};

#endif /* HAVE_EPOLL */

#ifdef HAVE_KQUEUE
/* **************************************************************************
 *                      kqueue interface for BSD
 *
 * Copyright (c) 2000 Doug White, 2006 James Knight, 2007 Christian Heimes
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifdef HAVE_SYS_EVENT_H
#include <sys/event.h>
#endif

PyDoc_STRVAR(kqueue_event_doc,
"kevent(ident, filter=KQ_FILTER_READ, flags=KQ_EV_ADD, fflags=0, data=0, udata=0)\n\
\n\
This object is the equivalent of the struct kevent for the C API.\n\
\n\
See the kqueue manpage for more detailed information about the meaning\n\
of the arguments.\n\
\n\
One minor note: while you might hope that udata could store a\n\
reference to a python object, it cannot, because it is impossible to\n\
keep a proper reference count of the object once it's passed into the\n\
kernel. Therefore, I have restricted it to only storing an integer.  I\n\
recommend ignoring it and simply using the 'ident' field to key off\n\
of. You could also set up a dictionary on the python side to store a\n\
udata->object mapping.");

typedef struct {
    PyObject_HEAD
    struct kevent e;
} kqueue_event_Object;

static PyTypeObject kqueue_event_Type;

#define kqueue_event_Check(op) (PyObject_TypeCheck((op), &kqueue_event_Type))

typedef struct {
    PyObject_HEAD
    SOCKET kqfd;                /* kqueue control fd */
} kqueue_queue_Object;

static PyTypeObject kqueue_queue_Type;

#define kqueue_queue_Check(op) (PyObject_TypeCheck((op), &kqueue_queue_Type))

#if (SIZEOF_UINTPTR_T != SIZEOF_VOID_P)
#   error uintptr_t does not match void *!
#elif defined(HAVE_LONG_LONG) && (SIZEOF_UINTPTR_T == SIZEOF_LONG_LONG)
#   define T_UINTPTRT         T_ULONGLONG
#   define T_INTPTRT          T_LONGLONG
#   define UINTPTRT_FMT_UNIT  "K"
#   define INTPTRT_FMT_UNIT   "L"
#elif (SIZEOF_UINTPTR_T == SIZEOF_LONG)
#   define T_UINTPTRT         T_ULONG
#   define T_INTPTRT          T_LONG
#   define UINTPTRT_FMT_UNIT  "k"
#   define INTPTRT_FMT_UNIT   "l"
#elif (SIZEOF_UINTPTR_T == SIZEOF_INT)
#   define T_UINTPTRT         T_UINT
#   define T_INTPTRT          T_INT
#   define UINTPTRT_FMT_UNIT  "I"
#   define INTPTRT_FMT_UNIT   "i"
#else
#   error uintptr_t does not match int, long, or long long!
#endif

#if defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG == 8
#   define T_INT64          T_LONGLONG
#   define INT64_FMT_UNIT   "L"
#elif SIZEOF_LONG == 8
#   define T_INT64          T_LONG
#   define INT64_FMT_UNIT   "l"
#elif SIZEOF_INT == 8
#   define T_INT64          T_INT
#   define INT64_FMT_UNIT   "i"
#else
#   define INT64_FMT_UNIT   "_"
#endif

#if defined(HAVE_LONG_LONG) && SIZEOF_LONG_LONG == 4
#   define T_UINT32         T_ULONGLONG
#   define UINT32_FMT_UNIT  "K"
#elif SIZEOF_LONG == 4
#   define T_UINT32         T_ULONG
#   define UINT32_FMT_UNIT  "k"
#elif SIZEOF_INT == 4
#   define T_UINT32         T_UINT
#   define UINT32_FMT_UNIT  "I"
#else
#   define UINT32_FMT_UNIT  "_"
#endif

/*
 * kevent is not standard and its members vary across BSDs.
 */
#ifdef __NetBSD__
#   define FILTER_TYPE      T_UINT32
#   define FILTER_FMT_UNIT  UINT32_FMT_UNIT
#   define FLAGS_TYPE       T_UINT32
#   define FLAGS_FMT_UNIT   UINT32_FMT_UNIT
#   define FFLAGS_TYPE      T_UINT32
#   define FFLAGS_FMT_UNIT  UINT32_FMT_UNIT
#else
#   define FILTER_TYPE      T_SHORT
#   define FILTER_FMT_UNIT  "h"
#   define FLAGS_TYPE       T_USHORT
#   define FLAGS_FMT_UNIT   "H"
#   define FFLAGS_TYPE      T_UINT
#   define FFLAGS_FMT_UNIT  "I"
#endif

#if defined(__NetBSD__) || defined(__OpenBSD__)
#   define DATA_TYPE        T_INT64
#   define DATA_FMT_UNIT    INT64_FMT_UNIT
#else
#   define DATA_TYPE        T_INTPTRT
#   define DATA_FMT_UNIT    INTPTRT_FMT_UNIT
#endif

/* Unfortunately, we can't store python objects in udata, because
 * kevents in the kernel can be removed without warning, which would
 * forever lose the refcount on the object stored with it.
 */

#define KQ_OFF(x) offsetof(kqueue_event_Object, x)
static struct PyMemberDef kqueue_event_members[] = {
    {"ident",           T_UINTPTRT,     KQ_OFF(e.ident)},
    {"filter",          FILTER_TYPE,    KQ_OFF(e.filter)},
    {"flags",           FLAGS_TYPE,     KQ_OFF(e.flags)},
    {"fflags",          T_UINT,         KQ_OFF(e.fflags)},
    {"data",            DATA_TYPE,      KQ_OFF(e.data)},
    {"udata",           T_UINTPTRT,     KQ_OFF(e.udata)},
    {NULL} /* Sentinel */
};
#undef KQ_OFF

static PyObject *

kqueue_event_repr(kqueue_event_Object *s)
{
    char buf[1024];
    PyOS_snprintf(
        buf, sizeof(buf),
#ifdef HAVE_LONG_LONG
        "<select.kevent ident=%zu filter=%d flags=0x%x fflags=0x%x "
        "data=0x%llx udata=%p>",
        (size_t)(s->e.ident), (int)s->e.filter, (unsigned int)s->e.flags,
        (unsigned int)s->e.fflags, (long long)(s->e.data), (void *)s->e.udata);
#else
        "<select.kevent ident=%zu filter=%d flags=0x%x fflags=0x%x "
        "data=0x%llx udata=%p>",
        (size_t)(s->e.ident), (int)s->e.filter, (unsigned int)s->e.flags,
        (unsigned int)s->e.fflags, (long)(s->e.data), (void *)s->e.udata);
#endif
    return PyString_FromString(buf);
}

static int
kqueue_event_init(kqueue_event_Object *self, PyObject *args, PyObject *kwds)
{
    PyObject *pfd;
    static char *kwlist[] = {"ident", "filter", "flags", "fflags",
                             "data", "udata", NULL};
    static const char fmt[] = "O|"
                FILTER_FMT_UNIT FLAGS_FMT_UNIT FFLAGS_FMT_UNIT DATA_FMT_UNIT
                UINTPTRT_FMT_UNIT ":kevent";

    EV_SET(&(self->e), 0, EVFILT_READ, EV_ADD, 0, 0, 0); /* defaults */

    if (!PyArg_ParseTupleAndKeywords(args, kwds, fmt, kwlist,
        &pfd, &(self->e.filter), &(self->e.flags),
        &(self->e.fflags), &(self->e.data), &(self->e.udata))) {
        return -1;
    }

    if (PyInt_Check(pfd)) {
        self->e.ident = PyInt_AsUnsignedLongMask(pfd);
    }
    else if (PyLong_Check(pfd)) {
#if defined(HAVE_LONG_LONG) && (SIZEOF_UINTPTR_T == SIZEOF_LONG_LONG)
        self->e.ident = PyLong_AsUnsignedLongLongMask(pfd);
#else
        self->e.ident = PyLong_AsUnsignedLongMask(pfd);
#endif
    }
    else {
        self->e.ident = PyObject_AsFileDescriptor(pfd);
    }
    if (PyErr_Occurred()) {
        return -1;
    }
    return 0;
}

static PyObject *
kqueue_event_richcompare(kqueue_event_Object *s, kqueue_event_Object *o,
                         int op)
{
    int result;

    if (!kqueue_event_Check(o)) {
        Py_INCREF(Py_NotImplemented);
        return Py_NotImplemented;
    }

#define CMP(a, b) ((a) != (b)) ? ((a) < (b) ? -1 : 1)
    result = CMP(s->e.ident, o->e.ident)
           : CMP(s->e.filter, o->e.filter)
           : CMP(s->e.flags, o->e.flags)
           : CMP(s->e.fflags, o->e.fflags)
           : CMP(s->e.data, o->e.data)
           : CMP((Py_intptr_t)s->e.udata, (Py_intptr_t)o->e.udata)
           : 0;
#undef CMP

    switch (op) {
        case Py_EQ:
        result = (result == 0);
        break;
        case Py_NE:
        result = (result != 0);
        break;
        case Py_LE:
        result = (result <= 0);
        break;
        case Py_GE:
        result = (result >= 0);
        break;
        case Py_LT:
        result = (result < 0);
        break;
        case Py_GT:
        result = (result > 0);
        break;
    }
    return PyBool_FromLong((long)result);
}

static PyTypeObject kqueue_event_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "select.kevent",                                    /* tp_name */
    sizeof(kqueue_event_Object),                        /* tp_basicsize */
    0,                                                  /* tp_itemsize */
    0,                                                  /* tp_dealloc */
    0,                                                  /* tp_print */
    0,                                                  /* tp_getattr */
    0,                                                  /* tp_setattr */
    0,                                                  /* tp_compare */
    (reprfunc)kqueue_event_repr,                        /* tp_repr */
    0,                                                  /* tp_as_number */
    0,                                                  /* tp_as_sequence */
    0,                                                  /* tp_as_mapping */
    0,                                                  /* tp_hash */
    0,                                                  /* tp_call */
    0,                                                  /* tp_str */
    0,                                                  /* tp_getattro */
    0,                                                  /* tp_setattro */
    0,                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                 /* tp_flags */
    kqueue_event_doc,                                   /* tp_doc */
    0,                                                  /* tp_traverse */
    0,                                                  /* tp_clear */
    (richcmpfunc)kqueue_event_richcompare,              /* tp_richcompare */
    0,                                                  /* tp_weaklistoffset */
    0,                                                  /* tp_iter */
    0,                                                  /* tp_iternext */
    0,                                                  /* tp_methods */
    kqueue_event_members,                               /* tp_members */
    0,                                                  /* tp_getset */
    0,                                                  /* tp_base */
    0,                                                  /* tp_dict */
    0,                                                  /* tp_descr_get */
    0,                                                  /* tp_descr_set */
    0,                                                  /* tp_dictoffset */
    (initproc)kqueue_event_init,                        /* tp_init */
    0,                                                  /* tp_alloc */
    0,                                                  /* tp_new */
    0,                                                  /* tp_free */
};

static PyObject *
kqueue_queue_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed kqueue fd");
    return NULL;
}

static int
kqueue_queue_internal_close(kqueue_queue_Object *self)
{
    int save_errno = 0;
    if (self->kqfd >= 0) {
        int kqfd = self->kqfd;
        self->kqfd = -1;
        Py_BEGIN_ALLOW_THREADS
        if (close(kqfd) < 0)
            save_errno = errno;
        Py_END_ALLOW_THREADS
    }
    return save_errno;
}

static PyObject *
newKqueue_Object(PyTypeObject *type, SOCKET fd)
{
    kqueue_queue_Object *self;
    assert(type != NULL && type->tp_alloc != NULL);
    self = (kqueue_queue_Object *) type->tp_alloc(type, 0);
    if (self == NULL) {
        return NULL;
    }

    if (fd == -1) {
        Py_BEGIN_ALLOW_THREADS
        self->kqfd = kqueue();
        Py_END_ALLOW_THREADS
    }
    else {
        self->kqfd = fd;
    }
    if (self->kqfd < 0) {
        Py_DECREF(self);
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    return (PyObject *)self;
}

static PyObject *
kqueue_queue_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{

    if ((args != NULL && PyObject_Size(args)) ||
                    (kwds != NULL && PyObject_Size(kwds))) {
        PyErr_SetString(PyExc_ValueError,
                        "select.kqueue doesn't accept arguments");
        return NULL;
    }

    return newKqueue_Object(type, -1);
}

static void
kqueue_queue_dealloc(kqueue_queue_Object *self)
{
    kqueue_queue_internal_close(self);
    Py_TYPE(self)->tp_free(self);
}

static PyObject*
kqueue_queue_close(kqueue_queue_Object *self)
{
    errno = kqueue_queue_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_IOError);
        return NULL;
    }
    Py_RETURN_NONE;
}

PyDoc_STRVAR(kqueue_queue_close_doc,
"close() -> None\n\
\n\
Close the kqueue control file descriptor. Further operations on the kqueue\n\
object will raise an exception.");

static PyObject*
kqueue_queue_get_closed(kqueue_queue_Object *self)
{
    if (self->kqfd < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject*
kqueue_queue_fileno(kqueue_queue_Object *self)
{
    if (self->kqfd < 0)
        return kqueue_queue_err_closed();
    return PyInt_FromLong(self->kqfd);
}

PyDoc_STRVAR(kqueue_queue_fileno_doc,
"fileno() -> int\n\
\n\
Return the kqueue control file descriptor.");

static PyObject*
kqueue_queue_fromfd(PyObject *cls, PyObject *args)
{
    SOCKET fd;

    if (!PyArg_ParseTuple(args, "i:fromfd", &fd))
        return NULL;

    return newKqueue_Object((PyTypeObject*)cls, fd);
}

PyDoc_STRVAR(kqueue_queue_fromfd_doc,
"fromfd(fd) -> kqueue\n\
\n\
Create a kqueue object from a given control fd.");

static PyObject *
kqueue_queue_control(kqueue_queue_Object *self, PyObject *args)
{
    int nevents = 0;
    int gotevents = 0;
    int nchanges = 0;
    int i = 0;
    PyObject *otimeout = NULL;
    PyObject *ch = NULL;
    PyObject *seq = NULL, *ei = NULL;
    PyObject *result = NULL;
    struct kevent *evl = NULL;
    struct kevent *chl = NULL;
    struct timespec timeoutspec;
    struct timespec *ptimeoutspec;

    if (self->kqfd < 0)
        return kqueue_queue_err_closed();

    if (!PyArg_ParseTuple(args, "Oi|O:control", &ch, &nevents, &otimeout))
        return NULL;

    if (nevents < 0) {
        PyErr_Format(PyExc_ValueError,
            "Length of eventlist must be 0 or positive, got %d",
            nevents);
        return NULL;
    }

    if (otimeout == Py_None || otimeout == NULL) {
        ptimeoutspec = NULL;
    }
    else if (PyNumber_Check(otimeout)) {
        double timeout;
        long seconds;

        timeout = PyFloat_AsDouble(otimeout);
        if (timeout == -1 && PyErr_Occurred())
            return NULL;
        if (timeout > (double)LONG_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "timeout period too long");
            return NULL;
        }
        if (timeout < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "timeout must be positive or None");
            return NULL;
        }

        seconds = (long)timeout;
        timeout = timeout - (double)seconds;
        timeoutspec.tv_sec = seconds;
        timeoutspec.tv_nsec = (long)(timeout * 1E9);
        ptimeoutspec = &timeoutspec;
    }
    else {
        PyErr_Format(PyExc_TypeError,
            "timeout argument must be an number "
            "or None, got %.200s",
            Py_TYPE(otimeout)->tp_name);
        return NULL;
    }

    if (ch != NULL && ch != Py_None) {
        seq = PySequence_Fast(ch, "changelist is not iterable");
        if (seq == NULL) {
            return NULL;
        }
        if (PySequence_Fast_GET_SIZE(seq) > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError,
                            "changelist is too long");
            goto error;
        }
        nchanges = (int)PySequence_Fast_GET_SIZE(seq);

        chl = PyMem_New(struct kevent, nchanges);
        if (chl == NULL) {
            PyErr_NoMemory();
            goto error;
        }
        for (i = 0; i < nchanges; ++i) {
            ei = PySequence_Fast_GET_ITEM(seq, i);
            if (!kqueue_event_Check(ei)) {
                PyErr_SetString(PyExc_TypeError,
                    "changelist must be an iterable of "
                    "select.kevent objects");
                goto error;
            }
            chl[i] = ((kqueue_event_Object *)ei)->e;
        }
        Py_CLEAR(seq);
    }

    /* event list */
    if (nevents) {
        evl = PyMem_New(struct kevent, nevents);
        if (evl == NULL) {
            PyErr_NoMemory();
            goto error;
        }
    }

    Py_BEGIN_ALLOW_THREADS
    gotevents = kevent(self->kqfd, chl, nchanges,
                       evl, nevents, ptimeoutspec);
    Py_END_ALLOW_THREADS

    if (gotevents == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        goto error;
    }

    result = PyList_New(gotevents);
    if (result == NULL) {
        goto error;
    }

    for (i = 0; i < gotevents; i++) {
        kqueue_event_Object *ch;

        ch = PyObject_New(kqueue_event_Object, &kqueue_event_Type);
        if (ch == NULL) {
            goto error;
        }
        ch->e = evl[i];
        PyList_SET_ITEM(result, i, (PyObject *)ch);
    }
    PyMem_Free(chl);
    PyMem_Free(evl);
    return result;

    error:
    PyMem_Free(chl);
    PyMem_Free(evl);
    Py_XDECREF(result);
    Py_XDECREF(seq);
    return NULL;
}

PyDoc_STRVAR(kqueue_queue_control_doc,
"control(changelist, max_events[, timeout=None]) -> eventlist\n\
\n\
Calls the kernel kevent function.\n\
- changelist must be an iterable of kevent objects describing the changes\n\
  to be made to the kernel's watch list or None.\n\
- max_events lets you specify the maximum number of events that the\n\
  kernel will return.\n\
- timeout is the maximum time to wait in seconds, or else None,\n\
  to wait forever. timeout accepts floats for smaller timeouts, too.");


static PyMethodDef kqueue_queue_methods[] = {
    {"fromfd",          (PyCFunction)kqueue_queue_fromfd,
     METH_VARARGS | METH_CLASS, kqueue_queue_fromfd_doc},
    {"close",           (PyCFunction)kqueue_queue_close,        METH_NOARGS,
     kqueue_queue_close_doc},
    {"fileno",          (PyCFunction)kqueue_queue_fileno,       METH_NOARGS,
     kqueue_queue_fileno_doc},
    {"control",         (PyCFunction)kqueue_queue_control,
     METH_VARARGS ,     kqueue_queue_control_doc},
    {NULL,      NULL},
};

static PyGetSetDef kqueue_queue_getsetlist[] = {
    {"closed", (getter)kqueue_queue_get_closed, NULL,
     "True if the kqueue handler is closed"},
    {0},
};

PyDoc_STRVAR(kqueue_queue_doc,
"Kqueue syscall wrapper.\n\
\n\
For example, to start watching a socket for input:\n\
>>> kq = kqueue()\n\
>>> sock = socket()\n\
>>> sock.connect((host, port))\n\
>>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_ADD)], 0)\n\
\n\
To wait one second for it to become writeable:\n\
>>> kq.control(None, 1, 1000)\n\
\n\
To stop listening:\n\
>>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_DELETE)], 0)");

static PyTypeObject kqueue_queue_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "select.kqueue",                                    /* tp_name */
    sizeof(kqueue_queue_Object),                        /* tp_basicsize */
    0,                                                  /* tp_itemsize */
    (destructor)kqueue_queue_dealloc,                   /* tp_dealloc */
    0,                                                  /* tp_print */
    0,                                                  /* tp_getattr */
    0,                                                  /* tp_setattr */
    0,                                                  /* tp_compare */
    0,                                                  /* tp_repr */
    0,                                                  /* tp_as_number */
    0,                                                  /* tp_as_sequence */
    0,                                                  /* tp_as_mapping */
    0,                                                  /* tp_hash */
    0,                                                  /* tp_call */
    0,                                                  /* tp_str */
    0,                                                  /* tp_getattro */
    0,                                                  /* tp_setattro */
    0,                                                  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,                                 /* tp_flags */
    kqueue_queue_doc,                                   /* tp_doc */
    0,                                                  /* tp_traverse */
    0,                                                  /* tp_clear */
    0,                                                  /* tp_richcompare */
    0,                                                  /* tp_weaklistoffset */
    0,                                                  /* tp_iter */
    0,                                                  /* tp_iternext */
    kqueue_queue_methods,                               /* tp_methods */
    0,                                                  /* tp_members */
    kqueue_queue_getsetlist,                            /* tp_getset */
    0,                                                  /* tp_base */
    0,                                                  /* tp_dict */
    0,                                                  /* tp_descr_get */
    0,                                                  /* tp_descr_set */
    0,                                                  /* tp_dictoffset */
    0,                                                  /* tp_init */
    0,                                                  /* tp_alloc */
    kqueue_queue_new,                                   /* tp_new */
    0,                                                  /* tp_free */
};

#endif /* HAVE_KQUEUE */
/* ************************************************************************ */

PyDoc_STRVAR(select_doc,
"select(rlist, wlist, xlist[, timeout]) -> (rlist, wlist, xlist)\n\
\n\
Wait until one or more file descriptors are ready for some kind of I/O.\n\
The first three arguments are sequences of file descriptors to be waited for:\n\
rlist -- wait until ready for reading\n\
wlist -- wait until ready for writing\n\
xlist -- wait for an ``exceptional condition''\n\
If only one kind of condition is required, pass [] for the other lists.\n\
A file descriptor is either a socket or file object, or a small integer\n\
gotten from a fileno() method call on one of those.\n\
\n\
The optional 4th argument specifies a timeout in seconds; it may be\n\
a floating point number to specify fractions of seconds.  If it is absent\n\
or None, the call will never time out.\n\
\n\
The return value is a tuple of three lists corresponding to the first three\n\
arguments; each contains the subset of the corresponding file descriptors\n\
that are ready.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows and OpenVMS, only sockets are supported; on Unix, all file\n\
descriptors can be used.");

static PyMethodDef select_methods[] = {
    {"select",          select_select,  METH_VARARGS,   select_doc},
#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)
    {"poll",            select_poll,    METH_NOARGS,    poll_doc},
#endif /* HAVE_POLL */
    {0,         0},     /* sentinel */
};

PyDoc_STRVAR(module_doc,
"This module supports asynchronous I/O on multiple file descriptors.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows and OpenVMS, only sockets are supported; on Unix, all file descriptors.");

PyMODINIT_FUNC
initselect(void)
{
    PyObject *m;
    m = Py_InitModule3("select", select_methods, module_doc);
    if (m == NULL)
        return;

    SelectError = PyErr_NewException("select.error", NULL, NULL);
    Py_INCREF(SelectError);
    PyModule_AddObject(m, "error", SelectError);

#ifdef PIPE_BUF
#ifdef HAVE_BROKEN_PIPE_BUF
#undef PIPE_BUF
#define PIPE_BUF 512
#endif
    PyModule_AddIntConstant(m, "PIPE_BUF", PIPE_BUF);
#endif

#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)
#ifdef __APPLE__
    if (select_have_broken_poll()) {
        if (PyObject_DelAttrString(m, "poll") == -1) {
            PyErr_Clear();
        }
    } else {
#else
    {
#endif
        Py_TYPE(&poll_Type) = &PyType_Type;
        PyModule_AddIntConstant(m, "POLLIN", POLLIN);
        PyModule_AddIntConstant(m, "POLLPRI", POLLPRI);
        PyModule_AddIntConstant(m, "POLLOUT", POLLOUT);
        PyModule_AddIntConstant(m, "POLLERR", POLLERR);
        PyModule_AddIntConstant(m, "POLLHUP", POLLHUP);
        PyModule_AddIntConstant(m, "POLLNVAL", POLLNVAL);

#ifdef POLLRDNORM
        PyModule_AddIntConstant(m, "POLLRDNORM", POLLRDNORM);
#endif
#ifdef POLLRDBAND
        PyModule_AddIntConstant(m, "POLLRDBAND", POLLRDBAND);
#endif
#ifdef POLLWRNORM
        PyModule_AddIntConstant(m, "POLLWRNORM", POLLWRNORM);
#endif
#ifdef POLLWRBAND
        PyModule_AddIntConstant(m, "POLLWRBAND", POLLWRBAND);
#endif
#ifdef POLLMSG
        PyModule_AddIntConstant(m, "POLLMSG", POLLMSG);
#endif
    }
#endif /* HAVE_POLL */

#ifdef HAVE_EPOLL
    Py_TYPE(&pyEpoll_Type) = &PyType_Type;
    if (PyType_Ready(&pyEpoll_Type) < 0)
        return;

    Py_INCREF(&pyEpoll_Type);
    PyModule_AddObject(m, "epoll", (PyObject *) &pyEpoll_Type);

    PyModule_AddIntConstant(m, "EPOLLIN", EPOLLIN);
    PyModule_AddIntConstant(m, "EPOLLOUT", EPOLLOUT);
    PyModule_AddIntConstant(m, "EPOLLPRI", EPOLLPRI);
    PyModule_AddIntConstant(m, "EPOLLERR", EPOLLERR);
    PyModule_AddIntConstant(m, "EPOLLHUP", EPOLLHUP);
    PyModule_AddIntConstant(m, "EPOLLET", EPOLLET);
#ifdef EPOLLONESHOT
    /* Kernel 2.6.2+ */
    PyModule_AddIntConstant(m, "EPOLLONESHOT", EPOLLONESHOT);
#endif
    /* PyModule_AddIntConstant(m, "EPOLL_RDHUP", EPOLLRDHUP); */
#ifdef EPOLLRDNORM
    PyModule_AddIntConstant(m, "EPOLLRDNORM", EPOLLRDNORM);
#endif
#ifdef EPOLLRDBAND
    PyModule_AddIntConstant(m, "EPOLLRDBAND", EPOLLRDBAND);
#endif
#ifdef EPOLLWRNORM
    PyModule_AddIntConstant(m, "EPOLLWRNORM", EPOLLWRNORM);
#endif
#ifdef EPOLLWRBAND
    PyModule_AddIntConstant(m, "EPOLLWRBAND", EPOLLWRBAND);
#endif
#ifdef EPOLLMSG
    PyModule_AddIntConstant(m, "EPOLLMSG", EPOLLMSG);
#endif
#endif /* HAVE_EPOLL */

#ifdef HAVE_KQUEUE
    kqueue_event_Type.tp_new = PyType_GenericNew;
    Py_TYPE(&kqueue_event_Type) = &PyType_Type;
    if(PyType_Ready(&kqueue_event_Type) < 0)
        return;

    Py_INCREF(&kqueue_event_Type);
    PyModule_AddObject(m, "kevent", (PyObject *)&kqueue_event_Type);

    Py_TYPE(&kqueue_queue_Type) = &PyType_Type;
    if(PyType_Ready(&kqueue_queue_Type) < 0)
        return;
    Py_INCREF(&kqueue_queue_Type);
    PyModule_AddObject(m, "kqueue", (PyObject *)&kqueue_queue_Type);

    /* event filters */
    PyModule_AddIntConstant(m, "KQ_FILTER_READ", EVFILT_READ);
    PyModule_AddIntConstant(m, "KQ_FILTER_WRITE", EVFILT_WRITE);
    PyModule_AddIntConstant(m, "KQ_FILTER_AIO", EVFILT_AIO);
    PyModule_AddIntConstant(m, "KQ_FILTER_VNODE", EVFILT_VNODE);
    PyModule_AddIntConstant(m, "KQ_FILTER_PROC", EVFILT_PROC);
#ifdef EVFILT_NETDEV
    PyModule_AddIntConstant(m, "KQ_FILTER_NETDEV", EVFILT_NETDEV);
#endif
    PyModule_AddIntConstant(m, "KQ_FILTER_SIGNAL", EVFILT_SIGNAL);
    PyModule_AddIntConstant(m, "KQ_FILTER_TIMER", EVFILT_TIMER);

    /* event flags */
    PyModule_AddIntConstant(m, "KQ_EV_ADD", EV_ADD);
    PyModule_AddIntConstant(m, "KQ_EV_DELETE", EV_DELETE);
    PyModule_AddIntConstant(m, "KQ_EV_ENABLE", EV_ENABLE);
    PyModule_AddIntConstant(m, "KQ_EV_DISABLE", EV_DISABLE);
    PyModule_AddIntConstant(m, "KQ_EV_ONESHOT", EV_ONESHOT);
    PyModule_AddIntConstant(m, "KQ_EV_CLEAR", EV_CLEAR);

    PyModule_AddIntConstant(m, "KQ_EV_SYSFLAGS", EV_SYSFLAGS);
    PyModule_AddIntConstant(m, "KQ_EV_FLAG1", EV_FLAG1);

    PyModule_AddIntConstant(m, "KQ_EV_EOF", EV_EOF);
    PyModule_AddIntConstant(m, "KQ_EV_ERROR", EV_ERROR);

    /* READ WRITE filter flag */
    PyModule_AddIntConstant(m, "KQ_NOTE_LOWAT", NOTE_LOWAT);

    /* VNODE filter flags  */
    PyModule_AddIntConstant(m, "KQ_NOTE_DELETE", NOTE_DELETE);
    PyModule_AddIntConstant(m, "KQ_NOTE_WRITE", NOTE_WRITE);
    PyModule_AddIntConstant(m, "KQ_NOTE_EXTEND", NOTE_EXTEND);
    PyModule_AddIntConstant(m, "KQ_NOTE_ATTRIB", NOTE_ATTRIB);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINK", NOTE_LINK);
    PyModule_AddIntConstant(m, "KQ_NOTE_RENAME", NOTE_RENAME);
    PyModule_AddIntConstant(m, "KQ_NOTE_REVOKE", NOTE_REVOKE);

    /* PROC filter flags  */
    PyModule_AddIntConstant(m, "KQ_NOTE_EXIT", NOTE_EXIT);
    PyModule_AddIntConstant(m, "KQ_NOTE_FORK", NOTE_FORK);
    PyModule_AddIntConstant(m, "KQ_NOTE_EXEC", NOTE_EXEC);
    PyModule_AddIntConstant(m, "KQ_NOTE_PCTRLMASK", NOTE_PCTRLMASK);
    PyModule_AddIntConstant(m, "KQ_NOTE_PDATAMASK", NOTE_PDATAMASK);

    PyModule_AddIntConstant(m, "KQ_NOTE_TRACK", NOTE_TRACK);
    PyModule_AddIntConstant(m, "KQ_NOTE_CHILD", NOTE_CHILD);
    PyModule_AddIntConstant(m, "KQ_NOTE_TRACKERR", NOTE_TRACKERR);

    /* NETDEV filter flags */
#ifdef EVFILT_NETDEV
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKUP", NOTE_LINKUP);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKDOWN", NOTE_LINKDOWN);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKINV", NOTE_LINKINV);
#endif

#endif /* HAVE_KQUEUE */
}
