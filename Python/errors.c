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

void
PyErr_Restore(type, value, traceback)
	PyObject *type;
	PyObject *value;
	PyObject *traceback;
{
	PyThreadState *tstate = PyThreadState_Get();
	PyObject *oldtype, *oldvalue, *oldtraceback;

	if (traceback != NULL && !PyTraceBack_Check(traceback)) {
		/* XXX Should never happen -- fatal error instead? */
		Py_DECREF(traceback);
		traceback = NULL;
	}

	/* Save these in locals to safeguard against recursive
	   invocation through Py_XDECREF */
	oldtype = tstate->curexc_type;
	oldvalue = tstate->curexc_value;
	oldtraceback = tstate->curexc_traceback;

	tstate->curexc_type = type;
	tstate->curexc_value = value;
	tstate->curexc_traceback = traceback;

	Py_XDECREF(oldtype);
	Py_XDECREF(oldvalue);
	Py_XDECREF(oldtraceback);
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
	PyThreadState *tstate = PyThreadState_Get();

	return tstate->curexc_type;
}

void
PyErr_Fetch(p_type, p_value, p_traceback)
	PyObject **p_type;
	PyObject **p_value;
	PyObject **p_traceback;
{
	PyThreadState *tstate = PyThreadState_Get();

	*p_type = tstate->curexc_type;
	*p_value = tstate->curexc_value;
	*p_traceback = tstate->curexc_traceback;

	tstate->curexc_type = NULL;
	tstate->curexc_value = NULL;
	tstate->curexc_traceback = NULL;
}

void
PyErr_Clear()
{
	PyErr_Restore(NULL, NULL, NULL);
}

/* Convenience functions to set a type error exception and return 0 */

int
PyErr_BadArgument()
{
	PyErr_SetString(PyExc_TypeError,
			"illegal argument type for built-in operation");
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
	PyErr_SetString(PyExc_SystemError,
			"bad argument to internal function");
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
