#ifndef Py_INTERNAL_OBJECT_ALLOC_H
#define Py_INTERNAL_OBJECT_ALLOC_H

#include "pycore_object.h"      // _PyType_HasFeature()
#include "pycore_pystate.h"     // _PyThreadState_GET()
#include "pycore_tstate.h"      // _PyThreadStateImpl

#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Sets the heap used for PyObject_Malloc(), PyObject_Realloc(), etc. calls in
// Py_GIL_DISABLED builds. We use different heaps depending on if the object
// supports GC and if it has a pre-header. We smuggle the choice of heap
// through the _mimalloc_thread_state. In the default build, this simply
// calls PyObject_Malloc().
static inline void *
_PyObject_MallocWithType(PyTypeObject *tp, size_t size)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct _mimalloc_thread_state *m = &tstate->mimalloc;
    if (_PyType_HasFeature(tp, Py_TPFLAGS_PREHEADER)) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_GC_PRE];
    }
    else if (_PyType_IS_GC(tp)) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_GC];
    }
#endif
    void *mem = PyObject_Malloc(size);
#ifdef Py_GIL_DISABLED
    m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
#endif
    return mem;
}

static inline void *
_PyObject_ReallocWithType(PyTypeObject *tp, void *ptr, size_t size)
{
#ifdef Py_GIL_DISABLED
    _PyThreadStateImpl *tstate = (_PyThreadStateImpl *)_PyThreadState_GET();
    struct _mimalloc_thread_state *m = &tstate->mimalloc;
    if (_PyType_HasFeature(tp, Py_TPFLAGS_PREHEADER)) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_GC_PRE];
    }
    else if (_PyType_IS_GC(tp)) {
        m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_GC];
    }
#endif
    void *mem = PyObject_Realloc(ptr, size);
#ifdef Py_GIL_DISABLED
    m->current_object_heap = &m->heaps[_Py_MIMALLOC_HEAP_OBJECT];
#endif
    return mem;
}

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_OBJECT_ALLOC_H
