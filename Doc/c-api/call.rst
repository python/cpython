.. highlight:: c

.. _call:

Call Protocol
=============

CPython supports two different calling protocols:
*tp_call* and vectorcall.

The *tp_call* Protocol
----------------------

Classes can implement callables by setting :c:member:`~PyTypeObject.tp_call`.

A call is made using a tuple for the positional arguments
and a dict for the keyword arguments, just like ``f(*args, **kwargs)``
in the Python language.
*args* must be non-NULL
(use an empty tuple if there are no arguments)
but *kwargs* may be *NULL* if there are no keyword arguments.

This convention is not only used by *tp_call*:
:c:member:`~PyTypeObject.tp_new` and :c:member:`~PyTypeObject.tp_init`
also pass arguments this way
(apart from the special handling of *subtype* and *self*).

For making a call this way, use :c:func:`PyObject_Call`.

.. _vectorcall:

The Vectorcall Protocol
-----------------------

.. versionadded:: 3.8

The vectorcall protocol was introduced in :pep:`590`
for making calls more efficient.

Classes can implement the vectorcall protocol by enabling the
:const:`_Py_TPFLAGS_HAVE_VECTORCALL` flag and setting
:c:member:`~PyTypeObject.tp_vectorcall_offset` to the offset inside the
object structure where a *vectorcallfunc* appears.
This is a function with the following signature:

.. c:type:: PyObject *(*vectorcallfunc)(PyObject *callable, PyObject *const *args, size_t nargsf, PyObject *kwnames)

   - *callable* is the object being called.
   - *args* is a C array consisting of the positional arguments followed by the
     values of the keyword arguments.
     This can be *NULL* if there are no arguments.
   - *nargsf* is the number of positional arguments plus possibly the
     :const:`PY_VECTORCALL_ARGUMENTS_OFFSET` flag.
     To get the actual number of positional arguments from *nargsf*,
     use :c:func:`PyVectorcall_NARGS`.
   - *kwnames* is a tuple containing the names of the keyword arguments;
     in other words, the keys of the kwargs dict.
     These names must be strings (instances of ``str`` or a subclass)
     and they must be unique.
     If there are no keyword arguments, then *kwnames* can instead be *NULL*.

.. c:var:: PY_VECTORCALL_ARGUMENTS_OFFSET

   If this flag is set in a vectorcall *nargsf* argument, the callee is allowed
   to temporarily change ``args[-1]``. In other words, *args* points to
   argument 1 (not 0) in the allocated vector.
   The callee must restore the value of ``args[-1]`` before returning.

   For :c:func:`_PyObject_VectorcallMethod`, this flag means instead that
   ``args[0]`` may be changed.

   Whenever they can do so cheaply (without additional allocation), callers
   are encouraged to use :const:`PY_VECTORCALL_ARGUMENTS_OFFSET`.
   Doing so will allow callables such as bound methods to make their onward
   calls (which include a prepended *self* argument) very efficiently.

.. warning::

   As rule of thumb, CPython will internally use the vectorcall
   protocol if the callable supports it. However, this is not a hard rule,
   *tp_call* may be used instead.
   Therefore, a class supporting vectorcall must also implement
   :c:member:`~PyTypeObject.tp_call`.
   Moreover, the callable must behave the same
   regardless of which protocol is used.
   The recommended way to achieve this is by setting
   :c:member:`~PyTypeObject.tp_call` to :c:func:`PyVectorcall_Call`.

   An object should not implement vectorcall if that would be slower
   than *tp_call*. For example, if the callee needs to convert
   the arguments to an args tuple and kwargs dict anyway, then there is no point
   in implementing vectorcall.

For making a vectorcall, use :c:func:`_PyObject_Vectorcall`.

Recursion Control
-----------------

When using *tp_call*, callees do not need to worry about
:ref:`recursion <recursion>`: CPython uses
:c:func:`Py_EnterRecursiveCall` and :c:func:`Py_LeaveRecursiveCall`
for calls made using *tp_call*.

