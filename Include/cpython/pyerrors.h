#ifndef Py_CPYTHON_ERRORS_H
#  error "this header file must not be included directly"
#endif

/* Error objects */

/* PyException_HEAD defines the initial segment of every exception class. */
#define PyException_HEAD PyObject_HEAD PyObject *dict;\
             PyObject *args; PyObject *notes; PyObject *traceback;\
             PyObject *context; PyObject *cause;\
             char suppress_context;

typedef struct {
    PyException_HEAD
} PyBaseExceptionObject;

typedef struct {
    PyException_HEAD
    PyObject *msg;
    PyObject *excs;
} PyBaseExceptionGroupObject;

typedef struct {
    PyException_HEAD
    PyObject *msg;
    PyObject *filename;
    PyObject *lineno;
    PyObject *offset;
    PyObject *end_lineno;
    PyObject *end_offset;
    PyObject *text;
    PyObject *print_file_and_line;
} PySyntaxErrorObject;

typedef struct {
    PyException_HEAD
    PyObject *msg;
    PyObject *name;
    PyObject *path;
    PyObject *name_from;
} PyImportErrorObject;

typedef struct {
    PyException_HEAD
    PyObject *encoding;
    PyObject *object;
    Py_ssize_t start;
    Py_ssize_t end;
    PyObject *reason;
} PyUnicodeErrorObject;

typedef struct {
    PyException_HEAD
    PyObject *code;
} PySystemExitObject;

typedef struct {
    PyException_HEAD
    PyObject *myerrno;
    PyObject *strerror;
    PyObject *filename;
    PyObject *filename2;
#ifdef MS_WINDOWS
    PyObject *winerror;
#endif
    Py_ssize_t written;   /* only for BlockingIOError, -1 otherwise */
} PyOSErrorObject;

typedef struct {
    PyException_HEAD
    PyObject *value;
} PyStopIterationObject;

typedef struct {
    PyException_HEAD
    PyObject *name;
} PyNameErrorObject;

typedef struct {
    PyException_HEAD
    PyObject *obj;
    PyObject *name;
} PyAttributeErrorObject;

/* Compatibility typedefs */
typedef PyOSErrorObject PyEnvironmentErrorObject;
#ifdef MS_WINDOWS
typedef PyOSErrorObject PyWindowsErrorObject;
#endif

/* Error handling definitions */

PyAPI_FUNC(void) _PyErr_SetKeyError(PyObject *);
PyAPI_FUNC(_PyErr_StackItem*) _PyErr_GetTopmostException(PyThreadState *tstate);
PyAPI_FUNC(PyObject*) _PyErr_GetHandledException(PyThreadState *);
PyAPI_FUNC(void) _PyErr_SetHandledException(PyThreadState *, PyObject *);
PyAPI_FUNC(void) _PyErr_GetExcInfo(PyThreadState *, PyObject **, PyObject **, PyObject **);

/* Context manipulation (PEP 3134) */

Py_DEPRECATED(3.12) PyAPI_FUNC(void) _PyErr_ChainExceptions(PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(void) _PyErr_ChainExceptions1(PyObject *);

/* Like PyErr_Format(), but saves current exception as __context__ and
   __cause__.
 */
PyAPI_FUNC(PyObject *) _PyErr_FormatFromCause(
    PyObject *exception,
    const char *format,   /* ASCII-encoded string  */
    ...
    );

/* In exceptions.c */

PyAPI_FUNC(int) _PyException_AddNote(
     PyObject *exc,
     PyObject *note);

PyAPI_FUNC(PyObject*) PyUnstable_Exc_PrepReraiseStar(
     PyObject *orig,
     PyObject *excs);

/* In signalmodule.c */

int PySignal_SetWakeupFd(int fd);
PyAPI_FUNC(int) _PyErr_CheckSignals(void);

/* Support for adding program text to SyntaxErrors */

PyAPI_FUNC(void) PyErr_SyntaxLocationObject(
    PyObject *filename,
    int lineno,
    int col_offset);

PyAPI_FUNC(void) PyErr_RangedSyntaxLocationObject(
    PyObject *filename,
    int lineno,
    int col_offset,
    int end_lineno,
    int end_col_offset);

PyAPI_FUNC(PyObject *) PyErr_ProgramTextObject(
    PyObject *filename,
    int lineno);

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

PyAPI_FUNC(void) _PyErr_WriteUnraisableMsg(
    const char *err_msg,
    PyObject *obj);

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalErrorFunc(
    const char *func,
    const char *message);

PyAPI_FUNC(void) _Py_NO_RETURN _Py_FatalErrorFormat(
    const char *func,
    const char *format,
    ...);

extern PyObject *_PyErr_SetImportErrorWithNameFrom(
        PyObject *,
        PyObject *,
        PyObject *,
        PyObject *);


#define Py_FatalError(message) _Py_FatalErrorFunc(__func__, (message))
