#ifndef Py_INTERNAL_INTERP_FRAME_H
#define Py_INTERNAL_INTERP_FRAME_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include "pycore_code.h"          // _PyCode_CODE()
#include "pycore_interpframe_structs.h" // _PyControlRecord
#include "pycore_pyatomic_ft_wrappers.h" // FT_ATOMIC_LOAD_PTR_ACQUIRE()
#include "pycore_stackref.h"      // PyStackRef_AsPyObjectBorrow()
#include "pycore_stats.h"         // CALL_STAT_INC()

#ifdef __cplusplus
extern "C" {
#endif

#define _PyControlRecord_LASTI(IF) \
    ((int)((IF)->instr_ptr - _PyFrame_GetBytecode((IF))))

static inline PyCodeObject *_PyFrame_GetCode(_PyControlRecord *f) {
    assert(!PyStackRef_IsNull(f->f_executable));
    PyObject *executable = PyStackRef_AsPyObjectBorrow(f->f_executable);
    assert(PyCode_Check(executable));
    return (PyCodeObject *)executable;
}

// Similar to _PyFrame_GetCode(), but return NULL if the frame is invalid or
// freed. Used by dump_frame() in Python/traceback.c. The function uses
// heuristics to detect freed memory, it's not 100% reliable.
static inline PyCodeObject* _Py_NO_SANITIZE_THREAD
_PyFrame_SafeGetCode(_PyControlRecord *f)
{
    if (PyStackRef_IsNull(f->f_executable)) {
        return NULL;
    }
    void *ptr;
    memcpy(&ptr, &f->f_executable, sizeof(f->f_executable));
    if (_PyMem_IsPtrFreed(ptr)) {
        return NULL;
    }
    PyObject *executable = PyStackRef_AsPyObjectBorrow(f->f_executable);
    if (_PyObject_IsFreed(executable)) {
        return NULL;
    }
    if (!PyCode_Check(executable)) {
        return NULL;
    }
    return (PyCodeObject *)executable;
}

static inline _Py_CODEUNIT *
_PyFrame_GetBytecode(_PyControlRecord *f)
{
#ifdef Py_GIL_DISABLED
    PyCodeObject *co = _PyFrame_GetCode(f);
    _PyCodeArray *tlbc = _PyCode_GetTLBCArray(co);
    assert(f->tlbc_index >= 0 && f->tlbc_index < tlbc->size);
    return (_Py_CODEUNIT *)tlbc->entries[f->tlbc_index];
#else
    return _PyCode_CODE(_PyFrame_GetCode(f));
#endif
}

// Similar to PyUnstable_InterpreterFrame_GetLasti(), but return NULL if the
// frame is invalid or freed. Used by dump_frame() in Python/traceback.c. The
// function uses heuristics to detect freed memory, it's not 100% reliable.
static inline int _Py_NO_SANITIZE_THREAD
_PyFrame_SafeGetLasti(struct _PyControlRecord *f)
{
    // Code based on _PyFrame_GetBytecode() but replace _PyFrame_GetCode()
    // with _PyFrame_SafeGetCode().
    PyCodeObject *co = _PyFrame_SafeGetCode(f);
    if (co == NULL) {
        return -1;
    }

    _Py_CODEUNIT *bytecode;
#ifdef Py_GIL_DISABLED
    _PyCodeArray *tlbc = _PyCode_GetTLBCArray(co);
    assert(f->tlbc_index >= 0 && f->tlbc_index < tlbc->size);
    bytecode = (_Py_CODEUNIT *)tlbc->entries[f->tlbc_index];
#else
    bytecode = _PyCode_CODE(co);
#endif

    return (int)(f->instr_ptr - bytecode) * sizeof(_Py_CODEUNIT);
}

static inline PyFunctionObject *_PyFrame_GetFunction(_PyInterpreterDataFrame *f) {
    PyObject *func = PyStackRef_AsPyObjectBorrow(f->f_funcobj);
    assert(PyFunction_Check(func));
    return (PyFunctionObject *)func;
}

static inline _PyStackRef *_PyFrame_Stackbase(_PyControlRecord *f, _PyInterpreterDataFrame *fp) {
    return (fp->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus);
}

static inline _PyStackRef _PyFrame_StackPeek(_PyControlRecord *f, int depth) {
    assert(f->stackpointer > _PyFrame_Stackbase(f, f->data));
    assert(!PyStackRef_IsNull(f->stackpointer[-depth]));
    return f->stackpointer[-depth];
}

static inline _PyStackRef _PyFrame_StackPop(_PyControlRecord *f) {
    assert(f->stackpointer > _PyFrame_Stackbase(f, f->data));
    f->stackpointer--;
    return *f->stackpointer;
}

static inline void _PyFrame_StackPush(_PyControlRecord *f, _PyStackRef value) {
    *f->stackpointer = value;
    f->stackpointer++;
}

#define FRAME_SPECIALS_SIZE ((int)(offsetof(_PyInterpreterDataFrame, localsplus) / sizeof(PyObject *)))


static inline int
_PyFrame_NumSlotsForCodeObject(PyCodeObject *code)
{
    /* This function needs to remain in sync with the calculation of
     * co_framesize in Tools/build/deepfreeze.py */
    assert(code->co_framesize >= FRAME_SPECIALS_SIZE);
    return code->co_framesize - FRAME_SPECIALS_SIZE;
}

static inline void
_PyFrame_Copy(_PyControlRecord *src, _PyInterpreterDataFrame *src_data,
              _PyControlRecord *dest, _PyInterpreterDataFrame *dest_data)
{
    assert(src->data == src_data && dest->data == dest_data);
    dest->f_executable = PyStackRef_MakeHeapSafe(src->f_executable);
    // Don't leave a dangling pointer to the old frame when creating generators
    // and coroutines:
    dest->state = CONTROL_DETACHED;
    dest_data->f_funcobj = PyStackRef_MakeHeapSafe(src_data->f_funcobj);
    dest_data->f_globals = src_data->f_globals;
    dest_data->f_builtins = src_data->f_builtins;
    dest_data->f_locals = src_data->f_locals;
    dest_data->frame_obj = src_data->frame_obj;
    dest->instr_ptr = src->instr_ptr;
    dest->return_offset = 0;
#ifdef Py_GIL_DISABLED
    dest->tlbc_index = src->tlbc_index;
#endif
    assert(src->stackpointer != NULL);
    int stacktop = (int)(src->stackpointer - src_data->localsplus);
    assert(stacktop >= 0);
    dest->stackpointer = dest_data->localsplus + stacktop;
#ifdef Py_DEBUG
    dest->stackpointer_valid =  src->stackpointer_valid;
    dest->lltrace = src->lltrace;
#endif
    for (int i = 0; i < stacktop; i++) {
        dest_data->localsplus[i] = PyStackRef_MakeHeapSafe(src_data->localsplus[i]);
    }
}

#ifdef Py_GIL_DISABLED
static inline void
_PyFrame_InitializeTLBC(PyThreadState *tstate, _PyControlRecord *frame,
                        PyCodeObject *code)
{
    _Py_CODEUNIT *tlbc = _PyCode_GetTLBCFast(tstate, code);
    if (tlbc == NULL) {
        // No thread-local bytecode exists for this thread yet; use the main
        // thread's copy, deferring thread-local bytecode creation to the
        // execution of RESUME.
        frame->instr_ptr = _PyCode_CODE(code);
        frame->tlbc_index = 0;
    }
    else {
        frame->instr_ptr = tlbc;
        frame->tlbc_index = ((_PyThreadStateImpl *)tstate)->tlbc_index;
    }
}
#endif

/* Consumes reference to func and locals. */
static inline void
_PyFrame_Initialize(
    PyThreadState *tstate, _PyControlRecord *frame,
    _PyInterpreterDataFrame *fp, _PyStackRef func,
    PyObject *locals, PyCodeObject *code, int null_locals_from)
{
    fp->f_funcobj = func;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    fp->f_builtins = func_obj->func_builtins;
    fp->f_globals = func_obj->func_globals;
    fp->f_locals = locals;
    frame->stackpointer = fp->localsplus + code->co_nlocalsplus;
    fp->frame_obj = NULL;
#ifdef Py_GIL_DISABLED
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    (void)tstate;
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->return_offset = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;
#ifdef Py_DEBUG
    frame->stackpointer_valid = 1;
    frame->lltrace = 0;
#endif

    for (int i = null_locals_from; i < code->co_nlocalsplus; i++) {
        fp->localsplus[i] = PyStackRef_NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline _PyStackRef*
_PyFrame_GetLocalsArray(_PyInterpreterDataFrame *frame)
{
    return frame->localsplus;
}

// Fetches the stack pointer, and (on debug builds) sets stackpointer to NULL.
// Having stackpointer == NULL makes it easier to catch missing stack pointer
// spills/restores (which could expose invalid values to the GC) using asserts.
static inline _PyStackRef*
_PyFrame_GetStackPointer(_PyControlRecord *frame)
{
    return frame->stackpointer;
}

static inline void
_PyFrame_SetStackPointer(_PyControlRecord *frame, _PyStackRef *stack_pointer)
{
    frame->stackpointer = stack_pointer;
}

static inline void
_PyFrame_StackPointerValidate(_PyControlRecord *frame)
{
#ifdef Py_DEBUG
/* Avoid bloating the JIT code */
#ifndef _Py_JIT
    assert(frame->stackpointer_valid == 0);
#endif
    frame->stackpointer_valid = 1;
#endif
}

static inline void
_PyFrame_StackPointerInvalidate(_PyControlRecord *frame)
{
#ifdef Py_DEBUG
/* Avoid bloating the JIT code */
#ifndef _Py_JIT
    assert(frame->stackpointer_valid == 1);
#endif
    frame->stackpointer_valid = 0;
#endif
}

static inline void
_PyFrame_StackAssertInvalid(_PyControlRecord *frame)
{
#ifdef Py_DEBUG
/* Avoid bloating the JIT code */
#ifndef _Py_JIT
    assert(frame->stackpointer_valid == 0);
#endif
#endif
}


/* Determine whether a frame is incomplete.
 * A frame is incomplete if it is part way through
 * creating cell objects or a generator or coroutine.
 *
 * Frames on the frame stack are incomplete until the
 * first RESUME instruction.
 * Frames owned by a generator are always complete.
 *
 * NOTE: We allow racy accesses to the instruction pointer
 * from other threads for sys._current_frames() and similar APIs.
 */
static inline bool _Py_NO_SANITIZE_THREAD
_PyFrame_IsIncomplete(_PyControlRecord *frame)
{
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return true;
    }
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
           frame->instr_ptr < _PyFrame_GetBytecode(frame) +
                                  _PyFrame_GetCode(frame)->_co_firsttraceable;
}

static inline _PyControlRecord *
_PyFrame_GetCaller(_PyControlRecord *frame)
{
    assert(frame->state != CONTROL_DETACHED && frame->state != CONTROL_BASE);
    do {
        --frame;
    } while (frame->state < CONTROL_ACTIVE);
    return frame;
}

static inline _PyControlRecord *
_PyFrame_GetPrevious(_PyControlRecord *frame)
{
    if (frame->state == CONTROL_DETACHED || frame->state == CONTROL_BASE) {
        return NULL;
    }
    return _PyFrame_GetCaller(frame);
}

static inline _PyControlRecord *
_PyFrame_GetFirstComplete(_PyControlRecord *frame)
{
    while (frame && _PyFrame_IsIncomplete(frame)) {
        frame = _PyFrame_GetPrevious(frame);
    }
    return frame;
}

static inline _PyControlRecord *
_PyThreadState_GetFrame(PyThreadState *tstate)
{
    return _PyFrame_GetFirstComplete(tstate->current_frame);
}

// Update last_profiled_frame for remote profiler frame caching.
// Only update if we're removing the exact frame that was last profiled.
// This avoids corrupting the cache when transient frames (called and returned
// between profiler samples) update last_profiled_frame to addresses the
// profiler never saw.
// The sequence distinguishes this anchor from a later frame that reuses the
// same _PyControlRecord address.
#define _PyThreadState_UpdateLastProfiledFrame(tstate, frame, previous) \
    do { \
        PyThreadState *tstate_ = (tstate); \
        _PyControlRecord *frame_ = (frame); \
        if (tstate_->last_profiled_frame == frame_) { \
            tstate_->last_profiled_frame = (previous); \
            tstate_->last_profiled_frame_seq++; \
        } \
    } while (0)

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyAPI_FUNC(PyFrameObject *)
_PyFrame_MakeAndSetFrameObject(_PyControlRecord *frame, _PyInterpreterDataFrame *fp);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed reference */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyControlRecord *frame, _PyInterpreterDataFrame *fp)
{

    assert(!_PyFrame_IsIncomplete(frame));
    PyFrameObject *res = (PyFrameObject*)FT_ATOMIC_LOAD_PTR_ACQUIRE(fp->frame_obj);
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame, fp);
}

