.. highlight:: c

.. _sentinelobjects:

Sentinel objects
----------------

.. c:var:: PyTypeObject PySentinel_Type

   This instance of :c:type:`PyTypeObject` represents the Python
   :class:`sentinel` type.  This is the same object as :class:`sentinel`.

   .. versionadded:: next

.. c:function:: int PySentinel_Check(PyObject *o)

   Return true if *o* is a :class:`sentinel` object.  The :class:`sentinel` type
   does not allow subclasses, so this check is exact.

   .. versionadded:: next

.. c:function:: PyObject* PySentinel_New(const char *name, const char *module_name)

   Return a new :class:`sentinel` object with :attr:`~sentinel.__name__` set to
   *name* and :attr:`~sentinel.__module__` set to *module_name*.
   *name* must not be ``NULL``. If *module_name* is ``NULL``, :attr:`~sentinel.__module__`
   is set to ``None``.
   Return ``NULL`` with an exception set on failure.

   For pickling to work, *module_name* must be the name of an importable
   module, and the sentinel must be accessible from that module under a
   path matching *name*.  Pickle treats *name* as a global variable name
   in *module_name* (see :meth:`object.__reduce__`).

   .. versionadded:: next
