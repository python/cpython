
/* Module support implementation */

#include "Python.h"

typedef double va_double;

/* Package context -- the full module name for package imports */
char *_Py_PackageContext = NULL;

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
"Python C API version mismatch for module %.100s:\
 This Python has API version %d, module %.100s has version %d.";

PyObject *
Py_InitModule4(char *name, PyMethodDef *methods, char *doc,
	       PyObject *passthrough, int module_api_version)
{
	PyObject *m, *d, *v, *n;
	PyMethodDef *ml;
	if (!Py_IsInitialized())
	    Py_FatalError("Interpreter not initialized (version mismatch?)");
	if (module_api_version != PYTHON_API_VERSION) {
		char message[512];
		PyOS_snprintf(message, sizeof(message), 
			      api_version_warning, name, 
			      PYTHON_API_VERSION, name, 
			      module_api_version);
		if (PyErr_Warn(PyExc_RuntimeWarning, message)) 
			return NULL;
	}
	/* Make sure name is fully qualified.

	   This is a bit of a hack: when the shared library is loaded,
	   the module name is "package.module", but the module calls
	   Py_InitModule*() with just "module" for the name.  The shared
	   library loader squirrels away the true name of the module in
	   _Py_PackageContext, and Py_InitModule*() will substitute this
	   (if the name actually matches).
	*/
	if (_Py_PackageContext != NULL) {
		char *p = strrchr(_Py_PackageContext, '.');
		if (p != NULL && strcmp(name, p+1) == 0) {
			name = _Py_PackageContext;
			_Py_PackageContext = NULL;
		}
	}
	if ((m = PyImport_AddModule(name)) == NULL)
		return NULL;
	d = PyModule_GetDict(m);
	if (methods != NULL) {
		n = PyString_FromString(name);
		if (n == NULL)
			return NULL;
		for (ml = methods; ml->ml_name != NULL; ml++) {
			if ((ml->ml_flags & METH_CLASS) ||
			    (ml->ml_flags & METH_STATIC)) {
				PyErr_SetString(PyExc_ValueError,
						"module functions cannot set"
						" METH_CLASS or METH_STATIC");
				return NULL;
			}
			v = PyCFunction_NewEx(ml, passthrough, n);
			if (v == NULL)
				return NULL;
			if (PyDict_SetItemString(d, ml->ml_name, v) != 0) {
				Py_DECREF(v);
				return NULL;
			}
			Py_DECREF(v);
		}
	}
	if (doc != NULL) {
		v = PyString_FromString(doc);
		if (v == NULL || PyDict_SetItemString(d, "__doc__", v) != 0) {
			Py_XDECREF(v);
			return NULL;
		}
		Py_DECREF(v);
	}
	return m;
}


/* Helper for mkvalue() to scan the length of a format */

static int
countformat(char *format, int endchar)
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

static PyObject *do_mktuple(char**, va_list *, int, int);
static PyObject *do_mklist(char**, va_list *, int, int);
static PyObject *do_mkdict(char**, va_list *, int, int);
static PyObject *do_mkvalue(char**, va_list *);


