Parameters of the numeric hash implementation
=============================================

See :ref:`numeric-hash` for more details about hashing of numeric types.

.. c:macro:: PyUnstable_PyHASH_MODULUS

   The Mersenne prime ``P = 2**n -1``, used for numeric hash scheme.

.. c:macro:: PyUnstable_PyHASH_BITS

   The exponent ``n`` of ``P``.

.. c:macro:: PyUnstable_PyHASH_INF

   The hash value returned for a positive infinity.

.. c:macro:: PyUnstable_PyHASH_IMAG

   The multiplier used for the imaginary part of a complex number.
