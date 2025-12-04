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

   Return true if *ob* is a generator object; *ob* must not be ``NULL``.  This
   function always succeeds.


.. c:function:: int PyGen_CheckExact(PyObject *ob)

   Return true if *ob*'s type is :c:type:`PyGen_Type`; *ob* must not be
   ``NULL``.  This function always succeeds.


.. c:function:: PyObject* PyGen_New(PyFrameObject *frame)

   Create and return a new generator object based on the *frame* object.
   A reference to *frame* is stolen by this function. The argument must not be
   ``NULL``.

.. c:function:: PyObject* PyGen_NewWithQualName(PyFrameObject *frame, PyObject *name, PyObject *qualname)

   Create and return a new generator object based on the *frame* object,
   with ``__name__`` and ``__qualname__`` set to *name* and *qualname*.
   A reference to *frame* is stolen by this function.  The *frame* argument
   must not be ``NULL``.

.. c:function:: PyCodeObject* PyGen_GetCode(PyGenObject *gen)

   Return a new :term:`strong reference` to the code object wrapped by *gen*.
   This function always succeeds.


Asynchronous Generator Objects
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. seealso::
   :pep:`525`

.. c:var:: PyTypeObject PyAsyncGen_Type

   The type object corresponding to asynchronous generator objects. This is
   available as :class:`types.AsyncGeneratorType` in the Python layer.

   .. versionadded:: 3.6

.. c:function:: PyObject *PyAsyncGen_New(PyFrameObject *frame, PyObject *name, PyObject *qualname)

   Create a new asynchronous generator wrapping *frame*, with ``__name__`` and
   ``__qualname__`` set to *name* and *qualname*. *frame* is stolen by this
   function and must not be ``NULL``.

   On success, this function returns a :term:`strong reference` to the
   new asynchronous generator. On failure, this function returns ``NULL``
   with an exception set.

   .. versionadded:: 3.6

.. c:function:: int PyAsyncGen_CheckExact(PyObject *op)

   Return true if *op* is an asynchronous generator object, false otherwise.
   This function always succeeds.

   .. versionadded:: 3.6
