/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
*/

#if defined(HAVE_POLL_H) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "Python.h"
#include "structmember.h"         // PyMemberDef

#ifdef HAVE_SYS_DEVPOLL_H
#include <sys/resource.h>
#include <sys/devpoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

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

#ifdef MS_WINDOWS
#  define WIN32_LEAN_AND_MEAN
#  include <winsock.h>
#else
#  define SOCKET int
#endif

typedef struct {
    PyObject *close;
    PyTypeObject *poll_Type;
    PyTypeObject *devpoll_Type;
    PyTypeObject *pyEpoll_Type;
    PyTypeObject *kqueue_event_Type;
    PyTypeObject *kqueue_queue_Type;
} _selectstate;

static struct PyModuleDef selectmodule;

static inline _selectstate*
get_select_state(PyObject *module)
{
    void *state = PyModule_GetState(module);
    assert(state != NULL);
    return (_selectstate *)state;
}

#define _selectstate_by_type(type) get_select_state(PyType_GetModule(type))

/*[clinic input]
module select
class select.poll "pollObject *" "_selectstate_by_type(type)->poll_Type"
class select.devpoll "devpollObject *" "_selectstate_by_type(type)->devpoll_Type"
class select.epoll "pyEpoll_Object *" "_selectstate_by_type(type)->pyEpoll_Type"
class select.kqueue "kqueue_queue_Object *" "_selectstate_by_type(type)->kqueue_queue_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=8072de35824aa327]*/

/* list of Python objects and their file descriptor */
typedef struct {
    PyObject *obj;                           /* owned reference */
    SOCKET fd;
    int sentinel;                            /* -1 == sentinel */
} pylist;

static void
reap_obj(pylist fd2obj[FD_SETSIZE + 1])
{
    unsigned int i;
    for (i = 0; i < (unsigned int)FD_SETSIZE + 1 && fd2obj[i].sentinel >= 0; i++) {
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
    int max = -1;
    unsigned int index = 0;
    Py_ssize_t i;
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
            goto finally;

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
        if (index >= (unsigned int)FD_SETSIZE) {
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

/*[clinic input]
select.select

    rlist: object
    wlist: object
    xlist: object
    timeout as timeout_obj: object = None
    /

Wait until one or more file descriptors are ready for some kind of I/O.

The first three arguments are iterables of file descriptors to be waited for:
rlist -- wait until ready for reading
wlist -- wait until ready for writing
xlist -- wait for an "exceptional condition"
If only one kind of condition is required, pass [] for the other lists.

A file descriptor is either a socket or file object, or a small integer
gotten from a fileno() method call on one of those.

The optional 4th argument specifies a timeout in seconds; it may be
a floating point number to specify fractions of seconds.  If it is absent
or None, the call will never time out.

The return value is a tuple of three lists corresponding to the first three
arguments; each contains the subset of the corresponding file descriptors
that are ready.

*** IMPORTANT NOTICE ***
On Windows, only sockets are supported; on Unix, all file
descriptors can be used.
[clinic start generated code]*/

static PyObject *
select_select_impl(PyObject *module, PyObject *rlist, PyObject *wlist,
                   PyObject *xlist, PyObject *timeout_obj)
/*[clinic end generated code: output=2b3cfa824f7ae4cf input=e467f5d68033de00]*/
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
    PyObject *ret = NULL;
    fd_set ifdset, ofdset, efdset;
    struct timeval tv, *tvp;
    int imax, omax, emax, max;
    int n;
    _PyTime_t timeout, deadline = 0;

    if (timeout_obj == Py_None)
        tvp = (struct timeval *)NULL;
    else {
        if (_PyTime_FromSecondsObject(&timeout, timeout_obj,
                                      _PyTime_ROUND_TIMEOUT) < 0) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_SetString(PyExc_TypeError,
                                "timeout must be a float or None");
            }
            return NULL;
        }

        if (_PyTime_AsTimeval(timeout, &tv, _PyTime_ROUND_TIMEOUT) == -1)
            return NULL;
        if (tv.tv_sec < 0) {
            PyErr_SetString(PyExc_ValueError, "timeout must be non-negative");
            return NULL;
        }
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

    /* Convert iterables to fd_sets, and get maximum fd number
     * propagates the Python exception set in seq2set()
     */
    rfd2obj[0].sentinel = -1;
    wfd2obj[0].sentinel = -1;
    efd2obj[0].sentinel = -1;
    if ((imax = seq2set(rlist, &ifdset, rfd2obj)) < 0)
        goto finally;
    if ((omax = seq2set(wlist, &ofdset, wfd2obj)) < 0)
        goto finally;
    if ((emax = seq2set(xlist, &efdset, efd2obj)) < 0)
        goto finally;

    max = imax;
    if (omax > max) max = omax;
    if (emax > max) max = emax;

    if (tvp)
        deadline = _PyTime_GetMonotonicClock() + timeout;

    do {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        n = select(max, &ifdset, &ofdset, &efdset, tvp);
        Py_END_ALLOW_THREADS

        if (errno != EINTR)
            break;

        /* select() was interrupted by a signal */
        if (PyErr_CheckSignals())
            goto finally;

        if (tvp) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                /* bpo-35310: lists were unmodified -- clear them explicitly */
                FD_ZERO(&ifdset);
                FD_ZERO(&ofdset);
                FD_ZERO(&efdset);
                n = 0;
                break;
            }
            _PyTime_AsTimeval_noraise(timeout, &tv, _PyTime_ROUND_CEILING);
            /* retry select() with the recomputed timeout */
        }
    } while (1);

#ifdef MS_WINDOWS
    if (n == SOCKET_ERROR) {
        PyErr_SetExcFromWindowsErr(PyExc_OSError, WSAGetLastError());
    }
#else
    if (n < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
    }
#endif
    else {
        /* any of these three calls can raise an exception.  it's more
           convenient to test for this after all three calls... but
           is that acceptable?
        */
        rlist = set2list(&ifdset, rfd2obj);
        wlist = set2list(&ofdset, wfd2obj);
        xlist = set2list(&efdset, efd2obj);
        if (PyErr_Occurred())
            ret = NULL;
        else
            ret = PyTuple_Pack(3, rlist, wlist, xlist);

        Py_XDECREF(rlist);
        Py_XDECREF(wlist);
        Py_XDECREF(xlist);
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

/* Update the malloc'ed array of pollfds to match the dictionary
   contained within a pollObject.  Return 1 on success, 0 on an error.
*/

static int
update_ufd_array(pollObject *self)
{
    Py_ssize_t i, pos;
    PyObject *key, *value;
    struct pollfd *old_ufds = self->ufds;

    self->ufd_len = PyDict_GET_SIZE(self->dict);
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
        self->ufds[i].fd = (int)PyLong_AsLong(key);
        self->ufds[i].events = (short)(unsigned short)PyLong_AsLong(value);
        i++;
    }
    assert(i == self->ufd_len);
    self->ufd_uptodate = 1;
    return 1;
}

/*[clinic input]
select.poll.register

    fd: fildes
      either an integer, or an object with a fileno() method returning an int
    eventmask: unsigned_short(c_default="POLLIN | POLLPRI | POLLOUT") = select.POLLIN | select.POLLPRI | select.POLLOUT
      an optional bitmask describing the type of events to check for
    /

Register a file descriptor with the polling object.
[clinic start generated code]*/

