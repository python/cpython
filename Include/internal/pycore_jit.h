#ifdef _Py_JIT

typedef _PyInterpreterFrame *(*_PyJITExecuteFunction)(_PyExecutorObject *executor, _PyInterpreterFrame *frame, PyObject **stack_pointer);


typedef _PyInterpreterFrame *(*_PyJITContinueFunction)(_PyInterpreterFrame *frame, PyObject **stack_pointer, PyThreadState *tstate);

int _PyJIT_Compile(_PyUOpExecutorObject *executor);
void _PyJIT_Free(_PyUOpExecutorObject *executor);

#endif