
/* fcntl module */

#include "Python.h"

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#include <sys/ioctl.h>
#include <fcntl.h>
#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

static int
conv_descriptor(PyObject *object, int *target)
{
    int fd = PyObject_AsFileDescriptor(object);

    if (fd < 0)
        return 0;
    *target = fd;
    return 1;
}


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

	if (PyArg_ParseTuple(args, "O&is#:fcntl",
                             conv_descriptor, &fd, &code, &str, &len)) {
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
	if (!PyArg_ParseTuple(args,
             "O&i|i;fcntl requires a file or file descriptor,"
             " an integer and optionally a third integer or a string", 
			      conv_descriptor, &fd, &code, &arg)) {
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

PyDoc_STRVAR(fcntl_doc,
"fcntl(fd, opt, [arg])\n\
\n\
Perform the requested operation on file descriptor fd.  The operation\n\
is defined by op and is operating system dependent.  These constants are\n\
available from the fcntl module.  The argument arg is optional, and\n\
defaults to 0; it may be an int or a string. If arg is given as a string,\n\
the return value of fcntl is a string of that length, containing the\n\
resulting value put in the arg buffer by the operating system.The length\n\
of the arg string is not allowed to exceed 1024 bytes. If the arg given\n\
is an integer or if none is specified, the result value is an integer\n\
corresponding to the return value of the fcntl call in the C code.");


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
	int mutate_arg = 0;
	char buf[1024];

	if (PyArg_ParseTuple(args, "O&iw#|i:ioctl",
                             conv_descriptor, &fd, &code, 
			     &str, &len, &mutate_arg)) {
		char *arg;

		if (PyTuple_Size(args) == 3) {
#if (PY_MAJOR_VERSION>2) || (PY_MINOR_VERSION>=5)
#error Remove the warning, change mutate_arg to 1
#endif
			if (PyErr_Warn(PyExc_FutureWarning,
       "ioctl with mutable buffer will mutate the buffer by default in 2.5"
				    ) < 0)
				return NULL;
			mutate_arg = 0;
		}
	       	if (mutate_arg) {
			if (len <= sizeof buf) {
				memcpy(buf, str, len);
				arg = buf;
			} 
			else {
				arg = str;
			}
		}
		else {
			if (len > sizeof buf) {
				PyErr_SetString(PyExc_ValueError,
					"ioctl string arg too long");
				return NULL;
			}
			else {
				memcpy(buf, str, len);
				arg = buf;
			}
		}
		if (buf == arg) {
			Py_BEGIN_ALLOW_THREADS /* think array.resize() */
			ret = ioctl(fd, code, arg);
			Py_END_ALLOW_THREADS
		}
		else {
			ret = ioctl(fd, code, arg);
		}
		if (mutate_arg && (len < sizeof buf)) {
			memcpy(str, buf, len);
		}
		if (ret < 0) {
			PyErr_SetFromErrno(PyExc_IOError);
			return NULL;
		}
		if (mutate_arg) {
			return PyInt_FromLong(ret);
		}
		else {
			return PyString_FromStringAndSize(buf, len);
		}
	}

	PyErr_Clear();
	if (PyArg_ParseTuple(args, "O&is#:ioctl",
                             conv_descriptor, &fd, &code, &str, &len)) {
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
	if (!PyArg_ParseTuple(args,
	     "O&i|i;ioctl requires a file or file descriptor,"
	     " an integer and optionally a integer or buffer argument",
			      conv_descriptor, &fd, &code, &arg)) {
	  return NULL;
	}
	Py_BEGIN_ALLOW_THREADS
#ifdef __VMS
	ret = ioctl(fd, code, (void *)arg);
#else
	ret = ioctl(fd, code, arg);
#endif
	Py_END_ALLOW_THREADS
	if (ret < 0) {
		PyErr_SetFromErrno(PyExc_IOError);
		return NULL;
	}
	return PyInt_FromLong((long)ret);
}

PyDoc_STRVAR(ioctl_doc,
"ioctl(fd, opt[, arg[, mutate_flag]])\n\
\n\
Perform the requested operation on file descriptor fd.  The operation is\n\
defined by opt and is operating system dependent.  Typically these codes are\n\
retrieved from the fcntl or termios library modules.\n\
\n\
The argument arg is optional, and defaults to 0; it may be an int or a\n\
buffer containing character data (most likely a string or an array). \n\
\n\
If the argument is a mutable buffer (such as an array) and if the\n\
mutate_flag argument (which is only allowed in this case) is true then the\n\
buffer is (in effect) passed to the operating system and changes made by\n\
the OS will be reflected in the contents of the buffer after the call has\n\
returned.  The return value is the integer returned by the ioctl system\n\
call.\n\
\n\
If the argument is a mutable buffer and the mutable_flag argument is not\n\
passed or is false, the behavior is as if a string had been passed.  This\n\
behavior will change in future releases of Python.\n\
\n\
If the argument is an immutable buffer (most likely a string) then a copy\n\
of the buffer is passed to the operating system and the return value is a\n\
string of the same length containing whatever the operating system put in\n\
the buffer.  The length of the arg buffer in this case is not allowed to\n\
exceed 1024 bytes.\n\
\n\
If the arg given is an integer or if none is specified, the result value is\n\
an integer corresponding to the return value of the ioctl call in the C\n\
code.");


/* flock(fd, operation) */

static PyObject *
fcntl_flock(PyObject *self, PyObject *args)
{
	int fd;
	int code;
	int ret;

	if (!PyArg_ParseTuple(args, "O&i:flock",
                              conv_descriptor, &fd, &code))
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

PyDoc_STRVAR(flock_doc,
"flock(fd, operation)\n\
\n\
Perform the lock operation op on file descriptor fd.  See the Unix \n\
manual flock(3) for details.  (On some systems, this function is\n\
emulated using fcntl().)");


/* lockf(fd, operation) */
static PyObject *
fcntl_lockf(PyObject *self, PyObject *args)
{
	int fd, code, ret, whence = 0;
	PyObject *lenobj = NULL, *startobj = NULL;

	if (!PyArg_ParseTuple(args, "O&i|OOi:lockf",
                              conv_descriptor, &fd, &code,
			      &lenobj, &startobj, &whence))
	    return NULL;

#if defined(PYOS_OS2) && defined(PYCC_GCC)
	PyErr_SetString(PyExc_NotImplementedError,
			"lockf not supported on OS/2 (EMX)");
	return NULL;
#else
#ifndef LOCK_SH
#define LOCK_SH		1	/* shared lock */
#define LOCK_EX		2	/* exclusive lock */
#define LOCK_NB		4	/* don't block when locking */
#define LOCK_UN		8	/* unlock */
#endif  /* LOCK_SH */
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
#endif  /* defined(PYOS_OS2) && defined(PYCC_GCC) */
}

PyDoc_STRVAR(lockf_doc,
"lockf (fd, operation, length=0, start=0, whence=0)\n\
\n\
This is essentially a wrapper around the fcntl() locking calls.  fd is the\n\
file descriptor of the file to lock or unlock, and operation is one of the\n\
following values:\n\
\n\
    LOCK_UN - unlock\n\
    LOCK_SH - acquire a shared lock\n\
    LOCK_EX - acquire an exclusive lock\n\
\n\
When operation is LOCK_SH or LOCK_EX, it can also be bit-wise OR'd with\n\
LOCK_NB to avoid blocking on lock acquisition.  If LOCK_NB is used and the\n\
lock cannot be acquired, an IOError will be raised and the exception will\n\
have an errno attribute set to EACCES or EAGAIN (depending on the operating\n\
system -- for portability, check for either value).\n\
\n\
length is the number of bytes to lock, with the default meaning to lock to\n\
EOF.  start is the byte offset, relative to whence, to that the lock\n\
starts.  whence is as with fileobj.seek(), specifically:\n\
\n\
    0 - relative to the start of the file (SEEK_SET)\n\
    1 - relative to the current buffer position (SEEK_CUR)\n\
    2 - relative to the end of the file (SEEK_END)");

/* List of functions */

static PyMethodDef fcntl_methods[] = {
	{"fcntl",	fcntl_fcntl, METH_VARARGS, fcntl_doc},
	{"ioctl",	fcntl_ioctl, METH_VARARGS, ioctl_doc},
	{"flock",	fcntl_flock, METH_VARARGS, flock_doc},
	{"lockf",       fcntl_lockf, METH_VARARGS, lockf_doc},
	{NULL,		NULL}		/* sentinel */
};


PyDoc_STRVAR(module_doc,
"This module performs file control and I/O control on file \n\
descriptors.  It is an interface to the fcntl() and ioctl() Unix\n\
routines.  File descriptors can be obtained with the fileno() method of\n\
a file or socket object.");

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

#define INS(x) if (ins(d, #x, (long)x)) return -1

static int
all_ins(PyObject* d)
{
        if (ins(d, "LOCK_SH", (long)LOCK_SH)) return -1;
        if (ins(d, "LOCK_EX", (long)LOCK_EX)) return -1;
        if (ins(d, "LOCK_NB", (long)LOCK_NB)) return -1;
        if (ins(d, "LOCK_UN", (long)LOCK_UN)) return -1;
/* GNU extensions, as of glibc 2.2.4 */
#ifdef LOCK_MAND
        if (ins(d, "LOCK_MAND", (long)LOCK_MAND)) return -1;
#endif
#ifdef LOCK_READ
        if (ins(d, "LOCK_READ", (long)LOCK_READ)) return -1;
#endif
#ifdef LOCK_WRITE
        if (ins(d, "LOCK_WRITE", (long)LOCK_WRITE)) return -1;
#endif
#ifdef LOCK_RW
        if (ins(d, "LOCK_RW", (long)LOCK_RW)) return -1;
#endif

#ifdef F_DUPFD
        if (ins(d, "F_DUPFD", (long)F_DUPFD)) return -1;
#endif
#ifdef F_GETFD
        if (ins(d, "F_GETFD", (long)F_GETFD)) return -1;
#endif
#ifdef F_SETFD
        if (ins(d, "F_SETFD", (long)F_SETFD)) return -1;
#endif
#ifdef F_GETFL
        if (ins(d, "F_GETFL", (long)F_GETFL)) return -1;
#endif
#ifdef F_SETFL
        if (ins(d, "F_SETFL", (long)F_SETFL)) return -1;
#endif
#ifdef F_GETLK
        if (ins(d, "F_GETLK", (long)F_GETLK)) return -1;
#endif
#ifdef F_SETLK
        if (ins(d, "F_SETLK", (long)F_SETLK)) return -1;
#endif
#ifdef F_SETLKW
        if (ins(d, "F_SETLKW", (long)F_SETLKW)) return -1;
#endif
#ifdef F_GETOWN
        if (ins(d, "F_GETOWN", (long)F_GETOWN)) return -1;
#endif
#ifdef F_SETOWN
        if (ins(d, "F_SETOWN", (long)F_SETOWN)) return -1;
#endif
#ifdef F_GETSIG
        if (ins(d, "F_GETSIG", (long)F_GETSIG)) return -1;
#endif
#ifdef F_SETSIG
        if (ins(d, "F_SETSIG", (long)F_SETSIG)) return -1;
#endif
#ifdef F_RDLCK
        if (ins(d, "F_RDLCK", (long)F_RDLCK)) return -1;
#endif
#ifdef F_WRLCK
        if (ins(d, "F_WRLCK", (long)F_WRLCK)) return -1;
#endif
#ifdef F_UNLCK
        if (ins(d, "F_UNLCK", (long)F_UNLCK)) return -1;
#endif
/* LFS constants */
#ifdef F_GETLK64
        if (ins(d, "F_GETLK64", (long)F_GETLK64)) return -1;
#endif
#ifdef F_SETLK64
        if (ins(d, "F_SETLK64", (long)F_SETLK64)) return -1;
#endif
#ifdef F_SETLKW64
        if (ins(d, "F_SETLKW64", (long)F_SETLKW64)) return -1;
#endif
/* GNU extensions, as of glibc 2.2.4. */
#ifdef F_SETLEASE
        if (ins(d, "F_SETLEASE", (long)F_SETLEASE)) return -1;
#endif
#ifdef F_GETLEASE
        if (ins(d, "F_GETLEASE", (long)F_GETLEASE)) return -1;
#endif
#ifdef F_NOTIFY
        if (ins(d, "F_NOTIFY", (long)F_NOTIFY)) return -1;
#endif
/* Old BSD flock(). */
#ifdef F_EXLCK
        if (ins(d, "F_EXLCK", (long)F_EXLCK)) return -1;
#endif
#ifdef F_SHLCK
        if (ins(d, "F_SHLCK", (long)F_SHLCK)) return -1;
#endif

/* For F_{GET|SET}FL */
#ifdef FD_CLOEXEC
        if (ins(d, "FD_CLOEXEC", (long)FD_CLOEXEC)) return -1;
#endif

/* For F_NOTIFY */
#ifdef DN_ACCESS
        if (ins(d, "DN_ACCESS", (long)DN_ACCESS)) return -1;
#endif
#ifdef DN_MODIFY
        if (ins(d, "DN_MODIFY", (long)DN_MODIFY)) return -1;
#endif
#ifdef DN_CREATE
        if (ins(d, "DN_CREATE", (long)DN_CREATE)) return -1;
#endif
#ifdef DN_DELETE
        if (ins(d, "DN_DELETE", (long)DN_DELETE)) return -1;
#endif
#ifdef DN_RENAME
        if (ins(d, "DN_RENAME", (long)DN_RENAME)) return -1;
#endif
#ifdef DN_ATTRIB
        if (ins(d, "DN_ATTRIB", (long)DN_ATTRIB)) return -1;
#endif
#ifdef DN_MULTISHOT
        if (ins(d, "DN_MULTISHOT", (long)DN_MULTISHOT)) return -1;
#endif

#ifdef HAVE_STROPTS_H
	/* Unix 98 guarantees that these are in stropts.h. */
	INS(I_PUSH);
	INS(I_POP);
	INS(I_LOOK);
	INS(I_FLUSH);
	INS(I_FLUSHBAND);
	INS(I_SETSIG);
	INS(I_GETSIG);
	INS(I_FIND);
	INS(I_PEEK);
	INS(I_SRDOPT);
	INS(I_GRDOPT);
	INS(I_NREAD);
	INS(I_FDINSERT);
	INS(I_STR);
	INS(I_SWROPT);
#ifdef I_GWROPT
	/* despite the comment above, old-ish glibcs miss a couple... */
	INS(I_GWROPT);
#endif
	INS(I_SENDFD);
	INS(I_RECVFD);
	INS(I_LIST);
	INS(I_ATMARK);
	INS(I_CKBAND);
	INS(I_GETBAND);
	INS(I_CANPUT);
	INS(I_SETCLTIME);
#ifdef I_GETCLTIME
	INS(I_GETCLTIME);
#endif
	INS(I_LINK);
	INS(I_UNLINK);
	INS(I_PLINK);
	INS(I_PUNLINK);
#endif
	
	return 0;
}

PyMODINIT_FUNC
initfcntl(void)
{
	PyObject *m, *d;

	/* Create the module and add the functions and documentation */
	m = Py_InitModule3("fcntl", fcntl_methods, module_doc);

	/* Add some symbolic constants to the module */
	d = PyModule_GetDict(m);
	all_ins(d);
}
