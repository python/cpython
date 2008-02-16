.. highlightlang:: c

.. _instancemethod-objects:

Instance Method Objects
-----------------------

.. index:: object: instancemethod

An instance method is a wrapper for a :cdata:`PyCFunction` and the new way
to bind a :cdata:`PyCFunction` to a class object. It replaces the former call
``PyMethod_New(func, NULL, class)``.


.. cvar:: PyTypeObject PyInstanceMethod_Type

   This instance of :ctype:`PyTypeObject` represents the Python instance
   method type. It is not exposed to Python programs.


.. cfunction:: int PyInstanceMethod_Check(PyObject *o)

   Return true if *o* is an instance method object (has type
   :cdata:`PyInstanceMethod_Type`).  The parameter must not be *NULL*.


.. cfunction:: PyObject* PyInstanceMethod_New(PyObject *func)

   Return a new instance method object, with *func* being any callable object
   *func* is is the function that will be called when the instance method is
   called.


.. cfunction:: PyObject* PyInstanceMethod_Function(PyObject *im)

   Return the function object associated with the instance method *im*.


.. cfunction:: PyObject* PyInstanceMethod_GET_FUNCTION(PyObject *im)

   Macro version of :cfunc:`PyInstanceMethod_Function` which avoids error checking.


.. _method-objects:

Method Objects
--------------

.. index:: object: method

Methods are bound function objects. Methods are always bound to an instance of
an user-defined class. Unbound methods (methods bound to a class object) are
no longer available.


.. cvar:: PyTypeObject PyMethod_Type

   .. index:: single: MethodType (in module types)

   This instance of :ctype:`PyTypeObject` represents the Python method type.  This
   is exposed to Python programs as ``types.MethodType``.


.. cfunction:: int PyMethod_Check(PyObject *o)

   Return true if *o* is a method object (has type :cdata:`PyMethod_Type`).  The
   parameter must not be *NULL*.


.. cfunction:: PyObject* PyMethod_New(PyObject *func, PyObject *self)

   Return a new method object, with *func* being any callable object and *self*
   the instance the method should be bound. *func* is is the function that will
   be called when the method is called. *self* must not be *NULL*.


.. cfunction:: PyObject* PyMethod_Function(PyObject *meth)

   Return the function object associated with the method *meth*.


.. cfunction:: PyObject* PyMethod_GET_FUNCTION(PyObject *meth)

   Macro version of :cfunc:`PyMethod_Function` which avoids error checking.


.. cfunction:: PyObject* PyMethod_Self(PyObject *meth)

   Return the instance associated with the method *meth*.


.. cfunction:: PyObject* PyMethod_GET_SELF(PyObject *meth)

   Macro version of :cfunc:`PyMethod_Self` which avoids error checking.


.. cfunction:: int PyMethod_ClearFreeList(void)

   Clear the free list. Return the total number of freed items.

