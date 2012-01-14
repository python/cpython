.. highlightlang:: c

.. _allocating-objects:

Allocating Objects on the Heap
==============================


.. c:function:: PyObject* _PyObject_New(PyTypeObject *type)


.. c:function:: PyVarObject* _PyObject_NewVar(PyTypeObject *type, Py_ssize_t size)

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: void _PyObject_Del(PyObject *op)


.. c:function:: PyObject* PyObject_Init(PyObject *op, PyTypeObject *type)

   Initialize a newly-allocated object *op* with its type and initial
   reference.  Returns the initialized object.  If *type* indicates that the
   object participates in the cyclic garbage detector, it is added to the
   detector's set of observed objects. Other fields of the object are not
   affected.


.. c:function:: PyVarObject* PyObject_InitVar(PyVarObject *op, PyTypeObject *type, Py_ssize_t size)

   This does everything :c:func:`PyObject_Init` does, and also initializes the
   length information for a variable-size object.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: TYPE* PyObject_New(TYPE, PyTypeObject *type)

   Allocate a new Python object using the C structure type *TYPE* and the
   Python type object *type*.  Fields not defined by the Python object header
   are not initialized; the object's reference count will be one.  The size of
   the memory allocation is determined from the :attr:`tp_basicsize` field of
   the type object.


.. c:function:: TYPE* PyObject_NewVar(TYPE, PyTypeObject *type, Py_ssize_t size)

   Allocate a new Python object using the C structure type *TYPE* and the
   Python type object *type*.  Fields not defined by the Python object header
   are not initialized.  The allocated memory allows for the *TYPE* structure
   plus *size* fields of the size given by the :attr:`tp_itemsize` field of
   *type*.  This is useful for implementing objects like tuples, which are
   able to determine their size at construction time.  Embedding the array of
   fields into the same allocation decreases the number of allocations,
   improving the memory management efficiency.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *size*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: void PyObject_Del(PyObject *op)

   Releases memory allocated to an object using :c:func:`PyObject_New` or
   :c:func:`PyObject_NewVar`.  This is normally called from the
   :attr:`tp_dealloc` handler specified in the object's type.  The fields of
   the object should not be accessed after this call as the memory is no
   longer a valid Python object.


.. c:function:: PyObject* Py_InitModule(char *name, PyMethodDef *methods)

   Create a new module object based on a name and table of functions,
   returning the new module object.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the
      *methods* argument.


.. c:function:: PyObject* Py_InitModule3(char *name, PyMethodDef *methods, char *doc)

   Create a new module object based on a name and table of functions,
   returning the new module object.  If *doc* is non-*NULL*, it will be used
   to define the docstring for the module.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the
      *methods* argument.


.. c:function:: PyObject* Py_InitModule4(char *name, PyMethodDef *methods, char *doc, PyObject *self, int apiver)

   Create a new module object based on a name and table of functions,
   returning the new module object.  If *doc* is non-*NULL*, it will be used
   to define the docstring for the module.  If *self* is non-*NULL*, it will
   passed to the functions of the module as their (otherwise *NULL*) first
   parameter.  (This was added as an experimental feature, and there are no
   known uses in the current version of Python.)  For *apiver*, the only value
   which should be passed is defined by the constant
   :const:`PYTHON_API_VERSION`.

   .. note::

      Most uses of this function should probably be using the
      :c:func:`Py_InitModule3` instead; only use this if you are sure you need
      it.

   .. versionchanged:: 2.3
      Older versions of Python did not support *NULL* as the value for the
      *methods* argument.


.. c:var:: PyObject _Py_NoneStruct

   Object which is visible in Python as ``None``.  This should only be
   accessed using the ``Py_None`` macro, which evaluates to a pointer to this
   object.
