/* ABCMeta implementation */
#ifndef Py_BUILD_CORE_BUILTIN
#  define Py_BUILD_CORE_MODULE 1
#endif

#include "Python.h"
#include "pycore_critical_section.h"  // Py_BEGIN_CRITICAL_SECTION()
#include "pycore_moduleobject.h"      // _PyModule_GetState()
#include "pycore_object.h"            // _PyType_LookupStackRefAndVersion()
#include "pycore_object_deferred.h"   // _PyObject_SetDeferredRefcount()
#include "pycore_pyatomic_ft_wrappers.h"  // FT_ATOMIC_LOAD_PTR_ACQUIRE()
#include "pycore_pymem.h"             // _PyMem_FreeDelayed()
#include "pycore_pystate.h"           // _PyThreadState_GET()
#include "pycore_runtime.h"           // _Py_ID()
#include "pycore_stackref.h"          // _PyStackRef
#include "pycore_typeobject.h"        // _PyType_GetSubclasses()
#include "pycore_pyhash.h"
#include "clinic/_abc.c.h"

/*[clinic input]
module _abc
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=964f5328e1aefcda]*/

PyDoc_STRVAR(_abc__doc__,
"Module contains faster C implementation of abc.ABCMeta");

typedef struct {
    PyTypeObject *_abc_data_type;
    uint64_t abc_invalidation_counter;
    // Default implementations, compared by identity to skip the call
    // when a class doesn't override them.
    PyObject *object_class_descr;
    PyObject *object_subclasshook;
    PyObject *type_subclasses;
    // ABCMeta.__subclasscheck__, set by abc.py on import. Classes using it
    // are checked directly instead of going through issubclass().
    PyObject *subclasscheck_func;
} _abcmodule_state;

static inline _abcmodule_state*
get_abc_state(PyObject *module)
{
    void *state = _PyModule_GetState(module);
    assert(state != NULL);
    return (_abcmodule_state *)state;
}

static inline uint64_t
get_invalidation_counter(_abcmodule_state *state)
{
    return FT_ATOMIC_LOAD_UINT64_RELAXED(state->abc_invalidation_counter);
}

static inline void
increment_invalidation_counter(_abcmodule_state *state)
{
    FT_ATOMIC_ADD_UINT64(state->abc_invalidation_counter, 1);
}


// The registry and the caches of an ABC are sets of classes, stored as
// open addressed hash tables keyed by the address of the class.
//
// Each entry also holds a weakref to the class. It is only used to check
// that the class is still alive, as the address of a dead class can be
// reused by a new one. The weakrefs have no callback, so the same weakref
// is shared by all the tables a class is in, and dead entries are only
// removed when the table is rebuilt.
typedef struct {
    PyObject *key;   /* borrowed ref of a class */
    PyObject *ref;   /* weakref to key */
} abc_entry;

typedef struct {
    size_t mask;     /* number of entries - 1 */
    size_t used;     /* entries with a key, live or dead */
    abc_entry entries[];
} abc_table;

#define TABLE_MIN_SIZE 8
#define PERTURB_SHIFT 5

// Shared empty table, so that lookups don't need a NULL check.
// It is never modified or freed.
static abc_table empty_table_struct = {
    0,              /* mask */
    0,              /* used */
    {{NULL, NULL}}, /* entries */
};

#define EMPTY_TABLE (&empty_table_struct)

static inline size_t
abc_table_nbytes(size_t size)
{
    return offsetof(abc_table, entries) + size * sizeof(abc_entry);
}

static abc_table *
abc_table_new(size_t size)
{
    assert((size & (size - 1)) == 0 && size >= TABLE_MIN_SIZE);
    abc_table *table = PyMem_Calloc(1, abc_table_nbytes(size));
    if (table == NULL) {
        PyErr_NoMemory();
        return NULL;
    }
    table->mask = size - 1;
    table->used = 0;
    return table;
}

// Free a table and its weakrefs. Use delayed if other threads
// can still be reading the table.
static void
abc_table_release(abc_table *table, int delayed)
{
    if (table == EMPTY_TABLE) {
        return;
    }
    for (size_t i = 0; i <= table->mask; i++) {
        PyObject *ref = table->entries[i].ref;
        if (delayed) {
            _PyObject_XDecRefDelayed(ref);
        }
        else {
            Py_XDECREF(ref);
        }
    }
    if (delayed) {
        _PyMem_FreeDelayed(table, abc_table_nbytes(table->mask + 1));
    }
    else {
        PyMem_Free(table);
    }
}

