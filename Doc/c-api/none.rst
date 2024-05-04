.. highlight:: c

.. _noneobject:

The ``None`` Object
-------------------

.. index:: pair: object; None

Note that the :c:type:`PyTypeObject` for ``None`` is not directly exposed in the
Python/C API.  Since ``None`` is a singleton, testing for object identity (using
``==`` in C) is sufficient. There is no :c:func:`!PyNone_Check` function for the
same reason.


.. c:var:: PyObject* Py_None

   The Python ``None`` object, denoting lack of value.  This object has no methods
   and is :term:`immortal`.

   .. versionchanged:: 3.12
      :c:data:`Py_None` is :term:`immortal`.

.. c:macro:: Py_RETURN_NONE

   Return :c:data:`Py_None` from a function.
