
/* Module support implementation */

#include "Python.h"
#include "pycore_abstract.h"   // _PyIndex_Check()
#include "pycore_object.h"     // _PyType_IsReady()
#include "pycore_unicodeobject.h"     // _PyUnicodeWriter_FormatV()

typedef double va_double;

static PyObject *va_build_value(const char *, va_list);


int
_Py_convert_optional_to_ssize_t(PyObject *obj, void *result)
{
    Py_ssize_t limit;
    if (obj == Py_None) {
        return 1;
    }
    else if (_PyIndex_Check(obj)) {
        limit = PyNumber_AsSsize_t(obj, PyExc_OverflowError);
        if (limit == -1 && PyErr_Occurred()) {
            return 0;
        }
    }
    else {
        PyErr_Format(PyExc_TypeError,
                     "argument should be integer or None, not '%.200s'",
                     Py_TYPE(obj)->tp_name);
        return 0;
    }
    *((Py_ssize_t *)result) = limit;
    return 1;
}

int
_Py_convert_optional_to_non_negative_ssize_t(PyObject *obj, void *result)
{
    if (!_Py_convert_optional_to_ssize_t(obj, result)) {
        return 0;
    }
    if (obj != Py_None && *((Py_ssize_t *)result) < 0) {
        PyErr_SetString(PyExc_ValueError, "argument cannot be negative");
        return 0;
    }
    return 1;
}


/* Helper for mkvalue() to scan the length of a format */

static Py_ssize_t
countformat(const char *format, char endchar)
{
    Py_ssize_t count = 0;
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
            if (level == 0) {
                count++;
            }
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
            if (level == 0) {
                count++;
            }
        }
        format++;
    }
    return count;
}


/* Generic function to create a value -- the inverse of getargs() */
/* After an original idea and first implementation by Steven Miale */

static PyObject *do_mktuple(const char**, va_list *, char, Py_ssize_t);
static int do_mkstack(PyObject **, const char**, va_list *, char, Py_ssize_t);
static PyObject *do_mklist(const char**, va_list *, char, Py_ssize_t);
static PyObject *do_mkdict(const char**, va_list *, char, Py_ssize_t);
static PyObject *do_mkvalue(const char**, va_list *);

static int
check_end(const char **p_format, char endchar)
{
    const char *f = *p_format;
    while (*f != endchar) {
        if (*f != ' ' && *f != '\t' && *f != ',' && *f != ':') {
            PyErr_SetString(PyExc_SystemError,
                            "Unmatched paren in format");
            return 0;
        }
        f++;
    }
    if (endchar) {
        f++;
    }
    *p_format = f;
    return 1;
}

static void
do_ignore(const char **p_format, va_list *p_va, char endchar, Py_ssize_t n)
{
    assert(PyErr_Occurred());
    PyObject *v = PyTuple_New(n);
    for (Py_ssize_t i = 0; i < n; i++) {
        PyObject *exc = PyErr_GetRaisedException();
        PyObject *w = do_mkvalue(p_format, p_va);
        PyErr_SetRaisedException(exc);
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
    if (!check_end(p_format, endchar)) {
        return;
    }
}

static PyObject *
do_mkdict(const char **p_format, va_list *p_va, char endchar, Py_ssize_t n)
{
    PyObject *d;
    Py_ssize_t i;
    if (n < 0)
        return NULL;
    if (n % 2) {
        PyErr_SetString(PyExc_SystemError,
                        "Bad dict format");
        do_ignore(p_format, p_va, endchar, n);
        return NULL;
    }
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    if ((d = PyDict_New()) == NULL) {
        do_ignore(p_format, p_va, endchar, n);
        return NULL;
    }
    for (i = 0; i < n; i+= 2) {
        PyObject *k, *v;

        k = do_mkvalue(p_format, p_va);
        if (k == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1);
            Py_DECREF(d);
            return NULL;
        }
        v = do_mkvalue(p_format, p_va);
        if (v == NULL || PyDict_SetItem(d, k, v) < 0) {
            do_ignore(p_format, p_va, endchar, n - i - 2);
            Py_DECREF(k);
            Py_XDECREF(v);
            Py_DECREF(d);
            return NULL;
        }
        Py_DECREF(k);
        Py_DECREF(v);
    }
    if (!check_end(p_format, endchar)) {
        Py_DECREF(d);
        return NULL;
    }
    return d;
}