static inline size_t
abc_hash(PyObject *key)
{
    return (size_t)_Py_HashPointerRaw(key);
}

static inline int
abc_entry_is_live(PyObject *ref, PyObject *key)
{
    // The caller holds a strong reference to key, so a weakref pointing
    // to key cannot be of a dead class which was at the same address.
    // wr_object only changes when key dies, so a relaxed load is enough.
    assert(ref != NULL);
    PyWeakReference *wr = (PyWeakReference *)ref;
    return FT_ATOMIC_LOAD_PTR_RELAXED(wr->wr_object) == key;
}

static abc_entry *
abc_table_find(abc_table *table, PyObject *key, PyObject **found)
{
    size_t mask = table->mask;
    size_t perturb = abc_hash(key);
    size_t i = perturb & mask;
    for (;;) {
        abc_entry *entry = &table->entries[i];
        PyObject *k = FT_ATOMIC_LOAD_PTR_ACQUIRE(entry->key);
        if (k == NULL || k == key) {
            *found = k;
            return entry;
        }
        perturb >>= PERTURB_SHIFT;
        i = (i * 5 + perturb + 1) & mask;
    }
}

// Lock-free, the caller must hold a strong reference to key.
static int
abc_table_contains(abc_table *table, PyObject *key)
{
    PyObject *found;
    abc_entry *entry = abc_table_find(table, key, &found);
    if (found == NULL) {
        return 0;
    }
    PyObject *ref = FT_ATOMIC_LOAD_PTR_ACQUIRE(entry->ref);
    return abc_entry_is_live(ref, key);
}

static abc_table *
abc_table_rebuild(abc_table *old, size_t extra)
{
    size_t live = 0;
    for (size_t i = 0; i <= old->mask; i++) {
        abc_entry *entry = &old->entries[i];
        if (entry->key != NULL && abc_entry_is_live(entry->ref, entry->key)) {
            live++;
        }
    }
    size_t size = TABLE_MIN_SIZE;
    while (size < (live + extra) * 2) {
        size <<= 1;
    }
    abc_table *table = abc_table_new(size);
    if (table == NULL) {
        return NULL;
    }
    for (size_t i = 0; i <= old->mask; i++) {
        abc_entry *entry = &old->entries[i];
        if (entry->key == NULL) {
            continue;
        }
        if (abc_entry_is_live(entry->ref, entry->key)) {
            PyObject *found;
            abc_entry *dst = abc_table_find(table, entry->key, &found);
            assert(found == NULL);
            dst->key = entry->key;
            dst->ref = entry->ref;
            table->used++;
        }
        else {
            _PyObject_XDecRefDelayed(entry->ref);
        }
    }
    return table;
}

static int
abc_table_add(abc_table **ptable, PyObject *key, PyObject *ref)
{
    abc_table *table = *ptable;
    PyObject *found;
    abc_entry *entry = abc_table_find(table, key, &found);
    if (found == key) {
        // The existing weakref can be of a dead class at the same address.
        PyObject *old = entry->ref;
        if (old == ref) {
            Py_DECREF(ref);
        }
        else {
            FT_ATOMIC_STORE_PTR_RELEASE(entry->ref, ref);
            _PyObject_XDecRefDelayed(old);
        }
        return 0;
    }
    // Always true for EMPTY_TABLE.
    if ((table->used + 1) * 3 > (table->mask + 1) * 2) {
        abc_table *newtable = abc_table_rebuild(table, 1);
        if (newtable == NULL) {
            Py_DECREF(ref);
            return -1;
        }
        FT_ATOMIC_STORE_PTR_RELEASE(*ptable, newtable);
        if (table != EMPTY_TABLE) {
            // The weakrefs were moved to newtable.
            _PyMem_FreeDelayed(table, abc_table_nbytes(table->mask + 1));
        }
        table = newtable;
        entry = abc_table_find(table, key, &found);
    }
    assert(found == NULL);
    FT_ATOMIC_STORE_PTR_RELAXED(entry->ref, ref);
    FT_ATOMIC_STORE_PTR_RELEASE(entry->key, key);
    table->used++;
    return 0;
}

static void
abc_table_clear(abc_table **ptable)
{
    abc_table *table = *ptable;
    if (table != EMPTY_TABLE) {
        FT_ATOMIC_STORE_PTR_RELAXED(*ptable, EMPTY_TABLE);
        abc_table_release(table, 1);
    }
}

