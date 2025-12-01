.. highlight:: c

.. _descriptor-objects:

Descriptor Objects
------------------

"Descriptors" are objects that describe some attribute of an object. They are
found in the dictionary of type objects.

.. XXX document these!

.. c:var:: PyTypeObject PyProperty_Type

   The type object for the built-in descriptor types.


.. c:function:: PyObject* PyDescr_NewGetSet(PyTypeObject *type, struct PyGetSetDef *getset)


.. c:function:: PyObject* PyDescr_NewMember(PyTypeObject *type, struct PyMemberDef *meth)


.. c:var:: PyTypeObject PyMemberDescr_Type

   The type object for member descriptor objects created from
   :c:type:`PyMemberDef` structures. These descriptors expose fields of a
   C struct as attributes on a type, and correspond
   to :class:`types.MemberDescriptorType` objects in Python.



.. c:var:: PyTypeObject PyGetSetDescr_Type

   The type object for get/set descriptor objects created from
   :c:type:`PyGetSetDef` structures. These descriptors implement attributes
   whose value is computed by C getter and setter functions, and are used
   for many built-in type attributes.


.. c:function:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)


.. c:var:: PyTypeObject PyMethodDescr_Type

   The type object for method descriptor objects created from
   :c:type:`PyMethodDef` structures. These descriptors expose C functions as
   methods on a type, and correspond to :class:`types.MemberDescriptorType`
   objects in Python.


.. c:function:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *wrapper, void *wrapped)


.. c:var:: PyTypeObject PyWrapperDescr_Type

   The type object for wrapper descriptor objects created by
   :c:func:`PyDescr_NewWrapper` and :c:func:`PyWrapper_New`. Wrapper
   descriptors are used internally to expose special methods implemented
   via wrapper structures, and appear in Python as
   :class:`types.WrapperDescriptorType` objects.


.. c:function:: PyObject* PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)


.. c:function:: int PyDescr_IsData(PyObject *descr)

   Return non-zero if the descriptor object *descr* describes a data attribute, or
   ``0`` if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.


.. c:function:: PyObject* PyWrapper_New(PyObject *, PyObject *)


Built-in descriptors
^^^^^^^^^^^^^^^^^^^^

.. c:var:: PyTypeObject PySuper_Type

   The type object for super objects. This is the same object as
   :class:`super` in the Python layer.


.. c:var:: PyTypeObject PyClassMethod_Type

   The type of class method objects. This is the same object as
   :class:`classmethod` in the Python layer.


.. c:var:: PyTypeObject PyClassMethodDescr_Type

   The type object for C-level class method descriptor objects.
   This is the type of the descriptors created for :func:`classmethod` defined in
   C extension types, and is the same object as :class:`classmethod`
   in Python.


.. c:function:: PyObject *PyClassMethod_New(PyObject *callable)

   Create a new :class:`classmethod` object wrapping *callable*.
   *callable* must be a callable object and must not be ``NULL``.

   On success, this function returns a :term:`strong reference` to a new class
   method descriptor. On failure, this function returns ``NULL`` with an
   exception set.


.. c:var:: PyTypeObject PyStaticMethod_Type

   The type of static method objects. This is the same object as
   :class:`staticmethod` in the Python layer.


.. c:function:: PyObject *PyStaticMethod_New(PyObject *callable)

   Create a new :class:`staticmethod` object wrapping *callable*.
   *callable* must be a callable object and must not be ``NULL``.

   On success, this function returns a :term:`strong reference` to a new static
   method descriptor. On failure, this function returns ``NULL`` with an
   exception set.