static PyObject *
do_mkdict(char **p_format, va_list *p_va, int endchar, int n)
{
	PyObject *d;
	int i;
	int itemfailed = 0;
	if (n < 0)
		return NULL;
	if ((d = PyDict_New()) == NULL)
		return NULL;
	/* Note that we can't bail immediately on error as this will leak
	   refcounts on any 'N' arguments. */
	for (i = 0; i < n; i+= 2) {
		PyObject *k, *v;
		int err;
		k = do_mkvalue(p_format, p_va);
		if (k == NULL) {
			itemfailed = 1;
			Py_INCREF(Py_None);
			k = Py_None;
		}
		v = do_mkvalue(p_format, p_va);
		if (v == NULL) {
			itemfailed = 1;
			Py_INCREF(Py_None);
			v = Py_None;
		}
		err = PyDict_SetItem(d, k, v);
		Py_DECREF(k);
		Py_DECREF(v);
		if (err < 0 || itemfailed) {
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
do_mklist(char **p_format, va_list *p_va, int endchar, int n)
{
	PyObject *v;
	int i;
	int itemfailed = 0;
	if (n < 0)
		return NULL;
	if ((v = PyList_New(n)) == NULL)
		return NULL;
	/* Note that we can't bail immediately on error as this will leak
	   refcounts on any 'N' arguments. */
	for (i = 0; i < n; i++) {
		PyObject *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			itemfailed = 1;
			Py_INCREF(Py_None);
			w = Py_None;
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
	if (itemfailed) {
		Py_DECREF(v);
		v = NULL;
	}
	return v;
}

#ifdef Py_USING_UNICODE
static int
_ustrlen(Py_UNICODE *u)
{
	int i = 0;
	Py_UNICODE *v = u;
	while (*v != 0) { i++; v++; } 
	return i;
}
#endif

static PyObject *
do_mktuple(char **p_format, va_list *p_va, int endchar, int n)
{
	PyObject *v;
	int i;
	int itemfailed = 0;
	if (n < 0)
		return NULL;
	if ((v = PyTuple_New(n)) == NULL)
		return NULL;
	/* Note that we can't bail immediately on error as this will leak
	   refcounts on any 'N' arguments. */
	for (i = 0; i < n; i++) {
		PyObject *w = do_mkvalue(p_format, p_va);
		if (w == NULL) {
			itemfailed = 1;
			Py_INCREF(Py_None);
			w = Py_None;
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
	if (itemfailed) {
		Py_DECREF(v);
		v = NULL;
	}
	return v;
}

static PyObject *
do_mkvalue(char **p_format, va_list *p_va)
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
		case 'B':
		case 'h':
		case 'i':
			return PyInt_FromLong((long)va_arg(*p_va, int));
			
		case 'H':
			return PyInt_FromLong((long)va_arg(*p_va, unsigned int));

		case 'l':
			return PyInt_FromLong((long)va_arg(*p_va, long));

		case 'k':
			return PyInt_FromLong((long)va_arg(*p_va, unsigned long));

#ifdef HAVE_LONG_LONG
		case 'L':
			return PyLong_FromLongLong((PY_LONG_LONG)va_arg(*p_va, PY_LONG_LONG));

		case 'K':
			return PyLong_FromLongLong((PY_LONG_LONG)va_arg(*p_va, unsigned PY_LONG_LONG));
#endif
#ifdef Py_USING_UNICODE
		case 'u':
		{
			PyObject *v;
			Py_UNICODE *u = va_arg(*p_va, Py_UNICODE *);
			int n;
			if (**p_format == '#') {
				++*p_format;
				n = va_arg(*p_va, int);
			}
			else
				n = -1;
			if (u == NULL) {
				v = Py_None;
				Py_INCREF(v);
			}
			else {
				if (n < 0)
					n = _ustrlen(u);
				v = PyUnicode_FromUnicode(u, n);
			}
			return v;
		}
#endif
		case 'f':
		case 'd':
			return PyFloat_FromDouble(
				(double)va_arg(*p_va, va_double));

#ifndef WITHOUT_COMPLEX
		case 'D':
			return PyComplex_FromCComplex(
				*((Py_complex *)va_arg(*p_va, Py_complex *)));
#endif /* WITHOUT_COMPLEX */

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
				if (n < 0) {
					size_t m = strlen(str);
					if (m > INT_MAX) {
						PyErr_SetString(PyExc_OverflowError,
							"string too long for Python string");
						return NULL;
					}
					n = (int)m;
				}
				v = PyString_FromStringAndSize(str, n);
			}
			return v;
		}

		case 'N':
		case 'S':
		case 'O':
		if (**p_format == '&') {
			typedef PyObject *(*converter)(void *);
			converter func = va_arg(*p_va, converter);
			void *arg = va_arg(*p_va, void *);
			++*p_format;
			return (*func)(arg);
		}
		else {
			PyObject *v;
			v = va_arg(*p_va, PyObject *);
			if (v != NULL) {
				if (*(*p_format - 1) != 'N')
					Py_INCREF(v);
			}
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
					"NULL object passed to Py_BuildValue");
			return v;
		}

		case ':':
		case ',':
		case ' ':
		case '\t':
			break;

		default:
			PyErr_SetString(PyExc_SystemError,
				"bad format char passed to Py_BuildValue");
			return NULL;

		}
	}
}


PyObject *
Py_BuildValue(char *format, ...)
{
	va_list va;
	PyObject* retval;
	va_start(va, format);
	retval = Py_VaBuildValue(format, va);
	va_end(va);
	return retval;
}

PyObject *
Py_VaBuildValue(char *format, va_list va)
{
	char *f = format;
	int n = countformat(f, '\0');
	va_list lva;

#ifdef VA_LIST_IS_ARRAY
	memcpy(lva, va, sizeof(va_list));
#else
#ifdef __va_copy
	__va_copy(lva, va);
#else
	lva = va;
#endif
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


PyObject *
PyEval_CallFunction(PyObject *obj, char *format, ...)
{
	va_list vargs;
	PyObject *args;
	PyObject *res;

	va_start(vargs, format);

	args = Py_VaBuildValue(format, vargs);
	va_end(vargs);

	if (args == NULL)
		return NULL;

	res = PyEval_CallObject(obj, args);
	Py_DECREF(args);

	return res;
}


PyObject *
PyEval_CallMethod(PyObject *obj, char *methodname, char *format, ...)
{
	va_list vargs;
	PyObject *meth;
	PyObject *args;
	PyObject *res;

	meth = PyObject_GetAttrString(obj, methodname);
	if (meth == NULL)
		return NULL;

	va_start(vargs, format);

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

int
PyModule_AddObject(PyObject *m, char *name, PyObject *o)
{
	PyObject *dict;
	if (!PyModule_Check(m)) {
		PyErr_SetString(PyExc_TypeError,
			    "PyModule_AddObject() needs module as first arg");
		return -1;
	}
	if (!o) {
		if (!PyErr_Occurred())
			PyErr_SetString(PyExc_TypeError,
					"PyModule_AddObject() needs non-NULL value");
		return -1;
	}

	dict = PyModule_GetDict(m);
	if (dict == NULL) {
		/* Internal error -- modules must have a dict! */
		PyErr_Format(PyExc_SystemError, "module '%s' has no __dict__",
			     PyModule_GetName(m));
		return -1;
	}
	if (PyDict_SetItemString(dict, name, o))
		return -1;
	Py_DECREF(o);
	return 0;
}

int 
PyModule_AddIntConstant(PyObject *m, char *name, long value)
{
	return PyModule_AddObject(m, name, PyInt_FromLong(value));
}

int 
PyModule_AddStringConstant(PyObject *m, char *name, char *value)
{
	return PyModule_AddObject(m, name, PyString_FromString(value));
}
