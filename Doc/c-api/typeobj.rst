.. highlightlang:: c

.. _type-structs:

Type Objects
============

Perhaps one of the most important structures of the Python object system is the
structure that defines a new type: the :c:type:`PyTypeObject` structure.  Type
objects can be handled using any of the :c:func:`PyObject_\*` or
:c:func:`PyType_\*` functions, but do not offer much that's interesting to most
Python applications. These objects are fundamental to how objects behave, so
they are very important to the interpreter itself and to any extension module
that implements new types.

Type objects are fairly large compared to most of the standard types. The reason
for the size is that each type object stores a large number of values, mostly C
function pointers, each of which implements a small part of the type's
functionality.  The fields of the type object are examined in detail in this
section.  The fields will be described in the order in which they occur in the
structure.

Typedefs: unaryfunc, binaryfunc, ternaryfunc, inquiry, intargfunc,
intintargfunc, intobjargproc, intintobjargproc, objobjargproc, destructor,
freefunc, printfunc, getattrfunc, getattrofunc, setattrfunc, setattrofunc,
reprfunc, hashfunc

The structure definition for :c:type:`PyTypeObject` can be found in
:file:`Include/object.h`.  For convenience of reference, this repeats the
definition found there:

.. literalinclude:: ../includes/typestruct.h


The type object structure extends the :c:type:`PyVarObject` structure. The
:attr:`ob_size` field is used for dynamic types (created by  :func:`type_new`,
usually called from a class statement). Note that :c:data:`PyType_Type` (the
metatype) initializes :c:member:`~PyTypeObject.tp_itemsize`, which means that its instances (i.e.
type objects) *must* have the :attr:`ob_size` field.


.. c:member:: PyObject* PyObject._ob_next
             PyObject* PyObject._ob_prev

   These fields are only present when the macro ``Py_TRACE_REFS`` is defined.
   Their initialization to ``NULL`` is taken care of by the ``PyObject_HEAD_INIT``
   macro.  For statically allocated objects, these fields always remain ``NULL``.
   For dynamically allocated objects, these two fields are used to link the object
   into a doubly-linked list of *all* live objects on the heap.  This could be used
   for various debugging purposes; currently the only use is to print the objects
   that are still alive at the end of a run when the environment variable
   :envvar:`PYTHONDUMPREFS` is set.

   These fields are not inherited by subtypes.


.. c:member:: Py_ssize_t PyObject.ob_refcnt

   This is the type object's reference count, initialized to ``1`` by the
   ``PyObject_HEAD_INIT`` macro.  Note that for statically allocated type objects,
   the type's instances (objects whose :attr:`ob_type` points back to the type) do
   *not* count as references.  But for dynamically allocated type objects, the
   instances *do* count as references.

   This field is not inherited by subtypes.


.. c:member:: PyTypeObject* PyObject.ob_type

   This is the type's type, in other words its metatype.  It is initialized by the
   argument to the ``PyObject_HEAD_INIT`` macro, and its value should normally be
   ``&PyType_Type``.  However, for dynamically loadable extension modules that must
   be usable on Windows (at least), the compiler complains that this is not a valid
   initializer.  Therefore, the convention is to pass ``NULL`` to the
   ``PyObject_HEAD_INIT`` macro and to initialize this field explicitly at the
   start of the module's initialization function, before doing anything else.  This
   is typically done like this::

      Foo_Type.ob_type = &PyType_Type;

   This should be done before any instances of the type are created.
   :c:func:`PyType_Ready` checks if :attr:`ob_type` is ``NULL``, and if so,
   initializes it to the :attr:`ob_type` field of the base class.
   :c:func:`PyType_Ready` will not change this field if it is non-zero.

   This field is inherited by subtypes.


.. c:member:: Py_ssize_t PyVarObject.ob_size

   For statically allocated type objects, this should be initialized to zero.  For
   dynamically allocated type objects, this field has a special internal meaning.

   This field is not inherited by subtypes.


.. c:member:: const char* PyTypeObject.tp_name

   Pointer to a NUL-terminated string containing the name of the type. For types
   that are accessible as module globals, the string should be the full module
   name, followed by a dot, followed by the type name; for built-in types, it
   should be just the type name.  If the module is a submodule of a package, the
   full package name is part of the full module name.  For example, a type named
   :class:`T` defined in module :mod:`M` in subpackage :mod:`Q` in package :mod:`P`
   should have the :c:member:`~PyTypeObject.tp_name` initializer ``"P.Q.M.T"``.

   For dynamically allocated type objects, this should just be the type name, and
   the module name explicitly stored in the type dict as the value for key
   ``'__module__'``.

   For statically allocated type objects, the tp_name field should contain a dot.
   Everything before the last dot is made accessible as the :attr:`__module__`
   attribute, and everything after the last dot is made accessible as the
   :attr:`~definition.__name__` attribute.

   If no dot is present, the entire :c:member:`~PyTypeObject.tp_name` field is made accessible as the
   :attr:`~definition.__name__` attribute, and the :attr:`__module__` attribute is undefined
   (unless explicitly set in the dictionary, as explained above).  This means your
   type will be impossible to pickle.  Additionally, it will not be listed in
   module documentations created with pydoc.

   This field is not inherited by subtypes.


.. c:member:: Py_ssize_t PyTypeObject.tp_basicsize
             Py_ssize_t PyTypeObject.tp_itemsize

   These fields allow calculating the size in bytes of instances of the type.

   There are two kinds of types: types with fixed-length instances have a zero
   :c:member:`~PyTypeObject.tp_itemsize` field, types with variable-length instances have a non-zero
   :c:member:`~PyTypeObject.tp_itemsize` field.  For a type with fixed-length instances, all
   instances have the same size, given in :c:member:`~PyTypeObject.tp_basicsize`.

   For a type with variable-length instances, the instances must have an
   :attr:`ob_size` field, and the instance size is :c:member:`~PyTypeObject.tp_basicsize` plus N
   times :c:member:`~PyTypeObject.tp_itemsize`, where N is the "length" of the object.  The value of
   N is typically stored in the instance's :attr:`ob_size` field.  There are
   exceptions:  for example, ints use a negative :attr:`ob_size` to indicate a
   negative number, and N is ``abs(ob_size)`` there.  Also, the presence of an
   :attr:`ob_size` field in the instance layout doesn't mean that the instance
   structure is variable-length (for example, the structure for the list type has
   fixed-length instances, yet those instances have a meaningful :attr:`ob_size`
   field).

   The basic size includes the fields in the instance declared by the macro
   :c:macro:`PyObject_HEAD` or :c:macro:`PyObject_VAR_HEAD` (whichever is used to
   declare the instance struct) and this in turn includes the :attr:`_ob_prev` and
   :attr:`_ob_next` fields if they are present.  This means that the only correct
   way to get an initializer for the :c:member:`~PyTypeObject.tp_basicsize` is to use the
   ``sizeof`` operator on the struct used to declare the instance layout.
   The basic size does not include the GC header size.

   These fields are inherited separately by subtypes.  If the base type has a
   non-zero :c:member:`~PyTypeObject.tp_itemsize`, it is generally not safe to set
   :c:member:`~PyTypeObject.tp_itemsize` to a different non-zero value in a subtype (though this
   depends on the implementation of the base type).

   A note about alignment: if the variable items require a particular alignment,
   this should be taken care of by the value of :c:member:`~PyTypeObject.tp_basicsize`.  Example:
   suppose a type implements an array of ``double``. :c:member:`~PyTypeObject.tp_itemsize` is
   ``sizeof(double)``. It is the programmer's responsibility that
   :c:member:`~PyTypeObject.tp_basicsize` is a multiple of ``sizeof(double)`` (assuming this is the
   alignment requirement for ``double``).


