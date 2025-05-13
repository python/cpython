#if defined(PY_CALL_TRAMPOLINE)

#include <emscripten.h>             // EM_JS, EM_JS_DEPS
#include <Python.h>
#include "pycore_runtime.h"         // _PyRuntime

typedef int (*CountArgsFunc)(PyCFunctionWithKeywords func);

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
EMSCRIPTEN_KEEPALIVE const int _PyEM_EMSCRIPTEN_COUNT_ARGS_OFFSET = offsetof(_PyRuntimeState, emscripten_count_args_function);

EM_JS(CountArgsFunc, _PyEM_GetCountArgsPtr, (), {
    return Module._PyEM_CountArgsPtr; // initialized below
}
// Binary module for the checks. It has to be done in web assembly because
// clang/llvm have no support yet for the reference types yet. In fact, the wasm
// binary toolkit doesn't yet support the ref.test instruction either. To
// convert the following textual wasm to a binary, you can build wabt from this
// branch: https://github.com/WebAssembly/wabt/pull/2529 and then use that
// wat2wasm binary.
//
// (module
//     (type $type0 (func (param) (result i32)))
//     (type $type1 (func (param i32) (result i32)))
//     (type $type2 (func (param i32 i32) (result i32)))
//     (type $type3 (func (param i32 i32 i32) (result i32)))
//     (type $blocktype (func (param) (result)))
//     (table $funcs (import "e" "t") 0 funcref)
//     (export "f" (func $f))
//     (func $f (param $fptr i32) (result i32)
//              (local $fref funcref)
//         local.get $fptr
//         table.get $funcs
//         local.tee $fref
//         ref.test $type3
//         if $blocktype
//             i32.const 3
//             return
//         end
//         local.get $fref
//         ref.test $type2
//         if $blocktype
//             i32.const 2
//             return
//         end
//         local.get $fref
//         ref.test $type1
//         if $blocktype
//             i32.const 1
//             return
//         end
//         local.get $fref
//         ref.test $type0
//         if $blocktype
//             i32.const 0
//             return
//         end
//         i32.const -1
//     )
// )

function getPyEMCountArgsPtr() {
    let isIOS = globalThis.navigator && /iPad|iPhone|iPod/.test(navigator.platform);
    if (isIOS) {
        return 0;
    }

    // Try to initialize countArgsFunc
    const code = new Uint8Array([
        0x00, 0x61, 0x73, 0x6d, // \0asm magic number
        0x01, 0x00, 0x00, 0x00, // version 1
        0x01, 0x1a, // Type section, body is 0x1a bytes
            0x05, // 6 entries
            0x60, 0x00, 0x01, 0x7f,                      // (type $type0 (func (param) (result i32)))
            0x60, 0x01, 0x7f, 0x01, 0x7f,                // (type $type1 (func (param i32) (result i32)))
            0x60, 0x02, 0x7f, 0x7f, 0x01, 0x7f,          // (type $type2 (func (param i32 i32) (result i32)))
            0x60, 0x03, 0x7f, 0x7f, 0x7f, 0x01, 0x7f,    // (type $type3 (func (param i32 i32 i32) (result i32)))
            0x60, 0x00, 0x00,                            // (type $blocktype (func (param) (result)))
        0x02, 0x09, // Import section, 0x9 byte body
            0x01, // 1 import (table $funcs (import "e" "t") 0 funcref)
            0x01, 0x65, // "e"
            0x01, 0x74, // "t"
            0x01,       // importing a table
            0x70,       // of entry type funcref
            0x00, 0x00, // table limits: no max, min of 0
        0x03, 0x02,   // Function section
            0x01, 0x01, // We're going to define one function of type 1 (func (param i32) (result i32))
        0x07, 0x05, // export section
            0x01, // 1 export
            0x01, 0x66, // called "f"
            0x00, // a function
            0x00, // at index 0

        0x0a, 56,  // Code section,
            0x01, 54, // one entry of length 54
            0x01, 0x01, 0x70, // one local of type funcref
            // Body of the function
            0x20, 0x00,       // local.get $fptr
            0x25, 0x00,       // table.get $funcs
            0x22, 0x01,       // local.tee $fref
            0xfb, 0x14, 0x03, // ref.test $type3
            0x04, 0x04,       // if (type $blocktype)
                0x41, 0x03,   //   i32.const 3
                0x0f,         //   return
            0x0b,             // end block

            0x20, 0x01,       // local.get $fref
            0xfb, 0x14, 0x02, // ref.test $type2
            0x04, 0x04,       // if (type $blocktype)
                0x41, 0x02,   //   i32.const 2
                0x0f,         //   return
            0x0b,             // end block

            0x20, 0x01,       // local.get $fref
            0xfb, 0x14, 0x01, // ref.test $type1
            0x04, 0x04,       // if (type $blocktype)
                0x41, 0x01,   //   i32.const 1
                0x0f,         //   return
            0x0b,             // end block

            0x20, 0x01,       // local.get $fref
            0xfb, 0x14, 0x00, // ref.test $type0
            0x04, 0x04,       // if (type $blocktype)
                0x41, 0x00,   //   i32.const 0
                0x0f,         //   return
            0x0b,             // end block

            0x41, 0x7f,       // i32.const -1
            0x0b // end function
    ]);
    try {
        const mod = new WebAssembly.Module(code);
        const inst = new WebAssembly.Instance(mod, { e: { t: wasmTable } });
        return addFunction(inst.exports.f);
    } catch (e) {
        // If something goes wrong, we'll null out _PyEM_CountFuncParams and fall
        // back to the JS trampoline.
        return 0;
    }
}

addOnPreRun(() => {
    const ptr = getPyEMCountArgsPtr();
    Module._PyEM_CountArgsPtr = ptr;
    const offset = HEAP32[__PyEM_EMSCRIPTEN_COUNT_ARGS_OFFSET / 4];
    HEAP32[(__PyRuntime + offset) / 4] = ptr;
});
);

void
_Py_EmscriptenTrampoline_Init(_PyRuntimeState *runtime)
{
    runtime->emscripten_count_args_function = _PyEM_GetCountArgsPtr();
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
    CountArgsFunc count_args = _PyRuntime.emscripten_count_args_function;
    if (count_args == 0) {
        return _PyEM_TrampolineCall_JS(func, self, args, kw);
    }
    switch (count_args(func)) {
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

#endif
