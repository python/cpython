#ifndef Py_CPYTHON_TRACEBACK_H
#  error "this header file must not be included directly"
#endif

typedef struct _traceback PyTracebackObject;

struct _traceback {
    PyObject_HEAD
    PyTracebackObject *tb_next;
    PyFrameObject *tb_frame;
    int tb_lasti;
    int tb_lineno;
};

PyAPI_FUNC(void) PyUnstable_DumpTraceback(int fd, PyThreadState *tstate);

PyAPI_FUNC(const char*) PyUnstable_DumpTracebackThreads(
    int fd,
    PyInterpreterState *interp,
    PyThreadState *current_tstate);
