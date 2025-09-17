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
