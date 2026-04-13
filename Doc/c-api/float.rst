.. highlight:: c

.. _floatobjects:

Floating-Point Objects
======================

.. index:: pair: object; floating-point


.. c:type:: PyFloatObject

   This subtype of :c:type:`PyObject` represents a Python floating-point object.


.. c:var:: PyTypeObject PyFloat_Type

   This instance of :c:type:`PyTypeObject` represents the Python floating-point
   type.  This is the same object as :class:`float` in the Python layer.


.. c:function:: int PyFloat_Check(PyObject *p)

   Return true if its argument is a :c:type:`PyFloatObject` or a subtype of
   :c:type:`PyFloatObject`.  This function always succeeds.


.. c:function:: int PyFloat_CheckExact(PyObject *p)

   Return true if its argument is a :c:type:`PyFloatObject`, but not a subtype of
   :c:type:`PyFloatObject`.  This function always succeeds.


.. c:function:: PyObject* PyFloat_FromString(PyObject *str)

   Create a :c:type:`PyFloatObject` object based on the string value in *str*, or
   ``NULL`` on failure.


.. c:function:: PyObject* PyFloat_FromDouble(double v)

   Create a :c:type:`PyFloatObject` object from *v*, or ``NULL`` on failure.


.. c:function:: double PyFloat_AsDouble(PyObject *pyfloat)

   Return a C :c:expr:`double` representation of the contents of *pyfloat*.  If
   *pyfloat* is not a Python floating-point object but has a :meth:`~object.__float__`
   method, this method will first be called to convert *pyfloat* into a float.
   If :meth:`!__float__` is not defined then it falls back to :meth:`~object.__index__`.
   This method returns ``-1.0`` upon failure, so one should call
   :c:func:`PyErr_Occurred` to check for errors.

   .. versionchanged:: 3.8
      Use :meth:`~object.__index__` if available.


.. c:function:: double PyFloat_AS_DOUBLE(PyObject *pyfloat)

   Return a C :c:expr:`double` representation of the contents of *pyfloat*, but
   without error checking.


.. c:function:: PyObject* PyFloat_GetInfo(void)

   Return a structseq instance which contains information about the
   precision, minimum and maximum values of a float. It's a thin wrapper
   around the header file :file:`float.h`.


.. c:function:: double PyFloat_GetMax()

   Return the maximum representable finite float *DBL_MAX* as C :c:expr:`double`.


.. c:function:: double PyFloat_GetMin()

   Return the minimum normalized positive float *DBL_MIN* as C :c:expr:`double`.


.. c:macro:: Py_INFINITY

   This macro expands to a constant expression of type :c:expr:`double`, that
   represents the positive infinity.

   It is equivalent to the :c:macro:`!INFINITY` macro from the C11 standard
   ``<math.h>`` header.

   .. deprecated:: 3.15
      The macro is :term:`soft deprecated`.


.. c:macro:: Py_NAN

   This macro expands to a constant expression of type :c:expr:`double`, that
   represents a quiet not-a-number (qNaN) value.

   On most platforms, this is equivalent to the :c:macro:`!NAN` macro from
   the C11 standard ``<math.h>`` header.


.. c:macro:: Py_HUGE_VAL

   Equivalent to :c:macro:`!INFINITY`.

   .. deprecated:: 3.14
      The macro is :term:`soft deprecated`.


.. c:macro:: Py_MATH_E

   The definition (accurate for a :c:expr:`double` type) of the :data:`math.e` constant.


.. c:macro:: Py_MATH_El

   High precision (long double) definition of :data:`~math.e` constant.

   .. deprecated-removed:: 3.15 3.20


.. c:macro:: Py_MATH_PI

   The definition (accurate for a :c:expr:`double` type) of the :data:`math.pi` constant.


.. c:macro:: Py_MATH_PIl

   High precision (long double) definition of :data:`~math.pi` constant.

   .. deprecated-removed:: 3.15 3.20


.. c:macro:: Py_MATH_TAU

   The definition (accurate for a :c:expr:`double` type) of the :data:`math.tau` constant.

   .. versionadded:: 3.6


.. c:macro:: Py_RETURN_NAN

   Return :data:`math.nan` from a function.

   On most platforms, this is equivalent to ``return PyFloat_FromDouble(NAN)``.


.. c:macro:: Py_RETURN_INF(sign)

   Return :data:`math.inf` or :data:`-math.inf <math.inf>` from a function,
   depending on the sign of *sign*.

   On most platforms, this is equivalent to the following::

      return PyFloat_FromDouble(copysign(INFINITY, sign));


