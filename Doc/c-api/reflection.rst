.. highlightlang:: c

.. _reflection:

Reflection
==========

.. cfunction:: PyObject* PyEval_GetBuiltins()

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.


.. cfunction:: PyObject* PyEval_GetLocals()

   Return a dictionary of the local variables in the current execution frame,
   or *NULL* if no frame is currently executing.
   

.. cfunction:: PyObject* PyEval_GetGlobals()

   Return a dictionary of the global variables in the current execution frame,
   or *NULL* if no frame is currently executing.


.. cfunction:: PyFrameObject* PyEval_GetFrame()

   Return the current thread state's frame, which is *NULL* if no frame is
   currently executing.


.. cfunction:: int PyEval_GetRestricted()

   If there is a current frame and it is executing in restricted mode, return true,
   otherwise false.


.. cfunction:: const char* PyEval_GetFuncName(PyObject *func)

   Return the name of *func* if it is a function, class or instance object, else the
   name of *func*\s type.


.. cfunction:: const char* PyEval_GetFuncDesc(PyObject *func)

   Return a description string, depending on the type of *func*.
   Return values include "()" for functions and methods, " constructor",
   " instance", and " object".  Concatenated with the result of
   :cfunc:`PyEval_GetFuncName`, the result will be a description of
   *func*.
