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
#include <io.h>
#include <conio.h>
#include <sys/locking.h>
#include <crtdbg.h>
#include <windows.h>

#ifdef _MSC_VER
#if _MSC_VER >= 1500
#include <crtassem.h>
#endif
#endif

// Force the malloc heap to clean itself up, and free unused blocks
// back to the OS.  (According to the docs, only works on NT.)
static PyObject *
msvcrt_heapmin(PyObject *self, PyObject *args)
{
	if (!PyArg_ParseTuple(args, ":heapmin"))
		return NULL;

	if (_heapmin() != 0)
		return PyErr_SetFromErrno(PyExc_IOError);

	Py_INCREF(Py_None);
	return Py_None;
}

// Perform locking operations on a C runtime file descriptor.
static PyObject *
msvcrt_locking(PyObject *self, PyObject *args)
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
static PyObject *
msvcrt_setmode(PyObject *self, PyObject *args)
{
	int fd;
	int flags;
	if (!PyArg_ParseTuple(args,"ii:setmode", &fd, &flags))
		return NULL;

	flags = _setmode(fd, flags);
	if (flags == -1)
		return PyErr_SetFromErrno(PyExc_IOError);

	return PyLong_FromLong(flags);
}

// Convert an OS file handle to a C runtime file descriptor.
static PyObject *
msvcrt_open_osfhandle(PyObject *self, PyObject *args)
{
	long handle;
	int flags;
	int fd;

	if (!PyArg_ParseTuple(args, "li:open_osfhandle", &handle, &flags))
		return NULL;

	fd = _open_osfhandle(handle, flags);
	if (fd == -1)
		return PyErr_SetFromErrno(PyExc_IOError);

	return PyLong_FromLong(fd);
}

