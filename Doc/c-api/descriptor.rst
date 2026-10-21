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

   Describes how a type slot is exposed to Python as a special method.

   This is the structure behind the *slot wrapper* objects found in the
   dictionaries of built-in and extension types, such as ``int.__add__`` or
   ``object.__repr__``. CPython keeps an internal table of these records, one
   for each special method name that corresponds to a slot of
   :c:type:`PyTypeObject` or one of its sub-structures (such as
   :c:type:`PyNumberMethods`).

   Extensions do not normally need this structure, because CPython creates
   wrapper descriptors for the slots of every type automatically. It is useful
   when a C function with a slot-like signature should be exposed as a
   special method without being installed in a type slot, or when a slot
   function needs to be exposed under a different name or with a different
   docstring than the default one.

   A ``wrapperbase`` is not copied by :c:func:`PyDescr_NewWrapper`; it must
   stay valid for as long as any descriptor created from it exists.
   It is usually a static variable.

   .. c:member:: const char *name

      The name of the special method, such as ``"__add__"``.
      This becomes the descriptor's :attr:`~definition.__name__` and is
      used in error messages.

   .. c:member:: int offset

      Offset of the slot that this record describes, relative to
      the start of :c:type:`!PyHeapTypeObject`.
      This is generally not useful to extensions, as :c:func:`PyDescr_NewWrapper`
      does not use this; set this to 0 when using that function.

   .. c:member:: void *function

      The generic slot implementation that CPython installs in a type's slot
      when the corresponding special method is defined in Python, for example
      in a subclass. Like :c:member:`~wrapperbase.offset`, it is only used by
      CPython's internal table; set it to ``NULL`` for records that are only
      used with :c:func:`PyDescr_NewWrapper`.

   .. c:member:: wrapperfunc wrapper

      The function that adapts the slot's C calling convention to a Python
      call. It receives the instance the special method was called on, a
      tuple of the remaining positional arguments, and the wrapped slot
      function pointer. It must check that the number and types of arguments
      are suitable for the slot function, call it, and convert the result
      to a Python object.

      If :c:member:`~wrapperbase.flags` includes
      :c:macro:`PyWrapperFlag_KEYWORDS`, this field must instead point to a
      :c:type:`wrapperfunc_kwds` function, cast to :c:type:`wrapperfunc`.

   .. c:member:: const char *doc

      The docstring of the special method, or ``NULL``.

   .. c:member:: int flags

      Bitwise OR of flags that describe the wrapper function. Currently, the
      only flag is :c:macro:`PyWrapperFlag_KEYWORDS`; all other bits must be
      zero.

   .. c:member:: PyObject *name_strobj

      Reserved for internal use. Initialize it to ``NULL``.

.. c:type:: PyObject *(*wrapperfunc)(PyObject *self, PyObject *args, void *wrapped)

   The type of the :c:member:`~wrapperbase.wrapper` function of a
   :c:struct:`wrapperbase`. *self* is the instance the special method is called
   on, *args* is a tuple of the positional arguments that follow it, and
   *wrapped* is the slot function pointer that was passed to
   :c:func:`PyDescr_NewWrapper`.

   On success, return a :term:`strong reference` to the result. Return ``NULL``
   with an exception set on failure.

.. c:type:: PyObject *(*wrapperfunc_kwds)(PyObject *self, PyObject *args, void *wrapped, PyObject *kwds)

   Like :c:type:`wrapperfunc`, but with an additional *kwds* argument holding
   a dictionary of keyword arguments, or ``NULL`` if no keyword arguments were
   passed. Used for wrappers whose :c:member:`~wrapperbase.flags` include
   :c:macro:`PyWrapperFlag_KEYWORDS`.

.. c:macro:: PyWrapperFlag_KEYWORDS

   Flag that can be set in the :c:member:`~wrapperbase.flags` field of a
   :c:struct:`wrapperbase` structure.
   When set, the :c:member:`~wrapperbase.wrapper` function has the
   :c:type:`wrapperfunc_kwds` signature and is passed the keyword arguments
   of the call as a :class:`dict`.
   When not set, calling the wrapper descriptor with keyword arguments
   raises :exc:`TypeError`.

