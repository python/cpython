.. highlightlang:: c

.. _typeobjects:

Type Objects
------------

.. index:: object: type


.. ctype:: PyTypeObject

   The C structure of the objects used to describe built-in types.


.. cvar:: PyObject* PyType_Type

   .. index:: single: TypeType (in module types)

   This is the type object for type objects; it is the same object as ``type`` and
   ``types.TypeType`` in the Python layer.


.. cfunction:: int PyType_Check(PyObject *o)

   Return true if the object *o* is a type object, including instances of types
   derived from the standard type object.  Return false in all other cases.


.. cfunction:: int PyType_CheckExact(PyObject *o)

   Return true if the object *o* is a type object, but not a subtype of the
   standard type object.  Return false in all other cases.

   .. versionadded:: 2.2


.. cfunction:: unsigned int PyType_ClearCache(void)

   Clear the internal lookup cache. Return the current version tag.

   .. versionadded:: 2.6


.. cfunction:: void PyType_Modified(PyTypeObject *type)

   Invalidate the internal lookup cache for the type and all of its
   subtypes.  This function must be called after any manual
   modification of the attributes or base classes of the type.

   .. versionadded:: 2.6


.. cfunction:: int PyType_HasFeature(PyObject *o, int feature)

   Return true if the type object *o* sets the feature *feature*.  Type features
   are denoted by single bit flags.


.. cfunction:: int PyType_IS_GC(PyObject *o)

   Return true if the type object includes support for the cycle detector; this
   tests the type flag :const:`Py_TPFLAGS_HAVE_GC`.

   .. versionadded:: 2.0


.. cfunction:: int PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)

   Return true if *a* is a subtype of *b*.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)

   .. versionadded:: 2.2

   .. versionchanged:: 2.5
      This function used an :ctype:`int` type for *nitems*. This might require
      changes in your code for properly supporting 64-bit systems.


.. cfunction:: PyObject* PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)

   .. versionadded:: 2.2


.. cfunction:: int PyType_Ready(PyTypeObject *type)

   Finalize a type object.  This should be called on all type objects to finish
   their initialization.  This function is responsible for adding inherited slots
   from a type's base class.  Return ``0`` on success, or return ``-1`` and sets an
   exception on error.

   .. versionadded:: 2.2
