#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_frame.h"         // _PyFrame_New_NoTrack()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_stackref.h"      // _Py_VISIT_STACKREF()


int
_PyFrame_Traverse(_PyControlRecord *frame, _PyInterpreterDataFrame *fp,
                  visitproc visit, void *arg)
{
    Py_VISIT(fp->frame_obj);
    Py_VISIT(fp->f_locals);
    _Py_VISIT_STACKREF(fp->f_funcobj);
    _Py_VISIT_STACKREF(frame->f_executable);
    return _PyGC_VisitFrameStack(frame, fp, visit, arg);
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{
    PyObject *exc = PyErr_GetRaisedException();

    PyFrameObject *f = _PyFrame_New_NoTrack(_PyFrame_GetCode(frame));
    if (f == NULL) {
        Py_XDECREF(exc);
        return NULL;
    }
    PyErr_SetRaisedException(exc);

    // GH-97002: There was a time when a frame object could be created when we
    // are allocating the new frame object f above, so fp->frame_obj would
    // be assigned already. That path does not exist anymore. We won't call any
    // Python code in this function and garbage collection will not run.
    // Notice that _PyFrame_New_NoTrack() can potentially raise a MemoryError,
    // but it won't allocate a traceback until the frame unwinds, so we are safe
    // here.
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    f->f_frame = frame;
#ifdef Py_GIL_DISABLED
    PyFrameObject *expected = NULL;
    if (!_Py_atomic_compare_exchange_ptr(&fp->frame_obj, &expected, f)) {
        Py_DECREF(f);
        return expected;
    }
#else
    assert(fp->frame_obj == NULL);
    fp->frame_obj = f;
#endif
    return f;
}

void
_PyFrame_TakeOwnership(PyFrameObject *f, _PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(frame->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyControlRecord *new_frame = (_PyControlRecord *)f->_f_frame_data;
    _PyFrame_Copy(frame, fp, new_frame, new_frame->data);
    // _PyFrame_Copy takes the reference to the executable,
    // so we need to restore it.
    new_frame->f_executable = PyStackRef_DUP(new_frame->f_executable);
    f->f_frame = new_frame;
    new_frame->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    if (_PyFrame_IsIncomplete(new_frame)) {
        // This may be a newly-created generator or coroutine frame. Since it's
        // dead anyways, just pretend that the first RESUME ran:
        PyCodeObject *code = _PyFrame_GetCode(new_frame);
        new_frame->instr_ptr =
            _PyFrame_GetBytecode(new_frame) + code->_co_firsttraceable + 1;
    }
    assert(!_PyFrame_IsIncomplete(new_frame));
    assert(f->f_back == NULL);
    _PyControlRecord *prev = _PyFrame_GetFirstComplete(_PyFrame_GetPrevious(frame));
    if (prev) {
        assert(prev->owner < FRAME_OWNED_BY_INTERPRETER);
        PyObject *exc = PyErr_GetRaisedException();
        PyFrameObject *back = _PyFrame_GetFrameObject(prev, prev->data);
        if (back == NULL) {
            /* Memory error here. */
            assert(PyErr_ExceptionMatches(PyExc_MemoryError));
            /* Nothing we can do about it */
            PyErr_Clear();
        }
        else {
            f->f_back = (PyFrameObject *)Py_NewRef(back);
        }
        PyErr_SetRaisedException(exc);
    }
    if (!_PyObject_GC_IS_TRACKED((PyObject *)f)) {
        _PyObject_GC_TRACK((PyObject *)f);
    }
    Py_END_CRITICAL_SECTION();
}

void
_PyFrame_ClearLocals(_PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{
    assert(frame->stackpointer != NULL);
    _PyStackRef *sp = frame->stackpointer;
    _PyStackRef *locals = fp->localsplus;
    frame->stackpointer = locals;
    while (sp > locals) {
        sp--;
        PyStackRef_XCLOSE(*sp);
    }
    Py_CLEAR(fp->f_locals);
}

void
_PyFrame_ClearExceptCode(_PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the enclosing generator, if any. */
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
           FT_ATOMIC_LOAD_INT8_RELAXED(_PyGen_GetGeneratorFromFrame(fp)->gi_frame_state) == FRAME_CLEARED);
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->current_frame != frame);
    PyFrameObject *f = FT_ATOMIC_LOAD_PTR_RELAXED(fp->frame_obj);
    if (f != NULL) {
        FT_ATOMIC_STORE_PTR_RELAXED(fp->frame_obj, NULL);
        if (!_PyObject_IsUniquelyReferenced((PyObject *)f)) {
            _PyFrame_TakeOwnership(f, frame, fp);
            Py_DECREF(f);
            return;
        }
        Py_DECREF(f);
    }
    _PyFrame_ClearLocals(frame, fp);
    PyStackRef_CLEAR(fp->f_funcobj);
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyControlRecord *frame)
{
    return PyStackRef_AsPyObjectNew(frame->f_executable);
}

// NOTE: We allow racy accesses to the instruction pointer from other threads
// for sys._current_frames() and similar APIs.
int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLasti(struct _PyControlRecord *frame)
{
    return _PyControlRecord_LASTI(frame) * sizeof(_Py_CODEUNIT);
}

int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLine(_PyControlRecord *frame)
{
    int addr = _PyControlRecord_LASTI(frame) * sizeof(_Py_CODEUNIT);
    return PyCode_Addr2Line(_PyFrame_GetCode(frame), addr);
}
