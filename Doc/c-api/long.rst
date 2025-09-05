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


.. c:function:: PyObject* PyLong_FromInt32(int32_t value)
                PyObject* PyLong_FromInt64(int64_t value)

   Return a new :c:type:`PyLongObject` object from a signed C
   :c:expr:`int32_t` or :c:expr:`int64_t`, or ``NULL``
   with an exception set on failure.

   .. versionadded:: 3.14


.. c:function:: PyObject* PyLong_FromUnsignedLongLong(unsigned long long v)

   Return a new :c:type:`PyLongObject` object from a C :c:expr:`unsigned long long`,
   or ``NULL`` on failure.


.. c:function:: PyObject* PyLong_FromUInt32(uint32_t value)
                PyObject* PyLong_FromUInt64(uint64_t value)

   Return a new :c:type:`PyLongObject` object from an unsigned C
   :c:expr:`uint32_t` or :c:expr:`uint64_t`, or ``NULL``
   with an exception set on failure.

   .. versionadded:: 3.14


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

   .. seealso:: :c:func:`PyLong_AsNativeBytes()` and
      :c:func:`PyLong_FromNativeBytes()` functions can be used to convert
      a :c:type:`PyLongObject` to/from an array of bytes in base ``256``.


.. c:function:: PyObject* PyLong_FromUnicodeObject(PyObject *u, int base)

   Convert a sequence of Unicode digits in the string *u* to a Python integer
   value.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyLong_FromVoidPtr(void *p)

   Create a Python integer from the pointer *p*. The pointer value can be
   retrieved from the resulting value using :c:func:`PyLong_AsVoidPtr`.


.. c:function:: PyObject* PyLong_FromNativeBytes(const void* buffer, size_t n_bytes, int flags)

   Create a Python integer from the value contained in the first *n_bytes* of
   *buffer*, interpreted as a two's-complement signed number.

   *flags* are as for :c:func:`PyLong_AsNativeBytes`. Passing ``-1`` will select
   the native endian that CPython was compiled with and assume that the
   most-significant bit is a sign bit. Passing
   ``Py_ASNATIVEBYTES_UNSIGNED_BUFFER`` will produce the same result as calling
   :c:func:`PyLong_FromUnsignedNativeBytes`. Other flags are ignored.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyLong_FromUnsignedNativeBytes(const void* buffer, size_t n_bytes, int flags)

   Create a Python integer from the value contained in the first *n_bytes* of
   *buffer*, interpreted as an unsigned number.

   *flags* are as for :c:func:`PyLong_AsNativeBytes`. Passing ``-1`` will select
   the native endian that CPython was compiled with and assume that the
   most-significant bit is not a sign bit. Flags other than endian are ignored.

   .. versionadded:: 3.13


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

   .. c:namespace:: NULL

   .. c:function:: long PyLong_AS_LONG(PyObject *obj)

      A :term:`soft deprecated` alias.
      Exactly equivalent to the preferred ``PyLong_AsLong``. In particular,
      it can fail with :exc:`OverflowError` or another exception.

      .. deprecated:: 3.14
         The function is soft deprecated.

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


.. c:function:: int PyLong_AsInt32(PyObject *obj, int32_t *value)
                int PyLong_AsInt64(PyObject *obj, int64_t *value)

   Set *\*value* to a signed C :c:expr:`int32_t` or :c:expr:`int64_t`
   representation of *obj*.

   If *obj* is not an instance of :c:type:`PyLongObject`, first call its
   :meth:`~object.__index__` method (if present) to convert it to a
   :c:type:`PyLongObject`.

   If the *obj* value is out of range, raise an :exc:`OverflowError`.

   Set *\*value* and return ``0`` on success.
   Set an exception and return ``-1`` on error.

   *value* must not be ``NULL``.

   .. versionadded:: 3.14


.. c:function:: int PyLong_AsUInt32(PyObject *obj, uint32_t *value)
                int PyLong_AsUInt64(PyObject *obj, uint64_t *value)

   Set *\*value* to an unsigned C :c:expr:`uint32_t` or :c:expr:`uint64_t`
   representation of *obj*.

   If *obj* is not an instance of :c:type:`PyLongObject`, first call its
   :meth:`~object.__index__` method (if present) to convert it to a
   :c:type:`PyLongObject`.

   * If *obj* is negative, raise a :exc:`ValueError`.
   * If the *obj* value is out of range, raise an :exc:`OverflowError`.

   Set *\*value* and return ``0`` on success.
   Set an exception and return ``-1`` on error.

   *value* must not be ``NULL``.

   .. versionadded:: 3.14


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


