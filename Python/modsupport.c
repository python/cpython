
/* Module support implementation */

#include "Python.h"

#define FLAG_SIZE_T 1
typedef double va_double;

static PyObject *va_build_value(const char *, va_list, int);

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
Py_InitModule4(const char *name, PyMethodDef *methods, const char *doc,
               PyObject *passthrough, int module_api_version)
{
    PyObject *m, *d, *v, *n;
    PyMethodDef *ml;
    PyInterpreterState *interp = PyThreadState_Get()->interp;
    if (interp->modules == NULL)
        Py_FatalError("Python import machinery not initialized");
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
                Py_DECREF(n);
                return NULL;
            }
            v = PyCFunction_NewEx(ml, passthrough, n);
            if (v == NULL) {
                Py_DECREF(n);
                return NULL;
            }
            if (PyDict_SetItemString(d, ml->ml_name, v) != 0) {
                Py_DECREF(v);
                Py_DECREF(n);
                return NULL;
            }
            Py_DECREF(v);
        }
        Py_DECREF(n);
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
countformat(const char *format, int endchar)
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

static PyObject *do_mktuple(const char**, va_list *, int, int, int);
static PyObject *do_mklist(const char**, va_list *, int, int, int);
static PyObject *do_mkdict(const char**, va_list *, int, int, int);
static PyObject *do_mkvalue(const char**, va_list *, int);


static void
do_ignore(const char **p_format, va_list *p_va, int endchar, int n, int flags)
{
    PyObject *v;
    int i;
    assert(PyErr_Occurred());
    v = PyTuple_New(n);
    for (i = 0; i < n; i++) {
        PyObject *exception, *value, *tb, *w;
        PyErr_Fetch(&exception, &value, &tb);
        w = do_mkvalue(p_format, p_va, flags);
        PyErr_Restore(exception, value, tb);
        if (w != NULL) {
            if (v != NULL) {
                PyTuple_SET_ITEM(v, i, w);
            }
            else {
                Py_DECREF(w);
            }
        }
    }
    Py_XDECREF(v);
    if (**p_format != endchar) {
        PyErr_SetString(PyExc_SystemError,
                        "Unmatched paren in format");
        return;
    }
    if (endchar)
        ++*p_format;
}

static PyObject *
do_mkdict(const char **p_format, va_list *p_va, int endchar, int n, int flags)
{
    PyObject *d;
    int i;
    if (n < 0)
        return NULL;
    if (n % 2) {
        PyErr_SetString(PyExc_SystemError,
                        "Bad dict format");
        do_ignore(p_format, p_va, endchar, n, flags);
        return NULL;
    }
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    if ((d = PyDict_New()) == NULL) {
        do_ignore(p_format, p_va, endchar, n, flags);
        return NULL;
    }
    for (i = 0; i < n; i+= 2) {
        PyObject *k, *v;

        k = do_mkvalue(p_format, p_va, flags);
        if (k == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1, flags);
            Py_DECREF(d);
            return NULL;
        }
        v = do_mkvalue(p_format, p_va, flags);
        if (v == NULL || PyDict_SetItem(d, k, v) < 0) {
            do_ignore(p_format, p_va, endchar, n - i - 2, flags);
            Py_DECREF(k);
            Py_XDECREF(v);
            Py_DECREF(d);
            return NULL;
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
    if (**p_format != endchar) {
        Py_DECREF(d);
        PyErr_SetString(PyExc_SystemError,
                        "Unmatched paren in format");
        return NULL;
    }
    if (endchar)
        ++*p_format;
    return d;
}

static PyObject *
do_mklist(const char **p_format, va_list *p_va, int endchar, int n, int flags)
{
    PyObject *v;
    int i;
    if (n < 0)
        return NULL;
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    v = PyList_New(n);
    if (v == NULL) {
        do_ignore(p_format, p_va, endchar, n, flags);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        PyObject *w = do_mkvalue(p_format, p_va, flags);
        if (w == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1, flags);
            Py_DECREF(v);
            return NULL;
        }
        PyList_SET_ITEM(v, i, w);
    }
    if (**p_format != endchar) {
        Py_DECREF(v);
        PyErr_SetString(PyExc_SystemError,
                        "Unmatched paren in format");
        return NULL;
    }
    if (endchar)
        ++*p_format;
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
do_mktuple(const char **p_format, va_list *p_va, int endchar, int n, int flags)
{
    PyObject *v;
    int i;
    if (n < 0)
        return NULL;
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    if ((v = PyTuple_New(n)) == NULL) {
        do_ignore(p_format, p_va, endchar, n, flags);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        PyObject *w = do_mkvalue(p_format, p_va, flags);
        if (w == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1, flags);
            Py_DECREF(v);
            return NULL;
        }
        PyTuple_SET_ITEM(v, i, w);
    }
    if (**p_format != endchar) {
        Py_DECREF(v);
        PyErr_SetString(PyExc_SystemError,
                        "Unmatched paren in format");
        return NULL;
    }
    if (endchar)
        ++*p_format;
    return v;
}

