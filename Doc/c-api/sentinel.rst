.. highlight:: c

.. _sentinelobjects:

Sentinel Objects
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
   Return ``NULL`` with an exception set on failure.

   *module_name* should be the name of an importable module if the sentinel
   should support pickling.

   .. versionadded:: next