static PyObject *
select_poll_register_impl(pollObject *self, int fd, unsigned short eventmask)
/*[clinic end generated code: output=0dc7173c800a4a65 input=34e16cfb28d3c900]*/
{
    PyObject *key, *value;
    int err;

    /* Add entry to the internal dictionary: the key is the
       file descriptor, and the value is the event mask. */
    key = PyLong_FromLong(fd);
    if (key == NULL)
        return NULL;
    value = PyLong_FromLong(eventmask);
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

    Py_RETURN_NONE;
}


/*[clinic input]
select.poll.modify

    fd: fildes
      either an integer, or an object with a fileno() method returning
      an int
    eventmask: unsigned_short
      a bitmask describing the type of events to check for
    /

Modify an already registered file descriptor.
[clinic start generated code]*/

static PyObject *
select_poll_modify_impl(pollObject *self, int fd, unsigned short eventmask)
/*[clinic end generated code: output=1a7b88bf079eff17 input=a8e383df075c32cf]*/
{
    PyObject *key, *value;
    int err;

    /* Modify registered fd */
    key = PyLong_FromLong(fd);
    if (key == NULL)
        return NULL;
    err = PyDict_Contains(self->dict, key);
    if (err < 0) {
        Py_DECREF(key);
        return NULL;
    }
    if (err == 0) {
        errno = ENOENT;
        PyErr_SetFromErrno(PyExc_OSError);
        Py_DECREF(key);
        return NULL;
    }
    value = PyLong_FromLong(eventmask);
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

    Py_RETURN_NONE;
}


/*[clinic input]
select.poll.unregister

    fd: fildes
    /

Remove a file descriptor being tracked by the polling object.
[clinic start generated code]*/

static PyObject *
select_poll_unregister_impl(pollObject *self, int fd)
/*[clinic end generated code: output=8c9f42e75e7d291b input=4b4fccc1040e79cb]*/
{
    PyObject *key;

    /* Check whether the fd is already in the array */
    key = PyLong_FromLong(fd);
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

    Py_RETURN_NONE;
}

/*[clinic input]
select.poll.poll

    timeout as timeout_obj: object = None
    /

Polls the set of registered file descriptors.

Returns a list containing any descriptors that have events or errors to
report, as a list of (fd, event) 2-tuples.
[clinic start generated code]*/

static PyObject *
select_poll_poll_impl(pollObject *self, PyObject *timeout_obj)
/*[clinic end generated code: output=876e837d193ed7e4 input=7a446ed45189e894]*/
{
    PyObject *result_list = NULL;
    int poll_result, i, j;
    PyObject *value = NULL, *num = NULL;
    _PyTime_t timeout = -1, ms = -1, deadline = 0;
    int async_err = 0;

    if (timeout_obj != Py_None) {
        if (_PyTime_FromMillisecondsObject(&timeout, timeout_obj,
                                           _PyTime_ROUND_TIMEOUT) < 0) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_SetString(PyExc_TypeError,
                                "timeout must be an integer or None");
            }
            return NULL;
        }

        ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_TIMEOUT);
        if (ms < INT_MIN || ms > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "timeout is too large");
            return NULL;
        }

        if (timeout >= 0) {
            deadline = _PyTime_GetMonotonicClock() + timeout;
        }
    }

    /* On some OSes, typically BSD-based ones, the timeout parameter of the
       poll() syscall, when negative, must be exactly INFTIM, where defined,
       or -1. See issue 31334. */
    if (ms < 0) {
#ifdef INFTIM
        ms = INFTIM;
#else
        ms = -1;
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
    async_err = 0;
    do {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        poll_result = poll(self->ufds, self->ufd_len, (int)ms);
        Py_END_ALLOW_THREADS

        if (errno != EINTR)
            break;

        /* poll() was interrupted by a signal */
        if (PyErr_CheckSignals()) {
            async_err = 1;
            break;
        }

        if (timeout >= 0) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                poll_result = 0;
                break;
            }
            ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_CEILING);
            /* retry poll() with the recomputed timeout */
        }
    } while (1);

    self->poll_running = 0;

    if (poll_result < 0) {
        if (!async_err)
            PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    /* build the result list */

    result_list = PyList_New(poll_result);
    if (!result_list)
        return NULL;

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
        num = PyLong_FromLong(self->ufds[i].fd);
        if (num == NULL) {
            Py_DECREF(value);
            goto error;
        }
        PyTuple_SET_ITEM(value, 0, num);

        /* The &0xffff is a workaround for AIX.  'revents'
           is a 16-bit short, and IBM assigned POLLNVAL
           to be 0x8000, so the conversion to int results
           in a negative number. See SF bug #923315. */
        num = PyLong_FromLong(self->ufds[i].revents & 0xffff);
        if (num == NULL) {
            Py_DECREF(value);
            goto error;
        }
        PyTuple_SET_ITEM(value, 1, num);
        PyList_SET_ITEM(result_list, j, value);
        i++;
    }
    return result_list;

  error:
    Py_DECREF(result_list);
    return NULL;
}

static pollObject *
newPollObject(PyObject *module)
{
    pollObject *self;
    self = PyObject_New(pollObject, get_select_state(module)->poll_Type);
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

static PyObject *
poll_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyErr_Format(PyExc_TypeError, "Cannot create '%.200s' instances", _PyType_Name(type));
    return NULL;
}

static void
poll_dealloc(pollObject *self)
{
    PyObject* type = (PyObject *)Py_TYPE(self);
    if (self->ufds != NULL)
        PyMem_DEL(self->ufds);
    Py_XDECREF(self->dict);
    PyObject_Del(self);
    Py_DECREF(type);
}


#ifdef HAVE_SYS_DEVPOLL_H
static PyMethodDef devpoll_methods[];

typedef struct {
    PyObject_HEAD
    int fd_devpoll;
    int max_n_fds;
    int n_fds;
    struct pollfd *fds;
} devpollObject;

static PyObject *
devpoll_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed devpoll object");
    return NULL;
}

static int devpoll_flush(devpollObject *self)
{
    int size, n;

    if (!self->n_fds) return 0;

    size = sizeof(struct pollfd)*self->n_fds;
    self->n_fds = 0;

    n = _Py_write(self->fd_devpoll, self->fds, size);
    if (n == -1)
        return -1;

    if (n < size) {
        /*
        ** Data writed to /dev/poll is a binary data structure. It is not
        ** clear what to do if a partial write occurred. For now, raise
        ** an exception and see if we actually found this problem in
        ** the wild.
        ** See http://bugs.python.org/issue6397.
        */
        PyErr_Format(PyExc_OSError, "failed to write all pollfds. "
                "Please, report at http://bugs.python.org/. "
                "Data to report: Size tried: %d, actual size written: %d.",
                size, n);
        return -1;
    }
    return 0;
}

