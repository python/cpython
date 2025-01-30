#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#include <stdbool.h>
#include <stddef.h>               // offsetof()
#include "pycore_code.h"          // STATS
#include "pycore_stackref.h"      // _PyStackRef

/* See InternalDocs/frames.md for an explanation of the frame stack
 * including explanation of the PyFrameObject and _PyInterpreterFrame
 * structs. */


struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _PyInterpreterFrame *f_frame; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    PyObject *f_extra_locals;   /* Dict for locals set by users using f_locals, could be NULL */
    /* This is purely for backwards compatibility for PyEval_GetLocals.
       PyEval_GetLocals requires a borrowed reference so the actual reference
       is stored here */
    PyObject *f_locals_cache;
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);


/* other API */

typedef enum _framestate {
    FRAME_CREATED = -3,
    FRAME_SUSPENDED = -2,
    FRAME_SUSPENDED_YIELD_FROM = -1,
    FRAME_EXECUTING = 0,
    FRAME_COMPLETED = 1,
    FRAME_CLEARED = 4
} PyFrameState;

#define FRAME_STATE_SUSPENDED(S) ((S) == FRAME_SUSPENDED || (S) == FRAME_SUSPENDED_YIELD_FROM)
#define FRAME_STATE_FINISHED(S) ((S) >= FRAME_COMPLETED)

enum _frameowner {
    FRAME_OWNED_BY_THREAD = 0,
    FRAME_OWNED_BY_GENERATOR = 1,
    FRAME_OWNED_BY_FRAME_OBJECT = 2,
    FRAME_OWNED_BY_INTERPRETER = 3,
    FRAME_OWNED_BY_CSTACK = 4,
};

typedef struct _PyInterpreterFrame {
    _PyStackRef f_executable; /* Deferred or strong reference (code object or None) */
    struct _PyInterpreterFrame *previous;
    _PyStackRef f_funcobj; /* Deferred or strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    _Py_CODEUNIT *instr_ptr; /* Instruction currently executing (or about to begin) */
    _PyStackRef *stackpointer;
#ifdef Py_GIL_DISABLED
    /* Index of thread-local bytecode containing instr_ptr. */
    int32_t tlbc_index;
#endif
    uint16_t return_offset;  /* Only relevant during a function call */
    char owner;
#ifdef Py_DEBUG
    uint8_t visited:1;
    uint8_t lltrace:7;
#else
    uint8_t visited;
#endif
    /* Locals and stack */
    _PyStackRef localsplus[1];
} _PyInterpreterFrame;

#define _PyInterpreterFrame_LASTI(IF) \
    ((int)((IF)->instr_ptr - _PyFrame_GetBytecode((IF))))

static inline PyCodeObject *_PyFrame_GetCode(_PyInterpreterFrame *f) {
    PyObject *executable = PyStackRef_AsPyObjectBorrow(f->f_executable);
    assert(PyCode_Check(executable));
    return (PyCodeObject *)executable;
}

static inline _Py_CODEUNIT *
_PyFrame_GetBytecode(_PyInterpreterFrame *f)
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

static inline PyFunctionObject *_PyFrame_GetFunction(_PyInterpreterFrame *f) {
    PyObject *func = PyStackRef_AsPyObjectBorrow(f->f_funcobj);
    assert(PyFunction_Check(func));
    return (PyFunctionObject *)func;
}

static inline _PyStackRef *_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return (f->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus);
}

static inline _PyStackRef _PyFrame_StackPeek(_PyInterpreterFrame *f) {
    assert(f->stackpointer >  f->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus);
    assert(!PyStackRef_IsNull(f->stackpointer[-1]));
    return f->stackpointer[-1];
}

static inline _PyStackRef _PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stackpointer >  f->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus);
    f->stackpointer--;
    return *f->stackpointer;
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, _PyStackRef value) {
    *f->stackpointer = value;
    f->stackpointer++;
}

#define FRAME_SPECIALS_SIZE ((int)((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *)))

static inline int
_PyFrame_NumSlotsForCodeObject(PyCodeObject *code)
{
    /* This function needs to remain in sync with the calculation of
     * co_framesize in Tools/build/deepfreeze.py */
    assert(code->co_framesize >= FRAME_SPECIALS_SIZE);
    return code->co_framesize - FRAME_SPECIALS_SIZE;
}

static inline void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest)
{
    *dest = *src;
    assert(src->stackpointer != NULL);
    int stacktop = (int)(src->stackpointer - src->localsplus);
    assert(stacktop >= _PyFrame_GetCode(src)->co_nlocalsplus);
    dest->stackpointer = dest->localsplus + stacktop;
    for (int i = 1; i < stacktop; i++) {
        dest->localsplus[i] = src->localsplus[i];
    }
    // Don't leave a dangling pointer to the old frame when creating generators
    // and coroutines:
    dest->previous = NULL;
}

