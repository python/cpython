.. highlight:: c

.. _mapping:

Mapping Protocol
================

See also :c:func:`PyObject_GetItem`, :c:func:`PyObject_SetItem` and
:c:func:`PyObject_DelItem`.


.. c:function:: int PyMapping_Check(PyObject *o)

   Return ``1`` if the object provides the mapping protocol or supports slicing,
   and ``0`` otherwise.  Note that it returns ``1`` for Python classes with
   a :meth:`~object.__getitem__` method, since in general it is impossible to
   determine what type of keys the class supports. This function always succeeds.


.. c:function:: Py_ssize_t PyMapping_Size(PyObject *o)
               Py_ssize_t PyMapping_Length(PyObject *o)

   .. index:: pair: built-in function; len

   Returns the number of keys in object *o* on success, and ``-1`` on failure.
   This is equivalent to the Python expression ``len(o)``.


.. c:function:: PyObject* PyMapping_GetItemString(PyObject *o, const char *key)

   This is the same as :c:func:`PyObject_GetItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.


.. c:function:: int PyMapping_GetOptionalItem(PyObject *obj, PyObject *key, PyObject **result)

   Variant of :c:func:`PyObject_GetItem` which doesn't raise
   :exc:`KeyError` if the key is not found.

   If the key is found, return ``1`` and set *\*result* to a new
   :term:`strong reference` to the corresponding value.
   If the key is not found, return ``0`` and set *\*result* to ``NULL``;
   the :exc:`KeyError` is silenced.
   If an error other than :exc:`KeyError` is raised, return ``-1`` and
   set *\*result* to ``NULL``.

   .. versionadded:: 3.13


.. c:function:: int PyMapping_GetOptionalItemString(PyObject *obj, const char *key, PyObject **result)

   This is the same as :c:func:`PyMapping_GetOptionalItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.

   .. versionadded:: 3.13


.. c:function:: int PyMapping_SetItemString(PyObject *o, const char *key, PyObject *v)

   This is the same as :c:func:`PyObject_SetItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.


.. c:function:: int PyMapping_DelItem(PyObject *o, PyObject *key)

   This is an alias of :c:func:`PyObject_DelItem`.


.. c:function:: int PyMapping_DelItemString(PyObject *o, const char *key)

   This is the same as :c:func:`PyObject_DelItem`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.


.. c:function:: int PyMapping_HasKeyWithError(PyObject *o, PyObject *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.
   This is equivalent to the Python expression ``key in o``.
   On failure, return ``-1``.

   .. versionadded:: 3.13


.. c:function:: int PyMapping_HasKeyStringWithError(PyObject *o, const char *key)

   This is the same as :c:func:`PyMapping_HasKeyWithError`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.

   .. versionadded:: 3.13


.. c:function:: int PyMapping_HasKey(PyObject *o, PyObject *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.
   This is equivalent to the Python expression ``key in o``.
   This function always succeeds.

   .. note::

      Exceptions which occur when this calls :meth:`~object.__getitem__`
      method are silently ignored.
      For proper error handling, use :c:func:`PyMapping_HasKeyWithError`,
      :c:func:`PyMapping_GetOptionalItem` or :c:func:`PyObject_GetItem()` instead.


.. c:function:: int PyMapping_HasKeyString(PyObject *o, const char *key)

   This is the same as :c:func:`PyMapping_HasKey`, but *key* is
   specified as a :c:expr:`const char*` UTF-8 encoded bytes string,
   rather than a :c:expr:`PyObject*`.

   .. note::

      Exceptions that occur when this calls :meth:`~object.__getitem__`
      method or while creating the temporary :class:`str`
      object are silently ignored.
      For proper error handling, use :c:func:`PyMapping_HasKeyStringWithError`,
      :c:func:`PyMapping_GetOptionalItemString` or
      :c:func:`PyMapping_GetItemString` instead.


.. c:function:: PyObject* PyMapping_Keys(PyObject *o)

   On success, return a list of the keys in object *o*.  On failure, return
   ``NULL``.

   .. versionchanged:: 3.7
      Previously, the function returned a list or a tuple.


.. c:function:: PyObject* PyMapping_Values(PyObject *o)

   On success, return a list of the values in object *o*.  On failure, return
   ``NULL``.

   .. versionchanged:: 3.7
      Previously, the function returned a list or a tuple.


.. c:function:: PyObject* PyMapping_Items(PyObject *o)

   On success, return a list of the items in object *o*, where each item is a
   tuple containing a key-value pair.  On failure, return ``NULL``.

   .. versionchanged:: 3.7
      Previously, the function returned a list or a tuple.