static PyObject *
internal_devpoll_register(devpollObject *self, int fd,
                          unsigned short events, int remove)
{
    if (self->fd_devpoll < 0)
        return devpoll_err_closed();

    if (remove) {
        self->fds[self->n_fds].fd = fd;
        self->fds[self->n_fds].events = POLLREMOVE;

        if (++self->n_fds == self->max_n_fds) {
            if (devpoll_flush(self))
                return NULL;
        }
    }

    self->fds[self->n_fds].fd = fd;
    self->fds[self->n_fds].events = (signed short)events;

    if (++self->n_fds == self->max_n_fds) {
        if (devpoll_flush(self))
            return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
select.devpoll.register

    fd: fildes
        either an integer, or an object with a fileno() method returning
        an int
    eventmask: unsigned_short(c_default="POLLIN | POLLPRI | POLLOUT") = select.POLLIN | select.POLLPRI | select.POLLOUT
        an optional bitmask describing the type of events to check for
    /

Register a file descriptor with the polling object.
[clinic start generated code]*/

static PyObject *
select_devpoll_register_impl(devpollObject *self, int fd,
                             unsigned short eventmask)
/*[clinic end generated code: output=6e07fe8b74abba0c input=22006fabe9567522]*/
{
    return internal_devpoll_register(self, fd, eventmask, 0);
}

/*[clinic input]
select.devpoll.modify

    fd: fildes
        either an integer, or an object with a fileno() method returning
        an int
    eventmask: unsigned_short(c_default="POLLIN | POLLPRI | POLLOUT") = select.POLLIN | select.POLLPRI | select.POLLOUT
        an optional bitmask describing the type of events to check for
    /

Modify a possible already registered file descriptor.
[clinic start generated code]*/

static PyObject *
select_devpoll_modify_impl(devpollObject *self, int fd,
                           unsigned short eventmask)
/*[clinic end generated code: output=bc2e6d23aaff98b4 input=09fa335db7cdc09e]*/
{
    return internal_devpoll_register(self, fd, eventmask, 1);
}

/*[clinic input]
select.devpoll.unregister

    fd: fildes
    /

Remove a file descriptor being tracked by the polling object.
[clinic start generated code]*/

static PyObject *
select_devpoll_unregister_impl(devpollObject *self, int fd)
/*[clinic end generated code: output=95519ffa0c7d43fe input=b4ea42a4442fd467]*/
{
    if (self->fd_devpoll < 0)
        return devpoll_err_closed();

    self->fds[self->n_fds].fd = fd;
    self->fds[self->n_fds].events = POLLREMOVE;

    if (++self->n_fds == self->max_n_fds) {
        if (devpoll_flush(self))
            return NULL;
    }

    Py_RETURN_NONE;
}

/*[clinic input]
select.devpoll.poll
    timeout as timeout_obj: object = None
    /

Polls the set of registered file descriptors.

Returns a list containing any descriptors that have events or errors to
report, as a list of (fd, event) 2-tuples.
[clinic start generated code]*/

static PyObject *
select_devpoll_poll_impl(devpollObject *self, PyObject *timeout_obj)
/*[clinic end generated code: output=2654e5457cca0b3c input=fd0db698d84f0333]*/
{
    struct dvpoll dvp;
    PyObject *result_list = NULL;
    int poll_result, i;
    PyObject *value, *num1, *num2;
    _PyTime_t timeout, ms, deadline = 0;

    if (self->fd_devpoll < 0)
        return devpoll_err_closed();

    /* Check values for timeout */
    if (timeout_obj == Py_None) {
        timeout = -1;
        ms = -1;
    }
    else {
        if (_PyTime_FromMillisecondsObject(&timeout, timeout_obj,
                                           _PyTime_ROUND_TIMEOUT) < 0) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_SetString(PyExc_TypeError,
                                "timeout must be an integer or None");
            }
            return NULL;
        }

        ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_TIMEOUT);
        if (ms < -1 || ms > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "timeout is too large");
            return NULL;
        }
    }

    if (devpoll_flush(self))
        return NULL;

    dvp.dp_fds = self->fds;
    dvp.dp_nfds = self->max_n_fds;
    dvp.dp_timeout = (int)ms;

    if (timeout >= 0)
        deadline = _PyTime_GetMonotonicClock() + timeout;

    do {
        /* call devpoll() */
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        poll_result = ioctl(self->fd_devpoll, DP_POLL, &dvp);
        Py_END_ALLOW_THREADS

        if (errno != EINTR)
            break;

        /* devpoll() was interrupted by a signal */
        if (PyErr_CheckSignals())
            return NULL;

        if (timeout >= 0) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                poll_result = 0;
                break;
            }
            ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_CEILING);
            dvp.dp_timeout = (int)ms;
            /* retry devpoll() with the recomputed timeout */
        }
    } while (1);

    if (poll_result < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    /* build the result list */
    result_list = PyList_New(poll_result);
    if (!result_list)
        return NULL;

    for (i = 0; i < poll_result; i++) {
        num1 = PyLong_FromLong(self->fds[i].fd);
        num2 = PyLong_FromLong(self->fds[i].revents);
        if ((num1 == NULL) || (num2 == NULL)) {
            Py_XDECREF(num1);
            Py_XDECREF(num2);
            goto error;
        }
        value = PyTuple_Pack(2, num1, num2);
        Py_DECREF(num1);
        Py_DECREF(num2);
        if (value == NULL)
            goto error;
        PyList_SET_ITEM(result_list, i, value);
    }

    return result_list;

  error:
    Py_DECREF(result_list);
    return NULL;
}

static int
devpoll_internal_close(devpollObject *self)
{
    int save_errno = 0;
    if (self->fd_devpoll >= 0) {
        int fd = self->fd_devpoll;
        self->fd_devpoll = -1;
        Py_BEGIN_ALLOW_THREADS
        if (close(fd) < 0)
            save_errno = errno;
        Py_END_ALLOW_THREADS
    }
    return save_errno;
}

/*[clinic input]
select.devpoll.close

Close the devpoll file descriptor.

Further operations on the devpoll object will raise an exception.
[clinic start generated code]*/

