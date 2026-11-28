.. highlight:: c

.. _setobjects:

Set Objects
-----------

.. index::
   pair: object; set
   pair: object; frozenset

This section details the public API for :class:`set` and :class:`frozenset`
objects.  Any functionality not listed below is best accessed using either
the abstract object protocol (including :c:func:`PyObject_CallMethod`,
:c:func:`PyObject_RichCompareBool`, :c:func:`PyObject_Hash`,
:c:func:`PyObject_Repr`, :c:func:`PyObject_IsTrue`, :c:func:`PyObject_Print`, and
:c:func:`PyObject_GetIter`) or the abstract number protocol (including
:c:func:`PyNumber_And`, :c:func:`PyNumber_Subtract`, :c:func:`PyNumber_Or`,
:c:func:`PyNumber_Xor`, :c:func:`PyNumber_InPlaceAnd`,
:c:func:`PyNumber_InPlaceSubtract`, :c:func:`PyNumber_InPlaceOr`, and
:c:func:`PyNumber_InPlaceXor`).


.. c:type:: PySetObject

   This subtype of :c:type:`PyObject` is used to hold the internal data for both
   :class:`set` and :class:`frozenset` objects.  It is like a :c:type:`PyDictObject`
   in that it is a fixed size for small sets (much like tuple storage) and will
   point to a separate, variable sized block of memory for medium and large sized
   sets (much like list storage). None of the fields of this structure should be
   considered public and all are subject to change.  All access should be done through
   the documented API rather than by manipulating the values in the structure.


.. c:var:: PyTypeObject PySet_Type

   This is an instance of :c:type:`PyTypeObject` representing the Python
   :class:`set` type.


.. c:var:: PyTypeObject PyFrozenSet_Type

   This is an instance of :c:type:`PyTypeObject` representing the Python
   :class:`frozenset` type.

The following type check macros work on pointers to any Python object. Likewise,
the constructor functions work with any iterable Python object.


.. c:function:: int PySet_Check(PyObject *p)

   Return true if *p* is a :class:`set` object or an instance of a subtype.
   This function always succeeds.

.. c:function:: int PyFrozenSet_Check(PyObject *p)

   Return true if *p* is a :class:`frozenset` object or an instance of a
   subtype.  This function always succeeds.

.. c:function:: int PyAnySet_Check(PyObject *p)

   Return true if *p* is a :class:`set` object, a :class:`frozenset` object, or an
   instance of a subtype.  This function always succeeds.

.. c:function:: int PySet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`set` object but not an instance of a
   subtype.  This function always succeeds.

   .. versionadded:: 3.10

.. c:function:: int PyAnySet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`set` object or a :class:`frozenset` object but
   not an instance of a subtype.  This function always succeeds.


.. c:function:: int PyFrozenSet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`frozenset` object but not an instance of a
   subtype.  This function always succeeds.


.. c:function:: PyObject* PySet_New(PyObject *iterable)

   Return a new :class:`set` containing objects returned by the *iterable*.  The
   *iterable* may be ``NULL`` to create a new empty set.  Return the new set on
   success or ``NULL`` on failure.  Raise :exc:`TypeError` if *iterable* is not
   actually iterable.  The constructor is also useful for copying a set
   (``c=set(s)``).

   See also the :ref:`PySetBuilder API <pysetbuilder>`.

   .. note::

      The operation is atomic on :term:`free threading <free-threaded build>`
      when *iterable* is a :class:`set`, :class:`frozenset`, :class:`dict` or :class:`frozendict`.


.. c:function:: PyObject* PyFrozenSet_New(PyObject *iterable)

   Return a new :class:`frozenset` containing objects returned by the *iterable*.
   The *iterable* may be ``NULL`` to create a new empty frozenset.  Return the new
   set on success or ``NULL`` on failure.  Raise :exc:`TypeError` if *iterable* is
   not actually iterable.

   See also the :ref:`PySetBuilder API <pysetbuilder>`.

   .. note::

      The operation is atomic on :term:`free threading <free-threaded build>`
      when *iterable* is a :class:`set`, :class:`frozenset`, :class:`dict` or :class:`frozendict`.


The following functions and macros are available for instances of :class:`set`
or :class:`frozenset` or instances of their subtypes.


.. c:function:: Py_ssize_t PySet_Size(PyObject *anyset)

   .. index:: pair: built-in function; len

   Return the length of a :class:`set` or :class:`frozenset` object. Equivalent to
   ``len(anyset)``.  Raises a :exc:`SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.


.. c:function:: Py_ssize_t PySet_GET_SIZE(PyObject *anyset)

   Macro form of :c:func:`PySet_Size` without error checking.


.. c:function:: int PySet_Contains(PyObject *anyset, PyObject *key)

   Return ``1`` if found, ``0`` if not found, and ``-1`` if an error is encountered.  Unlike
   the Python :meth:`~object.__contains__` method, this function does not automatically
   convert unhashable sets into temporary frozensets.  Raise a :exc:`TypeError` if
   the *key* is unhashable. Raise :exc:`SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.

   .. note::

      The operation is atomic on :term:`free threading <free-threaded build>`
      when *key* is :class:`str`, :class:`int`, :class:`float`, :class:`bool` or :class:`bytes`.

.. c:function:: int PySet_Add(PyObject *set, PyObject *key)

   Add *key* to a :class:`set` instance.  Also works with :class:`frozenset`
   instances (like :c:func:`PyTuple_SetItem` it can be used to fill in the values
   of brand new frozensets before they are exposed to other code).  Return ``0`` on
   success or ``-1`` on failure. Raise a :exc:`TypeError` if the *key* is
   unhashable. Raise a :exc:`MemoryError` if there is no room to grow.  Raise a
   :exc:`SystemError` if *set* is not an instance of :class:`set` or its
   subtype.

   .. note::

      The operation is atomic on :term:`free threading <free-threaded build>`
      when *key* is :class:`str`, :class:`int`, :class:`float`, :class:`bool` or :class:`bytes`.



