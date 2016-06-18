.. highlightlang:: c

.. _classobjects:

Class and Instance Objects
--------------------------

.. index:: object: class

Note that the class objects described here represent old-style classes, which
will go away in Python 3. When creating new types for extension modules, you
will want to work with type objects (section :ref:`typeobjects`).


.. c:type:: PyClassObject

   The C structure of the objects used to describe built-in classes.


.. c:var:: PyObject* PyClass_Type

   .. index:: single: ClassType (in module types)

   This is the type object for class objects; it is the same object as
   ``types.ClassType`` in the Python layer.


.. c:function:: int PyClass_Check(PyObject *o)

   Return true if the object *o* is a class object, including instances of types
   derived from the standard class object.  Return false in all other cases.


.. c:function:: int PyClass_IsSubclass(PyObject *klass, PyObject *base)

   Return true if *klass* is a subclass of *base*. Return false in all other cases.


.. index:: object: instance

There are very few functions specific to instance objects.


.. c:var:: PyTypeObject PyInstance_Type

   Type object for class instances.


.. c:function:: int PyInstance_Check(PyObject *obj)

   Return true if *obj* is an instance.


.. c:function:: PyObject* PyInstance_New(PyObject *class, PyObject *arg, PyObject *kw)

   Create a new instance of a specific class.  The parameters *arg* and *kw* are
   used as the positional and keyword parameters to the object's constructor.


.. c:function:: PyObject* PyInstance_NewRaw(PyObject *class, PyObject *dict)

   Create a new instance of a specific class without calling its constructor.
   *class* is the class of new object.  The *dict* parameter will be used as the
   object's :attr:`~object.__dict__`; if *NULL*, a new dictionary will be created for the
   instance.