.. c:function:: Py_ssize_t PyLong_AsNativeBytes(PyObject *pylong, void* buffer, Py_ssize_t n_bytes, int flags)

   Copy the Python integer value *pylong* to a native *buffer* of size
   *n_bytes*. The *flags* can be set to ``-1`` to behave similarly to a C cast,
   or to values documented below to control the behavior.

   Returns ``-1`` with an exception raised on error.  This may happen if
   *pylong* cannot be interpreted as an integer, or if *pylong* was negative
   and the ``Py_ASNATIVEBYTES_REJECT_NEGATIVE`` flag was set.

   Otherwise, returns the number of bytes required to store the value.
   If this is equal to or less than *n_bytes*, the entire value was copied.
   All *n_bytes* of the buffer are written: large buffers are padded with
   zeroes.

   If the returned value is greater than *n_bytes*, the value was
   truncated: as many of the lowest bits of the value as could fit are written,
   and the higher bits are ignored. This matches the typical behavior
   of a C-style downcast.

   .. note::

      Overflow is not considered an error. If the returned value
      is larger than *n_bytes*, most significant bits were discarded.

   ``0`` will never be returned.

   Values are always copied as two's-complement.

   Usage example::

      int32_t value;
      Py_ssize_t bytes = PyLong_AsNativeBytes(pylong, &value, sizeof(value), -1);
      if (bytes < 0) {
          // Failed. A Python exception was set with the reason.
          return NULL;
      }
      else if (bytes <= (Py_ssize_t)sizeof(value)) {
          // Success!
      }
      else {
          // Overflow occurred, but 'value' contains the truncated
          // lowest bits of pylong.
      }

   Passing zero to *n_bytes* will return the size of a buffer that would
   be large enough to hold the value. This may be larger than technically
   necessary, but not unreasonably so. If *n_bytes=0*, *buffer* may be
   ``NULL``.

   .. note::

      Passing *n_bytes=0* to this function is not an accurate way to determine
      the bit length of the value.

   To get at the entire Python value of an unknown size, the function can be
   called twice: first to determine the buffer size, then to fill it::

      // Ask how much space we need.
      Py_ssize_t expected = PyLong_AsNativeBytes(pylong, NULL, 0, -1);
      if (expected < 0) {
          // Failed. A Python exception was set with the reason.
          return NULL;
      }
      assert(expected != 0);  // Impossible per the API definition.
      uint8_t *bignum = malloc(expected);
      if (!bignum) {
          PyErr_SetString(PyExc_MemoryError, "bignum malloc failed.");
          return NULL;
      }
      // Safely get the entire value.
      Py_ssize_t bytes = PyLong_AsNativeBytes(pylong, bignum, expected, -1);
      if (bytes < 0) {  // Exception has been set.
          free(bignum);
          return NULL;
      }
      else if (bytes > expected) {  // This should not be possible.
          PyErr_SetString(PyExc_RuntimeError,
              "Unexpected bignum truncation after a size check.");
          free(bignum);
          return NULL;
      }
      // The expected success given the above pre-check.
      // ... use bignum ...
      free(bignum);

   *flags* is either ``-1`` (``Py_ASNATIVEBYTES_DEFAULTS``) to select defaults
   that behave most like a C cast, or a combination of the other flags in
   the table below.
   Note that ``-1`` cannot be combined with other flags.

   Currently, ``-1`` corresponds to
   ``Py_ASNATIVEBYTES_NATIVE_ENDIAN | Py_ASNATIVEBYTES_UNSIGNED_BUFFER``.

   .. c:namespace:: NULL

   ============================================= ======
   Flag                                          Value
   ============================================= ======
   .. c:macro:: Py_ASNATIVEBYTES_DEFAULTS        ``-1``
   .. c:macro:: Py_ASNATIVEBYTES_BIG_ENDIAN      ``0``
   .. c:macro:: Py_ASNATIVEBYTES_LITTLE_ENDIAN   ``1``
   .. c:macro:: Py_ASNATIVEBYTES_NATIVE_ENDIAN   ``3``
   .. c:macro:: Py_ASNATIVEBYTES_UNSIGNED_BUFFER ``4``
   .. c:macro:: Py_ASNATIVEBYTES_REJECT_NEGATIVE ``8``
   .. c:macro:: Py_ASNATIVEBYTES_ALLOW_INDEX     ``16``
   ============================================= ======

   Specifying ``Py_ASNATIVEBYTES_NATIVE_ENDIAN`` will override any other endian
   flags. Passing ``2`` is reserved.

   By default, sufficient buffer will be requested to include a sign bit.
   For example, when converting 128 with *n_bytes=1*, the function will return
   2 (or more) in order to store a zero sign bit.

   If ``Py_ASNATIVEBYTES_UNSIGNED_BUFFER`` is specified, a zero sign bit
   will be omitted from size calculations. This allows, for example, 128 to fit
   in a single-byte buffer. If the destination buffer is later treated as
   signed, a positive input value may become negative.
   Note that the flag does not affect handling of negative values: for those,
   space for a sign bit is always requested.

   Specifying ``Py_ASNATIVEBYTES_REJECT_NEGATIVE`` causes an exception to be set
   if *pylong* is negative. Without this flag, negative values will be copied
   provided there is enough space for at least one sign bit, regardless of
   whether ``Py_ASNATIVEBYTES_UNSIGNED_BUFFER`` was specified.

   If ``Py_ASNATIVEBYTES_ALLOW_INDEX`` is specified and a non-integer value is
   passed, its :meth:`~object.__index__` method will be called first. This may
   result in Python code executing and other threads being allowed to run, which
   could cause changes to other objects or values in use. When *flags* is
   ``-1``, this option is not set, and non-integer values will raise
   :exc:`TypeError`.

   .. note::

      With the default *flags* (``-1``, or *UNSIGNED_BUFFER*  without
      *REJECT_NEGATIVE*), multiple Python integers can map to a single value
      without overflow. For example, both ``255`` and ``-1`` fit a single-byte
      buffer and set all its bits.
      This matches typical C cast behavior.

   .. versionadded:: 3.13


