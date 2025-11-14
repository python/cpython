.. highlight:: c

.. _allocating-objects:

Allocating Objects on the Heap
==============================


.. c:function:: PyObject* _PyObject_New(PyTypeObject *type)


.. c:function:: PyVarObject* _PyObject_NewVar(PyTypeObject *type, Py_ssize_t size)


.. c:function:: PyObject* PyObject_Init(PyObject *op, PyTypeObject *type)

   Initialize a newly allocated object *op* with its type and initial
   reference.  Returns the initialized object.  Other fields of the object are
   not initialized.  Despite its name, this function is unrelated to the
   object's :meth:`~object.__init__` method (:c:member:`~PyTypeObject.tp_init`
   slot).  Specifically, this function does **not** call the object's
   :meth:`!__init__` method.

   In general, consider this function to be a low-level routine. Use
   :c:member:`~PyTypeObject.tp_alloc` where possible.
   For implementing :c:member:`!tp_alloc` for your type, prefer
   :c:func:`PyType_GenericAlloc` or :c:func:`PyObject_New`.

   .. note::

      This function only initializes the object's memory corresponding to the
      initial :c:type:`PyObject` structure.  It does not zero the rest.


.. c:function:: PyVarObject* PyObject_InitVar(PyVarObject *op, PyTypeObject *type, Py_ssize_t size)

   This does everything :c:func:`PyObject_Init` does, and also initializes the
   length information for a variable-size object.

   .. note::

      This function only initializes some of the object's memory.  It does not
      zero the rest.


.. c:macro:: PyObject_New(TYPE, typeobj)

   Allocates a new Python object using the C structure type *TYPE* and the
   Python type object *typeobj* (``PyTypeObject*``) by calling
   :c:func:`PyObject_Malloc` to allocate memory and initializing it like
   :c:func:`PyObject_Init`.  The caller will own the only reference to the
   object (i.e. its reference count will be one).

   Avoid calling this directly to allocate memory for an object; call the type's
   :c:member:`~PyTypeObject.tp_alloc` slot instead.

   When populating a type's :c:member:`~PyTypeObject.tp_alloc` slot,
   :c:func:`PyType_GenericAlloc` is preferred over a custom function that
   simply calls this macro.

   This macro does not call :c:member:`~PyTypeObject.tp_alloc`,
   :c:member:`~PyTypeObject.tp_new` (:meth:`~object.__new__`), or
   :c:member:`~PyTypeObject.tp_init` (:meth:`~object.__init__`).

   This cannot be used for objects with :c:macro:`Py_TPFLAGS_HAVE_GC` set in
   :c:member:`~PyTypeObject.tp_flags`; use :c:macro:`PyObject_GC_New` instead.

   Memory allocated by this macro must be freed with :c:func:`PyObject_Free`
   (usually called via the object's :c:member:`~PyTypeObject.tp_free` slot).

   .. note::

      The returned memory is not guaranteed to have been completely zeroed
      before it was initialized.

   .. note::

      This macro does not construct a fully initialized object of the given
      type; it merely allocates memory and prepares it for further
      initialization by :c:member:`~PyTypeObject.tp_init`.  To construct a
      fully initialized object, call *typeobj* instead.  For example::

         PyObject *foo = PyObject_CallNoArgs((PyObject *)&PyFoo_Type);

   .. seealso::

      * :c:func:`PyObject_Free`
      * :c:macro:`PyObject_GC_New`
      * :c:func:`PyType_GenericAlloc`
      * :c:member:`~PyTypeObject.tp_alloc`


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

   Avoid calling this directly to allocate memory for an object; call the type's
   :c:member:`~PyTypeObject.tp_alloc` slot instead.

   When populating a type's :c:member:`~PyTypeObject.tp_alloc` slot,
   :c:func:`PyType_GenericAlloc` is preferred over a custom function that
   simply calls this macro.

   This cannot be used for objects with :c:macro:`Py_TPFLAGS_HAVE_GC` set in
   :c:member:`~PyTypeObject.tp_flags`; use :c:macro:`PyObject_GC_NewVar`
   instead.

   Memory allocated by this function must be freed with :c:func:`PyObject_Free`
   (usually called via the object's :c:member:`~PyTypeObject.tp_free` slot).

   .. note::

      The returned memory is not guaranteed to have been completely zeroed
      before it was initialized.

   .. note::

      This macro does not construct a fully initialized object of the given
      type; it merely allocates memory and prepares it for further
      initialization by :c:member:`~PyTypeObject.tp_init`.  To construct a
      fully initialized object, call *typeobj* instead.  For example::

         PyObject *list_instance = PyObject_CallNoArgs((PyObject *)&PyList_Type);

   .. seealso::

      * :c:func:`PyObject_Free`
      * :c:macro:`PyObject_GC_NewVar`
      * :c:func:`PyType_GenericAlloc`
      * :c:member:`~PyTypeObject.tp_alloc`


.. c:var:: PyObject _Py_NoneStruct

   Object which is visible in Python as ``None``.  This should only be accessed
   using the :c:macro:`Py_None` macro, which evaluates to a pointer to this
   object.


.. seealso::

   :ref:`moduleobjects`
      To allocate and create extension modules.


Deprecated aliases
^^^^^^^^^^^^^^^^^^

These are :term:`soft deprecated` aliases to existing functions and macros.
They exist solely for backwards compatibility.


.. list-table::
   :widths: auto
   :header-rows: 1

   * * Deprecated alias
     * Function
   * * .. c:macro:: PyObject_NEW(type, typeobj)
     * :c:macro:`PyObject_New`
   * * .. c:macro:: PyObject_NEW_VAR(type, typeobj, n)
     * :c:macro:`PyObject_NewVar`
   * * .. c:macro:: PyObject_INIT(op, typeobj)
     * :c:func:`PyObject_Init`
   * * .. c:macro:: PyObject_INIT_VAR(op, typeobj, n)
     * :c:func:`PyObject_InitVar`
   * * .. c:macro:: PyObject_MALLOC(n)
     * :c:func:`PyObject_Malloc`
   * * .. c:macro:: PyObject_REALLOC(p, n)
     * :c:func:`PyObject_Realloc`
   * * .. c:macro:: PyObject_FREE(p)
     * :c:func:`PyObject_Free`
   * * .. c:macro:: PyObject_DEL(p)
     * :c:func:`PyObject_Free`
   * * .. c:macro:: PyObject_Del(p)
     * :c:func:`PyObject_Free`
