#ifndef Py_ERRORS_H
#define Py_ERRORS_H
#ifdef __cplusplus
extern "C" {
#endif


/* Error handling definitions */

PyAPI_FUNC(void) PyErr_SetNone(PyObject *);
PyAPI_FUNC(void) PyErr_SetObject(PyObject *, PyObject *);
PyAPI_FUNC(void) PyErr_SetString(PyObject *, const char *);
PyAPI_FUNC(PyObject *) PyErr_Occurred(void);
PyAPI_FUNC(void) PyErr_Clear(void);
PyAPI_FUNC(void) PyErr_Fetch(PyObject **, PyObject **, PyObject **);
PyAPI_FUNC(void) PyErr_Restore(PyObject *, PyObject *, PyObject *);

/* Error testing and normalization */
PyAPI_FUNC(int) PyErr_GivenExceptionMatches(PyObject *, PyObject *);
PyAPI_FUNC(int) PyErr_ExceptionMatches(PyObject *);
PyAPI_FUNC(void) PyErr_NormalizeException(PyObject**, PyObject**, PyObject**);


/* Predefined exceptions */

PyAPI_DATA(PyObject *) PyExc_Exception;
PyAPI_DATA(PyObject *) PyExc_StopIteration;
PyAPI_DATA(PyObject *) PyExc_StandardError;
PyAPI_DATA(PyObject *) PyExc_ArithmeticError;
PyAPI_DATA(PyObject *) PyExc_LookupError;

PyAPI_DATA(PyObject *) PyExc_AssertionError;
PyAPI_DATA(PyObject *) PyExc_AttributeError;
PyAPI_DATA(PyObject *) PyExc_EOFError;
PyAPI_DATA(PyObject *) PyExc_FloatingPointError;
PyAPI_DATA(PyObject *) PyExc_EnvironmentError;
PyAPI_DATA(PyObject *) PyExc_IOError;
PyAPI_DATA(PyObject *) PyExc_OSError;
PyAPI_DATA(PyObject *) PyExc_ImportError;
PyAPI_DATA(PyObject *) PyExc_IndexError;
PyAPI_DATA(PyObject *) PyExc_KeyError;
PyAPI_DATA(PyObject *) PyExc_KeyboardInterrupt;
PyAPI_DATA(PyObject *) PyExc_MemoryError;
PyAPI_DATA(PyObject *) PyExc_NameError;
PyAPI_DATA(PyObject *) PyExc_OverflowError;
PyAPI_DATA(PyObject *) PyExc_RuntimeError;
PyAPI_DATA(PyObject *) PyExc_NotImplementedError;
PyAPI_DATA(PyObject *) PyExc_SyntaxError;
PyAPI_DATA(PyObject *) PyExc_IndentationError;
PyAPI_DATA(PyObject *) PyExc_TabError;
PyAPI_DATA(PyObject *) PyExc_ReferenceError;
PyAPI_DATA(PyObject *) PyExc_SystemError;
PyAPI_DATA(PyObject *) PyExc_SystemExit;
PyAPI_DATA(PyObject *) PyExc_TypeError;
PyAPI_DATA(PyObject *) PyExc_UnboundLocalError;
PyAPI_DATA(PyObject *) PyExc_UnicodeError;
PyAPI_DATA(PyObject *) PyExc_ValueError;
PyAPI_DATA(PyObject *) PyExc_ZeroDivisionError;
#ifdef MS_WINDOWS
PyAPI_DATA(PyObject *) PyExc_WindowsError;
#endif

PyAPI_DATA(PyObject *) PyExc_MemoryErrorInst;

/* Predefined warning categories */
PyAPI_DATA(PyObject *) PyExc_Warning;
PyAPI_DATA(PyObject *) PyExc_UserWarning;
PyAPI_DATA(PyObject *) PyExc_DeprecationWarning;
PyAPI_DATA(PyObject *) PyExc_PendingDeprecationWarning;
PyAPI_DATA(PyObject *) PyExc_SyntaxWarning;
PyAPI_DATA(PyObject *) PyExc_OverflowWarning;
PyAPI_DATA(PyObject *) PyExc_RuntimeWarning;
PyAPI_DATA(PyObject *) PyExc_FutureWarning;


/* Convenience functions */

PyAPI_FUNC(int) PyErr_BadArgument(void);
PyAPI_FUNC(PyObject *) PyErr_NoMemory(void);
PyAPI_FUNC(PyObject *) PyErr_SetFromErrno(PyObject *);
PyAPI_FUNC(PyObject *) PyErr_SetFromErrnoWithFilename(PyObject *, char *);
PyAPI_FUNC(PyObject *) PyErr_Format(PyObject *, const char *, ...)
			__attribute__((format(printf, 2, 3)));
#ifdef MS_WINDOWS
PyAPI_FUNC(PyObject *) PyErr_SetFromWindowsErrWithFilename(int, const char *);
PyAPI_FUNC(PyObject *) PyErr_SetFromWindowsErr(int);
PyAPI_FUNC(PyObject *) PyErr_SetExcFromWindowsErrWithFilename(
	PyObject *,int, const char *);
PyAPI_FUNC(PyObject *) PyErr_SetExcFromWindowsErr(PyObject *, int);
#endif

/* Export the old function so that the existing API remains available: */
PyAPI_FUNC(void) PyErr_BadInternalCall(void);
PyAPI_FUNC(void) _PyErr_BadInternalCall(char *filename, int lineno);
/* Mask the old API with a call to the new API for code compiled under
   Python 2.0: */
#define PyErr_BadInternalCall() _PyErr_BadInternalCall(__FILE__, __LINE__)

/* Function to create a new exception */
PyAPI_FUNC(PyObject *) PyErr_NewException(char *name, PyObject *base,
                                         PyObject *dict);
PyAPI_FUNC(void) PyErr_WriteUnraisable(PyObject *);

/* Issue a warning or exception */
PyAPI_FUNC(int) PyErr_Warn(PyObject *, char *);
PyAPI_FUNC(int) PyErr_WarnExplicit(PyObject *, char *,
					 char *, int, char *, PyObject *);

/* In sigcheck.c or signalmodule.c */
PyAPI_FUNC(int) PyErr_CheckSignals(void);
PyAPI_FUNC(void) PyErr_SetInterrupt(void);

/* Support for adding program text to SyntaxErrors */
PyAPI_FUNC(void) PyErr_SyntaxLocation(char *, int);
PyAPI_FUNC(PyObject *) PyErr_ProgramText(char *, int);

/* These APIs aren't really part of the error implementation, but
   often needed to format error messages; the native C lib APIs are
   not available on all platforms, which is why we provide emulations
   for those platforms in Python/mysnprintf.c,
   WARNING:  The return value of snprintf varies across platforms; do
   not rely on any particular behavior; eventually the C99 defn may
   be reliable.
*/
#if defined(MS_WIN32) && !defined(HAVE_SNPRINTF)
# define HAVE_SNPRINTF
# define snprintf _snprintf
# define vsnprintf _vsnprintf
#endif

#include <stdarg.h>
PyAPI_FUNC(int) PyOS_snprintf(char *str, size_t size, const char  *format, ...)
			__attribute__((format(printf, 3, 4)));
PyAPI_FUNC(int) PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va)
			__attribute__((format(printf, 3, 0)));

#ifdef __cplusplus
}
#endif
#endif /* !Py_ERRORS_H */
