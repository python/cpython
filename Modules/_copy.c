#include "Python.h"

/*[clinic input]
module _copy
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=b34c1b75f49dbfff]*/

/*
 * Duplicate of builtin_id. Looking around the CPython code base, it seems to
 * be quite common to just open-code this as PyLong_FromVoidPtr, though I'm
 * not sure those cases actually need to interoperate with Python code that
 * uses id(). We do, however, so it would be nicer there was an official
 * public API (e.g. PyObject_Id, maybe just a macro to avoid extra
 * indirection) providing this..
 */
static PyObject*
object_id(PyObject* v)
{
    return PyLong_FromVoidPtr(v);
}

static int
memo_keepalive(PyObject* x, PyObject* memo)
{
    PyObject* memoid, * list;
    int ret;

    memoid = object_id(memo);
    if (memoid == NULL)
        return -1;

    /* try: memo[id(memo)].append(x) */
    list = PyDict_GetItem(memo, memoid);
    if (list != NULL) {
        Py_DECREF(memoid);
        return PyList_Append(list, x);
    }

    /* except KeyError: memo[id(memo)] = [x] */
    list = PyList_New(1);
    if (list == NULL) {
        Py_DECREF(memoid);
        return -1;
    }
    Py_INCREF(x);
    PyList_SET_ITEM(list, 0, x);
    ret = PyDict_SetItem(memo, memoid, list);
    Py_DECREF(memoid);
    Py_DECREF(list);
    return ret;
}

/* Forward declaration. */
static PyObject* do_deepcopy(PyObject* x, PyObject* memo);

static PyObject*
do_deepcopy_fallback(PyObject* x, PyObject* memo)
{
    static PyObject* copymodule;
    _Py_IDENTIFIER(_deepcopy_fallback);

    if (copymodule == NULL) {
        copymodule = PyImport_ImportModule("copy");
        if (copymodule == NULL)
            return NULL;
    }

    assert(copymodule != NULL);

    return _PyObject_CallMethodIdObjArgs(copymodule, &PyId__deepcopy_fallback,
        x, memo, NULL);
}

static PyObject*
deepcopy_list(PyObject* x, PyObject* memo, PyObject* id_x, Py_hash_t hash_id_x)
{
    PyObject* y, * elem;
    Py_ssize_t i, size;

    assert(PyList_CheckExact(x));
    size = PyList_GET_SIZE(x);

    /*
     * Make a copy of x, then replace each element with its deepcopy. This
     * avoids building the new list with repeated PyList_Append calls, and
     * also avoids problems that could occur if some user-defined __deepcopy__
     * mutates the source list. However, this doesn't eliminate all possible
     * problems, since Python code can still get its hands on y via the memo,
     * so we're still careful to check 'i < PyList_GET_SIZE(y)' before
     * getting/setting in the loop below.
     */
    y = PyList_GetSlice(x, 0, size);
    if (y == NULL)
        return NULL;
    assert(PyList_CheckExact(y));

    if (_PyDict_SetItem_KnownHash(memo, id_x, y, hash_id_x) < 0) {
        Py_DECREF(y);
        return NULL;
    }

    for (i = 0; i < PyList_GET_SIZE(y); ++i) {
        elem = PyList_GET_ITEM(y, i);
        Py_INCREF(elem);
        Py_SETREF(elem, do_deepcopy(elem, memo));
        if (elem == NULL) {
            Py_DECREF(y);
            return NULL;
        }
        /*
         * This really should not happen, but let's just return what's left in
         * the list.
         */
        if (i >= PyList_GET_SIZE(y)) {
            Py_DECREF(elem);
            break;
        }
        PyList_SetItem(y, i, elem);
    }
    return y;
}

/*
 * Helpers exploiting ma_version_tag. dict_iter_next returns 0 if the dict is
 * exhausted, 1 if the next key/value pair was succesfully fetched, -1 if
 * mutation is detected.
 */
struct dict_iter {
    PyObject* d;
    uint64_t tag;
    Py_ssize_t pos;
};

