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

extern int _PyObject_SupportedAsScript(PyObject *);
extern const char* _Py_SourceAsString(
    PyObject *cmd,
    const char *funcname,
    const char *what,
    PyCompilerFlags *cf,
    PyObject **cmd_copy);

extern PyObject * _Py_CompileStringObjectWithModule(
    const char *str,
    PyObject *filename, int start,
    PyCompilerFlags *flags, int optimize,
    PyObject *module);


/* Stack size, in "pointers". This must be large enough, so
 * no two calls to check recursion depth are more than this far
 * apart. In practice, that means it must be larger than the C
 * stack consumption of PyEval_EvalDefault */
#if defined(_Py_ADDRESS_SANITIZER) || defined(_Py_THREAD_SANITIZER)
#  define _PyOS_LOG2_STACK_MARGIN 12
#elif defined(Py_DEBUG) && defined(WIN32)
#  define _PyOS_LOG2_STACK_MARGIN 12
#else
#  define _PyOS_LOG2_STACK_MARGIN 11
#endif
#define _PyOS_STACK_MARGIN (1 << _PyOS_LOG2_STACK_MARGIN)
#define _PyOS_STACK_MARGIN_BYTES (_PyOS_STACK_MARGIN * sizeof(void *))

#if SIZEOF_VOID_P == 8
#  define _PyOS_STACK_MARGIN_SHIFT (_PyOS_LOG2_STACK_MARGIN + 3)
#else
#  define _PyOS_STACK_MARGIN_SHIFT (_PyOS_LOG2_STACK_MARGIN + 2)
#endif

#ifdef _Py_THREAD_SANITIZER
#  define _PyOS_MIN_STACK_SIZE (_PyOS_STACK_MARGIN_BYTES * 6)
#else
#  define _PyOS_MIN_STACK_SIZE (_PyOS_STACK_MARGIN_BYTES * 3)
#endif


#ifdef __cplusplus
}
#endif
#endif  // !Py_INTERNAL_PYTHONRUN_H

