#include "Python.h"

#include "pycore_lock.h"        // PyMutex_LockFlags()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include "pycore_object.h"      // _Py_IncRefTotal
#include "pycore_typeid.h"

// This contains code for allocating unique ids to heap type objects
// and re-using those ids when the type is deallocated.
//
// See Include/internal/pycore_typeid.h for more details.

#ifdef Py_GIL_DISABLED

#define POOL_MIN_SIZE 8

#define LOCK_POOL(pool) PyMutex_LockFlags(&pool->mutex, _Py_LOCK_DONT_DETACH)
#define UNLOCK_POOL(pool) PyMutex_Unlock(&pool->mutex)

static int
resize_interp_type_id_pool(struct _Py_type_id_pool *pool)
{
    if ((size_t)pool->size > PY_SSIZE_T_MAX / (2 * sizeof(*pool->table))) {
        return -1;
    }

    Py_ssize_t new_size = pool->size * 2;
    if (new_size < POOL_MIN_SIZE) {
        new_size = POOL_MIN_SIZE;
    }

    _Py_type_id_entry *table = PyMem_Realloc(pool->table,
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
    if (tstate->types.is_finalized) {
        return -1;
    }

    struct _Py_type_id_pool *pool = &tstate->base.interp->type_ids;
    Py_ssize_t size = _Py_atomic_load_ssize(&pool->size);

    Py_ssize_t *refcnts = PyMem_Realloc(tstate->types.refcounts,
                                        size * sizeof(Py_ssize_t));
    if (refcnts == NULL) {
        return -1;
    }

    Py_ssize_t old_size = tstate->types.size;
    if (old_size < size) {
       memset(refcnts + old_size, 0, (size - old_size) * sizeof(Py_ssize_t));
    }

    tstate->types.refcounts = refcnts;
    tstate->types.size = size;
    return 0;
}

void
_PyType_AssignId(PyHeapTypeObject *type)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_type_id_pool *pool = &interp->type_ids;

    LOCK_POOL(pool);
    if (pool->freelist == NULL) {
        if (resize_interp_type_id_pool(pool) < 0) {
            type->unique_id = -1;
            UNLOCK_POOL(pool);
            return;
        }
    }

    _Py_type_id_entry *entry = pool->freelist;
    pool->freelist = entry->next;
    entry->type = type;
    _PyObject_SetDeferredRefcount((PyObject *)type);
    type->unique_id = (entry - pool->table);
    UNLOCK_POOL(pool);
}

void
_PyType_ReleaseId(PyHeapTypeObject *type)
{
    PyInterpreterState *interp = _PyInterpreterState_GET();
    struct _Py_type_id_pool *pool = &interp->type_ids;

    if (type->unique_id < 0) {
        // The type doesn't have an id assigned.
        return;
    }

    LOCK_POOL(pool);
    _Py_type_id_entry *entry = &pool->table[type->unique_id];
    assert(entry->type == type);
    entry->next = pool->freelist;
    pool->freelist = entry;

    type->unique_id = -1;
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

    assert(type->unique_id < tstate->types.size);
    tstate->types.refcounts[type->unique_id]++;
#ifdef Py_REF_DEBUG
    _Py_IncRefTotal((PyThreadState *)tstate);
#endif
    _Py_INCREF_STAT_INC();
}

void
_PyType_MergeThreadLocalRefcounts(_PyThreadStateImpl *tstate)
{
    if (tstate->types.refcounts == NULL) {
        return;
    }

    struct _Py_type_id_pool *pool = &tstate->base.interp->type_ids;

    LOCK_POOL(pool);
    for (Py_ssize_t i = 0, n = tstate->types.size; i < n; i++) {
        Py_ssize_t refcnt = tstate->types.refcounts[i];
        if (refcnt != 0) {
            PyObject *type = (PyObject *)pool->table[i].type;
            assert(PyType_Check(type));

            _Py_atomic_add_ssize(&type->ob_ref_shared,
                                 refcnt << _Py_REF_SHARED_SHIFT);
            tstate->types.refcounts[i] = 0;
        }
    }
    UNLOCK_POOL(pool);
}

void
_PyType_FinalizeThreadLocalRefcounts(_PyThreadStateImpl *tstate)
{
    _PyType_MergeThreadLocalRefcounts(tstate);

    PyMem_Free(tstate->types.refcounts);
    tstate->types.refcounts = NULL;
    tstate->types.size = 0;
    tstate->types.is_finalized = 1;
}

void
_PyType_FinalizeIdPool(PyInterpreterState *interp)
{
    struct _Py_type_id_pool *pool = &interp->type_ids;

    // First, set the free-list to NULL values
    while (pool->freelist) {
        _Py_type_id_entry *next = pool->freelist->next;
        pool->freelist->type = NULL;
        pool->freelist = next;
    }

    // Now everything non-NULL is a type. Set the type's id to -1 in case it
    // outlives the interpreter.
    for (Py_ssize_t i = 0; i < pool->size; i++) {
        PyHeapTypeObject *ht = pool->table[i].type;
        if (ht) {
            ht->unique_id = -1;
            pool->table[i].type = NULL;
        }
    }
    PyMem_Free(pool->table);
    pool->table = NULL;
    pool->freelist = NULL;
    pool->size = 0;
}

#endif   /* Py_GIL_DISABLED */
