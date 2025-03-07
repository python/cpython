#ifndef Py_INTERNAL_PYTHONRUN_H
#define Py_INTERNAL_PYTHONRUN_H
#ifdef __cplusplus
extern "C" {
#endif

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

extern int _PyRun_SimpleFileObject(
    FILE *fp,
    PyObject *filename,
    int closeit,
    PyCompilerFlags *flags);

extern int _PyRun_AnyFileObject(
    FILE *fp,
    PyObject *filename,
    int closeit,
    PyCompilerFlags *flags);

extern int _PyRun_InteractiveLoopObject(
    FILE *fp,
    PyObject *filename,
    PyCompilerFlags *flags);

extern const char* _Py_SourceAsString(
    PyObject *cmd,
    const char *funcname,
    const char *what,
    PyCompilerFlags *cf,
    PyObject **cmd_copy);

#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYTHONRUN_H

