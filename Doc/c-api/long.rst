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
   is interpreted according to the radix in *base*, or ``NULL`` on failure.  If
   *pend* is non-``NULL``, *\*pend* will point to the end of *str* on success or
   to the first character that could not be processed on error.  If *base* is ``0``,
   *str* is interpreted using the :ref:`integers` definition; in this case, leading
   zeros in a non-zero decimal number raises a :exc:`ValueError`.  If *base* is not
   ``0``, it must be between ``2`` and ``36``, inclusive.  Leading and trailing
   whitespace and single underscores after a base specifier and between digits are
   ignored.  If there are no digits or *str* is not NULL-terminated following the
   digits and trailing whitespace, :exc:`ValueError` will be raised.

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


.. c:function:: PyObject* PyLong_FromNativeBytes(const void* buffer, size_t n_bytes, int endianness)

   Create a Python integer from the value contained in the first *n_bytes* of
   *buffer*, interpreted as a two's-complement signed number.

   *endianness* may be passed ``-1`` for the native endian that CPython was
   compiled with, or else ``0`` for big endian and ``1`` for little.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyLong_FromUnsignedNativeBytes(const void* buffer, size_t n_bytes, int endianness)

   Create a Python integer from the value contained in the first *n_bytes* of
   *buffer*, interpreted as an unsigned number.

   *endianness* may be passed ``-1`` for the native endian that CPython was
   compiled with, or else ``0`` for big endian and ``1`` for little.

   .. versionadded:: 3.13


.. XXX alias PyLong_AS_LONG (for now)
.. c:function:: long PyLong_AsLong(PyObject *obj)

   .. index::
      single: LONG_MAX (C macro)
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


.. c:function:: int PyLong_AsInt(PyObject *obj)

   Similar to :c:func:`PyLong_AsLong`, but store the result in a C
   :c:expr:`int` instead of a C :c:expr:`long`.

   .. versionadded:: 3.13


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
      single: PY_SSIZE_T_MAX (C macro)
      single: OverflowError (built-in exception)

   Return a C :c:type:`Py_ssize_t` representation of *pylong*.  *pylong* must
   be an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:type:`Py_ssize_t`.

   Returns ``-1`` on error.  Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: unsigned long PyLong_AsUnsignedLong(PyObject *pylong)

   .. index::
      single: ULONG_MAX (C macro)
      single: OverflowError (built-in exception)

   Return a C :c:expr:`unsigned long` representation of *pylong*.  *pylong*
   must be an instance of :c:type:`PyLongObject`.

   Raise :exc:`OverflowError` if the value of *pylong* is out of range for a
   :c:expr:`unsigned long`.

   Returns ``(unsigned long)-1`` on error.
   Use :c:func:`PyErr_Occurred` to disambiguate.


.. c:function:: size_t PyLong_AsSize_t(PyObject *pylong)

   .. index::
      single: SIZE_MAX (C macro)
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


.. c:function:: Py_ssize_t PyLong_AsNativeBytes(PyObject *pylong, void* buffer, Py_ssize_t n_bytes, int endianness)

   Copy the Python integer value to a native *buffer* of size *n_bytes*::

      int32_t value;
      Py_ssize_t bytes = PyLong_AsNativeBits(pylong, &value, sizeof(value), -1);
      if (bytes < 0) {
          // A Python exception was set with the reason.
          return NULL;
      }
      else if (bytes <= (Py_ssize_t)sizeof(value)) {
          // Success!
      }
      else {
          // Overflow occurred, but 'value' contains the truncated
          // lowest bits of pylong.
      }

   The above example may look *similar* to
   :c:func:`PyLong_As* <PyLong_AsSize_t>`
   but instead fills in a specific caller defined type and never raises an
   error about of the :class:`int` *pylong*'s value regardless of *n_bytes*
   or the returned byte count.

   To get at the entire potentially big Python value, this can be used to
   reserve enough space and copy it::

      // Ask how much space we need.
      Py_ssize_t expected = PyLong_AsNativeBits(pylong, NULL, 0, -1);
      if (expected < 0) {
          // A Python exception was set with the reason.
          return NULL;
      }
      assert(expected != 0);  // Impossible per the API definition.
      uint8_t *bignum = malloc(expected);
      if (!bignum) {
          PyErr_SetString(PyExc_MemoryError, "bignum malloc failed.");
          return NULL;
      }
      // Safely get the entire value.
      Py_ssize_t bytes = PyLong_AsNativeBits(pylong, bignum, expected, -1);
      if (bytes < 0) {  // Exception set.
          free(bignum);
          return NULL;
      }
      else if (bytes > expected) {  // Be safe, should not be possible.
          PyErr_SetString(PyExc_RuntimeError,
              "Unexpected bignum truncation after a size check.");
          free(bignum);
          return NULL;
      }
      // The expected success given the above pre-check.
      // ... use bignum ...
      free(bignum);

   *endianness* may be passed ``-1`` for the native endian that CPython was
   compiled with, or ``0`` for big endian and ``1`` for little.

   Returns ``-1`` with an exception raised if *pylong* cannot be interpreted as
   an integer. Otherwise, return the size of the buffer required to store the
   value. If this is equal to or less than *n_bytes*, the entire value was
   copied. ``0`` will never be returned.

   Unless an exception is raised, all *n_bytes* of the buffer will always be
   written. In the case of truncation, as many of the lowest bits of the value
   as could fit are written. This allows the caller to ignore all non-negative
   results if the intent is to match the typical behavior of a C-style
   downcast. No exception is set on truncation.

   Values are always copied as two's-complement and sufficient buffer will be
   requested to include a sign bit. For example, this may cause an value that
   fits into 8 bytes when treated as unsigned to request 9 bytes, even though
   all eight bytes were copied into the buffer. What has been omitted is the
   zero sign bit -- redundant if the caller's intention is to treat the value
   as unsigned.

   Passing zero to *n_bytes* will return the size of a buffer that would
   be large enough to hold the value. This may be larger than technically
   necessary, but not unreasonably so.

   .. note::

      Passing *n_bytes=0* to this function is not an accurate way to determine
      the bit length of a value.

   .. versionadded:: 3.13


.. c:function:: int PyUnstable_Long_IsCompact(const PyLongObject* op)

   Return 1 if *op* is compact, 0 otherwise.

   This function makes it possible for performance-critical code to implement
   a “fast path” for small integers. For compact values use
   :c:func:`PyUnstable_Long_CompactValue`; for others fall back to a
   :c:func:`PyLong_As* <PyLong_AsSize_t>` function or
   :c:func:`PyLong_AsNativeBytes`.

   The speedup is expected to be negligible for most users.

   Exactly what values are considered compact is an implementation detail
   and is subject to change.

.. c:function:: Py_ssize_t PyUnstable_Long_CompactValue(const PyLongObject* op)

   If *op* is compact, as determined by :c:func:`PyUnstable_Long_IsCompact`,
   return its value.

   Otherwise, the return value is undefined.

