/* select - Module containing unix select(2) call.
   Under Unix, the file descriptors are small integers.
   Under Win32, select only exists for sockets, and sockets may
   have any value except INVALID_SOCKET.
*/

#include "Python.h"

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
#  define WIN32_LEAN_AND_MEAN
#  include <winsock.h>
#else
#  define SOCKET int
#  if defined(__VMS)
#    include <socket.h>
#  endif
#endif


static PyObject *SelectError;

/* list of Python objects and their file descriptor */
typedef struct {
	PyObject *obj;			     /* owned reference */
	SOCKET fd;
	int sentinel;			     /* -1 == sentinel */
} pylist;

static void
reap_obj(pylist fd2obj[FD_SETSIZE + 1])
{
	int i;
	for (i = 0; i < FD_SETSIZE + 1 && fd2obj[i].sentinel >= 0; i++) {
		Py_XDECREF(fd2obj[i].obj);
		fd2obj[i].obj = NULL;
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
        int len = -1;
        PyObject* fast_seq = NULL;
	PyObject* o = NULL;

	fd2obj[0].obj = (PyObject*)0;	     /* set list to zero size */
	FD_ZERO(set);

        fast_seq=PySequence_Fast(seq, "arguments 1-3 must be sequences");
        if (!fast_seq)
            return -1;

        len = PySequence_Fast_GET_SIZE(fast_seq);

	for (i = 0; i < len; i++)  {
		SOCKET v;

		/* any intervening fileno() calls could decr this refcnt */
		if (!(o = PySequence_Fast_GET_ITEM(fast_seq, i)))
                    return -1;

		Py_INCREF(o);
		v = PyObject_AsFileDescriptor( o );
		if (v == -1) goto finally;

#if defined(_MSC_VER)
		max = 0;		     /* not used for Win32 */
#else  /* !_MSC_VER */
		if (v < 0 || v >= FD_SETSIZE) {
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
#ifndef _MSC_VER
			if (fd > FD_SETSIZE) {
				PyErr_SetString(PyExc_SystemError,
			   "filedescriptor out of range returned in select()");
				goto finally;
			}
#endif
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
		tv.tv_usec = (long)(timeout*1000000.0);
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
	else if (n == 0) {
                /* optimization */
		ifdlist = PyList_New(0);
		if (ifdlist) {
			ret = PyTuple_Pack(3, ifdlist, ifdlist, ifdlist);
			Py_DECREF(ifdlist);
		}
	}
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

		Py_DECREF(ifdlist);
		Py_DECREF(ofdlist);
		Py_DECREF(efdlist);
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

	self->ufd_len = PyDict_Size(self->dict);
	PyMem_Resize(self->ufds, struct pollfd, self->ufd_len);
	if (self->ufds == NULL) {
		PyErr_NoMemory();
		return 0;
	}

	i = pos = 0;
	while (PyDict_Next(self->dict, &pos, &key, &value)) {
		self->ufds[i].fd = PyLong_AsLong(key);
		self->ufds[i].events = (short)PyLong_AsLong(value);
		i++;
	}
	self->ufd_uptodate = 1;
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
	int fd, events = POLLIN | POLLPRI | POLLOUT;
	int err;

	if (!PyArg_ParseTuple(args, "O|i:register", &o, &events)) {
		return NULL;
	}
  
	fd = PyObject_AsFileDescriptor(o);
	if (fd == -1) return NULL;

	/* Add entry to the internal dictionary: the key is the 
	   file descriptor, and the value is the event mask. */
	key = PyLong_FromLong(fd);
	if (key == NULL)
		return NULL;
	value = PyLong_FromLong(events);
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
		timeout = PyLong_AsLong(tout);
		Py_DECREF(tout);
		if (timeout == -1 && PyErr_Occurred())
			return NULL;
	}

	/* Ensure the ufd array is up to date */
	if (!self->ufd_uptodate) 
		if (update_ufd_array(self) == 0)
			return NULL;

	/* call poll() */
	Py_BEGIN_ALLOW_THREADS;
	poll_result = poll(self->ufds, self->ufd_len, timeout);
	Py_END_ALLOW_THREADS;
 
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
 			if ((PyList_SetItem(result_list, j, value)) == -1) {
				Py_DECREF(value);
				goto error;
 			}
 			i++;
 		}
 	}
 	return result_list;

  error:
	Py_DECREF(result_list);
	return NULL;
}

static PyMethodDef poll_methods[] = {
	{"register",	(PyCFunction)poll_register,	
	 METH_VARARGS,  poll_register_doc},
	{"unregister",	(PyCFunction)poll_unregister,	
	 METH_O,        poll_unregister_doc},
	{"poll",	(PyCFunction)poll_poll,	
	 METH_VARARGS,  poll_poll_doc},
	{NULL,		NULL}		/* sentinel */
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
	"select.poll",		/*tp_name*/
	sizeof(pollObject),	/*tp_basicsize*/
	0,			/*tp_itemsize*/
	/* methods */
	(destructor)poll_dealloc, /*tp_dealloc*/
	0,			/*tp_print*/
	(getattrfunc)poll_getattr, /*tp_getattr*/
	0,                      /*tp_setattr*/
	0,			/*tp_compare*/
	0,			/*tp_repr*/
	0,			/*tp_as_number*/
	0,			/*tp_as_sequence*/
	0,			/*tp_as_mapping*/
	0,			/*tp_hash*/
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
On Windows and OpenVMS, only sockets are supported; on Unix, all file descriptors.");

static PyMethodDef select_methods[] = {
    {"select",	select_select, METH_VARARGS, select_doc},
#if defined(HAVE_POLL) 
    {"poll",    select_poll,   METH_NOARGS, poll_doc},
#endif /* HAVE_POLL */
    {0,  	0},			     /* sentinel */
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
#if defined(HAVE_POLL) 

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
}
