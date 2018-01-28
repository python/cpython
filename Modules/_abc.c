/* ABCMeta implementation */

/* TODO: Think (ask) about thread-safety. */
/* TODO: Add checks only to those calls that can fail and use _GET_SIZE etc. */
/* TODO: Think about inlining some calls (like __subclasses__) and/or using macros */
/* TODO: Use separate branches with "fast paths" */

#include "Python.h"
#include "structmember.h"

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");

_Py_IDENTIFIER(__abstractmethods__);
_Py_IDENTIFIER(__class__);
_Py_IDENTIFIER(__dict__);
_Py_IDENTIFIER(__bases__);
_Py_IDENTIFIER(_abc_impl);

/* A global counter that is incremented each time a class is
   registered as a virtual subclass of anything.  It forces the
   negative cache to be cleared before its next use.
   Note: this counter is private. Use `abc.get_cache_token()` for
   external code. */
static PyObject *abc_invalidation_counter;

typedef struct {
    PyObject_HEAD
    Py_ssize_t iterating;
    PyObject *data; /* The actual set of weak references. */
    PyObject *pending; /* Pending removals collected during iteration. */
    PyObject *in_weakreflist;
} _guarded_set;

static void
gset_dealloc(_guarded_set *self)
{
    Py_DECREF(self->data);
    Py_DECREF(self->pending);
    if (self->in_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *) self);
    }
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
gset_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *data = PySet_New(NULL);
    if (data == NULL) {
        return NULL;
    }
    PyObject *pending = PyList_New(0);
    if (pending == NULL) {
        Py_DECREF(data);
        return NULL;
    }
    _guarded_set *self = (_guarded_set *) type->tp_alloc(type, 0);
    if (self != NULL) {
        self->iterating = 0;
        self->data = data;
        self->pending = pending;
        self->in_weakreflist = NULL;
    }
    return (PyObject *) self;
}

PyDoc_STRVAR(guarded_set_doc,
"Internal weak set guarded against deletion during iteration.\n\
Used by ABC machinery.");

static PyTypeObject _guarded_set_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_guarded_set",                     /*tp_name*/
    sizeof(_guarded_set),               /*tp_size*/
    .tp_dealloc = (destructor)gset_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_weaklistoffset = offsetof(_guarded_set, in_weakreflist),
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = gset_new,
};

/* This object stores internal state for ABCs.
   Note that we can use normal sets for caches,
   since they are never iterated over. */
typedef struct {
    PyObject_HEAD
    _guarded_set *_abc_registry;
    PyObject *_abc_cache; /* Normal set of weak references. */
    PyObject *_abc_negative_cache; /* Normal set of weak references. */
    PyObject *_abc_negative_cache_version;
} _abc_data;

static void
abc_data_dealloc(_abc_data *self)
{
    Py_DECREF(self->_abc_registry);
    Py_DECREF(self->_abc_cache);
    Py_DECREF(self->_abc_negative_cache);
    Py_DECREF(self->_abc_negative_cache_version);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
abc_data_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *registry = NULL, *cache = NULL, *negative_cache = NULL;
    registry = gset_new(&_guarded_set_type, NULL, NULL);
    cache = PySet_New(NULL);
    negative_cache = PySet_New(NULL);
    if (!registry || !cache || !negative_cache) {
        goto error;
    }

    _abc_data *self = (_abc_data *) type->tp_alloc(type, 0);
    if (self == NULL) {
        goto error;
    }

    self->_abc_registry = (_guarded_set *)registry;
    self->_abc_cache = cache;
    self->_abc_negative_cache = negative_cache;
    self->_abc_negative_cache_version = abc_invalidation_counter;
    Py_INCREF(abc_invalidation_counter);
    return (PyObject *) self;

error:
    Py_XDECREF(registry);
    Py_XDECREF(cache);
    Py_XDECREF(negative_cache);
    return NULL;
}

PyDoc_STRVAR(abc_data_doc,
"Internal state held by ABC machinery.");

static PyTypeObject _abc_data_type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "_abc_data",                        /*tp_name*/
    sizeof(_abc_data),                  /*tp_size*/
    .tp_dealloc = (destructor)abc_data_dealloc,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_alloc = PyType_GenericAlloc,
    .tp_new = abc_data_new,
};

