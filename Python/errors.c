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
extern char *PyMac_StrError Py_PROTO((int));
#undef strerror
#define strerror PyMac_StrError
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
	PyThreadState *tstate = PyThreadState_GET();
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


int
PyErr_GivenExceptionMatches(err, exc)
     PyObject *err, *exc;
{
	if (PyTuple_Check(exc)) {
		int i, n;
		n = PyTuple_Size(exc);
		for (i = 0; i < n; i++) {
			/* Test recursively */
		     if (PyErr_GivenExceptionMatches(
			     err, PyTuple_GET_ITEM(exc, i)))
		     {
			     return 1;
		     }
		}
		return 0;
	}
	/* err might be an instance, so check its class. */
	if (PyInstance_Check(err))
		err = (PyObject*)((PyInstanceObject*)err)->in_class;

	if (PyClass_Check(err) && PyClass_Check(exc))
		return PyClass_IsSubclass(err, exc);

	return err == exc;
}
	

int
PyErr_ExceptionMatches(exc)
     PyObject *exc;
{
	return PyErr_GivenExceptionMatches(PyErr_Occurred(), exc);
}


/* Used in many places to normalize a raised exception, including in
   eval_code2(), do_raise(), and PyErr_Print()
*/
void
PyErr_NormalizeException(exc, val, tb)
     PyObject **exc;
     PyObject **val;
     PyObject **tb;
{
	PyObject *type = *exc;
	PyObject *value = *val;
	PyObject *inclass = NULL;

	/* If PyErr_SetNone() was used, the value will have been actually
	   set to NULL.
	*/
	if (!value) {
		value = Py_None;
		Py_INCREF(value);
	}

	if (PyInstance_Check(value))
		inclass = (PyObject*)((PyInstanceObject*)value)->in_class;

	/* Normalize the exception so that if the type is a class, the
	   value will be an instance.
	*/
	if (PyClass_Check(type)) {
		/* if the value was not an instance, or is not an instance
		   whose class is (or is derived from) type, then use the
		   value as an argument to instantiation of the type
		   class.
		*/
		if (!inclass || !PyClass_IsSubclass(inclass, type)) {
			PyObject *args, *res;

			if (value == Py_None)
				args = Py_BuildValue("()");
			else if (PyTuple_Check(value)) {
				Py_INCREF(value);
				args = value;
			}
			else
				args = Py_BuildValue("(O)", value);

			if (args == NULL)
				goto finally;
			res = PyEval_CallObject(type, args);
			Py_DECREF(args);
			if (res == NULL)
				goto finally;
			Py_DECREF(value);
			value = res;
		}
		/* if the class of the instance doesn't exactly match the
		   class of the type, believe the instance
		*/
		else if (inclass != type) {
 			Py_DECREF(type);
			type = inclass;
			Py_INCREF(type);
		}
	}
	*exc = type;
	*val = value;
	return;
finally:
	Py_DECREF(type);
	Py_DECREF(value);
	Py_XDECREF(*tb);
	PyErr_Fetch(exc, val, tb);
	/* normalize recursively */
	PyErr_NormalizeException(exc, val, tb);
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
	/* raise the pre-allocated instance if it still exists */
	if (PyExc_MemoryErrorInst)
		PyErr_SetObject(PyExc_MemoryError, PyExc_MemoryErrorInst);
	else
		/* this will probably fail since there's no memory and hee,
		   hee, we have to instantiate this class
		*/
		PyErr_SetNone(PyExc_MemoryError);

	return NULL;
}

PyObject *
PyErr_SetFromErrnoWithFilename(exc, filename)
	PyObject *exc;
	char *filename;
{
	PyObject *v;
	char *s;
	int i = errno;
#ifdef EINTR
	if (i == EINTR && PyErr_CheckSignals())
		return NULL;
#endif
	if (i == 0)
		s = "Error"; /* Sometimes errno didn't get set */
	else
		s = strerror(i);
	if (filename != NULL && Py_UseClassExceptionsFlag)
		v = Py_BuildValue("(iss)", i, s, filename);
	else
		v = Py_BuildValue("(is)", i, s);
	if (v != NULL) {
		PyErr_SetObject(exc, v);
		Py_DECREF(v);
	}
	return NULL;
}
	

PyObject *
PyErr_SetFromErrno(exc)
	PyObject *exc;
{
	return PyErr_SetFromErrnoWithFilename(exc, NULL);
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


PyObject *
PyErr_NewException(name, base, dict)
	char *name; /* modulename.classname */
	PyObject *base;
	PyObject *dict;
{
	char *dot;
	PyObject *modulename = NULL;
	PyObject *classname = NULL;
	PyObject *mydict = NULL;
	PyObject *bases = NULL;
	PyObject *result = NULL;
	dot = strrchr(name, '.');
	if (dot == NULL) {
		PyErr_SetString(PyExc_SystemError,
			"PyErr_NewException: name must be module.class");
		return NULL;
	}
	if (base == NULL)
		base = PyExc_Exception;
	if (!PyClass_Check(base)) {
		/* Must be using string-based standard exceptions (-X) */
		return PyString_FromString(name);
	}
	if (dict == NULL) {
		dict = mydict = PyDict_New();
		if (dict == NULL)
			goto failure;
	}
	if (PyDict_GetItemString(dict, "__module__") == NULL) {
		modulename = PyString_FromStringAndSize(name, (int)(dot-name));
		if (modulename == NULL)
			goto failure;
		if (PyDict_SetItemString(dict, "__module__", modulename) != 0)
			goto failure;
	}
	classname = PyString_FromString(dot+1);
	if (classname == NULL)
		goto failure;
	bases = Py_BuildValue("(O)", base);
	if (bases == NULL)
		goto failure;
	result = PyClass_New(bases, dict, classname);
  failure:
	Py_XDECREF(bases);
	Py_XDECREF(mydict);
	Py_XDECREF(classname);
	Py_XDECREF(modulename);
	return result;
}
