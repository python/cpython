.. highlightlang:: c

.. _sequence:

Sequence Protocol
=================


.. c:function:: int PySequence_Check(PyObject *o)

   Return ``1`` if the object provides sequence protocol, and ``0`` otherwise.
   This function always succeeds.


.. c:function:: Py_ssize_t PySequence_Size(PyObject *o)
               Py_ssize_t PySequence_Length(PyObject *o)

   .. index:: builtin: len

   Returns the number of objects in sequence *o* on success, and ``-1`` on failure.
   For objects that do not provide sequence protocol, this is equivalent to the
   Python expression ``len(o)``.

   .. versionchanged:: 2.5
      These functions returned an :c:type:`int` type. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PySequence_Concat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and *NULL* on failure.
   This is the equivalent of the Python expression ``o1 + o2``.


.. c:function:: PyObject* PySequence_Repeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or *NULL* on
   failure.  This is the equivalent of the Python expression ``o * count``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *count*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PySequence_InPlaceConcat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and *NULL* on failure.
   The operation is done *in-place* when *o1* supports it.  This is the equivalent
   of the Python expression ``o1 += o2``.


.. c:function:: PyObject* PySequence_InPlaceRepeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or *NULL* on
   failure.  The operation is done *in-place* when *o* supports it.  This is the
   equivalent of the Python expression ``o *= count``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *count*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PySequence_GetItem(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o*, or *NULL* on failure. This is the equivalent of
   the Python expression ``o[i]``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PySequence_GetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Return the slice of sequence object *o* between *i1* and *i2*, or *NULL* on
   failure. This is the equivalent of the Python expression ``o[i1:i2]``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i1* and *i2*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: int PySequence_SetItem(PyObject *o, Py_ssize_t i, PyObject *v)

   Assign object *v* to the *i*\ th element of *o*.  Returns ``-1`` on failure.  This
   is the equivalent of the Python statement ``o[i] = v``.  This function *does
   not* steal a reference to *v*.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: int PySequence_DelItem(PyObject *o, Py_ssize_t i)

   Delete the *i*\ th element of object *o*.  Returns ``-1`` on failure.  This is the
   equivalent of the Python statement ``del o[i]``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: int PySequence_SetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2, PyObject *v)

   Assign the sequence object *v* to the slice in sequence object *o* from *i1* to
   *i2*.  This is the equivalent of the Python statement ``o[i1:i2] = v``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i1* and *i2*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: int PySequence_DelSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Delete the slice in sequence object *o* from *i1* to *i2*.  Returns ``-1`` on
   failure.  This is the equivalent of the Python statement ``del o[i1:i2]``.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i1* and *i2*. This might
      require changes in your code for properly supporting 64-bit systems.


.. c:function:: Py_ssize_t PySequence_Count(PyObject *o, PyObject *value)

   Return the number of occurrences of *value* in *o*, that is, return the number
   of keys for which ``o[key] == value``.  On failure, return ``-1``.  This is
   equivalent to the Python expression ``o.count(value)``.

   .. versionchanged:: 2.5
      This function returned an :c:type:`int` type. This might require changes
      in your code for properly supporting 64-bit systems.


.. c:function:: int PySequence_Contains(PyObject *o, PyObject *value)

   Determine if *o* contains *value*.  If an item in *o* is equal to *value*,
   return ``1``, otherwise return ``0``. On error, return ``-1``.  This is
   equivalent to the Python expression ``value in o``.


.. c:function:: Py_ssize_t PySequence_Index(PyObject *o, PyObject *value)

   Return the first index *i* for which ``o[i] == value``.  On error, return
   ``-1``.    This is equivalent to the Python expression ``o.index(value)``.

   .. versionchanged:: 2.5
      This function returned an :c:type:`int` type. This might require changes
      in your code for properly supporting 64-bit systems.


.. c:function:: PyObject* PySequence_List(PyObject *o)

   Return a list object with the same contents as the arbitrary sequence *o*.  The
   returned list is guaranteed to be new.


.. c:function:: PyObject* PySequence_Tuple(PyObject *o)

   .. index:: builtin: tuple

   Return a tuple object with the same contents as the arbitrary sequence *o* or
   *NULL* on failure.  If *o* is a tuple, a new reference will be returned,
   otherwise a tuple will be constructed with the appropriate contents.  This is
   equivalent to the Python expression ``tuple(o)``.


.. c:function:: PyObject* PySequence_Fast(PyObject *o, const char *m)

   Returns the sequence *o* as a tuple, unless it is already a tuple or list, in
   which case *o* is returned.  Use :c:func:`PySequence_Fast_GET_ITEM` to access the
   members of the result.  Returns *NULL* on failure.  If the object is not a
   sequence, raises :exc:`TypeError` with *m* as the message text.


.. c:function:: PyObject* PySequence_Fast_GET_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o*, assuming that *o* was returned by
   :c:func:`PySequence_Fast`, *o* is not *NULL*, and that *i* is within bounds.

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: PyObject** PySequence_Fast_ITEMS(PyObject *o)

   Return the underlying array of PyObject pointers.  Assumes that *o* was returned
   by :c:func:`PySequence_Fast` and *o* is not *NULL*.

   Note, if a list gets resized, the reallocation may relocate the items array.
   So, only use the underlying array pointer in contexts where the sequence
   cannot change.

   .. versionadded:: 2.4


.. c:function:: PyObject* PySequence_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o* or *NULL* on failure. Macro form of
   :c:func:`PySequence_GetItem` but without checking that
   :c:func:`PySequence_Check` on *o* is true and without adjustment for negative
   indices.

   .. versionadded:: 2.3

   .. versionchanged:: 2.5
      This function used an :c:type:`int` type for *i*. This might require
      changes in your code for properly supporting 64-bit systems.


.. c:function:: Py_ssize_t PySequence_Fast_GET_SIZE(PyObject *o)

   Returns the length of *o*, assuming that *o* was returned by
   :c:func:`PySequence_Fast` and that *o* is not *NULL*.  The size can also be
   gotten by calling :c:func:`PySequence_Size` on *o*, but
   :c:func:`PySequence_Fast_GET_SIZE` is faster because it can assume *o* is a list
   or tuple.