static PyObject *
do_mklist(const char **p_format, va_list *p_va, char endchar, Py_ssize_t n)
{
    PyObject *v;
    Py_ssize_t i;
    if (n < 0)
        return NULL;
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    v = PyList_New(n);
    if (v == NULL) {
        do_ignore(p_format, p_va, endchar, n);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        PyObject *w = do_mkvalue(p_format, p_va);
        if (w == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1);
            Py_DECREF(v);
            return NULL;
        }
        PyList_SET_ITEM(v, i, w);
    }
    if (!check_end(p_format, endchar)) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
}

static int
do_mkstack(PyObject **stack, const char **p_format, va_list *p_va,
           char endchar, Py_ssize_t n)
{
    Py_ssize_t i;

    if (n < 0) {
        return -1;
    }
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    for (i = 0; i < n; i++) {
        PyObject *w = do_mkvalue(p_format, p_va);
        if (w == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1);
            goto error;
        }
        stack[i] = w;
    }
    if (!check_end(p_format, endchar)) {
        goto error;
    }
    return 0;

error:
    n = i;
    for (i=0; i < n; i++) {
        Py_DECREF(stack[i]);
    }
    return -1;
}

static PyObject *
do_mktuple(const char **p_format, va_list *p_va, char endchar, Py_ssize_t n)
{
    PyObject *v;
    Py_ssize_t i;
    if (n < 0)
        return NULL;
    /* Note that we can't bail immediately on error as this will leak
       refcounts on any 'N' arguments. */
    if ((v = PyTuple_New(n)) == NULL) {
        do_ignore(p_format, p_va, endchar, n);
        return NULL;
    }
    for (i = 0; i < n; i++) {
        PyObject *w = do_mkvalue(p_format, p_va);
        if (w == NULL) {
            do_ignore(p_format, p_va, endchar, n - i - 1);
            Py_DECREF(v);
            return NULL;
        }
        PyTuple_SET_ITEM(v, i, w);
    }
    if (!check_end(p_format, endchar)) {
        Py_DECREF(v);
        return NULL;
    }
    return v;
}

static PyObject *
do_mkvalue(const char **p_format, va_list *p_va)
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
            return PyLong_FromLong((long)va_arg(*p_va, int));

        case 'H':
            return PyLong_FromLong((long)va_arg(*p_va, unsigned int));

        case 'I':
        {
            unsigned int n;
            n = va_arg(*p_va, unsigned int);
            return PyLong_FromUnsignedLong(n);
        }

        case 'n':
#if SIZEOF_SIZE_T!=SIZEOF_LONG
            return PyLong_FromSsize_t(va_arg(*p_va, Py_ssize_t));
