#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>
#include <Python.h>

EMSCRIPTEN_KEEPALIVE int _PyEM_type_reflection_available;

EM_JS_DEPS(_PyEMJS_TrampolineCall, "$getWasmTableEntry")
EM_JS(PyObject*, _PyEMJS_TrampolineCall, (PyCFunctionWithKeywords func, PyObject *arg1, PyObject *arg2, PyObject *arg3), {
    return getWasmTableEntry(func)(arg1, arg2, arg3);
}
);

EM_JS(int, _PyEM_CountFuncParams, (PyCFunctionWithKeywords func), {
  let n = _PyEM_CountFuncParams.cache.get(func);
  if (n !== undefined) {
    return n;
  }
  n = WebAssembly.Function.type(getWasmTableEntry(func)).parameters.length;
  _PyEM_CountFuncParams.cache.set(func, n);
  return n;
}
_PyEM_CountFuncParams.cache = new Map();
switch(typeof Module.preRun) {
    case "undefined":
        Module.preRun = [];
        break;
    case "function":
        Module.preRun = [Module.preRun];
        break;
}
Module.preRun.push(() => HEAP32[__PyEM_type_reflection_available/4] = "Function" in WebAssembly);
)


typedef PyObject* (*zero_arg)(void);
typedef PyObject* (*one_arg)(PyObject*);
typedef PyObject* (*two_arg)(PyObject*, PyObject*);
typedef PyObject* (*three_arg)(PyObject*, PyObject*, PyObject*);

PyObject*
_PyEM_TrampolineCall(PyCFunctionWithKeywords func,
              PyObject* self,
              PyObject* args,
              PyObject* kw)
{
  if (!_PyEM_type_reflection_available) {
    return _PyEMJS_TrampolineCall(func, self, args, kw);
  } else {
    switch (_PyEM_CountFuncParams(func)) {
      case 0:
        return ((zero_arg)func)();
      case 1:
        return ((one_arg)func)(self);
      case 2:
        return ((two_arg)func)(self, args);
      case 3:
        return ((three_arg)func)(self, args, kw);
      default:
        PyErr_SetString(PyExc_SystemError, "Handler takes too many arguments");
        return NULL;
    }
  }
}


#endif
