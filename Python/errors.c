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

/* Error handling */

#include "Python.h"

#ifdef SYMANTEC__CFM68K__
#pragma lib_export on
#endif

#ifdef macintosh
#ifndef __MSL__
/* Replace strerror with a Mac specific routine.
   XXX PROBLEM: some positive errors have a meaning for MacOS,
   but some library routines set Unix error numbers...
*/
extern char *PyMac_StrError Py_PROTO((int));
#undef strerror
#define strerror PyMac_StrError
#endif
#endif /* macintosh */

#ifndef __STDC__
#ifndef MS_WINDOWS
extern char *strerror Py_PROTO((int));
#endif
#endif

/* Last exception stored */

static PyObject *last_exception;
static PyObject *last_exc_val;

void
PyErr_Restore(exception, value, traceback)
	PyObject *exception;
	PyObject *value;
	PyObject *traceback;
{
	PyErr_Clear();

	last_exception = exception;
	last_exc_val = value;
	(void) PyTraceBack_Store(traceback);
	Py_XDECREF(traceback);
}

void
PyErr_SetObject(exception, value)
	PyObject *exception;
	PyObject *value;
{
	Py_XINCREF(exception);
	Py_XINCREF(value);
	PyErr_Restore(exception, value, (PyObject *)NULL);
}

void
PyErr_SetNone(exception)
	PyObject *exception;
{
	PyErr_SetObject(exception, (PyObject *)NULL);
}

void
PyErr_SetString(exception, string)
	PyObject *exception;
	const char *string;
{
	PyObject *value = PyString_FromString(string);
	PyErr_SetObject(exception, value);
	Py_XDECREF(value);
}


PyObject *
PyErr_Occurred()
{
	return last_exception;
}

void
PyErr_Fetch(p_exc, p_val, p_tb)
	PyObject **p_exc;
	PyObject **p_val;
	PyObject **p_tb;
{
	*p_exc = last_exception;
	last_exception = NULL;
	*p_val = last_exc_val;
	last_exc_val = NULL;
	*p_tb = PyTraceBack_Fetch();
}

void
PyErr_Clear()
{
	PyObject *tb;
	Py_XDECREF(last_exception);
	last_exception = NULL;
	Py_XDECREF(last_exc_val);
	last_exc_val = NULL;
	/* Also clear interpreter stack trace */
	tb = PyTraceBack_Fetch();
	Py_XDECREF(tb);
}

/* Convenience functions to set a type error exception and return 0 */

int
PyErr_BadArgument()
{
	PyErr_SetString(PyExc_TypeError, "illegal argument type for built-in operation");
	return 0;
}

PyObject *
PyErr_NoMemory()
{
	PyErr_SetNone(PyExc_MemoryError);
	return NULL;
}

PyObject *
PyErr_SetFromErrno(exc)
	PyObject *exc;
{
	PyObject *v;
	int i = errno;
#ifdef EINTR
	if (i == EINTR && PyErr_CheckSignals())
		return NULL;
#endif
	v = Py_BuildValue("(is)", i, strerror(i));
	if (v != NULL) {
		PyErr_SetObject(exc, v);
		Py_DECREF(v);
	}
	return NULL;
}

void
PyErr_BadInternalCall()
{
	PyErr_SetString(PyExc_SystemError, "bad argument to internal function");
}


#ifdef HAVE_STDARG_PROTOTYPES
PyObject *
PyErr_Format(PyObject *exception, const char *format, ...)
#else
PyObject *
PyErr_Format(exception, format, va_alist)
	PyObject *exception;
	const char *format;
	va_dcl
#endif
{
	va_list vargs;
	char buffer[500]; /* Caller is responsible for limiting the format */

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	vsprintf(buffer, format, vargs);
	PyErr_SetString(exception, buffer);
	return NULL;
}
