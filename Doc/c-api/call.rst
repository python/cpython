.. highlight:: c

.. _call:

Call Protocol
=============

.. c:function:: int PyCallable_Check(PyObject *o)

   Determine if the object *o* is callable.  Return ``1`` if the object is callable
   and ``0`` otherwise.  This function always succeeds.


.. c:function:: PyObject* PyObject_CallNoArgs(PyObject *callable)

   Call a callable Python object *callable* without any arguments. It is the
   most efficient way to call a callable Python object without any argument.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. versionadded:: 3.9


.. c:function:: PyObject* _PyObject_CallOneArg(PyObject *callable, PyObject *arg)

   Call a callable Python object *callable* with exactly 1 positional argument
   *arg* and no keyword arguments.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. versionadded:: 3.9


.. c:function:: PyObject* PyObject_Call(PyObject *callable, PyObject *args, PyObject *kwargs)

   Call a callable Python object *callable*, with arguments given by the
   tuple *args*, and named arguments given by the dictionary *kwargs*.

   *args* must not be *NULL*, use an empty tuple if no arguments are needed.
   If no named arguments are needed, *kwargs* can be *NULL*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression:
   ``callable(*args, **kwargs)``.


.. c:function:: PyObject* PyObject_CallObject(PyObject *callable, PyObject *args)

   Call a callable Python object *callable*, with arguments given by the
   tuple *args*.  If no arguments are needed, then *args* can be *NULL*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression: ``callable(*args)``.


.. c:function:: PyObject* PyObject_CallFunction(PyObject *callable, const char *format, ...)

   Call a callable Python object *callable*, with a variable number of C arguments.
   The C arguments are described using a :c:func:`Py_BuildValue` style format
   string.  The format can be *NULL*, indicating that no arguments are provided.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression: ``callable(*args)``.

   Note that if you only pass :c:type:`PyObject \*` args,
   :c:func:`PyObject_CallFunctionObjArgs` is a faster alternative.

   .. versionchanged:: 3.4
      The type of *format* was changed from ``char *``.


.. c:function:: PyObject* PyObject_CallMethod(PyObject *obj, const char *name, const char *format, ...)

   Call the method named *name* of object *obj* with a variable number of C
   arguments.  The C arguments are described by a :c:func:`Py_BuildValue` format
   string that should  produce a tuple.

   The format can be *NULL*, indicating that no arguments are provided.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression:
   ``obj.name(arg1, arg2, ...)``.

   Note that if you only pass :c:type:`PyObject \*` args,
   :c:func:`PyObject_CallMethodObjArgs` is a faster alternative.

   .. versionchanged:: 3.4
      The types of *name* and *format* were changed from ``char *``.


.. c:function:: PyObject* PyObject_CallFunctionObjArgs(PyObject *callable, ..., NULL)

   Call a callable Python object *callable*, with a variable number of
   :c:type:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression:
   ``callable(arg1, arg2, ...)``.