// Exported for external JIT support
PyAPI_FUNC(void)
_PyFrame_ClearLocals(_PyControlRecord *frame, _PyInterpreterDataFrame *fp);

/* Clears all references in the frame.
 * If take is non-zero, then the _PyControlRecord frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 *
 * Exported for external JIT support
 */
 PyAPI_FUNC(void)
_PyFrame_ClearExceptCode(_PyControlRecord *frame, _PyInterpreterDataFrame *fp);

PyAPI_FUNC(void)
_PyFrame_TakeOwnership(PyFrameObject *f, _PyControlRecord *frame,
                       _PyInterpreterDataFrame *fp);

int
_PyFrame_Traverse(_PyControlRecord *frame, _PyInterpreterDataFrame *fp,
                  visitproc visit, void *arg);

bool
_PyFrame_HasHiddenLocals(_PyControlRecord *frame);

PyObject *
_PyFrame_GetLocals(_PyControlRecord *frame);

static inline bool
_PyThreadState_HasControlSpace(PyThreadState *tstate, int frames)
{
    return frames <= tstate->controlstack_limit - tstate->controlstack_top;
}

static inline bool
_PyThreadState_HasStackSpace(PyThreadState *tstate, int size)
{
    return _PyThreadState_HasControlSpace(tstate, 2) &&
        tstate->datastack_top != NULL &&
        size < tstate->datastack_limit - tstate->datastack_top;
}

