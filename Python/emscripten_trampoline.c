#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS, EM_JS_DEPS
#include <Python.h>
#include "pycore_runtime.h"         // _PyRuntime

// We use the _PyRuntime.emscripten_trampoline field to store a function pointer
// for a wasm-gc based trampoline if it works. Otherwise fall back to JS
// trampoline. The JS trampoline breaks stack switching but every runtime that
// supports stack switching also supports wasm-gc.
//
// We'd like to make the trampoline call into a direct call but currently we
// need to import the wasmTable to compile trampolineModule. emcc >= 4.0.19
// defines the table in WebAssembly and exports it so we won't have access to it
// until after the main module is compiled.
//
// To fix this, one natural solution would be to pass a funcref to the
// trampoline instead of a table index. Several PRs would be needed to fix
// things in llvm and emscripten in order to make this possible.
//
// The performance costs of an extra call_indirect aren't that large anyways.
// The JIT should notice that the target is always the same and turn into a
// check
//
// if (call_target != expected) deoptimize;
// direct_call(call_target, args);

// Offset of emscripten_trampoline in _PyRuntimeState. There's a couple of
// alternatives:
//
// 1. Just make emscripten_trampoline a real C global variable instead of a
//    field of _PyRuntimeState. This would violate our rule against mutable
//    globals.
//
// 2. #define a preprocessor constant equal to a hard coded number and make a
//    _Static_assert(offsetof(_PyRuntimeState, emscripten_trampoline) == OURCONSTANT)
//    This has the disadvantage that we have to update the hard coded constant
//    when _PyRuntimeState changes
//
// So putting the mutable constant in _PyRuntime and using a immutable global to
// record the offset so we can access it from JS is probably the best way.
EMSCRIPTEN_KEEPALIVE const int _PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET = offsetof(_PyRuntimeState, emscripten_trampoline);

typedef PyObject* (*TrampolineFunc)(int* success,
                                    PyCFunctionWithKeywords func,
                                    PyObject* self,
                                    PyObject* args,
                                    PyObject* kw);

/**
 * Backwards compatible trampoline works with all JS runtimes
 */
EM_JS(PyObject*, _PyEM_TrampolineCall_JS, (PyCFunctionWithKeywords func, PyObject *arg1, PyObject *arg2, PyObject *arg3), {
    return wasmTable.get(func)(arg1, arg2, arg3);
}
// Try to compile wasm-gc trampoline if possible.
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
    let trampolineModule;
    try {
        trampolineModule = getWasmTrampolineModule();
    } catch (e) {
        // Compilation error due to missing wasm-gc support, fall back to JS
        // trampoline
        return 0;
    }
    const trampolineInstance = new WebAssembly.Instance(trampolineModule, {
        env: { __indirect_function_table: wasmTable, memory: wasmMemory },
    });
    return addFunction(trampolineInstance.exports.trampoline_call);
}
// We have to be careful to work correctly with memory snapshots -- the value of
// _PyRuntimeState.emscripten_trampoline needs to reflect whether wasm-gc is
// available in the current runtime, not in the runtime the snapshot was taken
// in. This writes the appropriate value to
// _PyRuntimeState.emscripten_trampoline from JS startup code that runs every
// time, whether we are restoring a snapshot or not.
addOnPreRun(function setEmscriptenTrampoline() {
    const ptr = getPyEMTrampolinePtr();
    const offset = HEAP32[__PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET / 4];
    HEAP32[(__PyRuntime + offset) / 4] = ptr;
});
);

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
    int success = 1;
    PyObject *result = trampoline(&success, func, self, args, kw);
    if (!success) {
        PyErr_SetString(PyExc_SystemError, "Handler takes too many arguments");
    }
    return result;
}

#else
// This is exported so we need to define it even when it isn't used
__attribute__((used)) const int _PyEM_EMSCRIPTEN_TRAMPOLINE_OFFSET = 0;
#endif
