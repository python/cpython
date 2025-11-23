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


.. c:function:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)


.. c:function:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *wrapper, void *wrapped)


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

