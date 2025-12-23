#define _PY_INTERPRETER

#include "Python.h"
#include "pycore_frame.h"         // _PyFrame_New_NoTrack()
#include "pycore_interpframe.h"   // _PyFrame_GetCode()
#include "pycore_genobject.h"     // _PyGen_GetGeneratorFromFrame()
#include "pycore_stackref.h"      // _Py_VISIT_STACKREF()


int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg)
{
    Py_VISIT(frame->frame_obj);
    Py_VISIT(frame->f_locals);
    _Py_VISIT_STACKREF(frame->f_funcobj);
    _Py_VISIT_STACKREF(frame->f_executable);
    return _PyGC_VisitFrameStack(frame, visit, arg);
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame)
{
    assert(frame->frame_obj == NULL);
    PyObject *exc = PyErr_GetRaisedException();

    PyFrameObject *f = _PyFrame_New_NoTrack(_PyFrame_GetCode(frame));
    if (f == NULL) {
        Py_XDECREF(exc);
        return NULL;
    }
    PyErr_SetRaisedException(exc);

    // GH-97002: There was a time when a frame object could be created when we
    // are allocating the new frame object f above, so frame->frame_obj would
    // be assigned already. That path does not exist anymore. We won't call any
    // Python code in this function and garbage collection will not run.
    // Notice that _PyFrame_New_NoTrack() can potentially raise a MemoryError,
    // but it won't allocate a traceback until the frame unwinds, so we are safe
    // here.
    assert(frame->frame_obj == NULL);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    f->f_frame = frame;
    frame->frame_obj = f;
    return f;
}

static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(frame->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(frame->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    _PyFrame_Copy(frame, new_frame);
    // _PyFrame_Copy takes the reference to the executable,
    // so we need to restore it.
    frame->f_executable = PyStackRef_DUP(new_frame->f_executable);
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
    _PyInterpreterFrame *prev = _PyFrame_GetFirstComplete(frame->previous);
    if (prev) {
        assert(prev->owner < FRAME_OWNED_BY_INTERPRETER);
        PyObject *exc = PyErr_GetRaisedException();
        /* Link PyFrameObjects.f_back and remove link through _PyInterpreterFrame.previous */
        PyFrameObject *back = _PyFrame_GetFrameObject(prev);
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
_PyFrame_ClearLocals(_PyInterpreterFrame *frame)
{
    assert(frame->stackpointer != NULL);
    _PyStackRef *sp = frame->stackpointer;
    _PyStackRef *locals = frame->localsplus;
    frame->stackpointer = locals;
    while (sp > locals) {
        sp--;
        PyStackRef_XCLOSE(*sp);
    }
    Py_CLEAR(frame->f_locals);
}

void
_PyFrame_ClearExceptCode(_PyInterpreterFrame *frame)
{
    /* It is the responsibility of the owning generator/coroutine
     * to have cleared the enclosing generator, if any. */
    assert(frame->owner != FRAME_OWNED_BY_GENERATOR ||
        _PyGen_GetGeneratorFromFrame(frame)->gi_frame_state == FRAME_CLEARED);
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->current_frame != frame);
    if (frame->frame_obj) {
        PyFrameObject *f = frame->frame_obj;
        frame->frame_obj = NULL;
        if (!_PyObject_IsUniquelyReferenced((PyObject *)f)) {
            take_ownership(f, frame);
            Py_DECREF(f);
            return;
        }
        Py_DECREF(f);
    }
    _PyFrame_ClearLocals(frame);
    PyStackRef_CLEAR(frame->f_funcobj);
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrame *frame)
{
    return PyStackRef_AsPyObjectNew(frame->f_executable);
}

int
PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrame *frame)
{
    return _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
}

// NOTE: We allow racy accesses to the instruction pointer from other threads
// for sys._current_frames() and similar APIs.
int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLine(_PyInterpreterFrame *frame)
{
    int addr = _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
    return PyCode_Addr2Line(_PyFrame_GetCode(frame), addr);
}

const PyTypeObject *const PyUnstable_ExecutableKinds[PyUnstable_EXECUTABLE_KINDS+1] = {
    [PyUnstable_EXECUTABLE_KIND_SKIP] = &_PyNone_Type,
    [PyUnstable_EXECUTABLE_KIND_PY_FUNCTION] = &PyCode_Type,
    [PyUnstable_EXECUTABLE_KIND_BUILTIN_FUNCTION] = &PyMethod_Type,
    [PyUnstable_EXECUTABLE_KIND_METHOD_DESCRIPTOR] = &PyMethodDescr_Type,
    [PyUnstable_EXECUTABLE_KINDS] = NULL,
};
