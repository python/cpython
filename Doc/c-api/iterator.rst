.. highlight:: c

.. _iterator-objects:

Iterator Objects
----------------

Python provides two general-purpose iterator objects.  The first, a sequence
iterator, works with an arbitrary sequence supporting the :meth:`~object.__getitem__`
method.  The second works with a callable object and a sentinel value, calling
the callable for each item in the sequence, and ending the iteration when the
sentinel value is returned.


.. c:var:: PyTypeObject PySeqIter_Type

   Type object for iterator objects returned by :c:func:`PySeqIter_New` and the
   one-argument form of the :func:`iter` built-in function for built-in sequence
   types.


.. c:function:: int PySeqIter_Check(PyObject *op)

   Return true if the type of *op* is :c:data:`PySeqIter_Type`.  This function
   always succeeds.


.. c:function:: PyObject* PySeqIter_New(PyObject *seq)

   Return an iterator that works with a general sequence object, *seq*.  The
   iteration ends when the sequence raises :exc:`IndexError` for the subscripting
   operation.


.. c:var:: PyTypeObject PyCallIter_Type

   Type object for iterator objects returned by :c:func:`PyCallIter_New` and the
   two-argument form of the :func:`iter` built-in function.


.. c:function:: int PyCallIter_Check(PyObject *op)

   Return true if the type of *op* is :c:data:`PyCallIter_Type`.  This
   function always succeeds.


.. c:function:: PyObject* PyCallIter_New(PyObject *callable, PyObject *sentinel)

   Return a new iterator.  The first parameter, *callable*, can be any Python
   callable object that can be called with no parameters; each call to it should
   return the next item in the iteration.  When *callable* returns a value equal to
   *sentinel*, the iteration will be terminated.


.. _range-objects:

Range Objects
^^^^^^^^^^^^^

.. c:var:: PyTypeObject PyRange_Type

   The type object for :class:`range` objects.


.. c:var:: PyTypeObject PyRangeIter_Type

   The type object for iterators over :class:`range` objects.


.. c:var:: PyTypeObject PyLongRangeIter_Type

   The type object for iterators over :class:`range` objects with large bounds.


.. c:function:: int PyRange_Check(PyObject *o)

   Return true if the object *o* is an instance of a :class:`range` object.
   This function always succeeds.


.. _other-builtin-iterator-types:

Other Builtin Iterator Types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

These are built-in iteration types that are included in Python's C API, but
provide no additional functions. They are here for completeness.

.. c:var:: PyTypeObject PyEnum_Type

   The type object for :class:`enumerate` objects.


.. c:var:: PyTypeObject PyMap_Type

   The type object for :class:`map` objects.


.. c:var:: PyTypeObject PyReversed_Type

   The type object for :class:`reversed` objects.


.. c:var:: PyTypeObject PyZip_Type

   The type object for :class:`zip` objects.