static PyObject *
select_devpoll_close_impl(devpollObject *self)
/*[clinic end generated code: output=26b355bd6429f21b input=6273c30f5560a99b]*/
{
    errno = devpoll_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
devpoll_get_closed(devpollObject *self, void *Py_UNUSED(ignored))
{
    if (self->fd_devpoll < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

/*[clinic input]
select.devpoll.fileno

Return the file descriptor.
[clinic start generated code]*/

static PyObject *
select_devpoll_fileno_impl(devpollObject *self)
/*[clinic end generated code: output=26920929f8d292f4 input=ef15331ebde6c368]*/
{
    if (self->fd_devpoll < 0)
        return devpoll_err_closed();
    return PyLong_FromLong(self->fd_devpoll);
}

static PyGetSetDef devpoll_getsetlist[] = {
    {"closed", (getter)devpoll_get_closed, NULL,
     "True if the devpoll object is closed"},
    {0},
};

static devpollObject *
newDevPollObject(PyObject *module)
{
    devpollObject *self;
    int fd_devpoll, limit_result;
    struct pollfd *fds;
    struct rlimit limit;

    /*
    ** If we try to process more that getrlimit()
    ** fds, the kernel will give an error, so
    ** we set the limit here. It is a dynamic
    ** value, because we can change rlimit() anytime.
    */
    limit_result = getrlimit(RLIMIT_NOFILE, &limit);
    if (limit_result == -1) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    fd_devpoll = _Py_open("/dev/poll", O_RDWR);
    if (fd_devpoll == -1)
        return NULL;

    fds = PyMem_NEW(struct pollfd, limit.rlim_cur);
    if (fds == NULL) {
        close(fd_devpoll);
        PyErr_NoMemory();
        return NULL;
    }

    self = PyObject_New(devpollObject, get_select_state(module)->devpoll_Type);
    if (self == NULL) {
        close(fd_devpoll);
        PyMem_DEL(fds);
        return NULL;
    }
    self->fd_devpoll = fd_devpoll;
    self->max_n_fds = limit.rlim_cur;
    self->n_fds = 0;
    self->fds = fds;

    return self;
}

static PyObject *
devpoll_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    PyErr_Format(PyExc_TypeError, "Cannot create '%.200s' instances", _PyType_Name(type));
    return NULL;
}

static void
devpoll_dealloc(devpollObject *self)
{
    PyObject *type = (PyObject *)Py_TYPE(self);
    (void)devpoll_internal_close(self);
    PyMem_DEL(self->fds);
    PyObject_Del(self);
    Py_DECREF(type);
}

static PyType_Slot devpoll_Type_slots[] = {
    {Py_tp_dealloc, devpoll_dealloc},
    {Py_tp_getset, devpoll_getsetlist},
    {Py_tp_methods, devpoll_methods},
    {Py_tp_new, devpoll_new},
    {0, 0},
};

static PyType_Spec devpoll_Type_spec = {
    "select.devpoll",
    sizeof(devpollObject),
    0,
    Py_TPFLAGS_DEFAULT,
    devpoll_Type_slots
};

#endif  /* HAVE_SYS_DEVPOLL_H */


/*[clinic input]
select.poll

Returns a polling object.

This object supports registering and unregistering file descriptors, and then
polling them for I/O events.
[clinic start generated code]*/

static PyObject *
select_poll_impl(PyObject *module)
/*[clinic end generated code: output=16a665a4e1d228c5 input=3f877909d5696bbf]*/
{
    return (PyObject *)newPollObject(module);
}

#ifdef HAVE_SYS_DEVPOLL_H

/*[clinic input]
select.devpoll

Returns a polling object.

This object supports registering and unregistering file descriptors, and then
polling them for I/O events.
[clinic start generated code]*/

static PyObject *
select_devpoll_impl(PyObject *module)
/*[clinic end generated code: output=ea9213cc87fd9581 input=53a1af94564f00a3]*/
{
    return (PyObject *)newDevPollObject(module);
}
#endif


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

static PyObject *
pyepoll_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed epoll object");
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
    assert(type != NULL);
    allocfunc epoll_alloc = PyType_GetSlot(type, Py_tp_alloc);
    assert(epoll_alloc != NULL);
    self = (pyEpoll_Object *) epoll_alloc(type, 0);
    if (self == NULL)
        return NULL;

    if (fd == -1) {
        Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_EPOLL_CREATE1
        self->epfd = epoll_create1(EPOLL_CLOEXEC);
#else
        self->epfd = epoll_create(sizehint);
#endif
        Py_END_ALLOW_THREADS
    }
    else {
        self->epfd = fd;
    }
    if (self->epfd < 0) {
        Py_DECREF(self);
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

#ifndef HAVE_EPOLL_CREATE1
    if (fd == -1 && _Py_set_inheritable(self->epfd, 0, NULL) < 0) {
        Py_DECREF(self);
        return NULL;
    }
#endif

    return (PyObject *)self;
}


/*[clinic input]
@classmethod
select.epoll.__new__

    sizehint: int = -1
      The expected number of events to be registered.  It must be positive,
      or -1 to use the default.  It is only used on older systems where
      epoll_create1() is not available; otherwise it has no effect (though its
      value is still checked).
    flags: int = 0
      Deprecated and completely ignored.  However, when supplied, its value
      must be 0 or select.EPOLL_CLOEXEC, otherwise OSError is raised.

Returns an epolling object.
[clinic start generated code]*/

static PyObject *
select_epoll_impl(PyTypeObject *type, int sizehint, int flags)
/*[clinic end generated code: output=c87404e705013bb5 input=303e3295e7975e43]*/
{
    if (sizehint == -1) {
        sizehint = FD_SETSIZE - 1;
    }
    else if (sizehint <= 0) {
        PyErr_SetString(PyExc_ValueError, "negative sizehint");
        return NULL;
    }

#ifdef HAVE_EPOLL_CREATE1
    if (flags && flags != EPOLL_CLOEXEC) {
        PyErr_SetString(PyExc_OSError, "invalid flags");
        return NULL;
    }
#endif

    return newPyEpoll_Object(type, sizehint, -1);
}


static void
pyepoll_dealloc(pyEpoll_Object *self)
{
    PyTypeObject* type = Py_TYPE(self);
    (void)pyepoll_internal_close(self);
    freefunc epoll_free = PyType_GetSlot(type, Py_tp_free);
    epoll_free((PyObject *)self);
    Py_DECREF((PyObject *)type);
}

/*[clinic input]
select.epoll.close

Close the epoll control file descriptor.

Further operations on the epoll object will raise an exception.
[clinic start generated code]*/

static PyObject *
select_epoll_close_impl(pyEpoll_Object *self)
/*[clinic end generated code: output=ee2144c446a1a435 input=ca6c66ba5a736bfd]*/
{
    errno = pyepoll_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}


static PyObject*
pyepoll_get_closed(pyEpoll_Object *self, void *Py_UNUSED(ignored))
{
    if (self->epfd < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

/*[clinic input]
select.epoll.fileno

Return the epoll control file descriptor.
[clinic start generated code]*/

static PyObject *
select_epoll_fileno_impl(pyEpoll_Object *self)
/*[clinic end generated code: output=e171375fdc619ba3 input=c11091a6aee60b5c]*/
{
    if (self->epfd < 0)
        return pyepoll_err_closed();
    return PyLong_FromLong(self->epfd);
}


/*[clinic input]
@classmethod
select.epoll.fromfd

    fd: int
    /

Create an epoll object from a given control fd.
[clinic start generated code]*/

static PyObject *
select_epoll_fromfd_impl(PyTypeObject *type, int fd)
/*[clinic end generated code: output=c15de2a083524e8e input=faecefdb55e3046e]*/
{
    SOCKET s_fd = (SOCKET)fd;
    return newPyEpoll_Object(type, FD_SETSIZE - 1, s_fd);
}


static PyObject *
pyepoll_internal_ctl(int epfd, int op, int fd, unsigned int events)
{
    struct epoll_event ev;
    int result;

    if (epfd < 0)
        return pyepoll_err_closed();

    switch (op) {
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
        Py_END_ALLOW_THREADS
        break;
    default:
        result = -1;
        errno = EINVAL;
    }

    if (result < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}

/*[clinic input]
select.epoll.register

    fd: fildes
      the target file descriptor of the operation
    eventmask: unsigned_int(c_default="EPOLLIN | EPOLLPRI | EPOLLOUT", bitwise=True) = select.EPOLLIN | select.EPOLLPRI | select.EPOLLOUT
      a bit set composed of the various EPOLL constants

Registers a new fd or raises an OSError if the fd is already registered.

The epoll interface supports all file descriptors that support poll.
[clinic start generated code]*/

static PyObject *
select_epoll_register_impl(pyEpoll_Object *self, int fd,
                           unsigned int eventmask)
/*[clinic end generated code: output=318e5e6386520599 input=a5071b71edfe3578]*/
{
    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_ADD, fd, eventmask);
}

/*[clinic input]
select.epoll.modify

    fd: fildes
      the target file descriptor of the operation
    eventmask: unsigned_int(bitwise=True)
      a bit set composed of the various EPOLL constants

Modify event mask for a registered file descriptor.
[clinic start generated code]*/

static PyObject *
select_epoll_modify_impl(pyEpoll_Object *self, int fd,
                         unsigned int eventmask)
/*[clinic end generated code: output=7e3447307cff6f65 input=88a83dac53a8c3da]*/
{
    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_MOD, fd, eventmask);
}

/*[clinic input]
select.epoll.unregister

    fd: fildes
      the target file descriptor of the operation

Remove a registered file descriptor from the epoll object.
[clinic start generated code]*/

static PyObject *
select_epoll_unregister_impl(pyEpoll_Object *self, int fd)
/*[clinic end generated code: output=07c5dbd612a512d4 input=3093f68d3644743d]*/
{
    return pyepoll_internal_ctl(self->epfd, EPOLL_CTL_DEL, fd, 0);
}

/*[clinic input]
select.epoll.poll

    timeout as timeout_obj: object = None
      the maximum time to wait in seconds (as float);
      a timeout of None or -1 makes poll wait indefinitely
    maxevents: int = -1
      the maximum number of events returned; -1 means no limit

Wait for events on the epoll file descriptor.

Returns a list containing any descriptors that have events to report,
as a list of (fd, events) 2-tuples.
[clinic start generated code]*/

static PyObject *
select_epoll_poll_impl(pyEpoll_Object *self, PyObject *timeout_obj,
                       int maxevents)
/*[clinic end generated code: output=e02d121a20246c6c input=33d34a5ea430fd5b]*/
{
    int nfds, i;
    PyObject *elist = NULL, *etuple = NULL;
    struct epoll_event *evs = NULL;
    _PyTime_t timeout = -1, ms = -1, deadline = 0;

    if (self->epfd < 0)
        return pyepoll_err_closed();

    if (timeout_obj != Py_None) {
        /* epoll_wait() has a resolution of 1 millisecond, round towards
           infinity to wait at least timeout seconds. */
        if (_PyTime_FromSecondsObject(&timeout, timeout_obj,
                                      _PyTime_ROUND_TIMEOUT) < 0) {
            if (PyErr_ExceptionMatches(PyExc_TypeError)) {
                PyErr_SetString(PyExc_TypeError,
                                "timeout must be an integer or None");
            }
            return NULL;
        }

        ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_CEILING);
        if (ms < INT_MIN || ms > INT_MAX) {
            PyErr_SetString(PyExc_OverflowError, "timeout is too large");
            return NULL;
        }
        /* epoll_wait(2) treats all arbitrary negative numbers the same
           for the timeout argument, but -1 is the documented way to block
           indefinitely in the epoll_wait(2) documentation, so we set ms
           to -1 if the value of ms is a negative number.

           Note that we didn't use INFTIM here since it's non-standard and
           isn't available under Linux. */
        if (ms < 0) {
            ms = -1;
        }

        if (timeout >= 0) {
            deadline = _PyTime_GetMonotonicClock() + timeout;
        }
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
        PyErr_NoMemory();
        return NULL;
    }

    do {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        nfds = epoll_wait(self->epfd, evs, maxevents, (int)ms);
        Py_END_ALLOW_THREADS

        if (errno != EINTR)
            break;

        /* poll() was interrupted by a signal */
        if (PyErr_CheckSignals())
            goto error;

        if (timeout >= 0) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                nfds = 0;
                break;
            }
            ms = _PyTime_AsMilliseconds(timeout, _PyTime_ROUND_CEILING);
            /* retry epoll_wait() with the recomputed timeout */
        }
    } while(1);

    if (nfds < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
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


/*[clinic input]
select.epoll.__enter__

[clinic start generated code]*/

static PyObject *
select_epoll___enter___impl(pyEpoll_Object *self)
/*[clinic end generated code: output=ab45d433504db2a0 input=3c22568587efeadb]*/
{
    if (self->epfd < 0)
        return pyepoll_err_closed();

    Py_INCREF(self);
    return (PyObject *)self;
}

/*[clinic input]
select.epoll.__exit__

    exc_type:  object = None
    exc_value: object = None
    exc_tb:    object = None
    /

[clinic start generated code]*/

static PyObject *
select_epoll___exit___impl(pyEpoll_Object *self, PyObject *exc_type,
                           PyObject *exc_value, PyObject *exc_tb)
/*[clinic end generated code: output=c480f38ce361748e input=7ae81a5a4c1a98d8]*/
{
    _selectstate *state = _selectstate_by_type(Py_TYPE(self));
    return PyObject_CallMethodObjArgs((PyObject *)self, state->close, NULL);
}

static PyGetSetDef pyepoll_getsetlist[] = {
    {"closed", (getter)pyepoll_get_closed, NULL,
     "True if the epoll handler is closed"},
    {0},
};

PyDoc_STRVAR(pyepoll_doc,
"select.epoll(sizehint=-1, flags=0)\n\
\n\
Returns an epolling object\n\
\n\
sizehint must be a positive integer or -1 for the default size. The\n\
sizehint is used to optimize internal data structures. It doesn't limit\n\
the maximum number of monitored events.");

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

#define kqueue_event_Check(op, state) (PyObject_TypeCheck((op), state->kqueue_event_Type))

typedef struct {
    PyObject_HEAD
    SOCKET kqfd;                /* kqueue control fd */
} kqueue_queue_Object;

#if (SIZEOF_UINTPTR_T != SIZEOF_VOID_P)
#   error uintptr_t does not match void *!
#elif (SIZEOF_UINTPTR_T == SIZEOF_LONG_LONG)
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

#if SIZEOF_LONG_LONG == 8
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

#if SIZEOF_LONG_LONG == 4
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
        "<select.kevent ident=%zu filter=%d flags=0x%x fflags=0x%x "
        "data=0x%llx udata=%p>",
        (size_t)(s->e.ident), (int)s->e.filter, (unsigned int)s->e.flags,
        (unsigned int)s->e.fflags, (long long)(s->e.data), (void *)s->e.udata);
    return PyUnicode_FromString(buf);
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

    if (PyLong_Check(pfd)) {
        self->e.ident = PyLong_AsSize_t(pfd);
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
    _selectstate *state = _selectstate_by_type(Py_TYPE(s));

    if (!kqueue_event_Check(o, state)) {
        Py_RETURN_NOTIMPLEMENTED;
    }

#define CMP(a, b) ((a) != (b)) ? ((a) < (b) ? -1 : 1)
    result = CMP(s->e.ident, o->e.ident)
           : CMP(s->e.filter, o->e.filter)
           : CMP(s->e.flags, o->e.flags)
           : CMP(s->e.fflags, o->e.fflags)
           : CMP(s->e.data, o->e.data)
           : CMP((intptr_t)s->e.udata, (intptr_t)o->e.udata)
           : 0;
#undef CMP

    Py_RETURN_RICHCOMPARE(result, 0, op);
}

static PyType_Slot kqueue_event_Type_slots[] = {
    {Py_tp_doc, (void*)kqueue_event_doc},
    {Py_tp_init, kqueue_event_init},
    {Py_tp_members, kqueue_event_members},
    {Py_tp_new, PyType_GenericNew},
    {Py_tp_repr, kqueue_event_repr},
    {Py_tp_richcompare, kqueue_event_richcompare},
    {0, 0},
};

static PyType_Spec kqueue_event_Type_spec = {
    "select.kevent",
    sizeof(kqueue_event_Object),
    0,
    Py_TPFLAGS_DEFAULT,
    kqueue_event_Type_slots
};

static PyObject *
kqueue_queue_err_closed(void)
{
    PyErr_SetString(PyExc_ValueError, "I/O operation on closed kqueue object");
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
    assert(type != NULL);
    allocfunc queue_alloc = PyType_GetSlot(type, Py_tp_alloc);
    assert(queue_alloc != NULL);
    self = (kqueue_queue_Object *) queue_alloc(type, 0);
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
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }

    if (fd == -1) {
        if (_Py_set_inheritable(self->kqfd, 0, NULL) < 0) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject *)self;
}

