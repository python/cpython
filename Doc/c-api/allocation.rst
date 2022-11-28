.. highlight:: c

.. _allocating-objects:

Allocating Objects on the Heap
==============================


.. c:function:: PyObject* _PyObject_New(PyTypeObject *type)


.. c:function:: PyVarObject* _PyObject_NewVar(PyTypeObject *type, Py_ssize_t size)


.. c:function:: PyObject* PyObject_Init(PyObject *op, PyTypeObject *type)

   Initialize a newly allocated object *op* with its type and initial
   reference.  Returns the initialized object.  If *type* indicates that the
   object participates in the cyclic garbage detector, it is added to the
   detector's set of observed objects. Other fields of the object are not
   affected.


.. c:function:: PyVarObject* PyObject_InitVar(PyVarObject *op, PyTypeObject *type, Py_ssize_t size)

   This does everything :c:func:`PyObject_Init` does, and also initializes the
   length information for a variable-size object.


.. c:function:: TYPE* PyObject_New(TYPE, PyTypeObject *type)

   Allocate a new Python object using the C structure type *TYPE* and the
   Python type object *type*.  Fields not defined by the Python object header
   are not initialized; the object's reference count will be one.  The size of
   the memory allocation is determined from the :c:member:`~PyTypeObject.tp_basicsize` field of
   the type object.


.. c:function:: TYPE* PyObject_NewVar(TYPE, PyTypeObject *type, Py_ssize_t size)

   Allocate a new Python object using the C structure type *TYPE* and the
   Python type object *type*.  Fields not defined by the Python object header
   are not initialized.  The allocated memory allows for the *TYPE* structure
   plus *size* fields of the size given by the :c:member:`~PyTypeObject.tp_itemsize` field of
   *type*.  This is useful for implementing objects like tuples, which are
   able to determine their size at construction time.  Embedding the array of
   fields into the same allocation decreases the number of allocations,
   improving the memory management efficiency.


.. c:function:: void PyObject_Del(void *op)

   Releases memory allocated to an object using :c:func:`PyObject_New` or
   :c:func:`PyObject_NewVar`.  This is normally called from the
   :c:member:`~PyTypeObject.tp_dealloc` handler specified in the object's type.  The fields of
   the object should not be accessed after this call as the memory is no
   longer a valid Python object.


.. c:var:: PyObject _Py_NoneStruct

   Object which is visible in Python as ``None``.  This should only be accessed
   using the :c:macro:`Py_None` macro, which evaluates to a pointer to this
   object.


.. seealso::

   :c:func:`PyModule_Create`
      To allocate and create extension modules.