.. c:member:: destructor PyTypeObject.tp_dealloc

   A pointer to the instance destructor function.  This function must be defined
   unless the type guarantees that its instances will never be deallocated (as is
   the case for the singletons ``None`` and ``Ellipsis``).

   The destructor function is called by the :c:func:`Py_DECREF` and
   :c:func:`Py_XDECREF` macros when the new reference count is zero.  At this point,
   the instance is still in existence, but there are no references to it.  The
   destructor function should free all references which the instance owns, free all
   memory buffers owned by the instance (using the freeing function corresponding
   to the allocation function used to allocate the buffer), and finally (as its
   last action) call the type's :c:member:`~PyTypeObject.tp_free` function.  If the type is not
   subtypable (doesn't have the :const:`Py_TPFLAGS_BASETYPE` flag bit set), it is
   permissible to call the object deallocator directly instead of via
   :c:member:`~PyTypeObject.tp_free`.  The object deallocator should be the one used to allocate the
   instance; this is normally :c:func:`PyObject_Del` if the instance was allocated
   using :c:func:`PyObject_New` or :c:func:`PyObject_VarNew`, or
   :c:func:`PyObject_GC_Del` if the instance was allocated using
   :c:func:`PyObject_GC_New` or :c:func:`PyObject_GC_NewVar`.

   This field is inherited by subtypes.


.. c:member:: printfunc PyTypeObject.tp_print

   Reserved slot, formerly used for print formatting in Python 2.x.


.. c:member:: getattrfunc PyTypeObject.tp_getattr

   An optional pointer to the get-attribute-string function.

   This field is deprecated.  When it is defined, it should point to a function
   that acts the same as the :c:member:`~PyTypeObject.tp_getattro` function, but taking a C string
   instead of a Python string object to give the attribute name.  The signature is ::

      PyObject * tp_getattr(PyObject *o, char *attr_name);

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_getattro`: a subtype
   inherits both :c:member:`~PyTypeObject.tp_getattr` and :c:member:`~PyTypeObject.tp_getattro` from its base type when
   the subtype's :c:member:`~PyTypeObject.tp_getattr` and :c:member:`~PyTypeObject.tp_getattro` are both ``NULL``.


.. c:member:: setattrfunc PyTypeObject.tp_setattr

   An optional pointer to the function for setting and deleting attributes.

   This field is deprecated.  When it is defined, it should point to a function
   that acts the same as the :c:member:`~PyTypeObject.tp_setattro` function, but taking a C string
   instead of a Python string object to give the attribute name.  The signature is ::

      PyObject * tp_setattr(PyObject *o, char *attr_name, PyObject *v);

   The *v* argument is set to ``NULL`` to delete the attribute.
   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_setattro`: a subtype
   inherits both :c:member:`~PyTypeObject.tp_setattr` and :c:member:`~PyTypeObject.tp_setattro` from its base type when
   the subtype's :c:member:`~PyTypeObject.tp_setattr` and :c:member:`~PyTypeObject.tp_setattro` are both ``NULL``.


.. c:member:: PyAsyncMethods* tp_as_async

   Pointer to an additional structure that contains fields relevant only to
   objects which implement :term:`awaitable` and :term:`asynchronous iterator`
   protocols at the C-level.  See :ref:`async-structs` for details.

   .. versionadded:: 3.5
      Formerly known as ``tp_compare`` and ``tp_reserved``.


.. c:member:: reprfunc PyTypeObject.tp_repr

   .. index:: builtin: repr

   An optional pointer to a function that implements the built-in function
   :func:`repr`.

   The signature is the same as for :c:func:`PyObject_Repr`; it must return a string
   or a Unicode object.  Ideally, this function should return a string that, when
   passed to :func:`eval`, given a suitable environment, returns an object with the
   same value.  If this is not feasible, it should return a string starting with
   ``'<'`` and ending with ``'>'`` from which both the type and the value of the
   object can be deduced.

   When this field is not set, a string of the form ``<%s object at %p>`` is
   returned, where ``%s`` is replaced by the type name, and ``%p`` by the object's
   memory address.

   This field is inherited by subtypes.

.. c:member:: PyNumberMethods* tp_as_number

   Pointer to an additional structure that contains fields relevant only to
   objects which implement the number protocol.  These fields are documented in
   :ref:`number-structs`.

   The :c:member:`~PyTypeObject.tp_as_number` field is not inherited, but the contained fields are
   inherited individually.


.. c:member:: PySequenceMethods* tp_as_sequence

   Pointer to an additional structure that contains fields relevant only to
   objects which implement the sequence protocol.  These fields are documented
   in :ref:`sequence-structs`.

   The :c:member:`~PyTypeObject.tp_as_sequence` field is not inherited, but the contained fields
   are inherited individually.


.. c:member:: PyMappingMethods* tp_as_mapping

   Pointer to an additional structure that contains fields relevant only to
   objects which implement the mapping protocol.  These fields are documented in
   :ref:`mapping-structs`.

   The :c:member:`~PyTypeObject.tp_as_mapping` field is not inherited, but the contained fields
   are inherited individually.


.. c:member:: hashfunc PyTypeObject.tp_hash

   .. index:: builtin: hash

   An optional pointer to a function that implements the built-in function
   :func:`hash`.

   The signature is the same as for :c:func:`PyObject_Hash`; it must return a
   value of the type Py_hash_t.  The value ``-1`` should not be returned as a
   normal return value; when an error occurs during the computation of the hash
   value, the function should set an exception and return ``-1``.

   This field can be set explicitly to :c:func:`PyObject_HashNotImplemented` to
   block inheritance of the hash method from a parent type. This is interpreted
   as the equivalent of ``__hash__ = None`` at the Python level, causing
   ``isinstance(o, collections.Hashable)`` to correctly return ``False``. Note
   that the converse is also true - setting ``__hash__ = None`` on a class at
   the Python level will result in the ``tp_hash`` slot being set to
   :c:func:`PyObject_HashNotImplemented`.

   When this field is not set, an attempt to take the hash of the
   object raises :exc:`TypeError`.

   This field is inherited by subtypes together with
   :c:member:`~PyTypeObject.tp_richcompare`: a subtype inherits both of
   :c:member:`~PyTypeObject.tp_richcompare` and :c:member:`~PyTypeObject.tp_hash`, when the subtype's
   :c:member:`~PyTypeObject.tp_richcompare` and :c:member:`~PyTypeObject.tp_hash` are both ``NULL``.


.. c:member:: ternaryfunc PyTypeObject.tp_call

   An optional pointer to a function that implements calling the object.  This
   should be ``NULL`` if the object is not callable.  The signature is the same as
   for :c:func:`PyObject_Call`.

   This field is inherited by subtypes.


.. c:member:: reprfunc PyTypeObject.tp_str

   An optional pointer to a function that implements the built-in operation
   :func:`str`.  (Note that :class:`str` is a type now, and :func:`str` calls the
   constructor for that type.  This constructor calls :c:func:`PyObject_Str` to do
   the actual work, and :c:func:`PyObject_Str` will call this handler.)

   The signature is the same as for :c:func:`PyObject_Str`; it must return a string
   or a Unicode object.  This function should return a "friendly" string
   representation of the object, as this is the representation that will be used,
   among other things, by the :func:`print` function.

   When this field is not set, :c:func:`PyObject_Repr` is called to return a string
   representation.

   This field is inherited by subtypes.


.. c:member:: getattrofunc PyTypeObject.tp_getattro

   An optional pointer to the get-attribute function.

   The signature is the same as for :c:func:`PyObject_GetAttr`.  It is usually
   convenient to set this field to :c:func:`PyObject_GenericGetAttr`, which
   implements the normal way of looking for object attributes.

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_getattr`: a subtype
   inherits both :c:member:`~PyTypeObject.tp_getattr` and :c:member:`~PyTypeObject.tp_getattro` from its base type when
   the subtype's :c:member:`~PyTypeObject.tp_getattr` and :c:member:`~PyTypeObject.tp_getattro` are both ``NULL``.


.. c:member:: setattrofunc PyTypeObject.tp_setattro

   An optional pointer to the function for setting and deleting attributes.

   The signature is the same as for :c:func:`PyObject_SetAttr`, but setting
   *v* to ``NULL`` to delete an attribute must be supported.  It is usually
   convenient to set this field to :c:func:`PyObject_GenericSetAttr`, which
   implements the normal way of setting object attributes.

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_setattr`: a subtype
   inherits both :c:member:`~PyTypeObject.tp_setattr` and :c:member:`~PyTypeObject.tp_setattro` from its base type when
   the subtype's :c:member:`~PyTypeObject.tp_setattr` and :c:member:`~PyTypeObject.tp_setattro` are both ``NULL``.


.. c:member:: PyBufferProcs* PyTypeObject.tp_as_buffer

   Pointer to an additional structure that contains fields relevant only to objects
   which implement the buffer interface.  These fields are documented in
   :ref:`buffer-structs`.

   The :c:member:`~PyTypeObject.tp_as_buffer` field is not inherited, but the contained fields are
   inherited individually.


.. c:member:: unsigned long PyTypeObject.tp_flags

   This field is a bit mask of various flags.  Some flags indicate variant
   semantics for certain situations; others are used to indicate that certain
   fields in the type object (or in the extension structures referenced via
   :c:member:`~PyTypeObject.tp_as_number`, :c:member:`~PyTypeObject.tp_as_sequence`, :c:member:`~PyTypeObject.tp_as_mapping`, and
   :c:member:`~PyTypeObject.tp_as_buffer`) that were historically not always present are valid; if
   such a flag bit is clear, the type fields it guards must not be accessed and
   must be considered to have a zero or ``NULL`` value instead.

   Inheritance of this field is complicated.  Most flag bits are inherited
   individually, i.e. if the base type has a flag bit set, the subtype inherits
   this flag bit.  The flag bits that pertain to extension structures are strictly
   inherited if the extension structure is inherited, i.e. the base type's value of
   the flag bit is copied into the subtype together with a pointer to the extension
   structure.  The :const:`Py_TPFLAGS_HAVE_GC` flag bit is inherited together with
   the :c:member:`~PyTypeObject.tp_traverse` and :c:member:`~PyTypeObject.tp_clear` fields, i.e. if the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit is clear in the subtype and the
   :c:member:`~PyTypeObject.tp_traverse` and :c:member:`~PyTypeObject.tp_clear` fields in the subtype exist and have
   ``NULL`` values.

   The following bit masks are currently defined; these can be ORed together using
   the ``|`` operator to form the value of the :c:member:`~PyTypeObject.tp_flags` field.  The macro
   :c:func:`PyType_HasFeature` takes a type and a flags value, *tp* and *f*, and
   checks whether ``tp->tp_flags & f`` is non-zero.


   .. data:: Py_TPFLAGS_HEAPTYPE

      This bit is set when the type object itself is allocated on the heap.  In this
      case, the :attr:`ob_type` field of its instances is considered a reference to
      the type, and the type object is INCREF'ed when a new instance is created, and
      DECREF'ed when an instance is destroyed (this does not apply to instances of
      subtypes; only the type referenced by the instance's ob_type gets INCREF'ed or
      DECREF'ed).


   .. data:: Py_TPFLAGS_BASETYPE

      This bit is set when the type can be used as the base type of another type.  If
      this bit is clear, the type cannot be subtyped (similar to a "final" class in
      Java).


   .. data:: Py_TPFLAGS_READY

      This bit is set when the type object has been fully initialized by
      :c:func:`PyType_Ready`.


   .. data:: Py_TPFLAGS_READYING

      This bit is set while :c:func:`PyType_Ready` is in the process of initializing
      the type object.


   .. data:: Py_TPFLAGS_HAVE_GC

      This bit is set when the object supports garbage collection.  If this bit
      is set, instances must be created using :c:func:`PyObject_GC_New` and
      destroyed using :c:func:`PyObject_GC_Del`.  More information in section
      :ref:`supporting-cycle-detection`.  This bit also implies that the
      GC-related fields :c:member:`~PyTypeObject.tp_traverse` and :c:member:`~PyTypeObject.tp_clear` are present in
      the type object.


   .. data:: Py_TPFLAGS_DEFAULT

      This is a bitmask of all the bits that pertain to the existence of certain
      fields in the type object and its extension structures. Currently, it includes
      the following bits: :const:`Py_TPFLAGS_HAVE_STACKLESS_EXTENSION`,
      :const:`Py_TPFLAGS_HAVE_VERSION_TAG`.


   .. data:: Py_TPFLAGS_LONG_SUBCLASS
   .. data:: Py_TPFLAGS_LIST_SUBCLASS
   .. data:: Py_TPFLAGS_TUPLE_SUBCLASS
   .. data:: Py_TPFLAGS_BYTES_SUBCLASS
   .. data:: Py_TPFLAGS_UNICODE_SUBCLASS
   .. data:: Py_TPFLAGS_DICT_SUBCLASS
   .. data:: Py_TPFLAGS_BASE_EXC_SUBCLASS
   .. data:: Py_TPFLAGS_TYPE_SUBCLASS

      These flags are used by functions such as
      :c:func:`PyLong_Check` to quickly determine if a type is a subclass
      of a built-in type; such specific checks are faster than a generic
      check, like :c:func:`PyObject_IsInstance`. Custom types that inherit
      from built-ins should have their :c:member:`~PyTypeObject.tp_flags`
      set appropriately, or the code that interacts with such types
      will behave differently depending on what kind of check is used.


   .. data:: Py_TPFLAGS_HAVE_FINALIZE

      This bit is set when the :c:member:`~PyTypeObject.tp_finalize` slot is present in the
      type structure.

      .. versionadded:: 3.4


.. c:member:: const char* PyTypeObject.tp_doc

   An optional pointer to a NUL-terminated C string giving the docstring for this
   type object.  This is exposed as the :attr:`__doc__` attribute on the type and
   instances of the type.

   This field is *not* inherited by subtypes.


.. c:member:: traverseproc PyTypeObject.tp_traverse

   An optional pointer to a traversal function for the garbage collector.  This is
   only used if the :const:`Py_TPFLAGS_HAVE_GC` flag bit is set.  More information
   about Python's garbage collection scheme can be found in section
   :ref:`supporting-cycle-detection`.

   The :c:member:`~PyTypeObject.tp_traverse` pointer is used by the garbage collector to detect
   reference cycles. A typical implementation of a :c:member:`~PyTypeObject.tp_traverse` function
   simply calls :c:func:`Py_VISIT` on each of the instance's members that are Python
   objects.  For example, this is function :c:func:`local_traverse` from the
   :mod:`_thread` extension module::

      static int
      local_traverse(localobject *self, visitproc visit, void *arg)
      {
          Py_VISIT(self->args);
          Py_VISIT(self->kw);
          Py_VISIT(self->dict);
          return 0;
      }

   Note that :c:func:`Py_VISIT` is called only on those members that can participate
   in reference cycles.  Although there is also a ``self->key`` member, it can only
   be ``NULL`` or a Python string and therefore cannot be part of a reference cycle.

   On the other hand, even if you know a member can never be part of a cycle, as a
   debugging aid you may want to visit it anyway just so the :mod:`gc` module's
   :func:`~gc.get_referents` function will include it.

   Note that :c:func:`Py_VISIT` requires the *visit* and *arg* parameters to
   :c:func:`local_traverse` to have these specific names; don't name them just
   anything.

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_clear` and the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit: the flag bit, :c:member:`~PyTypeObject.tp_traverse`, and
   :c:member:`~PyTypeObject.tp_clear` are all inherited from the base type if they are all zero in
   the subtype.


.. c:member:: inquiry PyTypeObject.tp_clear

   An optional pointer to a clear function for the garbage collector. This is only
   used if the :const:`Py_TPFLAGS_HAVE_GC` flag bit is set.

   The :c:member:`~PyTypeObject.tp_clear` member function is used to break reference cycles in cyclic
   garbage detected by the garbage collector.  Taken together, all :c:member:`~PyTypeObject.tp_clear`
   functions in the system must combine to break all reference cycles.  This is
   subtle, and if in any doubt supply a :c:member:`~PyTypeObject.tp_clear` function.  For example,
   the tuple type does not implement a :c:member:`~PyTypeObject.tp_clear` function, because it's
   possible to prove that no reference cycle can be composed entirely of tuples.
   Therefore the :c:member:`~PyTypeObject.tp_clear` functions of other types must be sufficient to
   break any cycle containing a tuple.  This isn't immediately obvious, and there's
   rarely a good reason to avoid implementing :c:member:`~PyTypeObject.tp_clear`.

   Implementations of :c:member:`~PyTypeObject.tp_clear` should drop the instance's references to
   those of its members that may be Python objects, and set its pointers to those
   members to ``NULL``, as in the following example::

      static int
      local_clear(localobject *self)
      {
          Py_CLEAR(self->key);
          Py_CLEAR(self->args);
          Py_CLEAR(self->kw);
          Py_CLEAR(self->dict);
          return 0;
      }

   The :c:func:`Py_CLEAR` macro should be used, because clearing references is
   delicate:  the reference to the contained object must not be decremented until
   after the pointer to the contained object is set to ``NULL``.  This is because
   decrementing the reference count may cause the contained object to become trash,
   triggering a chain of reclamation activity that may include invoking arbitrary
   Python code (due to finalizers, or weakref callbacks, associated with the
   contained object). If it's possible for such code to reference *self* again,
   it's important that the pointer to the contained object be ``NULL`` at that time,
   so that *self* knows the contained object can no longer be used.  The
   :c:func:`Py_CLEAR` macro performs the operations in a safe order.

   Because the goal of :c:member:`~PyTypeObject.tp_clear` functions is to break reference cycles,
   it's not necessary to clear contained objects like Python strings or Python
   integers, which can't participate in reference cycles. On the other hand, it may
   be convenient to clear all contained Python objects, and write the type's
   :c:member:`~PyTypeObject.tp_dealloc` function to invoke :c:member:`~PyTypeObject.tp_clear`.

   More information about Python's garbage collection scheme can be found in
   section :ref:`supporting-cycle-detection`.

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_traverse` and the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit: the flag bit, :c:member:`~PyTypeObject.tp_traverse`, and
   :c:member:`~PyTypeObject.tp_clear` are all inherited from the base type if they are all zero in
   the subtype.


.. c:member:: richcmpfunc PyTypeObject.tp_richcompare

   An optional pointer to the rich comparison function, whose signature is
   ``PyObject *tp_richcompare(PyObject *a, PyObject *b, int op)``. The first
   parameter is guaranteed to be an instance of the type that is defined
   by :c:type:`PyTypeObject`.

   The function should return the result of the comparison (usually ``Py_True``
   or ``Py_False``).  If the comparison is undefined, it must return
   ``Py_NotImplemented``, if another error occurred it must return ``NULL`` and
   set an exception condition.

   .. note::

      If you want to implement a type for which only a limited set of
      comparisons makes sense (e.g. ``==`` and ``!=``, but not ``<`` and
      friends), directly raise :exc:`TypeError` in the rich comparison function.

   This field is inherited by subtypes together with :c:member:`~PyTypeObject.tp_hash`:
   a subtype inherits :c:member:`~PyTypeObject.tp_richcompare` and :c:member:`~PyTypeObject.tp_hash` when
   the subtype's :c:member:`~PyTypeObject.tp_richcompare` and :c:member:`~PyTypeObject.tp_hash` are both
   ``NULL``.

   The following constants are defined to be used as the third argument for
   :c:member:`~PyTypeObject.tp_richcompare` and for :c:func:`PyObject_RichCompare`:

   +----------------+------------+
   | Constant       | Comparison |
   +================+============+
   | :const:`Py_LT` | ``<``      |
   +----------------+------------+
   | :const:`Py_LE` | ``<=``     |
   +----------------+------------+
   | :const:`Py_EQ` | ``==``     |
   +----------------+------------+
   | :const:`Py_NE` | ``!=``     |
   +----------------+------------+
   | :const:`Py_GT` | ``>``      |
   +----------------+------------+
   | :const:`Py_GE` | ``>=``     |
   +----------------+------------+

   The following macro is defined to ease writing rich comparison functions:

   .. c:function:: PyObject *Py_RETURN_RICHCOMPARE(VAL_A, VAL_B, int op)

      Return ``Py_True`` or ``Py_False`` from the function, depending on the
      result of a comparison.
      VAL_A and VAL_B must be orderable by C comparison operators (for example,
      they may be C ints or floats). The third argument specifies the requested
      operation, as for :c:func:`PyObject_RichCompare`.

      The return value's reference count is properly incremented.

      On error, sets an exception and returns NULL from the function.

      .. versionadded:: 3.7


.. c:member:: Py_ssize_t PyTypeObject.tp_weaklistoffset

   If the instances of this type are weakly referenceable, this field is greater
   than zero and contains the offset in the instance structure of the weak
   reference list head (ignoring the GC header, if present); this offset is used by
   :c:func:`PyObject_ClearWeakRefs` and the :c:func:`PyWeakref_\*` functions.  The
   instance structure needs to include a field of type :c:type:`PyObject\*` which is
   initialized to ``NULL``.

   Do not confuse this field with :c:member:`~PyTypeObject.tp_weaklist`; that is the list head for
   weak references to the type object itself.

   This field is inherited by subtypes, but see the rules listed below. A subtype
   may override this offset; this means that the subtype uses a different weak
   reference list head than the base type.  Since the list head is always found via
   :c:member:`~PyTypeObject.tp_weaklistoffset`, this should not be a problem.

   When a type defined by a class statement has no :attr:`~object.__slots__` declaration,
   and none of its base types are weakly referenceable, the type is made weakly
   referenceable by adding a weak reference list head slot to the instance layout
   and setting the :c:member:`~PyTypeObject.tp_weaklistoffset` of that slot's offset.

   When a type's :attr:`__slots__` declaration contains a slot named
   :attr:`__weakref__`, that slot becomes the weak reference list head for
   instances of the type, and the slot's offset is stored in the type's
   :c:member:`~PyTypeObject.tp_weaklistoffset`.

   When a type's :attr:`__slots__` declaration does not contain a slot named
   :attr:`__weakref__`, the type inherits its :c:member:`~PyTypeObject.tp_weaklistoffset` from its
   base type.

.. c:member:: getiterfunc PyTypeObject.tp_iter

   An optional pointer to a function that returns an iterator for the object.  Its
   presence normally signals that the instances of this type are iterable (although
   sequences may be iterable without this function).

   This function has the same signature as :c:func:`PyObject_GetIter`.

   This field is inherited by subtypes.


.. c:member:: iternextfunc PyTypeObject.tp_iternext

   An optional pointer to a function that returns the next item in an iterator.
   When the iterator is exhausted, it must return ``NULL``; a :exc:`StopIteration`
   exception may or may not be set.  When another error occurs, it must return
   ``NULL`` too.  Its presence signals that the instances of this type are
   iterators.

   Iterator types should also define the :c:member:`~PyTypeObject.tp_iter` function, and that
   function should return the iterator instance itself (not a new iterator
   instance).

   This function has the same signature as :c:func:`PyIter_Next`.

   This field is inherited by subtypes.


.. c:member:: struct PyMethodDef* PyTypeObject.tp_methods

   An optional pointer to a static ``NULL``-terminated array of :c:type:`PyMethodDef`
   structures, declaring regular methods of this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :c:member:`~PyTypeObject.tp_dict` below) containing a method descriptor.

   This field is not inherited by subtypes (methods are inherited through a
   different mechanism).


