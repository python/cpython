.. highlightlang:: c

.. _gen-objects:

Generator Objects
-----------------

Generator objects are what Python uses to implement generator iterators. They
are normally created by iterating over a function that yields values, rather
than explicitly calling :cfunc:`PyGen_New`.


.. ctype:: PyGenObject

   The C structure used for generator objects.


.. cvar:: PyTypeObject PyGen_Type

   The type object corresponding to generator objects


.. cfunction:: int PyGen_Check(ob)

   Return true if *ob* is a generator object; *ob* must not be *NULL*.


.. cfunction:: int PyGen_CheckExact(ob)

   Return true if *ob*'s type is *PyGen_Type* is a generator object; *ob* must not
   be *NULL*.


.. cfunction:: PyObject* PyGen_New(PyFrameObject *frame)

   Create and return a new generator object based on the *frame* object. A
   reference to *frame* is stolen by this function. The parameter must not be
   *NULL*.
