.. highlightlang:: c

.. _longobjects:

Long Integer Objects
--------------------

.. index:: object: long integer


.. c:type:: PyLongObject

   This subtype of :c:type:`PyObject` represents a Python long integer object.


.. c:var:: PyTypeObject PyLong_Type

   .. index:: single: LongType (in modules types)

   This instance of :c:type:`PyTypeObject` represents the Python long integer type.
   This is the same object as ``long`` and ``types.LongType``.


.. c:function:: int PyLong_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyLongObject` or a subtype of
   :c:type:`PyLongObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. c:function:: int PyLong_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyLongObject`, but not a subtype of
   :c:type:`PyLongObject`.

   .. versionadded:: 2.2


.. c:function:: PyObject* PyLong_FromLong(long v)

   Return a new :c:type:`PyLongObject` object from *v*, or *NULL* on failure.


.. c:function:: PyObject* PyLong_FromUnsignedLong(unsigned long v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`unsigned long`, or
   *NULL* on failure.


.. c:function:: PyObject* PyLong_FromSsize_t(Py_ssize_t v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`Py_ssize_t`, or
   *NULL* on failure.

   .. versionadded:: 2.6


.. c:function:: PyObject* PyLong_FromSize_t(size_t v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`size_t`, or
   *NULL* on failure.

   .. versionadded:: 2.6


.. c:function:: PyObject* PyLong_FromLongLong(PY_LONG_LONG v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`long long`, or *NULL*
   on failure.


.. c:function:: PyObject* PyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`unsigned long long`,
   or *NULL* on failure.


.. c:function:: PyObject* PyLong_FromDouble(double v)

   Return a new :c:type:`PyLongObject` object from the integer part of *v*, or
   *NULL* on failure.


.. c:function:: PyObject* PyLong_FromString(char *str, char **pend, int base)

   Return a new :c:type:`PyLongObject` based on the string value in *str*, which is
   interpreted according to the radix in *base*.  If *pend* is non-*NULL*,
   *\*pend* will point to the first character in *str* which follows the
   representation of the number.  If *base* is ``0``, the radix will be determined
   based on the leading characters of *str*: if *str* starts with ``'0x'`` or
   ``'0X'``, radix 16 will be used; if *str* starts with ``'0'``, radix 8 will be
   used; otherwise radix 10 will be used.  If *base* is not ``0``, it must be
   between ``2`` and ``36``, inclusive.  Leading spaces are ignored.  If there are
   no digits, :exc:`ValueError` will be raised.


.. c:function:: PyObject* PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)

   Convert a sequence of Unicode digits to a Python long integer value.  The first
   parameter, *u*, points to the first character of the Unicode string, *length*
   gives the number of characters, and *base* is the radix for the conversion.  The
   radix must be in the range [2, 36]; if it is out of range, :exc:`ValueError`
   will be raised.

   .. versionadded:: 1.6

   .. versionchanged:: 2.5
      This function used an :c:type:`int` for *length*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer or long integer from the pointer *p*. The pointer value
   can be retrieved from the resulting value using :c:func:`PyLong_AsVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      If the integer is larger than LONG_MAX, a positive long integer is returned.


.. c:function:: long PyLong_AsLong(PyObject *pylong)

   .. index::
      single: LONG_MAX
      single: OverflowError (built-in exception)

   Return a C :c:type:`long` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`LONG_MAX`, an :exc:`OverflowError` is raised
   and ``-1`` will be returned.


.. c:function:: long PyLong_AsLongAndOverflow(PyObject *pylong, int *overflow)

   Return a C :c:type:`long` representation of the contents of
   *pylong*.  If *pylong* is greater than :const:`LONG_MAX` or less
   than :const:`LONG_MIN`, set *\*overflow* to ``1`` or ``-1``,
   respectively, and return ``-1``; otherwise, set *\*overflow* to
   ``0``.  If any other exception occurs (for example a TypeError or
   MemoryError), then ``-1`` will be returned and *\*overflow* will
   be ``0``.

   .. versionadded:: 2.7


.. c:function:: PY_LONG_LONG PyLong_AsLongLongAndOverflow(PyObject *pylong, int *overflow)

   Return a C :c:type:`long long` representation of the contents of
   *pylong*.  If *pylong* is greater than :const:`PY_LLONG_MAX` or less
   than :const:`PY_LLONG_MIN`, set *\*overflow* to ``1`` or ``-1``,
   respectively, and return ``-1``; otherwise, set *\*overflow* to
   ``0``.  If any other exception occurs (for example a TypeError or
   MemoryError), then ``-1`` will be returned and *\*overflow* will
   be ``0``.

   .. versionadded:: 2.7


.. c:function:: Py_ssize_t PyLong_AsSsize_t(PyObject *pylong)

   .. index::
      single: PY_SSIZE_T_MAX
      single: OverflowError (built-in exception)

   Return a C :c:type:`Py_ssize_t` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`PY_SSIZE_T_MAX`, an :exc:`OverflowError` is raised
   and ``-1`` will be returned.

   .. versionadded:: 2.6


.. c:function:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX
      single: OverflowError (built-in exception)

   Return a C :c:type:`unsigned long` representation of the contents of *pylong*.
   If *pylong* is greater than :const:`ULONG_MAX`, an :exc:`OverflowError` is
   raised.


.. c:function:: PY_LONG_LONG PyLong_AsLongLong(PyObject *pylong)

   .. index::
      single: OverflowError (built-in exception)

   Return a C :c:type:`long long` from a Python long integer.  If
   *pylong* cannot be represented as a :c:type:`long long`, an
   :exc:`OverflowError` is raised and ``-1`` is returned.

   .. versionadded:: 2.2


.. c:function:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLong(PyObject *pylong)

   .. index::
      single: OverflowError (built-in exception)

   Return a C :c:type:`unsigned long long` from a Python long integer. If
   *pylong* cannot be represented as an :c:type:`unsigned long long`, an
   :exc:`OverflowError` is raised and ``(unsigned long long)-1`` is
   returned.

   .. versionadded:: 2.2

   .. versionchanged:: 2.7
      A negative *pylong* now raises :exc:`OverflowError`, not
      :exc:`TypeError`.


.. c:function:: unsigned long PyLong_AsUnsignedLongMask(PyObject *io)

   Return a C :c:type:`unsigned long` from a Python long integer, without checking
   for overflow.

   .. versionadded:: 2.3


.. c:function:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLongMask(PyObject *io)

   Return a C :c:type:`unsigned long long` from a Python long integer, without
   checking for overflow.

   .. versionadded:: 2.3


.. c:function:: double PyLong_AsDouble(PyObject *pylong)

   Return a C :c:type:`double` representation of the contents of *pylong*.  If
   *pylong* cannot be approximately represented as a :c:type:`double`, an
   :exc:`OverflowError` exception is raised and ``-1.0`` will be returned.


.. c:function:: void* PyLong_AsVoidPtr(PyObject *pylong)

   Convert a Python integer or long integer *pylong* to a C :c:type:`void` pointer.
   If *pylong* cannot be converted, an :exc:`OverflowError` will be raised.  This
   is only assured to produce a usable :c:type:`void` pointer for values created
   with :c:func:`PyLong_FromVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      For values outside 0..LONG_MAX, both signed and unsigned integers are accepted.


