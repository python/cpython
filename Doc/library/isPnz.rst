.. function:: isPnz(x)

   Determine if a number is positive, negative, or zero.

   :param x: A number (integer or float)
   :type x: int or float
   :return: The sign of the number
   :rtype: bool or None
   :raises TypeError: if the input is not a number
   :raises OverflowError: if the input is too large or too small
   :raises ValueError: if the input is NaN

   Returns:
     * ``True`` if *x* is positive (non-zero)
     * ``False`` if *x* is negative (non-zero) or negative zero (-0.0)
     * ``None`` if *x* is positive zero (+0.0)

   This function uses ``copysign`` to detect signed zeros,
   ensuring compatibility with IEEE 754 floating-point standards.

   The function can accept both integer and floating-point values. 
   Integer values are automatically converted to floating-point numbers before comparison.

     Example::

      >>> isPnz(10)
      True
      >>> isPnz(-5)
      False
      >>> isPnz(0.0)
      None
      >>> isPnz(-0.0)
      False
      >>> isPnz("text")
      TypeError: Argument must be an int or float

   .. note::
      This function is intended for use with numbers following the IEEE 754 standard for floating-point arithmetic.

   .. versionadded:: 3.12