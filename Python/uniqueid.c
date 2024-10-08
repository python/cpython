#include "Python.h"

#include "pycore_lock.h"        // PyMutex_LockFlags()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include "pycore_object.h"      // _Py_IncRefTotal
#include "pycore_uniqueid.h"

// This contains code for allocating unique ids for per-thread reference
// counting and re-using those ids when an object is deallocated.
//
// Currently, per-thread reference counting is only used for heap types.
//
// See Include/internal/pycore_uniqueid.h for more details.

#ifdef Py_GIL_DISABLED

#define POOL_MIN_SIZE 8

#define LOCK_POOL(pool) PyMutex_LockFlags(&pool->mutex, _Py_LOCK_DONT_DETACH)
#define UNLOCK_POOL(pool) PyMutex_Unlock(&pool->mutex)

static int
resize_interp_type_id_pool(struct _Py_unique_id_pool *pool)
{
    if ((size_t)pool->size > PY_SSIZE_T_MAX / (2 * sizeof(*pool->table))) {
        return -1;
    }

    Py_ssize_t new_size = pool->size * 2;
    if (new_size < POOL_MIN_SIZE) {
        new_size = POOL_MIN_SIZE;
    }

    _Py_unique_id_entry *table = PyMem_Realloc(pool->table,
                                               new_size * sizeof(*pool->table));
    if (table == NULL) {
        return -1;
    }

    Py_ssize_t start = pool->size;
    for (Py_ssize_t i = start; i < new_size - 1; i++) {
        table[i].next = &table[i + 1];
    }
    table[new_size - 1].next = NULL;

    pool->table = table;
    pool->freelist = &table[start];
    _Py_atomic_store_ssize(&pool->size, new_size);
    return 0;
}

static int
resize_local_refcounts(_PyThreadStateImpl *tstate)
{
    if (tstate->refcounts.is_finalized) {
        return -1;
    }

    struct _Py_unique_id_pool *pool = &tstate->base.interp->unique_ids;
    Py_ssize_t size = _Py_atomic_load_ssize(&pool->size);

    Py_ssize_t *refcnts = PyMem_Realloc(tstate->refcounts.values,
                                        size * sizeof(Py_ssize_t));
    if (refcnts == NULL) {
        return -1;
    }

    Py_ssize_t old_size = tstate->refcounts.size;
    if (old_size < size) {
       memset(refcnts + old_size, 0, (size - old_size) * sizeof(Py_ssize_t));
    }

    tstate->refcounts.values = refcnts;
    tstate->refcounts.size = size;
    return 0;
}

Py_ssize_t
_PyObject_AssignUniqueId(PyObject *obj)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_unique_id_pool *pool = &interp->unique_ids;

    LOCK_POOL(pool);
    if (pool->freelist == NULL) {
        if (resize_interp_type_id_pool(pool) < 0) {
            UNLOCK_POOL(pool);
            return -1;
        }
    }

    _Py_unique_id_entry *entry = pool->freelist;
    pool->freelist = entry->next;
    entry->obj = obj;
    _PyObject_SetDeferredRefcount(obj);
    Py_ssize_t unique_id = (entry - pool->table);
    UNLOCK_POOL(pool);
    return unique_id;
}

void
_PyObject_ReleaseUniqueId(Py_ssize_t unique_id)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_unique_id_pool *pool = &interp->unique_ids;

    if (unique_id < 0) {
        // The id is not assigned
        return;
    }

    LOCK_POOL(pool);
    _Py_unique_id_entry *entry = &pool->table[unique_id];
    entry->next = pool->freelist;
    pool->freelist = entry;
    UNLOCK_POOL(pool);
}

void
_PyType_IncrefSlow(PyHeapTypeObject *type)
{
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    if (type->unique_id < 0 || resize_local_refcounts(tstate) < 0) {
        // just incref the type directly.
        Py_INCREF(type);
        return;
    }

    assert(type->unique_id < tstate->refcounts.size);
    tstate->refcounts.values[type->unique_id]++;
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal((PyThreadState *)tstate);
#endif
    _Py_INCREF_STAT_INC();
}

void
_PyObject_MergePerThreadRefcounts(_PyThreadStateImpl *tstate)
{
    if (tstate->refcounts.values == NULL) {
        return;
    }

    struct _Py_unique_id_pool *pool = &tstate->base.interp->unique_ids;

    LOCK_POOL(pool);
    for (Py_ssize_t i = 0, n = tstate->refcounts.size; i < n; i++) {
        Py_ssize_t refcnt = tstate->refcounts.values[i];
        if (refcnt != 0) {
            PyObject *obj = pool->table[i].obj;
            _Py_atomic_add_ssize(&obj->ob_ref_shared,
                                 refcnt << _Py_REF_SHARED_SHIFT);
            tstate->refcounts.values[i] = 0;
        }
    }
    UNLOCK_POOL(pool);
}

void
_PyObject_FinalizePerThreadRefcounts(_PyThreadStateImpl *tstate)
{
    _PyObject_MergePerThreadRefcounts(tstate);

    PyMem_Free(tstate->refcounts.values);
    tstate->refcounts.values = NULL;
    tstate->refcounts.size = 0;
    tstate->refcounts.is_finalized = 1;
}

void
_PyObject_FinalizeUniqueIdPool(PyInterpreterState *interp)
{
    struct _Py_unique_id_pool *pool = &interp->unique_ids;

    // First, set the free-list to NULL values
    while (pool->freelist) {
        _Py_unique_id_entry *next = pool->freelist->next;
        pool->freelist->obj = NULL;
        pool->freelist = next;
    }

    // Now everything non-NULL is a type. Set the type's id to -1 in case it
    // outlives the interpreter.
    for (Py_ssize_t i = 0; i < pool->size; i++) {
        PyObject *obj = pool->table[i].obj;
        pool->table[i].obj = NULL;
        if (obj == NULL) {
            continue;
        }
        if (PyType_Check(obj)) {
            assert(PyType_HasFeature((PyTypeObject *)obj, Py_TPFLAGS_HEAPTYPE));
            ((PyHeapTypeObject *)obj)->unique_id = -1;
        }
        else {
            Py_UNREACHABLE();
        }
    }
    PyMem_Free(pool->table);
    pool->table = NULL;
    pool->freelist = NULL;
    pool->size = 0;
}

#endif   /* Py_GIL_DISABLED */
