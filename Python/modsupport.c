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

/* Module support implementation */

#include "Python.h"

#ifdef MPW /* MPW pushes 'extended' for float and double types with varargs */
typedef extended va_double;
#else
typedef double va_double;
#endif

/* Py_InitModule4() parameters:
   - name is the module name
   - methods is the list of top-level functions
   - doc is the documentation string
   - passthrough is passed as self to functions defined in the module
   - api_version is the value of PYTHON_API_VERSION at the time the
     module was compiled

   Return value is a borrowed reference to the module object; or NULL
   if an error occurred (in Python 1.4 and before, errors were fatal).
   Errors may still leak memory.
*/

static char api_version_warning[] =
"WARNING: Python C API version mismatch for module %s:\n\
  This Python has API version %d, module %s has version %d.\n";

PyObject *
Py_InitModule4(name, methods, doc, passthrough, module_api_version)
	char *name;
	PyMethodDef *methods;
	char *doc;
	PyObject *passthrough;
	int module_api_version;
{
	PyObject *m, *d, *v;
	PyMethodDef *ml;
	if (module_api_version != PYTHON_API_VERSION)
		fprintf(stderr, api_version_warning,
			name, PYTHON_API_VERSION, name, module_api_version);
	if ((m = PyImport_AddModule(name)) == NULL)
		return NULL;
	d = PyModule_GetDict(m);
	for (ml = methods; ml->ml_name != NULL; ml++) {
		v = PyCFunction_New(ml, passthrough);
		if (v == NULL)
			return NULL;
		if (PyDict_SetItemString(d, ml->ml_name, v) != 0)
			return NULL;
		Py_DECREF(v);
	}
	if (doc != NULL) {
		v = PyString_FromString(doc);
		if (v == NULL || PyDict_SetItemString(d, "__doc__", v) != 0)
			return NULL;
		Py_DECREF(v);
	}
	return m;
}


/* Helper for mkvalue() to scan the length of a format */

static int countformat Py_PROTO((char *format, int endchar));
static int countformat(format, endchar)
	char *format;
	int endchar;
{
	int count = 0;
	int level = 0;
	while (level > 0 || *format != endchar) {
		switch (*format) {
		case '\0':
			/* Premature end */
			PyErr_SetString(PyExc_SystemError,
					"unmatched paren in format");
			return -1;
		case '(':
		case '[':
		case '{':
			if (level == 0)
				count++;
			level++;
			break;
		case ')':
		case ']':
		case '}':
			level--;
			break;
		case '#':
		case '&':
		case ',':
		case ':':
		case ' ':
		case '\t':
			break;
		default:
			if (level == 0)
				count++;
		}
		format++;
	}
	return count;
}


/* Generic function to create a value -- the inverse of getargs() */
/* After an original idea and first implementation by Steven Miale */

static PyObject *do_mktuple Py_PROTO((char**, va_list *, int, int));
static PyObject *do_mklist Py_PROTO((char**, va_list *, int, int));
static PyObject *do_mkdict Py_PROTO((char**, va_list *, int, int));
static PyObject *do_mkvalue Py_PROTO((char**, va_list *));


static PyObject *
do_mkdict(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	PyObject *d;
	int i;
	if (n < 0)
		return NULL;
	if ((d = PyDict_New()) == NULL)
		return NULL;
	for (i = 0; i < n; i+= 2) {
		PyObject *k, *v;
		k = do_mkvalue(p_format, p_va);
		if (k == NULL) {
			Py_DECREF(d);
			return NULL;
		}
		v = do_mkvalue(p_format, p_va);
		if (v == NULL) {
			Py_DECREF(k);
			Py_DECREF(d);
			return NULL;
		}
		if (PyDict_SetItem(d, k, v) < 0) {
			Py_DECREF(k);
			Py_DECREF(v);
			Py_DECREF(d);
			return NULL;
		}
	}
	if (d != NULL && **p_format != endchar) {
		Py_DECREF(d);
		d = NULL;
		PyErr_SetString(PyExc_SystemError,
				"Unmatched paren in format");
	}
	else if (endchar)
		++*p_format;
	return d;
}

static PyObject *
do_mklist(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	PyObject *v;
	int i;
	if (n < 0)
		return NULL;
	if ((v = PyList_New(n)) == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyObject *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyList_SetItem(v, i, w);
	}
	if (v != NULL && **p_format != endchar) {
		Py_DECREF(v);
		v = NULL;
		PyErr_SetString(PyExc_SystemError,
				"Unmatched paren in format");
	}
	else if (endchar)
		++*p_format;
	return v;
}

static PyObject *
do_mktuple(p_format, p_va, endchar, n)
	char **p_format;
	va_list *p_va;
	int endchar;
	int n;
{
	PyObject *v;
	int i;
	if (n < 0)
		return NULL;
	if ((v = PyTuple_New(n)) == NULL)
		return NULL;
	for (i = 0; i < n; i++) {
		PyObject *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			Py_DECREF(v);
			return NULL;
		}
		PyTuple_SetItem(v, i, w);
	}
	if (v != NULL && **p_format != endchar) {
		Py_DECREF(v);
		v = NULL;
		PyErr_SetString(PyExc_SystemError,
				"Unmatched paren in format");
	}
	else if (endchar)
		++*p_format;
	return v;
}

