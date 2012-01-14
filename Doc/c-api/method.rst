.. highlightlang:: c

.. _method-objects:

Method Objects
--------------

.. index:: object: method

There are some useful functions that are useful for working with method objects.


.. c:var:: PyTypeObject PyMethod_Type

   .. index:: single: MethodType (in module types)

   This instance of :c:type:`PyTypeObject` represents the Python method type.  This
   is exposed to Python programs as ``types.MethodType``.


.. c:function:: int PyMethod_Check(PyObject *o)

   Return true if *o* is a method object (has type :c:data:`PyMethod_Type`).  The
   parameter must not be *NULL*.


.. c:function:: PyObject* PyMethod_New(PyObject *func, PyObject *self, PyObject *class)

   Return a new method object, with *func* being any callable object; this is the
   function that will be called when the method is called.  If this method should
   be bound to an instance, *self* should be the instance and *class* should be the
   class of *self*, otherwise *self* should be *NULL* and *class* should be the
   class which provides the unbound method..


.. c:function:: PyObject* PyMethod_Class(PyObject *meth)

   Return the class object from which the method *meth* was created; if this was
   created from an instance, it will be the class of the instance.


.. c:function:: PyObject* PyMethod_GET_CLASS(PyObject *meth)

   Macro version of :c:func:`PyMethod_Class` which avoids error checking.


.. c:function:: PyObject* PyMethod_Function(PyObject *meth)

   Return the function object associated with the method *meth*.


.. c:function:: PyObject* PyMethod_GET_FUNCTION(PyObject *meth)

   Macro version of :c:func:`PyMethod_Function` which avoids error checking.


.. c:function:: PyObject* PyMethod_Self(PyObject *meth)

   Return the instance associated with the method *meth* if it is bound, otherwise
   return *NULL*.


.. c:function:: PyObject* PyMethod_GET_SELF(PyObject *meth)

   Macro version of :c:func:`PyMethod_Self` which avoids error checking.


.. c:function:: int PyMethod_ClearFreeList()

   Clear the free list. Return the total number of freed items.

   .. versionadded:: 2.6
