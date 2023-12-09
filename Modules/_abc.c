/* ABCMeta implementation */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_moduleobject.h"  // _PyModule_GetState()
#include "pycore_object.h"        // _PyType_GetSubclasses()
#include "pycore_runtime.h"       // _Py_ID()
#include "pycore_typeobject.h"    // _PyType_GetMRO()
#include "clinic/_abc.c.h"

/*[clinic input]
module _abc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=964f5328e1aefcda]*/

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");

typedef struct {
    PyTypeObject *_abc_data_type;
    unsigned long long abc_invalidation_counter;
} _abcmodule_state;

static inline _abcmodule_state*
get_abc_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_abcmodule_state *)state;
}

/* This object stores internal state for ABCs.
   Note that we can use normal sets for caches,
   since they are never iterated over. */
typedef struct {
    PyObject_HEAD
    PyObject *_abc_registry;
    PyObject *_abc_cache; /* Normal set of weak references. */
    PyObject *_abc_negative_cache; /* Normal set of weak references. */
    unsigned long long _abc_negative_cache_version;
} _abc_data;

static int
abc_data_traverse(_abc_data *self, visitproc visit, void *arg)
{
    Py_VISIT(Py_TYPE(self));
    Py_VISIT(self->_abc_registry);
    Py_VISIT(self->_abc_cache);
    Py_VISIT(self->_abc_negative_cache);
    return 0;
}

static int
abc_data_clear(_abc_data *self)
{
    Py_CLEAR(self->_abc_registry);
    Py_CLEAR(self->_abc_cache);
    Py_CLEAR(self->_abc_negative_cache);
    return 0;
}

static void
abc_data_dealloc(_abc_data *self)
{
    PyObject_GC_UnTrack(self);
    PyTypeObject *tp = Py_TYPE(self);
    (void)abc_data_clear(self);
    tp->tp_free(self);
    Py_DECREF(tp);
}

static PyObject *
abc_data_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _abc_data *self = (_abc_data *) type->tp_alloc(type, 0);
    _abcmodule_state *state = NULL;
    if (self == NULL) {
        return NULL;
    }

    state = _PyType_GetModuleState(type);
    if (state == NULL) {
        Py_DECREF(self);
        return NULL;
    }

    self->_abc_registry = NULL;
    self->_abc_cache = NULL;
    self->_abc_negative_cache = NULL;
    self->_abc_negative_cache_version = state->abc_invalidation_counter;
    return (PyObject *) self;
}

PyDoc_STRVAR(abc_data_doc,
"Internal state held by ABC machinery.");

static PyType_Slot _abc_data_type_spec_slots[] = {
    {Py_tp_doc, (void *)abc_data_doc},
    {Py_tp_new, abc_data_new},
    {Py_tp_dealloc, abc_data_dealloc},
    {Py_tp_traverse, abc_data_traverse},
    {Py_tp_clear, abc_data_clear},
    {0, 0}
};

static PyType_Spec _abc_data_type_spec = {
    .name = "_abc._abc_data",
    .basicsize = sizeof(_abc_data),
    .flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC,
    .slots = _abc_data_type_spec_slots,
};

static _abc_data *
_get_impl(PyObject *module, PyObject *self)
{
    _abcmodule_state *state = get_abc_state(module);
    PyObject *impl = PyObject_GetAttr(self, &_Py_ID(_abc_impl));
    if (impl == NULL) {
        return NULL;
    }
    if (!Py_IS_TYPE(impl, state->_abc_data_type)) {
        PyErr_SetString(PyExc_TypeError, "_abc_impl is set to a wrong type");
        Py_DECREF(impl);
        return NULL;
    }
    return (_abc_data *)impl;
}

