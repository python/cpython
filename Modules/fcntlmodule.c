
/* fcntl module */

#include "Python.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>


/* fcntl(fd, opt, [arg]) */

static PyObject *
fcntl_fcntl(PyObject *self, PyObject *args)
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (PyArg_ParseTuple(args, "iis#:fcntl", &fd, &code, &str, &len)) {
		if (len > sizeof buf) {
			PyErr_SetString(PyExc_ValueError,
					"fcntl string arg too long");
			return NULL;
		}
		memcpy(buf, str, len);
		Py_BEGIN_ALLOW_THREADS
		ret = fcntl(fd, code, buf);
		Py_END_ALLOW_THREADS
		if (ret < 0) {
			PyErr_SetFromErrno(PyExc_IOError);
			return NULL;
		}
		return PyString_FromStringAndSize(buf, len);
	}

	PyErr_Clear();
	arg = 0;
	if (!PyArg_ParseTuple(args, "ii|i;fcntl requires 2 integers and optionally a third integer or a string", 
			      &fd, &code, &arg)) {
	  return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	ret = fcntl(fd, code, arg);
	Py_END_ALLOW_THREADS
	if (ret < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	return PyInt_FromLong((long)ret);
}

static char fcntl_doc [] =

"fcntl(fd, opt, [arg])\n\
\n\
Perform the requested operation on file descriptor fd.  The operation\n\
is defined by op and is operating system dependent.  Typically these\n\
codes can be retrieved from the library module FCNTL.  The argument arg\n\
is optional, and defaults to 0; it may be an int or a string. If arg is\n\
given as a string, the return value of fcntl is a string of that length,\n\
containing the resulting value put in the arg buffer by the operating system.\n\
The length of the arg string is not allowed to exceed 1024 bytes. If the arg\n\
given is an integer or if none is specified, the result value is an integer\n\
corresponding to the return value of the fcntl call in the C code.";


/* ioctl(fd, opt, [arg]) */

static PyObject *
fcntl_ioctl(PyObject *self, PyObject *args)
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (PyArg_ParseTuple(args, "iis#:ioctl", &fd, &code, &str, &len)) {
		if (len > sizeof buf) {
			PyErr_SetString(PyExc_ValueError,
					"ioctl string arg too long");
			return NULL;
		}
		memcpy(buf, str, len);
		Py_BEGIN_ALLOW_THREADS
		ret = ioctl(fd, code, buf);
		Py_END_ALLOW_THREADS
		if (ret < 0) {
			PyErr_SetFromErrno(PyExc_IOError);
			return NULL;
		}
		return PyString_FromStringAndSize(buf, len);
	}

	PyErr_Clear();
	arg = 0;
	if (!PyArg_ParseTuple(args, "ii|i;ioctl requires 2 integers and optionally a third integer or a string", 
			      &fd, &code, &arg)) {
	  return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
	ret = ioctl(fd, code, arg);
	Py_END_ALLOW_THREADS
	if (ret < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	return PyInt_FromLong((long)ret);
}

static char ioctl_doc [] =
"ioctl(fd, opt, [arg])\n\
\n\
Perform the requested operation on file descriptor fd.  The operation\n\
is defined by op and is operating system dependent.  Typically these\n\
codes can be retrieved from the library module IOCTL.  The argument arg\n\
is optional, and defaults to 0; it may be an int or a string. If arg is\n\
given as a string, the return value of ioctl is a string of that length,\n\
containing the resulting value put in the arg buffer by the operating system.\n\
The length of the arg string is not allowed to exceed 1024 bytes. If the arg\n\
given is an integer or if none is specified, the result value is an integer\n\
corresponding to the return value of the ioctl call in the C code.";


/* flock(fd, operation) */

static PyObject *
fcntl_flock(PyObject *self, PyObject *args)
{
	int fd;
	int code;
	int ret;

	if (!PyArg_ParseTuple(args, "ii:flock", &fd, &code))
		return NULL;

#ifdef HAVE_FLOCK
	Py_BEGIN_ALLOW_THREADS
	ret = flock(fd, code);
	Py_END_ALLOW_THREADS
#else

#ifndef LOCK_SH
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* don't block when locking */
#define LOCK_UN		8	/* unlock */
#endif
	{
		struct flock l;
		if (code == LOCK_UN)
			l.l_type = F_UNLCK;
		else if (code & LOCK_SH)
			l.l_type = F_RDLCK;
		else if (code & LOCK_EX)
			l.l_type = F_WRLCK;
		else {
			PyErr_SetString(PyExc_ValueError,
					"unrecognized flock argument");
			return NULL;
		}
		l.l_whence = l.l_start = l.l_len = 0;
		Py_BEGIN_ALLOW_THREADS
		ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
		Py_END_ALLOW_THREADS
	}
#endif /* HAVE_FLOCK */
	if (ret < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char flock_doc [] =
"flock(fd, operation)\n\
\n\
Perform the lock operation op on file descriptor fd.  See the Unix \n\
manual flock(3) for details.  (On some systems, this function is\n\
emulated using fcntl().)";


/* lockf(fd, operation) */
static PyObject *
fcntl_lockf(PyObject *self, PyObject *args)
{
	int fd, code, ret, whence = 0;
	PyObject *lenobj = NULL, *startobj = NULL;

	if (!PyArg_ParseTuple(args, "ii|OOi:lockf", &fd, &code,
			      &lenobj, &startobj, &whence))
	    return NULL;

#ifndef LOCK_SH
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* don't block when locking */
#define LOCK_UN		8	/* unlock */
#endif
	{
		struct flock l;
		if (code == LOCK_UN)
			l.l_type = F_UNLCK;
		else if (code & LOCK_SH)
			l.l_type = F_RDLCK;
		else if (code & LOCK_EX)
			l.l_type = F_WRLCK;
		else {
			PyErr_SetString(PyExc_ValueError,
					"unrecognized flock argument");
			return NULL;
		}
		l.l_start = l.l_len = 0;
		if (startobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
			l.l_start = PyInt_AsLong(startobj);
#else
			l.l_start = PyLong_Check(startobj) ?
					PyLong_AsLongLong(startobj) :
					PyInt_AsLong(startobj);
#endif
			if (PyErr_Occurred())
				return NULL;
		}
		if (lenobj != NULL) {
#if !defined(HAVE_LARGEFILE_SUPPORT)
			l.l_len = PyInt_AsLong(lenobj);
#else
			l.l_len = PyLong_Check(lenobj) ?
					PyLong_AsLongLong(lenobj) :
					PyInt_AsLong(lenobj);
#endif
			if (PyErr_Occurred())
				return NULL;
		}
		l.l_whence = whence;
		Py_BEGIN_ALLOW_THREADS
		ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
		Py_END_ALLOW_THREADS
	}
	if (ret < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

static char lockf_doc [] =
"lockf (fd, operation)\n\
\n\
This is a wrapper around the FCNTL.F_SETLK and FCNTL.F_SETLKW fcntl()\n\
calls.  See the Unix manual for details.";

/* List of functions */

static PyMethodDef fcntl_methods[] = {
	{"fcntl",	fcntl_fcntl, METH_VARARGS, fcntl_doc},
	{"ioctl",	fcntl_ioctl, METH_VARARGS, ioctl_doc},
	{"flock",	fcntl_flock, METH_VARARGS, flock_doc},
	{"lockf",       fcntl_lockf, METH_VARARGS, lockf_doc},
	{NULL,		NULL}		/* sentinel */
};


static char module_doc [] =

"This module performs file control and I/O control on file \n\
descriptors.  It is an interface to the fcntl() and ioctl() Unix\n\
routines.  File descriptors can be obtained with the fileno() method of\n\
a file or socket object.";

/* Module initialisation */

static int
ins(PyObject* d, char* symbol, long value)
{
        PyObject* v = PyInt_FromLong(value);
        if (!v || PyDict_SetItemString(d, symbol, v) < 0)
                return -1;

        Py_DECREF(v);
        return 0;
}

static int
all_ins(PyObject* d)
{
        if (ins(d, "LOCK_SH", (long)LOCK_SH)) return -1;
        if (ins(d, "LOCK_EX", (long)LOCK_EX)) return -1;
        if (ins(d, "LOCK_NB", (long)LOCK_NB)) return -1;
        if (ins(d, "LOCK_UN", (long)LOCK_UN)) return -1;
	return 0;
}

DL_EXPORT(void)
initfcntl(void)
{
	PyObject *m, *d;

	/* Create the module and add the functions and documentation */
	m = Py_InitModule3("fcntl", fcntl_methods, module_doc);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	all_ins(d);
}
