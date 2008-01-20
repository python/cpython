.. highlightlang:: c

.. _descriptor-objects:

Descriptor Objects
------------------

"Descriptors" are objects that describe some attribute of an object. They are
found in the dictionary of type objects.

.. XXX document these!

.. cvar:: PyTypeObject PyProperty_Type

   The type object for the built-in descriptor types.


.. cfunction:: PyObject* PyDescr_NewGetSet(PyTypeObject *type, struct PyGetSetDef *getset)


.. cfunction:: PyObject* PyDescr_NewMember(PyTypeObject *type, struct PyMemberDef *meth)


.. cfunction:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)


.. cfunction:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *wrapper, void *wrapped)


.. cfunction:: PyObject* PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)


.. cfunction:: int PyDescr_IsData(PyObject *descr)

   Return true if the descriptor objects *descr* describes a data attribute, or
   false if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.


.. cfunction:: PyObject* PyWrapper_New(PyObject *, PyObject *)
