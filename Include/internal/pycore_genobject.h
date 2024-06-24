#ifndef Py_INTERNAL_GENOBJECT_H
#define Py_INTERNAL_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_frame.h"

struct _PyGenObject {
    /* The gi_ prefix is for generator-iterator. */
    PyObject_HEAD                                               \
    /* List of weak reference. */                               \
    PyObject *gi_weakreflist;                                   \
    /* Name of the generator. */                                \
    PyObject *gi_name;                                          \
    /* Qualified name of the generator. */                      \
    PyObject *gi_qualname;                                      \
    _PyErr_StackItem gi_exc_state;                              \
    PyObject *gi_cr_origin_or_ag_finalizer;                     \
    char gi_hooks_inited;                                       \
    char gi_closed;                                             \
    char gi_running_async;                                      \
    /* The frame */                                             \
    int8_t gi_frame_state;                                      \
    struct _PyInterpreterFrame gi_iframe;                       \
};

static inline
PyGenObject *_PyGen_GetGeneratorFromFrame(_PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    size_t offset_in_gen = offsetof(PyGenObject, gi_iframe);
    return (PyGenObject *)(((char *)frame) - offset_in_gen);
}

PyAPI_FUNC(PyObject *)_PyGen_yf(PyGenObject *);
extern void _PyGen_Finalize(PyObject *self);

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyGen_SetStopIterationValue(PyObject *);

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyGen_FetchStopIterationValue(PyObject **);

PyAPI_FUNC(PyObject *)_PyCoro_GetAwaitableIter(PyObject *o);
extern PyObject *_PyAsyncGenValueWrapperNew(PyThreadState *state, PyObject *);

extern PyTypeObject _PyCoroWrapper_Type;
extern PyTypeObject _PyAsyncGenWrappedValue_Type;
extern PyTypeObject _PyAsyncGenAThrow_Type;

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GENOBJECT_H */
