.. highlight:: c

.. _listobjects:

List Objects
------------

.. index:: pair: object; list


.. c:type:: PyListObject

   This subtype of :c:type:`PyObject` represents a Python list object.


.. c:var:: PyTypeObject PyList_Type

   This instance of :c:type:`PyTypeObject` represents the Python list type.
   This is the same object as :class:`list` in the Python layer.


.. c:function:: int PyList_Check(PyObject *p)

   Return true if *p* is a list object or an instance of a subtype of the list
   type.  This function always succeeds.


.. c:function:: int PyList_CheckExact(PyObject *p)

   Return true if *p* is a list object, but not an instance of a subtype of
   the list type.  This function always succeeds.


.. c:function:: PyObject* PyList_New(Py_ssize_t len)

   Return a new list of length *len* on success, or ``NULL`` on failure.

   .. note::

      If *len* is greater than zero, the returned list object's items are
      set to ``NULL``. Thus you cannot use abstract API functions such as
      :c:func:`PySequence_SetItem` or expose the object to Python code before
      setting all items to a real object with :c:func:`PyList_SetItem` or
      :c:func:`PyList_SET_ITEM()`. The following APIs are safe APIs before
      the list is fully initialized: :c:func:`PyList_SetItem()` and :c:func:`PyList_SET_ITEM()`.



.. c:function:: Py_ssize_t PyList_Size(PyObject *list)

   .. index:: pair: built-in function; len

   Return the length of the list object in *list*; this is equivalent to
   ``len(list)`` on a list object.


.. c:function:: Py_ssize_t PyList_GET_SIZE(PyObject *list)

   Similar to :c:func:`PyList_Size`, but without error checking.


.. c:function:: PyObject* PyList_GetItemRef(PyObject *list, Py_ssize_t index)

   Return the object at position *index* in the list pointed to by *list*.  The
   position must be non-negative; indexing from the end of the list is not
   supported.  If *index* is out of bounds (:code:`<0 or >=len(list)`),
   return ``NULL`` and set an :exc:`IndexError` exception.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyList_GetItem(PyObject *list, Py_ssize_t index)

   Like :c:func:`PyList_GetItemRef`, but returns a
   :term:`borrowed reference` instead of a :term:`strong reference`.


.. c:function:: PyObject* PyList_GET_ITEM(PyObject *list, Py_ssize_t i)

   Similar to :c:func:`PyList_GetItem`, but without error checking.


.. c:function:: int PyList_SetItem(PyObject *list, Py_ssize_t index, PyObject *item)

   Set the item at index *index* in list to *item*.  Return ``0`` on success.
   If *index* is out of bounds, return ``-1`` and set an :exc:`IndexError`
   exception.

   .. note::

      This function "steals" a reference to *item* and discards a reference to
      an item already in the list at the affected position.


.. c:function:: void PyList_SET_ITEM(PyObject *list, Py_ssize_t i, PyObject *o)

   Macro form of :c:func:`PyList_SetItem` without error checking. This is
   normally only used to fill in new lists where there is no previous content.

   Bounds checking is performed as an assertion if Python is built in
   :ref:`debug mode <debug-build>` or :option:`with assertions
   <--with-assertions>`.

   .. note::

      This macro "steals" a reference to *item*, and, unlike
      :c:func:`PyList_SetItem`, does *not* discard a reference to any item that
      is being replaced; any reference in *list* at position *i* will be
      leaked.


.. c:function:: int PyList_Insert(PyObject *list, Py_ssize_t index, PyObject *item)

   Insert the item *item* into list *list* in front of index *index*.  Return
   ``0`` if successful; return ``-1`` and set an exception if unsuccessful.
   Analogous to ``list.insert(index, item)``.


.. c:function:: int PyList_Append(PyObject *list, PyObject *item)

   Append the object *item* at the end of list *list*. Return ``0`` if
   successful; return ``-1`` and set an exception if unsuccessful.  Analogous
   to ``list.append(item)``.


.. c:function:: PyObject* PyList_GetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high)

   Return a list of the objects in *list* containing the objects *between* *low*
   and *high*.  Return ``NULL`` and set an exception if unsuccessful.  Analogous
   to ``list[low:high]``.  Indexing from the end of the list is not supported.


.. c:function:: int PyList_SetSlice(PyObject *list, Py_ssize_t low, Py_ssize_t high, PyObject *itemlist)

   Set the slice of *list* between *low* and *high* to the contents of
   *itemlist*.  Analogous to ``list[low:high] = itemlist``. The *itemlist* may
   be ``NULL``, indicating the assignment of an empty list (slice deletion).
   Return ``0`` on success, ``-1`` on failure.  Indexing from the end of the
   list is not supported.


.. c:function:: int PyList_Extend(PyObject *list, PyObject *iterable)

   Extend *list* with the contents of *iterable*.  This is the same as
   ``PyList_SetSlice(list, PY_SSIZE_T_MAX, PY_SSIZE_T_MAX, iterable)``
   and analogous to ``list.extend(iterable)`` or ``list += iterable``.

   Raise an exception and return ``-1`` if *list* is not a :class:`list`
   object. Return 0 on success.

   .. versionadded:: 3.13


.. c:function:: int PyList_Clear(PyObject *list)

   Remove all items from *list*.  This is the same as
   ``PyList_SetSlice(list, 0, PY_SSIZE_T_MAX, NULL)`` and analogous to
   ``list.clear()`` or ``del list[:]``.

   Raise an exception and return ``-1`` if *list* is not a :class:`list`
   object.  Return 0 on success.

   .. versionadded:: 3.13


.. c:function:: int PyList_Sort(PyObject *list)

   Sort the items of *list* in place.  Return ``0`` on success, ``-1`` on
   failure.  This is equivalent to ``list.sort()``.


.. c:function:: int PyList_Reverse(PyObject *list)

   Reverse the items of *list* in place.  Return ``0`` on success, ``-1`` on
   failure.  This is the equivalent of ``list.reverse()``.


.. c:function:: PyObject* PyList_AsTuple(PyObject *list)

   .. index:: pair: built-in function; tuple

   Return a new tuple object containing the contents of *list*; equivalent to
   ``tuple(list)``.
