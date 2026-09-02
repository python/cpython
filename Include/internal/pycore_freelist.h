#ifndef Py_INTERNAL_FREELIST_H
#define Py_INTERNAL_FREELIST_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_freelist_state.h"      // struct _Py_freelists
#include "pycore_interp_structs.h"      // PyInterpreterState
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_STORE_PTR_RELAXED()
#include "pycore_pystate.h"             // _PyThreadState_GET
#include "pycore_stats.h"               // OBJECT_STAT_INC

static inline struct _Py_freelists *
_Py_freelists_GET(void)
{
#ifdef Py_DEBUG
    _Py_AssertHoldsTstate();
#endif

#ifdef Py_GIL_DISABLED
    PyThreadState *tstate = _PyThreadState_GET();
    return &((_PyThreadStateImpl*)tstate)->freelists;
#else
    PyInterpreterState *interp = _PyInterpreterState_GET();
    return &interp->object_state.freelists;
#endif
}

// Pushes `op` to the freelist, calls `freefunc` if the freelist is full
#define _Py_FREELIST_FREE(NAME, op, freefunc) \
    _PyFreeList_Free(&_Py_freelists_GET()->NAME, _PyObject_CAST(op), \
                     Py_ ## NAME ## _MAXFREELIST, freefunc)
// Pushes `op` to the freelist, returns 1 if successful, 0 if the freelist is full
#define _Py_FREELIST_PUSH(NAME, op, limit) \
    _PyFreeList_Push(&_Py_freelists_GET()->NAME, _PyObject_CAST(op), limit)

// Pops a PyObject from the freelist, returns NULL if the freelist is empty.
#define _Py_FREELIST_POP(TYPE, NAME) \
    _Py_CAST(TYPE*, _PyFreeList_Pop(&_Py_freelists_GET()->NAME))

// Pops a non-PyObject data structure from the freelist, returns NULL if the
// freelist is empty.
#define _Py_FREELIST_POP_MEM(NAME) \
    _PyFreeList_PopMem(&_Py_freelists_GET()->NAME)

#define _Py_FREELIST_SIZE(NAME) (int)((_Py_freelists_GET()->NAME).size)

static inline int
_PyFreeList_Push(struct _Py_freelist *fl, void *obj, Py_ssize_t maxsize)
{
    if (fl->size < maxsize && fl->size >= 0) {
        FT_ATOMIC_STORE_PTR_RELAXED(*(void **)obj, fl->freelist);
        fl->freelist = obj;
        fl->size++;
        OBJECT_STAT_INC(to_freelist);
        return 1;
    }
    return 0;
}

static inline void
_PyFreeList_Free(struct _Py_freelist *fl, void *obj, Py_ssize_t maxsize,
                 freefunc dofree)
{
    if (!_PyFreeList_Push(fl, obj, maxsize)) {
        dofree(obj);
    }
}

static inline void *
_PyFreeList_PopNoStats(struct _Py_freelist *fl)
{
    void *obj = fl->freelist;
    if (obj != NULL) {
        assert(fl->size > 0);
        fl->freelist = *(void **)obj;
        fl->size--;
    }
    return obj;
}

static inline PyObject *
_PyFreeList_Pop(struct _Py_freelist *fl)
{
    PyObject *op = _PyFreeList_PopNoStats(fl);
    if (op != NULL) {
        OBJECT_STAT_INC(from_freelist);
        _Py_NewReference(op);
    }
    return op;
}

static inline void *
_PyFreeList_PopMem(struct _Py_freelist *fl)
{
    void *op = _PyFreeList_PopNoStats(fl);
    if (op != NULL) {
        OBJECT_STAT_INC(from_freelist);
    }
    return op;
}

extern void _PyObject_ClearFreeLists(struct _Py_freelists *freelists, int is_finalization);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FREELIST_H */
