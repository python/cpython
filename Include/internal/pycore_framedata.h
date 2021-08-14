#ifndef Py_INTERNAL_FRAMEDATA_H
#define Py_INTERNAL_FRAMEDATA_H
#ifdef __cplusplus
extern "C" {
#endif

/* Internal-use-only frame object constructor */
PyFrameObject*
_PyFrame_New_NoTrack(_Py_framedata *, int);

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

// The _Py_framedata typedef is in Include/pyframeobject.h
struct _Py_execution_frame {
    PyObject *globals;
    PyObject *builtins;
    PyObject *locals;
    PyCodeObject *code;
    PyFrameObject *frame_obj; // Full frame object (if created)
    /* Borrowed reference to a generator, or NULL */
    PyObject *generator;
    _Py_framedata *previous;
    int lasti;       /* Last instruction if called */
    int stackdepth;  /* Depth of value stack */
    int nlocalsplus;
    PyFrameState state;       /* What state the frame is in */
    PyObject *stack[1];
};

static inline int _Py_framedata_IsRunnable(_Py_framedata *fdata) {
    return fdata->state < FRAME_EXECUTING;
}

static inline int _Py_framedata_IsExecuting(_Py_framedata *fdata) {
    return fdata->state == FRAME_EXECUTING;
}

static inline int _Py_framedata_HasCompleted(_Py_framedata *fdata) {
    return fdata->state > FRAME_EXECUTING;
}

#define FRAME_SPECIALS_SIZE ((sizeof(_Py_framedata)-1)/sizeof(PyObject *))

_Py_framedata *
_Py_framedata_HeapAlloc(PyFrameConstructor *con, PyObject *locals);

static inline void
_Py_framedata_InitializeSpecials(
    _Py_framedata *fdata, PyFrameConstructor *con,
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
_Py_framedata_GetLocalsArray(_Py_framedata *fdata)
{
    return ((PyObject **)fdata) - fdata->nlocalsplus;
}

/* For use by _Py_framedata_GetFrameObject
  Do not call directly. */
PyFrameObject *
_Py_framedata_MakeAndSetFrameObject(_Py_framedata *fdata);

/* Gets the PyFrameObject for this frame, lazily
 * creating it if necessary.
 * Returns a borrowed referennce */
static inline PyFrameObject *
_Py_framedata_GetFrameObject(_Py_framedata *fdata)
{
    PyFrameObject *res = fdata->frame_obj;
    if (res != NULL) {
        return res;
    }
    return _Py_framedata_MakeAndSetFrameObject(fdata);
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
_Py_framedata_Clear(_Py_framedata *fdata, int take);

int
_Py_framedata_Traverse(_Py_framedata *fdata, visitproc visit, void *arg);

int
_Py_framedata_FastToLocalsWithError(_Py_framedata *frame);

void
_Py_framedata_LocalsToFast(_Py_framedata *frame, int clear);

_Py_framedata *_PyThreadState_Push_framedata(
    PyThreadState *tstate, PyFrameConstructor *con, PyObject *locals);

void _PyThreadState_Pop_framedata(PyThreadState *tstate, _Py_framedata *frame);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_FRAMEDATA_H */
