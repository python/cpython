.. highlightlang:: c

.. _longobjects:

Integer Objects
---------------

.. index:: object: long integer
           object: integer

All integers are implemented as "long" integer objects of arbitrary size.

.. ctype:: PyLongObject

   This subtype of :ctype:`PyObject` represents a Python integer object.


.. cvar:: PyTypeObject PyLong_Type

   This instance of :ctype:`PyTypeObject` represents the Python integer type.
   This is the same object as ``int``.


.. cfunction:: int PyLong_Check(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject` or a subtype of
   :ctype:`PyLongObject`.


.. cfunction:: int PyLong_CheckExact(PyObject *p)

   Return true if its argument is a :ctype:`PyLongObject`, but not a subtype of
   :ctype:`PyLongObject`.


.. cfunction:: PyObject* PyLong_FromLong(long v)

   Return a new :ctype:`PyLongObject` object from *v*, or *NULL* on failure.

   The current implementation keeps an array of integer objects for all integers
   between ``-5`` and ``256``, when you create an int in that range you actually
   just get back a reference to the existing object. So it should be possible to
   change the value of ``1``.  I suspect the behaviour of Python in this case is
   undefined. :-)


.. cfunction:: PyObject* PyLong_FromUnsignedLong(unsigned long v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`unsigned long`, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromSsize_t(Py_ssize_t v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`Py_ssize_t`, or
   *NULL* on failure.


.. cfunction:: PyObject* PyLong_FromSize_t(size_t v)

   Return a new :ctype:`PyLongObject` object from a C :ctype:`size_t`, or
   *NULL* on failure.


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

   Return a new :ctype:`PyLongObject` based on the string value in *str*, which
   is interpreted according to the radix in *base*.  If *pend* is non-*NULL*,
   ``*pend`` will point to the first character in *str* which follows the
   representation of the number.  If *base* is ``0``, the radix will be
   determined based on the leading characters of *str*: if *str* starts with
   ``'0x'`` or ``'0X'``, radix 16 will be used; if *str* starts with ``'0o'`` or
   ``'0O'``, radix 8 will be used; if *str* starts with ``'0b'`` or ``'0B'``,
   radix 2 will be used; otherwise radix 10 will be used.  If *base* is not
   ``0``, it must be between ``2`` and ``36``, inclusive.  Leading spaces are
   ignored.  If there are no digits, :exc:`ValueError` will be raised.


.. cfunction:: PyObject* PyLong_FromUnicode(Py_UNICODE *u, Py_ssize_t length, int base)

   Convert a sequence of Unicode digits to a Python integer value.  The Unicode
   string is first encoded to a byte string using :cfunc:`PyUnicode_EncodeDecimal`
   and then converted using :cfunc:`PyLong_FromString`.


.. cfunction:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer from the pointer *p*. The pointer value can be
   retrieved from the resulting value using :cfunc:`PyLong_AsVoidPtr`.


.. XXX alias PyLong_AS_LONG (for now) 
.. cfunction:: long PyLong_AsLong(PyObject *pylong)

   .. index::
      single: LONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`long` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`LONG_MAX`, raise an :exc:`OverflowError`,
   and return -1. Convert non-long objects automatically to long first,
   and return -1 if that raises exceptions.

.. cfunction:: long PyLong_AsLongAndOverflow(PyObject *pylong, int* overflow)

   Return a C :ctype:`long` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`LONG_MAX`, return -1 and
   set `*overflow` to 1 (for overflow) or -1 (for underflow). 
   If an exception is set because of type errors, also return -1.


.. cfunction:: Py_ssize_t PyLong_AsSsize_t(PyObject *pylong)

   .. index::
      single: PY_SSIZE_T_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`Py_ssize_t` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`PY_SSIZE_T_MAX`, an :exc:`OverflowError` is raised
   and ``-1`` will be returned.


.. cfunction:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX
      single: OverflowError (built-in exception)

   Return a C :ctype:`unsigned long` representation of the contents of *pylong*.
   If *pylong* is greater than :const:`ULONG_MAX`, an :exc:`OverflowError` is
   raised.


.. cfunction:: Py_ssize_t PyLong_AsSsize_t(PyObject *pylong)

   .. index::
      single: PY_SSIZE_T_MAX

   Return a :ctype:`Py_ssize_t` representation of the contents of *pylong*.  If
   *pylong* is greater than :const:`PY_SSIZE_T_MAX`, an :exc:`OverflowError` is
   raised.


.. cfunction:: size_t PyLong_AsSize_t(PyObject *pylong)

   Return a :ctype:`size_t` representation of the contents of *pylong*.  If
   *pylong* is greater than the maximum value for a :ctype:`size_t`, an
   :exc:`OverflowError` is raised.


.. cfunction:: PY_LONG_LONG PyLong_AsLongLong(PyObject *pylong)

   Return a C :ctype:`long long` from a Python integer.  If *pylong* cannot be
   represented as a :ctype:`long long`, an :exc:`OverflowError` will be raised.


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLong(PyObject *pylong)

   Return a C :ctype:`unsigned long long` from a Python integer. If *pylong*
   cannot be represented as an :ctype:`unsigned long long`, an :exc:`OverflowError`
   will be raised if the value is positive, or a :exc:`TypeError` will be raised if
   the value is negative.


.. cfunction:: unsigned long PyLong_AsUnsignedLongMask(PyObject *io)

   Return a C :ctype:`unsigned long` from a Python integer, without checking for
   overflow.


.. cfunction:: unsigned PY_LONG_LONG PyLong_AsUnsignedLongLongMask(PyObject *io)

   Return a C :ctype:`unsigned long long` from a Python integer, without
   checking for overflow.


.. cfunction:: double PyLong_AsDouble(PyObject *pylong)

   Return a C :ctype:`double` representation of the contents of *pylong*.  If
   *pylong* cannot be approximately represented as a :ctype:`double`, an
   :exc:`OverflowError` exception is raised and ``-1.0`` will be returned.


.. cfunction:: void* PyLong_AsVoidPtr(PyObject *pylong)

   Convert a Python integer *pylong* to a C :ctype:`void` pointer.
   If *pylong* cannot be converted, an :exc:`OverflowError` will be raised.  This
   is only assured to produce a usable :ctype:`void` pointer for values created
   with :cfunc:`PyLong_FromVoidPtr`.
