
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
DL_IMPORT(int) PyRun_AnyFileEx(FILE *, char *, int);

DL_IMPORT(int) PyRun_SimpleString(char *);
DL_IMPORT(int) PyRun_SimpleFile(FILE *, char *);
DL_IMPORT(int) PyRun_SimpleFileEx(FILE *, char *, int);
DL_IMPORT(int) PyRun_InteractiveOne(FILE *, char *);
DL_IMPORT(int) PyRun_InteractiveLoop(FILE *, char *);

DL_IMPORT(struct _node *) PyParser_SimpleParseString(char *, int);
DL_IMPORT(struct _node *) PyParser_SimpleParseFile(FILE *, char *, int);

DL_IMPORT(PyObject *) PyRun_String(char *, int, PyObject *, PyObject *);
DL_IMPORT(PyObject *) PyRun_File(FILE *, char *, int, PyObject *, PyObject *);
DL_IMPORT(PyObject *) PyRun_FileEx(FILE *, char *, int,
				   PyObject *, PyObject *, int);

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

/* Stack size, in "pointers" (so we get extra safety margins
   on 64-bit platforms).  On a 32-bit platform, this translates
   to a 8k margin. */
#define PYOS_STACK_MARGIN 2048

#if defined(WIN32) && !defined(MS_WIN64) && defined(_MSC_VER)
/* Enable stack checking under Microsoft C */
#define USE_STACKCHECK
#endif

#ifdef USE_STACKCHECK
/* Check that we aren't overflowing our stack */
DL_IMPORT(int) PyOS_CheckStack(void);
#endif

/* Signals */
typedef void (*PyOS_sighandler_t)(int);
DL_IMPORT(PyOS_sighandler_t) PyOS_getsig(int);
DL_IMPORT(PyOS_sighandler_t) PyOS_setsig(int, PyOS_sighandler_t);


#ifdef __cplusplus
}
#endif
#endif /* !Py_PYTHONRUN_H */
