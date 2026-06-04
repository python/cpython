// This file must be compiled with -mgc to enable the extra wasm-gc
// instructions. It has to be compiled separately because not enough JS runtimes
// support wasm-gc yet. If the JS runtime does not support wasm-gc (or has buggy
// support like iOS), we will use the JS trampoline fallback.

// We can't import Python.h here because it is compiled/linked with -nostdlib.
// We don't need to know what's inside PyObject* anyways. We could just call it
// void* everywhere. There are two reasons to do this:
// 1. to improve readability
// 2. eventually when we are comfortable requiring wasm-gc, we can merge this
//    into emscripten_trampoline.c without worrying about it.
typedef void PyObject;

typedef PyObject* (*three_arg)(PyObject*, PyObject*, PyObject*);
typedef PyObject* (*two_arg)(PyObject*, PyObject*);
typedef PyObject* (*one_arg)(PyObject*);
typedef PyObject* (*zero_arg)(void);

#define TRY_RETURN_CALL(ty, args...) \
  if (__builtin_wasm_test_function_pointer_signature((ty)func)) { \
    return ((ty)func)(args); \
  }

__attribute__((export_name("trampoline_call"))) PyObject*
trampoline_call(int* success,
                void* func,
                PyObject* self,
                PyObject* args,
                PyObject* kw)
{
  *success = 1;
  TRY_RETURN_CALL(three_arg, self, args, kw);
  TRY_RETURN_CALL(two_arg, self, args);
  TRY_RETURN_CALL(one_arg, self);
  TRY_RETURN_CALL(zero_arg);
  *success = 0;
  return 0;
}
