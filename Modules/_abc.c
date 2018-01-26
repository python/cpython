/* ABCMeta implementation */

/* TODO: Get it compiled */
/* TODO: Get tests passing */
/* TODO: DECREF all WeakRefs! */
/* TODO: Fix refleaks! */
/* TODO: Think (ask) about thread-safety. */
/* TODO: Add checks only to those calls that can fail and use _GET_SIZE etc. */
/* TODO: Think about inlining some calls (like __subclasses__) and/or using macros */
/* TODO: Use separate branches with "fast paths" */

#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");

_Py_IDENTIFIER(stdout);
_Py_IDENTIFIER(__abstractmethods__);
_Py_IDENTIFIER(__class__);
_Py_IDENTIFIER(__dict__);

/* A global counter that is incremented each time a class is
   registered as a virtual subclass of anything.  It forces the
   negative cache to be cleared before its next use.
   Note: this counter is private. Use `abc.get_cache_token()` for
   external code. */
static PyObject *abc_invalidation_counter = PyLong_FromLong(0);

static PyObject *_the_registry = PyDict_New(NULL);
static PyObject *_the_cache = PyDict_New(NULL);
static PyObject *_the_negative_cache = PyDict_New(NULL);
static PyObject *_the_cache_version = PyDict_New(NULL);

typedef struct {
    PyObject_HEAD
    unsigned long iterating;
    PyObject *data;
    PyObject *pending;
    PyObject *in_weakreflist;
} _guarded_set;

static void
gset_dealloc(_guarded_set *self)
{
    if (self->data != NULL) {
        PySet_Clear(self->data);
    }
    if (self->pending != NULL) {
        PyList_Type->tp_dealloc(self->pending);
    }
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
gset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _guarded_set *self;

    self = (_guarded_set *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->iterating = 0;
        self->data = PySet_New(NULL);
        self->pending = PyList_New(NULL);
        self->in_weakreflist = NULL;
    }
    return (PyObject *) self;
}

static PyTypeObject _guarded_set_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_guarded_set",                     /*tp_name*/
    sizeof(_guarded_set),               /*tp_size*/
    0,                                  /*tp_itemsize*/
    (destructor)gset_dealloc,           /*tp_dealloc*/
    0,                                  /*tp_print*/
    0,                                  /*tp_getattr*/
    0,                                  /*tp_setattr*/
    0,                                  /*tp_reserved*/
    0,                                  /*tp_repr*/
    0,                                  /*tp_as_number*/
    0,                                  /*tp_as_sequence*/
    0,                                  /*tp_as_mapping*/
    0,                                  /*tp_hash*/
    0,                                  /*tp_call*/
    0,                                  /*tp_str*/
    0,                                  /*tp_getattro*/
    0,                                  /*tp_setattro*/
    0,                                  /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,                 /* tp_flags */
    0,                                  /*tp_doc*/
    0,                                  /*tp_traverse*/
    0,                                  /*tp_clear*/
    0,                                  /*tp_richcompare*/
    offsetof(_guarded_set, in_weakreflist), /*tp_weaklistoffset*/
    0,                                  /*tp_iter*/
    0,                                  /*tp_iternext*/
    0,                                  /*tp_methods*/
    0,                                  /* tp_members */
    0,                                  /* tp_getset */
    0,                                  /* tp_base */
    0,                                  /* tp_dict */
    0,                                  /* tp_descr_get */
    0,                                  /* tp_descr_set */
    0,                                  /* tp_dictoffset */
    0,                                  /* tp_init */
    PyType_GenericAlloc,                /* tp_alloc */
    gset_new                            /* tp_new */
    };

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
_in_cache(PyObject *self, PyObject *cls)
{
    PyObject *key, *cache;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    cache = PyObject_GetItem(_the_cache, key);
    if (!cache) {
        return 0;
    }
    return _in_weak_set(cache, cls);
}

int
_in_negative_cache(PyObject *self, PyObject *cls)
{
    PyObject *key, *cache;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    cache = PyObject_GetItem(_the_negative_cache, key);
    if (!cache) {
        return 0;
    }
    return _in_weak_set(cache, cls);
}