The following functions are available for instances of :class:`set` or its
subtypes but not for instances of :class:`frozenset` or its subtypes.


.. c:function:: int PySet_Discard(PyObject *set, PyObject *key)

   Return ``1`` if found and removed, ``0`` if not found (no action taken), and ``-1`` if an
   error is encountered.  Does not raise :exc:`KeyError` for missing keys.  Raise a
   :exc:`TypeError` if the *key* is unhashable.  Unlike the Python :meth:`~set.discard`
   method, this function does not automatically convert unhashable sets into
   temporary frozensets. Raise :exc:`SystemError` if *set* is not an
   instance of :class:`set` or its subtype.

   .. note::

      The operation is atomic on :term:`free threading <free-threaded build>`
      when *key* is :class:`str`, :class:`int`, :class:`float`, :class:`bool` or :class:`bytes`.


.. c:function:: PyObject* PySet_Pop(PyObject *set)

   Return a new reference to an arbitrary object in the *set*, and removes the
   object from the *set*.  Return ``NULL`` on failure.  Raise :exc:`KeyError` if the
   set is empty. Raise a :exc:`SystemError` if *set* is not an instance of
   :class:`set` or its subtype.


.. c:function:: int PySet_Clear(PyObject *set)

   Empty an existing set of all elements. Return ``0`` on
   success. Return ``-1`` and raise :exc:`SystemError` if *set* is not an instance of
   :class:`set` or its subtype.

   .. note::

      In the :term:`free-threaded build`, the set is emptied before its entries
      are cleared, so other threads will observe an empty set rather than
      intermediate states.


Deprecated API
^^^^^^^^^^^^^^

.. c:macro:: PySet_MINSIZE

   A constant representing the size of an internal
   preallocated table inside :c:type:`PySetObject` instances.

   This is documented solely for completeness, as there are no guarantees
   that a given version of CPython uses preallocated tables with a fixed
   size.
   In code that does not deal with unstable set internals,
   :c:macro:`!PySet_MINSIZE` can be replaced with a small constant like ``8``.

   If looking for the size of a set, use :c:func:`PySet_Size` instead.

   .. soft-deprecated:: 3.14


.. _pysetbuilder:

PySetBuilder
------------

The :c:type:`PySetBuilder` API can be used to create a :class:`set` object
or a :class:`frozenset` object.

See also the :c:func:`PySet_New` and :c:func:`PyFrozenSet_New` functions.

.. versionadded:: next


.. c:type:: PySetBuilder

   A :class:`set` or :class:`frozenset` builder object.

   The API is **not thread safe**. To share a builder with multiple threads, a
   critical section or a lock is needed.

   The instance must be destroyed by :c:func:`PySetBuilder_Finish` on
   success, or :c:func:`PySetBuilder_Discard` on error.


.. c:function:: PySetBuilder* PySetBuilder_CreateSet(Py_ssize_t size)

   Create a :class:`set` using a :c:type:`PySetBuilder`.

   If *size* is greater than zero, allocate an internal table big enough to
   store *size* items.

   On error, set an exception and return ``NULL``.

   *size* must be positive or zero.

.. c:function:: PySetBuilder* PySetBuilder_CreateFrozenSet(Py_ssize_t size)

   Similar to :c:func:`PySetBuilder_CreateSet`, but create a :class:`frozenset`
   object instead of a :class:`set` object.

.. c:function:: PyObject* PySetBuilder_Finish(PySetBuilder *builder)

   Finish a :c:type:`PySetBuilder` created by
   :c:func:`PySetBuilder_CreateFrozenSet`.

   On success, return a :class:`set` object (if
   :c:func:`PySetBuilder_CreateSet` was used) or a :class:`frozenset` object
   (if :c:func:`PySetBuilder_CreateFrozenSet` was used).
   On error, set an exception and return ``NULL``.

   The builder instance is invalid after the call in any case.
   No API can be called on the builder after :c:func:`PySetBuilder_Finish`.

.. c:function:: void PySetBuilder_Discard(PySetBuilder *builder)

   Discard a :c:type:`PySetBuilder` created by :c:func:`PySetBuilder_CreateFrozenSet`.

   Do nothing if *builder* is ``NULL``.

   The builder instance is invalid after the call.
   No API can be called on the builder after :c:func:`PySetBuilder_Discard`.

.. c:function:: int PySetBuilder_Add(PySetBuilder *builder, PyObject *item)

   Add *item* to a set builder.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

.. c:function:: int PySetBuilder_AddSteal(PySetBuilder *builder, PyObject *item)

   Similar to :c:func:`PySetBuilder_Add`, but :term:`steal` the *item*
   reference.

.. c:function:: int PySetBuilder_Update(PySetBuilder *builder, PyObject *iterable)

   Update the set builder, adding elements from *iterable*.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

   On error, elements already added are kept.

.. c:function:: int PySetBuilder_UpdateArray(PySetBuilder *builder, PyObject *const *items, Py_ssize_t nitem)

   Update the set builder, adding *nitem* elements from *items* array:
   copy references from *items* to the builder.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

   On error, elements already added are kept.

   *nitem* must be positive or zero.

.. c:function:: int PySetBuilder_UpdateArraySteal(PySetBuilder *builder, PyObject *const *items, Py_ssize_t nitem)

   Similar to :c:func:`PySetBuilder_UpdateArray`, but :term:`steal`
   references instead of copying references.
