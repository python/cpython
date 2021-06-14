.. highlight:: c

.. _typehintobjects:

Objects for Type Hinting
------------------------

Various built-in types for type hinting are provided.  Currently,
two types exist -- :ref:`GenericAlias <types-genericalias>` and
:ref:`Union <types-union>`.  Only ``GenericAlias`` objects are currently exposed
when including :file:`Python.h`.

.. c:function:: PyObject* Py_GenericAlias(PyObject *origin, PyObject *args)

   Return a :ref:`GenericAlias <types-genericalias>` object on success,
   ``NULL`` on failure.  Equivalent to calling the Python class
   :class:`types.GenericAlias`.  *origin* and *args* arguments set the
   ``GenericAlias``\ 's ``__origin__`` and ``__args__`` attributes respectively.
   *origin* should be a type, and *args* **must** be a :c:type:`PyTupleObject`.
   Minimal checking is done on the arguments, so the function will succeed even
   if *origin* is not a type.
   The ``GenericAlias``\ 's ``__parameters__`` attribute is constructed lazily
   from ``__args__``.

.. c:var:: PyTypeObject Py_GenericAliasType

   The C type of the object returned by :c:func:`Py_GenericAlias`. Equivalent to
   :class:`types.GenericAlias` in Python.
