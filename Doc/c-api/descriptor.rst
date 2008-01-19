.. highlightlang:: c

.. _descriptor-objects:

Descriptor Objects
------------------

"Descriptors" are objects that describe some attribute of an object. They are
found in the dictionary of type objects.


.. cvar:: PyTypeObject PyProperty_Type

   The type object for the built-in descriptor types.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewGetSet(PyTypeObject *type, struct PyGetSetDef *getset)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewMember(PyTypeObject *type, struct PyMemberDef *meth)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *wrapper, void *wrapped)

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)

   .. versionadded:: 2.3


.. cfunction:: int PyDescr_IsData(PyObject *descr)

   Return true if the descriptor objects *descr* describes a data attribute, or
   false if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.

   .. versionadded:: 2.2


.. cfunction:: PyObject* PyWrapper_New(PyObject *, PyObject *)

   .. versionadded:: 2.2
