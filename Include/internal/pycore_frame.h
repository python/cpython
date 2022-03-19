#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
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
 *
 * Field naming conventions:
 *
 *   * full frame object fields have an "f_*" prefix
 *   * frame data struct fields have no prefix
 *
 * Naming conventions for local variables, function parameters and fields in other structs:
 *
 *   * "frame", and "f" may refer to either full frame objects or frame data structs
 *     * the field naming convention usually makes the type unambiguous in code reviews
 *   * the following alternative names are used when more clarity is needed:
 *     * full frame objects: "frame_obj" (and variants like "frameobj" or "fobj")
 *     * frame data structs: "fdata"
 *   * the "iframe" name is still used in the generator & coroutine structs. It
 *     comes from a period where frame data structs were called "interpreter frames"
 *     (which implied a larger distinction between full frame objects and their
 *     associated lightweight frame data structs than is actually the case).
 *   * "current frame" should NOT be abbreviated as "cframe", as the latter typically
 *     refers to the C stack frame data that is used to separate Python level recursion
 *     from C level recursive calls to the eval loop function
 *
 * Function/macro parameter types:
 *
 *   * "PyFrame_*" functions and other public C API functions that relate to
 *     frames accept full frame objects
 *   * "_PyFrame_*" functions and other private C API functions that relate to
 *     frames accept either full frame objecst or frame data structs. Check
 *     the specific function signatures for details.
 *
 * Function return types:
 *
 *   * Public C API functions will only ever return full frame objects
 *   * Private C API functions with an underscore prefix may return frame
 *     data structs instead. Check the specific function signatures for details.
 */

struct _frame {
    PyObject_HEAD
    PyFrameObject *f_back;      /* previous frame, or NULL */
    struct _Py_frame *f_frame; /* points to the frame data */
    PyObject *f_trace;          /* Trace function */
    int f_lineno;               /* Current line number. Only valid if non-zero */
    char f_trace_lines;         /* Emit per-line trace events? */
    char f_trace_opcodes;       /* Emit per-opcode trace events? */
    char f_owns_frame;          /* This frame owns the frame */
    /* The frame data, if this frame object owns the frame */
    PyObject *_f_frame_data[1];
};

extern PyFrameObject* _PyFrame_New_NoTrack(PyCodeObject *code);


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

typedef struct _Py_frame {
    PyFunctionObject *func; /* Strong reference */
    PyObject *globals; /* Borrowed reference */
    PyObject *builtins; /* Borrowed reference */
    PyObject *locals; /* Strong reference, may be NULL */
    PyCodeObject *f_code; /* Strong reference */
    PyFrameObject *frame_obj; /* Strong reference, may be NULL */
    struct _Py_frame *previous;
    int f_lasti;       /* Last instruction if called */
    int stacktop;     /* Offset of TOS from localsplus  */
    PyFrameState f_state;  /* What state the frame is in */
    bool is_entry;  // Whether this is the "root" frame for the current _PyCFrame.
    bool is_generator;
    PyObject *localsplus[1];
} _Py_frame;

static inline int _PyFrame_IsRunnable(_Py_frame *f) {
    return f->f_state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(_Py_frame *f) {
    return f->f_state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(_Py_frame *f) {
    return f->f_state > FRAME_EXECUTING;
}

static inline PyObject **_PyFrame_Stackbase(_Py_frame *f) {
    return f->localsplus + f->f_code->co_nlocalsplus;
}

static inline PyObject *_PyFrame_StackPeek(_Py_frame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    assert(f->localsplus[f->stacktop-1] != NULL);
    return f->localsplus[f->stacktop-1];
}

static inline PyObject *_PyFrame_StackPop(_Py_frame *f) {
    assert(f->stacktop > f->f_code->co_nlocalsplus);
    f->stacktop--;
    return f->localsplus[f->stacktop];
}

static inline void _PyFrame_StackPush(_Py_frame *f, PyObject *value) {
    f->localsplus[f->stacktop] = value;
    f->stacktop++;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_Py_frame)-1)/sizeof(PyObject *))

void _PyFrame_Copy(_Py_frame *src, _Py_frame *dest);

/* Consumes reference to func */
static inline void
_PyFrame_InitializeSpecials(
    _Py_frame *frame, PyFunctionObject *func,
    PyObject *locals, int nlocalsplus)
{
    frame->func = func;
    frame->f_code = (PyCodeObject *)Py_NewRef(func->func_code);
    frame->builtins = func->func_builtins;
    frame->globals = func->func_globals;
    frame->locals = Py_XNewRef(locals);
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
_PyFrame_GetLocalsArray(_Py_frame *frame)
{
    return frame->localsplus;
}

static inline PyObject**
_PyFrame_GetStackPointer(_Py_frame *frame)
{
    return frame->localsplus+frame->stacktop;
}

static inline void
_PyFrame_SetStackPointer(_Py_frame *frame, PyObject **stack_pointer)
{
    frame->stacktop = (int)(stack_pointer - frame->localsplus);
}

/* For use by _PyFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyFrame_MakeAndSetFrameObject(_Py_frame *frame);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_Py_frame *frame)
{
    PyFrameObject *res =  frame->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyFrame_MakeAndSetFrameObject(frame);
}

/* Clears all references in the frame.
 * If take is non-zero, then the _Py_frame frame
 * may be transferred to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
void
_PyFrame_Clear(_Py_frame * frame);

int
_PyFrame_Traverse(_Py_frame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(_Py_frame *frame);

void
_PyFrame_LocalsToFast(_Py_frame *frame, int clear);

extern _Py_frame *
_PyThreadState_BumpFramePointerSlow(PyThreadState *tstate, size_t size);

static inline _Py_frame *
_PyThreadState_BumpFramePointer(PyThreadState *tstate, size_t size)
{
    PyObject **base = tstate->datastack_top;
    if (base) {
        PyObject **top = base + size;
        assert(tstate->datastack_limit);
        if (top < tstate->datastack_limit) {
            tstate->datastack_top = top;
            return (_Py_frame *)base;
        }
    }
    return _PyThreadState_BumpFramePointerSlow(tstate, size);
}

void _PyThreadState_PopFrame(PyThreadState *tstate, _Py_frame *frame);

/* Consume reference to func */
_Py_frame *
_PyFrame_Push(PyThreadState *tstate, PyFunctionObject *func);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
