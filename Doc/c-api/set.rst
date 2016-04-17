.. highlightlang:: c

.. _setobjects:

Set Objects
-----------

.. sectionauthor:: Raymond D. Hettinger <python@rcn.com>


.. index::
   object: set
   object: frozenset

.. versionadded:: 2.5

This section details the public API for :class:`set` and :class:`frozenset`
objects.  Any functionality not listed below is best accessed using the either
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
   considered public and are subject to change.  All access should be done through
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

   .. versionadded:: 2.6

.. c:function:: int PyFrozenSet_Check(PyObject *p)

   Return true if *p* is a :class:`frozenset` object or an instance of a
   subtype.

   .. versionadded:: 2.6

.. c:function:: int PyAnySet_Check(PyObject *p)

   Return true if *p* is a :class:`set` object, a :class:`frozenset` object, or an
   instance of a subtype.


.. c:function:: int PyAnySet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`set` object or a :class:`frozenset` object but
   not an instance of a subtype.


.. c:function:: int PyFrozenSet_CheckExact(PyObject *p)

   Return true if *p* is a :class:`frozenset` object but not an instance of a
   subtype.


.. c:function:: PyObject* PySet_New(PyObject *iterable)

   Return a new :class:`set` containing objects returned by the *iterable*.  The
   *iterable* may be *NULL* to create a new empty set.  Return the new set on
   success or *NULL* on failure.  Raise :exc:`TypeError` if *iterable* is not
   actually iterable.  The constructor is also useful for copying a set
   (``c=set(s)``).


.. c:function:: PyObject* PyFrozenSet_New(PyObject *iterable)

   Return a new :class:`frozenset` containing objects returned by the *iterable*.
   The *iterable* may be *NULL* to create a new empty frozenset.  Return the new
   set on success or *NULL* on failure.  Raise :exc:`TypeError` if *iterable* is
   not actually iterable.

   .. versionchanged:: 2.6
      Now guaranteed to return a brand-new :class:`frozenset`.  Formerly,
      frozensets of zero-length were a singleton.  This got in the way of
      building-up new frozensets with :meth:`PySet_Add`.

The following functions and macros are available for instances of :class:`set`
or :class:`frozenset` or instances of their subtypes.


.. c:function:: Py_ssize_t PySet_Size(PyObject *anyset)

   .. index:: builtin: len

   Return the length of a :class:`set` or :class:`frozenset` object. Equivalent to
   ``len(anyset)``.  Raises a :exc:`PyExc_SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.

   .. versionchanged:: 2.5
      This function returned an :c:type:`int`. This might require changes in
      your code for properly supporting 64-bit systems.


.. c:function:: Py_ssize_t PySet_GET_SIZE(PyObject *anyset)

   Macro form of :c:func:`PySet_Size` without error checking.


.. c:function:: int PySet_Contains(PyObject *anyset, PyObject *key)

   Return 1 if found, 0 if not found, and -1 if an error is encountered.  Unlike
   the Python :meth:`__contains__` method, this function does not automatically
   convert unhashable sets into temporary frozensets.  Raise a :exc:`TypeError` if
   the *key* is unhashable. Raise :exc:`PyExc_SystemError` if *anyset* is not a
   :class:`set`, :class:`frozenset`, or an instance of a subtype.


.. c:function:: int PySet_Add(PyObject *set, PyObject *key)

   Add *key* to a :class:`set` instance.  Does not apply to :class:`frozenset`
   instances.  Return 0 on success or -1 on failure. Raise a :exc:`TypeError` if
   the *key* is unhashable. Raise a :exc:`MemoryError` if there is no room to grow.
   Raise a :exc:`SystemError` if *set* is not an instance of :class:`set` or its
   subtype.

   .. versionchanged:: 2.6
      Now works with instances of :class:`frozenset` or its subtypes.
      Like :c:func:`PyTuple_SetItem` in that it can be used to fill-in the
      values of brand new frozensets before they are exposed to other code.

The following functions are available for instances of :class:`set` or its
subtypes but not for instances of :class:`frozenset` or its subtypes.


.. c:function:: int PySet_Discard(PyObject *set, PyObject *key)

   Return 1 if found and removed, 0 if not found (no action taken), and -1 if an
   error is encountered.  Does not raise :exc:`KeyError` for missing keys.  Raise a
   :exc:`TypeError` if the *key* is unhashable.  Unlike the Python :meth:`~set.discard`
   method, this function does not automatically convert unhashable sets into
   temporary frozensets. Raise :exc:`PyExc_SystemError` if *set* is not an
   instance of :class:`set` or its subtype.


.. c:function:: PyObject* PySet_Pop(PyObject *set)

   Return a new reference to an arbitrary object in the *set*, and removes the
   object from the *set*.  Return *NULL* on failure.  Raise :exc:`KeyError` if the
   set is empty. Raise a :exc:`SystemError` if *set* is not an instance of
   :class:`set` or its subtype.


.. c:function:: int PySet_Clear(PyObject *set)

   Empty an existing set of all elements.
