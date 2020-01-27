.. highlight:: c

.. _reflection:

Reflection
==========

.. c:function:: PyObject* PyEval_GetBuiltins()

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.


.. c:function:: PyObject* PyEval_GetLocals()

   Return a dictionary of the local variables in the current execution frame,
   or ``NULL`` if no frame is currently executing.


.. c:function:: PyObject* PyEval_GetGlobals()

   Return a dictionary of the global variables in the current execution frame,
   or ``NULL`` if no frame is currently executing.


.. c:function:: PyFrameObject* PyEval_GetFrame()

   Return the current thread state's frame, which is ``NULL`` if no frame is
   currently executing.


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.


.. c:function:: const char* PyEval_GetFuncName(PyObject *func)

   Return the name of *func* if it is a function, class or instance object, else the
   name of *func*\s type.


.. c:function:: const char* PyEval_GetFuncDesc(PyObject *func)

   Return a description string, depending on the type of *func*.
   Return values include "()" for functions and methods, " constructor",
   " instance", and " object".  Concatenated with the result of
   :c:func:`PyEval_GetFuncName`, the result will be a description of
   *func*.
