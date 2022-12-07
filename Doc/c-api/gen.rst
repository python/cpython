.. highlight:: c

.. _gen-objects:

Generator Objects
-----------------

Generator objects are what Python uses to implement generator iterators. They
are normally created by iterating over a function that yields values.


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
