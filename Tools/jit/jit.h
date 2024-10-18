// To use preserve_none in JIT builds, we need to declare a separate function pointer with __attribute__((preserve_none)) since this attribute is not supported in < clang 19.
// This is functionally the same as jit_func_native from Include/internal/pycore_jit.h except that it has the __attribute__((preserve_none)) attribute.
typedef _Py_CODEUNIT *(*jit_func_preserve_none)(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate) __attribute__((preserve_none));