.. c:member:: struct PyMemberDef* PyTypeObject.tp_members

   An optional pointer to a static ``NULL``-terminated array of :c:type:`PyMemberDef`
   structures, declaring regular data members (fields or slots) of instances of
   this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :c:member:`~PyTypeObject.tp_dict` below) containing a member descriptor.

   This field is not inherited by subtypes (members are inherited through a
   different mechanism).


.. c:member:: struct PyGetSetDef* PyTypeObject.tp_getset

   An optional pointer to a static ``NULL``-terminated array of :c:type:`PyGetSetDef`
   structures, declaring computed attributes of instances of this type.

   For each entry in the array, an entry is added to the type's dictionary (see
   :c:member:`~PyTypeObject.tp_dict` below) containing a getset descriptor.

   This field is not inherited by subtypes (computed attributes are inherited
   through a different mechanism).


.. c:member:: PyTypeObject* PyTypeObject.tp_base

   An optional pointer to a base type from which type properties are inherited.  At
   this level, only single inheritance is supported; multiple inheritance require
   dynamically creating a type object by calling the metatype.

   This field is not inherited by subtypes (obviously), but it defaults to
   ``&PyBaseObject_Type`` (which to Python programmers is known as the type
   :class:`object`).


.. c:member:: PyObject* PyTypeObject.tp_dict

   The type's dictionary is stored here by :c:func:`PyType_Ready`.

   This field should normally be initialized to ``NULL`` before PyType_Ready is
   called; it may also be initialized to a dictionary containing initial attributes
   for the type.  Once :c:func:`PyType_Ready` has initialized the type, extra
   attributes for the type may be added to this dictionary only if they don't
   correspond to overloaded operations (like :meth:`__add__`).

   This field is not inherited by subtypes (though the attributes defined in here
   are inherited through a different mechanism).

   .. warning::

      It is not safe to use :c:func:`PyDict_SetItem` on or otherwise modify
      :c:member:`~PyTypeObject.tp_dict` with the dictionary C-API.


