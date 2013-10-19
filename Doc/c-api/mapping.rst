.. highlightlang:: c

.. _mapping:

Mapping Protocol
================


.. c:function:: int PyMapping_Check(PyObject *o)

   Return ``1`` if the object provides mapping protocol, and ``0`` otherwise.  This
   function always succeeds.


.. c:function:: Py_ssize_t PyMapping_Size(PyObject *o)
               Py_ssize_t PyMapping_Length(PyObject *o)

   .. index:: builtin: len

   Returns the number of keys in object *o* on success, and ``-1`` on failure.  For
   objects that do not provide mapping protocol, this is equivalent to the Python
   expression ``len(o)``.


.. c:function:: int PyMapping_DelItemString(PyObject *o, const char *key)

   Remove the mapping for object *key* from the object *o*. Return ``-1`` on
   failure.  This is equivalent to the Python statement ``del o[key]``.


.. c:function:: int PyMapping_DelItem(PyObject *o, PyObject *key)

   Remove the mapping for object *key* from the object *o*. Return ``-1`` on
   failure.  This is equivalent to the Python statement ``del o[key]``.


.. c:function:: int PyMapping_HasKeyString(PyObject *o, char *key)

   On success, return ``1`` if the mapping object has the key *key* and ``0``
   otherwise.  This is equivalent to the Python expression ``key in o``.
   This function always succeeds.


.. c:function:: int PyMapping_HasKey(PyObject *o, PyObject *key)

   Return ``1`` if the mapping object has the key *key* and ``0`` otherwise.  This
   is equivalent to the Python expression ``key in o``.  This function always
   succeeds.


.. c:function:: PyObject* PyMapping_Keys(PyObject *o)

   On success, return a list of the keys in object *o*.  On failure, return *NULL*.
   This is equivalent to the Python expression ``list(o.keys())``.


.. c:function:: PyObject* PyMapping_Values(PyObject *o)

   On success, return a list of the values in object *o*.  On failure, return
   *NULL*. This is equivalent to the Python expression ``list(o.values())``.


.. c:function:: PyObject* PyMapping_Items(PyObject *o)

   On success, return a list of the items in object *o*, where each item is a tuple
   containing a key-value pair.  On failure, return *NULL*. This is equivalent to
   the Python expression ``list(o.items())``.


.. c:function:: PyObject* PyMapping_GetItemString(PyObject *o, const char *key)

   Return element of *o* corresponding to the object *key* or *NULL* on failure.
   This is the equivalent of the Python expression ``o[key]``.


.. c:function:: int PyMapping_SetItemString(PyObject *o, const char *key, PyObject *v)

   Map the object *key* to the value *v* in object *o*. Returns ``-1`` on failure.
   This is the equivalent of the Python statement ``o[key] = v``.