static PyObject *
_destroy(PyObject *setweakref, PyObject *objweakref)
{
    PyObject *set;
    set = PyWeakref_GET_OBJECT(setweakref);
    if (set == Py_None)
        Py_RETURN_NONE;
    Py_INCREF(set);
    if (PySet_Discard(set, objweakref) < 0) {
        return NULL;
    }
    Py_DECREF(set);
    Py_RETURN_NONE;
}

static PyMethodDef _destroy_def = {
    "_destroy", (PyCFunction) _destroy, METH_O
};

static PyObject *
_destroy_guarded(PyObject *setweakref, PyObject *objweakref)
{
    PyObject *set;
    set = PyWeakref_GET_OBJECT(setref);
    if (set == Py_None) {
        Py_RETURN_NONE;
    }
    Py_INCREF(set);
    if (set->iterating) {
        PyList_Append(set->pending, objweakref);
    } else {
        if (PySet_Discard(set, objweakref) < 0) {
            return NULL;
        }
    }
    Py_DECREF(set);
    Py_RETURN_NONE;
}

static PyMethodDef _destroy_guarded_def = {
    "_destroy_guarded", (PyCFunction) _destroy_guarded, METH_O
};

int
_add_to_weak_set(PyObject *set, PyObject *obj, int guarded)
{
    PyObject *ref, *wr;
    PyObject *destroy_cb;
    wr = PyWeakref_NewRef(set, NULL);
    if (!wr) {
        return 0;
    }
    if (guarded) {
        destroy_cb = PyCFunction_NewEx(&_destroy_guarded_def, wr, NULL);
    } else {
        destroy_cb = PyCFunction_NewEx(&_destroy_def, wr, NULL);
    }
    ref = PyWeakref_NewRef(obj, destroy_cb);
    if (!ref) {
        return 0;
    }
    if (PySet_Add(set, ref) < 0) {
        return 0;
    }
    return 1;
}

PyObject *
_get_registry(PyObject *self)
{
    PyObject *key;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    return PyObject_GetItem(_the_registry, key)->data;
}

int
_enter_iter(PyObject *self)
{
    PyObject *key, *registry;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    registry = PyObject_GetItem(_the_registry, key);
    registry->iterating++;
    return 1;
}

int
_exit_iter(PyObject *self)
{
    PyObject *key, *registry, *ref;
    int pos;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    registry->iterating--;
    if (registry->iterating) {
        return 1;
    }
    registry = PyObject_GetItem(_the_registry, key);
    for (pos = 0; pos < PyList_GET_SIZE(registry->pending); pos++) {
        ref = PyObject_CallMethod(registry->pending, "pop", NULL);
        PySet_Discard(registry->data, ref);
    }
    return 1;
}

int
_add_to_registry(PyObject *self, PyObject *cls)
{
    PyObject *registry = _get_registry(self);
    if (!registry) {
        return 0;
    }
    return _add_to_weak_set(registry, cls, 1);
}

int
_add_to_cache(PyObject *self, PyObject *cls)
{
    PyObject *key, *cache;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    cache = PyObject_GetItem(_the_cache, key);
    if (!cache) {
        return 0;
    }
    return _add_to_weak_set(cache, cls, 0);
}

int
_add_to_negative_cache(PyObject *self, PyObject *cls)
{
    PyObject *key, *cache;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    cache = PyObject_GetItem(_the_negative_cache, key);
    if (!cache) {
        return 0;
    }
    return _add_to_weak_set(cache, cls, 0);
}

PyDoc_STRVAR(_reset_caches_doc,
"Internal ABC helper to reset registry of a given class.\n\
\n\
Should be only used by refleak.py");

int
_reset_registry(PyObject *self)
{
    PyObject *registry = _get_registry(self);
    if (!registry) {
        return 0;
    }
    return PySet_Clear(registry->data);
}

PyDoc_STRVAR(_reset_caches_doc,
"Internal ABC helper to reset both caches of a given class.\n\
\n\
Should be only used by refleak.py");