/*[clinic input]
@classmethod
select.kqueue.__new__

Kqueue syscall wrapper.

For example, to start watching a socket for input:
>>> kq = kqueue()
>>> sock = socket()
>>> sock.connect((host, port))
>>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_ADD)], 0)

To wait one second for it to become writeable:
>>> kq.control(None, 1, 1000)

To stop listening:
>>> kq.control([kevent(sock, KQ_FILTER_WRITE, KQ_EV_DELETE)], 0)
[clinic start generated code]*/

static PyObject *
select_kqueue_impl(PyTypeObject *type)
/*[clinic end generated code: output=e0ff89f154d56236 input=cf625e49218366e8]*/
{
    return newKqueue_Object(type, -1);
}

static void
kqueue_queue_dealloc(kqueue_queue_Object *self)
{
    PyTypeObject* type = Py_TYPE(self);
    kqueue_queue_internal_close(self);
    freefunc kqueue_free = PyType_GetSlot(type, Py_tp_free);
    kqueue_free((PyObject *)self);
    Py_DECREF((PyObject *)type);
}

/*[clinic input]
select.kqueue.close

Close the kqueue control file descriptor.

Further operations on the kqueue object will raise an exception.
[clinic start generated code]*/

static PyObject *
select_kqueue_close_impl(kqueue_queue_Object *self)
/*[clinic end generated code: output=d1c7df0b407a4bc1 input=0b12d95430e0634c]*/
{
    errno = kqueue_queue_internal_close(self);
    if (errno < 0) {
        PyErr_SetFromErrno(PyExc_OSError);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject*
kqueue_queue_get_closed(kqueue_queue_Object *self, void *Py_UNUSED(ignored))
{
    if (self->kqfd < 0)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

/*[clinic input]
select.kqueue.fileno

Return the kqueue control file descriptor.
[clinic start generated code]*/

static PyObject *
select_kqueue_fileno_impl(kqueue_queue_Object *self)
/*[clinic end generated code: output=716f46112a4f6e5c input=41911c539ca2b0ca]*/
{
    if (self->kqfd < 0)
        return kqueue_queue_err_closed();
    return PyLong_FromLong(self->kqfd);
}

/*[clinic input]
@classmethod
select.kqueue.fromfd

    fd: int
    /

Create a kqueue object from a given control fd.
[clinic start generated code]*/

static PyObject *
select_kqueue_fromfd_impl(PyTypeObject *type, int fd)
/*[clinic end generated code: output=d02c3c7dc538a653 input=f6172a48ca4ecdd0]*/
{
    SOCKET s_fd = (SOCKET)fd;

    return newKqueue_Object(type, s_fd);
}

/*[clinic input]
select.kqueue.control

    changelist: object
        Must be an iterable of kevent objects describing the changes to be made
        to the kernel's watch list or None.
    maxevents: int
        The maximum number of events that the kernel will return.
    timeout as otimeout: object = None
        The maximum time to wait in seconds, or else None to wait forever.
        This accepts floats for smaller timeouts, too.
    /

Calls the kernel kevent function.
[clinic start generated code]*/

static PyObject *
select_kqueue_control_impl(kqueue_queue_Object *self, PyObject *changelist,
                           int maxevents, PyObject *otimeout)
/*[clinic end generated code: output=81324ff5130db7ae input=59c4e30811209c47]*/
{
    int gotevents = 0;
    int nchanges = 0;
    int i = 0;
    PyObject *seq = NULL, *ei = NULL;
    PyObject *result = NULL;
    struct kevent *evl = NULL;
    struct kevent *chl = NULL;
    struct timespec timeoutspec;
    struct timespec *ptimeoutspec;
    _PyTime_t timeout, deadline = 0;
    _selectstate *state = _selectstate_by_type(Py_TYPE(self));

    if (self->kqfd < 0)
        return kqueue_queue_err_closed();

    if (maxevents < 0) {
        PyErr_Format(PyExc_ValueError,
            "Length of eventlist must be 0 or positive, got %d",
            maxevents);
        return NULL;
    }

    if (otimeout == Py_None) {
        ptimeoutspec = NULL;
    }
    else {
        if (_PyTime_FromSecondsObject(&timeout,
                                      otimeout, _PyTime_ROUND_TIMEOUT) < 0) {
            PyErr_Format(PyExc_TypeError,
                "timeout argument must be a number "
                "or None, got %.200s",
                _PyType_Name(Py_TYPE(otimeout)));
            return NULL;
        }

        if (_PyTime_AsTimespec(timeout, &timeoutspec) == -1)
            return NULL;

        if (timeoutspec.tv_sec < 0) {
            PyErr_SetString(PyExc_ValueError,
                            "timeout must be positive or None");
            return NULL;
        }
        ptimeoutspec = &timeoutspec;
    }

    if (changelist != Py_None) {
        seq = PySequence_Fast(changelist, "changelist is not iterable");
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
        _selectstate *state = _selectstate_by_type(Py_TYPE(self));
        for (i = 0; i < nchanges; ++i) {
            ei = PySequence_Fast_GET_ITEM(seq, i);
            if (!kqueue_event_Check(ei, state)) {
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
    if (maxevents) {
        evl = PyMem_New(struct kevent, maxevents);
        if (evl == NULL) {
            PyErr_NoMemory();
            goto error;
        }
    }

    if (ptimeoutspec)
        deadline = _PyTime_GetMonotonicClock() + timeout;

    do {
        Py_BEGIN_ALLOW_THREADS
        errno = 0;
        gotevents = kevent(self->kqfd, chl, nchanges,
                           evl, maxevents, ptimeoutspec);
        Py_END_ALLOW_THREADS

        if (errno != EINTR)
            break;

        /* kevent() was interrupted by a signal */
        if (PyErr_CheckSignals())
            goto error;

        if (ptimeoutspec) {
            timeout = deadline - _PyTime_GetMonotonicClock();
            if (timeout < 0) {
                gotevents = 0;
                break;
            }
            if (_PyTime_AsTimespec(timeout, &timeoutspec) == -1)
                goto error;
            /* retry kevent() with the recomputed timeout */
        }
    } while (1);

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

        ch = PyObject_New(kqueue_event_Object, state->kqueue_event_Type);
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

static PyGetSetDef kqueue_queue_getsetlist[] = {
    {"closed", (getter)kqueue_queue_get_closed, NULL,
     "True if the kqueue handler is closed"},
    {0},
};

#endif /* HAVE_KQUEUE */


/* ************************************************************************ */

#include "clinic/selectmodule.c.h"

#if defined(HAVE_POLL) && !defined(HAVE_BROKEN_POLL)

static PyMethodDef poll_methods[] = {
    SELECT_POLL_REGISTER_METHODDEF
    SELECT_POLL_MODIFY_METHODDEF
    SELECT_POLL_UNREGISTER_METHODDEF
    SELECT_POLL_POLL_METHODDEF
    {NULL, NULL}           /* sentinel */
};


static PyType_Slot poll_Type_slots[] = {
    {Py_tp_dealloc, poll_dealloc},
    {Py_tp_methods, poll_methods},
    {Py_tp_new, poll_new},
    {0, 0},
};

static PyType_Spec poll_Type_spec = {
    "select.poll",
    sizeof(pollObject),
    0,
    Py_TPFLAGS_DEFAULT,
    poll_Type_slots
};

#ifdef HAVE_SYS_DEVPOLL_H

static PyMethodDef devpoll_methods[] = {
    SELECT_DEVPOLL_REGISTER_METHODDEF
    SELECT_DEVPOLL_MODIFY_METHODDEF
    SELECT_DEVPOLL_UNREGISTER_METHODDEF
    SELECT_DEVPOLL_POLL_METHODDEF
    SELECT_DEVPOLL_CLOSE_METHODDEF
    SELECT_DEVPOLL_FILENO_METHODDEF
    {NULL,              NULL}           /* sentinel */
};

#endif  /* HAVE_SYS_DEVPOLL_H */

#endif /* HAVE_POLL */

#ifdef HAVE_EPOLL

static PyMethodDef pyepoll_methods[] = {
    SELECT_EPOLL_FROMFD_METHODDEF
    SELECT_EPOLL_CLOSE_METHODDEF
    SELECT_EPOLL_FILENO_METHODDEF
    SELECT_EPOLL_MODIFY_METHODDEF
    SELECT_EPOLL_REGISTER_METHODDEF
    SELECT_EPOLL_UNREGISTER_METHODDEF
    SELECT_EPOLL_POLL_METHODDEF
    SELECT_EPOLL___ENTER___METHODDEF
    SELECT_EPOLL___EXIT___METHODDEF
    {NULL,      NULL},
};

static PyType_Slot pyEpoll_Type_slots[] = {
    {Py_tp_dealloc, pyepoll_dealloc},
    {Py_tp_doc, (void*)pyepoll_doc},
    {Py_tp_getattro, PyObject_GenericGetAttr},
    {Py_tp_getset, pyepoll_getsetlist},
    {Py_tp_methods, pyepoll_methods},
    {Py_tp_new, select_epoll},
    {0, 0},
};

static PyType_Spec pyEpoll_Type_spec = {
    "select.epoll",
    sizeof(pyEpoll_Object),
    0,
    Py_TPFLAGS_DEFAULT,
    pyEpoll_Type_slots
};

#endif /* HAVE_EPOLL */

#ifdef HAVE_KQUEUE

static PyMethodDef kqueue_queue_methods[] = {
    SELECT_KQUEUE_FROMFD_METHODDEF
    SELECT_KQUEUE_CLOSE_METHODDEF
    SELECT_KQUEUE_FILENO_METHODDEF
    SELECT_KQUEUE_CONTROL_METHODDEF
    {NULL,      NULL},
};

static PyType_Slot kqueue_queue_Type_slots[] = {
    {Py_tp_dealloc, kqueue_queue_dealloc},
    {Py_tp_doc, (void*)select_kqueue__doc__},
    {Py_tp_getset, kqueue_queue_getsetlist},
    {Py_tp_methods, kqueue_queue_methods},
    {Py_tp_new, select_kqueue},
    {0, 0},
};

static PyType_Spec kqueue_queue_Type_spec = {
    "select.kqueue",
    sizeof(kqueue_queue_Object),
    0,
    Py_TPFLAGS_DEFAULT,
    kqueue_queue_Type_slots
};

#endif /* HAVE_KQUEUE */





/* ************************************************************************ */


static PyMethodDef select_methods[] = {
    SELECT_SELECT_METHODDEF
    SELECT_POLL_METHODDEF
    SELECT_DEVPOLL_METHODDEF
    {0,         0},     /* sentinel */
};

PyDoc_STRVAR(module_doc,
"This module supports asynchronous I/O on multiple file descriptors.\n\
\n\
*** IMPORTANT NOTICE ***\n\
On Windows, only sockets are supported; on Unix, all file descriptors.");



static int
_select_traverse(PyObject *module, visitproc visit, void *arg)
{
    _selectstate *state = get_select_state(module);

    Py_VISIT(state->close);
    Py_VISIT(state->poll_Type);
    Py_VISIT(state->devpoll_Type);
    Py_VISIT(state->pyEpoll_Type);
    Py_VISIT(state->kqueue_event_Type);
    Py_VISIT(state->kqueue_queue_Type);
    return 0;
}

static int
_select_clear(PyObject *module)
{
    _selectstate *state = get_select_state(module);

    Py_CLEAR(state->close);
    Py_CLEAR(state->poll_Type);
    Py_CLEAR(state->devpoll_Type);
    Py_CLEAR(state->pyEpoll_Type);
    Py_CLEAR(state->kqueue_event_Type);
    Py_CLEAR(state->kqueue_queue_Type);
    return 0;
}

static void
_select_free(void *module)
{
    _select_clear((PyObject *)module);
}

int
_select_exec(PyObject *m)
{
    _selectstate *state = get_select_state(m);

    state->close = PyUnicode_InternFromString("close");
    if (state->close == NULL) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "error", PyExc_OSError) < 0) {
        return -1;
    }

#ifdef PIPE_BUF
#ifdef HAVE_BROKEN_PIPE_BUF
#undef PIPE_BUF
#define PIPE_BUF 512
#endif
    PyModule_AddIntMacro(m, PIPE_BUF);
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
        state->poll_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
            m, &poll_Type_spec, NULL);
        if (state->poll_Type == NULL) {
            return -1;
        }

        PyModule_AddIntMacro(m, POLLIN);
        PyModule_AddIntMacro(m, POLLPRI);
        PyModule_AddIntMacro(m, POLLOUT);
        PyModule_AddIntMacro(m, POLLERR);
        PyModule_AddIntMacro(m, POLLHUP);
        PyModule_AddIntMacro(m, POLLNVAL);

#ifdef POLLRDNORM
        PyModule_AddIntMacro(m, POLLRDNORM);
#endif
#ifdef POLLRDBAND
        PyModule_AddIntMacro(m, POLLRDBAND);
#endif
#ifdef POLLWRNORM
        PyModule_AddIntMacro(m, POLLWRNORM);
#endif
#ifdef POLLWRBAND
        PyModule_AddIntMacro(m, POLLWRBAND);
#endif
#ifdef POLLMSG
        PyModule_AddIntMacro(m, POLLMSG);
#endif
#ifdef POLLRDHUP
        /* Kernel 2.6.17+ */
        PyModule_AddIntMacro(m, POLLRDHUP);
#endif
    }
#endif /* HAVE_POLL */

#ifdef HAVE_SYS_DEVPOLL_H
    state->devpoll_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &devpoll_Type_spec, NULL);
    if (state->devpoll_Type == NULL) {
        return -1;
    }
#endif

#ifdef HAVE_EPOLL
    state->pyEpoll_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &pyEpoll_Type_spec, NULL);
    if (state->pyEpoll_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->pyEpoll_Type) < 0) {
        return -1;
    }

    PyModule_AddIntMacro(m, EPOLLIN);
    PyModule_AddIntMacro(m, EPOLLOUT);
    PyModule_AddIntMacro(m, EPOLLPRI);
    PyModule_AddIntMacro(m, EPOLLERR);
    PyModule_AddIntMacro(m, EPOLLHUP);
#ifdef EPOLLRDHUP
    /* Kernel 2.6.17 */
    PyModule_AddIntMacro(m, EPOLLRDHUP);
#endif
    PyModule_AddIntMacro(m, EPOLLET);
#ifdef EPOLLONESHOT
    /* Kernel 2.6.2+ */
    PyModule_AddIntMacro(m, EPOLLONESHOT);
#endif
#ifdef EPOLLEXCLUSIVE
    PyModule_AddIntMacro(m, EPOLLEXCLUSIVE);
#endif

#ifdef EPOLLRDNORM
    PyModule_AddIntMacro(m, EPOLLRDNORM);
#endif
#ifdef EPOLLRDBAND
    PyModule_AddIntMacro(m, EPOLLRDBAND);
#endif
#ifdef EPOLLWRNORM
    PyModule_AddIntMacro(m, EPOLLWRNORM);
#endif
#ifdef EPOLLWRBAND
    PyModule_AddIntMacro(m, EPOLLWRBAND);
#endif
#ifdef EPOLLMSG
    PyModule_AddIntMacro(m, EPOLLMSG);
#endif

#ifdef EPOLL_CLOEXEC
    PyModule_AddIntMacro(m, EPOLL_CLOEXEC);
#endif
#endif /* HAVE_EPOLL */

#ifdef HAVE_KQUEUE
    state->kqueue_event_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &kqueue_event_Type_spec, NULL);
    if (state->kqueue_event_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->kqueue_event_Type) < 0) {
        return -1;
    }

    state->kqueue_queue_Type = (PyTypeObject *)PyType_FromModuleAndSpec(
        m, &kqueue_queue_Type_spec, NULL);
    if (state->kqueue_queue_Type == NULL) {
        return -1;
    }
    if (PyModule_AddType(m, state->kqueue_queue_Type) < 0) {
        return -1;
    }

    /* event filters */
    PyModule_AddIntConstant(m, "KQ_FILTER_READ", EVFILT_READ);
    PyModule_AddIntConstant(m, "KQ_FILTER_WRITE", EVFILT_WRITE);