static int
_in_weak_set(PyObject *set, PyObject *obj)
{
    if (set == NULL || PySet_GET_SIZE(set) == 0) {
        return 0;
    }
    PyObject *ref = PyWeakref_NewRef(obj, NULL);
    if (ref == NULL) {
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

static PyObject *
_destroy(PyObject *setweakref, PyObject *objweakref)
{
    PyObject *set;
    set = PyWeakref_GET_OBJECT(setweakref);
    if (set == Py_None) {
        Py_RETURN_NONE;
    }
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

static int
_add_to_weak_set(PyObject **pset, PyObject *obj)
{
    if (*pset == NULL) {
        *pset = PySet_New(NULL);
        if (*pset == NULL) {
            return -1;
        }
    }

    PyObject *set = *pset;
    PyObject *ref, *wr;
    PyObject *destroy_cb;
    wr = PyWeakref_NewRef(set, NULL);
    if (wr == NULL) {
        return -1;
    }
    destroy_cb = PyCFunction_NewEx(&_destroy_def, wr, NULL);
    if (destroy_cb == NULL) {
        Py_DECREF(wr);
        return -1;
    }
    ref = PyWeakref_NewRef(obj, destroy_cb);
    Py_DECREF(destroy_cb);
    if (ref == NULL) {
        Py_DECREF(wr);
        return -1;
    }
    int ret = PySet_Add(set, ref);
    Py_DECREF(wr);
    Py_DECREF(ref);
    return ret;
}

/*[clinic input]
_abc._reset_registry

    self: object
    /

Internal ABC helper to reset registry of a given class.

Should be only used by refleak.py
[clinic start generated code]*/

static PyObject *
_abc__reset_registry(PyObject *module, PyObject *self)
/*[clinic end generated code: output=92d591a43566cc10 input=12a0b7eb339ac35c]*/
{
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }
    if (impl->_abc_registry != NULL && PySet_Clear(impl->_abc_registry) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    Py_DECREF(impl);
    Py_RETURN_NONE;
}

/*[clinic input]
_abc._reset_caches

    self: object
    /

Internal ABC helper to reset both caches of a given class.

Should be only used by refleak.py
[clinic start generated code]*/

static PyObject *
_abc__reset_caches(PyObject *module, PyObject *self)
/*[clinic end generated code: output=f296f0d5c513f80c input=c0ac616fd8acfb6f]*/
{
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }
    if (impl->_abc_cache != NULL && PySet_Clear(impl->_abc_cache) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    /* also the second cache */
    if (impl->_abc_negative_cache != NULL &&
            PySet_Clear(impl->_abc_negative_cache) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    Py_DECREF(impl);
    Py_RETURN_NONE;
}

/*[clinic input]
_abc._get_dump

    self: object
    /

Internal ABC helper for cache and registry debugging.

Return shallow copies of registry, of both caches, and
negative cache version. Don't call this function directly,
instead use ABC._dump_registry() for a nice repr.
[clinic start generated code]*/

static PyObject *
_abc__get_dump(PyObject *module, PyObject *self)
/*[clinic end generated code: output=9d9569a8e2c1c443 input=2c5deb1bfe9e3c79]*/
{
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }
    PyObject *res = Py_BuildValue("NNNK",
                                  PySet_New(impl->_abc_registry),
                                  PySet_New(impl->_abc_cache),
                                  PySet_New(impl->_abc_negative_cache),
                                  impl->_abc_negative_cache_version);
    Py_DECREF(impl);
    return res;
}

// Compute set of abstract method names.
static int
compute_abstract_methods(PyObject *self)
{
    int ret = -1;
    PyObject *abstracts = PyFrozenSet_New(NULL);
    if (abstracts == NULL) {
        return -1;
    }

    PyObject *ns = NULL, *items = NULL, *bases = NULL;  // Py_XDECREF()ed on error.

    /* Stage 1: direct abstract methods. */
    ns = PyObject_GetAttr(self, &_Py_ID(__dict__));
    if (!ns) {
        goto error;
    }

    // We can't use PyDict_Next(ns) even when ns is dict because
    // _PyObject_IsAbstract() can mutate ns.
    items = PyMapping_Items(ns);
    if (!items) {
        goto error;
    }
    assert(PyList_Check(items));
    for (Py_ssize_t pos = 0; pos < PyList_GET_SIZE(items); pos++) {
        PyObject *it = PySequence_Fast(
                PyList_GET_ITEM(items, pos),
                "items() returned non-iterable");
        if (!it) {
            goto error;
        }
        if (PySequence_Fast_GET_SIZE(it) != 2) {
            PyErr_SetString(PyExc_TypeError,
                            "items() returned item which size is not 2");
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
    bases = PyObject_GetAttr(self, &_Py_ID(__bases__));
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

        if (_PyObject_LookupAttr(item, &_Py_ID(__abstractmethods__),
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
                    (is_abstract && PySet_Add(abstracts, key) < 0))
            {
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

    if (PyObject_SetAttr(self, &_Py_ID(__abstractmethods__), abstracts) < 0) {
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

#define COLLECTION_FLAGS (Py_TPFLAGS_SEQUENCE | Py_TPFLAGS_MAPPING)

/*[clinic input]
_abc._abc_init

    self: object
    /

Internal ABC helper for class set-up. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_init(PyObject *module, PyObject *self)
/*[clinic end generated code: output=594757375714cda1 input=8d7fe470ff77f029]*/
{
    _abcmodule_state *state = get_abc_state(module);
    PyObject *data;
    if (compute_abstract_methods(self) < 0) {
        return NULL;
    }

    /* Set up inheritance registry. */
    data = abc_data_new(state->_abc_data_type, NULL, NULL);
    if (data == NULL) {
        return NULL;
    }
    if (PyObject_SetAttr(self, &_Py_ID(_abc_impl), data) < 0) {
        Py_DECREF(data);
        return NULL;
    }
    Py_DECREF(data);
    /* If __abc_tpflags__ & COLLECTION_FLAGS is set, then set the corresponding bit(s)
     * in the new class.
     * Used by collections.abc.Sequence and collections.abc.Mapping to indicate
     * their special status w.r.t. pattern matching. */
    if (PyType_Check(self)) {
        PyTypeObject *cls = (PyTypeObject *)self;
        PyObject *dict = _PyType_GetDict(cls);
        PyObject *flags = PyDict_GetItemWithError(dict,
                                                  &_Py_ID(__abc_tpflags__));
        if (flags == NULL) {
            if (PyErr_Occurred()) {
                return NULL;
            }
        }
        else {
            if (PyLong_CheckExact(flags)) {
                long val = PyLong_AsLong(flags);
                if (val == -1 && PyErr_Occurred()) {
                    return NULL;
                }
                if ((val & COLLECTION_FLAGS) == COLLECTION_FLAGS) {
                    PyErr_SetString(PyExc_TypeError, "__abc_tpflags__ cannot be both Py_TPFLAGS_SEQUENCE and Py_TPFLAGS_MAPPING");
                    return NULL;
                }
                ((PyTypeObject *)self)->tp_flags |= (val & COLLECTION_FLAGS);
            }
            if (PyDict_DelItem(dict, &_Py_ID(__abc_tpflags__)) < 0) {
                return NULL;
            }
        }
    }
    Py_RETURN_NONE;
}

static void
set_collection_flag_recursive(PyTypeObject *child, unsigned long flag)
{
    assert(flag == Py_TPFLAGS_MAPPING || flag == Py_TPFLAGS_SEQUENCE);
    if (PyType_HasFeature(child, Py_TPFLAGS_IMMUTABLETYPE) ||
        (child->tp_flags & COLLECTION_FLAGS) == flag)
    {
        return;
    }

    child->tp_flags &= ~COLLECTION_FLAGS;
    child->tp_flags |= flag;

    PyObject *grandchildren = _PyType_GetSubclasses(child);
    if (grandchildren == NULL) {
        return;
    }

    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(grandchildren); i++) {
        PyObject *grandchild = PyList_GET_ITEM(grandchildren, i);
        set_collection_flag_recursive((PyTypeObject *)grandchild, flag);
    }
    Py_DECREF(grandchildren);
}

/*[clinic input]
_abc._abc_register

    self: object
    subclass: object
    /

Internal ABC helper for subclasss registration. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_register_impl(PyObject *module, PyObject *self, PyObject *subclass)
/*[clinic end generated code: output=7851e7668c963524 input=ca589f8c3080e67f]*/
{
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "Can only register classes");
        return NULL;
    }
    int result = PyObject_IsSubclass(subclass, self);
    if (result > 0) {
        return Py_NewRef(subclass);  /* Already a subclass. */
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
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }
    if (_add_to_weak_set(&impl->_abc_registry, subclass) < 0) {
        Py_DECREF(impl);
        return NULL;
    }
    Py_DECREF(impl);

    /* Invalidate negative cache */
    get_abc_state(module)->abc_invalidation_counter++;

    /* Set Py_TPFLAGS_SEQUENCE  or Py_TPFLAGS_MAPPING flag */
    if (PyType_Check(self)) {
        unsigned long collection_flag = ((PyTypeObject *)self)->tp_flags & COLLECTION_FLAGS;
        if (collection_flag) {
            set_collection_flag_recursive((PyTypeObject *)subclass, collection_flag);
        }
    }
    return Py_NewRef(subclass);
}


/*[clinic input]
_abc._abc_instancecheck

    self: object
    instance: object
    /

Internal ABC helper for instance checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_instancecheck_impl(PyObject *module, PyObject *self,
                             PyObject *instance)
/*[clinic end generated code: output=b8b5148f63b6b56f input=a4f4525679261084]*/
{
    PyObject *subtype, *result = NULL, *subclass = NULL;
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }

    subclass = PyObject_GetAttr(instance, &_Py_ID(__class__));
    if (subclass == NULL) {
        Py_DECREF(impl);
        return NULL;
    }
    /* Inline the cache checking. */
    int incache = _in_weak_set(impl->_abc_cache, subclass);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_NewRef(Py_True);
        goto end;
    }
    subtype = (PyObject *)Py_TYPE(instance);
    if (subtype == subclass) {
        if (impl->_abc_negative_cache_version == get_abc_state(module)->abc_invalidation_counter) {
            incache = _in_weak_set(impl->_abc_negative_cache, subclass);
            if (incache < 0) {
                goto end;
            }
            if (incache > 0) {
                result = Py_NewRef(Py_False);
                goto end;
            }
        }
        /* Fall back to the subclass check. */
        result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                           subclass);
        goto end;
    }
    result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                       subclass);
    if (result == NULL) {
        goto end;
    }

    switch (PyObject_IsTrue(result)) {
    case -1:
        Py_SETREF(result, NULL);
        break;
    case 0:
        Py_DECREF(result);
        result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                           subtype);
        break;
    case 1:  // Nothing to do.
        break;
    default:
        Py_UNREACHABLE();
    }

end:
    Py_XDECREF(impl);
    Py_XDECREF(subclass);
    return result;
}


// Return -1 when exception occurred.
// Return 1 when result is set.
// Return 0 otherwise.
static int subclasscheck_check_registry(_abc_data *impl, PyObject *subclass,
                                        PyObject **result);

/*[clinic input]
_abc._abc_subclasscheck

    self: object
    subclass: object
    /

Internal ABC helper for subclasss checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_subclasscheck_impl(PyObject *module, PyObject *self,
                             PyObject *subclass)
/*[clinic end generated code: output=b56c9e4a530e3894 input=1d947243409d10b8]*/
{
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "issubclass() arg 1 must be a class");
        return NULL;
    }

    PyObject *ok, *subclasses = NULL, *result = NULL;
    _abcmodule_state *state = NULL;
    Py_ssize_t pos;
    int incache;
    _abc_data *impl = _get_impl(module, self);
    if (impl == NULL) {
        return NULL;
    }

    /* 1. Check cache. */
    incache = _in_weak_set(impl->_abc_cache, subclass);
    if (incache < 0) {
        goto end;
    }
    if (incache > 0) {
        result = Py_True;
        goto end;
    }

    state = get_abc_state(module);
    /* 2. Check negative cache; may have to invalidate. */
    if (impl->_abc_negative_cache_version < state->abc_invalidation_counter) {
        /* Invalidate the negative cache. */
        if (impl->_abc_negative_cache != NULL &&
                PySet_Clear(impl->_abc_negative_cache) < 0)
        {
            goto end;
        }
        impl->_abc_negative_cache_version = state->abc_invalidation_counter;
    }
    else {
        incache = _in_weak_set(impl->_abc_negative_cache, subclass);
        if (incache < 0) {
            goto end;
        }
        if (incache > 0) {
            result = Py_False;
            goto end;
        }
    }

    /* 3. Check the subclass hook. */
    ok = PyObject_CallMethodOneArg(
            (PyObject *)self, &_Py_ID(__subclasshook__), subclass);
    if (ok == NULL) {
        goto end;
    }
    if (ok == Py_True) {
        Py_DECREF(ok);
        if (_add_to_weak_set(&impl->_abc_cache, subclass) < 0) {
            goto end;
        }
        result = Py_True;
        goto end;
    }
    if (ok == Py_False) {
        Py_DECREF(ok);
        if (_add_to_weak_set(&impl->_abc_negative_cache, subclass) < 0) {
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
    PyObject *mro = _PyType_GetMRO((PyTypeObject *)subclass);
    assert(PyTuple_Check(mro));
    for (pos = 0; pos < PyTuple_GET_SIZE(mro); pos++) {
        PyObject *mro_item = PyTuple_GET_ITEM(mro, pos);
        assert(mro_item != NULL);
        if ((PyObject *)self == mro_item) {
            if (_add_to_weak_set(&impl->_abc_cache, subclass) < 0) {
                goto end;
            }
            result = Py_True;
            goto end;
        }
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    if (subclasscheck_check_registry(impl, subclass, &result)) {
        // Exception occurred or result is set.
        goto end;
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    subclasses = PyObject_CallMethod(self, "__subclasses__", NULL);
    if (subclasses == NULL) {
        goto end;
    }
    if (!PyList_Check(subclasses)) {
        PyErr_SetString(PyExc_TypeError, "__subclasses__() must return a list");
        goto end;
    }
    for (pos = 0; pos < PyList_GET_SIZE(subclasses); pos++) {
        PyObject *scls = PyList_GET_ITEM(subclasses, pos);
        Py_INCREF(scls);
        int r = PyObject_IsSubclass(subclass, scls);
        Py_DECREF(scls);
        if (r > 0) {
            if (_add_to_weak_set(&impl->_abc_cache, subclass) < 0) {
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
    if (_add_to_weak_set(&impl->_abc_negative_cache, subclass) < 0) {
        goto end;
    }
    result = Py_False;

end:
    Py_DECREF(impl);
    Py_XDECREF(subclasses);
    return Py_XNewRef(result);
}


static int
subclasscheck_check_registry(_abc_data *impl, PyObject *subclass,
                             PyObject **result)
{
    // Fast path: check subclass is in weakref directly.
    int ret = _in_weak_set(impl->_abc_registry, subclass);
    if (ret < 0) {
        *result = NULL;
        return -1;
    }
    if (ret > 0) {
        *result = Py_True;
        return 1;
    }

    if (impl->_abc_registry == NULL) {
        return 0;
    }
    Py_ssize_t registry_size = PySet_Size(impl->_abc_registry);
    if (registry_size == 0) {
        return 0;
    }
    // Weakref callback may remove entry from set.
    // So we take snapshot of registry first.
    PyObject **copy = PyMem_Malloc(sizeof(PyObject*) * registry_size);
    if (copy == NULL) {
        PyErr_NoMemory();
        return -1;
    }
    PyObject *key;
    Py_ssize_t pos = 0;
    Py_hash_t hash;
    Py_ssize_t i = 0;

    while (_PySet_NextEntry(impl->_abc_registry, &pos, &key, &hash)) {
        copy[i++] = Py_NewRef(key);
    }
    assert(i == registry_size);

    for (i = 0; i < registry_size; i++) {
        PyObject *rkey = PyWeakref_GetObject(copy[i]);
        if (rkey == NULL) {
            // Someone inject non-weakref type in the registry.
            ret = -1;
            break;
        }
        if (rkey == Py_None) {
            continue;
        }
        Py_INCREF(rkey);
        int r = PyObject_IsSubclass(subclass, rkey);
        Py_DECREF(rkey);
        if (r < 0) {
            ret = -1;
            break;
        }
        if (r > 0) {
            if (_add_to_weak_set(&impl->_abc_cache, subclass) < 0) {
                ret = -1;
                break;
            }
            *result = Py_True;
            ret = 1;
            break;
        }
    }

    for (i = 0; i < registry_size; i++) {
        Py_DECREF(copy[i]);
    }
    PyMem_Free(copy);
    return ret;
}

/*[clinic input]
_abc.get_cache_token

Returns the current ABC cache token.

The token is an opaque object (supporting equality testing) identifying the
current version of the ABC cache for virtual subclasses. The token changes
with every call to register() on any ABC.
[clinic start generated code]*/

static PyObject *
_abc_get_cache_token_impl(PyObject *module)
/*[clinic end generated code: output=c7d87841e033dacc input=70413d1c423ad9f9]*/
{
    _abcmodule_state *state = get_abc_state(module);
    return PyLong_FromUnsignedLongLong(state->abc_invalidation_counter);
}

static struct PyMethodDef _abcmodule_methods[] = {
    _ABC_GET_CACHE_TOKEN_METHODDEF
    _ABC__ABC_INIT_METHODDEF
    _ABC__RESET_REGISTRY_METHODDEF
    _ABC__RESET_CACHES_METHODDEF
    _ABC__GET_DUMP_METHODDEF
    _ABC__ABC_REGISTER_METHODDEF
    _ABC__ABC_INSTANCECHECK_METHODDEF
    _ABC__ABC_SUBCLASSCHECK_METHODDEF
    {NULL,       NULL}          /* sentinel */
};

static int
_abcmodule_exec(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    state->abc_invalidation_counter = 0;
    state->_abc_data_type = (PyTypeObject *)PyType_FromModuleAndSpec(module, &_abc_data_type_spec, NULL);
    if (state->_abc_data_type == NULL) {
        return -1;
    }

    return 0;
}

static int
_abcmodule_traverse(PyObject *module, visitproc visit, void *arg)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_VISIT(state->_abc_data_type);
    return 0;
}

static int
_abcmodule_clear(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_CLEAR(state->_abc_data_type);
    return 0;
}

static void
_abcmodule_free(void *module)
{
    _abcmodule_clear((PyObject *)module);
}

static PyModuleDef_Slot _abcmodule_slots[] = {
    {Py_mod_exec, _abcmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {0, NULL}
};

static struct PyModuleDef _abcmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "_abc",
    .m_doc = _abc__doc__,
    .m_size = sizeof(_abcmodule_state),
    .m_methods = _abcmodule_methods,
    .m_slots = _abcmodule_slots,
    .m_traverse = _abcmodule_traverse,
    .m_clear = _abcmodule_clear,
    .m_free = _abcmodule_free,
};

PyMODINIT_FUNC
PyInit__abc(void)
{
    return PyModuleDef_Init(&_abcmodule);
}
