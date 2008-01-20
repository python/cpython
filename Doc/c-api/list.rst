.. highlightlang:: c

.. _listobjects:

List Objects
------------

.. index:: object: list


.. ctype:: PyListObject

   This subtype of :ctype:`PyObject` represents a Python list object.


.. cvar:: PyTypeObject PyList_Type

   .. index:: single: ListType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python list type.  This is
   the same object as ``list`` and ``types.ListType`` in the Python layer.


.. cfunction:: int PyList_Check(PyObject *p)

   Return true if *p* is a list object or an instance of a subtype of the list
   type.


.. cfunction:: int PyList_CheckExact(PyObject *p)

   Return true if *p* is a list object, but not an instance of a subtype of the
   list type.


.. cfunction:: PyObject* PyList_New(Py_ssize_t len)

   Return a new list of length *len* on success, or *NULL* on failure.

   .. note::

      If *length* is greater than zero, the returned list object's items are set to
      ``NULL``.  Thus you cannot use abstract API functions such as
      :cfunc:`PySequence_SetItem`  or expose the object to Python code before setting
      all items to a real object with :cfunc:`PyList_SetItem`.


.. cfunction:: Py_ssize_t PyList_Size(PyObject *list)

   .. index:: builtin: len

   Return the length of the list object in *list*; this is equivalent to
   ``len(list)`` on a list object.


.. cfunction:: Py_ssize_t PyList_GET_SIZE(PyObject *list)

   Macro form of :cfunc:`PyList_Size` without error checking.


.. cfunction:: PyObject* PyList_GetItem(PyObject *list, Py_ssize_t index)

   Return the object at position *pos* in the list pointed to by *p*.  The position
   must be positive, indexing from the end of the list is not supported.  If *pos*
   is out of bounds, return *NULL* and set an :exc:`IndexError` exception.


.. cfunction:: PyObject* PyList_GET_ITEM(PyObject *list, Py_ssize_t i)

   Macro form of :cfunc:`PyList_GetItem` without error checking.


.. cfunction:: int PyList_SetItem(PyObject *list, Py_ssize_t index, PyObject *item)

   Set the item at index *index* in list to *item*.  Return ``0`` on success or
   ``-1`` on failure.

   .. note::

      This function "steals" a reference to *item* and discards a reference to an item
      already in the list at the affected position.


.. cfunction:: void PyList_SET_ITEM(PyObject *list, Py_ssize_t i, PyObject *o)

   Macro form of :cfunc:`PyList_SetItem` without error checking. This is normally
   only used to fill in new lists where there is no previous content.

   .. note::

      This function "steals" a reference to *item*, and, unlike
      :cfunc:`PyList_SetItem`, does *not* discard a reference to any item that it
      being replaced; any reference in *list* at position *i* will be leaked.


.. cfunction:: int PyList_Insert(PyObject *list, Py_ssize_t index, PyObject *item)

   Insert the item *item* into list *list* in front of index *index*.  Return ``0``
   if successful; return ``-1`` and set an exception if unsuccessful.  Analogous to
   ``list.insert(index, item)``.


.. cfunction:: int PyList_Append(PyObject *list, PyObject *item)

   Append the object *item* at the end of list *list*. Return ``0`` if successful;
   return ``-1`` and set an exception if unsuccessful.  Analogous to
   ``list.append(item)``.


.. cfunction:: PyObject* PyList_GetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high)

   Return a list of the objects in *list* containing the objects *between* *low*
   and *high*.  Return *NULL* and set an exception if unsuccessful. Analogous to
   ``list[low:high]``.


.. cfunction:: int PyList_SetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high, PyObject *itemlist)

   Set the slice of *list* between *low* and *high* to the contents of *itemlist*.
   Analogous to ``list[low:high] = itemlist``. The *itemlist* may be *NULL*,
   indicating the assignment of an empty list (slice deletion). Return ``0`` on
   success, ``-1`` on failure.


.. cfunction:: int PyList_Sort(PyObject *list)

   Sort the items of *list* in place.  Return ``0`` on success, ``-1`` on failure.
   This is equivalent to ``list.sort()``.


.. cfunction:: int PyList_Reverse(PyObject *list)

   Reverse the items of *list* in place.  Return ``0`` on success, ``-1`` on
   failure.  This is the equivalent of ``list.reverse()``.


.. cfunction:: PyObject* PyList_AsTuple(PyObject *list)

   .. index:: builtin: tuple

   Return a new tuple object containing the contents of *list*; equivalent to
   ``tuple(list)``.