#ifdef EVFILT_AIO
    PyModule_AddIntConstant(m, "KQ_FILTER_AIO", EVFILT_AIO);
#endif
#ifdef EVFILT_VNODE
    PyModule_AddIntConstant(m, "KQ_FILTER_VNODE", EVFILT_VNODE);
#endif
#ifdef EVFILT_PROC
    PyModule_AddIntConstant(m, "KQ_FILTER_PROC", EVFILT_PROC);
#endif
#ifdef EVFILT_NETDEV
    PyModule_AddIntConstant(m, "KQ_FILTER_NETDEV", EVFILT_NETDEV);
#endif
#ifdef EVFILT_SIGNAL
    PyModule_AddIntConstant(m, "KQ_FILTER_SIGNAL", EVFILT_SIGNAL);
#endif
    PyModule_AddIntConstant(m, "KQ_FILTER_TIMER", EVFILT_TIMER);

    /* event flags */
    PyModule_AddIntConstant(m, "KQ_EV_ADD", EV_ADD);
    PyModule_AddIntConstant(m, "KQ_EV_DELETE", EV_DELETE);
    PyModule_AddIntConstant(m, "KQ_EV_ENABLE", EV_ENABLE);
    PyModule_AddIntConstant(m, "KQ_EV_DISABLE", EV_DISABLE);
    PyModule_AddIntConstant(m, "KQ_EV_ONESHOT", EV_ONESHOT);
    PyModule_AddIntConstant(m, "KQ_EV_CLEAR", EV_CLEAR);