static void
dict_iter_init(struct dict_iter* di, PyObject* x)
{
    di->d = x;
    di->tag = ((PyDictObject*)x)->ma_version_tag;
    di->pos = 0;
}
static int
dict_iter_next(struct dict_iter* di, PyObject** key, PyObject** val)
{
    int ret = PyDict_Next(di->d, &di->pos, key, val);
    if (((PyDictObject*)di->d)->ma_version_tag != di->tag) {
        PyErr_SetString(PyExc_RuntimeError,
            "dict mutated during iteration");
        return -1;
    }
    return ret;
}

static PyObject*
deepcopy_dict(PyObject* x, PyObject* memo, PyObject* id_x, Py_hash_t hash_id_x)
{
    PyObject* y, * key, * val;
    Py_ssize_t size;
    int ret;
    struct dict_iter di;

    assert(PyDict_CheckExact(x));
    size = PyDict_Size(x);

    y = _PyDict_NewPresized(size);
    if (y == NULL)
        return NULL;

    if (_PyDict_SetItem_KnownHash(memo, id_x, y, hash_id_x) < 0) {
        Py_DECREF(y);
        return NULL;
    }

    dict_iter_init(&di, x);
    while ((ret = dict_iter_next(&di, &key, &val)) > 0) {
        Py_INCREF(key);
        Py_INCREF(val);

        Py_SETREF(key, do_deepcopy(key, memo));
        if (key == NULL) {
            Py_DECREF(val);
            break;
        }
        Py_SETREF(val, do_deepcopy(val, memo));
        if (val == NULL) {
            Py_DECREF(key);
            break;
        }

        ret = PyDict_SetItem(y, key, val);
        Py_DECREF(key);
        Py_DECREF(val);
        if (ret < 0)
            break; /* Shouldn't happen - y is presized */
    }
    /*
     * We're only ok if the iteration ended with ret == 0; otherwise we've
     * either detected mutation, or encountered an error during deepcopying or
     * setting a key/value pair in y.
     */
    if (ret != 0) {
        Py_DECREF(y);
        return NULL;
    }
    return y;
}

static PyObject*
deepcopy_tuple(PyObject* x, PyObject* memo, PyObject* id_x, Py_hash_t hash_id_x)
{
    PyObject* y, * z, * elem, * copy;
    Py_ssize_t i, size;
    int all_identical = 1; /* are all members their own deepcopy? */

    assert(PyTuple_CheckExact(x));
    size = PyTuple_GET_SIZE(x);

    y = PyTuple_New(size);
    if (y == NULL)
        return NULL;

    /*
     * We cannot add y to the memo just yet, since Python code would then be
     * able to observe a tuple with values changing. We do, however, have an
     * advantage over the Python implementation in that we can actually build
     * the tuple directly instead of using an intermediate list object.
     */
    for (i = 0; i < size; ++i) {
        elem = PyTuple_GET_ITEM(x, i);
        copy = do_deepcopy(elem, memo);
        if (copy == NULL) {
            Py_DECREF(y);
            return NULL;
        }
        if (copy != elem)
            all_identical = 0;
        PyTuple_SET_ITEM(y, i, copy);
    }

    /*
     * Tuples whose elements are all their own deepcopies don't
     * get memoized. Adding to the memo costs time and memory, and
     * we assume that pathological cases like [(1, 2, 3, 4)]*10000
     * are rare.
     */
    if (all_identical) {
        Py_INCREF(x);
        Py_SETREF(y, x);
    }
    /* Did we do a copy of the same tuple deeper down? */
    else if ((z = _PyDict_GetItem_KnownHash(memo, id_x, hash_id_x)) != NULL)
    {
        Py_INCREF(z);
        Py_SETREF(y, z);
    }
    /* Make sure any of our callers up the stack return this copy. */
    else if (_PyDict_SetItem_KnownHash(memo, id_x, y, hash_id_x) < 0) {
        Py_CLEAR(y);
    }
    return y;
}

#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

/*
 * Using the private _PyNone_Type and _PyNotImplemented_Type avoids
 * special-casing those in do_deepcopy.
 */
