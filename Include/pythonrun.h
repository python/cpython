/***********************************************************
Copyright (c) 2000, BeOpen.com.
Copyright (c) 1995-2000, Corporation for National Research Initiatives.
Copyright (c) 1990-1995, Stichting Mathematisch Centrum.
All rights reserved.

See the file "Misc/COPYRIGHT" for information on usage and
redistribution of this file, and for a DISCLAIMER OF ALL WARRANTIES.
******************************************************************/

/* Interfaces to parse and execute pieces of python code */

#ifndef Py_PYTHONRUN_H
#define Py_PYTHONRUN_H
#ifdef __cplusplus
extern "C" {
#endif

DL_IMPORT(void) Py_SetProgramName(char *);
DL_IMPORT(char *) Py_GetProgramName(void);

DL_IMPORT(void) Py_SetPythonHome(char *);
DL_IMPORT(char *) Py_GetPythonHome(void);

DL_IMPORT(void) Py_Initialize(void);
DL_IMPORT(void) Py_Finalize(void);
DL_IMPORT(int) Py_IsInitialized(void);
DL_IMPORT(PyThreadState *) Py_NewInterpreter(void);
DL_IMPORT(void) Py_EndInterpreter(PyThreadState *);

DL_IMPORT(int) PyRun_AnyFile(FILE *, char *);

DL_IMPORT(int) PyRun_SimpleString(char *);
DL_IMPORT(int) PyRun_SimpleFile(FILE *, char *);
DL_IMPORT(int) PyRun_InteractiveOne(FILE *, char *);
DL_IMPORT(int) PyRun_InteractiveLoop(FILE *, char *);

DL_IMPORT(struct _node *) PyParser_SimpleParseString(char *, int);
DL_IMPORT(struct _node *) PyParser_SimpleParseFile(FILE *, char *, int);

DL_IMPORT(PyObject *) PyRun_String(char *, int, PyObject *, PyObject *);
DL_IMPORT(PyObject *) PyRun_File(FILE *, char *, int, PyObject *, PyObject *);

DL_IMPORT(PyObject *) Py_CompileString(char *, char *, int);

DL_IMPORT(void) PyErr_Print(void);
DL_IMPORT(void) PyErr_PrintEx(int);

DL_IMPORT(int) Py_AtExit(void (*func)(void));

DL_IMPORT(void) Py_Exit(int);

DL_IMPORT(int) Py_FdIsInteractive(FILE *, char *);

/* In getpath.c */
DL_IMPORT(char *) Py_GetProgramFullPath(void);
DL_IMPORT(char *) Py_GetPrefix(void);
DL_IMPORT(char *) Py_GetExecPrefix(void);
DL_IMPORT(char *) Py_GetPath(void);

/* In their own files */
DL_IMPORT(const char *) Py_GetVersion(void);
DL_IMPORT(const char *) Py_GetPlatform(void);
DL_IMPORT(const char *) Py_GetCopyright(void);
DL_IMPORT(const char *) Py_GetCompiler(void);
DL_IMPORT(const char *) Py_GetBuildInfo(void);

/* Internal -- various one-time initializations */
DL_IMPORT(PyObject *) _PyBuiltin_Init(void);
DL_IMPORT(PyObject *) _PySys_Init(void);
DL_IMPORT(void) _PyImport_Init(void);
DL_IMPORT(void) init_exceptions(void);

/* Various internal finalizers */
DL_IMPORT(void) fini_exceptions(void);
DL_IMPORT(void) _PyImport_Fini(void);
DL_IMPORT(void) PyMethod_Fini(void);
DL_IMPORT(void) PyFrame_Fini(void);
DL_IMPORT(void) PyCFunction_Fini(void);
DL_IMPORT(void) PyTuple_Fini(void);
DL_IMPORT(void) PyString_Fini(void);
DL_IMPORT(void) PyInt_Fini(void);
DL_IMPORT(void) PyFloat_Fini(void);
DL_IMPORT(void) PyOS_FiniInterrupts(void);

/* Stuff with no proper home (yet) */
DL_IMPORT(char *) PyOS_Readline(char *);
extern DL_IMPORT(int) (*PyOS_InputHook)(void);
extern DL_IMPORT(char) *(*PyOS_ReadlineFunctionPointer)(char *);
#ifdef USE_STACKCHECK
int PyOS_CheckStack(void);		/* Check that we aren't overflowing our stack */
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYTHONRUN_H */
