- Issue #23571: PyObject_Call() and PyCFunction_Call() now raise a SystemError
  if a function returns a result and raises an exception. The SystemError is
  chained to the previous exception.

