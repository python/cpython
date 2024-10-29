// To use preserve_none in JIT builds, we need to declare a separate function
// pointer with __attribute__((preserve_none)), since this attribute may not be
// supported by the compiler used to build the rest of the interpreter.
typedef _Py_CODEUNIT *(*jit_func_preserve_none)(_PyInterpreterFrame *frame, _PyStackRef *stack_pointer, PyThreadState *tstate) __attribute__((preserve_none));