static PyObject *
abc_table_snapshot(abc_table *table)
{
    PyObject *list = PyList_New(0);
    if (list == NULL) {
        return NULL;
    }
    for (size_t i = 0; i <= table->mask; i++) {
        abc_entry *entry = &table->entries[i];
        if (entry->key == NULL) {
            continue;
        }
        PyObject *obj;
        if (PyWeakref_GetRef(entry->ref, &obj) < 0) {
            Py_DECREF(list);
            return NULL;
        }
        if (obj == NULL) {
            continue;
        }
        int rc = PyList_Append(list, obj);
        Py_DECREF(obj);
        if (rc < 0) {
            Py_DECREF(list);
            return NULL;
        }
    }
    return list;
}

static int
abc_table_traverse(abc_table *table, visitproc visit, void *arg)
{
    for (size_t i = 0; i <= table->mask; i++) {
        Py_VISIT(table->entries[i].ref);
    }
    return 0;
}


// Internal state of an ABC, stored in its _abc_impl attribute.

typedef struct {
    PyObject_HEAD
    abc_table *registry;
    abc_table *cache;
    abc_table *negative_cache;
    uint64_t negative_cache_version;
} _abc_data;

#define _abc_data_CAST(op)  ((_abc_data *)(op))

// The version is updated after the negative cache is cleared and read
// before the negative cache is, so a reader which sees the new version
// never sees the old entries.
static inline uint64_t
get_cache_version(_abc_data *impl)
{
    return FT_ATOMIC_LOAD_UINT64_ACQUIRE(impl->negative_cache_version);
}

static inline void
set_cache_version(_abc_data *impl, uint64_t version)
{
#ifdef Py_GIL_DISABLED
    _Py_atomic_store_uint64_release(&impl->negative_cache_version, version);
#else
    impl->negative_cache_version = version;
#endif
}

static int
abc_data_traverse(PyObject *op, visitproc visit, void *arg)
{
    _abc_data *self = _abc_data_CAST(op);
    Py_VISIT(Py_TYPE(self));
    if (abc_table_traverse(self->registry, visit, arg) < 0
        || abc_table_traverse(self->cache, visit, arg) < 0
        || abc_table_traverse(self->negative_cache, visit, arg) < 0)
    {
        return -1;
    }
    return 0;
}

static int
abc_data_clear(PyObject *op)
{
    _abc_data *self = _abc_data_CAST(op);
    abc_table *registry = self->registry;
    abc_table *cache = self->cache;
    abc_table *negative_cache = self->negative_cache;
    self->registry = EMPTY_TABLE;
    self->cache = EMPTY_TABLE;
    self->negative_cache = EMPTY_TABLE;
    abc_table_release(registry, 0);
    abc_table_release(cache, 0);
    abc_table_release(negative_cache, 0);
    return 0;
}

static void
abc_data_dealloc(PyObject *self)
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

    self->registry = EMPTY_TABLE;
    self->cache = EMPTY_TABLE;
    self->negative_cache = EMPTY_TABLE;
    self->negative_cache_version = get_invalidation_counter(state);
    // Avoid refcount contention between threads.
    _PyObject_SetDeferredRefcount((PyObject *)self);
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
lookup_impl(_abcmodule_state *state, PyObject *self, _PyCStackRef *cref)
{
    if (!PyType_Check(self)) {
        return NULL;
    }
    _PyType_LookupStackRefAndVersion((PyTypeObject *)self, &_Py_ID(_abc_impl),
                                     &cref->ref);
    if (PyStackRef_IsNull(cref->ref)) {
        return NULL;
    }
    PyObject *impl = PyStackRef_AsPyObjectBorrow(cref->ref);
    if (!Py_IS_TYPE(impl, state->_abc_data_type)) {
        PyStackRef_CLOSE(cref->ref);
        cref->ref = PyStackRef_NULL;
        return NULL;
    }
    return (_abc_data *)impl;
}

// Like lookup_impl() but falls back to a full attribute lookup,
// and sets an exception on failure.
static _abc_data *
get_impl(_abcmodule_state *state, PyObject *self, _PyCStackRef *cref)
{
    _abc_data *impl = lookup_impl(state, self, cref);
    if (impl != NULL) {
        return impl;
    }
    cref->ref = _PyObject_GetAttrStackRef(self, &_Py_ID(_abc_impl));
    if (PyStackRef_IsNull(cref->ref)) {
        return NULL;
    }
    impl = (_abc_data *)PyStackRef_AsPyObjectBorrow(cref->ref);
    if (!Py_IS_TYPE(impl, state->_abc_data_type)) {
        PyErr_SetString(PyExc_TypeError, "_abc_impl is set to a wrong type");
        PyStackRef_CLOSE(cref->ref);
        cref->ref = PyStackRef_NULL;
        return NULL;
    }
    return impl;
}

