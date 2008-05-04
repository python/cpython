.. highlightlang:: c

.. _number:

Number Protocol
===============


.. cfunction:: int PyNumber_Check(PyObject *o)

   Returns ``1`` if the object *o* provides numeric protocols, and false otherwise.
   This function always succeeds.


.. cfunction:: PyObject* PyNumber_Add(PyObject *o1, PyObject *o2)

   Returns the result of adding *o1* and *o2*, or *NULL* on failure.  This is the
   equivalent of the Python expression ``o1 + o2``.


.. cfunction:: PyObject* PyNumber_Subtract(PyObject *o1, PyObject *o2)

   Returns the result of subtracting *o2* from *o1*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 - o2``.


.. cfunction:: PyObject* PyNumber_Multiply(PyObject *o1, PyObject *o2)

   Returns the result of multiplying *o1* and *o2*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 * o2``.


.. cfunction:: PyObject* PyNumber_Divide(PyObject *o1, PyObject *o2)

   Returns the result of dividing *o1* by *o2*, or *NULL* on failure.  This is the
   equivalent of the Python expression ``o1 / o2``.


.. cfunction:: PyObject* PyNumber_FloorDivide(PyObject *o1, PyObject *o2)

   Return the floor of *o1* divided by *o2*, or *NULL* on failure.  This is
   equivalent to the "classic" division of integers.


.. cfunction:: PyObject* PyNumber_TrueDivide(PyObject *o1, PyObject *o2)

   Return a reasonable approximation for the mathematical value of *o1* divided by
   *o2*, or *NULL* on failure.  The return value is "approximate" because binary
   floating point numbers are approximate; it is not possible to represent all real
   numbers in base two.  This function can return a floating point value when
   passed two integers.


.. cfunction:: PyObject* PyNumber_Remainder(PyObject *o1, PyObject *o2)

   Returns the remainder of dividing *o1* by *o2*, or *NULL* on failure.  This is
   the equivalent of the Python expression ``o1 % o2``.


.. cfunction:: PyObject* PyNumber_Divmod(PyObject *o1, PyObject *o2)

   .. index:: builtin: divmod

   See the built-in function :func:`divmod`. Returns *NULL* on failure.  This is
   the equivalent of the Python expression ``divmod(o1, o2)``.


.. cfunction:: PyObject* PyNumber_Power(PyObject *o1, PyObject *o2, PyObject *o3)

   .. index:: builtin: pow

   See the built-in function :func:`pow`. Returns *NULL* on failure.  This is the
   equivalent of the Python expression ``pow(o1, o2, o3)``, where *o3* is optional.
   If *o3* is to be ignored, pass :cdata:`Py_None` in its place (passing *NULL* for
   *o3* would cause an illegal memory access).


.. cfunction:: PyObject* PyNumber_Negative(PyObject *o)

   Returns the negation of *o* on success, or *NULL* on failure. This is the
   equivalent of the Python expression ``-o``.


.. cfunction:: PyObject* PyNumber_Positive(PyObject *o)

   Returns *o* on success, or *NULL* on failure.  This is the equivalent of the
   Python expression ``+o``.


.. cfunction:: PyObject* PyNumber_Absolute(PyObject *o)

   .. index:: builtin: abs

   Returns the absolute value of *o*, or *NULL* on failure.  This is the equivalent
   of the Python expression ``abs(o)``.


.. cfunction:: PyObject* PyNumber_Invert(PyObject *o)

   Returns the bitwise negation of *o* on success, or *NULL* on failure.  This is
   the equivalent of the Python expression ``~o``.


.. cfunction:: PyObject* PyNumber_Lshift(PyObject *o1, PyObject *o2)

   Returns the result of left shifting *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 << o2``.


.. cfunction:: PyObject* PyNumber_Rshift(PyObject *o1, PyObject *o2)

   Returns the result of right shifting *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 >> o2``.


.. cfunction:: PyObject* PyNumber_And(PyObject *o1, PyObject *o2)

   Returns the "bitwise and" of *o1* and *o2* on success and *NULL* on failure.
   This is the equivalent of the Python expression ``o1 & o2``.


.. cfunction:: PyObject* PyNumber_Xor(PyObject *o1, PyObject *o2)

   Returns the "bitwise exclusive or" of *o1* by *o2* on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o1 ^ o2``.


.. cfunction:: PyObject* PyNumber_Or(PyObject *o1, PyObject *o2)

   Returns the "bitwise or" of *o1* and *o2* on success, or *NULL* on failure.
   This is the equivalent of the Python expression ``o1 | o2``.


.. cfunction:: PyObject* PyNumber_InPlaceAdd(PyObject *o1, PyObject *o2)

   Returns the result of adding *o1* and *o2*, or *NULL* on failure.  The operation
   is done *in-place* when *o1* supports it.  This is the equivalent of the Python
   statement ``o1 += o2``.


