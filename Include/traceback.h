
#ifndef Py_TRACEBACK_H
#define Py_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#include "pystate.h"

struct _frame;

/* Traceback interface */
#ifndef Py_LIMITED_API
typedef struct _traceback {
    PyObject_HEAD
    struct _traceback *tb_next;
    struct _frame *tb_frame;
    int tb_lasti;
    int tb_lineno;
} PyTracebackObject;
#endif

PyAPI_FUNC(int) PyTraceBack_Here(struct _frame *);
PyAPI_FUNC(int) PyTraceBack_Print(PyObject *, PyObject *);
#ifndef Py_LIMITED_API
PyAPI_FUNC(int) _Py_DisplaySourceLine(PyObject *, PyObject *, int, int);
PyAPI_FUNC(void) _PyTraceback_Add(const char *, const char *, int);
#endif

/* Reveal traceback type so we can typecheck traceback objects */
PyAPI_DATA(PyTypeObject) PyTraceBack_Type;
#define PyTraceBack_Check(v) (Py_TYPE(v) == &PyTraceBack_Type)

#ifndef Py_LIMITED_API
/* Write the Python traceback into the file 'fd'. For example:

       Traceback (most recent call first):
         File "xxx", line xxx in <xxx>
         File "xxx", line xxx in <xxx>
         ...
         File "xxx", line xxx in <xxx>

   This function is written for debug purpose only, to dump the traceback in
   the worst case: after a segmentation fault, at fatal error, etc. That's why,
   it is very limited. Strings are truncated to 100 characters and encoded to
   ASCII with backslashreplace. It doesn't write the source code, only the
   function name, filename and line number of each frame. Write only the first
   100 frames: if the traceback is truncated, write the line " ...".

   This function is signal safe. */

PyAPI_FUNC(void) _Py_DumpTraceback(
    int fd,
    PyThreadState *tstate);

/* Write the traceback of all threads into the file 'fd'. current_thread can be
   NULL.

   Return NULL on success, or an error message on error.

   This function is written for debug purpose only. It calls
   _Py_DumpTraceback() for each thread, and so has the same limitations. It
   only write the traceback of the first 100 threads: write "..." if there are
   more threads.

   If current_tstate is NULL, the function tries to get the Python thread state
   of the current thread. It is not an error if the function is unable to get
   the current Python thread state.

   If interp is NULL, the function tries to get the interpreter state from
   the current Python thread state, or from
   _PyGILState_GetInterpreterStateUnsafe() in last resort.

   It is better to pass NULL to interp and current_tstate, the function tries
   different options to retrieve these informations.

   This function is signal safe. */

PyAPI_FUNC(const char*) _Py_DumpTracebackThreads(
    int fd,
    PyInterpreterState *interp,
    PyThreadState *current_tstate);
#endif /* !Py_LIMITED_API */

#ifndef Py_LIMITED_API

/* Write a Unicode object into the file descriptor fd. Encode the string to
   ASCII using the backslashreplace error handler.

   Do nothing if text is not a Unicode object. The function accepts Unicode
   string which is not ready (PyUnicode_WCHAR_KIND).

   This function is signal safe. */
PyAPI_FUNC(void) _Py_DumpASCII(int fd, PyObject *text);

/* Format an integer as decimal into the file descriptor fd.

   This function is signal safe. */
PyAPI_FUNC(void) _Py_DumpDecimal(
    int fd,
    unsigned long value);

/* Format an integer as hexadecimal into the file descriptor fd with at least
   width digits.

   The maximum width is sizeof(unsigned long)*2 digits.

   This function is signal safe. */
PyAPI_FUNC(void) _Py_DumpHexadecimal(
    int fd,
    unsigned long value,
    Py_ssize_t width);

#endif   /* !Py_LIMITED_API */

#ifdef __cplusplus
}
#endif
#endif /* !Py_TRACEBACK_H */
