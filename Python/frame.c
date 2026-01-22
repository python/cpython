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
    _Py_VISIT_STACKREF(_PyFrame_Core(frame)->f_executable);
    return _PyGC_VisitFrameStack(frame, visit, arg);
}

PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame)
{
    PyObject *exc = PyErr_GetRaisedException();
    assert(frame->frame_obj == NULL);

    PyFrameObject *f = _PyFrame_New_NoTrack(_PyFrame_GetCode(_PyFrame_Core(frame)));
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
    assert(_PyFrame_Core(frame)->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    f->f_frame = frame;
    frame->frame_obj = f;
    return f;
}

static void
take_ownership(PyFrameObject *f, _PyInterpreterFrame *frame)
{
    Py_BEGIN_CRITICAL_SECTION(f);
    assert(_PyFrame_Core(frame)->owner < FRAME_OWNED_BY_INTERPRETER);
    assert(_PyFrame_Core(frame)->owner != FRAME_OWNED_BY_FRAME_OBJECT);
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)f->_f_frame_data;
    _PyFrame_Copy(frame, new_frame);
    // _PyFrame_Copy takes the reference to the executable,
    // so we need to restore it.
    _PyFrame_Core(frame)->f_executable = PyStackRef_DUP(_PyFrame_Core(new_frame)->f_executable);
    f->f_frame = new_frame;
    _PyFrame_Core(new_frame)->owner = FRAME_OWNED_BY_FRAME_OBJECT;
    if (_PyFrame_IsIncomplete(_PyFrame_Core(new_frame))) {
        // This may be a newly-created generator or coroutine frame. Since it's
        // dead anyways, just pretend that the first RESUME ran:
        PyCodeObject *code = _PyFrame_GetCode(_PyFrame_Core(new_frame));
        new_frame->instr_ptr =
            _PyFrame_GetBytecode(_PyFrame_Core(new_frame)) + code->_co_firsttraceable + 1;
    }
    assert(!_PyFrame_IsIncomplete(_PyFrame_Core(new_frame)));
    assert(f->f_back == NULL);
    _PyInterpreterFrameCore *prev = _PyFrame_GetFirstComplete(_PyFrame_Core(frame)->previous);
    if (prev) {
        assert(prev->owner < FRAME_OWNED_BY_INTERPRETER);
        PyObject *exc = PyErr_GetRaisedException();
        /* Link PyFrameObjects.f_back and remove link through _PyInterpreterFrameCore.previous */
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
    assert(!(_PyFrame_Core(frame)->owner & FRAME_OWNED_BY_GENERATOR) ||
           FT_ATOMIC_LOAD_INT8_RELAXED(_PyGen_GetGeneratorFromFrame(frame)->gi_frame_state) == FRAME_CLEARED);
    // GH-99729: Clearing this frame can expose the stack (via finalizers). It's
    // crucial that this frame has been unlinked, and is no longer visible:
    assert(_PyThreadState_GET()->current_frame != _PyFrame_Core(frame));
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

// Calls the frame reifier and returns the original function object
_PyInterpreterFrame *
_PyFrame_InitializeExternalFrame(_PyInterpreterFrameCore *frame)
{
    if (_PyFrame_IsExternalFrame(frame)) {
        PyObject *executor = PyStackRef_AsPyObjectBorrow(frame->f_executable);
        PyUnstable_PyExternalExecutable *jit_exec = (PyUnstable_PyExternalExecutable *)executor;
        _PyInterpreterFrame *res = jit_exec->ef_reifier(frame, executor);
        assert(_PyFrame_Core(res) == frame);
        return res;
    }

    return _PyFrame_Full(frame);
}

/* Unstable API functions */

PyObject *
PyUnstable_InterpreterFrame_GetCode(struct _PyInterpreterFrameCore *frame)
{
    if (_PyFrame_IsExternalFrame(frame)) {
        PyObject *executable = PyStackRef_AsPyObjectBorrow(frame->f_executable);
        return Py_NewRef(((PyUnstable_PyExternalExecutable *)executable)->ef_code);
    }
    return PyStackRef_AsPyObjectNew(frame->f_executable);
}

int
PyUnstable_InterpreterFrame_GetLasti(struct _PyInterpreterFrameCore *frame)
{
    return _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
}

// NOTE: We allow racy accesses to the instruction pointer from other threads
// for sys._current_frames() and similar APIs.
int _Py_NO_SANITIZE_THREAD
PyUnstable_InterpreterFrame_GetLine(_PyInterpreterFrameCore *frame)
{
    int addr = _PyInterpreterFrame_LASTI(frame) * sizeof(_Py_CODEUNIT);
    return PyCode_Addr2Line(_PyFrame_GetCode(frame), addr);
}

static int
jitexecutable_traverse(PyObject *self, visitproc visit, void *arg)
{
    PyUnstable_PyExternalExecutable *o = (PyUnstable_PyExternalExecutable *)self;
    Py_VISIT(o->ef_code);
    Py_VISIT(o->ef_state);
    return 0;
}

static int
jitexecutable_clear(PyObject *self)
{
    PyUnstable_PyExternalExecutable *o = (PyUnstable_PyExternalExecutable *)self;
    Py_CLEAR(o->ef_code);
    Py_CLEAR(o->ef_state);
    return 0;
}

static void
jitexecutable_dealloc(PyObject *self)
{
    PyUnstable_PyExternalExecutable *o = (PyUnstable_PyExternalExecutable *)self;
    PyObject_GC_UnTrack(self);
    Py_DECREF(o->ef_code);
    Py_XDECREF(o->ef_state);
    Py_TYPE(self)->tp_free(self);
}

PyTypeObject PyUnstable_ExternalExecutable_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    .tp_name = "jit_executable",
    .tp_basicsize = sizeof(PyUnstable_PyExternalExecutable),
    .tp_dealloc = jitexecutable_dealloc,
    .tp_traverse = jitexecutable_traverse,
    .tp_clear = jitexecutable_clear,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_IMMUTABLETYPE | Py_TPFLAGS_HAVE_GC,
    .tp_alloc = PyType_GenericAlloc,
    .tp_free = PyObject_GC_Del,
};

PyObject *
PyUnstable_MakeExternalExecutable(_PyFrame_Reifier reifier, PyCodeObject *code, PyObject *state)
{
    if (reifier == NULL) {
        PyErr_SetString(PyExc_ValueError, "need reifier");
        return NULL;
    } else if (code == NULL) {
        PyErr_SetString(PyExc_ValueError, "need code object");
        return NULL;
    }

    PyUnstable_PyExternalExecutable *jit_exec = PyObject_GC_New(PyUnstable_PyExternalExecutable,
                                                           &PyUnstable_ExternalExecutable_Type);
    if (jit_exec == NULL) {
        return NULL;
    }

    jit_exec->ef_reifier = reifier;
    jit_exec->ef_code = (PyCodeObject *)Py_NewRef(code);
    jit_exec->ef_state = Py_XNewRef(state);
    if (state != NULL && PyObject_GC_IsTracked(state)) {
        PyObject_GC_Track((PyObject *)jit_exec);
    }
    return (PyObject *)jit_exec;
}

const PyTypeObject *const PyUnstable_ExecutableKinds[PyUnstable_EXECUTABLE_KINDS+1] = {
    [PyUnstable_EXECUTABLE_KIND_SKIP] = &_PyNone_Type,
    [PyUnstable_EXECUTABLE_KIND_PY_FUNCTION] = &PyCode_Type,
    [PyUnstable_EXECUTABLE_KIND_BUILTIN_FUNCTION] = &PyMethod_Type,
    [PyUnstable_EXECUTABLE_KIND_METHOD_DESCRIPTOR] = &PyMethodDescr_Type,
    [PyUnstable_EXECUTABLE_KIND_JIT] = &PyUnstable_ExternalExecutable_Type,
    [PyUnstable_EXECUTABLE_KINDS] = NULL,
};
