.. highlightlang:: c

.. _noneobject:

The None Object
---------------

.. index:: object: None

Note that the :ctype:`PyTypeObject` for ``None`` is not directly exposed in the
Python/C API.  Since ``None`` is a singleton, testing for object identity (using
``==`` in C) is sufficient. There is no :cfunc:`PyNone_Check` function for the
same reason.


.. cvar:: PyObject* Py_None

   The Python ``None`` object, denoting lack of value.  This object has no methods.
   It needs to be treated just like any other object with respect to reference
   counts.


.. cmacro:: Py_RETURN_NONE

   Properly handle returning :cdata:`Py_None` from within a C function (that is,
   increment the reference count of None and return it.)