static PyObject *
do_mkvalue(p_format, p_va)
	char **p_format;
	va_list *p_va;
{
	for (;;) {
		switch (*(*p_format)++) {
		case '(':
			return do_mktuple(p_format, p_va, ')',
					  countformat(*p_format, ')'));

		case '[':
			return do_mklist(p_format, p_va, ']',
					 countformat(*p_format, ']'));

		case '{':
			return do_mkdict(p_format, p_va, '}',
					 countformat(*p_format, '}'));

		case 'b':
		case 'h':
		case 'i':
			return PyInt_FromLong((long)va_arg(*p_va, int));

		case 'l':
			return PyInt_FromLong((long)va_arg(*p_va, long));

		case 'f':
		case 'd':
			return PyFloat_FromDouble(
				(double)va_arg(*p_va, va_double));

		case 'c':
		{
			char p[1];
			p[0] = va_arg(*p_va, int);
			return PyString_FromStringAndSize(p, 1);
		}

		case 's':
		case 'z':
		{
			PyObject *v;
			char *str = va_arg(*p_va, char *);
			int n;
			if (**p_format == '#') {
				++*p_format;
				n = va_arg(*p_va, int);
			}
			else
				n = -1;
			if (str == NULL) {
				v = Py_None;
				Py_INCREF(v);
			}
			else {
				if (n < 0)
					n = strlen(str);
				v = PyString_FromStringAndSize(str, n);
			}
			return v;
		}

		case 'S':
		case 'O':
		if (**p_format == '&') {
			typedef PyObject *(*converter) Py_PROTO((void *));
			converter func = va_arg(*p_va, converter);
			void *arg = va_arg(*p_va, void *);
			++*p_format;
			return (*func)(arg);
		}
		else {
			PyObject *v;
			v = va_arg(*p_va, PyObject *);
			if (v != NULL)
				Py_INCREF(v);
			else if (!PyErr_Occurred())
				/* If a NULL was passed
				 * because a call that should
				 * have constructed a value
				 * failed, that's OK, and we
				 * pass the error on; but if
				 * no error occurred it's not
				 * clear that the caller knew
				 * what she was doing. */
				PyErr_SetString(PyExc_SystemError,
					   "NULL object passed to mkvalue");
			return v;
		}

		case ':':
		case ',':
		case ' ':
		case '\t':
			break;

		default:
			PyErr_SetString(PyExc_SystemError,
				   "bad format char passed to mkvalue");
			return NULL;

		}
	}
}


#ifdef HAVE_STDARG_PROTOTYPES
/* VARARGS 2 */
PyObject *Py_BuildValue(char *format, ...)
#else
/* VARARGS */
PyObject *Py_BuildValue(va_alist) va_dcl
#endif
{
	va_list va;
	PyObject* retval;
#ifdef HAVE_STDARG_PROTOTYPES
	va_start(va, format);
#else
	char *format;
	va_start(va);
	format = va_arg(va, char *);
#endif
	retval = Py_VaBuildValue(format, va);
	va_end(va);
	return retval;
}

PyObject *
Py_VaBuildValue(format, va)
	char *format;
	va_list va;
{
	char *f = format;
	int n = countformat(f, '\0');
	va_list lva;

#ifdef VA_LIST_IS_ARRAY
	memcpy(lva, va, sizeof(va_list));
#else
	lva = va;
#endif

	if (n < 0)
		return NULL;
	if (n == 0) {
		Py_INCREF(Py_None);
		return Py_None;
	}
	if (n == 1)
		return do_mkvalue(&f, &lva);
	return do_mktuple(&f, &lva, '\0', n);
}


#ifdef HAVE_STDARG_PROTOTYPES
PyObject *
PyEval_CallFunction(PyObject *obj, char *format, ...)
#else
PyObject *
PyEval_CallFunction(obj, format, va_alist)
	PyObject *obj;
	char *format;
	va_dcl
#endif
{
	va_list vargs;
	PyObject *args;
	PyObject *res;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	args = Py_VaBuildValue(format, vargs);
	va_end(vargs);

	if (args == NULL)
		return NULL;

	res = PyEval_CallObject(obj, args);
	Py_DECREF(args);

	return res;
}


#ifdef HAVE_STDARG_PROTOTYPES
PyObject *
PyEval_CallMethod(PyObject *obj, char *methonname, char *format, ...)
#else
PyObject *
PyEval_CallMethod(obj, methonname, format, va_alist)
	PyObject *obj;
	char *methonname;
	char *format;
	va_dcl
#endif
{
	va_list vargs;
	PyObject *meth;
	PyObject *args;
	PyObject *res;

	meth = PyObject_GetAttrString(obj, methonname);
	if (meth == NULL)
		return NULL;

#ifdef HAVE_STDARG_PROTOTYPES
	va_start(vargs, format);
#else
	va_start(vargs);
#endif

	args = Py_VaBuildValue(format, vargs);
	va_end(vargs);

	if (args == NULL) {
		Py_DECREF(meth);
		return NULL;
	}

	res = PyEval_CallObject(meth, args);
	Py_DECREF(meth);
	Py_DECREF(args);

	return res;
}
