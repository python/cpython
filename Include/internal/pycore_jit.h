typedef _PyInterpreterFrame *(*_PyJITFunction)(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer);

_PyJITFunction _PyJIT_CompileTrace(_PyUOpInstruction *trace, int size);
