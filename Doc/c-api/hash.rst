.. highlight:: c

PyHash API
----------

See also the :c:member:`PyTypeObject.tp_hash` member and :ref:`numeric-hash`
for more details about hashing of numeric types.

.. c:type:: Py_hash_t

   Hash value type: signed integer.

   .. versionadded:: 3.2

.. c:type:: Py_uhash_t

   Hash value type: unsigned integer.

   .. versionadded:: 3.2

.. c:macro:: PyHASH_MODULUS

   The Mersenne prime ``P = 2**n -1``, used for numeric hash scheme.

   .. versionadded:: 3.13

.. c:macro:: PyHASH_BITS

   The exponent ``n`` of ``P``.

   .. versionadded:: 3.13

.. c:macro:: PyHASH_INF

   The hash value returned for a positive infinity.

   .. versionadded:: 3.13

.. c:macro:: PyHASH_IMAG

   The multiplier used for the imaginary part of a complex number.

   .. versionadded:: 3.13

.. c:type:: PyHash_FuncDef

   Hash function definition used by :c:func:`PyHash_GetFuncDef`.

   .. c::member:: Py_hash_t (*const hash)(const void *, Py_ssize_t)

      Hash function.

   .. c:member:: const char *name

      Hash function name (UTF-8 encoded string).

   .. c:member:: const int hash_bits

      Internal size of the hash value in bits.

   .. c:member:: const int seed_bits

      Size of seed input in bits.

   .. versionadded:: 3.4


.. c:function:: PyHash_FuncDef* PyHash_GetFuncDef(void)

   Get the hash function definition.

   .. versionadded:: 3.4
