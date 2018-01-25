/* ABCMeta implementation */

/* TODO:

   Use fast paths where possible.
   In particular use capitals like PyList_GET_SIZE.
   Think (ask) about inlining some calls, like __subclasses__.
   Fix refleaks. */

#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");
#define DEFERRED_ADDRESS(ADDR) 0

_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(__abstractmethods__);
_Py_IDENTIFIER(__class__);


/* A global counter that is incremented each time a class is
   registered as a virtual subclass of anything.  It forces the
   negative cache to be cleared before its next use.
   Note: this counter is private. Use `abc.get_cache_token()` for
   external code. */
static Py_ssize_t abc_invalidation_counter = 0;


static PyObject *
abcmeta_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    abc *result = NULL;
    PyObject *ns, *bases, *items, *abstracts, *base_abstracts;
    PyObject *key, *value, *item, *iter;
    Py_ssize_t pos = 0;
    int ret;
    result = (abc *)PyType_Type.tp_new(type, args, kwds);
    if (!result) {
        return NULL;
    }
    /* Set up inheritance registry. */
    result->abc_registry = PySet_New(NULL);
    result->abc_cache = PySet_New(NULL);
    result->abc_negative_cache = PySet_New(NULL);
    if (!result->abc_registry || !result->abc_cache || !result->abc_negative_cache) {
        Py_DECREF(result);
        return NULL;
    }
    result->abc_negative_cache_version = abc_invalidation_counter;
    if (!(abstracts = PyFrozenSet_New(NULL))) {
        Py_DECREF(result);
        return NULL;
    }

    /* Compute set of abstract method names in two stages:
       Stage 1: direct abstract methods.
       (It is safe to assume everything is fine since type.__new__ succeeded.) */
    ns = PyTuple_GET_ITEM(args, 2);
    items = PyMapping_Items(ns); /* TODO: Fast path for exact dicts with PyDict_Next */
    if (!items) {
        Py_DECREF(result);
        return NULL;
    }
    for (pos = 0; pos < PySequence_Size(items); pos++) {
        item = PySequence_GetItem(items, pos);
        if (!PyTuple_Check(item) || (PyTuple_GET_SIZE(item) != 2)) {
            PyErr_SetString(PyExc_TypeError, "Iteration over class namespace must"
                                             " yield length-two tuples");
        }
        key = PyTuple_GetItem(item, 0);
        value = PyTuple_GetItem(item, 1);
        int is_abstract = _PyObject_IsAbstract(value);
        if (is_abstract < 0) {
            Py_DECREF(item);
            Py_DECREF(items);
            goto error;
        }
        if (is_abstract && PySet_Add(abstracts, key) < 0) {
            Py_DECREF(item);
            Py_DECREF(items);
            goto error;
        }
        Py_DECREF(item);
    }
    Py_DECREF(items);

    /* Stage 2: inherited abstract methods. */
    bases = PyTuple_GET_ITEM(args, 1);
    for (pos = 0; pos < PyTuple_Size(bases); pos++) {
        item = PyTuple_GET_ITEM(bases, pos);
        ret = _PyObject_LookupAttrId(item, &PyId___abstractmethods__, &base_abstracts);
        if (ret < 0) {
            goto error;
        } else if (!ret) {
            continue;
        }
        if (!(iter = PyObject_GetIter(base_abstracts))) {
            goto error;
        }
        while ((key = PyIter_Next(iter))) {
            ret = _PyObject_LookupAttr((PyObject *)result, key, &value);
            if (ret < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            } else if (!ret) {
                Py_DECREF(key);
                continue;
            }
            int is_abstract = _PyObject_IsAbstract(value);
            Py_DECREF(value);
            if (is_abstract < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            if (is_abstract && PySet_Add(abstracts, key) < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            Py_DECREF(key);
        }
        Py_DECREF(iter);
        if (PyErr_Occurred()) {
            goto error;
        }
    }
    if (_PyObject_SetAttrId((PyObject *)result, &PyId___abstractmethods__, abstracts) < 0) {
        goto error;
    }
    return (PyObject *)result;
error:
    Py_DECREF(result);
    Py_DECREF(abstracts);
    return NULL;
}

int
_in_weak_set(PyObject *set, PyObject *obj)
{
    PyObject *ref;
    int res;
    ref = PyWeakref_NewRef(obj, NULL);
    if (!ref) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            return 0;
        }
        return -1;
    }
    res = PySet_Contains(set, ref);
    Py_DECREF(ref);
    return res;
}

