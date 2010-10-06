.. highlightlang:: c

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

   Return true if the descriptor objects *descr* describes a data attribute, or
   false if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.


.. c:function:: PyObject* PyWrapper_New(PyObject *, PyObject *)
