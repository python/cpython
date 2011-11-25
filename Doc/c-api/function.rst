.. highlightlang:: c

.. _function-objects:

Function Objects
----------------

.. index:: object: function

There are a few functions specific to Python functions.


.. c:type:: PyFunctionObject

   The C structure used for functions.


.. c:var:: PyTypeObject PyFunction_Type

   .. index:: single: MethodType (in module types)

   This is an instance of :c:type:`PyTypeObject` and represents the Python function
   type.  It is exposed to Python programmers as ``types.FunctionType``.


.. c:function:: int PyFunction_Check(PyObject *o)

   Return true if *o* is a function object (has type :c:data:`PyFunction_Type`).
   The parameter must not be *NULL*.


.. c:function:: PyObject* PyFunction_New(PyObject *code, PyObject *globals)

   Return a new function object associated with the code object *code*. *globals*
   must be a dictionary with the global variables accessible to the function.

   The function's docstring, name and *__module__* are retrieved from the code
   object, the argument defaults and closure are set to *NULL*.


.. c:function:: PyObject* PyFunction_NewWithQualName(PyObject *code, PyObject *globals, PyObject *qualname)

   As :c:func:`PyFunction_New`, but also allows to set the function object's
   ``__qualname__`` attribute.  *qualname* should be a unicode object or NULL;
   if NULL, the ``__qualname__`` attribute is set to the same value as its
   ``__name__`` attribute.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyFunction_GetCode(PyObject *op)

   Return the code object associated with the function object *op*.


.. c:function:: PyObject* PyFunction_GetGlobals(PyObject *op)

   Return the globals dictionary associated with the function object *op*.


.. c:function:: PyObject* PyFunction_GetModule(PyObject *op)

   Return the *__module__* attribute of the function object *op*. This is normally
   a string containing the module name, but can be set to any other object by
   Python code.


.. c:function:: PyObject* PyFunction_GetDefaults(PyObject *op)

   Return the argument default values of the function object *op*. This can be a
   tuple of arguments or *NULL*.


.. c:function:: int PyFunction_SetDefaults(PyObject *op, PyObject *defaults)

   Set the argument default values for the function object *op*. *defaults* must be
   *Py_None* or a tuple.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. c:function:: PyObject* PyFunction_GetClosure(PyObject *op)

   Return the closure associated with the function object *op*. This can be *NULL*
   or a tuple of cell objects.


.. c:function:: int PyFunction_SetClosure(PyObject *op, PyObject *closure)

   Set the closure associated with the function object *op*. *closure* must be
   *Py_None* or a tuple of cell objects.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. c:function:: PyObject *PyFunction_GetAnnotations(PyObject *op)

   Return the annotations of the function object *op*. This can be a
   mutable dictionary or *NULL*.


.. c:function:: int PyFunction_SetAnnotations(PyObject *op, PyObject *annotations)

   Set the annotations for the function object *op*. *annotations*
   must be a dictionary or *Py_None*.

   Raises :exc:`SystemError` and returns ``-1`` on failure.
