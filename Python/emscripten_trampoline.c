#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS, EM_JS_DEPS
#include <Python.h>

// We have to be careful to work correctly with memory snapshots. Even if we are
// loading a memory snapshot, we need to perform the JS initialization work.
// That means we can't call the initialization code from C. Instead, we export
// this function pointer to JS and then fill it in a preRun function which runs
// unconditionally.
/**
 * Backwards compatible trampoline works with all JS runtimes
 */
EM_JS(
PyObject*,
_PyEM_TrampolineCall_inner, (int* success,
                             PyCFunctionWithKeywords func,
                             PyObject *arg1,
                             PyObject *arg2,
                             PyObject *arg3), {
    return wasmTable.get(func)(arg1, arg2, arg3);
}
try {
  const trampolineModule = getWasmTrampolineModule();
  const trampolineInstance = new WebAssembly.Instance(trampolineModule, {
    env: { __indirect_function_table: wasmTable, memory: wasmMemory },
  });
  _PyEM_TrampolineCall_inner = trampolineInstance.exports.trampoline_call;
} catch (e) {}
);

PyObject*
_PyEM_TrampolineCall(PyCFunctionWithKeywords func,
                     PyObject* self,
                     PyObject* args,
                     PyObject* kw)
{
    int success = 1;
    PyObject *result = _PyEM_TrampolineCall_inner(&success, func, self, args, kw);
    if (!success) {
        PyErr_SetString(PyExc_SystemError, "Handler takes too many arguments");
    }
    return result;
}

#endif