int
_reset_caches(PyObject *self)
{
    PyObject *cache, *key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    cache = PyObject_GetItem(_the_cache, key);
    if (!cache) {
        return 0;
    }
    if (!PySet_Clear(cache)) {
        return 0;
    }
    /* also the second cache */
    cache = PyObject_GetItem(_the_negative_cache, key);
    if (!cache) {
        return 0;
    }
    return PySet_Clear(cache);
}

int
_get_negative_cache_version(PyObject *self)
{
    PyObject *key;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    return PyObject_GetItem(_the_cache_version, key);
}

int
_set_negative_cache_version(PyObject *self, PyObject *version)
{
    PyObject *key;
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return 0;
    }
    return PyObject_SetItem(_the_cache_version, key, version);
}

PyDoc_STRVAR(_get_dump_doc,
"Internal ABC helper for cache and registry debugging.\n\
\n\
Return shallow copies of registry, of both caches, and\n\
negative cache version.\n\ Don't call this function directly,\n\
instead use ABC._dump_registry() for a nice repr.");

PyObject *
_get_dump(PyObject *self)
{
    PyObject *key, *registry, *cache, *negative_cache, *cache_version, *res = PyTuple_New(4);
    registry = _get_registry(self);
    if (!registry) {
        return NULL;
    }
    registry = PyObject_CallMethod(registry->data, "copy");
    if (!registry) {
        return NULL;
    }
    key = PyWeakref_NewRef(self, NULL);
    if (!key) {
        return NULL;
    }
    cache = PyObject_GetItem(_the_cache, key);
    if (!cache) {
        return NULL;
    }
    cache = PyObject_CallMethod(cache, "copy");
    if (!cache) {
        return NULL;
    }
    negative_cache = PyObject_GetItem(_the_negative_cache, key);
    if (!negative_cache) {
        return NULL;
    }
    negative_cache = PyObject_CallMethod(negative_cache, "copy");
    if (!negative_cache) {
        return NULL;
    }
    cache_version = _get_negative_cache_version(self);
    if (!cache_version) {
        return NULL;
    }
    PyTuple_SetItem(res, 0, registry);
    PyTuple_SetItem(res, 1, cache);
    PyTuple_SetItem(res, 2, negative_cache);
    PyTuple_SetItem(res, 3, cache_version);
    return res;
}

PyDoc_STRVAR(_abc_init_doc,
"Internal ABC helper for class set-up. Should be never used outside abc module");

static PyObject *
_abc_init(PyObject *self)
{
    abc *result = NULL;
    PyObject *ns, *bases, *items, *abstracts, *base_abstracts;
    PyObject *key, *value, *item, *iter, *registry, *cache;
    Py_ssize_t pos = 0;
    int ret;
    /* Set up inheritance registry. */
    ref = PyWeakref_NewRef(self, NULL);
    if (!ref) {
        return NULL;
    }
    registry = gset_new(&_guarded_set_type, NULL, NULL);
    if (!registry) {
        return NULL;
    }
    if (PyDict_SetItem(_the_registry, ref, registry) < 0) {
        return NULL;
    }
    cache = PySet_New(NULL);
    if (!cache) {
        return NULL;
    }
    if (PyDict_SetItem(_the_cache, ref, cache) < 0) {
        return NULL;
    }
    cache = PySet_New(NULL);
    if (!cache) {
        return NULL;
    }
    if (PyDict_SetItem(_the_negative_cache, ref, cache) < 0) {
        return NULL;
    }
    if (PyDict_SetItem(_the_cache_version, ref, abc_invalidation_counter) < 0) {
        return NULL;
    }
    if (!(abstracts = PyFrozenSet_New(NULL))) {
        return NULL;
    }
    /* Compute set of abstract method names in two stages:
       Stage 1: direct abstract methods.
       (It is safe to assume everything is fine since type.__new__ succeeded.) */
    ns = _PyObject_GetAttrId(self, &PyId___dict__);
    keys = PyMapping_Keys(ns); /* TODO: Fast path for exact dicts with PyDict_Next */
    if (!keys) {
        Py_DECREF(result);
        return NULL;
    }
    for (pos = 0; pos < PySequence_Size(keys); pos++) {
        key = PySequence_GetItem(keys, pos);
        if () {
            Py_DECREF(keys);
            goto error;
        }
        value = PyObject_GetItem(ns, key);
        if (!value) {
            Py_DECREF(keys);
            Py_DECREF(key);
            goto error;
        }
        int is_abstract = _PyObject_IsAbstract(value);
        if (is_abstract < 0) {
            Py_DECREF(keys);
            Py_DECREF(key);
            Py_DECREF(value);
            goto error;
        }
        if (is_abstract && PySet_Add(abstracts, key) < 0) {
            Py_DECREF(keys);
            Py_DECREF(key);
            Py_DECREF(value);
            goto error;
        }
        Py_DECREF(key);
        Py_DECREF(value);
    }
    Py_DECREF(keys);

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
    Py_RETURN_NONE;
error:
    Py_DECREF(abstracts);
    return NULL;
}

