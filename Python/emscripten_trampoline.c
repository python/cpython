#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS, EM_JS_DEPS
#include <Python.h>
#include "pycore_runtime.h"         // _PyRuntime

typedef PyObject *(*TrampolineFunc)(int *success, PyCFunctionWithKeywords func,
                                    PyObject *arg1, PyObject *arg2,
                                    PyObject *arg3);

EMSCRIPTEN_KEEPALIVE const char trampoline_inner_wasm[] = {
#embed "Python/emscripten_trampoline_inner.wasm"
};

EMSCRIPTEN_KEEPALIVE const int trampoline_inner_wasm_length =
    sizeof(trampoline_inner_wasm);

// Offset of emscripten_count_args_function in _PyRuntimeState. There's a couple
// of alternatives:
// 1. Just make emscripten_count_args_function a real C global variable instead
//    of a field of _PyRuntimeState. This would violate our rule against mutable
//    globals.
// 2. #define a preprocessor constant equal to a hard coded number and make a
//    _Static_assert(offsetof(_PyRuntimeState, emscripten_count_args_function)
//    == OURCONSTANT) This has the disadvantage that we have to update the hard
//    coded constant when _PyRuntimeState changes
//
// So putting the mutable constant in _PyRuntime and using a immutable global to
// record the offset so we can access it from JS is probably the best way.
EMSCRIPTEN_KEEPALIVE const int _PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET =
    offsetof(_PyRuntimeState, emscripten_trampoline);

EM_JS(TrampolineFunc, _PyEM_GetTrampolinePtr, (void), {
    return Module._PyEM_CountArgsPtr; // initialized below
}

function getPyEMTrampolinePtr() {
    // Starting with iOS 18.3.1, WebKit on iOS has an issue with the garbage
    // collector that breaks the call trampoline. See #130418 and
    // https://bugs.webkit.org/show_bug.cgi?id=293113 for details.
    let isIOS = globalThis.navigator && (
        /iPad|iPhone|iPod/.test(navigator.userAgent) ||
        // Starting with iPadOS 13, iPads might send a platform string that looks like a desktop Mac.
        // To differentiate, we check if the platform is 'MacIntel' (common for Macs and newer iPads)
        // AND if the device has multi-touch capabilities (navigator.maxTouchPoints > 1)
        (navigator.platform === 'MacIntel' && typeof navigator.maxTouchPoints !== 'undefined' && navigator.maxTouchPoints > 1)
    );
    if (isIOS) {
        return 0;
    }
    const code = HEAP8.subarray(
        _trampoline_inner_wasm,
        _trampoline_inner_wasm + HEAP32[_trampoline_inner_wasm_length / 4]);
    try {
        const mod = new WebAssembly.Module(code);
        const inst = new WebAssembly.Instance(mod, { e: { t: wasmTable } });
        return addFunction(inst.exports.trampoline_call);
    } catch (e) {
        // If something goes wrong, we'll null out _PyEM_CountFuncParams and fall
        // back to the JS trampoline.
        return 0;
    }
}

addOnPreRun(() => {
    const ptr = getPyEMTrampolinePtr();
    Module._PyEM_CountArgsPtr = ptr;
    const offset = HEAP32[__PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET / 4];
    HEAP32[(__PyRuntime + offset) / 4] = ptr;
});
);

void
_Py_EmscriptenTrampoline_Init(_PyRuntimeState *runtime)
{
    runtime->emscripten_trampoline = _PyEM_GetTrampolinePtr();
}

// We have to be careful to work correctly with memory snapshots. Even if we are
// loading a memory snapshot, we need to perform the JS initialization work.
// That means we can't call the initialization code from C. Instead, we export
// this function pointer to JS and then fill it in a preRun function which runs
// unconditionally.
/**
 * Backwards compatible trampoline works with all JS runtimes
 */
EM_JS(PyObject*, _PyEM_TrampolineCall_JS, (PyCFunctionWithKeywords func, PyObject *arg1, PyObject *arg2, PyObject *arg3), {
    return wasmTable.get(func)(arg1, arg2, arg3);
});

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
    TrampolineFunc trampoline = _PyRuntime.emscripten_trampoline;
    if (trampoline == 0) {
        return _PyEM_TrampolineCall_JS(func, self, args, kw);
    }
    int success = 0;
    PyObject *result = trampoline(&success, func, self, args, kw);
    if (!success) {
        PyErr_SetString(PyExc_SystemError, "Handler takes too many arguments");
    }
    return result;
}

#endif