.. c:member:: descrgetfunc PyTypeObject.tp_descr_get

   An optional pointer to a "descriptor get" function.

   The function signature is ::

      PyObject * tp_descr_get(PyObject *self, PyObject *obj, PyObject *type);

   .. XXX explain.

   This field is inherited by subtypes.


.. c:member:: descrsetfunc PyTypeObject.tp_descr_set

   An optional pointer to a function for setting and deleting
   a descriptor's value.

   The function signature is ::

      int tp_descr_set(PyObject *self, PyObject *obj, PyObject *value);

   The *value* argument is set to ``NULL`` to delete the value.
   This field is inherited by subtypes.

   .. XXX explain.


.. c:member:: Py_ssize_t PyTypeObject.tp_dictoffset

   If the instances of this type have a dictionary containing instance variables,
   this field is non-zero and contains the offset in the instances of the type of
   the instance variable dictionary; this offset is used by
   :c:func:`PyObject_GenericGetAttr`.

   Do not confuse this field with :c:member:`~PyTypeObject.tp_dict`; that is the dictionary for
   attributes of the type object itself.

   If the value of this field is greater than zero, it specifies the offset from
   the start of the instance structure.  If the value is less than zero, it
   specifies the offset from the *end* of the instance structure.  A negative
   offset is more expensive to use, and should only be used when the instance
   structure contains a variable-length part.  This is used for example to add an
   instance variable dictionary to subtypes of :class:`str` or :class:`tuple`. Note
   that the :c:member:`~PyTypeObject.tp_basicsize` field should account for the dictionary added to
   the end in that case, even though the dictionary is not included in the basic
   object layout.  On a system with a pointer size of 4 bytes,
   :c:member:`~PyTypeObject.tp_dictoffset` should be set to ``-4`` to indicate that the dictionary is
   at the very end of the structure.

   The real dictionary offset in an instance can be computed from a negative
   :c:member:`~PyTypeObject.tp_dictoffset` as follows::

      dictoffset = tp_basicsize + abs(ob_size)*tp_itemsize + tp_dictoffset
      if dictoffset is not aligned on sizeof(void*):
          round up to sizeof(void*)

   where :c:member:`~PyTypeObject.tp_basicsize`, :c:member:`~PyTypeObject.tp_itemsize` and :c:member:`~PyTypeObject.tp_dictoffset` are
   taken from the type object, and :attr:`ob_size` is taken from the instance.  The
   absolute value is taken because ints use the sign of :attr:`ob_size` to
   store the sign of the number.  (There's never a need to do this calculation
   yourself; it is done for you by :c:func:`_PyObject_GetDictPtr`.)

   This field is inherited by subtypes, but see the rules listed below. A subtype
   may override this offset; this means that the subtype instances store the
   dictionary at a difference offset than the base type.  Since the dictionary is
   always found via :c:member:`~PyTypeObject.tp_dictoffset`, this should not be a problem.

   When a type defined by a class statement has no :attr:`~object.__slots__` declaration,
   and none of its base types has an instance variable dictionary, a dictionary
   slot is added to the instance layout and the :c:member:`~PyTypeObject.tp_dictoffset` is set to
   that slot's offset.

   When a type defined by a class statement has a :attr:`__slots__` declaration,
   the type inherits its :c:member:`~PyTypeObject.tp_dictoffset` from its base type.

   (Adding a slot named :attr:`~object.__dict__` to the :attr:`__slots__` declaration does
   not have the expected effect, it just causes confusion.  Maybe this should be
   added as a feature just like :attr:`__weakref__` though.)