static inline Py_ssize_t
_PyThreadState_GetRecursionDepth(PyThreadState *tstate)
{
    return tstate->controlstack_top - tstate->base_frame - 1 -
        tstate->controlstack_extra;
}

static inline void
_PyThreadState_UpdateControlLimit(PyThreadState *tstate)
{
    size_t capacity = tstate->controlstack_end - tstate->base_frame;
    size_t limit = 1 + (size_t)tstate->py_recursion_limit +
        tstate->controlstack_extra + (tstate->recursion_headroom ? 50 : 0);
    tstate->controlstack_limit = tstate->base_frame + Py_MIN(capacity, limit);
}

static inline void
_PyThreadState_AdjustControlLimit(PyThreadState *tstate, int delta)
{
    tstate->controlstack_extra += delta;
    if (tstate->controlstack_limit < tstate->controlstack_end) {
        tstate->controlstack_limit += delta;
    }
    else {
        _PyThreadState_UpdateControlLimit(tstate);
    }
}

PyAPI_FUNC(_PyControlRecord *) _PyThreadState_PushControlSlow(PyThreadState *tstate);

static inline _PyControlRecord *
_PyThreadState_PushControlUnchecked(PyThreadState *tstate)
{
    assert(_PyThreadState_HasControlSpace(tstate, 1));
    _PyControlRecord *frame = tstate->controlstack_top++;
    frame->state = CONTROL_RESERVED;
    return frame;
}

