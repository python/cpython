#ifndef Py_WASM_TRAMPOLINE_H
#define Py_WASM_TRAMPOLINE_H

#include "Python.h"

/**
 * C function call trampolines to mitigate bad function pointer casts.
 *
 * Section 6.3.2.3, paragraph 8 reads:
 *
 *      A pointer to a function of one type may be converted to a pointer to a
 *      function of another type and back again; the result shall compare equal to
 *      the original pointer. If a converted pointer is used to call a function
 *      whose type is not compatible with the pointed-to type, the behavior is
 *      undefined.
 *
 * Typical native ABIs ignore additional arguments or fill in missing values
 * with 0/NULL in function pointer cast. Compilers do not show warnings when a
 * function pointer is explicitly casted to an incompatible type.
 *
 * Bad fpcasts are an issue in WebAssembly. Wasm's indirect_call has strict
 * function signature checks. Argument count, types, and return type must match.
 *
 * Third party code unintentionally rely on problematic fpcasts. The call
 * trampoline mitigates common occurrences of bad fpcasts on Wasm targets.
 */

#if defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)

PyObject*
_PyWasm_TrampolineCall(PyCFunctionWithKeywords func,
                       PyObject* self,
                       PyObject* args,
                       PyObject* kw);

int
_PyWasm_TrampolineCallSetter(setter func,
                             PyObject* self,
                             PyObject* val,
                             void* closure);

#define _PyCFunction_TrampolineCall(meth, self, args) \
    _PyWasm_TrampolineCall(*_PyCFunctionWithKeywords_CAST(meth), (self), (args), NULL)

#define _PyCFunctionWithKeywords_TrampolineCall(meth, self, args, kw) \
    _PyWasm_TrampolineCall((meth), (self), (args), (kw))

#define descr_set_trampoline_call(set, obj, value, closure)                 \
    _PyWasm_TrampolineCallSetter((set), (obj), (value), (closure))

#define descr_get_trampoline_call(get, obj, closure)                \
    _PyWasm_TrampolineCall(_PyCFunctionWithKeywords_CAST(get), (obj), \
                           (PyObject*)(closure), NULL)


#else // defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)

// Disable trampolines by directly calling the method.

#define _PyCFunction_TrampolineCall(meth, self, args) \
    (meth)((self), (args))

#define _PyCFunctionWithKeywords_TrampolineCall(meth, self, args, kw) \
    (meth)((self), (args), (kw))

#define descr_set_trampoline_call(set, obj, value, closure) \
    (set)((obj), (value), (closure))

#define descr_get_trampoline_call(get, obj, closure) \
    (get)((obj), (closure))

#endif // defined(__wasm__) && defined(PY_CALL_TRAMPOLINE)

#endif // ndef Py_WASM_TRAMPOLINE_H
