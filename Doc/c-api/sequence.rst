.. highlight:: c

.. _sequence:

Sequence Protocol
=================


.. c:function:: int PySequence_Check(PyObject *o)

   Return ``1`` if the object provides sequence protocol, and ``0`` otherwise.
   Note that it returns ``1`` for Python classes with a :meth:`__getitem__`
   method unless they are :class:`dict` subclasses since in general case it
   is impossible to determine what the type of keys it supports.  This
   function always succeeds.


.. c:function:: Py_ssize_t PySequence_Size(PyObject *o)
               Py_ssize_t PySequence_Length(PyObject *o)

   .. index:: builtin: len

   Returns the number of objects in sequence *o* on success, and ``-1`` on
   failure.  This is equivalent to the Python expression ``len(o)``.


.. c:function:: PyObject* PySequence_Concat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and ``NULL`` on failure.
   This is the equivalent of the Python expression ``o1 + o2``.


.. c:function:: PyObject* PySequence_Repeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or ``NULL`` on
   failure.  This is the equivalent of the Python expression ``o * count``.


.. c:function:: PyObject* PySequence_InPlaceConcat(PyObject *o1, PyObject *o2)

   Return the concatenation of *o1* and *o2* on success, and ``NULL`` on failure.
   The operation is done *in-place* when *o1* supports it.  This is the equivalent
   of the Python expression ``o1 += o2``.


.. c:function:: PyObject* PySequence_InPlaceRepeat(PyObject *o, Py_ssize_t count)

   Return the result of repeating sequence object *o* *count* times, or ``NULL`` on
   failure.  The operation is done *in-place* when *o* supports it.  This is the
   equivalent of the Python expression ``o *= count``.


.. c:function:: PyObject* PySequence_GetItem(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o*, or ``NULL`` on failure. This is the equivalent of
   the Python expression ``o[i]``.


.. c:function:: PyObject* PySequence_GetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Return the slice of sequence object *o* between *i1* and *i2*, or ``NULL`` on
   failure. This is the equivalent of the Python expression ``o[i1:i2]``.


.. c:function:: int PySequence_SetItem(PyObject *o, Py_ssize_t i, PyObject *v)

   Assign object *v* to the *i*\ th element of *o*.  Raise an exception
   and return ``-1`` on failure; return ``0`` on success.  This
   is the equivalent of the Python statement ``o[i] = v``.  This function *does
   not* steal a reference to *v*.

   If *v* is ``NULL``, the element is deleted, however this feature is
   deprecated in favour of using :c:func:`PySequence_DelItem`.


.. c:function:: int PySequence_DelItem(PyObject *o, Py_ssize_t i)

   Delete the *i*\ th element of object *o*.  Returns ``-1`` on failure.  This is the
   equivalent of the Python statement ``del o[i]``.


.. c:function:: int PySequence_SetSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2, PyObject *v)

   Assign the sequence object *v* to the slice in sequence object *o* from *i1* to
   *i2*.  This is the equivalent of the Python statement ``o[i1:i2] = v``.


.. c:function:: int PySequence_DelSlice(PyObject *o, Py_ssize_t i1, Py_ssize_t i2)

   Delete the slice in sequence object *o* from *i1* to *i2*.  Returns ``-1`` on
   failure.  This is the equivalent of the Python statement ``del o[i1:i2]``.


.. c:function:: Py_ssize_t PySequence_Count(PyObject *o, PyObject *value)

   Return the number of occurrences of *value* in *o*, that is, return the number
   of keys for which ``o[key] == value``.  On failure, return ``-1``.  This is
   equivalent to the Python expression ``o.count(value)``.


.. c:function:: int PySequence_Contains(PyObject *o, PyObject *value)

   Determine if *o* contains *value*.  If an item in *o* is equal to *value*,
   return ``1``, otherwise return ``0``. On error, return ``-1``.  This is
   equivalent to the Python expression ``value in o``.


.. c:function:: Py_ssize_t PySequence_Index(PyObject *o, PyObject *value)

   Return the first index *i* for which ``o[i] == value``.  On error, return
   ``-1``.    This is equivalent to the Python expression ``o.index(value)``.


.. c:function:: PyObject* PySequence_List(PyObject *o)

   Return a list object with the same contents as the sequence or iterable *o*,
   or ``NULL`` on failure.  The returned list is guaranteed to be new.  This is
   equivalent to the Python expression ``list(o)``.


.. c:function:: PyObject* PySequence_Tuple(PyObject *o)

   .. index:: builtin: tuple

   Return a tuple object with the same contents as the sequence or iterable *o*,
   or ``NULL`` on failure.  If *o* is a tuple, a new reference will be returned,
   otherwise a tuple will be constructed with the appropriate contents.  This is
   equivalent to the Python expression ``tuple(o)``.


.. c:function:: PyObject* PySequence_Fast(PyObject *o, const char *m)

   Return the sequence or iterable *o* as an object usable by the other
   ``PySequence_Fast*`` family of functions. If the object is not a sequence or
   iterable, raises :exc:`TypeError` with *m* as the message text. Returns
   ``NULL`` on failure.

   The ``PySequence_Fast*`` functions are thus named because they assume
   *o* is a :c:type:`PyTupleObject` or a :c:type:`PyListObject` and access
   the data fields of *o* directly.

   As a CPython implementation detail, if *o* is already a sequence or list, it
   will be returned.


.. c:function:: Py_ssize_t PySequence_Fast_GET_SIZE(PyObject *o)

   Returns the length of *o*, assuming that *o* was returned by
   :c:func:`PySequence_Fast` and that *o* is not ``NULL``.  The size can also be
   gotten by calling :c:func:`PySequence_Size` on *o*, but
   :c:func:`PySequence_Fast_GET_SIZE` is faster because it can assume *o* is a
   list or tuple.


.. c:function:: PyObject* PySequence_Fast_GET_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o*, assuming that *o* was returned by
   :c:func:`PySequence_Fast`, *o* is not ``NULL``, and that *i* is within bounds.


.. c:function:: PyObject** PySequence_Fast_ITEMS(PyObject *o)

   Return the underlying array of PyObject pointers.  Assumes that *o* was returned
   by :c:func:`PySequence_Fast` and *o* is not ``NULL``.

   Note, if a list gets resized, the reallocation may relocate the items array.
   So, only use the underlying array pointer in contexts where the sequence
   cannot change.


.. c:function:: PyObject* PySequence_ITEM(PyObject *o, Py_ssize_t i)

   Return the *i*\ th element of *o* or ``NULL`` on failure. Faster form of
   :c:func:`PySequence_GetItem` but without checking that
   :c:func:`PySequence_Check` on *o* is true and without adjustment for negative
   indices.
