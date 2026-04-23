.. highlight:: c

.. _descriptor-objects:

Descriptor Objects
------------------

"Descriptors" are objects that describe some attribute of an object. They are
found in the dictionary of type objects.

.. c:function:: PyObject* PyDescr_NewGetSet(PyTypeObject *type, struct PyGetSetDef *getset)

   Create a new get-set descriptor for extension type *type* from the
   :c:type:`PyGetSetDef` structure *getset*.

   Get-set descriptors expose attributes implemented by C getter and setter
   functions rather than stored directly in the instance. This is the same kind
   of descriptor created for entries in :c:member:`~PyTypeObject.tp_getset`, and
   it appears in Python as a :class:`types.GetSetDescriptorType` object.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

.. c:function:: PyObject* PyDescr_NewMember(PyTypeObject *type, struct PyMemberDef *member)

   Create a new member descriptor for extension type *type* from the
   :c:type:`PyMemberDef` structure *member*.

   Member descriptors expose fields in the type's C struct as Python
   attributes. This is the same kind of descriptor created for entries in
   :c:member:`~PyTypeObject.tp_members`, and it appears in Python as a
   :class:`types.MemberDescriptorType` object.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

.. c:var:: PyTypeObject PyMemberDescr_Type

   The type object for member descriptor objects created from
   :c:type:`PyMemberDef` structures. These descriptors expose fields of a
   C struct as attributes on a type, and correspond
   to :class:`types.MemberDescriptorType` objects in Python.



.. c:var:: PyTypeObject PyGetSetDescr_Type

   The type object for get/set descriptor objects created from
   :c:type:`PyGetSetDef` structures. These descriptors implement attributes
   whose value is computed by C getter and setter functions, and are used
   for many built-in type attributes. They correspond to
   :class:`types.GetSetDescriptorType` objects in Python.


.. c:function:: PyObject* PyDescr_NewMethod(PyTypeObject *type, struct PyMethodDef *meth)

   Create a new method descriptor for extension type *type* from the
   :c:type:`PyMethodDef` structure *meth*.

   Method descriptors expose C functions as methods on a type. This is the same
   kind of descriptor created for entries in
   :c:member:`~PyTypeObject.tp_methods`, and it appears in Python as a
   :class:`types.MethodDescriptorType` object.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

.. c:var:: PyTypeObject PyMethodDescr_Type

   The type object for method descriptor objects created from
   :c:type:`PyMethodDef` structures. These descriptors expose C functions as
   methods on a type, and correspond to :class:`types.MethodDescriptorType`
   objects in Python.


.. c:struct:: wrapperbase

   Describes a slot wrapper used by :c:func:`PyDescr_NewWrapper`.

   Each ``wrapperbase`` record stores the Python-visible name and metadata for a
   special method implemented by a type slot, together with the wrapper
   function used to adapt that slot to Python's calling convention.

.. c:function:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)

   Create a new wrapper descriptor for extension type *type* from the
   :c:struct:`wrapperbase` structure *base* and the wrapped slot function
   pointer
   *wrapped*.

   Wrapper descriptors expose special methods implemented by type slots. This
   is the same kind of descriptor that CPython creates for slot-based special
   methods such as ``__repr__`` or ``__add__``, and it appears in Python as a
   :class:`types.WrapperDescriptorType` object.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

.. c:var:: PyTypeObject PyWrapperDescr_Type

   The type object for wrapper descriptor objects created by
   :c:func:`PyDescr_NewWrapper` and :c:func:`PyWrapper_New`. Wrapper
   descriptors are used internally to expose special methods implemented
   via wrapper structures, and appear in Python as
   :class:`types.WrapperDescriptorType` objects.


.. c:function:: PyObject* PyDescr_NewClassMethod(PyTypeObject *type, PyMethodDef *method)

   Create a new class method descriptor for extension type *type* from the
   :c:type:`PyMethodDef` structure *method*.

   Class method descriptors expose C methods that receive the class rather than
   an instance when accessed. This is the same kind of descriptor created for
   ``METH_CLASS`` entries in :c:member:`~PyTypeObject.tp_methods`, and it
   appears in Python as a :class:`types.ClassMethodDescriptorType` object.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

.. c:function:: int PyDescr_IsData(PyObject *descr)

   Return non-zero if the descriptor object *descr* describes a data attribute, or
   ``0`` if it describes a method.  *descr* must be a descriptor object; there is
   no error checking.


.. c:function:: PyObject* PyWrapper_New(PyObject *d, PyObject *self)

   Create a new bound wrapper object from the wrapper descriptor *d* and the
   instance *self*.

   This is the bound form of a wrapper descriptor created by
   :c:func:`PyDescr_NewWrapper`. CPython creates these objects when a slot
   wrapper is accessed through an instance, and they appear in Python as
   :class:`types.MethodWrapperType` objects.

   On success, return a :term:`strong reference` to the wrapper object. Return
   ``NULL`` with an exception set on failure.

.. c:macro:: PyDescr_COMMON

   This is a macro including the common fields for a
   descriptor object.

   This was included in Python's C API by mistake; do not use it in extensions.
   For creating custom descriptor objects, create a class implementing the
   descriptor protocol (:c:member:`~PyTypeObject.tp_descr_get` and
   :c:member:`~PyTypeObject.tp_descr_set`).

   .. soft-deprecated:: 3.15


Built-in descriptors
^^^^^^^^^^^^^^^^^^^^

.. c:var:: PyTypeObject PyProperty_Type

   The type object for property objects. This is the same object as
   :class:`property` in the Python layer.


.. c:var:: PyTypeObject PySuper_Type

   The type object for super objects. This is the same object as
   :class:`super` in the Python layer.


.. c:var:: PyTypeObject PyClassMethod_Type

   The type of class method objects. This is the same object as
   :class:`classmethod` in the Python layer.


.. c:var:: PyTypeObject PyClassMethodDescr_Type

   The type object for C-level class method descriptor objects.
   This is the type of the descriptors created for :func:`classmethod` defined
   in C extension types, and corresponds to
   :class:`types.ClassMethodDescriptorType` objects in Python.


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

