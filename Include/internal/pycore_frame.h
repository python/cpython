#ifndef Py_INTERNAL_FRAMEDATA_H
#define Py_INTERNAL_FRAMEDATA_H
#ifdef __cplusplus
extern "C" {
#endif

/* Internal-use-only frame object constructor */
PyFrameObject*
_PyFrame_New_NoTrack(_PyInterpreterFrame *, int);

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

// The _PyInterpreterFrame typedef is in Include/pyframeobject.h
struct _Py_execution_frame {
    PyObject *globals;
    PyObject *builtins;
    PyObject *locals;
    PyCodeObject *code;
    PyFrameObject *frame_obj; // Full frame object (if created)
    /* Borrowed reference to a generator, or NULL */
    PyObject *generator;
    _PyInterpreterFrame *previous;
    int lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    int nlocalsplus;
    PyFrameState state;       /* What state the frame is in */
    PyObject *stack[1];
};

static inline int _PyInterpreterFrame_IsRunnable(_PyInterpreterFrame *fdata) {
    return fdata->state < FRAME_EXECUTING;
}

static inline int _PyInterpreterFrame_IsExecuting(_PyInterpreterFrame *fdata) {
    return fdata->state == FRAME_EXECUTING;
}

static inline int _PyInterpreterFrame_HasCompleted(_PyInterpreterFrame *fdata) {
    return fdata->state > FRAME_EXECUTING;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_PyInterpreterFrame)-1)/sizeof(PyObject *))

_PyInterpreterFrame *
_PyInterpreterFrame_HeapAlloc(PyFrameConstructor *con, PyObject *locals);

static inline void
_PyInterpreterFrame_InitializeSpecials(
    _PyInterpreterFrame *f, PyFrameConstructor *con,
    PyObject *locals, int nlocalsplus)
{
    fdata->code = (PyCodeObject *)Py_NewRef(con->fc_code);
    fdata->builtins = Py_NewRef(con->fc_builtins);
    fdata->globals = Py_NewRef(con->fc_globals);
    fdata->locals = Py_XNewRef(locals);
    fdata->nlocalsplus = nlocalsplus;
    fdata->stackdepth = 0;
    fdata->frame_obj = NULL;
    fdata->generator = NULL;
    fdata->lasti = -1;
    fdata->state = FRAME_CREATED;
}

/* Gets the pointer to the locals array
 * that precedes this frame.
 */
static inline PyObject**
_PyInterpreterFrame_GetLocalsArray(_PyInterpreterFrame *fdata)
{
    return ((PyObject **)fdata) - fdata->nlocalsplus;
}

/* For use by _PyInterpreterFrame_GetFrameObject
  Do not call directly. */
PyFrameObject *
_PyInterpreterFrame_MakeAndSetFrameObject(_PyInterpreterFrame *fdata);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_PyInterpreterFrame_GetFrameObject(_PyInterpreterFrame *fdata)
{
    PyFrameObject *res = fdata->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _PyInterpreterFrame_MakeAndSetFrameObject(fdata);
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
int
_PyInterpreterFrame_Clear(_PyInterpreterFrame *f, int take);

int
_PyInterpreterFrame_Traverse(_PyInterpreterFrame *f, visitproc visit, void *arg);

int
_PyInterpreterFrame_FastToLocalsWithError(_PyInterpreterFrame *frame);

void
_PyInterpreterFrame_LocalsToFast(_PyInterpreterFrame *frame, int clear);

_PyInterpreterFrame *_PyThreadState_PushFrame(
    PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals);

void _PyThreadState_PopFrame(PyThreadState *tstate, _PyInterpreterFrame *frame);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAMEDATA_H */
