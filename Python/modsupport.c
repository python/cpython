
/* Module support implementation */

#include "Python.h"

#ifdef MPW /* MPW pushes 'extended' for float and double types with varargs */
typedef extended va_double;
#else
typedef double va_double;
#endif

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
"WARNING: Python C API version mismatch for module %s:\n\
  This Python has API version %d, module %s has version %d.\n";

PyObject *
Py_InitModule4(char *name, PyMethodDef *methods, char *doc,
	       PyObject *passthrough, int module_api_version)
{
	PyObject *m, *d, *v;
	PyMethodDef *ml;
	if (!Py_IsInitialized())
	    Py_FatalError("Interpreter not initialized (version mismatch?)");
	if (module_api_version != PYTHON_API_VERSION)
		fprintf(stderr, api_version_warning,
			name, PYTHON_API_VERSION, name, module_api_version);
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

static int countformat(char *format, int endchar)
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
	if (n < 0)
		return NULL;
	if ((d = PyDict_New()) == NULL)
		return NULL;
	for (i = 0; i < n; i+= 2) {
		PyObject *k, *v;
		int err;
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
		err = PyDict_SetItem(d, k, v);
		Py_DECREF(k);
		Py_DECREF(v);
		if (err < 0) {
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

static int
_ustrlen(Py_UNICODE *u)
{
	int i = 0;
	Py_UNICODE *v = u;
	while (*v != 0) { i++; v++; } 
	return i;
}

static PyObject *
do_mktuple(char **p_format, va_list *p_va, int endchar, int n)
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

#ifdef HAVE_LONG_LONG
		case 'L':
			return PyLong_FromLongLong((LONG_LONG)va_arg(*p_va, LONG_LONG));
#endif
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


PyObject *Py_BuildValue(char *format, ...)
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
        if (!PyModule_Check(m) || o == NULL)
                return -1;
	dict = PyModule_GetDict(m);
	if (dict == NULL)
		return -1;
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
