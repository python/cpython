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


Pack and Unpack functions
-------------------------

The pack and unpack functions provide an efficient platform-independent way to
store floating-point values as byte strings. The Pack routines produce a bytes
string from a C :c:expr:`double`, and the Unpack routines produce a C
:c:expr:`double` from such a bytes string. The suffix (2, 4 or 8) specifies the
number of bytes in the bytes string.

On platforms that appear to use IEEE 754 formats these functions work by
copying bits. On other platforms, the 2-byte format is identical to the IEEE
754 binary16 half-precision format, the 4-byte format (32-bit) is identical to
the IEEE 754 binary32 single precision format, and the 8-byte format to the
IEEE 754 binary64 double precision format, although the packing of INFs and
NaNs (if such things exist on the platform) isn't handled correctly, and
attempting to unpack a bytes string containing an IEEE INF or NaN will raise an
exception.

Note that NaNs type may not be preserved on IEEE platforms (silent NaN become
quiet), for example on x86 systems in 32-bit mode.

On non-IEEE platforms with more precision, or larger dynamic range, than IEEE
754 supports, not all values can be packed; on non-IEEE platforms with less
precision, or smaller dynamic range, not all values can be unpacked. What
happens in such cases is partly accidental (alas).

.. versionadded:: 3.11

Pack functions
^^^^^^^^^^^^^^

The pack routines write 2, 4 or 8 bytes, starting at *p*. *le* is an
:c:expr:`int` argument, non-zero if you want the bytes string in little-endian
format (exponent last, at ``p+1``, ``p+3``, or ``p+6`` ``p+7``), zero if you
want big-endian format (exponent first, at *p*). The :c:macro:`PY_BIG_ENDIAN`
constant can be used to use the native endian: it is equal to ``1`` on big
endian processor, or ``0`` on little endian processor.

Return value: ``0`` if all is OK, ``-1`` if error (and an exception is set,
most likely :exc:`OverflowError`).

There are two problems on non-IEEE platforms:

* What this does is undefined if *x* is a NaN or infinity.
* ``-0.0`` and ``+0.0`` produce the same bytes string.

.. c:function:: int PyFloat_Pack2(double x, unsigned char *p, int le)

   Pack a C double as the IEEE 754 binary16 half-precision format.

.. c:function:: int PyFloat_Pack4(double x, unsigned char *p, int le)

   Pack a C double as the IEEE 754 binary32 single precision format.

.. c:function:: int PyFloat_Pack8(double x, unsigned char *p, int le)

   Pack a C double as the IEEE 754 binary64 double precision format.


Unpack functions
^^^^^^^^^^^^^^^^

The unpack routines read 2, 4 or 8 bytes, starting at *p*.  *le* is an
:c:expr:`int` argument, non-zero if the bytes string is in little-endian format
(exponent last, at ``p+1``, ``p+3`` or ``p+6`` and ``p+7``), zero if big-endian
(exponent first, at *p*). The :c:macro:`PY_BIG_ENDIAN` constant can be used to
use the native endian: it is equal to ``1`` on big endian processor, or ``0``
on little endian processor.

Return value: The unpacked double.  On error, this is ``-1.0`` and
:c:func:`PyErr_Occurred` is true (and an exception is set, most likely
:exc:`OverflowError`).

Note that on a non-IEEE platform this will refuse to unpack a bytes string that
represents a NaN or infinity.

.. c:function:: double PyFloat_Unpack2(const unsigned char *p, int le)

   Unpack the IEEE 754 binary16 half-precision format as a C double.

.. c:function:: double PyFloat_Unpack4(const unsigned char *p, int le)

   Unpack the IEEE 754 binary32 single precision format as a C double.

.. c:function:: double PyFloat_Unpack8(const unsigned char *p, int le)

   Unpack the IEEE 754 binary64 double precision format as a C double.
