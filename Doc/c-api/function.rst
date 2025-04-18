.. highlight:: c

.. _function-objects:

Function Objects
----------------

.. index:: pair: object; function

There are a few functions specific to Python functions.


.. c:type:: PyFunctionObject

   The C structure used for functions.


.. c:var:: PyTypeObject PyFunction_Type

   .. index:: single: MethodType (in module types)

   This is an instance of :c:type:`PyTypeObject` and represents the Python function
   type.  It is exposed to Python programmers as ``types.FunctionType``.


.. c:function:: int PyFunction_Check(PyObject *o)

   Return true if *o* is a function object (has type :c:data:`PyFunction_Type`).
   The parameter must not be ``NULL``.  This function always succeeds.


.. c:function:: PyObject* PyFunction_New(PyObject *code, PyObject *globals)

   Return a new function object associated with the code object *code*. *globals*
   must be a dictionary with the global variables accessible to the function.

   The function's docstring and name are retrieved from the code object.
   :attr:`~function.__module__`
   is retrieved from *globals*. The argument defaults, annotations and closure are
   set to ``NULL``. :attr:`~function.__qualname__` is set to the same value as
   the code object's :attr:`~codeobject.co_qualname` field.


.. c:function:: PyObject* PyFunction_NewWithQualName(PyObject *code, PyObject *globals, PyObject *qualname)

   As :c:func:`PyFunction_New`, but also allows setting the function object's
   :attr:`~function.__qualname__` attribute.
   *qualname* should be a unicode object or ``NULL``;
   if ``NULL``, the :attr:`!__qualname__` attribute is set to the same value as
   the code object's :attr:`~codeobject.co_qualname` field.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyFunction_GetCode(PyObject *op)

   Return the code object associated with the function object *op*.


.. c:function:: PyObject* PyFunction_GetGlobals(PyObject *op)

   Return the globals dictionary associated with the function object *op*.


.. c:function:: PyObject* PyFunction_GetModule(PyObject *op)

   Return a :term:`borrowed reference` to the :attr:`~function.__module__`
   attribute of the :ref:`function object <user-defined-funcs>` *op*.
   It can be *NULL*.

   This is normally a :class:`string <str>` containing the module name,
   but can be set to any other object by Python code.


.. c:function:: PyObject* PyFunction_GetDefaults(PyObject *op)

   Return the argument default values of the function object *op*. This can be a
   tuple of arguments or ``NULL``.


.. c:function:: int PyFunction_SetDefaults(PyObject *op, PyObject *defaults)

   Set the argument default values for the function object *op*. *defaults* must be
   ``Py_None`` or a tuple.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. c:function:: void PyFunction_SetVectorcall(PyFunctionObject *func, vectorcallfunc vectorcall)

   Set the vectorcall field of a given function object *func*.

   Warning: extensions using this API must preserve the behavior
   of the unaltered (default) vectorcall function!

   .. versionadded:: 3.12

.. c:function:: PyObject* PyFunction_GetClosure(PyObject *op)

   Return the closure associated with the function object *op*. This can be ``NULL``
   or a tuple of cell objects.


.. c:function:: int PyFunction_SetClosure(PyObject *op, PyObject *closure)

   Set the closure associated with the function object *op*. *closure* must be
   ``Py_None`` or a tuple of cell objects.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. c:function:: PyObject *PyFunction_GetAnnotations(PyObject *op)

   Return the annotations of the function object *op*. This can be a
   mutable dictionary or ``NULL``.


.. c:function:: int PyFunction_SetAnnotations(PyObject *op, PyObject *annotations)

   Set the annotations for the function object *op*. *annotations*
   must be a dictionary or ``Py_None``.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. c:function:: int PyFunction_AddWatcher(PyFunction_WatchCallback callback)

   Register *callback* as a function watcher for the current interpreter.
   Return an ID which may be passed to :c:func:`PyFunction_ClearWatcher`.
   In case of error (e.g. no more watcher IDs available),
   return ``-1`` and set an exception.

   .. versionadded:: 3.12


.. c:function:: int PyFunction_ClearWatcher(int watcher_id)

   Clear watcher identified by *watcher_id* previously returned from
   :c:func:`PyFunction_AddWatcher` for the current interpreter.
   Return ``0`` on success, or ``-1`` and set an exception on error
   (e.g.  if the given *watcher_id* was never registered.)

   .. versionadded:: 3.12


.. c:type:: PyFunction_WatchEvent

    Enumeration of possible function watcher events:

    - ``PyFunction_EVENT_CREATE``
    - ``PyFunction_EVENT_DESTROY``
    - ``PyFunction_EVENT_MODIFY_CODE``
    - ``PyFunction_EVENT_MODIFY_DEFAULTS``
    - ``PyFunction_EVENT_MODIFY_KWDEFAULTS``

   .. versionadded:: 3.12


.. c:type:: int (*PyFunction_WatchCallback)(PyFunction_WatchEvent event, PyFunctionObject *func, PyObject *new_value)

   Type of a function watcher callback function.

   If *event* is ``PyFunction_EVENT_CREATE`` or ``PyFunction_EVENT_DESTROY``
   then *new_value* will be ``NULL``. Otherwise, *new_value* will hold a
   :term:`borrowed reference` to the new value that is about to be stored in
   *func* for the attribute that is being modified.

   The callback may inspect but must not modify *func*; doing so could have
   unpredictable effects, including infinite recursion.

   If *event* is ``PyFunction_EVENT_CREATE``, then the callback is invoked
   after `func` has been fully initialized. Otherwise, the callback is invoked
   before the modification to *func* takes place, so the prior state of *func*
   can be inspected. The runtime is permitted to optimize away the creation of
   function objects when possible. In such cases no event will be emitted.
   Although this creates the possibility of an observable difference of
   runtime behavior depending on optimization decisions, it does not change
   the semantics of the Python code being executed.

   If *event* is ``PyFunction_EVENT_DESTROY``,  Taking a reference in the
   callback to the about-to-be-destroyed function will resurrect it, preventing
   it from being freed at this time. When the resurrected object is destroyed
   later, any watcher callbacks active at that time will be called again.

   If the callback sets an exception, it must return ``-1``; this exception will
   be printed as an unraisable exception using :c:func:`PyErr_WriteUnraisable`.
   Otherwise it should return ``0``.

   There may already be a pending exception set on entry to the callback. In
   this case, the callback should return ``0`` with the same exception still
   set. This means the callback may not call any other API that can set an
   exception unless it saves and clears the exception state first, and restores
   it before returning.

   .. versionadded:: 3.12
