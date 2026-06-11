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


Range Objects
^^^^^^^^^^^^^

.. c:var:: PyTypeObject PyRange_Type

   The type object for :class:`range` objects.


.. c:function:: int PyRange_Check(PyObject *o)

   Return true if the object *o* is an instance of a :class:`range` object.
   This function always succeeds.


Builtin Iterator Types
^^^^^^^^^^^^^^^^^^^^^^

These are built-in iteration types that are included in Python's C API, but
provide no additional functions. They are here for completeness.


.. list-table::
   :widths: auto
   :header-rows: 1

   * * C type
     * Python type
   * * .. c:var:: PyTypeObject PyEnum_Type
     * :py:class:`enumerate`
   * * .. c:var:: PyTypeObject PyFilter_Type
     * :py:class:`filter`
   * * .. c:var:: PyTypeObject PyMap_Type
     * :py:class:`map`
   * * .. c:var:: PyTypeObject PyReversed_Type
     * :py:class:`reversed`
   * * .. c:var:: PyTypeObject PyZip_Type
     * :py:class:`zip`


Other Iterator Objects
^^^^^^^^^^^^^^^^^^^^^^

.. c:var:: PyTypeObject PyByteArrayIter_Type
.. c:var:: PyTypeObject PyBytesIter_Type
.. c:var:: PyTypeObject PyListIter_Type
.. c:var:: PyTypeObject PyListRevIter_Type
.. c:var:: PyTypeObject PySetIter_Type
.. c:var:: PyTypeObject PyTupleIter_Type
.. c:var:: PyTypeObject PyRangeIter_Type
.. c:var:: PyTypeObject PyLongRangeIter_Type
.. c:var:: PyTypeObject PyDictIterKey_Type
.. c:var:: PyTypeObject PyDictRevIterKey_Type
.. c:var:: PyTypeObject PyDictIterValue_Type
.. c:var:: PyTypeObject PyDictRevIterValue_Type
.. c:var:: PyTypeObject PyDictIterItem_Type
.. c:var:: PyTypeObject PyDictRevIterItem_Type
.. c:var:: PyTypeObject PyODictIter_Type

   Type objects for iterators of various built-in objects.

   Do not create instances of these directly; prefer calling
   :c:func:`PyObject_GetIter` instead.

   Note that there is no guarantee that a given built-in type uses a given iterator
   type. For example, iterating over :class:`range` will use one of two iterator
   types depending on the size of the range. Other types may start using a
   similar scheme in the future, without warning.
