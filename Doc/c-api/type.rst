.. highlightlang:: c

.. _typeobjects:

Type Objects
------------

.. index:: object: type


.. c:type:: PyTypeObject

   The C structure of the objects used to describe built-in types.


.. c:var:: PyObject* PyType_Type

   This is the type object for type objects; it is the same object as
   :class:`type` in the Python layer.


.. c:function:: int PyType_Check(PyObject *o)

   Return true if the object *o* is a type object, including instances of types
   derived from the standard type object.  Return false in all other cases.


.. c:function:: int PyType_CheckExact(PyObject *o)

   Return true if the object *o* is a type object, but not a subtype of the
   standard type object.  Return false in all other cases.


.. c:function:: unsigned int PyType_ClearCache()

   Clear the internal lookup cache. Return the current version tag.

.. c:function:: long PyType_GetFlags(PyTypeObject* type)

   Return the :attr:`tp_flags` member of *type*. This function is primarily
   meant for use with `Py_LIMITED_API`; the individual flag bits are
   guaranteed to be stable across Python releases, but access to
   :attr:`tp_flags` itself is not part of the limited API.

   .. versionadded:: 3.2

.. c:function:: void PyType_Modified(PyTypeObject *type)

   Invalidate the internal lookup cache for the type and all of its
   subtypes.  This function must be called after any manual
   modification of the attributes or base classes of the type.


.. c:function:: int PyType_HasFeature(PyObject *o, int feature)

   Return true if the type object *o* sets the feature *feature*.  Type features
   are denoted by single bit flags.


.. c:function:: int PyType_IS_GC(PyObject *o)

   Return true if the type object includes support for the cycle detector; this
   tests the type flag :const:`Py_TPFLAGS_HAVE_GC`.


.. c:function:: int PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)

   Return true if *a* is a subtype of *b*.


.. c:function:: PyObject* PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)

   Generic handler for the :attr:`tp_alloc` slot of a type object.  Use
   Python's default memory allocation mechanism to allocate a new instance and
   initialize all its contents to *NULL*.

.. c:function:: PyObject* PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)

   Generic handler for the :attr:`tp_new` slot of a type object.  Create a
   new instance using the type's :attr:`tp_alloc` slot.

.. c:function:: int PyType_Ready(PyTypeObject *type)

   Finalize a type object.  This should be called on all type objects to finish
   their initialization.  This function is responsible for adding inherited slots
   from a type's base class.  Return ``0`` on success, or return ``-1`` and sets an
   exception on error.