For efficiency, this is not the case for calls done using vectorcall:
the callee should use *Py_EnterRecursiveCall* and *Py_LeaveRecursiveCall*
if needed.

C API Functions
---------------

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

   *args* must not be *NULL*; use an empty tuple if no arguments are needed.
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
   string that should produce a tuple.

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
   :c:type:`PyObject \*` arguments.  The arguments are provided as a variable number
   of parameters followed by *NULL*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   This is the equivalent of the Python expression:
   ``callable(arg1, arg2, ...)``.


.. c:function:: PyObject* PyObject_CallMethodObjArgs(PyObject *obj, PyObject *name, ..., NULL)

   Call a method of the Python object *obj*, where the name of the method is given as a
   Python string object in *name*.  It is called with a variable number of
   :c:type:`PyObject \*` arguments.  The arguments are provided as a variable number
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

   Call a callable Python object *callable*.
   The arguments are the same as for :c:type:`vectorcallfunc`.
   If *callable* supports vectorcall_, this directly calls
   the vectorcall function stored in *callable*.

   Return the result of the call on success, or raise an exception and return
   *NULL* on failure.

   .. note::

      This function is provisional and expected to become public in Python 3.9,
      with a different name and, possibly, changed semantics.
      If you use the function, plan for updating your code for Python 3.9.

   .. versionadded:: 3.8

.. c:function:: PyObject* _PyObject_FastCallDict(PyObject *callable, PyObject *const *args, size_t nargsf, PyObject *kwdict)

   Call *callable* with positional arguments passed exactly as in the vectorcall_ protocol,
   but with keyword arguments passed as a dictionary *kwdict*.
   The *args* array contains only the positional arguments.

   Regardless of which protocol is used internally,
   a conversion of arguments needs to be done.
   Therefore, this function should only be used if the caller
   already has a dictionary ready to use for the keyword arguments,
   but not a tuple for the positional arguments.

   .. note::

      This function is provisional and expected to become public in Python 3.9,
      with a different name and, possibly, changed semantics.
      If you use the function, plan for updating your code for Python 3.9.

   .. versionadded:: 3.8

.. c:function:: Py_ssize_t PyVectorcall_NARGS(size_t nargsf)

   Given a vectorcall *nargsf* argument, return the actual number of
   arguments.
   Currently equivalent to::

      (Py_ssize_t)(nargsf & ~PY_VECTORCALL_ARGUMENTS_OFFSET)

   However, the function ``PyVectorcall_NARGS`` should be used to allow
   for future extensions.

   .. versionadded:: 3.8

.. c:function:: vectorcallfunc _PyVectorcall_Function(PyObject *op)

   If *op* does not support the vectorcall protocol (either because the type
   does not or because the specific instance does not), return *NULL*.
   Otherwise, return the vectorcall function pointer stored in *op*.
   This function never raises an exception.

   This is mostly useful to check whether or not *op* supports vectorcall,
   which can be done by checking ``_PyVectorcall_Function(op) != NULL``.

   .. versionadded:: 3.8

.. c:function:: PyObject* PyVectorcall_Call(PyObject *callable, PyObject *tuple, PyObject *dict)

   Call *callable*'s :c:type:`vectorcallfunc` with positional and keyword
   arguments given in a tuple and dict, respectively.

   This is a specialized function, intended to be put in the
   :c:member:`~PyTypeObject.tp_call` slot or be used in an implementation of ``tp_call``.
   It does not check the :const:`_Py_TPFLAGS_HAVE_VECTORCALL` flag
   and it does not fall back to ``tp_call``.

   .. versionadded:: 3.8

.. c:function:: PyObject* _PyObject_VectorcallMethod(PyObject *name, PyObject *const *args, size_t nargsf, PyObject *kwnames)

   Call a method using the vectorcall calling convention. The name of the method
   is given as a Python string *name*. The object whose method is called is
   *args[0]*, and the *args* array starting at *args[1]* represents the arguments
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
