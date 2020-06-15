.. highlight:: c

.. _mapping:

Mapping Protocol
================

See also :c:func:`PyObject_GetItem`, :c:func:`PyObject_SetItem` and
:c:func:`PyObject_DelItem`.


.. c:function:: int PyMapping_Check(PyObject *o)

   Return ``1`` if the object provides mapping protocol or supports slicing,
   and ``0`` otherwise.  Note that it returns ``1`` for Python classes with
   a :meth:`__getitem__` method since in general case it is impossible to
   determine what type of keys it supports. This function always succeeds.


.. c:function:: Py_ssize_t PyMapping_Size(PyObject *o)
               Py_ssize_t PyMapping_Length(PyObject *o)

   .. index:: builtin: len

   Returns the number of keys in object *o* on success, and ``-1`` on failure.
   This is equivalent to the Python expression ``len(o)``.


.. c:function:: PyObject* PyMapping_GetItemString(PyObject *o, const char *key)

   Return element of *o* corresponding to the string *key* or ``NULL`` on failure.
   This is the equivalent of the Python expression ``o[key]``.
   See also :c:func:`PyObject_GetItem`.


.. c:function:: int PyMapping_SetItemString(PyObject *o, const char *key, PyObject *v)

   Map the string *key* to the value *v* in object *o*.  Returns ``-1`` on
   failure.  This is the equivalent of the Python statement ``o[key] = v``.
   See also :c:func:`PyObject_SetItem`.  This function *does not* steal a
   reference to *v*.


.. c:function:: int PyMapping_DelItem(PyObject *o, PyObject *key)

   Remove the mapping for the object *key* from the object *o*.  Return ``-1``
   on failure.  This is equivalent to the Python statement ``del o[key]``.
   This is an alias of :c:func:`PyObject_DelItem`.


.. c:function:: int PyMapping_DelItemString(PyObject *o, const char *key)

   Remove the mapping for the string *key* from the object *o*.  Return ``-1``
   on failure.  This is equivalent to the Python statement ``del o[key]``.


.. c:function:: int PyMapping_HasKey(PyObject *o, PyObject *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.
   This is equivalent to the Python expression ``key in o``.
   This function always succeeds.

   Note that exceptions which occur while calling the :meth:`__getitem__`
   method will get suppressed.
   To get error reporting use :c:func:`PyObject_GetItem()` instead.


.. c:function:: int PyMapping_HasKeyString(PyObject *o, const char *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.
   This is equivalent to the Python expression ``key in o``.
   This function always succeeds.

   Note that exceptions which occur while calling the :meth:`__getitem__`
   method and creating a temporary string object will get suppressed.
   To get error reporting use :c:func:`PyMapping_GetItemString()` instead.


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
