#if defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)

#include <Python.h>

typedef PyObject* (*three_arg)(PyObject*, PyObject*, PyObject*);
typedef PyObject* (*two_arg)(PyObject*, PyObject*);
typedef PyObject* (*one_arg)(PyObject*);
typedef PyObject* (*zero_arg)(void);

#define RETCALL_IF_SIG_MATCHES(ty, args...) \
  if (__builtin_wasm_test_function_pointer_signature((ty)func)) { \
    return ((ty)func)(args); \
  }

PyObject*
_PyWasm_TrampolineCall(PyCFunctionWithKeywords func,
                       PyObject* self,
                       PyObject* args,
                       PyObject* kw)
{
    RETCALL_IF_SIG_MATCHES(three_arg, self, args, kw);
    RETCALL_IF_SIG_MATCHES(two_arg, self, args);
    RETCALL_IF_SIG_MATCHES(one_arg, self);
    RETCALL_IF_SIG_MATCHES(zero_arg);
    PyErr_SetString(PyExc_SystemError, "Handler has incorrect signature");
    return NULL;
}

typedef int (*setter_three_arg)(PyObject*, PyObject*, void*);
typedef int (*setter_two_arg)(PyObject*, PyObject*);
typedef int (*setter_one_arg)(PyObject*);
typedef int (*setter_zero_arg)(void);

int
_PyWasm_TrampolineCallSetter(setter func,
                             PyObject* self,
                             PyObject* val,
                             void* closure)
{
    RETCALL_IF_SIG_MATCHES(setter_three_arg, self, val, closure);
    RETCALL_IF_SIG_MATCHES(setter_two_arg, self, val);
    RETCALL_IF_SIG_MATCHES(setter_one_arg, self);
    RETCALL_IF_SIG_MATCHES(setter_zero_arg);
    PyErr_SetString(PyExc_SystemError, "Handler has incorrect signature");
    return -1;
}

#endif // defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)