.. c:member:: initproc PyTypeObject.tp_init

   An optional pointer to an instance initialization function.

   This function corresponds to the :meth:`__init__` method of classes.  Like
   :meth:`__init__`, it is possible to create an instance without calling
   :meth:`__init__`, and it is possible to reinitialize an instance by calling its
   :meth:`__init__` method again.

   The function signature is ::

      int tp_init(PyObject *self, PyObject *args, PyObject *kwds)

   The self argument is the instance to be initialized; the *args* and *kwds*
   arguments represent positional and keyword arguments of the call to
   :meth:`__init__`.

   The :c:member:`~PyTypeObject.tp_init` function, if not ``NULL``, is called when an instance is
   created normally by calling its type, after the type's :c:member:`~PyTypeObject.tp_new` function
   has returned an instance of the type.  If the :c:member:`~PyTypeObject.tp_new` function returns an
   instance of some other type that is not a subtype of the original type, no
   :c:member:`~PyTypeObject.tp_init` function is called; if :c:member:`~PyTypeObject.tp_new` returns an instance of a
   subtype of the original type, the subtype's :c:member:`~PyTypeObject.tp_init` is called.

   This field is inherited by subtypes.


.. c:member:: allocfunc PyTypeObject.tp_alloc

   An optional pointer to an instance allocation function.

   The function signature is ::

      PyObject *tp_alloc(PyTypeObject *self, Py_ssize_t nitems)

   The purpose of this function is to separate memory allocation from memory
   initialization.  It should return a pointer to a block of memory of adequate
   length for the instance, suitably aligned, and initialized to zeros, but with
   :attr:`ob_refcnt` set to ``1`` and :attr:`ob_type` set to the type argument.  If
   the type's :c:member:`~PyTypeObject.tp_itemsize` is non-zero, the object's :attr:`ob_size` field
   should be initialized to *nitems* and the length of the allocated memory block
   should be ``tp_basicsize + nitems*tp_itemsize``, rounded up to a multiple of
   ``sizeof(void*)``; otherwise, *nitems* is not used and the length of the block
   should be :c:member:`~PyTypeObject.tp_basicsize`.

   Do not use this function to do any other instance initialization, not even to
   allocate additional memory; that should be done by :c:member:`~PyTypeObject.tp_new`.

   This field is inherited by static subtypes, but not by dynamic subtypes
   (subtypes created by a class statement); in the latter, this field is always set
   to :c:func:`PyType_GenericAlloc`, to force a standard heap allocation strategy.
   That is also the recommended value for statically defined types.


