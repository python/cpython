#ifndef Py_ERRORS_H
#define Py_ERRORS_H
#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Error handling definitions */

DL_IMPORT(void) PyErr_SetNone Py_PROTO((PyObject *));
DL_IMPORT(void) PyErr_SetObject Py_PROTO((PyObject *, PyObject *));
DL_IMPORT(void) PyErr_SetString Py_PROTO((PyObject *, const char *));
DL_IMPORT(PyObject *) PyErr_Occurred Py_PROTO((void));
DL_IMPORT(void) PyErr_Clear Py_PROTO((void));
DL_IMPORT(void) PyErr_Fetch Py_PROTO((PyObject **, PyObject **, PyObject **));
DL_IMPORT(void) PyErr_Restore Py_PROTO((PyObject *, PyObject *, PyObject *));

/* Error testing and normalization */
DL_IMPORT(int) PyErr_GivenExceptionMatches Py_PROTO((PyObject *, PyObject *));
DL_IMPORT(int) PyErr_ExceptionMatches Py_PROTO((PyObject *));
DL_IMPORT(void) PyErr_NormalizeException Py_PROTO((PyObject**, PyObject**, PyObject**));


/* Predefined exceptions */

extern DL_IMPORT(PyObject *) PyExc_Exception;
extern DL_IMPORT(PyObject *) PyExc_StandardError;
extern DL_IMPORT(PyObject *) PyExc_ArithmeticError;
extern DL_IMPORT(PyObject *) PyExc_LookupError;

extern DL_IMPORT(PyObject *) PyExc_AssertionError;
extern DL_IMPORT(PyObject *) PyExc_AttributeError;
extern DL_IMPORT(PyObject *) PyExc_EOFError;
extern DL_IMPORT(PyObject *) PyExc_FloatingPointError;
extern DL_IMPORT(PyObject *) PyExc_EnvironmentError;
extern DL_IMPORT(PyObject *) PyExc_IOError;
extern DL_IMPORT(PyObject *) PyExc_OSError;
extern DL_IMPORT(PyObject *) PyExc_ImportError;
extern DL_IMPORT(PyObject *) PyExc_IndexError;
extern DL_IMPORT(PyObject *) PyExc_KeyError;
extern DL_IMPORT(PyObject *) PyExc_KeyboardInterrupt;
extern DL_IMPORT(PyObject *) PyExc_MemoryError;
extern DL_IMPORT(PyObject *) PyExc_NameError;
extern DL_IMPORT(PyObject *) PyExc_OverflowError;
extern DL_IMPORT(PyObject *) PyExc_RuntimeError;
extern DL_IMPORT(PyObject *) PyExc_NotImplementedError;
extern DL_IMPORT(PyObject *) PyExc_SyntaxError;
extern DL_IMPORT(PyObject *) PyExc_SystemError;
extern DL_IMPORT(PyObject *) PyExc_SystemExit;
extern DL_IMPORT(PyObject *) PyExc_TypeError;
extern DL_IMPORT(PyObject *) PyExc_UnboundLocalError;
extern DL_IMPORT(PyObject *) PyExc_UnicodeError;
extern DL_IMPORT(PyObject *) PyExc_ValueError;
extern DL_IMPORT(PyObject *) PyExc_ZeroDivisionError;
#ifdef MS_WINDOWS
extern DL_IMPORT(PyObject *) PyExc_WindowsError;
#endif

extern DL_IMPORT(PyObject *) PyExc_MemoryErrorInst;


/* Convenience functions */

extern DL_IMPORT(int) PyErr_BadArgument Py_PROTO((void));
extern DL_IMPORT(PyObject *) PyErr_NoMemory Py_PROTO((void));
extern DL_IMPORT(PyObject *) PyErr_SetFromErrno Py_PROTO((PyObject *));
extern DL_IMPORT(PyObject *) PyErr_SetFromErrnoWithFilename Py_PROTO((PyObject *, char *));
extern DL_IMPORT(PyObject *) PyErr_Format Py_PROTO((PyObject *, const char *, ...));
#ifdef MS_WINDOWS
extern DL_IMPORT(PyObject *) PyErr_SetFromWindowsErrWithFilename(int, const char *);
extern DL_IMPORT(PyObject *) PyErr_SetFromWindowsErr(int);
#endif

extern DL_IMPORT(void) PyErr_BadInternalCall Py_PROTO((void));

/* Function to create a new exception */
DL_IMPORT(PyObject *) PyErr_NewException Py_PROTO((char *name, PyObject *base,
				       PyObject *dict));

/* In sigcheck.c or signalmodule.c */
extern DL_IMPORT(int) PyErr_CheckSignals Py_PROTO((void));
extern DL_IMPORT(void) PyErr_SetInterrupt Py_PROTO((void));
	

#ifdef __cplusplus
}
#endif
#endif /* !Py_ERRORS_H */
