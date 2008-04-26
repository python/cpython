.. highlightlang:: c

.. _longobjects:

Long Integer Objects
--------------------

.. index:: object: long integer


.. ctype:: PyLongObject

   This subtype of :ctype:`PyObject` represents a Python long integer object.


.. cvar:: PyTypeObject PyLong_Type

   .. index:: single: LongType (in modules types)

   This instance of :ctype:`PyTypeObject` represents the Python long integer type.
   This is the same object as ``long`` and ``types.LongType``.


.. cfunction:: int PyLong_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject` or a subtype of
   :ctype:`PyLongObject`.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyLong_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject`, but not a subtype of
   :ctype:`PyLongObject`.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyLong_FromLong(long v)

   Return a new :ctype:`PyLongObject` object from *v*, or *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromUnsignedLong(unsigned long v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`unsigned long`, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromSsize_t(Py_ssize_t v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`Py_ssize_t`, or
   *NULL* on failure.

   .. versionadded:: 2.5


.. cfunction:: PyObject* PyLong_FromSize_t(size_t v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`size_t`, or
   *NULL* on failure.

   .. versionadded:: 2.5


.. cfunction:: PyObject* PyLong_FromLongLong(PY_LONG_LONG v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`long long`, or *NULL*
   on failure.


.. cfunction:: PyObject* PyLong_FromUnsignedLongLong(unsigned PY_LONG_LONG v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`unsigned long long`,
   or *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromDouble(double v)

   Return a new :ctype:`PyLongObject` object from the integer part of *v*, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromString(char *str, char **pend, int base)

   Return a new :ctype:`PyLongObject` based on the string value in *str*, which is
   interpreted according to the radix in *base*.  If *pend* is non-*NULL*,
   ``*pend`` will point to the first character in *str* which follows the
   representation of the number.  If *base* is ``0``, the radix will be determined
   based on the leading characters of *str*: if *str* starts with ``'0x'`` or
   ``'0X'``, radix 16 will be used; if *str* starts with ``'0'``, radix 8 will be
   used; otherwise radix 10 will be used.  If *base* is not ``0``, it must be
   between ``2`` and ``36``, inclusive.  Leading spaces are ignored.  If there are
   no digits, :exc:`ValueError` will be raised.


.. cfunction:: PyObject* PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)

   Convert a sequence of Unicode digits to a Python long integer value.  The first
   parameter, *u*, points to the first character of the Unicode string, *length*
   gives the number of characters, and *base* is the radix for the conversion.  The
   radix must be in the range [2, 36]; if it is out of range, :exc:`ValueError`
   will be raised.

   .. versionadded:: 1.6


.. cfunction:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer or long integer from the pointer *p*. The pointer value
   can be retrieved from the resulting value using :cfunc:`PyLong_AsVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      If the integer is larger than LONG_MAX, a positive long integer is returned.


.. cfunction:: long PyLong_AsLong(PyObject *pylong)

   .. index::
      single: LONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`long` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`LONG_MAX`, an :exc:`OverflowError` is raised
   and ``-1`` will be returned. 


.. cfunction:: Py_ssize_t PyLong_AsSsize_t(PyObject *pylong)

   .. index::
      single: PY_SSIZE_T_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`Py_ssize_t` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`PY_SSIZE_T_MAX`, an :exc:`OverflowError` is raised
   and ``-1`` will be returned.

   .. versionadded:: 2.5


.. cfunction:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`unsigned long` representation of the contents of *pylong*.
   If *pylong* is greater than :const:`ULONG_MAX`, an :exc:`OverflowError` is
   raised.


.. cfunction:: PY_LONG_LONG PyLong_AsLongLong(PyObject *pylong)

   Return a C :ctype:`long long` from a Python long integer.  If *pylong* cannot be
   represented as a :ctype:`long long`, an :exc:`OverflowError` will be raised.

   .. versionadded:: 2.2


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLong(PyObject *pylong)

   Return a C :ctype:`unsigned long long` from a Python long integer. If *pylong*
   cannot be represented as an :ctype:`unsigned long long`, an :exc:`OverflowError`
   will be raised if the value is positive, or a :exc:`TypeError` will be raised if
   the value is negative.

   .. versionadded:: 2.2


.. cfunction:: unsigned long PyLong_AsUnsignedLongMask(PyObject *io)

   Return a C :ctype:`unsigned long` from a Python long integer, without checking
   for overflow.

   .. versionadded:: 2.3


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLongMask(PyObject *io)

   Return a C :ctype:`unsigned long long` from a Python long integer, without
   checking for overflow.

   .. versionadded:: 2.3


.. cfunction:: double PyLong_AsDouble(PyObject *pylong)

   Return a C :ctype:`double` representation of the contents of *pylong*.  If
   *pylong* cannot be approximately represented as a :ctype:`double`, an
   :exc:`OverflowError` exception is raised and ``-1.0`` will be returned.


.. cfunction:: void* PyLong_AsVoidPtr(PyObject *pylong)

   Convert a Python integer or long integer *pylong* to a C :ctype:`void` pointer.
   If *pylong* cannot be converted, an :exc:`OverflowError` will be raised.  This
   is only assured to produce a usable :ctype:`void` pointer for values created
   with :cfunc:`PyLong_FromVoidPtr`.

   .. versionadded:: 1.5.2

   .. versionchanged:: 2.5
      For values outside 0..LONG_MAX, both signed and unsigned integers are accepted.