.. c:function:: PyObject* PyDescr_NewWrapper(PyTypeObject *type, struct wrapperbase *base, void *wrapped)

   Create a new wrapper descriptor for extension type *type* from the
   :c:struct:`wrapperbase` structure *base* and the slot function pointer
   *wrapped*.

   Wrapper descriptors expose special methods implemented by type slots. This
   is the same kind of descriptor that CPython creates for slot-based special
   methods such as ``__repr__`` or ``__add__``, and it appears in Python as a
   :class:`types.WrapperDescriptorType` object.

   The descriptor only references *base*; see :c:struct:`wrapperbase` for the
   lifetime requirements.

   On success, return a :term:`strong reference` to the descriptor. Return
   ``NULL`` with an exception set on failure.

   For example, the following gives the ``__len__`` method of a static type a
   custom docstring. :c:func:`PyType_Ready` would normally create the
   ``__len__`` descriptor itself from the
   :c:member:`~PySequenceMethods.sq_length` slot, but it leaves the entry
   alone if the type's dictionary already contains one::

      static Py_ssize_t
      my_len(PyObject *self)
      {
          /* ... */
      }

      static PyObject *
      wrap_lenfunc(PyObject *self, PyObject *args, void *wrapped)
      {
          lenfunc func = (lenfunc)wrapped;
          if (PyTuple_GET_SIZE(args) != 0) {
              PyErr_SetString(PyExc_TypeError,
                              "__len__() takes no arguments");
              return NULL;
          }
          Py_ssize_t result = func(self);
          if (result == -1 && PyErr_Occurred()) {
              return NULL;
          }
          return PyLong_FromSsize_t(result);
      }

      static struct wrapperbase len_wrapper = {
          .name = "__len__",
          .wrapper = wrap_lenfunc,
          .doc = "__len__($self, /)\n--\n\nReturn the number of items.",
      };

      static PySequenceMethods my_as_sequence = {
          .sq_length = my_len,
      };

      static PyTypeObject MyType = {
          PyVarObject_HEAD_INIT(NULL, 0)
          .tp_name = "mymodule.MyType",
          .tp_as_sequence = &my_as_sequence,
          /* ... */
      };

      static int
      ready_my_type(void)
      {
          MyType.tp_dict = PyDict_New();
          if (MyType.tp_dict == NULL) {
              return -1;
          }
          PyObject *descr = PyDescr_NewWrapper(&MyType, &len_wrapper,
                                               (void *)my_len);
          if (descr == NULL) {
              return -1;
          }
          int result = PyDict_SetItemString(MyType.tp_dict, "__len__", descr);
          Py_DECREF(descr);
          if (result < 0) {
              return -1;
          }
          return PyType_Ready(&MyType);
      }

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

.. c:macro:: PyDescr_TYPE(descr)

   Return a :term:`borrowed reference` to the type that the descriptor
   *descr* belongs to. This is accessible as :attr:`~object.__objclass__`
   in Python.

   *descr* must be a descriptor object created by one of the
   ``PyDescr_New*`` functions (an instance of
   :c:var:`PyMethodDescr_Type`, :c:var:`PyClassMethodDescr_Type`,
   :c:var:`PyMemberDescr_Type`, :c:var:`PyGetSetDescr_Type`, or
   :c:var:`PyWrapperDescr_Type`). If the passed descriptor is not
   an instance of one of these types, the behavior is undefined.


.. c:macro:: PyDescr_NAME(descr)

   Return the name of the descriptor *descr* as a :term:`borrowed reference`
   to a :class:`str` object. This is available as :attr:`~definition.__name__`
   in Python.

   *descr* must be a descriptor object created by one of the ``PyDescr_New*``
   functions. If not, the behavior is undefined.


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