.. c:function:: PyObject* PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ..., NULL)

   Calls a method of the Python object *obj*, where the name of the method is given as a
   Python string object in *name*.  It is called with a variable number of
   :c:type:`PyObject\*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.


.. c:function:: PyObject* _PyObject_CallMethodNoArgs(PyObject *obj, PyObject *name)

   Call a method of the Python object *obj* without arguments,
   where the name of the method is given as a Python string object in *name*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. versionadded:: 3.9


.. c:function:: PyObject* _PyObject_CallMethodOneArg(PyObject *obj, PyObject *name, PyObject *arg)

   Call a method of the Python object *obj* with a single positional argument
   *arg*, where the name of the method is given as a Python string object in
   *name*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. versionadded:: 3.9


.. c:function:: PyObject* _PyObject_Vectorcall(PyObject *callable, PyObject *const *args, size_t nargsf, PyObject *kwnames)

   Call a callable Python object *callable*, using
   :c:data:`vectorcall <PyTypeObject.tp_vectorcall_offset>` if possible.

   *args* is a C array with the positional arguments.

   *nargsf* is the number of positional arguments plus optionally the flag
   :const:`PY_VECTORCALL_ARGUMENTS_OFFSET` (see below).
   To get actual number of arguments, use
   :c:func:`PyVectorcall_NARGS(nargsf) <PyVectorcall_NARGS>`.

   *kwnames* can be either NULL (no keyword arguments) or a tuple of keyword
   names, which must be strings. In the latter case, the values of the keyword
   arguments are stored in *args* after the positional arguments.
   The number of keyword arguments does not influence *nargsf*.

   *kwnames* must contain only objects of type ``str`` (not a subclass),
   and all keys must be unique.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This uses the vectorcall protocol if the callable supports it;
   otherwise, the arguments are converted to use
   :c:member:`~PyTypeObject.tp_call`.

   .. note::

      This function is provisional and expected to become public in Python 3.9,
      with a different name and, possibly, changed semantics.
      If you use the function, plan for updating your code for Python 3.9.

   .. versionadded:: 3.8

.. c:var:: PY_VECTORCALL_ARGUMENTS_OFFSET

   If set in a vectorcall *nargsf* argument, the callee is allowed to
   temporarily change ``args[-1]``. In other words, *args* points to
   argument 1 (not 0) in the allocated vector.
   The callee must restore the value of ``args[-1]`` before returning.

   For :c:func:`_PyObject_VectorcallMethod`, this flag means instead that
   ``args[0]`` may be changed.

   Whenever they can do so cheaply (without additional allocation), callers
   are encouraged to use :const:`PY_VECTORCALL_ARGUMENTS_OFFSET`.
   Doing so will allow callables such as bound methods to make their onward
   calls (which include a prepended *self* argument) cheaply.

   .. versionadded:: 3.8

.. c:function:: Py_ssize_t PyVectorcall_NARGS(size_t nargsf)

   Given a vectorcall *nargsf* argument, return the actual number of
   arguments.
   Currently equivalent to ``nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET``.

   .. versionadded:: 3.8

.. c:function:: PyObject* _PyObject_FastCallDict(PyObject *callable, PyObject *const *args, size_t nargsf, PyObject *kwdict)

   Same as :c:func:`_PyObject_Vectorcall` except that the keyword arguments
   are passed as a dictionary in *kwdict*. This may be *NULL* if there
   are no keyword arguments.

   For callables supporting :c:data:`vectorcall <PyTypeObject.tp_vectorcall_offset>`,
   the arguments are internally converted to the vectorcall convention.
   Therefore, this function adds some overhead compared to
   :c:func:`_PyObject_Vectorcall`.
   It should only be used if the caller already has a dictionary ready to use.

   .. note::

      This function is provisional and expected to become public in Python 3.9,
      with a different name and, possibly, changed semantics.
      If you use the function, plan for updating your code for Python 3.9.

   .. versionadded:: 3.8

.. c:function:: PyObject* _PyObject_VectorcallMethod(PyObject *name, PyObject *const *args, size_t nargsf, PyObject *kwnames)

   Call a method using the vectorcall calling convention. The name of the method
   is given as Python string *name*. The object whose method is called is
   *args[0]* and the *args* array starting at *args[1]* represents the arguments
   of the call. There must be at least one positional argument.
   *nargsf* is the number of positional arguments including *args[0]*,
   plus :const:`PY_VECTORCALL_ARGUMENTS_OFFSET` if the value of ``args[0]`` may
   temporarily be changed. Keyword arguments can be passed just like in
   :c:func:`_PyObject_Vectorcall`.

   If the object has the :const:`Py_TPFLAGS_METHOD_DESCRIPTOR` feature,
   this will actually call the unbound method object with the full
   *args* vector as arguments.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. versionadded:: 3.9
