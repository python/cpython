
/* Interfaces to parse and execute pieces of python code */

#ifndef Py_PYTHONRUN_H
#define Py_PYTHONRUN_H
#ifdef __cplusplus
extern "C" {
#endif

PyAPI_FUNC(PyObject *) Py_CompileString(const char *, const char *, int);

PyAPI_FUNC(void) PyErr_Print(void);
PyAPI_FUNC(void) PyErr_PrintEx(int);
PyAPI_FUNC(void) PyErr_Display(PyObject *, PyObject *, PyObject *);

#if !defined(Py_LIMITED_API) || Py_LIMITED_API+0 >= 0x030C0000
PyAPI_FUNC(void) PyErr_DisplayException(PyObject *);
#endif


/* Stuff with no proper home (yet) */
PyAPI_DATA(int) (*PyOS_InputHook)(void);

/* Stack size, in "pointers". This must be large enough, so
 * no two calls to check recursion depth are more than this far
 * apart. In practice, that means it must be larger than the C
 * stack consumption of PyEval_EvalDefault */
#if defined(_Py_ADDRESS_SANITIZER) || defined(_Py_THREAD_SANITIZER)
#  define PYOS_LOG2_STACK_MARGIN 12
#elif defined(Py_DEBUG) && defined(WIN32)
#  define PYOS_LOG2_STACK_MARGIN 12
#elif defined(__wasi__)
   /* Web assembly has two stacks, so this isn't really a size */
#  define PYOS_LOG2_STACK_MARGIN 9
#else
#  define PYOS_LOG2_STACK_MARGIN 11
#endif
#define PYOS_STACK_MARGIN (1 << PYOS_LOG2_STACK_MARGIN)
#define PYOS_STACK_MARGIN_BYTES (PYOS_STACK_MARGIN * sizeof(void *))

#if SIZEOF_VOID_P == 8
#define PYOS_STACK_MARGIN_SHIFT (PYOS_LOG2_STACK_MARGIN + 3)
#else
#define PYOS_STACK_MARGIN_SHIFT (PYOS_LOG2_STACK_MARGIN + 2)
#endif


#if defined(WIN32)
#define USE_STACKCHECK
#endif

#ifdef USE_STACKCHECK
/* Check that we aren't overflowing our stack */
PyAPI_FUNC(int) PyOS_CheckStack(void);
#endif

#ifndef Py_LIMITED_API
#  define Py_CPYTHON_PYTHONRUN_H
#  include "cpython/pythonrun.h"
#  undef Py_CPYTHON_PYTHONRUN_H
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_PYTHONRUN_H */
