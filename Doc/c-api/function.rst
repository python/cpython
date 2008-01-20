.. highlightlang:: c

.. _function-objects:

Function Objects
----------------

.. index:: object: function

There are a few functions specific to Python functions.


.. ctype:: PyFunctionObject

   The C structure used for functions.


.. cvar:: PyTypeObject PyFunction_Type

   .. index:: single: MethodType (in module types)

   This is an instance of :ctype:`PyTypeObject` and represents the Python function
   type.  It is exposed to Python programmers as ``types.FunctionType``.


.. cfunction:: int PyFunction_Check(PyObject *o)

   Return true if *o* is a function object (has type :cdata:`PyFunction_Type`).
   The parameter must not be *NULL*.


.. cfunction:: PyObject* PyFunction_New(PyObject *code, PyObject *globals)

   Return a new function object associated with the code object *code*. *globals*
   must be a dictionary with the global variables accessible to the function.

   The function's docstring, name and *__module__* are retrieved from the code
   object, the argument defaults and closure are set to *NULL*.


.. cfunction:: PyObject* PyFunction_GetCode(PyObject *op)

   Return the code object associated with the function object *op*.


.. cfunction:: PyObject* PyFunction_GetGlobals(PyObject *op)

   Return the globals dictionary associated with the function object *op*.


.. cfunction:: PyObject* PyFunction_GetModule(PyObject *op)

   Return the *__module__* attribute of the function object *op*. This is normally
   a string containing the module name, but can be set to any other object by
   Python code.


.. cfunction:: PyObject* PyFunction_GetDefaults(PyObject *op)

   Return the argument default values of the function object *op*. This can be a
   tuple of arguments or *NULL*.


.. cfunction:: int PyFunction_SetDefaults(PyObject *op, PyObject *defaults)

   Set the argument default values for the function object *op*. *defaults* must be
   *Py_None* or a tuple.

   Raises :exc:`SystemError` and returns ``-1`` on failure.


.. cfunction:: PyObject* PyFunction_GetClosure(PyObject *op)

   Return the closure associated with the function object *op*. This can be *NULL*
   or a tuple of cell objects.


.. cfunction:: int PyFunction_SetClosure(PyObject *op, PyObject *closure)

   Set the closure associated with the function object *op*. *closure* must be
   *Py_None* or a tuple of cell objects.

   Raises :exc:`SystemError` and returns ``-1`` on failure.
