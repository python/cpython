typedef _PyInterpreterFrame *(*_PyJITFunction)(_PyExecutorObject *self, _PyInterpreterFrame *frame, PyObject **stack_pointer);

PyAPI_FUNC(_PyJITFunction)_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace);
PyAPI_FUNC(void)_PyJIT_Free(_PyJITFunction trace);