PyDoc_STRVAR(_abc_subclasscheck_doc,
"Internal ABC helper for subclasss registration. Should be never used outside abc module");

static PyObject *
_abc_register(abc *self, PyObject *args)
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
    /* Invalidate negative cache */
    abc_invalidation_counter = PyNumber_Add(abc_invalidation_counter, PyLong_FromLong(1));
    return subclass;
}

PyDoc_STRVAR(_abc_instancecheck_doc,
"Internal ABC helper for instance checks. Should be never used outside abc module");

static PyObject *
_abc_instancecheck(PyObject *self, PyObject *args)
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
        if ((PyObject_RichCompareBool(_get_negative_cache_version(self),
                                     abc_invalidation_counter, Py_EQ) > 0) && incache) {
            Py_RETURN_FALSE;
        }
        /* Fall back to the subclass check. */
        return PyObject_CallMethod((self, "__subclasscheck__", "O", subclass);
    }
    result = PyObject_CallMethod(self, "__subclasscheck__", "O", subclass);
    if (!result) {
        return NULL;
    }
    if (result == Py_True) {
        return Py_True;
    }
    Py_DECREF(result);
    return PyObject_CallMethod(self, "__subclasscheck__", "O", subtype);
}


PyDoc_STRVAR(_abc_subclasscheck_doc,
"Internal ABC helper for subclasss checks. Should be never used outside abc module");

static PyObject *
_abc_subclasscheck(PyObject *self, PyObject *args)
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
    if (PyObject_RichCompareBool(_get_negative_cache_version(self),
                                 abc_invalidation_counter, Py_LT) > 0) {
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
    _enter_iter(self);
    while (_PySet_NextEntry(_get_registry(self), &pos, &key, &hash)) {
        rkey = PyWeakref_GetObject(key);
        if (rkey == Py_None) {
            continue;
        }
        result = PyObject_IsSubclass(subclass, rkey);
        if (result < 0) {
            _exit_iter(self);
            return NULL;
        }
        if (result > 0) {
            if (!_add_to_cache(self, subclass)) {
                _exit_iter(self);
                return NULL;
            }
            _exit_iter(self);
            Py_RETURN_TRUE;
        }
    }
    _exit_iter(self);

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod(self, "__subclasses__", NULL);
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
    Py_INCREF(abc_invalidation_counter);
    return abc_invalidation_counter;
}

static struct PyMethodDef module_functions[] = {
    {"get_cache_token", (PyCFunction)get_cache_token, METH_NOARGS,
        _cache_token_doc},
    {"_abc_init", (PyCFunction)_abc_init, METH_O,
        _abc_init_doc},
    {"_reset_registry", (PyCFunction)_reset_registry, METH_O,
        _abc_init_doc},
    {"_reset_caches", (PyCFunction)_reset_caches, METH_O,
        _reset_caches_doc},
    {"_get_dump", (PyCFunction)_get_dump, METH_O,
        _get_dump_doc},
    {"_abc_register", (PyCFunction)_abc_register, METH_VARARGS,
        _abc_resiter_doc},
    {"_abc_instancecheck", (PyCFunction)_abc_instancecheck, METH_VARARGS,
        _abc_instancecheck_doc},
    {"_abc_subclasscheck", (PyCFunction)_abc_subclasscheck, METH_VARARGS,
        _abc_subclasscheck_doc},
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