static int
type_lookup_is(PyTypeObject *type, PyObject *name, PyObject *expected)
{
    _PyStackRef ref;
    _PyType_LookupStackRefAndVersion(type, name, &ref);
    if (PyStackRef_IsNull(ref)) {
        return expected == NULL;
    }
    int res = (PyStackRef_AsPyObjectBorrow(ref) == expected);
    PyStackRef_CLOSE(ref);
    return res;
}

static int
add_to_table(_abc_data *impl, abc_table **ptable, PyObject *cls)
{
    // PyWeakref_NewRef() can lock cls, call it outside the critical section.
    PyObject *ref = PyWeakref_NewRef(cls, NULL);
    if (ref == NULL) {
        return -1;
    }
    int res;
    Py_BEGIN_CRITICAL_SECTION(impl);
    res = abc_table_add(ptable, cls, ref);
    Py_END_CRITICAL_SECTION();
    return res;
}

typedef enum {
    CACHE_UNKNOWN = -1,  // not cached or stale
    CACHE_NEGATIVE = 0,  // in the negative cache
    CACHE_POSITIVE = 1,  // in the positive cache
} cache_result;

static cache_result
lookup_caches(_abcmodule_state *state, _abc_data *impl, PyObject *subclass)
{
    abc_table *table = FT_ATOMIC_LOAD_PTR_ACQUIRE(impl->cache);
    if (abc_table_contains(table, subclass)) {
        return CACHE_POSITIVE;
    }
    if (get_cache_version(impl) == get_invalidation_counter(state)) {
        table = FT_ATOMIC_LOAD_PTR_ACQUIRE(impl->negative_cache);
        if (abc_table_contains(table, subclass)) {
            return CACHE_NEGATIVE;
        }
    }
    return CACHE_UNKNOWN;
}