.. cfunction:: PyObject* PyNumber_InPlaceSubtract(PyObject *o1, PyObject *o2)

   Returns the result of subtracting *o2* from *o1*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 -= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceMultiply(PyObject *o1, PyObject *o2)

   Returns the result of multiplying *o1* and *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 *= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceDivide(PyObject *o1, PyObject *o2)

   Returns the result of dividing *o1* by *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it. This is the equivalent of
   the Python statement ``o1 /= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceFloorDivide(PyObject *o1, PyObject *o2)

   Returns the mathematical floor of dividing *o1* by *o2*, or *NULL* on failure.
   The operation is done *in-place* when *o1* supports it.  This is the equivalent
   of the Python statement ``o1 //= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceTrueDivide(PyObject *o1, PyObject *o2)

   Return a reasonable approximation for the mathematical value of *o1* divided by
   *o2*, or *NULL* on failure.  The return value is "approximate" because binary
   floating point numbers are approximate; it is not possible to represent all real
   numbers in base two.  This function can return a floating point value when
   passed two integers.  The operation is done *in-place* when *o1* supports it.


.. cfunction:: PyObject* PyNumber_InPlaceRemainder(PyObject *o1, PyObject *o2)

   Returns the remainder of dividing *o1* by *o2*, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 %= o2``.


.. cfunction:: PyObject* PyNumber_InPlacePower(PyObject *o1, PyObject *o2, PyObject *o3)

   .. index:: builtin: pow

   See the built-in function :func:`pow`. Returns *NULL* on failure.  The operation
   is done *in-place* when *o1* supports it.  This is the equivalent of the Python
   statement ``o1 **= o2`` when o3 is :cdata:`Py_None`, or an in-place variant of
   ``pow(o1, o2, o3)`` otherwise. If *o3* is to be ignored, pass :cdata:`Py_None`
   in its place (passing *NULL* for *o3* would cause an illegal memory access).


.. cfunction:: PyObject* PyNumber_InPlaceLshift(PyObject *o1, PyObject *o2)

   Returns the result of left shifting *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 <<= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceRshift(PyObject *o1, PyObject *o2)

   Returns the result of right shifting *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 >>= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceAnd(PyObject *o1, PyObject *o2)

   Returns the "bitwise and" of *o1* and *o2* on success and *NULL* on failure. The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 &= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceXor(PyObject *o1, PyObject *o2)

   Returns the "bitwise exclusive or" of *o1* by *o2* on success, or *NULL* on
   failure.  The operation is done *in-place* when *o1* supports it.  This is the
   equivalent of the Python statement ``o1 ^= o2``.


.. cfunction:: PyObject* PyNumber_InPlaceOr(PyObject *o1, PyObject *o2)

   Returns the "bitwise or" of *o1* and *o2* on success, or *NULL* on failure.  The
   operation is done *in-place* when *o1* supports it.  This is the equivalent of
   the Python statement ``o1 |= o2``.


.. cfunction:: PyObject* PyNumber_Int(PyObject *o)

   .. index:: builtin: int

   Returns the *o* converted to an integer object on success, or *NULL* on failure.
   If the argument is outside the integer range a long object will be returned
   instead. This is the equivalent of the Python expression ``int(o)``.


.. cfunction:: PyObject* PyNumber_Long(PyObject *o)

   .. index:: builtin: long

   Returns the *o* converted to an integer object on success, or *NULL* on
   failure.  This is the equivalent of the Python expression ``long(o)``.


.. cfunction:: PyObject* PyNumber_Float(PyObject *o)

   .. index:: builtin: float

   Returns the *o* converted to a float object on success, or *NULL* on failure.
   This is the equivalent of the Python expression ``float(o)``.


.. cfunction:: PyObject* PyNumber_Index(PyObject *o)

   Returns the *o* converted to a Python int or long on success or *NULL* with a
   TypeError exception raised on failure.


.. cfunction:: PyObject* PyNumber_ToBase(PyObject *n, int base)

   Returns the the integer *n* converted to *base* as a string with a base
   marker of ``'0b'``, ``'0o'``, or ``'0x'`` if appended applicable.  When
   *base* is not 2, 8, 10, or 16, the format is ``'x#num'`` where x is the
   base. If *n* is not an int object, it is converted with
   :cfunc:`PyNumber_Index` first.

   .. versionadded:: 2.6


.. cfunction:: Py_ssize_t PyNumber_AsSsize_t(PyObject *o, PyObject *exc)

   Returns *o* converted to a Py_ssize_t value if *o* can be interpreted as an
   integer. If *o* can be converted to a Python int or long but the attempt to
   convert to a Py_ssize_t value would raise an :exc:`OverflowError`, then the
   *exc* argument is the type of exception that will be raised (usually
   :exc:`IndexError` or :exc:`OverflowError`).  If *exc* is *NULL*, then the
   exception is cleared and the value is clipped to *PY_SSIZE_T_MIN* for a negative
   integer or *PY_SSIZE_T_MAX* for a positive integer.


.. cfunction:: int PyIndex_Check(PyObject *o)

   Returns True if *o* is an index integer (has the nb_index slot of  the
   tp_as_number structure filled in).
