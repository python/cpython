/*********************************************************

	msvcrtmodule.c

	A Python interface to the Microsoft Visual C Runtime
	Library, providing access to those non-portable, but
	still useful routines.

	Only ever compiled with an MS compiler, so no attempt
	has been made to avoid MS language extensions, etc...

	This may only work on NT or 95...

	Author: Mark Hammond and Guido van Rossum.
	Maintenance: Guido van Rossum.

***********************************************************/

#include "Python.h"
#include "malloc.h"

// Force the malloc heap to clean itself up, and free unused blocks
// back to the OS.  (According to the docs, only works on NT.)
static PyObject *msvcrt_heapmin(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":heapmin"))
		return NULL;

	if (_heapmin() != 0)
		return PyErr_SetFromErrno(PyExc_IOError);

	Py_INCREF(Py_None);
	return Py_None;
}

// Perform locking operations on a C runtime file descriptor.
static PyObject *msvcrt_locking(PyObject *self, PyObject *args)
{
	int fd;
	int mode;
	long nbytes;
	int err;

	if (!PyArg_ParseTuple(args, "iil:locking", &fd, &mode, &nbytes))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	err = _locking(fd, mode, nbytes);
	Py_END_ALLOW_THREADS
	if (err != 0)
		return PyErr_SetFromErrno(PyExc_IOError);

	Py_INCREF(Py_None);
	return Py_None;
}

// Set the file translation mode for a C runtime file descriptor.
static PyObject *msvcrt_setmode(PyObject *self, PyObject *args)
{
	int fd;
	int flags;
	if (!PyArg_ParseTuple(args,"ii:setmode", &fd, &flags))
		return NULL;

	flags = _setmode(fd, flags);
	if (flags == -1)
		return PyErr_SetFromErrno(PyExc_IOError);

	return PyInt_FromLong(flags);
}

// Convert an OS file handle to a C runtime file descriptor.
static PyObject *msvcrt_open_osfhandle(PyObject *self, PyObject *args)
{
	long handle;
	int flags;
	int fd;

	if (!PyArg_ParseTuple(args, "li:open_osfhandle", &handle, &flags))
		return NULL;

	fd = _open_osfhandle(handle, flags);
	if (fd == -1)
		return PyErr_SetFromErrno(PyExc_IOError);

	return PyInt_FromLong(fd);
}

// Convert a C runtime file descriptor to an OS file handle.
static PyObject *msvcrt_get_osfhandle(PyObject *self, PyObject *args)
{
	int fd;
	intptr_t handle;

	if (!PyArg_ParseTuple(args,"i:get_osfhandle", &fd))
		return NULL;

	handle = _get_osfhandle(fd);
	if (handle == -1)
		return PyErr_SetFromErrno(PyExc_IOError);

	/* technically 'handle' is not a pointer, but a integer as
	   large as a pointer, Python's *VoidPtr interface is the
	   most appropriate here */
	return PyLong_FromVoidPtr((void*)handle);
}

/* Console I/O */
#include <conio.h>

static PyObject *msvcrt_kbhit(PyObject *self, PyObject *args)
{
	int ok;

	if (!PyArg_ParseTuple(args, ":kbhit"))
		return NULL;

	ok = _kbhit();
	return PyInt_FromLong(ok);
}

static PyObject *msvcrt_getch(PyObject *self, PyObject *args)
{
	int ch;
	char s[1];

	if (!PyArg_ParseTuple(args, ":getch"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getch();
	Py_END_ALLOW_THREADS
	s[0] = ch;
	return PyString_FromStringAndSize(s, 1);
}

static PyObject *msvcrt_getche(PyObject *self, PyObject *args)
{
	int ch;
	char s[1];

	if (!PyArg_ParseTuple(args, ":getche"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getche();
	Py_END_ALLOW_THREADS
	s[0] = ch;
	return PyString_FromStringAndSize(s, 1);
}

static PyObject *msvcrt_putch(PyObject *self, PyObject *args)
{
	char ch;

	if (!PyArg_ParseTuple(args, "c:putch", &ch))
		return NULL;

	_putch(ch);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject *msvcrt_ungetch(PyObject *self, PyObject *args)
{
	char ch;

	if (!PyArg_ParseTuple(args, "c:ungetch", &ch))
		return NULL;

	if (_ungetch(ch) == EOF)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}


/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
	{"heapmin",		msvcrt_heapmin, 1},
	{"locking",             msvcrt_locking, 1},
	{"setmode",		msvcrt_setmode, 1},
	{"open_osfhandle",	msvcrt_open_osfhandle, 1},
	{"get_osfhandle",	msvcrt_get_osfhandle, 1},
	{"kbhit",		msvcrt_kbhit, 1},
	{"getch",		msvcrt_getch, 1},
	{"getche",		msvcrt_getche, 1},
	{"putch",		msvcrt_putch, 1},
	{"ungetch",		msvcrt_ungetch, 1},
	{NULL,			NULL}
};

__declspec(dllexport) void
initmsvcrt(void)
{
	Py_InitModule("msvcrt", msvcrt_functions);
}
