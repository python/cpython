.. highlight:: c

PyHash API
----------

See also the :c:member:`PyTypeObject.tp_hash` member and :ref:`numeric-hash`.

.. c:type:: Py_hash_t

   Hash value type: signed integer.

   .. versionadded:: 3.2

.. c:type:: Py_uhash_t

   Hash value type: unsigned integer.

   .. versionadded:: 3.2

.. c:macro:: PyHASH_MODULUS

   The `Mersenne prime <https://en.wikipedia.org/wiki/Mersenne_prime>`_ ``P = 2**n -1``, used for numeric hash scheme.

   .. versionadded:: 3.13

.. c:macro:: PyHASH_BITS

   The exponent ``n`` of ``P`` in :c:macro:`PyHASH_MODULUS`.

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

   .. seealso::
      :pep:`456` "Secure and interchangeable hash algorithm".

   .. versionadded:: 3.4


.. c:function:: Py_hash_t Py_HashPointer(const void *ptr)

   Hash a pointer value: process the pointer value as an integer (cast it to
   ``uintptr_t`` internally). The pointer is not dereferenced.

   The function cannot fail: it cannot return ``-1``.

   .. versionadded:: 3.13
