#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS
#include <Python.h>
#include "pycore_runtime.h"         // _PyRuntime


/**
 * This is the GoogleChromeLabs approved way to feature detect type-reflection:
 * https://github.com/GoogleChromeLabs/wasm-feature-detect/blob/main/src/detectors/type-reflection/index.js
 */
EM_JS(int, _PyEM_detect_type_reflection, (), {
    return "Function" in WebAssembly;
});

void
_Py_EmscriptenTrampoline_Init(_PyRuntimeState *runtime)
{
    runtime->wasm_type_reflection_available = _PyEM_detect_type_reflection();
}

/**
 * Backwards compatible trampoline works with all JS runtimes
 */
EM_JS(PyObject*,
_PyEM_TrampolineCall_JavaScript, (PyCFunctionWithKeywords func,
                                  PyObject *arg1,
                                  PyObject *arg2,
                                  PyObject *arg3),
{
    return wasmTable.get(func)(arg1, arg2, arg3);
}
);

/**
 * In runtimes with WebAssembly type reflection, count the number of parameters
 * and cast to the appropriate signature
 */
EM_JS(int, _PyEM_CountFuncParams, (PyCFunctionWithKeywords func),
{
    let n = _PyEM_CountFuncParams.cache.get(func);

    if (n !== undefined) {
        return n;
    }
    n = WebAssembly.Function.type(wasmTable.get(func)).parameters.length;
    _PyEM_CountFuncParams.cache.set(func, n);
    return n;
}
_PyEM_CountFuncParams.cache = new Map();
)


typedef PyObject* (*zero_arg)(void);
typedef PyObject* (*one_arg)(PyObject*);
typedef PyObject* (*two_arg)(PyObject*, PyObject*);
typedef PyObject* (*three_arg)(PyObject*, PyObject*, PyObject*);


PyObject*
_PyEM_TrampolineCall_Reflection(PyCFunctionWithKeywords func,
                                PyObject* self,
                                PyObject* args,
                                PyObject* kw)
{
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
            PyErr_SetString(PyExc_SystemError,
                            "Handler takes too many arguments");
            return NULL;
    }
}

#endif
