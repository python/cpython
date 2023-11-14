.. highlight:: c

PyHash API
----------

See also the :c:member:`PyTypeObject.tp_hash` member.

Types
^^^^^

.. c:type:: Py_hash_t

   Hash value type: signed integer.

   .. versionadded:: 3.2


.. c:type:: Py_uhash_t

   Hash value type: unsigned integer.

   .. versionadded:: 3.2


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


Functions
^^^^^^^^^

.. c:function:: Py_hash_t PyHash_Double(double value, PyObject *obj)

   Hash a C double number.

   If *value* is not-a-number (NaN):

   * If *obj* is not ``NULL``, return the hash of the *obj* pointer.
   * Otherwise, return :data:`sys.hash_info.nan <sys.hash_info>` (``0``).

   The function cannot fail: it cannot return ``-1``.

   .. versionadded:: 3.13


.. c:function:: PyHash_FuncDef* PyHash_GetFuncDef(void)

   Get the hash function definition.

   .. seealso::
      :pep:`456` "Secure and interchangeable hash algorithm".

   .. versionadded:: 3.4
