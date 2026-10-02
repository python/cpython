#if defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)

#include <Python.h>

typedef PyObject* (*three_arg)(PyObject*, PyObject*, PyObject*);
typedef PyObject* (*two_arg)(PyObject*, PyObject*);
typedef PyObject* (*one_arg)(PyObject*);
typedef PyObject* (*zero_arg)(void);

#define TRY_RETURN_CALL(ty, args...) \
  if (__builtin_wasm_test_function_pointer_signature((ty)func)) { \
    return ((ty)func)(args); \
  }

PyObject*
_PyWasm_TrampolineCall(PyCFunctionWithKeywords func,
                       PyObject* self,
                       PyObject* args,
                       PyObject* kw)
{
    TRY_RETURN_CALL(three_arg, self, args, kw);
    TRY_RETURN_CALL(two_arg, self, args);
    TRY_RETURN_CALL(one_arg, self);
    TRY_RETURN_CALL(zero_arg);
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
    TRY_RETURN_CALL(setter_three_arg, self, val, closure);
    TRY_RETURN_CALL(setter_two_arg, self, val);
    TRY_RETURN_CALL(setter_one_arg, self);
    TRY_RETURN_CALL(setter_zero_arg);
    PyErr_SetString(PyExc_SystemError, "Handler has incorrect signature");
    return -1;
}

#endif // defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)