int
_add_weak_set(PyObject *set, PyObject *obj)
{
    PyObject *ref;
    ref = PyWeakref_NewRef(obj, NULL);
    if (!ref) {
        return 0;
    }
    if (PySet_Add(set, ref) < 0) {
        return 0;
    }
    return 1;
}

static PyObject *
abcmeta_register(abc *self, PyObject *args)
{
    PyObject *subclass = NULL;
    int result;
    if (!PyArg_UnpackTuple(args, "register", 1, 1, &subclass)) {
        return NULL;
    }
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return NULL;
    }
    result = PyObject_IsSubclass(subclass, (PyObject *)self);
    if (result > 0) {
        Py_INCREF(subclass);
        return subclass;  /* Already a subclass. */
    }
    if (result < 0) {
        return NULL;
    }
    /* Subtle: test for cycles *after* testing for "already a subclass";
       this means we allow X.register(X) and interpret it as a no-op. */
    result = PyObject_IsSubclass((PyObject *)self, subclass);
    if (result > 0) {
        /* This would create a cycle, which is bad for the algorithm below. */
        PyErr_SetString(PyExc_RuntimeError, "Refusing to create an inheritance cycle");
        return NULL;
    }
    if (result < 0) {
        return NULL;
    }
    if (!_add_to_registry(self, subclass)) {
        return NULL;
    }
    Py_INCREF(subclass);
    abc_invalidation_counter++;  /* Invalidate negative cache */
    return subclass;
}

static PyObject *
abcmeta_subclasscheck(abc *self, PyObject *args); /* Forward */

static PyObject *
abcmeta_instancecheck(abc *self, PyObject *args)
{
    PyObject *result, *subclass, *subtype, *instance = NULL;
    int incache;
    if (!PyArg_UnpackTuple(args, "__instancecheck__", 1, 1, &instance)) {
        return NULL;
    }
    subclass = _PyObject_GetAttrId(instance, &PyId___class__);
    /* Inline the cache checking. */
    incache = _in_cache(self, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (incache > 0) {
        Py_RETURN_TRUE;
    }
    subtype = (PyObject *)Py_TYPE(instance);
    if (subtype == subclass) {
        incache = _in_negative_cache(self, subclass);
        if (incache < 0) {
            return NULL;
        }
        if (_get_negative_cache_version(self) == abc_invalidation_counter && incache) {
            Py_RETURN_FALSE;
        }
        /* Fall back to the subclass check. */
        return PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subclass);
    }
    result = PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subclass);
    if (!result) {
        return NULL;
    }
    if (result == Py_True) {
        return Py_True;
    }
    Py_DECREF(result);
    return PyObject_CallMethod((PyObject *)self, "__subclasscheck__", "O", subtype);
}

int
_add_to_cache(PyObject *self, PyObject *cls)
{
    return 0;
}

int
_add_to_negative_cache(PyObject *self, PyObject *cls)
{
    return 0;
}

int
_in_cache(PyObject *self, PyObject *cls)
{
    return 0;
}

int
_in_negative_cache(PyObject *self, PyObject *cls)
{
    return 0;
}

int
_get_registry(PyObject *self)
{
    return 0;
}

int
_add_to_registry(PyObject *self, PyObject *cls)
{
    return 0;
}

int
_reset_registry(PyObject *self)
{
    return 0;
}

int
_reset_cache(PyObject *self)
{
    return 0;
}

int
_reset_negative_cache(PyObject *self)
{
    return 0;
}

int
_get_negative_cache_version(PyObject *self)
{
    return 0;
}

int
_set_negative_cache_version(PyObject *self, Py_ssize_t version)
{
    return 0;
}

PyObject *
_get_dump()
{
    return NULL;
}