.. c:macro:: Py_IS_FINITE(X)

   Return ``1`` if the given floating-point number *X* is finite,
   that is, it is normal, subnormal or zero, but not infinite or NaN.
   Return ``0`` otherwise.

   .. deprecated:: 3.14
      The macro is :term:`soft deprecated`.  Use :c:macro:`!isfinite` instead.


.. c:macro:: Py_IS_INFINITY(X)

   Return ``1`` if the given floating-point number *X* is positive or negative
   infinity.  Return ``0`` otherwise.

   .. deprecated:: 3.14
      The macro is :term:`soft deprecated`.  Use :c:macro:`!isinf` instead.


.. c:macro:: Py_IS_NAN(X)

   Return ``1`` if the given floating-point number *X* is a not-a-number (NaN)
   value.  Return ``0`` otherwise.

   .. deprecated:: 3.14
      The macro is :term:`soft deprecated`.  Use :c:macro:`!isnan` instead.


Pack and Unpack functions
-------------------------

The pack and unpack functions provide an efficient platform-independent way to
store floating-point values as byte strings. The Pack routines produce a bytes
string from a C :c:expr:`double`, and the Unpack routines produce a C
:c:expr:`double` from such a bytes string. The suffix (2, 4 or 8) specifies the
number of bytes in the bytes string:

* The 2-byte format is the IEEE 754 binary16 half-precision format.
* The 4-byte format is the IEEE 754 binary32 single-precision format.
* The 8-byte format is the IEEE 754 binary64 double-precision format.

The NaN type may not be preserved on some platforms while unpacking (signaling
NaNs become quiet NaNs), for example on x86 systems in 32-bit mode.

It's assumed that the :c:expr:`double` type has the IEEE 754 binary64 double
precision format.  What happens if it's not true is partly accidental (alas).
On non-IEEE platforms with more precision, or larger dynamic range, than IEEE
754 supports, not all values can be packed; on non-IEEE platforms with less
precision, or smaller dynamic range, not all values can be unpacked.  The
packing of special numbers like INFs and NaNs (if such things exist on the
platform) may not be handled correctly, and attempting to unpack a bytes string
containing an IEEE INF or NaN may raise an exception.

.. versionadded:: 3.11

Pack functions
^^^^^^^^^^^^^^

The pack routines write 2, 4 or 8 bytes, starting at *p*. *le* is an
:c:expr:`int` argument, non-zero if you want the bytes string in little-endian
format (exponent last, at ``p+1``, ``p+3``, or ``p+6`` and ``p+7``), zero if you
want big-endian format (exponent first, at *p*). Use the :c:macro:`!PY_LITTLE_ENDIAN`
constant to select the native endian: it is equal to ``0`` on big
endian processor, or ``1`` on little endian processor.

Return value: ``0`` if all is OK, ``-1`` if error (and an exception is set,
most likely :exc:`OverflowError`).

.. c:function:: int PyFloat_Pack2(double x, char *p, int le)

   Pack a C double as the IEEE 754 binary16 half-precision format.

.. c:function:: int PyFloat_Pack4(double x, char *p, int le)

   Pack a C double as the IEEE 754 binary32 single precision format.

.. c:function:: int PyFloat_Pack8(double x, char *p, int le)

   Pack a C double as the IEEE 754 binary64 double precision format.

   .. impl-detail::
      This function always succeeds in CPython.


Unpack functions
^^^^^^^^^^^^^^^^

The unpack routines read 2, 4 or 8 bytes, starting at *p*.  *le* is an
:c:expr:`int` argument, non-zero if the bytes string is in little-endian format
(exponent last, at ``p+1``, ``p+3`` or ``p+6`` and ``p+7``), zero if big-endian
(exponent first, at *p*). Use the :c:macro:`!PY_LITTLE_ENDIAN` constant to
select the native endian: it is equal to ``0`` on big endian processor, or ``1``
on little endian processor.

Return value: The unpacked double.  On error, this is ``-1.0`` and
:c:func:`PyErr_Occurred` is true (and an exception is set, most likely
:exc:`OverflowError`).

.. impl-detail::
    These functions always succeed in CPython.

.. c:function:: double PyFloat_Unpack2(const char *p, int le)

   Unpack the IEEE 754 binary16 half-precision format as a C double.

.. c:function:: double PyFloat_Unpack4(const char *p, int le)

   Unpack the IEEE 754 binary32 single precision format as a C double.

.. c:function:: double PyFloat_Unpack8(const char *p, int le)

   Unpack the IEEE 754 binary64 double precision format as a C double.
