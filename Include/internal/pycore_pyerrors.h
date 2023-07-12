#ifndef Py_INTERNAL_PYERRORS_H
#define Py_INTERNAL_PYERRORS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* Error handling definitions */

PyAPI_FUNC(_PyErr_StackItem*) _PyErr_GetTopmostException(PyThreadState *tstate);
PyAPI_FUNC(PyObject*) _PyErr_GetHandledException(PyThreadState *);
PyAPI_FUNC(void) _PyErr_SetHandledException(PyThreadState *, PyObject *);
PyAPI_FUNC(void) _PyErr_GetExcInfo(PyThreadState *, PyObject **, PyObject **, PyObject **);

/* Like PyErr_Format(), but saves current exception as __context__ and
   __cause__.
 */
PyAPI_FUNC(PyObject *) _PyErr_FormatFromCause(
    PyObject *exception,
    const char *format,   /* ASCII-encoded string  */
    ...
    );

PyAPI_FUNC(int) _PyException_AddNote(
     PyObject *exc,
     PyObject *note);

PyAPI_FUNC(int) _PyErr_CheckSignals(void);

/* Support for adding program text to SyntaxErrors */

PyAPI_FUNC(PyObject *) _PyErr_ProgramDecodedTextObject(
    PyObject *filename,
    int lineno,
    const char* encoding);

PyAPI_FUNC(PyObject *) _PyUnicodeTranslateError_Create(
    PyObject *object,
    Py_ssize_t start,
    Py_ssize_t end,
    const char *reason          /* UTF-8 encoded string */
    );

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalErrorFormat(
    const char *func,
    const char *format,
    ...);

extern PyObject *_PyErr_SetImportErrorWithNameFrom(
        PyObject *,
        PyObject *,
        PyObject *,
        PyObject *);


/* runtime lifecycle */

extern PyStatus _PyErr_InitTypes(PyInterpreterState *);
extern void _PyErr_FiniTypes(PyInterpreterState *);


/* other API */

static inline PyObject* _PyErr_Occurred(PyThreadState *tstate)
{
    assert(tstate != NULL);
    if (tstate->current_exception == NULL) {
        return NULL;
    }
    return (PyObject *)Py_TYPE(tstate->current_exception);
}

static inline void _PyErr_ClearExcState(_PyErr_StackItem *exc_state)
{
    Py_CLEAR(exc_state->exc_value);
}

PyAPI_FUNC(PyObject*) _PyErr_StackItemToExcInfoTuple(
    _PyErr_StackItem *err_info);

PyAPI_FUNC(void) _PyErr_Fetch(
    PyThreadState *tstate,
    PyObject **type,
    PyObject **value,
    PyObject **traceback);

extern PyObject *
_PyErr_GetRaisedException(PyThreadState *tstate);

PyAPI_FUNC(int) _PyErr_ExceptionMatches(
    PyThreadState *tstate,
    PyObject *exc);

void
_PyErr_SetRaisedException(PyThreadState *tstate, PyObject *exc);

PyAPI_FUNC(void) _PyErr_Restore(
    PyThreadState *tstate,
    PyObject *type,
    PyObject *value,
    PyObject *traceback);

PyAPI_FUNC(void) _PyErr_SetObject(
    PyThreadState *tstate,
    PyObject *type,
    PyObject *value);

PyAPI_FUNC(void) _PyErr_ChainStackItem(void);

PyAPI_FUNC(void) _PyErr_Clear(PyThreadState *tstate);

PyAPI_FUNC(void) _PyErr_SetNone(PyThreadState *tstate, PyObject *exception);

PyAPI_FUNC(PyObject *) _PyErr_NoMemory(PyThreadState *tstate);

PyAPI_FUNC(void) _PyErr_SetString(
    PyThreadState *tstate,
    PyObject *exception,
    const char *string);

PyAPI_FUNC(PyObject *) _PyErr_Format(
    PyThreadState *tstate,
    PyObject *exception,
    const char *format,
    ...);

PyAPI_FUNC(void) _PyErr_NormalizeException(
    PyThreadState *tstate,
    PyObject **exc,
    PyObject **val,
    PyObject **tb);

PyAPI_FUNC(PyObject *) _PyErr_FormatFromCauseTstate(
    PyThreadState *tstate,
    PyObject *exception,
    const char *format,
    ...);

PyAPI_FUNC(PyObject *) _PyExc_CreateExceptionGroup(
    const char *msg,
    PyObject *excs);

PyAPI_FUNC(PyObject *) _PyExc_PrepReraiseStar(
    PyObject *orig,
    PyObject *excs);

PyAPI_FUNC(int) _PyErr_CheckSignalsTstate(PyThreadState *tstate);

PyAPI_FUNC(void) _Py_DumpExtensionModules(int fd, PyInterpreterState *interp);

extern PyObject* _Py_Offer_Suggestions(PyObject* exception);
PyAPI_FUNC(Py_ssize_t) _Py_UTF8_Edit_Cost(PyObject *str_a, PyObject *str_b,
                                          Py_ssize_t max_cost);

void _PyErr_FormatNote(const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYERRORS_H */
