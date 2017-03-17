Issue #26312: SystemError is now raised in all programming bugs with using
PyArg_ParseTupleAndKeywords().  RuntimeError did raised before in some
programming bugs.