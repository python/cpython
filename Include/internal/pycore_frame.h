#ifndef Py_INTERNAL_FRAMEDATA_H
#define Py_INTERNAL_FRAMEDATA_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

/* Starting in CPython 3.11, CPython separates the frame state between the
 * full frame objects exposed by the Python and C runtime state introspection
 * APIs, and internal lighter weight frame data structs, which are simple C
 * structures owned by either the interpreter eval loop (while executing
 * ordinary functions), by a generator or coroutine object (for frames that
 * are able to be suspended), or by their corresponding full frame object (if
 * a state instrospection API has been invoked and the full frame object has
 * taken responsibility for the lifecycle of the frame data storage).
 *
 * This split storage eliminates a lot of allocation and deallocation of full
 * Python objects during code execution, providing a significant speed gain
 * over the previous approach of using full Python objects for both
 * introspection and code execution.
 */
// Declaration of _PyInterpreterFrame is in cpython/pystate.h for use in PyThreadState


struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _PyInterpreterFrame *f_fdata; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_owns_frame;          /* This frame owns the frame */
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);

/* runtime lifecycle */

extern void _PyFrame_Fini(PyInterpreterState *interp);


/* other API */

/* These values are chosen so that the inline functions below all
 * compare fdata->state to zero.
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
    frame->lasti refers to the index of the last instruction,
    unless it's -1 in which case next_instr should be first_instr.
*/

typedef struct _PyInterpreterFrame {
    PyFunctionObject *func; /* Strong reference */
    PyObject *globals; /* Borrowed reference */
    PyObject *builtins; /* Borrowed reference */
    PyObject *locals; /* Strong reference, may be NULL */
    PyCodeObject *code; /* Strong reference */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL */
    struct _PyInterpreterFrame *previous;
    int lasti;       /* Last instruction if called */
    int stacktop;     /* Offset of TOS from localsplus  */
    PyFrameState state;  /* What state the frame is in */
    bool is_entry;  // Whether this is the "root" frame for the current _PyCFrame.
    bool is_generator;
    PyObject *localsplus[1];
} _PyInterpreterFrame;

static inline int _PyFrame_IsRunnable(_PyInterpreterFrame *f) {
    return f->state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(_PyInterpreterFrame *f) {
    return f->state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(_PyInterpreterFrame *f) {
    return f->state > FRAME_EXECUTING;
}

static inline PyObject **_PyFrame_Stackbase(_PyInterpreterFrame *f) {
    return f->localsplus + f->code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->code->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(_PyInterpreterFrame *f) {
    assert(f->stacktop > f->code->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(_PyInterpreterFrame *f, PyObject *value) {
    f->localsplus[f->stacktop] = value;
    f->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *))

void _PyFrame_Copy(_PyInterpreterFrame *src, _PyInterpreterFrame *dest);

/* Consumes reference to func */
static inline void
_PyFrame_InitializeSpecials(
    _PyInterpreterFrame *frame, PyFunctionObject *func,
    PyObject *locals, int nlocalsplus)
{
    frame->func = func;
    frame->code = (PyCodeObject *)Py_NewRef(func->func_code);
    frame->builtins = func->func_builtins;
    frame->globals = func->func_globals;
    frame->locals = Py_XNewRef(locals);
    frame->stacktop = nlocalsplus;
    frame->frame_obj = NULL;
    frame->lasti = -1;
    frame->state = FRAME_CREATED;
    frame->is_entry = false;
    frame->is_generator = false;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(_PyInterpreterFrame *frame)
{
    return frame->localsplus;
}

static inline PyObject**
_PyFrame_GetStackPointer(_PyInterpreterFrame *frame)
{
    return frame->localsplus+frame->stacktop;
}

static inline void
_PyFrame_SetStackPointer(_PyInterpreterFrame *frame, PyObject **stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - frame->localsplus);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_PyInterpreterFrame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyInterpreterFrame *fdata)
{
    PyFrameObject *res = fdata->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(fdata);
}

/* Clears all references in the frame.
 * If take is non-zero, then the frame data
 * may be transfered to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_Clear(_PyInterpreterFrame * frame);

int
_PyFrame_Traverse(_PyInterpreterFrame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(_PyInterpreterFrame *frame);

void
_PyFrame_LocalsToFast(_PyInterpreterFrame *frame, int clear);

extern _PyInterpreterFrame *
_PyThreadState_BumpFramePointerSlow(PyThreadState *tstate, size_t size);

static inline _PyInterpreterFrame *
_PyThreadState_BumpFramePointer(PyThreadState *tstate, size_t size)
{
    PyObject **base = tstate->datastack_top;
    if (base) {
        PyObject **top = base + size;
        assert(tstate->datastack_limit);
        if (top < tstate->datastack_limit) {
            tstate->datastack_top = top;
            return (_PyInterpreterFrame *)base;
        }
    }
    return _PyThreadState_BumpFramePointerSlow(tstate, size);
}

void _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

/* Consume reference to func */
_PyInterpreterFrame *
_PyFrame_Push(PyThreadState *tstate, PyFunctionObject *func);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAMEDATA_H */
