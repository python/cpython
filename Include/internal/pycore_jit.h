typedef int (*_PyJITFunction)(PyThreadState *tstate, _PyInterpreterFrame *frame, PyObject **stack_pointer, _Py_CODEUNIT *next_instr);

PyAPI_FUNC(_PyJITFunction)_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace);
PyAPI_FUNC(void)_PyJIT_Free(_PyJITFunction trace);
