typedef int (*_PyJITFunction)(PyThreadState *, _PyInterpreterFrame *, PyObject **, _Py_CODEUNIT *);

PyAPI_FUNC(_PyJITFunction)_PyJIT_CompileTrace(int size, _Py_CODEUNIT **trace);
PyAPI_FUNC(void)_PyJIT_Free(_PyJITFunction trace);
