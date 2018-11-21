#ifndef Py_INTERNAL_OBJECT_H
#define Py_INTERNAL_OBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(Py_BUILD_CORE) && !defined(Py_BUILD_CORE_BUILTIN)
#  error "this header requires Py_BUILD_CORE or Py_BUILD_CORE_BUILTIN defined"
#endif

/* Tell the GC to track this object.
 *
 * NB: While the object is tracked by the collector, it must be safe to call the
 * ob_traverse method.
 *
 * Internal note: _PyRuntime.gc.generation0->_gc_prev doesn't have any bit flags
 * because it's not object header.  So we don't use _PyGCHead_PREV() and
 * _PyGCHead_SET_PREV() for it to avoid unnecessary bitwise operations.
 *
 * The PyObject_GC_Track() function is the public version of this macro.
 */
#define _PyObject_GC_TRACK(o) do { \
        PyGC_Head *g = _Py_AS_GC(o); \
        if (g->_gc_next != 0) { \
            Py_FatalError("GC object already tracked"); \
        } \
        assert((g->_gc_prev & _PyGC_PREV_MASK_COLLECTING) == 0); \
        PyGC_Head *last = (PyGC_Head*)(_PyRuntime.gc.generation0->_gc_prev); \
        _PyGCHead_SET_NEXT(last, g); \
        _PyGCHead_SET_PREV(g, last); \
        _PyGCHead_SET_NEXT(g, _PyRuntime.gc.generation0); \
        _PyRuntime.gc.generation0->_gc_prev = (uintptr_t)g; \
    } while (0);

/* Tell the GC to stop tracking this object.
 *
 * Internal note: This may be called while GC.  So _PyGC_PREV_MASK_COLLECTING must
 * be cleared.  But _PyGC_PREV_MASK_FINALIZED bit is kept.
 *
 * The PyObject_GC_UnTrack() function is the public version of this macro.
 */
#define _PyObject_GC_UNTRACK(o) do { \
        PyGC_Head *g = _Py_AS_GC(o); \
        PyGC_Head *prev = _PyGCHead_PREV(g); \
        PyGC_Head *next = _PyGCHead_NEXT(g); \
        assert(next != NULL); \
        _PyGCHead_SET_NEXT(prev, next); \
        _PyGCHead_SET_PREV(next, prev); \
        g->_gc_next = 0; \
        g->_gc_prev &= _PyGC_PREV_MASK_FINALIZED; \
    } while (0);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_OBJECT_H */
