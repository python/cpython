.. highlight:: c

.. _gen-objects:

Generator Objects
-----------------

Generator objects are what Python uses to implement generator iterators. They
are normally created by iterating over a function that yields values, rather
than explicitly calling :c:func:`PyGen_New` or :c:func:`PyGen_NewWithQualName`.


.. c:type:: PyGenObject

   The C structure used for generator objects.


.. c:var:: PyTypeObject PyGen_Type

   The type object corresponding to generator objects.


.. c:function:: int PyGen_Check(PyObject *ob)

   Return true if *ob* is a generator object; *ob* must not be ``NULL``.


.. c:function:: int PyGen_CheckExact(PyObject *ob)

   Return true if *ob*'s type is :c:type:`PyGen_Type`; *ob* must not be ``NULL``.


.. c:function:: PyObject* PyGen_New(PyFrameObject *frame)

   Create and return a new generator object based on the *frame* object.
   A reference to *frame* is stolen by this function. The argument must not be
   ``NULL``.

.. c:function:: PyObject* PyGen_NewWithQualName(PyFrameObject *frame, PyObject *name, PyObject *qualname)

   Create and return a new generator object based on the *frame* object,
   with ``__name__`` and ``__qualname__`` set to *name* and *qualname*.
   A reference to *frame* is stolen by this function.  The *frame* argument
   must not be ``NULL``.

.. c:function:: PySendResult PyGen_Send(PyGenObject *gen, PyObject *arg, PyObject **presult)

   Sends the *arg* value into the generator *gen*. Coroutine objects
   are also allowed to be as the *gen* argument but they need to be
   explicitly casted to PyGenObject*. Returns:

   - ``PYGEN_RETURN`` if generator returns. Return value is returned via *presult*.
   - ``PYGEN_NEXT`` if generator yields. Yielded value is returned via *presult*.
   - ``PYGEN_ERROR`` if generator has raised and exception. *presult* is set to ``NULL``.
