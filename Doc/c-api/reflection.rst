.. highlight:: c

.. _reflection:

Reflection
==========

.. c:function:: PyObject* PyEval_GetBuiltins(void)

   .. deprecated:: 3.13

      Use :c:func:`PyEval_GetFrameBuiltins` instead.

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.


.. c:function:: PyObject* PyEval_GetLocals(void)

   .. deprecated:: 3.13

      To avoid creating a reference cycle in :term:`optimized scopes <optimized scope>`,
      use either :c:func:`PyEval_GetFrameLocals` to obtain the same behaviour as calling
      :func:`locals` in Python code, or else call :c:func:`PyFrame_GetLocals` on the result
      of :c:func:`PyEval_GetFrame` to get the same result as this function without having to
      cache the proxy instance on the underlying frame.

   Return the :attr:`~frame.f_locals` attribute of the currently executing frame,
   or ``NULL`` if no frame is currently executing.

   If the frame refers to an :term:`optimized scope`, this returns a
   write-through proxy object that allows modifying the locals.
   In all other cases (classes, modules, :func:`exec`, :func:`eval`) it returns
   the mapping representing the frame locals directly (as described for
   :func:`locals`).

   .. versionchanged:: 3.13
      As part of :pep:`667`, return a proxy object for optimized scopes.


.. c:function:: PyObject* PyEval_GetGlobals(void)

   .. deprecated:: 3.13

      Use :c:func:`PyEval_GetFrameGlobals` instead.

   Return a dictionary of the global variables in the current execution frame,
   or ``NULL`` if no frame is currently executing.


.. c:function:: PyFrameObject* PyEval_GetFrame(void)

   Return the current thread state's frame, which is ``NULL`` if no frame is
   currently executing.

   See also :c:func:`PyThreadState_GetFrame`.


.. c:function:: PyObject* PyEval_GetFrameBuiltins(void)

   Return a dictionary of the builtins in the current execution frame,
   or the interpreter of the thread state if no frame is currently executing.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyEval_GetFrameLocals(void)

   Return a dictionary of the local variables in the current execution frame,
   or ``NULL`` if no frame is currently executing. Equivalent to calling
   :func:`locals` in Python code.

   To access :attr:`~frame.f_locals` on the current frame without making an independent
   snapshot in :term:`optimized scopes <optimized scope>`, call :c:func:`PyFrame_GetLocals`
   on the result of :c:func:`PyEval_GetFrame`.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyEval_GetFrameGlobals(void)

   Return a dictionary of the global variables in the current execution frame,
   or ``NULL`` if no frame is currently executing. Equivalent to calling
   :func:`globals` in Python code.

   .. versionadded:: 3.13


.. c:function:: const char* PyEval_GetFuncName(PyObject *func)

   Return the name of *func* if it is a function, class or instance object, else the
   name of *func*\s type.


.. c:function:: const char* PyEval_GetFuncDesc(PyObject *func)

   Return a description string, depending on the type of *func*.
   Return values include "()" for functions and methods, " constructor",
   " instance", and " object".  Concatenated with the result of
   :c:func:`PyEval_GetFuncName`, the result will be a description of
   *func*.