.. c:function:: int PyLong_GetSign(PyObject *obj, int *sign)

   Get the sign of the integer object *obj*.

   On success, set *\*sign* to the integer sign  (0, -1 or +1 for zero, negative or
   positive integer, respectively) and return 0.

   On failure, return -1 with an exception set.  This function always succeeds
   if *obj* is a :c:type:`PyLongObject` or its subtype.

   .. versionadded:: 3.14


.. c:function:: int PyLong_IsPositive(PyObject *obj)

   Check if the integer object *obj* is positive (``obj > 0``).

   If *obj* is an instance of :c:type:`PyLongObject` or its subtype,
   return ``1`` when it's positive and ``0`` otherwise.  Else set an
   exception and return ``-1``.

   .. versionadded:: 3.14


.. c:function:: int PyLong_IsNegative(PyObject *obj)

   Check if the integer object *obj* is negative (``obj < 0``).

   If *obj* is an instance of :c:type:`PyLongObject` or its subtype,
   return ``1`` when it's negative and ``0`` otherwise.  Else set an
   exception and return ``-1``.

   .. versionadded:: 3.14


.. c:function:: int PyLong_IsZero(PyObject *obj)

   Check if the integer object *obj* is zero.

   If *obj* is an instance of :c:type:`PyLongObject` or its subtype,
   return ``1`` when it's zero and ``0`` otherwise.  Else set an
   exception and return ``-1``.

   .. versionadded:: 3.14


.. c:function:: PyObject* PyLong_GetInfo(void)

   On success, return a read only :term:`named tuple`, that holds
   information about Python's internal representation of integers.
   See :data:`sys.int_info` for description of individual fields.

   On failure, return ``NULL`` with an exception set.

   .. versionadded:: 3.1


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

   .. versionadded:: 3.12


.. c:function:: Py_ssize_t PyUnstable_Long_CompactValue(const PyLongObject* op)

   If *op* is compact, as determined by :c:func:`PyUnstable_Long_IsCompact`,
   return its value.

   Otherwise, the return value is undefined.

   .. versionadded:: 3.12


Export API
^^^^^^^^^^

.. versionadded:: 3.14

.. c:struct:: PyLongLayout

   Layout of an array of "digits" ("limbs" in the GMP terminology), used to
   represent absolute value for arbitrary precision integers.

   Use :c:func:`PyLong_GetNativeLayout` to get the native layout of Python
   :class:`int` objects, used internally for integers with "big enough"
   absolute value.

   See also :data:`sys.int_info` which exposes similar information in Python.

   .. c:member:: uint8_t bits_per_digit

      Bits per digit. For example, a 15 bit digit means that bits 0-14 contain
      meaningful information.

   .. c:member:: uint8_t digit_size

      Digit size in bytes. For example, a 15 bit digit will require at least 2
      bytes.

   .. c:member:: int8_t digits_order

      Digits order:

      - ``1`` for most significant digit first
      - ``-1`` for least significant digit first

   .. c:member:: int8_t digit_endianness

      Digit endianness:

      - ``1`` for most significant byte first (big endian)
      - ``-1`` for least significant byte first (little endian)


