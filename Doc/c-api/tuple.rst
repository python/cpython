.. highlight:: c

.. _tupleobjects:

Tuple Objects
-------------

.. index:: pair: object; tuple


.. c:type:: PyTupleObject

   This subtype of :c:type:`PyObject` represents a Python tuple object.


.. c:var:: PyTypeObject PyTuple_Type

   This instance of :c:type:`PyTypeObject` represents the Python tuple type; it
   is the same object as :class:`tuple` in the Python layer.


.. c:function:: int PyTuple_Check(PyObject *p)

   Return true if *p* is a tuple object or an instance of a subtype of the
   tuple type.  This function always succeeds.


.. c:function:: int PyTuple_CheckExact(PyObject *p)

   Return true if *p* is a tuple object, but not an instance of a subtype of the
   tuple type.  This function always succeeds.


.. c:function:: PyObject* PyTuple_New(Py_ssize_t len)

   Return a new tuple object of size *len*, or ``NULL`` on failure.


.. c:function:: PyObject* PyTuple_Pack(Py_ssize_t n, ...)

   Return a new tuple object of size *n*, or ``NULL`` on failure. The tuple values
   are initialized to the subsequent *n* C arguments pointing to Python objects.
   ``PyTuple_Pack(2, a, b)`` is equivalent to ``Py_BuildValue("(OO)", a, b)``.


.. c:function:: Py_ssize_t PyTuple_Size(PyObject *p)

   Take a pointer to a tuple object, and return the size of that tuple.


.. c:function:: Py_ssize_t PyTuple_GET_SIZE(PyObject *p)

   Return the size of the tuple *p*, which must be non-``NULL`` and point to a tuple;
   no error checking is performed.


.. c:function:: PyObject* PyTuple_GetItem(PyObject *p, Py_ssize_t pos)

   Return the object at position *pos* in the tuple pointed to by *p*.  If *pos* is
   negative or out of bounds, return ``NULL`` and set an :exc:`IndexError` exception.


.. c:function:: PyObject* PyTuple_GET_ITEM(PyObject *p, Py_ssize_t pos)

   Like :c:func:`PyTuple_GetItem`, but does no checking of its arguments.


.. c:function:: PyObject* PyTuple_GetSlice(PyObject *p, Py_ssize_t low, Py_ssize_t high)

   Return the slice of the tuple pointed to by *p* between *low* and *high*,
   or ``NULL`` on failure.  This is the equivalent of the Python expression
   ``p[low:high]``.  Indexing from the end of the tuple is not supported.


.. c:function:: int PyTuple_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)

   Insert a reference to object *o* at position *pos* of the tuple pointed to by
   *p*.  Return ``0`` on success.  If *pos* is out of bounds, return ``-1``
   and set an :exc:`IndexError` exception.

   .. note::

      This function "steals" a reference to *o* and discards a reference to
      an item already in the tuple at the affected position.


.. c:function:: void PyTuple_SET_ITEM(PyObject *p, Py_ssize_t pos, PyObject *o)

   Like :c:func:`PyTuple_SetItem`, but does no error checking, and should *only* be
   used to fill in brand new tuples.

   Bounds checking is performed as an assertion if Python is built in
   :ref:`debug mode <debug-build>` or :option:`with assertions <--with-assertions>`.

   .. note::

      This function "steals" a reference to *o*, and, unlike
      :c:func:`PyTuple_SetItem`, does *not* discard a reference to any item that
      is being replaced; any reference in the tuple at position *pos* will be
      leaked.


.. c:function:: int _PyTuple_Resize(PyObject **p, Py_ssize_t newsize)

   Can be used to resize a tuple.  *newsize* will be the new length of the tuple.
   Because tuples are *supposed* to be immutable, this should only be used if there
   is only one reference to the object.  Do *not* use this if the tuple may already
   be known to some other part of the code.  The tuple will always grow or shrink
   at the end.  Think of this as destroying the old tuple and creating a new one,
   only more efficiently.  Returns ``0`` on success. Client code should never
   assume that the resulting value of ``*p`` will be the same as before calling
   this function. If the object referenced by ``*p`` is replaced, the original
   ``*p`` is destroyed.  On failure, returns ``-1`` and sets ``*p`` to ``NULL``, and
   raises :exc:`MemoryError` or :exc:`SystemError`.