.. c:member:: newfunc PyTypeObject.tp_new

   An optional pointer to an instance creation function.

   If this function is ``NULL`` for a particular type, that type cannot be called to
   create new instances; presumably there is some other way to create instances,
   like a factory function.

   The function signature is ::

      PyObject *tp_new(PyTypeObject *subtype, PyObject *args, PyObject *kwds)

   The subtype argument is the type of the object being created; the *args* and
   *kwds* arguments represent positional and keyword arguments of the call to the
   type.  Note that subtype doesn't have to equal the type whose :c:member:`~PyTypeObject.tp_new`
   function is called; it may be a subtype of that type (but not an unrelated
   type).

   The :c:member:`~PyTypeObject.tp_new` function should call ``subtype->tp_alloc(subtype, nitems)``
   to allocate space for the object, and then do only as much further
   initialization as is absolutely necessary.  Initialization that can safely be
   ignored or repeated should be placed in the :c:member:`~PyTypeObject.tp_init` handler.  A good
   rule of thumb is that for immutable types, all initialization should take place
   in :c:member:`~PyTypeObject.tp_new`, while for mutable types, most initialization should be
   deferred to :c:member:`~PyTypeObject.tp_init`.

   This field is inherited by subtypes, except it is not inherited by static types
   whose :c:member:`~PyTypeObject.tp_base` is ``NULL`` or ``&PyBaseObject_Type``.


.. c:member:: destructor PyTypeObject.tp_free

   An optional pointer to an instance deallocation function.  Its signature is
   :c:type:`freefunc`::

      void tp_free(void *)

   An initializer that is compatible with this signature is :c:func:`PyObject_Free`.

   This field is inherited by static subtypes, but not by dynamic subtypes
   (subtypes created by a class statement); in the latter, this field is set to a
   deallocator suitable to match :c:func:`PyType_GenericAlloc` and the value of the
   :const:`Py_TPFLAGS_HAVE_GC` flag bit.


.. c:member:: inquiry PyTypeObject.tp_is_gc

   An optional pointer to a function called by the garbage collector.

   The garbage collector needs to know whether a particular object is collectible
   or not.  Normally, it is sufficient to look at the object's type's
   :c:member:`~PyTypeObject.tp_flags` field, and check the :const:`Py_TPFLAGS_HAVE_GC` flag bit.  But
   some types have a mixture of statically and dynamically allocated instances, and
   the statically allocated instances are not collectible.  Such types should
   define this function; it should return ``1`` for a collectible instance, and
   ``0`` for a non-collectible instance. The signature is ::

      int tp_is_gc(PyObject *self)

   (The only example of this are types themselves.  The metatype,
   :c:data:`PyType_Type`, defines this function to distinguish between statically
   and dynamically allocated types.)

   This field is inherited by subtypes.


.. c:member:: PyObject* PyTypeObject.tp_bases

   Tuple of base types.

   This is set for types created by a class statement.  It should be ``NULL`` for
   statically defined types.

   This field is not inherited.


.. c:member:: PyObject* PyTypeObject.tp_mro

   Tuple containing the expanded set of base types, starting with the type itself
   and ending with :class:`object`, in Method Resolution Order.

   This field is not inherited; it is calculated fresh by :c:func:`PyType_Ready`.


