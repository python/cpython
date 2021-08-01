#ifndef Py_INTERNAL_XFRAME_H
#define Py_INTERNAL_XFRAME_H
#ifdef __cplusplus
extern "C" {
#endif

/* Internal-use-only introspection frame constructor */
PyFrameObject*
_PyFrame_New_NoTrack(_PyExecFrame *, int);

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

// The _PyExecFrame typedef is in Include/pyframeobject.h
struct _Py_execution_frame {
    PyObject *xf_globals;
    PyObject *xf_builtins;
    PyObject *xf_locals;
    PyCodeObject *xf_code;
    PyFrameObject *xf_frame_obj; // Introspection frame (if created)
    /* Borrowed reference to a generator, or NULL */
    PyObject *xf_generator;
    _PyExecFrame *xf_previous;
    int xf_lasti;       /* Last instruction if called */
    int xf_stackdepth;  /* Depth of value stack */
    int xf_nlocalsplus;
    PyFrameState xf_state;       /* What state the frame is in */
    PyObject *xf_stack[1];
};

static inline int _PyExecFrame_IsRunnable(_PyExecFrame *xf) {
    return xf->xf_state < FRAME_EXECUTING;
}

static inline int _PyExecFrame_IsExecuting(_PyExecFrame *xf) {
    return xf->xf_state == FRAME_EXECUTING;
}

static inline int _PyExecFrame_HasCompleted(_PyExecFrame *xf) {
    return xf->xf_state > FRAME_EXECUTING;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_PyExecFrame)-1)/sizeof(PyObject *))

_PyExecFrame *
_PyExecFrame_HeapAlloc(PyFrameConstructor *con, PyObject *locals);

static inline void
_PyExecFrame_InitializeSpecials(
    _PyExecFrame *xframe, PyFrameConstructor *con,
    PyObject *locals, int nlocalsplus)
{
    xframe->xf_code = (PyCodeObject *)Py_NewRef(con->fc_code);
    xframe->xf_builtins = Py_NewRef(con->fc_builtins);
    xframe->xf_globals = Py_NewRef(con->fc_globals);
    xframe->xf_locals = Py_XNewRef(locals);
    xframe->xf_nlocalsplus = nlocalsplus;
    xframe->xf_stackdepth = 0;
    xframe->xf_frame_obj = NULL;
    xframe->xf_generator = NULL;
    xframe->xf_lasti = -1;
    xframe->xf_state = FRAME_CREATED;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyExecFrame_GetLocalsArray(_PyExecFrame *xframe)
{
    return ((PyObject **)xframe) - xframe->xf_nlocalsplus;
}

/* For use by _PyExecFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyExecFrame_MakeAndSetFrameObject(_PyExecFrame *xframe);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyExecFrame_GetFrameObject(_PyExecFrame *xframe)
{
    PyFrameObject *res = xframe->xf_frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyExecFrame_MakeAndSetFrameObject(xframe);
}

/* Clears all references in the frame.
 * If take is non-zero, then the execution frame
 * may be transfered to the frame object it references
 * instead of being cleared. Either way
 * the caller no longer owns the references
 * in the frame.
 * take should  be set to 1 for heap allocated
 * frames like the ones in generators and coroutines.
 */
int
_PyExecFrame_Clear(_PyExecFrame *xframe, int take);

int
_PyExecFrame_Traverse(_PyExecFrame *xframe, visitproc visit, void *arg);

int
_PyExecFrame_FastToLocalsWithError(_PyExecFrame *frame);

void
_PyExecFrame_LocalsToFast(_PyExecFrame *frame, int clear);

_PyExecFrame *_PyThreadState_PushExecFrame(
    PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals);

void _PyThreadState_PopExecFrame(PyThreadState *tstate, _PyExecFrame *frame);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_XFRAME_H */
