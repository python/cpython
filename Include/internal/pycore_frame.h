#ifndef Py_INTERNAL_FRAME_H
#define Py_INTERNAL_FRAME_H
#ifdef __cplusplus
extern "C" {
#endif

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

typedef struct _interpreter_frame {
    PyObject *f_globals;
    PyObject *f_builtins;
    PyObject *f_locals;
    PyCodeObject *f_code;
    PyFrameObject *frame_obj;
    /* Borrowed reference to a generator, or NULL */
    PyObject *generator;
    struct _interpreter_frame *previous;
    int f_lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    int nlocalsplus;
    PyFrameState f_state;       /* What state the frame is in */
    PyObject *stack[1];
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

#define FRAME_SPECIALS_SIZE ((sizeof(InterpreterFrame)-1)/sizeof(PyObject *))

InterpreterFrame *
_PyInterpreterFrame_HeapAlloc(PyFrameConstructor *con, PyObject *locals);

static inline void
_PyFrame_InitializeSpecials(
    InterpreterFrame *frame, PyFrameConstructor *con,
    PyObject *locals, int nlocalsplus)
{
    frame->f_code = (PyCodeObject *)Py_NewRef(con->fc_code);
    frame->f_builtins = Py_NewRef(con->fc_builtins);
    frame->f_globals = Py_NewRef(con->fc_globals);
    frame->f_locals = Py_XNewRef(locals);
    frame->nlocalsplus = nlocalsplus;
    frame->stackdepth = 0;
    frame->frame_obj = NULL;
    frame->generator = NULL;
    frame->f_lasti = -1;
    frame->f_state = FRAME_CREATED;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyFrame_GetLocalsArray(InterpreterFrame *frame)
{
    return ((PyObject **)frame) - frame->nlocalsplus;
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
 * may be transfered to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
int
_PyFrame_Clear(InterpreterFrame * frame, int take);

int
_PyFrame_Traverse(InterpreterFrame *frame, visitproc visit, void *arg);

int
_PyFrame_FastToLocalsWithError(InterpreterFrame *frame);

void
_PyFrame_LocalsToFast(InterpreterFrame *frame, int clear);

InterpreterFrame *_PyThreadState_PushFrame(
    PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals);

void _PyThreadState_PopFrame(PyThreadState *tstate, InterpreterFrame *frame);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