.. _struct-sequence-objects:

Struct Sequence Objects
-----------------------

Struct sequence objects are the C equivalent of :func:`~collections.namedtuple`
objects, i.e. a sequence whose items can also be accessed through attributes.
To create a struct sequence, you first have to create a specific struct sequence
type.

.. c:function:: PyTypeObject* PyStructSequence_NewType(PyStructSequence_Desc *desc)

   Create a new struct sequence type from the data in *desc*, described below. Instances
   of the resulting type can be created with :c:func:`PyStructSequence_New`.


.. c:function:: void PyStructSequence_InitType(PyTypeObject *type, PyStructSequence_Desc *desc)

   Initializes a struct sequence type *type* from *desc* in place.


.. c:function:: int PyStructSequence_InitType2(PyTypeObject *type, PyStructSequence_Desc *desc)

   The same as ``PyStructSequence_InitType``, but returns ``0`` on success and ``-1`` on
   failure.

   .. versionadded:: 3.4


.. c:type:: PyStructSequence_Desc

   Contains the meta information of a struct sequence type to create.

   .. c:member:: const char *name

      Name of the struct sequence type.

   .. c:member:: const char *doc

      Pointer to docstring for the type or ``NULL`` to omit.

   .. c:member:: PyStructSequence_Field *fields

      Pointer to ``NULL``-terminated array with field names of the new type.

   .. c:member:: int n_in_sequence

      Number of fields visible to the Python side (if used as tuple).


.. c:type:: PyStructSequence_Field

   Describes a field of a struct sequence. As a struct sequence is modeled as a
   tuple, all fields are typed as :c:expr:`PyObject*`.  The index in the
   :c:member:`~PyStructSequence_Desc.fields` array of
   the :c:type:`PyStructSequence_Desc` determines which
   field of the struct sequence is described.

   .. c:member:: const char *name

      Name for the field or ``NULL`` to end the list of named fields,
      set to :c:data:`PyStructSequence_UnnamedField` to leave unnamed.

   .. c:member:: const char *doc

      Field docstring or ``NULL`` to omit.


.. c:var:: const char * const PyStructSequence_UnnamedField

   Special value for a field name to leave it unnamed.

   .. versionchanged:: 3.9
      The type was changed from ``char *``.


.. c:function:: PyObject* PyStructSequence_New(PyTypeObject *type)

   Creates an instance of *type*, which must have been created with
   :c:func:`PyStructSequence_NewType`.


.. c:function:: PyObject* PyStructSequence_GetItem(PyObject *p, Py_ssize_t pos)

   Return the object at position *pos* in the struct sequence pointed to by *p*.

   Bounds checking is performed as an assertion if Python is built in
   :ref:`debug mode <debug-build>` or :option:`with assertions <--with-assertions>`.


.. c:function:: PyObject* PyStructSequence_GET_ITEM(PyObject *p, Py_ssize_t pos)

   Alias to :c:func:`PyStructSequence_GetItem`.

   .. versionchanged:: 3.13
      Now implemented as an alias to :c:func:`PyStructSequence_GetItem`.


.. c:function:: void PyStructSequence_SetItem(PyObject *p, Py_ssize_t pos, PyObject *o)

   Sets the field at index *pos* of the struct sequence *p* to value *o*.  Like
   :c:func:`PyTuple_SET_ITEM`, this should only be used to fill in brand new
   instances.

   Bounds checking is performed as an assertion if Python is built in
   :ref:`debug mode <debug-build>` or :option:`with assertions <--with-assertions>`.

   .. note::

      This function "steals" a reference to *o*.


.. c:function:: void PyStructSequence_SET_ITEM(PyObject *p, Py_ssize_t *pos, PyObject *o)

   Alias to :c:func:`PyStructSequence_SetItem`.

   .. versionchanged:: 3.13
      Now implemented as an alias to :c:func:`PyStructSequence_SetItem`.
