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

/* See Objects/frame_layout.md for an explanation of the frame stack
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
    char f_fast_as_locals;      /* Have the fast locals of this frame been converted to a dict? */
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
    FRAME_OWNED_BY_CSTACK = 3,
};

typedef struct _PyInterpreterFrame {
    PyObject *f_executable; /* Strong reference */
    struct _PyInterpreterFrame *previous;
    PyObject *f_funcobj; /* Strong reference. Only valid if not on C stack */
    PyObject *f_globals; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_builtins; /* Borrowed reference. Only valid if not on C stack */
    PyObject *f_locals; /* Strong reference, may be NULL. Only valid if not on C stack */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL. Only valid if not on C stack */
    _Py_CODEUNIT *instr_ptr; /* Instruction currently executing (or about to begin) */
    int stacktop;  /* Offset of TOS from localsplus  */
    uint16_t return_offset;  /* Only relevant during a function call */
    char owner;
    /* Locals and stack */
    PyObject *localsplus[1];
} _PyInterpreterFrame;

#define _PyInterpreterFrame_LASTI(IF) \
    ((int)((IF)->instr_ptr - _PyCode_CODE(_PyFrame_GetCode(IF))))

static inline PyCodeObject *_PyFrame_GetCode(_PyInterpreterFrame *f) {
    assert(PyCode_Check(f->f_executable));
    return (PyCodeObject *)f->f_executable;
}

static inline PyObject **_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return f->localsplus + _PyFrame_GetCode(f)->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_PyInterpreterFrame *f) {
    assert(f->stacktop > _PyFrame_GetCode(f)->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stacktop > _PyFrame_GetCode(f)->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, PyObject *value) {
    f->localsplus[f->stacktop] = value;
    f->stacktop++;
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

void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest);

/* Consumes reference to func and locals.
   Does not initialize frame->previous, which happens
   when frame is linked into the frame stack.
 */
static inline void
_PyFrame_Initialize(
    _PyInterpreterFrame *frame, PyFunctionObject *func,
    PyObject *locals, PyCodeObject *code, int null_locals_from)
{
    frame->f_funcobj = (PyObject *)func;
    frame->f_executable = Py_NewRef(code);
    frame->f_builtins = func->func_builtins;
    frame->f_globals = func->func_globals;
    frame->f_locals = locals;
    frame->stacktop = code->co_nlocalsplus;
    frame->frame_obj = NULL;
    frame->instr_ptr = _PyCode_CODE(code);
    frame->return_offset = 0;
    frame->owner = FRAME_OWNED_BY_THREAD;

    for (int i = null_locals_from; i < code->co_nlocalsplus; i++) {
        frame->localsplus[i] = NULL;
    }
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->localsplus;
}

/* Fetches the stack pointer, and sets stacktop to -1.
   Having stacktop <= 0 ensures that invalid
   values are not visible to the cycle GC.
   We choose -1 rather than 0 to assist debugging. */
static inline PyObject**
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    PyObject **sp = frame->localsplus + frame->stacktop;
    frame->stacktop = -1;
    return sp;
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - frame->localsplus);
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
    if (frame->owner == FRAME_OWNED_BY_CSTACK) {
        return true;
    }
    return frame->owner != FRAME_OWNED_BY_GENERATOR &&
        frame->instr_ptr < _PyCode_CODE(_PyFrame_GetCode(frame)) + _PyFrame_GetCode(frame)->_co_firsttraceable;
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
 * Returns a borrowed referennce */
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

PyObject *
_PyFrame_GetLocals(_PyInterpreterFrame *frame, int include_hidden);

int
_PyFrame_FastToLocalsWithError(_PyInterpreterFrame *frame);

void
_PyFrame_LocalsToFast(_PyInterpreterFrame *frame, int clear);

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

void _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Pushes a frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace()
 * Consumes reference to func. */
static inline _PyInterpreterFrame *
_PyFrame_PushUnchecked(PyThreadState *tstate, PyFunctionObject *func, int null_locals_from)
{
    CALL_STAT_INC(frames_pushed);
    PyCodeObject *code = (PyCodeObject *)func->func_code;
    _PyInterpreterFrame *new_frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    _PyFrame_Initialize(new_frame, func, NULL, code, null_locals_from);
    return new_frame;
}

/* Pushes a trampoline frame without checking for space.
 * Must be guarded by _PyThreadState_HasStackSpace() */
static inline _PyInterpreterFrame *
_PyFrame_PushTrampolineUnchecked(PyThreadState *tstate, PyCodeObject *code, int stackdepth)
{
    CALL_STAT_INC(frames_pushed);
    _PyInterpreterFrame *frame = (_PyInterpreterFrame *)tstate->datastack_top;
    tstate->datastack_top += code->co_framesize;
    assert(tstate->datastack_top < tstate->datastack_limit);
    frame->f_funcobj = Py_None;
    frame->f_executable = Py_NewRef(code);
#ifdef Py_DEBUG
    frame->f_builtins = NULL;
    frame->f_globals = NULL;
#endif
    frame->f_locals = NULL;
    frame->stacktop = code->co_nlocalsplus + stackdepth;
    frame->frame_obj = NULL;
    frame->instr_ptr = _PyCode_CODE(code);
    frame->owner = FRAME_OWNED_BY_THREAD;
    frame->return_offset = 0;
    return frame;
}

static inline
PyGenObject *_PyFrame_GetGenerator(_PyInterpreterFrame *frame)
{
    assert(frame->owner == FRAME_OWNED_BY_GENERATOR);
    size_t offset_in_gen = offsetof(PyGenObject, gi_iframe);
    return (PyGenObject *)(((char *)frame) - offset_in_gen);
}

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