#ifdef EV_SYSFLAGS
    PyModule_AddIntConstant(m, "KQ_EV_SYSFLAGS", EV_SYSFLAGS);
#endif
#ifdef EV_FLAG1
    PyModule_AddIntConstant(m, "KQ_EV_FLAG1", EV_FLAG1);
#endif

    PyModule_AddIntConstant(m, "KQ_EV_EOF", EV_EOF);
    PyModule_AddIntConstant(m, "KQ_EV_ERROR", EV_ERROR);

    /* READ WRITE filter flag */
#ifdef NOTE_LOWAT
    PyModule_AddIntConstant(m, "KQ_NOTE_LOWAT", NOTE_LOWAT);
#endif

    /* VNODE filter flags  */
#ifdef EVFILT_VNODE
    PyModule_AddIntConstant(m, "KQ_NOTE_DELETE", NOTE_DELETE);
    PyModule_AddIntConstant(m, "KQ_NOTE_WRITE", NOTE_WRITE);
    PyModule_AddIntConstant(m, "KQ_NOTE_EXTEND", NOTE_EXTEND);
    PyModule_AddIntConstant(m, "KQ_NOTE_ATTRIB", NOTE_ATTRIB);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINK", NOTE_LINK);
    PyModule_AddIntConstant(m, "KQ_NOTE_RENAME", NOTE_RENAME);
    PyModule_AddIntConstant(m, "KQ_NOTE_REVOKE", NOTE_REVOKE);
