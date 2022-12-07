.. highlight:: c

.. _coro-objects:

Coroutine Objects
-----------------

.. versionadded:: 3.5

Coroutine objects are what functions declared with an ``async`` keyword
return.


.. c:type:: PyCoroObject

   The C structure used for coroutine objects.


.. c:var:: PyTypeObject PyCoro_Type

   The type object corresponding to coroutine objects.


.. c:function:: int PyCoro_CheckExact(PyObject *ob)

   Return true if *ob*'s type is :c:type:`PyCoro_Type`; *ob* must not be ``NULL``.
   This function always succeeds.
