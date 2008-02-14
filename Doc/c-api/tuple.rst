.. highlightlang:: c

.. _tupleobjects:

Tuple Objects
-------------

.. index:: object: tuple


.. ctype:: PyTupleObject

   This subtype of :ctype:`PyObject` represents a Python tuple object.


.. cvar:: PyTypeObject PyTuple_Type

   .. index:: single: TupleType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python tuple type; it is
   the same object as ``tuple`` and ``types.TupleType`` in the Python layer..


.. cfunction:: int PyTuple_Check(PyObject *p)

   Return true if *p* is a tuple object or an instance of a subtype of the tuple
   type.

   .. versionchanged:: 2.2
      Allowed subtypes to be accepted.


.. cfunction:: int PyTuple_CheckExact(PyObject *p)

   Return true if *p* is a tuple object, but not an instance of a subtype of the
   tuple type.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyTuple_New(Py_ssize_t len)

   Return a new tuple object of size *len*, or *NULL* on failure.


.. cfunction:: PyObject* PyTuple_Pack(Py_ssize_t n, ...)

   Return a new tuple object of size *n*, or *NULL* on failure. The tuple values
   are initialized to the subsequent *n* C arguments pointing to Python objects.
   ``PyTuple_Pack(2, a, b)`` is equivalent to ``Py_BuildValue("(OO)", a, b)``.

   .. versionadded:: 2.4


.. cfunction:: Py_ssize_t PyTuple_Size(PyObject *p)

   Take a pointer to a tuple object, and return the size of that tuple.


.. cfunction:: Py_ssize_t PyTuple_GET_SIZE(PyObject *p)

   Return the size of the tuple *p*, which must be non-*NULL* and point to a tuple;
   no error checking is performed.


.. cfunction:: PyObject* PyTuple_GetItem(PyObject *p, Py_ssize_t pos)

   Return the object at position *pos* in the tuple pointed to by *p*.  If *pos* is
   out of bounds, return *NULL* and sets an :exc:`IndexError` exception.


.. cfunction:: PyObject* PyTuple_GET_ITEM(PyObject *p, Py_ssize_t pos)

   Like :cfunc:`PyTuple_GetItem`, but does no checking of its arguments.


.. cfunction:: PyObject* PyTuple_GetSlice(PyObject *p, Py_ssize_t low, Py_ssize_t high)

   Take a slice of the tuple pointed to by *p* from *low* to *high* and return it
   as a new tuple.


.. cfunction:: int PyTuple_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)

   Insert a reference to object *o* at position *pos* of the tuple pointed to by
   *p*. Return ``0`` on success.

   .. note::

      This function "steals" a reference to *o*.


.. cfunction:: void PyTuple_SET_ITEM(PyObject *p, Py_ssize_t pos, PyObject *o)

   Like :cfunc:`PyTuple_SetItem`, but does no error checking, and should *only* be
   used to fill in brand new tuples.

   .. note::

      This function "steals" a reference to *o*.


.. cfunction:: int _PyTuple_Resize(PyObject **p, Py_ssize_t newsize)

   Can be used to resize a tuple.  *newsize* will be the new length of the tuple.
   Because tuples are *supposed* to be immutable, this should only be used if there
   is only one reference to the object.  Do *not* use this if the tuple may already
   be known to some other part of the code.  The tuple will always grow or shrink
   at the end.  Think of this as destroying the old tuple and creating a new one,
   only more efficiently.  Returns ``0`` on success. Client code should never
   assume that the resulting value of ``*p`` will be the same as before calling
   this function. If the object referenced by ``*p`` is replaced, the original
   ``*p`` is destroyed.  On failure, returns ``-1`` and sets ``*p`` to *NULL*, and
   raises :exc:`MemoryError` or :exc:`SystemError`.

   .. versionchanged:: 2.2
      Removed unused third parameter, *last_is_sticky*.


.. cfunction:: int PyMethod_ClearFreeList(void)

   Clear the free list. Return the total number of freed items.

   .. versionadded:: 2.6