static inline _PyControlRecord *
_PyThreadState_PushControl(PyThreadState *tstate)
{
    if (_PyThreadState_HasControlSpace(tstate, 1)) {
        return _PyThreadState_PushControlUnchecked(tstate);
    }
    return _PyThreadState_PushControlSlow(tstate);
}

static inline void
_PyThreadState_PopControl(PyThreadState *tstate, _PyControlRecord *frame)
{
    assert(frame + 1 == tstate->controlstack_top);
    tstate->controlstack_top = frame;
}

static inline _PyControlRecord *
_PyThreadState_PushFrameUnchecked(PyThreadState *tstate, int nslots)
{
    _PyControlRecord *frame = _PyThreadState_PushControlUnchecked(tstate);
    frame->data = (_PyInterpreterDataFrame *)tstate->datastack_top;
    tstate->datastack_top += nslots;
    assert(tstate->datastack_top <= tstate->datastack_limit);
    return frame;
}

// Exported for external JIT support
PyAPI_FUNC(_PyControlRecord *)
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size);

PyAPI_FUNC(void) _PyThreadState_PopFrame(
    PyThreadState *tstate, _PyControlRecord *frame, _PyInterpreterDataFrame *fp);

PyAPI_FUNC(void) _PyThreadState_PopDataFrame(
    PyThreadState *tstate, _PyInterpreterDataFrame *fp);

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyControlRecord *
_PyFrame_PushUnchecked(PyThreadState *tstate, _PyStackRef func, int null_locals_from)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    _PyInterpreterDataFrame *fp = (_PyInterpreterDataFrame *)tstate->datastack_top;
    _PyControlRecord *new_frame = _PyThreadState_PushFrameUnchecked(
        tstate, code->co_framesize);
    _PyFrame_Initialize(tstate, new_frame, fp, func, NULL, code, null_locals_from);
    return new_frame;
}

