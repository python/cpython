/***********************************************************
Copyright 1991-1995 by Stichting Mathematisch Centrum, Amsterdam,
The Netherlands.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the names of Stichting Mathematisch
Centrum or CWI or Corporation for National Research Initiatives or
CNRI not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.

While CWI is the initial source for this software, a modified version
is made available by the Corporation for National Research Initiatives
(CNRI) at the Internet address ftp://ftp.python.org.

STICHTING MATHEMATISCH CENTRUM AND CNRI DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL STICHTING MATHEMATISCH
CENTRUM OR CNRI BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

******************************************************************/

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
fcntl_fcntl(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (PyArg_Parse(args, "(iis#)", &fd, &code, &str, &len)) {
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
	if (PyArg_Parse(args, "(ii)", &fd, &code))
		arg = 0;
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(iii)", &fd, &code, &arg))
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
is optional, and defaults to 0; it may be an int or a string.";


/* ioctl(fd, opt, [arg]) */

static PyObject *
fcntl_ioctl(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int fd;
	int code;
	int arg;
	int ret;
	char *str;
	int len;
	char buf[1024];

	if (PyArg_Parse(args, "(iis#)", &fd, &code, &str, &len)) {
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
	if (PyArg_Parse(args, "(ii)", &fd, &code))
		arg = 0;
	else {
		PyErr_Clear();
		if (!PyArg_Parse(args, "(iii)", &fd, &code, &arg))
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
is optional, and defaults to 0; it may be an int or a string.";


/* flock(fd, operation) */

static PyObject *
fcntl_flock(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int fd;
	int code;
	int ret;

	if (!PyArg_Parse(args, "(ii)", &fd, &code))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
#ifdef HAVE_FLOCK
	ret = flock(fd, code);
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
		ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
	}
#endif /* HAVE_FLOCK */
	Py_END_ALLOW_THREADS
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
fcntl_lockf(self, args)
	PyObject *self; /* Not used */
	PyObject *args;
{
	int fd, code, len = 0, start = 0, whence = 0, ret;

	if (!PyArg_ParseTuple(args, "ii|iii", &fd, &code, &len, 
			       &start, &whence))
	    return NULL;

	Py_BEGIN_ALLOW_THREADS
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
		l.l_len = len;
		l.l_start = start;
		l.l_whence = whence;
		ret = fcntl(fd, (code & LOCK_NB) ? F_SETLK : F_SETLKW, &l);
	}
	Py_END_ALLOW_THREADS
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
	{"fcntl",	fcntl_fcntl, 0, fcntl_doc},
	{"ioctl",	fcntl_ioctl, 0, ioctl_doc},
	{"flock",	fcntl_flock, 0, flock_doc},
	{"lockf",       fcntl_lockf, 1, lockf_doc},
	{NULL,		NULL}		/* sentinel */
};


static char module_doc [] =

"This module performs file control and I/O control on file \n\
descriptors.  It is an interface to the fcntl() and ioctl() Unix\n\
routines.  File descriptors can be obtained with the fileno() method of\n\
a file or socket object.";

/* Module initialisation */

static int
ins(d, symbol, value)
        PyObject* d;
        char* symbol;
        long value;
{
        PyObject* v = PyInt_FromLong(value);
        if (!v || PyDict_SetItemString(d, symbol, v) < 0)
                return -1;

        Py_DECREF(v);
        return 0;
}

static int
all_ins(d)
        PyObject* d;
{
        if (ins(d, "LOCK_SH", (long)LOCK_SH)) return -1;
        if (ins(d, "LOCK_EX", (long)LOCK_EX)) return -1;
        if (ins(d, "LOCK_NB", (long)LOCK_NB)) return -1;
        if (ins(d, "LOCK_UN", (long)LOCK_UN)) return -1;
	return 0;
}

DL_EXPORT(void)
initfcntl()
{
	PyObject *m, *d;

	/* Create the module and add the functions and documentation */
	m = Py_InitModule3("fcntl", fcntl_methods, module_doc);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	all_ins(d);

	/* Check for errors */
	if (PyErr_Occurred())
		Py_FatalError("can't initialize module fcntl");
}
