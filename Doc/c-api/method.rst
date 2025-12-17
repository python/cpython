.. highlight:: c

.. _instancemethod-objects:

Instance Method Objects
-----------------------

.. index:: pair: object; instancemethod

An instance method is a wrapper for a :c:type:`PyCFunction` and the new way
to bind a :c:type:`PyCFunction` to a class object. It replaces the former call
``PyMethod_New(func, NULL, class)``.


.. c:var:: PyTypeObject PyInstanceMethod_Type

   This instance of :c:type:`PyTypeObject` represents the Python instance
   method type. It is not exposed to Python programs.


.. c:function:: int PyInstanceMethod_Check(PyObject *o)

   Return true if *o* is an instance method object (has type
   :c:data:`PyInstanceMethod_Type`).  The parameter must not be ``NULL``.
   This function always succeeds.


.. c:function:: PyObject* PyInstanceMethod_New(PyObject *func)

   Return a new instance method object, with *func* being any callable object.
   *func* is the function that will be called when the instance method is
   called.


.. c:function:: PyObject* PyInstanceMethod_Function(PyObject *im)

   Return the function object associated with the instance method *im*.


.. c:function:: PyObject* PyInstanceMethod_GET_FUNCTION(PyObject *im)

   Macro version of :c:func:`PyInstanceMethod_Function` which avoids error checking.


.. _method-objects:

Method Objects
--------------

.. index:: pair: object; method

Methods are bound function objects. Methods are always bound to an instance of
a user-defined class. Unbound methods (methods bound to a class object) are
no longer available.


.. c:var:: PyTypeObject PyMethod_Type

   .. index:: single: MethodType (in module types)

   This instance of :c:type:`PyTypeObject` represents the Python method type.  This
   is exposed to Python programs as ``types.MethodType``.


.. c:function:: int PyMethod_Check(PyObject *o)

   Return true if *o* is a method object (has type :c:data:`PyMethod_Type`).  The
   parameter must not be ``NULL``.  This function always succeeds.


.. c:function:: PyObject* PyMethod_New(PyObject *func, PyObject *self)

   Return a new method object, with *func* being any callable object and *self*
   the instance the method should be bound. *func* is the function that will
   be called when the method is called. *self* must not be ``NULL``.


.. c:function:: PyObject* PyMethod_Function(PyObject *meth)

   Return the function object associated with the method *meth*.


.. c:function:: PyObject* PyMethod_GET_FUNCTION(PyObject *meth)

   Macro version of :c:func:`PyMethod_Function` which avoids error checking.


.. c:function:: PyObject* PyMethod_Self(PyObject *meth)

   Return the instance associated with the method *meth*.


.. c:function:: PyObject* PyMethod_GET_SELF(PyObject *meth)

   Macro version of :c:func:`PyMethod_Self` which avoids error checking.