static int
_in_weak_set(PyObject *set, PyObject *obj)
{
    PyObject *ref = PyWeakref_NewRef(obj, NULL);
    if (!ref) {
        if (PyErr_ExceptionMatches(PyExc_TypeError)) {
            PyErr_Clear();
            return 0;
        }
        return -1;
    }
    int res = PySet_Contains(set, ref);
    Py_DECREF(ref);
    return res;
}

static _abc_data *
_get_impl(PyObject *self)
{
    PyObject *impl = _PyObject_GetAttrId(self, &PyId__abc_impl);
    if (!impl) {
        return NULL;
    }
    if (Py_TYPE(impl) != &_abc_data_type) {
        PyErr_SetString(PyExc_TypeError, "_abc_impl is set to a wrong type");
        Py_DECREF(impl);
        return NULL;
    }
    return (_abc_data *)impl;
}

static int
_in_cache(_abc_data *impl, PyObject *cls)
{
    return _in_weak_set(impl->_abc_cache, cls);
}

static int
_in_negative_cache(_abc_data *impl, PyObject *cls)
{
    return _in_weak_set(impl->_abc_negative_cache, cls);
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
        Py_DECREF(set);
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
    _guarded_set *gset;
    int res;
    set = PyWeakref_GET_OBJECT(setweakref);
    if (set == Py_None) {
        Py_RETURN_NONE;
    }
    Py_INCREF(set);
    gset = (_guarded_set *)set;
    if (gset->iterating) {
        res = PyList_Append(gset->pending, objweakref);
        if (!res) {
            Py_DECREF(set);
            return NULL;
        }
    } else {
        if (PySet_Discard(gset->data, objweakref) < 0) {
            Py_DECREF(set);
            return NULL;
        }
    }
    Py_DECREF(set);
    Py_RETURN_NONE;
}

static PyMethodDef _destroy_guarded_def = {
    "_destroy_guarded", (PyCFunction) _destroy_guarded, METH_O
};

static int
_add_to_weak_set(PyObject *set, PyObject *obj, int guarded)
{
    PyObject *ref, *wr;
    PyObject *destroy_cb;
    wr = PyWeakref_NewRef(set, NULL);
    if (!wr) {
        return -1;
    }
    if (guarded) {
        destroy_cb = PyCFunction_NewEx(&_destroy_guarded_def, wr, NULL);
    } else {
        destroy_cb = PyCFunction_NewEx(&_destroy_def, wr, NULL);
    }
    ref = PyWeakref_NewRef(obj, destroy_cb);
    Py_DECREF(destroy_cb);
    if (!ref) {
        Py_DECREF(wr);
        return -1;
    }
    if (guarded) {
        set = ((_guarded_set *) set)->data;
    }
    if (PySet_Add(set, ref) < 0) {
        Py_DECREF(wr);
        Py_DECREF(ref);
        return -1;
    }
    Py_DECREF(wr);
    Py_DECREF(ref);
    return 0;
}

static PyObject *
_get_registry(PyObject *self)
{
    _abc_data *impl;
    PyObject *res;
    impl = _get_impl(self);
    if (!impl) {
        return NULL;
    }
    res = impl->_abc_registry->data;
    Py_DECREF(impl);
    return res;
}

static void
_enter_iter(_guarded_set *gs)
{
    gs->iterating++;
}

static int
_exit_iter(_guarded_set *gs)
{
    gs->iterating--;
    if (gs->iterating) {
        return 0;
    }

    assert(PyList_CheckExact(gs->pending));
    Py_ssize_t pos = PyList_GET_SIZE(gs->pending);
    while (pos > 0) {
        pos--;
        PyObject *ref = PyList_GET_ITEM(gs->pending, pos);
        Py_INCREF(Py_None);
        PyList_SET_ITEM(gs->pending, pos, Py_None);

        if (ref == Py_None) {
            Py_DECREF(ref);
            continue;
        }
        if (PySet_Discard(gs->data, ref) < 0) {
            Py_DECREF(ref);
            return -1;
        }
    }
    return PyList_SetSlice(gs->pending,
        0, PyList_GET_SIZE(gs->pending), NULL);
}