// Clear the negative cache if register() was called since it was last
// validated. Returns 1 if it was cleared.
static int
validate_negative_cache(_abc_data *impl, uint64_t counter)
{
    if (get_cache_version(impl) == counter) {
        return 0;
    }
    int cleared = 0;
    Py_BEGIN_CRITICAL_SECTION(impl);
    if (impl->negative_cache_version != counter) {
        abc_table_clear(&impl->negative_cache);
        set_cache_version(impl, counter);
        cleared = 1;
    }
    Py_END_CRITICAL_SECTION();
    return cleared;
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
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(get_abc_state(module), self, &cref);
    if (impl != NULL) {
        Py_BEGIN_CRITICAL_SECTION(impl);
        abc_table_clear(&impl->registry);
        Py_END_CRITICAL_SECTION();
    }
    _PyThreadState_PopCStackRef(tstate, &cref);
    if (impl == NULL) {
        return NULL;
    }
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
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(get_abc_state(module), self, &cref);
    if (impl != NULL) {
        Py_BEGIN_CRITICAL_SECTION(impl);
        abc_table_clear(&impl->cache);
        abc_table_clear(&impl->negative_cache);
        Py_END_CRITICAL_SECTION();
    }
    _PyThreadState_PopCStackRef(tstate, &cref);
    if (impl == NULL) {
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
weakref_set_from_list(PyObject *list)
{
    PyObject *set = PySet_New(NULL);
    if (set == NULL) {
        return NULL;
    }
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(list); i++) {
        PyObject *ref = PyWeakref_NewRef(PyList_GET_ITEM(list, i), NULL);
        if (ref == NULL) {
            Py_DECREF(set);
            return NULL;
        }
        int rc = PySet_Add(set, ref);
        Py_DECREF(ref);
        if (rc < 0) {
            Py_DECREF(set);
            return NULL;
        }
    }
    return set;
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
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(get_abc_state(module), self, &cref);
    if (impl == NULL) {
        _PyThreadState_PopCStackRef(tstate, &cref);
        return NULL;
    }
    PyObject *lists[3] = {NULL, NULL, NULL};
    PyObject *sets[3] = {NULL, NULL, NULL};
    PyObject *res = NULL;
    uint64_t version;
    Py_BEGIN_CRITICAL_SECTION(impl);
    lists[0] = abc_table_snapshot(impl->registry);
    lists[1] = abc_table_snapshot(impl->cache);
    lists[2] = abc_table_snapshot(impl->negative_cache);
    version = get_cache_version(impl);
    Py_END_CRITICAL_SECTION();
    _PyThreadState_PopCStackRef(tstate, &cref);
    for (int i = 0; i < 3; i++) {
        if (lists[i] == NULL) {
            goto done;
        }
        sets[i] = weakref_set_from_list(lists[i]);
        if (sets[i] == NULL) {
            goto done;
        }
    }
    res = Py_BuildValue("OOOK", sets[0], sets[1], sets[2],
                        (unsigned long long)version);
done:
    for (int i = 0; i < 3; i++) {
        Py_XDECREF(lists[i]);
        Py_XDECREF(sets[i]);
    }
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

        if (PyObject_GetOptionalAttr(item, &_Py_ID(__abstractmethods__),
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
            if (PyObject_GetOptionalAttr(self, key, &value) < 0) {
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
@permit_long_summary
_abc._abc_init

    self: object
    /

Internal ABC helper for class set-up. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_init(PyObject *module, PyObject *self)
/*[clinic end generated code: output=594757375714cda1 input=0b3513f947736d39]*/
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
        PyObject *flags = NULL;
        if (PyDict_Pop(dict, &_Py_ID(__abc_tpflags__), &flags) < 0) {
            return NULL;
        }
        if (flags == NULL || !PyLong_CheckExact(flags)) {
            Py_XDECREF(flags);
            Py_RETURN_NONE;
        }

        long val = PyLong_AsLong(flags);
        Py_DECREF(flags);
        if (val == -1 && PyErr_Occurred()) {
            return NULL;
        }
        if ((val & COLLECTION_FLAGS) == COLLECTION_FLAGS) {
            PyErr_SetString(PyExc_TypeError, "__abc_tpflags__ cannot be both Py_TPFLAGS_SEQUENCE and Py_TPFLAGS_MAPPING");
            return NULL;
        }
        _PyType_SetFlags((PyTypeObject *)self, 0, val & COLLECTION_FLAGS);
    }
    Py_RETURN_NONE;
}

/*[clinic input]
@permit_long_summary
_abc._abc_register

    self: object
    subclass: object
    /

Internal ABC helper for subclasss registration. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_register_impl(PyObject *module, PyObject *self, PyObject *subclass)
/*[clinic end generated code: output=7851e7668c963524 input=135ab13a581b4414]*/
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
    _abcmodule_state *state = get_abc_state(module);
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(state, self, &cref);
    int err = (impl == NULL
               || add_to_table(impl, &impl->registry, subclass) < 0);
    _PyThreadState_PopCStackRef(tstate, &cref);
    if (err) {
        return NULL;
    }

    /* Invalidate negative cache */
    increment_invalidation_counter(state);

    /* Set Py_TPFLAGS_SEQUENCE or Py_TPFLAGS_MAPPING flag */
    if (PyType_Check(self)) {
        unsigned long collection_flag =
            PyType_GetFlags((PyTypeObject *)self) & COLLECTION_FLAGS;
        if (collection_flag) {
            _PyType_SetFlagsRecursive((PyTypeObject *)subclass,
                                      COLLECTION_FLAGS,
                                      collection_flag);
        }
    }
    return Py_NewRef(subclass);
}


// True if instance.__class__ is type(instance) and the lookup
// doesn't run any Python code.
static inline int
has_default_class_attr(_abcmodule_state *state, PyObject *instance)
{
    PyTypeObject *tp = Py_TYPE(instance);
    return tp->tp_getattro == PyObject_GenericGetAttr
        && type_lookup_is(tp, &_Py_ID(__class__), state->object_class_descr);
}

/*[clinic input]
@permit_long_summary
_abc._abc_instancecheck

    self: object
    instance: object
    /

Internal ABC helper for instance checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_instancecheck_impl(PyObject *module, PyObject *self,
                             PyObject *instance)
/*[clinic end generated code: output=b8b5148f63b6b56f input=0bbc8da0ea346719]*/
{
    _abcmodule_state *state = get_abc_state(module);
    PyObject *subtype = (PyObject *)Py_TYPE(instance);
    // subclass is either subtype, which is kept alive by instance, or
    // subclass_owned. The fast path doesn't incref subtype to avoid
    // refcount contention between threads.
    PyObject *subclass = subtype;
    PyObject *subclass_owned = NULL;
    PyObject *result = NULL;

    if (!has_default_class_attr(state, instance)) {
        if (PyObject_GetOptionalAttr(instance, &_Py_ID(__class__),
                                     &subclass_owned) < 0) {
            return NULL;
        }
        /* Fall back to the type when the instance has no __class__,
           matching the behaviour of the built-in isinstance()
           (gh-153772). */
        if (subclass_owned != NULL) {
            subclass = subclass_owned;
        }
    }

    // Fast path: cache hits don't need any locking.
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(state, self, &cref);
    cache_result incache = CACHE_UNKNOWN;
    if (impl != NULL) {
        incache = lookup_caches(state, impl, subclass);
    }
    _PyThreadState_PopCStackRef(tstate, &cref);
    if (impl == NULL) {
        Py_XDECREF(subclass_owned);
        return NULL;
    }
    if (incache == CACHE_POSITIVE
        || (incache == CACHE_NEGATIVE && subtype == subclass))
    {
        Py_XDECREF(subclass_owned);
        return PyBool_FromLong(incache);
    }

    /* A subclass check can change the instance's actual type. */
    Py_INCREF(subtype);
    result = PyObject_CallMethodOneArg(self, &_Py_ID(__subclasscheck__),
                                       subclass);
    if (result == NULL || subtype == subclass) {
        Py_DECREF(subtype);
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
    Py_DECREF(subtype);

end:
    Py_XDECREF(subclass_owned);
    return result;
}


static int subclasscheck(_abcmodule_state *state, _abc_data *impl,
                         PyObject *self, PyObject *subclass, int nested);

// Like PyObject_IsSubclass(subclass, cls), but doesn't add subclass to the
// negative cache of cls if cls uses the default ABCMeta.__subclasscheck__.
static int
issubclass_nested(_abcmodule_state *state, PyObject *subclass, PyObject *cls)
{
    assert(PyType_Check(cls));
    if (type_lookup_is(Py_TYPE(cls), &_Py_ID(__subclasscheck__),
                       state->subclasscheck_func))
    {
        PyThreadState *tstate = _PyThreadState_GET();
        _PyCStackRef cref;
        _PyThreadState_PushCStackRef(tstate, &cref);
        _abc_data *impl = lookup_impl(state, cls, &cref);
        int res = -1;
        if (impl != NULL) {
            if (!Py_EnterRecursiveCall(" in __subclasscheck__")) {
                res = subclasscheck(state, impl, cls, subclass, 1);
                Py_LeaveRecursiveCall();
            }
        }
        _PyThreadState_PopCStackRef(tstate, &cref);
        if (impl != NULL) {
            return res;
        }
    }
    return PyObject_IsSubclass(subclass, cls);
}

// Check if subclass is a subclass of a registered class.
// Returns -1 on error.
static int
check_registry(_abcmodule_state *state, _abc_data *impl, PyObject *subclass)
{
    abc_table *registry = FT_ATOMIC_LOAD_PTR_ACQUIRE(impl->registry);
    // Fast path: subclass is registered directly.
    if (abc_table_contains(registry, subclass)) {
        return 1;
    }
    // The registry can be modified by the checks, iterate over a copy.
    PyObject *classes;
    Py_BEGIN_CRITICAL_SECTION(impl);
    classes = abc_table_snapshot(impl->registry);
    Py_END_CRITICAL_SECTION();
    if (classes == NULL) {
        return -1;
    }
    int res = 0;
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(classes); i++) {
        res = PyObject_IsSubclass(subclass, PyList_GET_ITEM(classes, i));
        if (res != 0) {
            break;
        }
    }
    Py_DECREF(classes);
    return res;
}

// Check if subclass is a subclass of a subclass of self.
// Returns -1 on error.
static int
check_subclasses(_abcmodule_state *state, PyObject *self, PyObject *subclass)
{
    PyObject *subclasses;
    int native = 0;
    if (PyType_Check(self)
        && type_lookup_is((PyTypeObject *)self, &_Py_ID(__subclasses__), NULL)
        && type_lookup_is(Py_TYPE(self), &_Py_ID(__subclasses__),
                          state->type_subclasses))
    {
        // __subclasses__() is not overridden, get the subclasses directly.
        PyTypeObject *type = (PyTypeObject *)self;
        if (!(type->tp_flags & _Py_TPFLAGS_STATIC_BUILTIN)
            && FT_ATOMIC_LOAD_PTR_RELAXED(type->tp_subclasses) == NULL)
        {
            return 0;
        }
        subclasses = _PyType_GetSubclasses(type);
        native = 1;
    }
    else {
        subclasses = PyObject_CallMethodNoArgs(self, &_Py_ID(__subclasses__));
        if (subclasses != NULL && !PyList_Check(subclasses)) {
            PyErr_SetString(PyExc_TypeError, "__subclasses__() must return a list");
            Py_DECREF(subclasses);
            return -1;
        }
    }
    if (subclasses == NULL) {
        return -1;
    }
    int res = 0;
    for (Py_ssize_t i = 0; i < PyList_GET_SIZE(subclasses); i++) {
        PyObject *scls = PyList_GetItemRef(subclasses, i);
        if (scls == NULL) {
            res = -1;
            break;
        }
        if (native) {
            res = issubclass_nested(state, subclass, scls);
        }
        else {
            res = PyObject_IsSubclass(subclass, scls);
        }
        Py_DECREF(scls);
        if (res != 0) {
            break;
        }
    }
    Py_DECREF(subclasses);
    return res;
}

// Implementation of ABCMeta.__subclasscheck__. Returns -1 on error.
//
// nested is set when checking the subclasses of another ABC. Negative
// results are not cached then, otherwise every class in the hierarchy
// would cache them which is quadratic in the number of classes.
static int
subclasscheck(_abcmodule_state *state, _abc_data *impl,
              PyObject *self, PyObject *subclass, int nested)
{
    uint64_t counter = get_invalidation_counter(state);

    /* 1. Check cache. 2. Check negative cache; may have to invalidate. */
    if (!validate_negative_cache(impl, counter)) {
        cache_result incache = lookup_caches(state, impl, subclass);
        if (incache != CACHE_UNKNOWN) {
            return incache;
        }
    }
    else {
        abc_table *cache = FT_ATOMIC_LOAD_PTR_ACQUIRE(impl->cache);
        if (abc_table_contains(cache, subclass)) {
            return 1;
        }
    }

    /* 3. Check the subclass hook. */
    // The default hook always returns NotImplemented, skip the call.
    if (!(PyType_Check(self)
          && type_lookup_is((PyTypeObject *)self, &_Py_ID(__subclasshook__),
                            state->object_subclasshook)))
    {
        PyObject *ok = PyObject_CallMethodOneArg(
                self, &_Py_ID(__subclasshook__), subclass);
        if (ok == NULL) {
            return -1;
        }
        if (ok == Py_True) {
            goto positive;
        }
        if (ok == Py_False) {
            goto negative;
        }
        if (ok != Py_NotImplemented) {
            Py_DECREF(ok);
            PyErr_SetString(PyExc_AssertionError, "__subclasshook__ must return either"
                                                  " False, True, or NotImplemented");
            return -1;
        }
    }

    /* 4. Check if it's a direct subclass. */
    if (PyType_IsSubtype((PyTypeObject *)subclass, (PyTypeObject *)self)) {
        goto positive;
    }

    /* 5. Check if it's a subclass of a registered class (recursive). */
    int res = check_registry(state, impl, subclass);
    if (res < 0) {
        return -1;
    }
    if (res > 0) {
        goto positive;
    }

    /* 6. Check if it's a subclass of a subclass (recursive). */
    res = check_subclasses(state, self, subclass);
    if (res < 0) {
        return -1;
    }
    if (res > 0) {
        goto positive;
    }

negative:
    /* No dice; update negative cache. */
    // The result can be stale if register() was called in the meantime.
    if (!nested && get_invalidation_counter(state) == counter) {
        if (add_to_table(impl, &impl->negative_cache, subclass) < 0) {
            return -1;
        }
    }
    return 0;

positive:
    if (add_to_table(impl, &impl->cache, subclass) < 0) {
        return -1;
    }
    return 1;
}

/*[clinic input]
@permit_long_summary
_abc._abc_subclasscheck

    self: object
    subclass: object
    /

Internal ABC helper for subclasss checks. Should be never used outside abc module.
[clinic start generated code]*/

static PyObject *
_abc__abc_subclasscheck_impl(PyObject *module, PyObject *self,
                             PyObject *subclass)
/*[clinic end generated code: output=b56c9e4a530e3894 input=5bf1ef712f5d3610]*/
{
    if (!PyType_Check(subclass)) {
        PyErr_SetString(PyExc_TypeError, "issubclass() arg 1 must be a class");
        return NULL;
    }

    _abcmodule_state *state = get_abc_state(module);
    PyThreadState *tstate = _PyThreadState_GET();
    _PyCStackRef cref;
    _PyThreadState_PushCStackRef(tstate, &cref);
    _abc_data *impl = get_impl(state, self, &cref);
    int res = -1;
    if (impl != NULL) {
        res = lookup_caches(state, impl, subclass);
        if (res == CACHE_UNKNOWN) {
            res = subclasscheck(state, impl, self, subclass, 0);
        }
    }
    _PyThreadState_PopCStackRef(tstate, &cref);
    if (res < 0) {
        return NULL;
    }
    return PyBool_FromLong(res);
}

/*[clinic input]
_abc._set_subclasscheck

    func: object
    /

Internal ABC helper to register ABCMeta.__subclasscheck__.

Classes whose metaclass resolves __subclasscheck__ to func are checked
natively when walking the subclass tree of an ABC.
[clinic start generated code]*/

static PyObject *
_abc__set_subclasscheck(PyObject *module, PyObject *func)
/*[clinic end generated code: output=1dc657738311dea5 input=c2e47dd519524e44]*/
{
    _abcmodule_state *state = get_abc_state(module);
    assert(state->subclasscheck_func == NULL);
    state->subclasscheck_func = Py_NewRef(func);
    Py_RETURN_NONE;
}

/*[clinic input]
_abc.get_cache_token

Returns the current ABC cache token.

The token is an opaque object (supporting equality testing) identifying
the current version of the ABC cache for virtual subclasses.  The token
changes with every call to register() on any ABC.
[clinic start generated code]*/

static PyObject *
_abc_get_cache_token_impl(PyObject *module)
/*[clinic end generated code: output=c7d87841e033dacc input=d87acc04492f6bf3]*/
{
    _abcmodule_state *state = get_abc_state(module);
    return PyLong_FromUnsignedLongLong(get_invalidation_counter(state));
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
    _ABC__SET_SUBCLASSCHECK_METHODDEF
    {NULL,       NULL}          /* sentinel */
};

static int
get_type_dict_item(PyTypeObject *type, PyObject *name, PyObject **result)
{
    if (PyDict_GetItemRef(_PyType_GetDict(type), name, result) < 0) {
        return -1;
    }
    if (*result == NULL) {
        PyErr_Format(PyExc_SystemError, "%s has no %U", type->tp_name, name);
        return -1;
    }
    return 0;
}

static int
_abcmodule_exec(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    state->abc_invalidation_counter = 0;
    state->_abc_data_type = (PyTypeObject *)PyType_FromModuleAndSpec(module, &_abc_data_type_spec, NULL);
    if (state->_abc_data_type == NULL) {
        return -1;
    }
    if (get_type_dict_item(&PyBaseObject_Type, &_Py_ID(__class__),
                           &state->object_class_descr) < 0
        || get_type_dict_item(&PyBaseObject_Type, &_Py_ID(__subclasshook__),
                              &state->object_subclasshook) < 0
        || get_type_dict_item(&PyType_Type, &_Py_ID(__subclasses__),
                              &state->type_subclasses) < 0)
    {
        return -1;
    }
    return 0;
}

static int
_abcmodule_traverse(PyObject *module, visitproc visit, void *arg)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_VISIT(state->_abc_data_type);
    Py_VISIT(state->object_class_descr);
    Py_VISIT(state->object_subclasshook);
    Py_VISIT(state->type_subclasses);
    Py_VISIT(state->subclasscheck_func);
    return 0;
}

static int
_abcmodule_clear(PyObject *module)
{
    _abcmodule_state *state = get_abc_state(module);
    Py_CLEAR(state->_abc_data_type);
    Py_CLEAR(state->object_class_descr);
    Py_CLEAR(state->object_subclasshook);
    Py_CLEAR(state->type_subclasses);
    Py_CLEAR(state->subclasscheck_func);
    return 0;
}

static void
_abcmodule_free(void *module)
{
    (void)_abcmodule_clear((PyObject *)module);
}

static PyModuleDef_Slot _abcmodule_slots[] = {
    _Py_ABI_SLOT,
    {Py_mod_exec, _abcmodule_exec},
    {Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
    {Py_mod_gil, Py_MOD_GIL_NOT_USED},
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