#endif

    /* PROC filter flags  */
#ifdef EVFILT_PROC
    PyModule_AddIntConstant(m, "KQ_NOTE_EXIT", NOTE_EXIT);
    PyModule_AddIntConstant(m, "KQ_NOTE_FORK", NOTE_FORK);
    PyModule_AddIntConstant(m, "KQ_NOTE_EXEC", NOTE_EXEC);
    PyModule_AddIntConstant(m, "KQ_NOTE_PCTRLMASK", NOTE_PCTRLMASK);
    PyModule_AddIntConstant(m, "KQ_NOTE_PDATAMASK", NOTE_PDATAMASK);

    PyModule_AddIntConstant(m, "KQ_NOTE_TRACK", NOTE_TRACK);
    PyModule_AddIntConstant(m, "KQ_NOTE_CHILD", NOTE_CHILD);
    PyModule_AddIntConstant(m, "KQ_NOTE_TRACKERR", NOTE_TRACKERR);
#endif

    /* NETDEV filter flags */
#ifdef EVFILT_NETDEV
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKUP", NOTE_LINKUP);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKDOWN", NOTE_LINKDOWN);
    PyModule_AddIntConstant(m, "KQ_NOTE_LINKINV", NOTE_LINKINV);
#endif

#endif /* HAVE_KQUEUE */
    return 0;
}

static PyModuleDef_Slot _select_slots[] = {
    {Py_mod_exec, _select_exec},
    {0, NULL}
};

static struct PyModuleDef selectmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "select",
    .m_doc = module_doc,
    .m_size = sizeof(_selectstate),
    .m_methods = select_methods,
    .m_slots = _select_slots,
    .m_traverse = _select_traverse,
    .m_clear = _select_clear,
    .m_free = _select_free,
};

PyMODINIT_FUNC
PyInit_select(void)
{
    return PyModuleDef_Init(&selectmodule);
}