static inline _PyControlRecord *
_PyFrame_PushTrampoline(PyThreadState *tstate, PyCodeObject *code, int stackdepth)
{
    CALL_STAT_INC(frames_pushed);
    _PyControlRecord *frame = _PyThreadState_PushFrame(
        tstate, code->co_framesize);
    if (frame == NULL) {
        return NULL;
    }
    _PyInterpreterDataFrame *fp = frame->data;
    fp->f_funcobj = PyStackRef_None;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
#ifdef Py_DEBUG
    fp->f_builtins = NULL;
    fp->f_globals = NULL;
#endif
    fp->f_locals = NULL;
    assert(stackdepth <= code->co_stacksize);
    frame->stackpointer = fp->localsplus + code->co_nlocalsplus + stackdepth;
    fp->frame_obj = NULL;
#ifdef Py_GIL_DISABLED
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->owner = FRAME_OWNED_BY_THREAD;
#ifdef Py_DEBUG
    frame->stackpointer_valid = 1;
    frame->lltrace = 0;
#endif
    frame->return_offset = 0;
    return frame;
}

PyAPI_FUNC(_PyControlRecord *)
_PyEvalFramePushAndInit(PyThreadState *tstate, _PyStackRef func,
                        PyObject *locals, _PyStackRef const *args,
                        size_t argcount, PyObject *kwnames);

PyAPI_FUNC(_PyControlRecord *)
_PyEvalFramePushAndInit_Ex(PyThreadState *tstate, _PyStackRef func,
    PyObject *locals, Py_ssize_t nargs, PyObject *callargs, PyObject *kwargs);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_INTERP_FRAME_H