#endif
            /* Fall through from 'n' to 'l' if Py_ssize_t is long */
            _Py_FALLTHROUGH;
        case 'l':
            return PyLong_FromLong(va_arg(*p_va, long));

        case 'k':
        {
            unsigned long n;
            n = va_arg(*p_va, unsigned long);
            return PyLong_FromUnsignedLong(n);
        }

        case 'L':
            return PyLong_FromLongLong((long long)va_arg(*p_va, long long));

        case 'K':
            return PyLong_FromUnsignedLongLong(
                va_arg(*p_va, unsigned long long));

        case 'u':
        {
            PyObject *v;
            const wchar_t *u = va_arg(*p_va, wchar_t*);
            Py_ssize_t n;
            if (**p_format == '#') {
                ++*p_format;
                n = va_arg(*p_va, Py_ssize_t);
            }
            else
                n = -1;
            if (u == NULL) {
                v = Py_NewRef(Py_None);
            }
            else {
                if (n < 0)
                    n = wcslen(u);
                v = PyUnicode_FromWideChar(u, n);
            }
            return v;
        }
        case 'f':
        case 'd':
            return PyFloat_FromDouble(
                (double)va_arg(*p_va, va_double));

        case 'D':
            return PyComplex_FromCComplex(
                *((Py_complex *)va_arg(*p_va, Py_complex *)));

        case 'c':
        {
            char p[1];
            p[0] = (char)va_arg(*p_va, int);
            return PyBytes_FromStringAndSize(p, 1);
        }
        case 'C':
        {
            int i = va_arg(*p_va, int);
            return PyUnicode_FromOrdinal(i);
        }
        case 'p':
        {
            int i = va_arg(*p_va, int);
            return PyBool_FromLong(i);
        }

        case 's':
        case 'z':
        case 'U':   /* XXX deprecated alias */
        {
            PyObject *v;
            const char *str = va_arg(*p_va, const char *);
            Py_ssize_t n;
            if (**p_format == '#') {
                ++*p_format;
                n = va_arg(*p_va, Py_ssize_t);
            }
            else
                n = -1;
            if (str == NULL) {
                v = Py_NewRef(Py_None);
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
                v = PyUnicode_FromStringAndSize(str, n);
            }
            return v;
        }

        case 'y':
        {
            PyObject *v;
            const char *str = va_arg(*p_va, const char *);
            Py_ssize_t n;
            if (**p_format == '#') {
                ++*p_format;
                n = va_arg(*p_va, Py_ssize_t);
            }
            else
                n = -1;
            if (str == NULL) {
                v = Py_NewRef(Py_None);
            }
            else {
                if (n < 0) {
                    size_t m = strlen(str);
                    if (m > PY_SSIZE_T_MAX) {
                        PyErr_SetString(PyExc_OverflowError,
                            "string too long for Python bytes");
                        return NULL;
                    }
                    n = (Py_ssize_t)m;
                }
                v = PyBytes_FromStringAndSize(str, n);
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
    retval = va_build_value(format, va);
    va_end(va);
    return retval;
}

PyAPI_FUNC(PyObject *) /* abi only */
_Py_BuildValue_SizeT(const char *format, ...)
{
    va_list va;
    PyObject* retval;
    va_start(va, format);
    retval = va_build_value(format, va);
    va_end(va);
    return retval;
}

PyObject *
Py_VaBuildValue(const char *format, va_list va)
{
    return va_build_value(format, va);
}

PyAPI_FUNC(PyObject *) /* abi only */
_Py_VaBuildValue_SizeT(const char *format, va_list va)
{
    return va_build_value(format, va);
}

static PyObject *
va_build_value(const char *format, va_list va)
{
    const char *f = format;
    Py_ssize_t n = countformat(f, '\0');
    va_list lva;
    PyObject *retval;

    if (n < 0)
        return NULL;
    if (n == 0) {
        Py_RETURN_NONE;
    }
    va_copy(lva, va);
    if (n == 1) {
        retval = do_mkvalue(&f, &lva);
    } else {
        retval = do_mktuple(&f, &lva, '\0', n);
    }
    va_end(lva);
    return retval;
}

PyObject **
_Py_VaBuildStack(PyObject **small_stack, Py_ssize_t small_stack_len,
                const char *format, va_list va, Py_ssize_t *p_nargs)
{
    const char *f;
    Py_ssize_t n;
    va_list lva;
    PyObject **stack;
    int res;

    n = countformat(format, '\0');
    if (n < 0) {
        *p_nargs = 0;
        return NULL;
    }

    if (n == 0) {
        *p_nargs = 0;
        return small_stack;
    }

    if (n <= small_stack_len) {
        stack = small_stack;
    }
    else {
        stack = PyMem_Malloc(n * sizeof(stack[0]));
        if (stack == NULL) {
            PyErr_NoMemory();
            return NULL;
        }
    }

    va_copy(lva, va);
    f = format;
    res = do_mkstack(stack, &f, &lva, '\0', n);
    va_end(lva);

    if (res < 0) {
        if (stack != small_stack) {
            PyMem_Free(stack);
        }
        return NULL;
    }

    *p_nargs = n;
    return stack;
}


int
PyModule_AddObjectRef(PyObject *mod, const char *name, PyObject *value)
{
    if (!PyModule_Check(mod)) {
        PyErr_SetString(PyExc_TypeError,
                        "PyModule_AddObjectRef() first argument "
                        "must be a module");
        return -1;
    }
    if (!value) {
        if (!PyErr_Occurred()) {
            PyErr_SetString(PyExc_SystemError,
                            "PyModule_AddObjectRef() must be called "
                            "with an exception raised if value is NULL");
        }
        return -1;
    }

    PyObject *dict = PyModule_GetDict(mod);
    if (dict == NULL) {
        /* Internal error -- modules must have a dict! */
        PyErr_Format(PyExc_SystemError, "module '%s' has no __dict__",
                     PyModule_GetName(mod));
        return -1;
    }
    return PyDict_SetItemString(dict, name, value);
}

int
PyModule_Add(PyObject *mod, const char *name, PyObject *value)
{
    int res = PyModule_AddObjectRef(mod, name, value);
    Py_XDECREF(value);
    return res;
}

int
PyModule_AddObject(PyObject *mod, const char *name, PyObject *value)
{
    int res = PyModule_AddObjectRef(mod, name, value);
    if (res == 0) {
        Py_DECREF(value);
    }
    return res;
}

int
PyModule_AddIntConstant(PyObject *m, const char *name, long value)
{
    return PyModule_Add(m, name, PyLong_FromLong(value));
}

int
PyModule_AddStringConstant(PyObject *m, const char *name, const char *value)
{
    return PyModule_Add(m, name, PyUnicode_FromString(value));
}

int
PyModule_AddType(PyObject *module, PyTypeObject *type)
{
    if (!_PyType_IsReady(type) && PyType_Ready(type) < 0) {
        return -1;
    }

    const char *name = _PyType_Name(type);
    assert(name != NULL);

    return PyModule_AddObjectRef(module, name, (PyObject *)type);
}

static int _abiinfo_raise(const char *module_name, const char *format, ...)
{
    PyUnicodeWriter *writer = PyUnicodeWriter_Create(0);
    if (!writer) {
        return -1;
    }
    if (module_name) {
        if (PyUnicodeWriter_WriteUTF8(writer, module_name, -1) < 0) {
            PyUnicodeWriter_Discard(writer);
            return -1;
        }
        if (PyUnicodeWriter_WriteASCII(writer, ": ", 2) < 0) {
            PyUnicodeWriter_Discard(writer);
            return -1;
        }
    }
    va_list vargs;
    va_start(vargs, format);
    if (_PyUnicodeWriter_FormatV(writer, format, vargs) < 0) {
        PyUnicodeWriter_Discard(writer);
        return -1;
    }
    PyObject *message = PyUnicodeWriter_Finish(writer);
    if (!message) {
        return -1;
    }
    PyErr_SetObject(PyExc_ImportError, message);
    Py_DECREF(message);
    return -1;
}

int PyABIInfo_Check(PyABIInfo *info, const char *module_name)
{
    if (!info) {
        return _abiinfo_raise(module_name, "NULL PyABIInfo");
    }

    /* abiinfo_major_version */
    if (info->abiinfo_major_version == 0) {
        return 0;
    }
    if (info->abiinfo_major_version > 1) {
        return _abiinfo_raise(module_name, "PyABIInfo version too high");
    }

    /* Internal ABI */
    if (info->flags & PyABIInfo_INTERNAL) {
        if (info->abi_version && (info->abi_version != PY_VERSION_HEX)) {
            return _abiinfo_raise(
                module_name,
                "incompatible internal ABI (0x%x != 0x%x)",
                info->abi_version, PY_VERSION_HEX);
        }
    }

#define XY_MASK 0xffff0000
    if (info->flags & PyABIInfo_STABLE) {
        /* Greater-than major.minor version check */
        if (info->abi_version) {
            if ((info->abi_version & XY_MASK) > (PY_VERSION_HEX & XY_MASK)) {
                return _abiinfo_raise(
                    module_name,
                    "incompatible future stable ABI version (%d.%d)",
                    ((info->abi_version) >> 24) % 0xff,
                    ((info->abi_version) >> 16) % 0xff);
            }
            if (info->abi_version < Py_PACK_VERSION(3, 2)) {
                return _abiinfo_raise(
                    module_name,
                    "invalid stable ABI version (%d.%d)",
                    ((info->abi_version) >> 24) % 0xff,
                    ((info->abi_version) >> 16) % 0xff);
            }
        }
        if (info->flags & PyABIInfo_INTERNAL) {
            return _abiinfo_raise(module_name,
                                  "cannot use both internal and stable ABI");
        }
    }
    else {
        /* Exact major.minor version check */
        if (info->abi_version) {
            if ((info->abi_version & XY_MASK) != (PY_VERSION_HEX & XY_MASK)) {
                return _abiinfo_raise(
                    module_name,
                    "incompatible ABI version (%d.%d)",
                    ((info->abi_version) >> 24) % 0xff,
                    ((info->abi_version) >> 16) % 0xff);
            }
        }
    }
#undef XY_MASK

    /* Free-threading/GIL */
    uint16_t gilflags = info->flags & (PyABIInfo_GIL | PyABIInfo_FREETHREADED);
#if Py_GIL_DISABLED
    if (gilflags == PyABIInfo_GIL) {
        return _abiinfo_raise(module_name,
                              "incompatible with free-threaded CPython");
    }
#else
    if (gilflags == PyABIInfo_FREETHREADED) {
        return _abiinfo_raise(module_name,
                              "only compatible with free-threaded CPython");
    }
#endif

    return 0;
}


/* Exported functions for version helper macros */

#undef Py_PACK_FULL_VERSION
uint32_t
Py_PACK_FULL_VERSION(int x, int y, int z, int level, int serial)
{
    return _Py_PACK_FULL_VERSION(x, y, z, level, serial);
}

#undef Py_PACK_VERSION
uint32_t
Py_PACK_VERSION(int x, int y)
{
    return _Py_PACK_VERSION(x, y);
}
