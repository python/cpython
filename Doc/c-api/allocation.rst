.. highlight:: c

.. _allocating-objects:

Allocating Objects on the Heap
==============================


.. c:function:: PyObject* _PyObject_New(PyTypeObject *type)


.. c:function:: PyVarObject* _PyObject_NewVar(PyTypeObject *type, Py_ssize_t size)


.. c:function:: PyObject* PyObject_Init(PyObject *op, PyTypeObject *type)

   Initialize a newly allocated object *op* with its type and initial
   reference.  Returns the initialized object.  Other fields of the object are
   not initialized.  Specifically, this function does **not** call the object's
   :meth:`~object.__init__` method (:c:member:`~PyTypeObject.tp_init` slot).

   .. warning::

      This function does not guarantee that the memory will be completely
      zeroed before it is initialized.  Fields that are not initialized by this
      function will have indeterminate values, which might include sensitive
      data from previously destroyed objects (e.g., secret keys).


.. c:function:: PyVarObject* PyObject_InitVar(PyVarObject *op, PyTypeObject *type, Py_ssize_t size)

   This does everything :c:func:`PyObject_Init` does, and also initializes the
   length information for a variable-size object.


.. c:macro:: PyObject_New(TYPE, typeobj)

   Allocates a new Python object using the C structure type *TYPE* and the
   Python type object *typeobj* (``PyTypeObject*``) by calling
   :c:func:`PyObject_Malloc` to allocate memory and initializing it like
   :c:func:`PyObject_Init`.  The caller will own the only reference to the
   object (i.e. its reference count will be one).  The size of the memory
   allocation is determined from the :c:member:`~PyTypeObject.tp_basicsize`
   field of the type object.

   For a type's :c:member:`~PyTypeObject.tp_alloc` slot,
   :c:func:`PyType_GenericAlloc` is generally preferred over a custom function
   that simply calls this macro.

   This macro does not call :c:member:`~PyTypeObject.tp_alloc`,
   :c:member:`~PyTypeObject.tp_new` (:meth:`~object.__new__`), or
   :c:member:`~PyTypeObject.tp_init` (:meth:`~object.__init__`).

   This macro should not be used for objects with :c:macro:`Py_TPFLAGS_HAVE_GC`
   set in :c:member:`~PyTypeObject.tp_flags`; use :c:macro:`PyObject_GC_New`
   instead.

   Memory allocated by this function must be freed with
   :c:func:`PyObject_Free`.

   .. warning::

      The returned memory is not guaranteed to have been completely zeroed
      before it was initialized.  Fields that were not initialized by this
      function will have indeterminate values, which might include sensitive
      data from previously destroyed objects (e.g., secret keys).

   .. warning::

      This macro does not construct a fully initialized object of the given
      type; it merely allocates memory and prepares it for further
      initialization by :c:member:`~PyTypeObject.tp_init`.  To construct a
      fully initialized object, call *typeobj* instead.  For example::

         PyObject *foo = PyObject_CallNoArgs((PyObject *)&PyFoo_Type);


.. c:macro:: PyObject_NewVar(TYPE, typeobj, size)

   Like :c:macro:`PyObject_New` except:

   * It allocates enough memory for the *TYPE* structure plus *size*
     (``Py_ssize_t``) fields of the size given by the
     :c:member:`~PyTypeObject.tp_itemsize` field of *typeobj*.
   * The memory is initialized like :c:func:`PyObject_InitVar`.

   This is useful for implementing objects like tuples, which are able to
   determine their size at construction time.  Embedding the array of fields
   into the same allocation decreases the number of allocations, improving the
   memory management efficiency.

   This should not be used for objects with :c:macro:`Py_TPFLAGS_HAVE_GC` set
   in :c:member:`~PyTypeObject.tp_flags`; use :c:macro:`PyObject_GC_NewVar`
   instead.

   Memory allocated by this function must be freed with
   :c:func:`PyObject_Free`.


.. c:function:: void PyObject_Del(void *op)

   Same as :c:func:`PyObject_Free`.

.. c:var:: PyObject _Py_NoneStruct

   Object which is visible in Python as ``None``.  This should only be accessed
   using the :c:macro:`Py_None` macro, which evaluates to a pointer to this
   object.


.. seealso::

   :c:func:`PyModule_Create`
      To allocate and create extension modules.