static int
_add_to_registry(PyObject *self, PyObject *cls)
{
    _abc_data *impl;
    int res;
    impl = _get_impl(self);
    if (!impl) {
        return -1;
    }
    res = _add_to_weak_set((PyObject *)(impl->_abc_registry), cls, 1);
    Py_DECREF(impl);
    return res;
}

static int
_add_to_cache(_abc_data *impl, PyObject *cls)
{
    return _add_to_weak_set(impl->_abc_cache, cls, 0);
}

static int
_add_to_negative_cache(PyObject *self, PyObject *cls)
{
    _abc_data *impl;
    int res;
    impl = _get_impl(self);
    if (!impl) {
        return -1;
    }
    res = _add_to_weak_set(impl->_abc_negative_cache, cls, 0);
    Py_DECREF(impl);
    return res;
}

PyDoc_STRVAR(_reset_registry_doc,
"Internal ABC helper to reset registry of a given class.\n\
\n\
Should be only used by refleak.py");

static PyObject *
_reset_registry(PyObject *m, PyObject *args)
{
    PyObject *self, *registry;
    if (!PyArg_UnpackTuple(args, "_reset_registry", 1, 1, &self)) {
        return NULL;
    }
    registry = _get_registry(self);
    if (!registry) {
        return NULL;
    }
    if (PySet_Clear(registry) < 0) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static int
_reset_negative_cache(_abc_data *impl)
{
    return PySet_Clear(impl->_abc_negative_cache);
}

PyDoc_STRVAR(_reset_caches_doc,
"Internal ABC helper to reset both caches of a given class.\n\
\n\
Should be only used by refleak.py");

static PyObject *
_reset_caches(PyObject *m, PyObject *args)
{
    PyObject *self;
    _abc_data *impl;
    if (!PyArg_UnpackTuple(args, "_reset_caches", 1, 1, &self)) {
        return NULL;
    }
    impl = _get_impl(self);
    if (!impl) {
        return NULL;
    }
    if (PySet_Clear(impl->_abc_cache) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    /* also the second cache */
    if (_reset_negative_cache(impl) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    Py_DECREF(impl);
    Py_RETURN_NONE;
}

static PyObject *
_get_negative_cache_version(_abc_data *impl)
{
    return impl->_abc_negative_cache_version;
}

static void
_set_negative_cache_version(_abc_data *impl)
{
    Py_INCREF(abc_invalidation_counter);
    Py_SETREF(impl->_abc_negative_cache_version, abc_invalidation_counter);
}

PyDoc_STRVAR(_get_dump_doc,
"Internal ABC helper for cache and registry debugging.\n\
\n\
Return shallow copies of registry, of both caches, and\n\
negative cache version. Don't call this function directly,\n\
instead use ABC._dump_registry() for a nice repr.");

static PyObject *
_get_dump(PyObject *m, PyObject *args)
{
    PyObject *self, *registry, *cache, *negative_cache, *cache_version;
    _abc_data *impl;
    if (!PyArg_UnpackTuple(args, "_get_dump", 1, 1, &self)) {
        return NULL;
    }
    registry = _get_registry(self);
    if (!registry) {
        return NULL;
    }
    registry = PyObject_CallMethod(registry, "copy", NULL);
    if (!registry) {
        return NULL;
    }
    impl = _get_impl(self);
    if (!impl) {
        return NULL;
    }
    cache = PyObject_CallMethod(impl->_abc_cache, "copy", NULL);
    if (!cache) {
        Py_DECREF(impl);
        return NULL;
    }
    negative_cache = PyObject_CallMethod(impl->_abc_negative_cache, "copy", NULL);
    if (!negative_cache) {
        Py_DECREF(impl);
        return NULL;
    }
    cache_version = _get_negative_cache_version(impl);
    if (!cache_version) {
        Py_DECREF(impl);
        return NULL;
    }
    PyObject *res = PyTuple_Pack(4,
            registry, cache, negative_cache, cache_version);
    Py_DECREF(impl);
    return res;
}

// Compute set of abstract method names.
static int
compute_abstract_methods(PyObject *self)
{
    int ret = -1;
    PyObject *abstracts = PyFrozenSet_New(NULL);
    if (!abstracts) {
        return -1;
    }

    PyObject *ns=NULL, *items=NULL, *bases=NULL;  // Py_CLEAR()ed on error.

    /* Stage 1: direct abstract methods. */
    ns = _PyObject_GetAttrId(self, &PyId___dict__);
    if (!ns) {
        goto error;
    }

    /* TODO: Fast path for exact dicts with PyDict_Next */
    items = PyMapping_Items(ns);
    if (!items) {
        goto error;
    }

    for (Py_ssize_t pos = 0; pos < PyList_GET_SIZE(items); pos++) {
        PyObject *it = PySequence_Fast(
                PyList_GET_ITEM(items, pos),
                "items() returned non-sequence item");
        if (!it) {
            goto error;
        }
        if (PySequence_Fast_GET_SIZE(it) != 2) {
            PyErr_SetString(PyExc_TypeError, "items() returned not 2-tuple");
            Py_DECREF(it);
            goto error;
        }

        // borrowed
        PyObject *key = PySequence_Fast_GET_ITEM(it, 0);
        PyObject *value = PySequence_Fast_GET_ITEM(it, 1);
        // items or it may be cleared while accessing __abstractmethod__
        // So we need to keep strong reference for key
        Py_INCREF(key);
        int is_abstract = _PyObject_IsAbstract(value);
        if (is_abstract < 0 ||
                (is_abstract && PySet_Add(abstracts, key) < 0)) {
            Py_DECREF(it);
            Py_DECREF(key);
            goto error;
        }
        Py_DECREF(key);
        Py_DECREF(it);
    }

    /* Stage 2: inherited abstract methods. */
    bases = _PyObject_GetAttrId(self, &PyId___bases__);
    if (!bases) {
        goto error;
    }
    if (!PyTuple_Check(bases)) {
        PyErr_SetString(PyExc_TypeError, "__bases__ is not tuple");
        goto error;
    }

    for (Py_ssize_t pos = 0; pos < PyTuple_GET_SIZE(bases); pos++) {
        PyObject *item = PyTuple_GET_ITEM(bases, pos);  // borrowed
        PyObject *base_abstracts, *iter;

        if (_PyObject_LookupAttrId(item, &PyId___abstractmethods__,
                                   &base_abstracts) < 0) {
            goto error;
        }
        if (base_abstracts == NULL) {
            continue;
        }
        if (!(iter = PyObject_GetIter(base_abstracts))) {
            Py_DECREF(base_abstracts);
            goto error;
        }
        Py_DECREF(base_abstracts);
        PyObject *key, *value;
        while ((key = PyIter_Next(iter))) {
            if (_PyObject_LookupAttr(self, key, &value) < 0) {
                Py_DECREF(key);
                Py_DECREF(iter);
                goto error;
            }
            if (value == NULL) {
                Py_DECREF(key);
                continue;
            }

            int is_abstract = _PyObject_IsAbstract(value);
            Py_DECREF(value);
            if (is_abstract < 0 ||
                    (is_abstract && PySet_Add(abstracts, key) < 0)) {
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

    if (_PyObject_SetAttrId(self, &PyId___abstractmethods__, abstracts) < 0) {
        goto error;
    }

    ret = 0;
error:
    Py_DECREF(abstracts);
    Py_XDECREF(ns);
    Py_XDECREF(items);
    Py_XDECREF(bases);
    return ret;
}

PyDoc_STRVAR(_abc_init_doc,
"Internal ABC helper for class set-up. Should be never used outside abc module");

static PyObject *
_abc_init(PyObject *m, PyObject *args)
{
    PyObject *self, *data;
    if (!PyArg_UnpackTuple(args, "_abc_init", 1, 1, &self)) {
        return NULL;
    }

    if (compute_abstract_methods(self) < 0) {
        return NULL;
    }

    /* Set up inheritance registry. */
    data = abc_data_new(&_abc_data_type, NULL, NULL);
    if (!data) {
        return NULL;
    }
    if (_PyObject_SetAttrId(self, &PyId__abc_impl, data) < 0) {
        Py_DECREF(data);
        return NULL;
    }
    Py_DECREF(data);
    Py_RETURN_NONE;
}

PyDoc_STRVAR(_abc_register_doc,
"Internal ABC helper for subclasss registration. Should be never used outside abc module");

static PyObject *
_abc_register(PyObject *m, PyObject *args)
{
    PyObject *self, *subclass = NULL;
    int result;
    if (!PyArg_UnpackTuple(args, "_abc_register", 2, 2, &self, &subclass)) {
        return NULL;
    }
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return NULL;
    }
    result = PyObject_IsSubclass(subclass, self);
    if (result > 0) {
        Py_INCREF(subclass);
        return subclass;  /* Already a subclass. */
    }
    if (result < 0) {
        return NULL;
    }
    /* Subtle: test for cycles *after* testing for "already a subclass";
       this means we allow X.register(X) and interpret it as a no-op. */
    result = PyObject_IsSubclass(self, subclass);
    if (result > 0) {
        /* This would create a cycle, which is bad for the algorithm below. */
        PyErr_SetString(PyExc_RuntimeError, "Refusing to create an inheritance cycle");
        return NULL;
    }
    if (result < 0) {
        return NULL;
    }
    if (_add_to_registry(self, subclass) < 0) {
        return NULL;
    }

    /* Invalidate negative cache */
    PyObject *one = PyLong_FromLong(1);
    if (one == NULL) {
        return NULL;
    }
    PyObject *next_version = PyNumber_Add(abc_invalidation_counter, one);
    Py_DECREF(one);
    if (next_version == NULL) {
        return NULL;
    }
    Py_SETREF(abc_invalidation_counter, next_version);

    Py_INCREF(subclass);
    return subclass;
}

PyDoc_STRVAR(_abc_instancecheck_doc,
"Internal ABC helper for instance checks. Should be never used outside abc module");

static PyObject *
_abc_instancecheck(PyObject *m, PyObject *args)
{
    PyObject *self, *result = NULL, *subclass = NULL,
             *subtype, *instance = NULL;
    if (!PyArg_UnpackTuple(args, "_abc_instancecheck", 2, 2, &self, &instance)) {
        return NULL;
    }

    _abc_data *impl = _get_impl(self);
    if (!impl) {
        return NULL;
    }

    subclass = _PyObject_GetAttrId(instance, &PyId___class__);
    /* Inline the cache checking. */
    int incache = _in_cache(impl, subclass);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_True;
        Py_INCREF(result);
        goto end;
    }
    subtype = (PyObject *)Py_TYPE(instance);
    if (subtype == subclass) {
        int r = PyObject_RichCompareBool(
            _get_negative_cache_version(impl),
            abc_invalidation_counter, Py_EQ);
        assert(r >= 0);  // Both should be PyLong
        if (r > 0) {
            incache = _in_negative_cache(impl, subclass);
            if (incache < 0) {
                goto end;
            }
            if (incache) {
                result = Py_False;
                Py_INCREF(result);
                goto end;
            }
        }
        /* Fall back to the subclass check. */
        result = PyObject_CallMethod(self, "__subclasscheck__", "O", subclass);
        goto end;
    }
    result = PyObject_CallMethod(self, "__subclasscheck__", "O", subclass);
    if (!result || result == Py_True) {
        goto end;
    }
    Py_DECREF(result);
    result = PyObject_CallMethod(self, "__subclasscheck__", "O", subtype);

end:
    Py_CLEAR(impl);
    Py_CLEAR(subclass);
    return result;
}


PyDoc_STRVAR(_abc_subclasscheck_doc,
"Internal ABC helper for subclasss checks. Should be never used outside abc module");

static PyObject *
_abc_subclasscheck(PyObject *m, PyObject *args)
{
    PyObject *self, *subclasses = NULL, *subclass = NULL, *result = NULL;
    PyObject *ok, *mro, *key, *rkey;
    Py_ssize_t pos;
    int incache;
    if (!PyArg_UnpackTuple(args, "_abc_subclasscheck", 2, 2, &self, &subclass)) {
        return NULL;
    }

    _abc_data *impl = _get_impl(self);
    if (!impl) {
        return NULL;
    }

    /* 1. Check cache. */
    incache = _in_cache(impl, subclass);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_True;
        goto end;
    }

    /* 2. Check negative cache; may have to invalidate. */
    incache = _in_negative_cache(impl, subclass);
    if (incache < 0) {
        goto end;
    }
    if (PyObject_RichCompareBool(_get_negative_cache_version(impl),
                                 abc_invalidation_counter, Py_LT) > 0) {
        /* Invalidate the negative cache. */
        if (_reset_negative_cache(impl) < 0) {
            goto end;
        }
        _set_negative_cache_version(impl);
    } else if (incache) {
        result = Py_False;
        goto end;
    }

    /* 3. Check the subclass hook. */
    ok = PyObject_CallMethod((PyObject *)self, "__subclasshook__", "O", subclass);
    if (!ok) {
        goto end;
    }
    if (ok == Py_True) {
        Py_DECREF(ok);
        if (_add_to_cache(impl, subclass) < 0) {
            goto end;
        }
        result = Py_True;
        goto end;
    }
    if (ok == Py_False) {
        Py_DECREF(ok);
        if (_add_to_negative_cache(self, subclass) < 0) {
            goto end;
        }
        result = Py_False;
        goto end;
    }
    if (ok != Py_NotImplemented) {
        Py_DECREF(ok);
        PyErr_SetString(PyExc_AssertionError, "__subclasshook__ must return either"
                                              " False, True, or NotImplemented");
        goto end;
    }
    Py_DECREF(ok);

    /* 4. Check if it's a direct subclass. */
    mro = ((PyTypeObject *)subclass)->tp_mro;
    assert(PyTuple_Check(mro));
    for (pos = 0; pos < PyTuple_Size(mro); pos++) {
        if ((PyObject *)self == PyTuple_GetItem(mro, pos)) {
            if (_add_to_cache(impl, subclass) < 0) {
                goto end;
            }
            result = Py_True;
            goto end;
        }
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    pos = 0;
    Py_hash_t hash;
    _enter_iter(impl->_abc_registry);

    while (_PySet_NextEntry(impl->_abc_registry->data, &pos, &key, &hash)) {
        rkey = PyWeakref_GetObject(key);
        if (rkey == Py_None) {
            continue;
        }
        int r = PyObject_IsSubclass(subclass, rkey);
        if (r < 0) {
            _exit_iter(impl->_abc_registry);
            goto end;
        }
        if (r > 0) {
            if (_add_to_cache(impl, subclass) < 0) {
                _exit_iter(impl->_abc_registry);
                goto end;
            }
            if (_exit_iter(impl->_abc_registry) < 0) {
                goto end;
            }
            result = Py_True;
            goto end;
        }
    }
    if (_exit_iter(impl->_abc_registry) < 0) {
        goto end;
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod(self, "__subclasses__", NULL);
    if(!PyList_Check(subclasses)) {
        PyErr_SetString(PyExc_TypeError, "__subclasses__() must return a list");
        goto end;
    }
    for (pos = 0; pos < PyList_GET_SIZE(subclasses); pos++) {
        int r = PyObject_IsSubclass(subclass, PyList_GET_ITEM(subclasses, pos));
        if (r > 0) {
            if (_add_to_cache(impl, subclass) < 0) {
                goto end;
            }
            result = Py_True;
            goto end;
        }
        if (r < 0) {
            goto end;
        }
    }

    /* No dice; update negative cache. */
    if (_add_to_negative_cache(self, subclass) < 0) {
        goto end;
    }
    result = Py_False;

end:
    Py_XDECREF(impl);
    Py_XDECREF(subclasses);
    if (result != NULL) {
        Py_INCREF(result);
    }
    return result;
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
    {"_abc_init", (PyCFunction)_abc_init, METH_VARARGS,
        _abc_init_doc},
    {"_reset_registry", (PyCFunction)_reset_registry, METH_VARARGS,
        _reset_registry_doc},
    {"_reset_caches", (PyCFunction)_reset_caches, METH_VARARGS,
        _reset_caches_doc},
    {"_get_dump", (PyCFunction)_get_dump, METH_VARARGS,
        _get_dump_doc},
    {"_abc_register", (PyCFunction)_abc_register, METH_VARARGS,
        _abc_register_doc},
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
    if (PyType_Ready(&_abc_data_type) < 0) {
        return NULL;
    }
    _abc_data_type.tp_doc = abc_data_doc;

    if (PyType_Ready(&_guarded_set_type) < 0) {
        return NULL;
    }
    _guarded_set_type.tp_doc = guarded_set_doc;

    abc_invalidation_counter = PyLong_FromLong(0);
    return PyModule_Create(&_abcmodule);
}
