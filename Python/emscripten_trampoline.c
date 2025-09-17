#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS, EM_JS_DEPS
#include <Python.h>

EM_JS(
PyObject*,
_PyEM_TrampolineCall_inner, (int* success,
                             PyCFunctionWithKeywords func,
                             PyObject *arg1,
                             PyObject *arg2,
                             PyObject *arg3), {
    // JavaScript fallback trampoline
    return wasmTable.get(func)(arg1, arg2, arg3);
}
// Try to replace the JS definition of _PyEM_TrampolineCall_inner with a wasm
// version.
(function () {
    // iOS ships broken wasm-gc, so feature detect and turn it off
    const isIOS =
        globalThis.navigator &&
        (/iPad|iPhone|iPod/.test(navigator.userAgent) ||
            // Starting with iPadOS 13, iPads might send a platform string that looks like a desktop Mac.
            // To differentiate, we check if the platform is 'MacIntel' (common for Macs and newer iPads)
            // AND if the device has multi-touch capabilities (navigator.maxTouchPoints > 1)
            (navigator.platform === "MacIntel" &&
                typeof navigator.maxTouchPoints !== "undefined" &&
                navigator.maxTouchPoints > 1));
    if (isIOS) {
        return;
    }
    try {
        const trampolineModule = getWasmTrampolineModule();
        const trampolineInstance = new WebAssembly.Instance(trampolineModule, {
            env: { __indirect_function_table: wasmTable, memory: wasmMemory },
        });
        _PyEM_TrampolineCall_inner = trampolineInstance.exports.trampoline_call;
        console.log("Assigned _PyEM_TrampolineCall_inner!");
    } catch (e) {
        // Compilation error due to missing wasm-gc support, fall back to JS
        // trampoline
    }
})();
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