static PyObject *
do_mkvalue(const char **p_format, va_list *p_va, int flags)
{
    for (;;) {
        switch (*(*p_format)++) {
        case '(':
            return do_mktuple(p_format, p_va, ')',
                              countformat(*p_format, ')'), flags);

        case '[':
            return do_mklist(p_format, p_va, ']',
                             countformat(*p_format, ']'), flags);

        case '{':
            return do_mkdict(p_format, p_va, '}',
                             countformat(*p_format, '}'), flags);

        case 'b':
        case 'B':
        case 'h':
        case 'i':
            return PyInt_FromLong((long)va_arg(*p_va, int));

        case 'H':
            return PyInt_FromLong((long)va_arg(*p_va, unsigned int));

        case 'I':
        {
            unsigned int n;
            n = va_arg(*p_va, unsigned int);
            if (n > (unsigned long)PyInt_GetMax())
                return PyLong_FromUnsignedLong((unsigned long)n);
            else
                return PyInt_FromLong(n);
        }

        case 'n':
#if SIZEOF_SIZE_T!=SIZEOF_LONG
            return PyInt_FromSsize_t(va_arg(*p_va, Py_ssize_t));
#endif
            /* Fall through from 'n' to 'l' if Py_ssize_t is long */
        case 'l':
            return PyInt_FromLong(va_arg(*p_va, long));

        case 'k':
        {
            unsigned long n;
            n = va_arg(*p_va, unsigned long);
            if (n > (unsigned long)PyInt_GetMax())
                return PyLong_FromUnsignedLong(n);
            else
                return PyInt_FromLong(n);
        }

#ifdef HAVE_LONG_LONG
        case 'L':
            return PyLong_FromLongLong((PY_LONG_LONG)va_arg(*p_va, PY_LONG_LONG));

        case 'K':
            return PyLong_FromUnsignedLongLong((PY_LONG_LONG)va_arg(*p_va, unsigned PY_LONG_LONG));
#endif
#ifdef Py_USING_UNICODE
        case 'u':
        {
            PyObject *v;
            Py_UNICODE *u = va_arg(*p_va, Py_UNICODE *);
            Py_ssize_t n;
            if (**p_format == '#') {
                ++*p_format;
                if (flags & FLAG_SIZE_T)
                    n = va_arg(*p_va, Py_ssize_t);
                else
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
            p[0] = (char)va_arg(*p_va, int);
            return PyString_FromStringAndSize(p, 1);
        }

        case 's':
        case 'z':
        {
            PyObject *v;
            char *str = va_arg(*p_va, char *);
            Py_ssize_t n;
            if (**p_format == '#') {
                ++*p_format;
                if (flags & FLAG_SIZE_T)
                    n = va_arg(*p_va, Py_ssize_t);
                else
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
                    if (m > PY_SSIZE_T_MAX) {
                        PyErr_SetString(PyExc_OverflowError,
                            "string too long for Python string");
                        return NULL;
                    }
                    n = (Py_ssize_t)m;
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
Py_BuildValue(const char *format, ...)
{
    va_list va;
    PyObject* retval;
    va_start(va, format);
    retval = va_build_value(format, va, 0);
    va_end(va);
    return retval;
}

PyObject *
_Py_BuildValue_SizeT(const char *format, ...)
{
    va_list va;
    PyObject* retval;
    va_start(va, format);
    retval = va_build_value(format, va, FLAG_SIZE_T);
    va_end(va);
    return retval;
}

PyObject *
Py_VaBuildValue(const char *format, va_list va)
{
    return va_build_value(format, va, 0);
}

PyObject *
_Py_VaBuildValue_SizeT(const char *format, va_list va)
{
    return va_build_value(format, va, FLAG_SIZE_T);
}

static PyObject *
va_build_value(const char *format, va_list va, int flags)
{
    const char *f = format;
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
        return do_mkvalue(&f, &lva, flags);
    return do_mktuple(&f, &lva, '\0', n, flags);
}


PyObject *
PyEval_CallFunction(PyObject *obj, const char *format, ...)
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
PyEval_CallMethod(PyObject *obj, const char *methodname, const char *format, ...)
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
PyModule_AddObject(PyObject *m, const char *name, PyObject *o)
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
PyModule_AddIntConstant(PyObject *m, const char *name, long value)
{
    PyObject *o = PyInt_FromLong(value);
    if (!o)
        return -1;
    if (PyModule_AddObject(m, name, o) == 0)
        return 0;
    Py_DECREF(o);
    return -1;
}

int
PyModule_AddStringConstant(PyObject *m, const char *name, const char *value)
{
    PyObject *o = PyString_FromString(value);
    if (!o)
        return -1;
    if (PyModule_AddObject(m, name, o) == 0)
        return 0;
    Py_DECREF(o);
    return -1;
}
