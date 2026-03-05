#ifndef Py_INTERNAL_TRACEBACK_H
#define Py_INTERNAL_TRACEBACK_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

// Export for '_ctypes' shared extension
PyAPI_FUNC(int) _Py_DisplaySourceLine(PyObject *, PyObject *, int, int, int *, PyObject **);

// Export for 'pyexact' shared extension
PyAPI_FUNC(void) _PyTraceback_Add(const char *, const char *, int);

/* Write a Unicode object into the file descriptor fd. Encode the string to
   ASCII using the backslashreplace error handler.

   Do nothing if text is not a Unicode object.

   This function is signal safe. */
extern void _Py_DumpASCII(int fd, PyObject *text);

/* Format an integer as decimal into the file descriptor fd.

   This function is signal safe. */
extern void _Py_DumpDecimal(
    int fd,
    size_t value);

/* Format an integer as hexadecimal with width digits into fd file descriptor.
   The function is signal safe. */
extern void _Py_DumpHexadecimal(
    int fd,
    uintptr_t value,
    Py_ssize_t width);

extern PyObject* _PyTraceBack_FromFrame(
    PyObject *tb_next,
    PyFrameObject *frame);

#define EXCEPTION_TB_HEADER "Traceback (most recent call last):\n"
#define EXCEPTION_GROUP_TB_HEADER "Exception Group Traceback (most recent call last):\n"

/* Write the traceback tb to file f. Prefix each line with
   indent spaces followed by the margin (if it is not NULL). */
extern int _PyTraceBack_Print(
    PyObject *tb, const char *header, PyObject *f);
extern int _Py_WriteIndentedMargin(int, const char*, PyObject *);
extern int _Py_WriteIndent(int, PyObject *);

// Export for the faulthandler module
PyAPI_FUNC(void) _Py_InitDumpStack(void);
PyAPI_FUNC(void) _Py_DumpStack(int fd);

extern void _Py_DumpTraceback_Init(void);

#ifdef __cplusplus
}
#endif
#endif /* !Py_INTERNAL_TRACEBACK_H */