.. c:member:: destructor PyTypeObject.tp_finalize

   An optional pointer to an instance finalization function.  Its signature is
   :c:type:`destructor`::

      void tp_finalize(PyObject *)

   If :c:member:`~PyTypeObject.tp_finalize` is set, the interpreter calls it once when
   finalizing an instance.  It is called either from the garbage
   collector (if the instance is part of an isolated reference cycle) or
   just before the object is deallocated.  Either way, it is guaranteed
   to be called before attempting to break reference cycles, ensuring
   that it finds the object in a sane state.

   :c:member:`~PyTypeObject.tp_finalize` should not mutate the current exception status;
   therefore, a recommended way to write a non-trivial finalizer is::

      static void
      local_finalize(PyObject *self)
      {
          PyObject *error_type, *error_value, *error_traceback;

          /* Save the current exception, if any. */
          PyErr_Fetch(&error_type, &error_value, &error_traceback);

          /* ... */

          /* Restore the saved exception. */
          PyErr_Restore(error_type, error_value, error_traceback);
      }

   For this field to be taken into account (even through inheritance),
   you must also set the :const:`Py_TPFLAGS_HAVE_FINALIZE` flags bit.

   This field is inherited by subtypes.

   .. versionadded:: 3.4

   .. seealso:: "Safe object finalization" (:pep:`442`)


.. c:member:: PyObject* PyTypeObject.tp_cache

   Unused.  Not inherited.  Internal use only.


.. c:member:: PyObject* PyTypeObject.tp_subclasses

   List of weak references to subclasses.  Not inherited.  Internal use only.


.. c:member:: PyObject* PyTypeObject.tp_weaklist

   Weak reference list head, for weak references to this type object.  Not
   inherited.  Internal use only.

The remaining fields are only defined if the feature test macro
:const:`COUNT_ALLOCS` is defined, and are for internal use only. They are
documented here for completeness.  None of these fields are inherited by
subtypes.


.. c:member:: Py_ssize_t PyTypeObject.tp_allocs

   Number of allocations.


.. c:member:: Py_ssize_t PyTypeObject.tp_frees

   Number of frees.


.. c:member:: Py_ssize_t PyTypeObject.tp_maxalloc

   Maximum simultaneously allocated objects.


.. c:member:: PyTypeObject* PyTypeObject.tp_next

   Pointer to the next type object with a non-zero :c:member:`~PyTypeObject.tp_allocs` field.

Also, note that, in a garbage collected Python, tp_dealloc may be called from
any Python thread, not just the thread which created the object (if the object
becomes part of a refcount cycle, that cycle might be collected by a garbage
collection on any thread).  This is not a problem for Python API calls, since
the thread on which tp_dealloc is called will own the Global Interpreter Lock
(GIL). However, if the object being destroyed in turn destroys objects from some
other C or C++ library, care should be taken to ensure that destroying those
objects on the thread which called tp_dealloc will not violate any assumptions
of the library.


.. _number-structs:

Number Object Structures
========================

.. sectionauthor:: Amaury Forgeot d'Arc


.. c:type:: PyNumberMethods

   This structure holds pointers to the functions which an object uses to
   implement the number protocol.  Each function is used by the function of
   similar name documented in the :ref:`number` section.

   Here is the structure definition::

       typedef struct {
            binaryfunc nb_add;
            binaryfunc nb_subtract;
            binaryfunc nb_multiply;
            binaryfunc nb_remainder;
            binaryfunc nb_divmod;
            ternaryfunc nb_power;
            unaryfunc nb_negative;
            unaryfunc nb_positive;
            unaryfunc nb_absolute;
            inquiry nb_bool;
            unaryfunc nb_invert;
            binaryfunc nb_lshift;
            binaryfunc nb_rshift;
            binaryfunc nb_and;
            binaryfunc nb_xor;
            binaryfunc nb_or;
            unaryfunc nb_int;
            void *nb_reserved;
            unaryfunc nb_float;

            binaryfunc nb_inplace_add;
            binaryfunc nb_inplace_subtract;
            binaryfunc nb_inplace_multiply;
            binaryfunc nb_inplace_remainder;
            ternaryfunc nb_inplace_power;
            binaryfunc nb_inplace_lshift;
            binaryfunc nb_inplace_rshift;
            binaryfunc nb_inplace_and;
            binaryfunc nb_inplace_xor;
            binaryfunc nb_inplace_or;

            binaryfunc nb_floor_divide;
            binaryfunc nb_true_divide;
            binaryfunc nb_inplace_floor_divide;
            binaryfunc nb_inplace_true_divide;

            unaryfunc nb_index;

            binaryfunc nb_matrix_multiply;
            binaryfunc nb_inplace_matrix_multiply;
       } PyNumberMethods;

   .. note::

      Binary and ternary functions must check the type of all their operands,
      and implement the necessary conversions (at least one of the operands is
      an instance of the defined type).  If the operation is not defined for the
      given operands, binary and ternary functions must return
      ``Py_NotImplemented``, if another error occurred they must return ``NULL``
      and set an exception.

   .. note::

      The :c:data:`nb_reserved` field should always be ``NULL``.  It
      was previously called :c:data:`nb_long`, and was renamed in
      Python 3.0.1.


.. _mapping-structs:

Mapping Object Structures
=========================

.. sectionauthor:: Amaury Forgeot d'Arc


.. c:type:: PyMappingMethods

   This structure holds pointers to the functions which an object uses to
   implement the mapping protocol.  It has three members:

.. c:member:: lenfunc PyMappingMethods.mp_length

   This function is used by :c:func:`PyMapping_Size` and
   :c:func:`PyObject_Size`, and has the same signature.  This slot may be set to
   ``NULL`` if the object has no defined length.

.. c:member:: binaryfunc PyMappingMethods.mp_subscript

   This function is used by :c:func:`PyObject_GetItem` and
   :c:func:`PySequence_GetSlice`, and has the same signature as
   :c:func:`!PyObject_GetItem`.  This slot must be filled for the
   :c:func:`PyMapping_Check` function to return ``1``, it can be ``NULL``
   otherwise.

.. c:member:: objobjargproc PyMappingMethods.mp_ass_subscript

   This function is used by :c:func:`PyObject_SetItem`,
   :c:func:`PyObject_DelItem`, :c:func:`PyObject_SetSlice` and
   :c:func:`PyObject_DelSlice`.  It has the same signature as
   :c:func:`!PyObject_SetItem`, but *v* can also be set to ``NULL`` to delete
   an item.  If this slot is ``NULL``, the object does not support item
   assignment and deletion.


.. _sequence-structs:

Sequence Object Structures
==========================

.. sectionauthor:: Amaury Forgeot d'Arc


.. c:type:: PySequenceMethods

   This structure holds pointers to the functions which an object uses to
   implement the sequence protocol.

.. c:member:: lenfunc PySequenceMethods.sq_length

   This function is used by :c:func:`PySequence_Size` and
   :c:func:`PyObject_Size`, and has the same signature.  It is also used for
   handling negative indices via the :c:member:`~PySequenceMethods.sq_item`
   and the :c:member:`~PySequenceMethods.sq_ass_item` slots.

.. c:member:: binaryfunc PySequenceMethods.sq_concat

   This function is used by :c:func:`PySequence_Concat` and has the same
   signature.  It is also used by the ``+`` operator, after trying the numeric
   addition via the :c:member:`~PyNumberMethods.nb_add` slot.

.. c:member:: ssizeargfunc PySequenceMethods.sq_repeat

   This function is used by :c:func:`PySequence_Repeat` and has the same
   signature.  It is also used by the ``*`` operator, after trying numeric
   multiplication via the :c:member:`~PyNumberMethods.nb_multiply` slot.

