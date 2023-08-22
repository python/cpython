.. highlight:: c

.. _longobjects:

Integer Objects
---------------

.. index:: pair: object; long integer
           pair: object; integer

All integers are implemented as "long" integer objects of arbitrary size.

On error, most ``PyLong_As*`` APIs return ``(return type)-1`` which cannot be
distinguished from a number.  Use :c:func:`PyErr_Occurred` to disambiguate.

.. c:type:: PyLongObject

   This subtype of :c:type:`PyObject` represents a Python integer object.


.. c:var:: PyTypeObject PyLong_Type

   This instance of :c:type:`PyTypeObject` represents the Python integer type.
   This is the same object as :class:`int` in the Python layer.


.. c:function:: int PyLong_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyLongObject` or a subtype of
   :c:type:`PyLongObject`.  This function always succeeds.


.. c:function:: int PyLong_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyLongObject`, but not a subtype of
   :c:type:`PyLongObject`.  This function always succeeds.


.. c:function:: PyObject* PyLong_FromLong(long v)

   Return a new :c:type:`PyLongObject` object from *v*, or ``NULL`` on failure.

   The current implementation keeps an array of integer objects for all integers
   between ``-5`` and ``256``. When you create an int in that range you actually
   just get back a reference to the existing object.


.. c:function:: PyObject* PyLong_FromUnsignedLong(unsigned long v)

   Return a new :c:type:`PyLongObject` object from a C :c:expr:`unsigned long`, or
   ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromSsize_t(Py_ssize_t v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`Py_ssize_t`, or
   ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromSize_t(size_t v)

   Return a new :c:type:`PyLongObject` object from a C :c:type:`size_t`, or
   ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromLongLong(long long v)

   Return a new :c:type:`PyLongObject` object from a C :c:expr:`long long`, or ``NULL``
   on failure.


.. c:function:: PyObject* PyLong_FromUnsignedLongLong(unsigned long long v)

   Return a new :c:type:`PyLongObject` object from a C :c:expr:`unsigned long long`,
   or ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromDouble(double v)

   Return a new :c:type:`PyLongObject` object from the integer part of *v*, or
   ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromString(const char *str, char **pend, int base)

   Return a new :c:type:`PyLongObject` based on the string value in *str*, which
   is interpreted according to the radix in *base*.  If *pend* is non-``NULL``,
   *\*pend* will point to the first character in *str* which follows the
   representation of the number.  If *base* is ``0``, *str* is interpreted using
   the :ref:`integers` definition; in this case, leading zeros in a
   non-zero decimal number raises a :exc:`ValueError`. If *base* is not ``0``,
   it must be between ``2`` and ``36``, inclusive.  Leading spaces and single
   underscores after a base specifier and between digits are ignored.  If there
   are no digits, :exc:`ValueError` will be raised.

   .. seealso:: Python methods :meth:`int.to_bytes` and :meth:`int.from_bytes`
      to convert a :c:type:`PyLongObject` to/from an array of bytes in base
      ``256``. You can call those from C using :c:func:`PyObject_CallMethod`.


.. c:function:: PyObject* PyLong_FromUnicodeObject(PyObject *u, int base)

   Convert a sequence of Unicode digits in the string *u* to a Python integer
   value.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer from the pointer *p*. The pointer value can be
   retrieved from the resulting value using :c:func:`PyLong_AsVoidPtr`.


.. XXX alias PyLong_AS_LONG (for now)
.. c:function:: long PyLong_AsLong(PyObject *obj)

   .. index::
      single: LONG_MAX
      single: OverflowError (built-in exception)

   Return a C :c:expr:`long` representation of *obj*.  If *obj* is not an
   instance of :c:type:`PyLongObject`, first call its :meth:`~object.__index__` method
   (if present) to convert it to a :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *obj* is out of range for a
   :c:expr:`long`.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: long PyLong_AsLongAndOverflow(PyObject *obj, int *overflow)

   Return a C :c:expr:`long` representation of *obj*.  If *obj* is not an
   instance of :c:type:`PyLongObject`, first call its :meth:`~object.__index__`
   method (if present) to convert it to a :c:type:`PyLongObject`.

   If the value of *obj* is greater than :c:macro:`LONG_MAX` or less than
   :c:macro:`LONG_MIN`, set *\*overflow* to ``1`` or ``-1``, respectively, and
   return ``-1``; otherwise, set *\*overflow* to ``0``.  If any other exception
   occurs set *\*overflow* to ``0`` and return ``-1`` as usual.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: long long PyLong_AsLongLong(PyObject *obj)

   .. index::
      single: OverflowError (built-in exception)

   Return a C :c:expr:`long long` representation of *obj*.  If *obj* is not an
   instance of :c:type:`PyLongObject`, first call its :meth:`~object.__index__` method
   (if present) to convert it to a :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *obj* is out of range for a
   :c:expr:`long long`.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: long long PyLong_AsLongLongAndOverflow(PyObject *obj, int *overflow)

   Return a C :c:expr:`long long` representation of *obj*.  If *obj* is not an
   instance of :c:type:`PyLongObject`, first call its :meth:`~object.__index__` method
   (if present) to convert it to a :c:type:`PyLongObject`.

   If the value of *obj* is greater than :c:macro:`LLONG_MAX` or less than
   :c:macro:`LLONG_MIN`, set *\*overflow* to ``1`` or ``-1``, respectively,
   and return ``-1``; otherwise, set *\*overflow* to ``0``.  If any other
   exception occurs set *\*overflow* to ``0`` and return ``-1`` as usual.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.

   .. versionadded:: 3.2

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: Py_ssize_t PyLong_AsSsize_t(PyObject *pylong)

   .. index::
      single: PY_SSIZE_T_MAX
      single: OverflowError (built-in exception)

   Return a C :c:type:`Py_ssize_t` representation of *pylong*.  *pylong* must
   be an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:type:`Py_ssize_t`.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX
      single: OverflowError (built-in exception)

   Return a C :c:expr:`unsigned long` representation of *pylong*.  *pylong*
   must be an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:expr:`unsigned long`.

   Returns ``(unsigned long)-1`` on error.
   Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: size_t PyLong_AsSize_t(PyObject *pylong)

   .. index::
      single: SIZE_MAX
      single: OverflowError (built-in exception)

   Return a C :c:type:`size_t` representation of *pylong*.  *pylong* must be
   an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:type:`size_t`.

   Returns ``(size_t)-1`` on error.
   Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: unsigned long long PyLong_AsUnsignedLongLong(PyObject *pylong)

   .. index::
      single: OverflowError (built-in exception)

   Return a C :c:expr:`unsigned long long` representation of *pylong*.  *pylong*
   must be an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for an
   :c:expr:`unsigned long long`.

   Returns ``(unsigned long long)-1`` on error.
   Use :c:func:`PyErr_Occurred` to disambiguate.

   .. versionchanged:: 3.1
      A negative *pylong* now raises :exc:`OverflowError`, not :exc:`TypeError`.


.. c:function:: unsigned long PyLong_AsUnsignedLongMask(PyObject *obj)

   Return a C :c:expr:`unsigned long` representation of *obj*.  If *obj* is not
   an instance of :c:type:`PyLongObject`, first call its :meth:`~object.__index__`
   method (if present) to convert it to a :c:type:`PyLongObject`.

   If the value of *obj* is out of range for an :c:expr:`unsigned long`,
   return the reduction of that value modulo ``ULONG_MAX + 1``.

   Returns ``(unsigned long)-1`` on error.  Use :c:func:`PyErr_Occurred` to
   disambiguate.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: unsigned long long PyLong_AsUnsignedLongLongMask(PyObject *obj)

   Return a C :c:expr:`unsigned long long` representation of *obj*.  If *obj*
   is not an instance of :c:type:`PyLongObject`, first call its
   :meth:`~object.__index__` method (if present) to convert it to a
   :c:type:`PyLongObject`.

   If the value of *obj* is out of range for an :c:expr:`unsigned long long`,
   return the reduction of that value modulo ``ULLONG_MAX + 1``.

   Returns ``(unsigned long long)-1`` on error.  Use :c:func:`PyErr_Occurred`
   to disambiguate.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.

   .. versionchanged:: 3.10
      This function will no longer use :meth:`~object.__int__`.


.. c:function:: double PyLong_AsDouble(PyObject *pylong)

   Return a C :c:expr:`double` representation of *pylong*.  *pylong* must be
   an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:expr:`double`.

   Returns ``-1.0`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: void* PyLong_AsVoidPtr(PyObject *pylong)

   Convert a Python integer *pylong* to a C :c:expr:`void` pointer.
   If *pylong* cannot be converted, an :exc:`OverflowError` will be raised.  This
   is only assured to produce a usable :c:expr:`void` pointer for values created
   with :c:func:`PyLong_FromVoidPtr`.

   Returns ``NULL`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.