.. c:function:: const PyLongLayout* PyLong_GetNativeLayout(void)

   Get the native layout of Python :class:`int` objects.

   See the :c:struct:`PyLongLayout` structure.

   The function must not be called before Python initialization nor after
   Python finalization. The returned layout is valid until Python is
   finalized. The layout is the same for all Python sub-interpreters
   in a process, and so it can be cached.


.. c:struct:: PyLongExport

   Export of a Python :class:`int` object.

   There are two cases:

   * If :c:member:`digits` is ``NULL``, only use the :c:member:`value` member.
   * If :c:member:`digits` is not ``NULL``, use :c:member:`negative`,
     :c:member:`ndigits` and :c:member:`digits` members.

   .. c:member:: int64_t value

      The native integer value of the exported :class:`int` object.
      Only valid if :c:member:`digits` is ``NULL``.

   .. c:member:: uint8_t negative

      ``1`` if the number is negative, ``0`` otherwise.
      Only valid if :c:member:`digits` is not ``NULL``.

   .. c:member:: Py_ssize_t ndigits

      Number of digits in :c:member:`digits` array.
      Only valid if :c:member:`digits` is not ``NULL``.

   .. c:member:: const void *digits

      Read-only array of unsigned digits. Can be ``NULL``.


.. c:function:: int PyLong_Export(PyObject *obj, PyLongExport *export_long)

   Export a Python :class:`int` object.

   *export_long* must point to a :c:struct:`PyLongExport` structure allocated
   by the caller. It must not be ``NULL``.

   On success, fill in *\*export_long* and return ``0``.
   On error, set an exception and return ``-1``.

   :c:func:`PyLong_FreeExport` must be called when the export is no longer
   needed.

    .. impl-detail::
        This function always succeeds if *obj* is a Python :class:`int` object
        or a subclass.


.. c:function:: void PyLong_FreeExport(PyLongExport *export_long)

   Release the export *export_long* created by :c:func:`PyLong_Export`.

   .. impl-detail::
      Calling :c:func:`PyLong_FreeExport` is optional if *export_long->digits*
      is ``NULL``.


PyLongWriter API
^^^^^^^^^^^^^^^^

The :c:type:`PyLongWriter` API can be used to import an integer.

.. versionadded:: 3.14

.. c:struct:: PyLongWriter

   A Python :class:`int` writer instance.

   The instance must be destroyed by :c:func:`PyLongWriter_Finish` or
   :c:func:`PyLongWriter_Discard`.


.. c:function:: PyLongWriter* PyLongWriter_Create(int negative, Py_ssize_t ndigits, void **digits)

   Create a :c:type:`PyLongWriter`.

   On success, allocate *\*digits* and return a writer.
   On error, set an exception and return ``NULL``.

   *negative* is ``1`` if the number is negative, or ``0`` otherwise.

   *ndigits* is the number of digits in the *digits* array. It must be
   greater than 0.

   *digits* must not be NULL.

   After a successful call to this function, the caller should fill in the
   array of digits *digits* and then call :c:func:`PyLongWriter_Finish` to get
   a Python :class:`int`.
   The layout of *digits* is described by :c:func:`PyLong_GetNativeLayout`.

   Digits must be in the range [``0``; ``(1 << bits_per_digit) - 1``]
   (where the :c:struct:`~PyLongLayout.bits_per_digit` is the number of bits
   per digit).
   Any unused most significant digits must be set to ``0``.

   Alternately, call :c:func:`PyLongWriter_Discard` to destroy the writer
   instance without creating an :class:`~int` object.


.. c:function:: PyObject* PyLongWriter_Finish(PyLongWriter *writer)

   Finish a :c:type:`PyLongWriter` created by :c:func:`PyLongWriter_Create`.

   On success, return a Python :class:`int` object.
   On error, set an exception and return ``NULL``.

   The function takes care of normalizing the digits and converts the object
   to a compact integer if needed.

   The writer instance and the *digits* array are invalid after the call.


.. c:function:: void PyLongWriter_Discard(PyLongWriter *writer)

   Discard a :c:type:`PyLongWriter` created by :c:func:`PyLongWriter_Create`.

   If *writer* is ``NULL``, no operation is performed.

   The writer instance and the *digits* array are invalid after the call.
