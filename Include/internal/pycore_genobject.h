#ifndef Py_INTERNAL_GENOBJECT_H
#define Py_INTERNAL_GENOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_frame.h"         // FRAME_STATE_FINISHED()
#include "pycore_interpframe.h"

#include <stddef.h>               // offsetof()


static inline
PyGenObject *_PyGen_GetGeneratorFromFrame(_PyInterpreterDataFrame *frame)
{
    return (PyGenObject *)(((char *)frame) - offsetof(PyGenObject, gi_data));
}

// The generator finalizer is a no-op once its frame is cleared.
static inline int
_PyGen_FinalizerIsNoop(PyObject *op)
{
    return Py_TYPE(op)->tp_finalize == PyGen_Type.tp_finalize &&
        FRAME_STATE_FINISHED(((PyGenObject *)op)->gi_frame_state);
}
#ifdef Py_GIL_DISABLED
PyAPI_FUNC(void) _PyGen_LinkFrame(PyGenObject *gen, _PyControlRecord *frame, _PyStackRef *value);
PyAPI_FUNC(void) _PyGen_UnlinkFrame(PyGenObject *gen, _PyControlRecord *frame);
#else
static inline void
_PyGen_LinkFrame(PyGenObject *gen, _PyControlRecord *frame, _PyStackRef *value)
{
    const _PyControlRecord *saved = &gen->gi_iframe;
#ifdef Py_DEBUG
    unsigned int stackpointer_valid = saved->stackpointer_valid;
    unsigned int lltrace = saved->lltrace;
#endif
    frame->f_executable = saved->f_executable;
    frame->instr_ptr = saved->instr_ptr;
    frame->stackpointer = saved->stackpointer;
    frame->data = &gen->gi_data;
    if (value != NULL) {
        *frame->stackpointer++ = *value;
    }
    frame->state = CONTROL_RESERVED;
    frame->return_offset = 0;
    frame->owner = FRAME_OWNED_BY_GENERATOR;
#ifdef Py_DEBUG
    frame->stackpointer_valid = stackpointer_valid;
    frame->lltrace = lltrace;
#endif
    PyFrameObject *f = gen->gi_data.frame_obj;
    if (f != NULL) {
        f->f_frame = frame;
    }
    gen->gi_live_frame = frame;
}

static inline void
_PyGen_UnlinkFrame(PyGenObject *gen, _PyControlRecord *frame)
{
    gen->gi_iframe.instr_ptr = frame->instr_ptr;
    gen->gi_iframe.stackpointer = frame->stackpointer;
#ifdef Py_DEBUG
    gen->gi_iframe.stackpointer_valid = frame->stackpointer_valid;
    gen->gi_iframe.lltrace = frame->lltrace;
#endif
    PyFrameObject *f = gen->gi_data.frame_obj;
    if (f != NULL) {
        f->f_frame = &gen->gi_iframe;
    }
    gen->gi_live_frame = &gen->gi_iframe;
}
#endif

static inline _PyControlRecord *
_PyGen_PushFrameUnchecked(PyThreadState *tstate, _PyControlRecord *caller,
                         PyGenObject *gen, _PyStackRef value)
{
    assert(gen->gi_live_frame == &gen->gi_iframe);
    _PyControlRecord *frame = caller + 1;
    assert(frame == tstate->controlstack_top);
    assert(frame < tstate->controlstack_limit);
    tstate->controlstack_top = frame + 1;
    frame->state = CONTROL_RESERVED;
    _PyGen_LinkFrame(gen, frame, &value);
    return frame;
}

static inline _PyControlRecord *
_PyGen_PushFrame(PyThreadState *tstate, PyGenObject *gen)
{
    assert(gen->gi_live_frame == &gen->gi_iframe);
    _PyControlRecord *frame = _PyThreadState_PushControl(tstate);
    if (frame != NULL) {
        _PyGen_LinkFrame(gen, frame, NULL);
    }
    return frame;
}

static inline void
_PyGen_PopFrame(PyThreadState *tstate, PyGenObject *gen, _PyControlRecord *frame)
{
    assert(gen->gi_live_frame == frame);
#ifdef Py_GIL_DISABLED
    frame->state = CONTROL_RESERVED;
#endif
    _PyGen_UnlinkFrame(gen, frame);
    _PyThreadState_PopControl(tstate, frame);
}

#ifndef Py_GIL_DISABLED
static inline void
_PyGen_PopFrameFinished(PyThreadState *tstate, PyGenObject *gen,
                        _PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{
    assert(gen->gi_live_frame == frame);
    assert(fp->frame_obj == NULL);
    assert(fp == &gen->gi_data);
    _PyStackRef *sp = frame->stackpointer;
    _PyStackRef *locals = fp->localsplus;
    gen->gi_iframe.stackpointer = locals;
#ifdef Py_DEBUG
    gen->gi_iframe.stackpointer_valid = 1;
#endif
    gen->gi_live_frame = &gen->gi_iframe;
    _PyThreadState_PopControl(tstate, frame);
    while (sp > locals) {
        sp--;
        PyStackRef_XCLOSE(*sp);
    }
    Py_CLEAR(fp->f_locals);
    PyStackRef_CLEAR(fp->f_funcobj);
}
#endif

PyAPI_FUNC(PyObject *)_PyGen_yf(PyGenObject *);
extern int _PyGen_ClearFrame(PyGenObject *self);

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyGen_SetStopIterationValue(PyObject *);

// Export for '_asyncio' shared extension
PyAPI_FUNC(int) _PyGen_FetchStopIterationValue(PyObject **);
// Set the exception passed to throw(typ[, val[, tb]]).
// Return 0 on success, -1 on failure.
extern int _PyGen_SetException(PyObject *typ, PyObject *val, PyObject *tb);

PyAPI_FUNC(PyObject *)_PyCoro_GetAwaitableIter(PyObject *o);
PyAPI_FUNC(PyObject *)_PyAsyncGenValueWrapperNew(PyThreadState *state, PyObject *);

// Exported for external JIT support
PyAPI_FUNC(PyObject *) _PyCoro_ComputeOrigin(int origin_depth, _PyControlRecord *current_frame);

extern PyTypeObject _PyCoroWrapper_Type;
extern PyTypeObject _PyAsyncGenWrappedValue_Type;
extern PyTypeObject _PyAsyncGenAThrow_Type;

PyAPI_FUNC(PySendResult) _PyAsyncGenASend_Send(PyObject *iter, PyObject *arg, PyObject **result);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_GENOBJECT_H */