static PyObject *
abcmeta_subclasscheck(PyObject *self, PyObject *args)
{
    PyObject *subclasses, *subclass = NULL;
    PyObject *ok, *mro, *key, *rkey;
    Py_ssize_t pos;
    int incache, result;
    if (!PyArg_UnpackTuple(args, "__subclasscheck__", 1, 1, &subclass)) {
        return NULL;
    }
    /* TODO: clear the registry from dead refs from time to time
       on iteration here (have a counter for this) or maybe during a .register() call */
    /* TODO: Reset caches every n-th succes/failure correspondingly
       so that they don't grow too large */

    /* 1. Check cache. */
    incache = _in_cache(self, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (incache > 0) {
        Py_RETURN_TRUE;
    }
    /* 2. Check negative cache; may have to invalidate. */
    incache = _in_negative_cache(self, subclass);
    if (incache < 0) {
        return NULL;
    }
    if (_get_negative_cache_version(self) < abc_invalidation_counter) {
        /* Invalidate the negative cache. */
        if (!_reset_negative_cache(self)) {
            return NULL;
        }
        _set_negative_cache_version(self, abc_invalidation_counter);
    } else if (incache) {
        Py_RETURN_FALSE;
    }
    /* 3. Check the subclass hook. */
    ok = PyObject_CallMethod((PyObject *)self, "__subclasshook__", "O", subclass);
    if (!ok) {
        return NULL;
    }
    if (ok == Py_True) {
        if (!_add_to_cache(self, subclass)) {
            Py_DECREF(ok);
            return NULL;
        }
        return Py_True;
    }
    if (ok == Py_False) {
        if (!_add_to_negative_cache(self, subclass)) {
            Py_DECREF(ok);
            return NULL;
        }
        return Py_False;
    }
    Py_DECREF(ok);
    /* 4. Check if it's a direct subclass. */
    mro = ((PyTypeObject *)subclass)->tp_mro;
    for (pos = 0; pos < PyTuple_Size(mro); pos++) {
        if ((PyObject *)self == PyTuple_GetItem(mro, pos)) {
            if (!_add_to_cache(self, subclass)) {
                return NULL;
            }
            Py_RETURN_TRUE;
        }
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    pos = 0;
    Py_hash_t hash;
    while (_PySet_NextEntry(_get_registry(self), &pos, &key, &hash)) {
        rkey = PyWeakref_GetObject(key);
        if (rkey == Py_None) {
            continue;
        }
        result = PyObject_IsSubclass(subclass, rkey);
        if (result < 0) {
            return NULL;
        }
        if (result > 0) {
            if (!_add_to_cache(self, subclass)) {
                return NULL;
            }
            Py_RETURN_TRUE;
        }
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod((PyObject *)self, "__subclasses__", NULL);
    for (pos = 0; pos < PyList_GET_SIZE(subclasses); pos++) {
        result = PyObject_IsSubclass(subclass, PyList_GET_ITEM(subclasses, pos));
        if (result > 0) {
            if (!_add_to_cache(self, subclass)) {
                return NULL;
            }
            Py_DECREF(subclasses);
            Py_RETURN_TRUE;
        }
        if (result < 0) {
            Py_DECREF(subclasses);
            return NULL;
        }
    }
    Py_DECREF(subclasses);
    /* No dice; update negative cache. */
    if (!_add_to_negative_cache(self, subclass)) {
        return NULL;
    }
    Py_RETURN_FALSE;
}


PyDoc_STRVAR(_cache_token_doc,
"Returns the current ABC cache token.\n\
\n\
The token is an opaque object (supporting equality testing) identifying the\n\
current version of the ABC cache for virtual subclasses. The token changes\n\
with every call to ``register()`` on any ABC.");

static PyObject *
get_cache_token(void)
{
    return PyLong_FromSsize_t(abc_invalidation_counter);
}

static struct PyMethodDef module_functions[] = {
    {"get_cache_token", (PyCFunction)get_cache_token, METH_NOARGS,
        _cache_token_doc},
    {NULL,       NULL}          /* sentinel */
};

static struct PyModuleDef _abcmodule = {
    PyModuleDef_HEAD_INIT,
    "_abc",
    _abc__doc__,
    -1,
    module_functions,
    NULL,
    NULL,
    NULL,
    NULL
};


PyMODINIT_FUNC
PyInit__abc(void)
{
    return PyModule_Create(&_abcmodule);
}
