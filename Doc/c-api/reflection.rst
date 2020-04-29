.. highlight:: c

.. _reflection:

Reflection
==========

.. c:function:: PyObject* PyEval_GetBuiltins(void)

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.


.. c:function:: PyObject* PyEval_GetLocals(void)

   Return a dictionary of the local variables in the current execution frame,
   or ``NULL`` if no frame is currently executing.


.. c:function:: PyObject* PyEval_GetGlobals(void)

   Return a dictionary of the global variables in the current execution frame,
   or ``NULL`` if no frame is currently executing.


.. c:function:: PyFrameObject* PyEval_GetFrame(void)

   Return the current thread state's frame, which is ``NULL`` if no frame is
   currently executing.

   See also :c:func:`PyThreadState_GetFrame`.


.. c:function:: int PyFrame_GetBack(PyFrameObject *frame)

   Get the *frame* next outer frame.

   Return a strong reference, or ``NULL`` if *frame* has no outer frame.

   *frame* must not be ``NULL``.

   .. versionadded:: 3.9


.. c:function:: int PyFrame_GetCode(PyFrameObject *frame)

   Get the *frame* code.

   Return a strong reference.

   *frame* must not be ``NULL``. The result (frame code) cannot be ``NULL``.

   .. versionadded:: 3.9


.. c:function:: int PyFrame_GetLineNumber(PyFrameObject *frame)

   Return the line number that *frame* is currently executing.

   *frame* must not be ``NULL``.


.. c:function:: const char* PyEval_GetFuncName(PyObject *func)

   Return the name of *func* if it is a function, class or instance object, else the
   name of *func*\s type.


.. c:function:: const char* PyEval_GetFuncDesc(PyObject *func)

   Return a description string, depending on the type of *func*.
   Return values include "()" for functions and methods, " constructor",
   " instance", and " object".  Concatenated with the result of
   :c:func:`PyEval_GetFuncName`, the result will be a description of
   *func*.
