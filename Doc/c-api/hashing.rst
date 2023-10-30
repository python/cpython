Parameters of the numeric hash implementation
=============================================

See :ref:`numeric-hash` for more details about hashing of numeric types.

.. c:macro:: _PyHASH_MODULUS

   The Mersenne prime ``P = 2**n -1``, used for numeric hash scheme.

.. c:macro:: _PyHASH_BITS

   The exponent ``n`` of ``P``.

.. c:macro:: _PyHASH_INF

   The hash value returned for a positive infinity.

.. c:macro:: _PyHASH_IMAG

   The multiplier used for the imaginary part of a complex number.
