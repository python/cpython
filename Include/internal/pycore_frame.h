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

typedef struct _py_frame {
    PyObject *globals;
    PyObject *builtins;
    PyObject *locals;
    PyCodeObject *code;
    PyFrameObject *frame_obj;
    struct _py_frame *previous;
    int lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    PyFrameState f_state;       /* What state the frame is in */
    PyObject *stack[1];
} _PyFrame;

static inline int _PyFrame_IsRunnable(_PyFrame *f) {
    return f->f_state < FRAME_EXECUTING;
}

static inline int _PyFrame_IsExecuting(_PyFrame *f) {
    return f->f_state == FRAME_EXECUTING;
}

static inline int _PyFrameHasCompleted(_PyFrame *f) {
    return f->f_state > FRAME_EXECUTING;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_PyFrame)-1)/sizeof(PyObject *))

int _PyFrame_TakeLocals(PyFrameObject *f);

static inline void
_PyFrame_InitializeSpecials(_PyFrame *frame, PyFrameConstructor *con, PyObject *locals)
{
    frame->code = (PyCodeObject *)Py_NewRef(con->fc_code);
    frame->builtins = Py_NewRef(con->fc_builtins);
    frame->globals = Py_NewRef(con->fc_globals);
    frame->locals = Py_XNewRef(locals);
    frame->frame_obj = NULL;
    frame->lasti = -1;
}

static inline void
_PyFrame_ClearSpecials(_PyFrame *frame)
{
    assert(frame->frame_obj == NULL);
    Py_XDECREF(frame->locals);
    Py_DECREF(frame->globals);
    Py_DECREF(frame->builtins);
    Py_DECREF(frame->code);
}

static inline PyObject**
_PyFrame_GetLocalsArray(_PyFrame *frame)
{
    return ((PyObject **)frame) - frame->code->co_nlocalsplus;
}

/* Returns a borrowed reference */
static inline PyFrameObject *
_PyFrame_GetFrameObject(_PyFrame *frame)
{
    /* Will need to handle lazy frames */
    assert(frame->frame_obj != NULL);
    return frame->frame_obj;
}

int
_PyFrame_FastToLocalsWithError(_PyFrame *frame);

void
_PyFrame_LocalsToFast(_PyFrame *frame, int clear);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAME_H */
