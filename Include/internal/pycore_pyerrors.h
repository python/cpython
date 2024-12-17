#ifndef Py_INTERNAL_PYERRORS_H
#define Py_INTERNAL_PYERRORS_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif


/* Error handling definitions */

extern _PyErr_StackItem* _PyErr_GetTopmostException(PyThreadState *tstate);
extern PyObject* _PyErr_GetHandledException(PyThreadState *);
extern void _PyErr_SetHandledException(PyThreadState *, PyObject *);
extern void _PyErr_GetExcInfo(PyThreadState *, PyObject **, PyObject **, PyObject **);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(void) _PyErr_SetKeyError(PyObject *);


// Like PyErr_Format(), but saves current exception as __context__ and
// __cause__.
// Export for '_sqlite3' shared extension.
PyAPI_FUNC(PyObject*) _PyErr_FormatFromCause(
    PyObject *exception,
    const char *format,   /* ASCII-encoded string  */
    ...
    );

extern int _PyException_AddNote(
     PyObject *exc,
     PyObject *note);

extern int _PyErr_CheckSignals(void);

/* Support for adding program text to SyntaxErrors */

// Export for test_peg_generator
PyAPI_FUNC(PyObject*) _PyErr_ProgramDecodedTextObject(
    PyObject *filename,
    int lineno,
    const char* encoding);

extern PyObject* _PyUnicodeTranslateError_Create(
    PyObject *object,
    Py_ssize_t start,
    Py_ssize_t end,
    const char *reason          /* UTF-8 encoded string */
    );

extern void _Py_NO_RETURN _Py_FatalErrorFormat(
    const char *func,
    const char *format,
    ...);

extern PyObject* _PyErr_SetImportErrorWithNameFrom(
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

extern PyObject* _PyErr_StackItemToExcInfoTuple(
    _PyErr_StackItem *err_info);

extern void _PyErr_Fetch(
    PyThreadState *tstate,
    PyObject **type,
    PyObject **value,
    PyObject **traceback);

extern PyObject* _PyErr_GetRaisedException(PyThreadState *tstate);

PyAPI_FUNC(int) _PyErr_ExceptionMatches(
    PyThreadState *tstate,
    PyObject *exc);

extern void _PyErr_SetRaisedException(PyThreadState *tstate, PyObject *exc);

extern void _PyErr_Restore(
    PyThreadState *tstate,
    PyObject *type,
    PyObject *value,
    PyObject *traceback);

extern void _PyErr_SetObject(
    PyThreadState *tstate,
    PyObject *type,
    PyObject *value);

extern void _PyErr_ChainStackItem(void);

PyAPI_FUNC(void) _PyErr_Clear(PyThreadState *tstate);

extern void _PyErr_SetNone(PyThreadState *tstate, PyObject *exception);

extern PyObject* _PyErr_NoMemory(PyThreadState *tstate);

PyAPI_FUNC(void) _PyErr_SetString(
    PyThreadState *tstate,
    PyObject *exception,
    const char *string);

/*
 * Set an exception with the error message decoded from the current locale
 * encoding (LC_CTYPE).
 *
 * Exceptions occurring in decoding take priority over the desired exception.
 *
 * Exported for '_ctypes' shared extensions.
 */
PyAPI_FUNC(void) _PyErr_SetLocaleString(
    PyObject *exception,
    const char *string);

PyAPI_FUNC(PyObject*) _PyErr_Format(
    PyThreadState *tstate,
    PyObject *exception,
    const char *format,
    ...);

extern void _PyErr_NormalizeException(
    PyThreadState *tstate,
    PyObject **exc,
    PyObject **val,
    PyObject **tb);

extern PyObject* _PyErr_FormatFromCauseTstate(
    PyThreadState *tstate,
    PyObject *exception,
    const char *format,
    ...);

extern PyObject* _PyExc_CreateExceptionGroup(
    const char *msg,
    PyObject *excs);

extern PyObject* _PyExc_PrepReraiseStar(
    PyObject *orig,
    PyObject *excs);

extern int _PyErr_CheckSignalsTstate(PyThreadState *tstate);

extern void _Py_DumpExtensionModules(int fd, PyInterpreterState *interp);
extern PyObject* _Py_CalculateSuggestions(PyObject *dir, PyObject *name);
extern PyObject* _Py_Offer_Suggestions(PyObject* exception);

// Export for '_testinternalcapi' shared extension
PyAPI_FUNC(Py_ssize_t) _Py_UTF8_Edit_Cost(PyObject *str_a, PyObject *str_b,
                                          Py_ssize_t max_cost);

void _PyErr_FormatNote(const char *format, ...);

/* Context manipulation (PEP 3134) */

Py_DEPRECATED(3.12) extern void _PyErr_ChainExceptions(PyObject *, PyObject *, PyObject *);

// implementation detail for the codeop module.
// Exported for test.test_peg_generator.test_c_parser
PyAPI_DATA(PyTypeObject) _PyExc_IncompleteInputError;
#define PyExc_IncompleteInputError ((PyObject *)(&_PyExc_IncompleteInputError))

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_PYERRORS_H */