static PyTypeObject* const atomic_type[] = {
    &_PyNone_Type,    /* type(None) */
    &PyUnicode_Type,  /* str */
    &PyBytes_Type,    /* bytes */
    &PyLong_Type,     /* int */
    &PyBool_Type,     /* bool */
    &PyFloat_Type,    /* float */
    &PyComplex_Type,  /* complex */
    &PyType_Type,     /* type */
    &PyEllipsis_Type, /* type(Ellipsis) */
    &_PyNotImplemented_Type, /* type(NotImplemented) */
};
#define N_ATOMIC_TYPES ARRAY_SIZE(atomic_type)

struct deepcopy_dispatcher {
    PyTypeObject* type;
    PyObject* (*handler)(PyObject* x, PyObject* memo,
        PyObject* id_x, Py_hash_t hash_id_x);
};

static const struct deepcopy_dispatcher deepcopy_dispatch[] = {
    {&PyList_Type, deepcopy_list},
    {&PyDict_Type, deepcopy_dict},
    {&PyTuple_Type, deepcopy_tuple},
};
#define N_DISPATCHERS ARRAY_SIZE(deepcopy_dispatch)

static PyObject* do_deepcopy(PyObject* x, PyObject* memo)
{
    unsigned i;
    PyObject* y, * id_x;
    const struct deepcopy_dispatcher* dd;
    Py_hash_t hash_id_x;

    assert(PyDict_CheckExact(memo));

    /*
     * No need to have a separate dispatch function for this. Also, the
     * array would have to be quite a lot larger before a smarter data
     * structure is worthwhile.
     */
    for (i = 0; i < N_ATOMIC_TYPES; ++i) {
        if (Py_TYPE(x) == atomic_type[i]) {
            Py_INCREF(x);
            return x;
        }
    }

    /* Have we already done a deepcopy of x? */
    id_x = object_id(x);
    if (id_x == NULL)
        return NULL;
    hash_id_x = PyObject_Hash(id_x);

    y = _PyDict_GetItem_KnownHash(memo, id_x, hash_id_x);
    if (y != NULL) {
        Py_DECREF(id_x);
        Py_INCREF(y);
        return y;
    }
    /*
     * Hold on to id_x and its hash a little longer - the dispatch handlers
     * will all need it.
     */
    for (i = 0; i < N_DISPATCHERS; ++i) {
        dd = &deepcopy_dispatch[i];
        if (Py_TYPE(x) != dd->type)
            continue;

        y = dd->handler(x, memo, id_x, hash_id_x);
        Py_DECREF(id_x);
        if (y == NULL)
            return NULL;
        if (x != y && memo_keepalive(x, memo) < 0) {
            Py_DECREF(y);
            return NULL;
        }
        return y;
    }

    Py_DECREF(id_x);

    return do_deepcopy_fallback(x, memo);
}

/*
 * This is the Python entry point. Hopefully we can stay in the C code
 * most of the time, but we will occasionally call the Python code to
 * handle the stuff that's very inconvenient to write in C, and that
 * will then call back to us.
 */

 /*[clinic input]
 _copy.deepcopy

   x: object
   memo: object = None
   /

 Create a deep copy of x

 See the documentation for the copy module for details.

 [clinic start generated code]*/

static PyObject*
_copy_deepcopy_impl(PyObject* module, PyObject* x, PyObject* memo)
/*[clinic end generated code: output=825a9c8dd4bfc002 input=24ec531bf0923156]*/
{
    PyObject* result;

    if (memo == Py_None) {
        memo = PyDict_New();
        if (memo == NULL)
            return NULL;
    }
    else {
        if (!PyDict_CheckExact(memo)) {
            PyErr_SetString(PyExc_TypeError, "memo must be a dict");
            return NULL;
        }
        Py_INCREF(memo);
    }

    result = do_deepcopy(x, memo);

    Py_DECREF(memo);
    return result;
}

#include "clinic/_copy.c.h"

static PyMethodDef functions[] = {
    _COPY_DEEPCOPY_METHODDEF
    {NULL, NULL}
};

static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "_copy",
    "C implementation of deepcopy",
    -1,
    functions,
    NULL,
    NULL,
    NULL,
    NULL
};

PyMODINIT_FUNC
PyInit__copy(void)
{
    PyObject* module = PyModule_Create(&moduledef);
    if (module == NULL)
        return NULL;

    return module;
}
