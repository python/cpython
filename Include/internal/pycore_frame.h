#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _interpreter_frame *f_frame; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_owns_frame;          /* This frame owns the frame */
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

/* runtime lifecycle */

extern void _PyFrame_Fini(PyInterpreterState *interp);


/* other API */

/* These values are chosen so that the inline functions below all
 * compare f_state to zero.
 */
enum _framestate {
    FRAME_CREATED = -2,
    FRAME_SUSPENDED = -1,
    FRAME_EXECUTING = 0,
    FRAME_RETURNED = 1,
    FRAME_UNWINDING = 2,
    FRAME_RAISED = 3,
    FRAME_CLEARED = 4
};

typedef signed char PyFrameState;

/*
    frame->f_lasti refers to the index of the last instruction,
    unless it's -1 in which case next_instr should be first_instr.
*/

typedef struct _interpreter_frame {
    PyFunctionObject *f_func; /* Strong reference */
    PyObject *f_globals; /* Borrowed reference */
    PyObject *f_builtins; /* Borrowed reference */
    PyObject *f_locals; /* Strong reference, may be NULL */
    PyCodeObject *f_code; /* Strong reference */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL */
    struct _interpreter_frame *previous;
    int f_lasti;       /* Last instruction if called */
    int stacktop;     /* Offset of TOS from localsplus  */
    PyFrameState f_state;  /* What state the frame is in */
    bool is_entry;  // Whether this is the "root" frame for the current CFrame.
    bool is_generator;
    PyObject *localsplus[1];
} InterpreterFrame;

static inline int _PyFrame_IsRunnable(InterpreterFrame *f) {
    return f->f_state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(InterpreterFrame *f) {
    return f->f_state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(InterpreterFrame *f) {
    return f->f_state > FRAME_EXECUTING;
}

static inline PyObject **_PyFrame_Stackbase(InterpreterFrame *f) {
    return f->localsplus + f->f_code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(InterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(InterpreterFrame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(InterpreterFrame *f, PyObject *value) {
    f->localsplus[f->stacktop] = value;
    f->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((sizeof(InterpreterFrame)-1)/sizeof(PyObject *))

void _PyFrame_Copy(InterpreterFrame *src, InterpreterFrame *dest);

/* Consumes reference to func */
static inline void
_PyFrame_InitializeSpecials(
    InterpreterFrame *frame, PyFunctionObject *func,
    PyObject *locals, int nlocalsplus)
{
    frame->f_func = func;
    frame->f_code = (PyCodeObject *)Py_NewRef(func->func_code);
    frame->f_builtins = func->func_builtins;
    frame->f_globals = func->func_globals;
    frame->f_locals = Py_XNewRef(locals);
    frame->stacktop = nlocalsplus;
    frame->frame_obj = NULL;
    frame->f_lasti = -1;
    frame->f_state = FRAME_CREATED;
    frame->is_entry = false;
    frame->is_generator = false;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(InterpreterFrame *frame)
{
    return frame->localsplus;
}

static inline PyObject**
_PyFrame_GetStackPointer(InterpreterFrame *frame)
{
    return frame->localsplus+frame->stacktop;
}

static inline void
_PyFrame_SetStackPointer(InterpreterFrame *frame, PyObject **stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - frame->localsplus);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(InterpreterFrame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyFrame_GetFrameObject(InterpreterFrame *frame)
{
    PyFrameObject *res =  frame->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame);
}

/* Clears all references in the frame.
 * If take is non-zero, then the InterpreterFrame frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_Clear(InterpreterFrame * frame);

int
_PyFrame_Traverse(InterpreterFrame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(InterpreterFrame *frame);

void
_PyFrame_LocalsToFast(InterpreterFrame *frame, int clear);

extern InterpreterFrame *
_PyThreadState_BumpFramePointerSlow(PyThreadState *tstate, size_t size);

static inline InterpreterFrame *
_PyThreadState_BumpFramePointer(PyThreadState *tstate, size_t size)
{
    PyObject **base = tstate->datastack_top;
    if (base) {
        PyObject **top = base + size;
        assert(tstate->datastack_limit);
        if (top < tstate->datastack_limit) {
            tstate->datastack_top = top;
            return (InterpreterFrame *)base;
        }
    }
    return _PyThreadState_BumpFramePointerSlow(tstate, size);
}

void _PyThreadState_PopFrame(PyThreadState *tstate, InterpreterFrame *frame);

/* Consume reference to func */
InterpreterFrame *
_PyFrame_Push(PyThreadState *tstate, PyFunctionObject *func);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
