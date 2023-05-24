typedef enum {
    _JUSTIN_RETURN_DEOPT = -1,
    _JUSTIN_RETURN_OK = 0,
    _JUSTIN_RETURN_GOTO_ERROR = 1,
    _JUSTIN_RETURN_GOTO_EXIT_UNWIND = 2,
    _JUSTIN_RETURN_GOTO_HANDLE_EVAL_BREAKER  = 3,
} _PyJITReturnCode;

typedef _PyJITReturnCode (*_PyJITFunction)(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer, _Py_CODEUNIT *next_instr);

PyAPI_FUNC(_PyJITFunction)_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace);
PyAPI_FUNC(void)_PyJIT_Free(_PyJITFunction trace);