// Convert a C runtime file descriptor to an OS file handle.
static PyObject *
msvcrt_get_osfhandle(PyObject *self, PyObject *args)
{
	int fd;
	Py_intptr_t handle;

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

static PyObject *
msvcrt_kbhit(PyObject *self, PyObject *args)
{
	int ok;

	if (!PyArg_ParseTuple(args, ":kbhit"))
		return NULL;

	ok = _kbhit();
	return PyLong_FromLong(ok);
}

static PyObject *
msvcrt_getch(PyObject *self, PyObject *args)
{
	int ch;
	char s[1];

	if (!PyArg_ParseTuple(args, ":getch"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getch();
	Py_END_ALLOW_THREADS
	s[0] = ch;
	return PyBytes_FromStringAndSize(s, 1);
}

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_getwch(PyObject *self, PyObject *args)
{
	Py_UNICODE ch;
	Py_UNICODE u[1];

	if (!PyArg_ParseTuple(args, ":getwch"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getwch();
	Py_END_ALLOW_THREADS
	u[0] = ch;
	return PyUnicode_FromUnicode(u, 1);
}
#endif

static PyObject *
msvcrt_getche(PyObject *self, PyObject *args)
{
	int ch;
	char s[1];

	if (!PyArg_ParseTuple(args, ":getche"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getche();
	Py_END_ALLOW_THREADS
	s[0] = ch;
	return PyBytes_FromStringAndSize(s, 1);
}

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_getwche(PyObject *self, PyObject *args)
{
	Py_UNICODE ch;
	Py_UNICODE s[1];

	if (!PyArg_ParseTuple(args, ":getwche"))
		return NULL;

	Py_BEGIN_ALLOW_THREADS
	ch = _getwche();
	Py_END_ALLOW_THREADS
	s[0] = ch;
	return PyUnicode_FromUnicode(s, 1);
}
#endif

static PyObject *
msvcrt_putch(PyObject *self, PyObject *args)
{
	char ch;

	if (!PyArg_ParseTuple(args, "c:putch", &ch))
		return NULL;

	_putch(ch);
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_putwch(PyObject *self, PyObject *args)
{
	int ch;

	if (!PyArg_ParseTuple(args, "C:putwch", &ch))
		return NULL;

	_putwch(ch);
	Py_RETURN_NONE;

}
#endif

static PyObject *
msvcrt_ungetch(PyObject *self, PyObject *args)
{
	char ch;

	if (!PyArg_ParseTuple(args, "c:ungetch", &ch))
		return NULL;

	if (_ungetch(ch) == EOF)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}

#ifdef _WCONIO_DEFINED
static PyObject *
msvcrt_ungetwch(PyObject *self, PyObject *args)
{
	int ch;

	if (!PyArg_ParseTuple(args, "C:ungetwch", &ch))
		return NULL;

	if (_ungetwch(ch) == WEOF)
		return PyErr_SetFromErrno(PyExc_IOError);
	Py_INCREF(Py_None);
	return Py_None;
}
#endif

static void
insertint(PyObject *d, char *name, int value)
{
	PyObject *v = PyLong_FromLong((long) value);
	if (v == NULL) {
		/* Don't bother reporting this error */
		PyErr_Clear();
	}
	else {
		PyDict_SetItemString(d, name, v);
		Py_DECREF(v);
	}
}

#ifdef _DEBUG

static PyObject*
msvcrt_setreportfile(PyObject *self, PyObject *args)
{
	int type, file;
	_HFILE res;

	if (!PyArg_ParseTuple(args, "ii", &type, &file))
		return NULL;
	res = _CrtSetReportFile(type, (_HFILE)file);
	return PyLong_FromLong((long)res);
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject*
msvcrt_setreportmode(PyObject *self, PyObject *args)
{
	int type, mode;
	int res;

	if (!PyArg_ParseTuple(args, "ii", &type, &mode))
		return NULL;
	res = _CrtSetReportMode(type, mode);
	if (res == -1)
	    return PyErr_SetFromErrno(PyExc_IOError);
	return PyLong_FromLong(res);
}

static PyObject*
msvcrt_seterrormode(PyObject *self, PyObject *args)
{
	int mode, res;

	if (!PyArg_ParseTuple(args, "i", &mode))
		return NULL;
	res = _set_error_mode(mode);
	return PyLong_FromLong(res);
}

#endif

static PyObject*
seterrormode(PyObject *self, PyObject *args)
{
	unsigned int mode, res;

	if (!PyArg_ParseTuple(args, "I", &mode))
		return NULL;
	res = SetErrorMode(mode);
	return PyLong_FromUnsignedLong(res);
}


/* List of functions exported by this module */
static struct PyMethodDef msvcrt_functions[] = {
	{"heapmin",		msvcrt_heapmin, METH_VARARGS},
	{"locking",             msvcrt_locking, METH_VARARGS},
	{"setmode",		msvcrt_setmode, METH_VARARGS},
	{"open_osfhandle",	msvcrt_open_osfhandle, METH_VARARGS},
	{"get_osfhandle",	msvcrt_get_osfhandle, METH_VARARGS},
	{"kbhit",		msvcrt_kbhit, METH_VARARGS},
	{"getch",		msvcrt_getch, METH_VARARGS},
	{"getche",		msvcrt_getche, METH_VARARGS},
	{"putch",		msvcrt_putch, METH_VARARGS},
	{"ungetch",		msvcrt_ungetch, METH_VARARGS},
	{"SetErrorMode",	seterrormode, METH_VARARGS},
#ifdef _DEBUG
	{"CrtSetReportFile",	msvcrt_setreportfile, METH_VARARGS},
	{"CrtSetReportMode",	msvcrt_setreportmode, METH_VARARGS},
	{"set_error_mode",	msvcrt_seterrormode, METH_VARARGS},
#endif
#ifdef _WCONIO_DEFINED
	{"getwch",		msvcrt_getwch, METH_VARARGS},
	{"getwche",		msvcrt_getwche, METH_VARARGS},
	{"putwch",		msvcrt_putwch, METH_VARARGS},
	{"ungetwch",		msvcrt_ungetwch, METH_VARARGS},
#endif
	{NULL,			NULL}
};


static struct PyModuleDef msvcrtmodule = {
	PyModuleDef_HEAD_INIT,
	"msvcrt",
	NULL,
	-1,
	msvcrt_functions,
	NULL,
	NULL,
	NULL,
	NULL
};

PyMODINIT_FUNC
PyInit_msvcrt(void)
{
	int st;
	PyObject *d;
	PyObject *m = PyModule_Create(&msvcrtmodule);
	if (m == NULL)
		return NULL;
	d = PyModule_GetDict(m);

	/* constants for the locking() function's mode argument */
	insertint(d, "LK_LOCK", _LK_LOCK);
	insertint(d, "LK_NBLCK", _LK_NBLCK);
	insertint(d, "LK_NBRLCK", _LK_NBRLCK);
	insertint(d, "LK_RLCK", _LK_RLCK);
	insertint(d, "LK_UNLCK", _LK_UNLCK);
	insertint(d, "SEM_FAILCRITICALERRORS", SEM_FAILCRITICALERRORS);
	insertint(d, "SEM_NOALIGNMENTFAULTEXCEPT", SEM_NOALIGNMENTFAULTEXCEPT);
	insertint(d, "SEM_NOGPFAULTERRORBOX", SEM_NOGPFAULTERRORBOX);
	insertint(d, "SEM_NOOPENFILEERRORBOX", SEM_NOOPENFILEERRORBOX);
#ifdef _DEBUG
	insertint(d, "CRT_WARN", _CRT_WARN);
	insertint(d, "CRT_ERROR", _CRT_ERROR);
	insertint(d, "CRT_ASSERT", _CRT_ASSERT);
	insertint(d, "CRTDBG_MODE_DEBUG", _CRTDBG_MODE_DEBUG);
	insertint(d, "CRTDBG_MODE_FILE", _CRTDBG_MODE_FILE);
	insertint(d, "CRTDBG_MODE_WNDW", _CRTDBG_MODE_WNDW);
	insertint(d, "CRTDBG_REPORT_MODE", _CRTDBG_REPORT_MODE);
	insertint(d, "CRTDBG_FILE_STDERR", (int)_CRTDBG_FILE_STDERR);
	insertint(d, "CRTDBG_FILE_STDOUT", (int)_CRTDBG_FILE_STDOUT);
	insertint(d, "CRTDBG_REPORT_FILE", (int)_CRTDBG_REPORT_FILE);
#endif

	/* constants for the crt versions */
#ifdef _VC_ASSEMBLY_PUBLICKEYTOKEN
	st = PyModule_AddStringConstant(m, "VC_ASSEMBLY_PUBLICKEYTOKEN",
					_VC_ASSEMBLY_PUBLICKEYTOKEN);
	if (st < 0) return NULL;
#endif
#ifdef _CRT_ASSEMBLY_VERSION
	st = PyModule_AddStringConstant(m, "CRT_ASSEMBLY_VERSION",
					_CRT_ASSEMBLY_VERSION);
	if (st < 0) return NULL;
#endif
#ifdef __LIBRARIES_ASSEMBLY_NAME_PREFIX
	st = PyModule_AddStringConstant(m, "LIBRARIES_ASSEMBLY_NAME_PREFIX",
					__LIBRARIES_ASSEMBLY_NAME_PREFIX);
	if (st < 0) return NULL;
#endif

        return m;
}