.. c:member:: ssizeargfunc PySequenceMethods.sq_item

   This function is used by :c:func:`PySequence_GetItem` and has the same
   signature.  It is also used by :c:func:`PyObject_GetItem`, after trying
   the subscription via the :c:member:`~PyMappingMethods.mp_subscript` slot.
   This slot must be filled for the :c:func:`PySequence_Check`
   function to return ``1``, it can be ``NULL`` otherwise.

   Negative indexes are handled as follows: if the :attr:`sq_length` slot is
   filled, it is called and the sequence length is used to compute a positive
   index which is passed to :attr:`sq_item`.  If :attr:`sq_length` is ``NULL``,
   the index is passed as is to the function.

.. c:member:: ssizeobjargproc PySequenceMethods.sq_ass_item

   This function is used by :c:func:`PySequence_SetItem` and has the same
   signature.  It is also used by :c:func:`PyObject_SetItem` and
   :c:func:`PyObject_DelItem`, after trying the item assignment and deletion
   via the :c:member:`~PyMappingMethods.mp_ass_subscript` slot.
   This slot may be left to ``NULL`` if the object does not support
   item assignment and deletion.

.. c:member:: objobjproc PySequenceMethods.sq_contains

   This function may be used by :c:func:`PySequence_Contains` and has the same
   signature.  This slot may be left to ``NULL``, in this case
   :c:func:`!PySequence_Contains` simply traverses the sequence until it
   finds a match.

.. c:member:: binaryfunc PySequenceMethods.sq_inplace_concat

   This function is used by :c:func:`PySequence_InPlaceConcat` and has the same
   signature.  It should modify its first operand, and return it.  This slot
   may be left to ``NULL``, in this case :c:func:`!PySequence_InPlaceConcat`
   will fall back to :c:func:`PySequence_Concat`.  It is also used by the
   augmented assignment ``+=``, after trying numeric in-place addition
   via the :c:member:`~PyNumberMethods.nb_inplace_add` slot.

.. c:member:: ssizeargfunc PySequenceMethods.sq_inplace_repeat

   This function is used by :c:func:`PySequence_InPlaceRepeat` and has the same
   signature.  It should modify its first operand, and return it.  This slot
   may be left to ``NULL``, in this case :c:func:`!PySequence_InPlaceRepeat`
   will fall back to :c:func:`PySequence_Repeat`.  It is also used by the
   augmented assignment ``*=``, after trying numeric in-place multiplication
   via the :c:member:`~PyNumberMethods.nb_inplace_multiply` slot.


.. _buffer-structs:

Buffer Object Structures
========================

.. sectionauthor:: Greg J. Stein <greg@lyra.org>
.. sectionauthor:: Benjamin Peterson
.. sectionauthor:: Stefan Krah

.. c:type:: PyBufferProcs

   This structure holds pointers to the functions required by the
   :ref:`Buffer protocol <bufferobjects>`. The protocol defines how
   an exporter object can expose its internal data to consumer objects.

.. c:member:: getbufferproc PyBufferProcs.bf_getbuffer

   The signature of this function is::

       int (PyObject *exporter, Py_buffer *view, int flags);

   Handle a request to *exporter* to fill in *view* as specified by *flags*.
   Except for point (3), an implementation of this function MUST take these
   steps:

   (1) Check if the request can be met. If not, raise :c:data:`PyExc_BufferError`,
       set :c:data:`view->obj` to ``NULL`` and return ``-1``.

   (2) Fill in the requested fields.

   (3) Increment an internal counter for the number of exports.

   (4) Set :c:data:`view->obj` to *exporter* and increment :c:data:`view->obj`.

   (5) Return ``0``.

   If *exporter* is part of a chain or tree of buffer providers, two main
   schemes can be used:

   * Re-export: Each member of the tree acts as the exporting object and
     sets :c:data:`view->obj` to a new reference to itself.

   * Redirect: The buffer request is redirected to the root object of the
     tree. Here, :c:data:`view->obj` will be a new reference to the root
     object.

   The individual fields of *view* are described in section
   :ref:`Buffer structure <buffer-structure>`, the rules how an exporter
   must react to specific requests are in section
   :ref:`Buffer request types <buffer-request-types>`.

   All memory pointed to in the :c:type:`Py_buffer` structure belongs to
   the exporter and must remain valid until there are no consumers left.
   :c:member:`~Py_buffer.format`, :c:member:`~Py_buffer.shape`,
   :c:member:`~Py_buffer.strides`, :c:member:`~Py_buffer.suboffsets`
   and :c:member:`~Py_buffer.internal`
   are read-only for the consumer.

   :c:func:`PyBuffer_FillInfo` provides an easy way of exposing a simple
   bytes buffer while dealing correctly with all request types.

   :c:func:`PyObject_GetBuffer` is the interface for the consumer that
   wraps this function.

.. c:member:: releasebufferproc PyBufferProcs.bf_releasebuffer

   The signature of this function is::

       void (PyObject *exporter, Py_buffer *view);

   Handle a request to release the resources of the buffer. If no resources
   need to be released, :c:member:`PyBufferProcs.bf_releasebuffer` may be
   ``NULL``. Otherwise, a standard implementation of this function will take
   these optional steps:

   (1) Decrement an internal counter for the number of exports.

   (2) If the counter is ``0``, free all memory associated with *view*.

   The exporter MUST use the :c:member:`~Py_buffer.internal` field to keep
   track of buffer-specific resources. This field is guaranteed to remain
   constant, while a consumer MAY pass a copy of the original buffer as the
   *view* argument.


   This function MUST NOT decrement :c:data:`view->obj`, since that is
   done automatically in :c:func:`PyBuffer_Release` (this scheme is
   useful for breaking reference cycles).


   :c:func:`PyBuffer_Release` is the interface for the consumer that
   wraps this function.


.. _async-structs:


Async Object Structures
=======================

.. sectionauthor:: Yury Selivanov <yselivanov@sprymix.com>

.. versionadded:: 3.5

.. c:type:: PyAsyncMethods

   This structure holds pointers to the functions required to implement
   :term:`awaitable` and :term:`asynchronous iterator` objects.

   Here is the structure definition::

        typedef struct {
            unaryfunc am_await;
            unaryfunc am_aiter;
            unaryfunc am_anext;
        } PyAsyncMethods;

.. c:member:: unaryfunc PyAsyncMethods.am_await

   The signature of this function is::

      PyObject *am_await(PyObject *self)

   The returned object must be an iterator, i.e. :c:func:`PyIter_Check` must
   return ``1`` for it.

   This slot may be set to ``NULL`` if an object is not an :term:`awaitable`.

.. c:member:: unaryfunc PyAsyncMethods.am_aiter

   The signature of this function is::

      PyObject *am_aiter(PyObject *self)

   Must return an :term:`awaitable` object.  See :meth:`__anext__` for details.

   This slot may be set to ``NULL`` if an object does not implement
   asynchronous iteration protocol.

.. c:member:: unaryfunc PyAsyncMethods.am_anext

   The signature of this function is::

      PyObject *am_anext(PyObject *self)

   Must return an :term:`awaitable` object.  See :meth:`__anext__` for details.
   This slot may be set to ``NULL``.