#ifdef Py_GIL_DISABLED
static inline void
_PyFrame_InitializeTLBC(PyThreadState *tstate, _PyInterpreterFrame *frame,
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

/* Consumes reference to func and locals.
   Does not initialize frame->previous, which happens
   when frame is linked into the frame stack.
 */
static inline void
_PyFrame_Initialize(
    PyThreadState *tstate, _PyInterpreterFrame *frame, _PyStackRef func,
    PyObject *locals, PyCodeObject *code, int null_locals_from, _PyInterpreterFrame *previous)
{
    frame->previous = previous;
    frame->f_funcobj = func;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    frame->f_builtins = func_obj->func_builtins;
    frame->f_globals = func_obj->func_globals;
    frame->f_locals = locals;
    frame->stackpointer = frame->localsplus + code->co_nlocalsplus;
    frame->frame_obj = NULL;
#ifdef Py_GIL_DISABLED
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    (void)tstate;
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->return_offset = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif

    for (int i = null_locals_from; i < code->co_nlocalsplus; i++) {
        frame->localsplus[i] = PyStackRef_NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline _PyStackRef*
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->localsplus;
}

/* Fetches the stack pointer, and sets stackpointer to NULL.
   Having stackpointer == NULL ensures that invalid
   values are not visible to the cycle GC. */
static inline _PyStackRef*
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    assert(frame->stackpointer != NULL);
    _PyStackRef *sp = frame->stackpointer;
    frame->stackpointer = NULL;
    return sp;
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer)
{
    assert(frame->stackpointer == NULL);
    frame->stackpointer = stack_pointer;
}

/* Determine whether a frame is incomplete.
 * A frame is incomplete if it is part way through
 * creating cell objects or a generator or coroutine.
 *
 * Frames on the frame stack are incomplete until the
 * first RESUME instruction.
 * Frames owned by a generator are always complete.
 */
static inline bool
_PyFrame_IsIncomplete(_PyInterpreterFrame *frame)
{
    if (frame->owner >= FRAME_OWNED_BY_INTERPRETER) {
        return true;
    }
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
           frame->instr_ptr < _PyFrame_GetBytecode(frame) +
                                  _PyFrame_GetCode(frame)->_co_firsttraceable;
}

static inline _PyInterpreterFrame *
_PyFrame_GetFirstComplete(_PyInterpreterFrame *frame)
{
    while (frame && _PyFrame_IsIncomplete(frame)) {
        frame = frame->previous;
    }
    return frame;
}

static inline _PyInterpreterFrame *
_PyThreadState_GetFrame(PyThreadState *tstate)
{
    return _PyFrame_GetFirstComplete(tstate->current_frame);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed reference */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyInterpreterFrame *frame)
{

    assert(!_PyFrame_IsIncomplete(frame));
    PyFrameObject *res =  frame->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame);
}

void
_PyFrame_ClearLocals(_PyInterpreterFrame *frame);

/* Clears all references in the frame.
 * If take is non-zero, then the _PyInterpreterFrame frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_ClearExceptCode(_PyInterpreterFrame * frame);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

bool
_PyFrame_HasHiddenLocals(_PyInterpreterFrame *frame);

PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame);

static inline bool
_PyThreadState_HasStackSpace(PyThreadState *tstate, int size)
{
    assert(
        (tstate->datastack_top == NULL && tstate->datastack_limit == NULL)
        ||
        (tstate->datastack_top != NULL && tstate->datastack_limit != NULL)
    );
    return tstate->datastack_top != NULL &&
        size < tstate->datastack_limit - tstate->datastack_top;
}

extern _PyInterpreterFrame *
_PyThreadState_PushFrame(PyThreadState *tstate, size_t size);

PyAPI_FUNC(void) _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, _PyStackRef func, int null_locals_from, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    PyFunctionObject *func_obj = (PyFunctionObject *)PyStackRef_AsPyObjectBorrow(func);
    PyCodeObject *code = (PyCodeObject *)func_obj->func_code;
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    _PyFrame_Initialize(tstate, new_frame, func, NULL, code, null_locals_from,
                        previous);
    return new_frame;
}

/* Pushes a trampoline frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace() */
static inline _PyInterpreterFrame *
_PyFrame_PushTrampolineUnchecked(PyThreadState *tstate, PyCodeObject *code, int stackdepth, _PyInterpreterFrame * previous)
{
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    frame->previous = previous;
    frame->f_funcobj = PyStackRef_None;
    frame->f_executable = PyStackRef_FromPyObjectNew(code);
#ifdef Py_DEBUG
    frame->f_builtins = NULL;
    frame->f_globals = NULL;
#endif
    frame->f_locals = NULL;
    assert(stackdepth <= code->co_stacksize);
    frame->stackpointer = frame->localsplus + code->co_nlocalsplus + stackdepth;
    frame->frame_obj = NULL;
#ifdef Py_GIL_DISABLED
    _PyFrame_InitializeTLBC(tstate, frame, code);
#else
    frame->instr_ptr = _PyCode_CODE(code);
#endif
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->visited = 0;
#ifdef Py_DEBUG
    frame->lltrace = 0;
#endif
    frame->return_offset = 0;
    return frame;
}

PyAPI_FUNC(_PyInterpreterFrame *)
_PyEvalFramePushAndInit(PyThreadState *tstate, _PyStackRef func,
                        PyObject *locals, _PyStackRef const* args,
                        size_t argcount, PyObject *kwnames,
                        _PyInterpreterFrame *previous);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
