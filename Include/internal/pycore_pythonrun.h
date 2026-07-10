#ifndef Py_INTERNAL_PYTHONRUN_H
#define Py_INTERNAL_PYTHONRUN_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern PyObject* _PyRun_SimpleFile(
    FILE *fp,
    PyObject *filename,
    int closeit,
    PyCompilerFlags *flags);

extern PyObject* _PyRun_AnyFile(
    FILE *fp,
    PyObject *filename,
    int closeit,
    PyCompilerFlags *flags);

extern const char* _Py_SourceAsString(
    PyObject *cmd,
    const char *funcname,
    const char *what,
    PyCompilerFlags *cf,
    PyObject **cmd_copy);

// Export for special main.c string compiling with source tracebacks
extern PyObject* _PyRun_SimpleString(
    const char *command,
    PyObject* name,
    PyCompilerFlags *flags);


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYTHONRUN_H

