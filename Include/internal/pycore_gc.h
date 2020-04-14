#ifndef Py_INTERNAL_GC_H
#define Py_INTERNAL_GC_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

/* GC information is stored BEFORE the object structure. */
typedef struct {
    // Pointer to next object in the list.
    // 0 means the object is not tracked
    uintptr_t _gc_next;

    // Pointer to previous object in the list.
    // Lowest two bits are used for flags documented later.
    uintptr_t _gc_prev;
} PyGC_Head;

#define _Py_AS_GC(o) ((PyGC_Head *)(o)-1)

/* True if the object is currently tracked by the GC. */
#define _PyObject_GC_IS_TRACKED(o) (_Py_AS_GC(o)->_gc_next != 0)

/* True if the object may be tracked by the GC in the future, or already is.
   This can be useful to implement some optimizations. */
#define _PyObject_GC_MAY_BE_TRACKED(obj) \
    (PyObject_IS_GC(obj) && \
        (!PyTuple_CheckExact(obj) || _PyObject_GC_IS_TRACKED(obj)))


/* Bit flags for _gc_prev */
/* Bit 0 is set when tp_finalize is called */
#define _PyGC_PREV_MASK_FINALIZED  (1)
/* Bit 1 is set when the object is in generation which is GCed currently. */
#define _PyGC_PREV_MASK_COLLECTING (2)
/* The (N-2) most significant bits contain the real address. */
#define _PyGC_PREV_SHIFT           (2)
#define _PyGC_PREV_MASK            (((uintptr_t) -1) << _PyGC_PREV_SHIFT)

// Lowest bit of _gc_next is used for flags only in GC.
// But it is always 0 for normal code.
#define _PyGCHead_NEXT(g)        ((PyGC_Head*)(g)->_gc_next)
#define _PyGCHead_SET_NEXT(g, p) ((g)->_gc_next = (uintptr_t)(p))

// Lowest two bits of _gc_prev is used for _PyGC_PREV_MASK_* flags.
#define _PyGCHead_PREV(g) ((PyGC_Head*)((g)->_gc_prev & _PyGC_PREV_MASK))
#define _PyGCHead_SET_PREV(g, p) do { \
    assert(((uintptr_t)p & ~_PyGC_PREV_MASK) == 0); \
    (g)->_gc_prev = ((g)->_gc_prev & ~_PyGC_PREV_MASK) \
        | ((uintptr_t)(p)); \
    } while (0)

#define _PyGCHead_FINALIZED(g) \
    (((g)->_gc_prev & _PyGC_PREV_MASK_FINALIZED) != 0)
#define _PyGCHead_SET_FINALIZED(g) \
    ((g)->_gc_prev |= _PyGC_PREV_MASK_FINALIZED)

#define _PyGC_FINALIZED(o) \
    _PyGCHead_FINALIZED(_Py_AS_GC(o))
#define _PyGC_SET_FINALIZED(o) \
    _PyGCHead_SET_FINALIZED(_Py_AS_GC(o))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GC_H */
