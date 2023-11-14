.. highlight:: c

PyHash API
----------

See also the :c:member:`PyTypeObject.tp_hash` member.

.. c:type:: Py_hash_t

   Hash value type: signed integer.

   .. versionadded:: 3.2

.. c:type:: Py_uhash_t

   Hash value type: unsigned integer.

   .. versionadded:: 3.2


.. c:type:: PyHash_FuncDef

   Hash function definition used by :c:func:`PyHash_GetFuncDef`.

   .. c::member:: Py_hash_t (*const hash)(const void *, Py_ssize_t)

   .. c:member:: const char *name

      Hash function (UTF-8 encoded name).

   .. c:member:: const int hash_bits

      Internal size of hash value.

   .. c:member:: const int seed_bits

      Size of seed input.

   .. versionadded:: 3.4


.. c:function:: PyHash_FuncDef* PyHash_GetFuncDef(void)

   Get the hash function definition.

   .. versionadded:: 3.4
