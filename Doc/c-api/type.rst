.. highlight:: c

.. _typeobjects:

Type Objects
============

.. index:: pair: object; type


.. c:type:: PyTypeObject

   The C structure of the objects used to describe built-in types.


.. c:var:: PyTypeObject PyType_Type

   This is the type object for type objects; it is the same object as
   :class:`type` in the Python layer.


.. c:function:: int PyType_Check(PyObject *o)

   Return non-zero if the object *o* is a type object, including instances of
   types derived from the standard type object.  Return 0 in all other cases.
   This function always succeeds.


.. c:function:: int PyType_CheckExact(PyObject *o)

   Return non-zero if the object *o* is a type object, but not a subtype of
   the standard type object.  Return 0 in all other cases.  This function
   always succeeds.


.. c:function:: unsigned int PyType_ClearCache()

   Clear the internal lookup cache. Return the current version tag.

.. c:function:: unsigned long PyType_GetFlags(PyTypeObject* type)

   Return the :c:member:`~PyTypeObject.tp_flags` member of *type*. This function is primarily
   meant for use with ``Py_LIMITED_API``; the individual flag bits are
   guaranteed to be stable across Python releases, but access to
   :c:member:`~PyTypeObject.tp_flags` itself is not part of the :ref:`limited API <limited-c-api>`.

   .. versionadded:: 3.2

   .. versionchanged:: 3.4
      The return type is now ``unsigned long`` rather than ``long``.


.. c:function:: PyObject* PyType_GetDict(PyTypeObject* type)

   Return the type object's internal namespace, which is otherwise only
   exposed via a read-only proxy (:attr:`cls.__dict__ <type.__dict__>`).
   This is a
   replacement for accessing :c:member:`~PyTypeObject.tp_dict` directly.
   The returned dictionary must be treated as read-only.

   This function is meant for specific embedding and language-binding cases,
   where direct access to the dict is necessary and indirect access
   (e.g. via the proxy or :c:func:`PyObject_GetAttr`) isn't adequate.

   Extension modules should continue to use ``tp_dict``,
   directly or indirectly, when setting up their own types.

   .. versionadded:: 3.12


.. c:function:: void PyType_Modified(PyTypeObject *type)

   Invalidate the internal lookup cache for the type and all of its
   subtypes.  This function must be called after any manual
   modification of the attributes or base classes of the type.


.. c:function:: int PyType_AddWatcher(PyType_WatchCallback callback)

   Register *callback* as a type watcher. Return a non-negative integer ID
   which must be passed to future calls to :c:func:`PyType_Watch`. In case of
   error (e.g. no more watcher IDs available), return ``-1`` and set an
   exception.

   In free-threaded builds, :c:func:`PyType_AddWatcher` is not thread-safe,
   so it must be called at start up (before spawning the first thread).

   .. versionadded:: 3.12


.. c:function:: int PyType_ClearWatcher(int watcher_id)

   Clear watcher identified by *watcher_id* (previously returned from
   :c:func:`PyType_AddWatcher`). Return ``0`` on success, ``-1`` on error (e.g.
   if *watcher_id* was never registered.)

   An extension should never call ``PyType_ClearWatcher`` with a *watcher_id*
   that was not returned to it by a previous call to
   :c:func:`PyType_AddWatcher`.

   .. versionadded:: 3.12


.. c:function:: int PyType_Watch(int watcher_id, PyObject *type)

   Mark *type* as watched. The callback granted *watcher_id* by
   :c:func:`PyType_AddWatcher` will be called whenever
   :c:func:`PyType_Modified` reports a change to *type*. (The callback may be
   called only once for a series of consecutive modifications to *type*, if
   :c:func:`!_PyType_Lookup` is not called on *type* between the modifications;
   this is an implementation detail and subject to change.)

   An extension should never call ``PyType_Watch`` with a *watcher_id* that was
   not returned to it by a previous call to :c:func:`PyType_AddWatcher`.

   .. versionadded:: 3.12


.. c:function:: int PyType_Unwatch(int watcher_id, PyObject *type)

   Mark *type* as not watched. This undoes a previous call to
   :c:func:`PyType_Watch`. *type* must not be ``NULL``.

   An extension should never call this function with a *watcher_id* that was
   not returned to it by a previous call to :c:func:`PyType_AddWatcher`.

   On success, this function returns ``0``. On failure, this function returns
   ``-1`` with an exception set.

   .. versionadded:: 3.12


.. c:type:: int (*PyType_WatchCallback)(PyObject *type)

   Type of a type-watcher callback function.

   The callback must not modify *type* or cause :c:func:`PyType_Modified` to be
   called on *type* or any type in its MRO; violating this rule could cause
   infinite recursion.

   .. versionadded:: 3.12


.. c:function:: int PyType_HasFeature(PyTypeObject *o, int feature)

   Return non-zero if the type object *o* sets the feature *feature*.
   Type features are denoted by single bit flags.


.. c:function:: int PyType_FastSubclass(PyTypeObject *type, int flag)

   Return non-zero if the type object *type* sets the subclass flag *flag*.
   Subclass flags are denoted by
   :c:macro:`Py_TPFLAGS_*_SUBCLASS <Py_TPFLAGS_LONG_SUBCLASS>`.
   This function is used by many ``_Check`` functions for common types.

   .. seealso::
       :c:func:`PyObject_TypeCheck`, which is used as a slower alternative in
       ``_Check`` functions for types that don't come with subclass flags.


.. c:function:: int PyType_IS_GC(PyTypeObject *o)

   Return true if the type object includes support for the cycle detector; this
   tests the type flag :c:macro:`Py_TPFLAGS_HAVE_GC`.


.. c:function:: int PyType_IsSubtype(PyTypeObject *a, PyTypeObject *b)

   Return true if *a* is a subtype of *b*.

   This function only checks for actual subtypes, which means that
   :meth:`~type.__subclasscheck__` is not called on *b*.  Call
   :c:func:`PyObject_IsSubclass` to do the same check that :func:`issubclass`
   would do.


.. c:function:: PyObject* PyType_GenericAlloc(PyTypeObject *type, Py_ssize_t nitems)

   Generic handler for the :c:member:`~PyTypeObject.tp_alloc` slot of a type
   object.  Uses Python's default memory allocation mechanism to allocate memory
   for a new instance, zeros the memory, then initializes the memory as if by
   calling :c:func:`PyObject_Init` or :c:func:`PyObject_InitVar`.

   Do not call this directly to allocate memory for an object; call the type's
   :c:member:`~PyTypeObject.tp_alloc` slot instead.

   For types that support garbage collection (i.e., the
   :c:macro:`Py_TPFLAGS_HAVE_GC` flag is set), this function behaves like
   :c:macro:`PyObject_GC_New` or :c:macro:`PyObject_GC_NewVar` (except the
   memory is guaranteed to be zeroed before initialization), and should be
   paired with :c:func:`PyObject_GC_Del` in :c:member:`~PyTypeObject.tp_free`.
   Otherwise, it behaves like :c:macro:`PyObject_New` or
   :c:macro:`PyObject_NewVar` (except the memory is guaranteed to be zeroed
   before initialization) and should be paired with :c:func:`PyObject_Free` in
   :c:member:`~PyTypeObject.tp_free`.


.. c:function:: PyObject* PyType_GenericNew(PyTypeObject *type, PyObject *args, PyObject *kwds)

   Generic handler for the :c:member:`~PyTypeObject.tp_new` slot of a type
   object.  Creates a new instance using the type's
   :c:member:`~PyTypeObject.tp_alloc` slot and returns the resulting object.


.. c:function:: int PyType_Ready(PyTypeObject *type)

   Finalize a type object.  This should be called on all type objects to finish
   their initialization.  This function is responsible for adding inherited slots
   from a type's base class.  Return ``0`` on success, or return ``-1`` and sets an
   exception on error.

   .. note::
       If some of the base classes implements the GC protocol and the provided
       type does not include the :c:macro:`Py_TPFLAGS_HAVE_GC` in its flags, then
       the GC protocol will be automatically implemented from its parents. On
       the contrary, if the type being created does include
       :c:macro:`Py_TPFLAGS_HAVE_GC` in its flags then it **must** implement the
       GC protocol itself by at least implementing the
       :c:member:`~PyTypeObject.tp_traverse` handle.


.. c:function:: PyObject* PyType_GetName(PyTypeObject *type)

   Return the type's name. Equivalent to getting the type's
   :attr:`~type.__name__` attribute.

   .. versionadded:: 3.11


.. c:function:: PyObject* PyType_GetQualName(PyTypeObject *type)

   Return the type's qualified name. Equivalent to getting the
   type's :attr:`~type.__qualname__` attribute.

   .. versionadded:: 3.11

.. c:function:: PyObject* PyType_GetFullyQualifiedName(PyTypeObject *type)

   Return the type's fully qualified name. Equivalent to
   ``f"{type.__module__}.{type.__qualname__}"``, or :attr:`type.__qualname__`
   if :attr:`type.__module__` is not a string or is equal to ``"builtins"``.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyType_GetModuleName(PyTypeObject *type)

   Return the type's module name. Equivalent to getting the
   :attr:`type.__module__` attribute.

   .. versionadded:: 3.13


.. c:function:: void* PyType_GetSlot(PyTypeObject *type, int slot)

   Return the function pointer stored in the given slot. If the
   result is ``NULL``, this indicates that either the slot is ``NULL``,
   or that the function was called with invalid parameters.
   Callers will typically cast the result pointer into the appropriate
   function type.

   See :c:member:`PyType_Slot.slot` for possible values of the *slot* argument.

   .. versionadded:: 3.4

   .. versionchanged:: 3.10
      :c:func:`PyType_GetSlot` can now accept all types.
      Previously, it was limited to :ref:`heap types <heap-types>`.


.. c:function:: PyObject* PyType_GetModule(PyTypeObject *type)

   Return the module object associated with the given type when the type was
   created using :c:func:`PyType_FromModuleAndSpec`.

   The returned reference is :term:`borrowed <borrowed reference>` from *type*,
   and will be valid as long as you hold a reference to *type*.
   Do not release it with :c:func:`Py_DECREF` or similar.

   If no module is associated with the given type, sets :py:class:`TypeError`
   and returns ``NULL``.

   This function is usually used to get the module in which a method is defined.
   Note that in such a method, ``PyType_GetModule(Py_TYPE(self))``
   may not return the intended result.
   ``Py_TYPE(self)`` may be a *subclass* of the intended class, and subclasses
   are not necessarily defined in the same module as their superclass.
   See :c:type:`PyCMethod` to get the class that defines the method.
   See :c:func:`PyType_GetModuleByToken` for cases when :c:type:`!PyCMethod`
   cannot be used.

   .. versionadded:: 3.9


.. c:function:: void* PyType_GetModuleState(PyTypeObject *type)

   Return the state of the module object associated with the given type.
   This is a shortcut for calling :c:func:`PyModule_GetState()` on the result
   of :c:func:`PyType_GetModule`.

   If no module is associated with the given type, sets :py:class:`TypeError`
   and returns ``NULL``.

   If the *type* has an associated module but its state is ``NULL``,
   returns ``NULL`` without setting an exception.

   .. versionadded:: 3.9


.. c:function:: PyObject* PyType_GetModuleByToken(PyTypeObject *type, const void *mod_token)

   Find the first superclass whose module has the given
   :ref:`module token <ext-module-token>`, and return that module.

   If no module is found, raises a :py:class:`TypeError` and returns ``NULL``.

   This function is intended to be used together with
   :c:func:`PyModule_GetState()` to get module state from slot methods (such as
   :c:member:`~PyTypeObject.tp_init` or :c:member:`~PyNumberMethods.nb_add`)
   and other places where a method's defining class cannot be passed using the
   :c:type:`PyCMethod` calling convention.

   .. versionadded:: 3.15


.. c:function:: PyObject* PyType_GetModuleByDef(PyTypeObject *type, struct PyModuleDef *def)

   Find the first superclass whose module was created from the given
   :c:type:`PyModuleDef` *def*, or whose :ref:`module token <ext-module-token>`
   is equal to *def*, and return that module.

   Note that modules created from a :c:type:`PyModuleDef` always have their
   token set to the :c:type:`PyModuleDef`'s address.
   In other words, this function is equivalent to
   :c:func:`PyType_GetModuleByToken`, except that it:

   - returns a borrowed reference, and
   - has a non-``void*`` argument type (which is a cosmetic difference in C).

   The returned reference is :term:`borrowed <borrowed reference>` from *type*,
   and will be valid as long as you hold a reference to *type*.
   Do not release it with :c:func:`Py_DECREF` or similar.

   .. versionadded:: 3.11


.. c:function:: int PyType_GetBaseByToken(PyTypeObject *type, void *tp_token, PyTypeObject **result)

   Find the first superclass in *type*'s :term:`method resolution order` whose
   :c:macro:`Py_tp_token` token is equal to *tp_token*.

   * If found, set *\*result* to a new :term:`strong reference`
     to it and return ``1``.
   * If not found, set *\*result* to ``NULL`` and return ``0``.
   * On error, set *\*result* to ``NULL`` and return ``-1`` with an
     exception set.

   The *result* argument may be ``NULL``, in which case *\*result* is not set.
   Use this if you need only the return value.

   The *tp_token* argument may not be ``NULL``.

   .. versionadded:: 3.14


.. c:function:: int PyUnstable_Type_AssignVersionTag(PyTypeObject *type)

   Attempt to assign a version tag to the given type.

   Returns 1 if the type already had a valid version tag or a new one was
   assigned, or 0 if a new tag could not be assigned.

   .. versionadded:: 3.12


.. c:function:: int PyType_SUPPORTS_WEAKREFS(PyTypeObject *type)

   Return true if instances of *type* support creating weak references, false
   otherwise. This function always succeeds. *type* must not be ``NULL``.

   .. seealso::
      * :ref:`weakrefobjects`
      * :py:mod:`weakref`


.. _creating-heap-types:

Creating Heap-Allocated Types
-----------------------------

The following function is used to create :ref:`heap types <heap-types>`:

.. c:function:: PyObject *PyType_FromSlots(const PySlot *slots)

   Create and return a :ref:`heap type <heap-types>` from a :c:type:`!PySlot`
   array.
   See :ref:`capi-slots` for general information on slots,
   and :ref:`pyslot_type_slot_ids` for slots specific to type creation.

   This function calls :c:func:`PyType_Ready` on the new type.

   Note that this function does *not* fully match the behavior of
   calling :py:class:`type() <type>` or using the :keyword:`class` statement.
   With user-provided base types or metaclasses, prefer
   :ref:`calling <capi-call>` :py:class:`type` (or the metaclass)
   over ``PyType_From*`` functions.
   Specifically:

   * :py:meth:`~object.__new__` is not called on the new class
     (and it must be set to ``type.__new__``).
   * :py:meth:`~object.__init__` is not called on the new class.
   * :py:meth:`~object.__init_subclass__` is not called on any bases.
   * :py:meth:`~object.__set_name__` is not called on new descriptors.

   Slots are typically defined as a global static constant arrays.
   However, sometimes slot values are not statically known at compile time.
   For example, slots like :c:data:`Py_tp_bases`, :c:data:`Py_tp_metaclass`
   and :c:data:`Py_tp_module` require live Python objects.
   In this case, it is recommended to put such slots on the stack,
   and use :c:macro:`Py_slot_subslots` to refer to an array of static slots.
   For example::

      static const PySlot my_slots[] = {
         PySlot_STATIC_DATA(Py_tp_name, "MyClass"),
         PySlot_FUNC(Py_tp_repr, my_repr_func),
         ...
         PySlot_END
      };

      PyObject *make_my_class(PyObject *module) {
         PySlot all_slots[] = {
            PySlot_STATIC_DATA(Py_slot_subslots, my_slots),
            PySlot_DATA(Py_tp_module, module),
            PySlot_END
         };
         return PyType_FromSlots(all_slots);
      }

Heap types created without the :c:macro:`Py_TPFLAGS_IMMUTABLETYPE` flag may be
modified, for example by setting attributes on them, as with classes defined
in Python code.
Sometimes, such modifications are necessary to fully initialize a type,
but you may wish to prevent users from changing the type after
the initialization is done:

.. c:function:: int PyType_Freeze(PyTypeObject *type)

   Make a type immutable: set the :c:macro:`Py_TPFLAGS_IMMUTABLETYPE` flag.

   All base classes of *type* must be immutable.

   On success, return ``0``.
   On error, set an exception and return ``-1``.

   The type must not be used before it's made immutable. For example, type
   instances must not be created before the type is made immutable.

   .. versionadded:: 3.14


.. _pyslot_type_slot_ids:

Type slot IDs
.............

Most type slot IDs are named like the field names of the structures
:c:type:`PyTypeObject`, :c:type:`PyNumberMethods`,
:c:type:`PySequenceMethods`, :c:type:`PyMappingMethods` and
:c:type:`PyAsyncMethods` with an added ``Py_`` prefix.
For example, use:

* :c:data:`Py_tp_dealloc` to set :c:member:`PyTypeObject.tp_dealloc`
* :c:data:`Py_nb_add` to set :c:member:`PyNumberMethods.nb_add`
* :c:data:`Py_sq_length` to set :c:member:`PySequenceMethods.sq_length`

The following slots need additional considerations when specified as slots:

* :c:data:`Py_tp_name`
* :c:data:`Py_tp_basicsize` and :c:data:`Py_tp_extra_basicsize`
* :c:data:`Py_tp_itemsize`
* :c:data:`Py_tp_flags`

Additional slots do not directly correspond to a :c:type:`!PyTypeObject`
struct field:

* :c:data:`Py_tp_token`
* :c:data:`Py_tp_metaclass`
* :c:data:`Py_tp_module`

The following “offset” fields cannot be set using :c:type:`PyType_Slot`:

* :c:member:`~PyTypeObject.tp_weaklistoffset`
  (use :c:macro:`Py_TPFLAGS_MANAGED_WEAKREF` instead if possible)
* :c:member:`~PyTypeObject.tp_dictoffset`
  (use :c:macro:`Py_TPFLAGS_MANAGED_DICT` instead if possible)
* :c:member:`~PyTypeObject.tp_vectorcall_offset`
  (use ``"__vectorcalloffset__"`` in :ref:`PyMemberDef <pymemberdef-offsets>`)

If it is not possible to switch to a ``MANAGED`` flag (for example,
for vectorcall or to support Python older than 3.12), specify the
offset in :c:data:`Py_tp_members`.
See :ref:`PyMemberDef documentation <pymemberdef-offsets>`
for details.

The following internal fields cannot be set at all when creating a heap
type:

* :c:member:`~PyTypeObject.tp_dict`,
  :c:member:`~PyTypeObject.tp_mro`,
  :c:member:`~PyTypeObject.tp_cache`,
  :c:member:`~PyTypeObject.tp_subclasses`, and
  :c:member:`~PyTypeObject.tp_weaklist`.

The :c:data:`Py_tp_base` slot is equivalent to :c:data:`Py_tp_bases`;
both may be set either to a type or a tuple of types.
If both are specified, the value of :c:data:`Py_tp_bases`
is used.

Slot values may not be ``NULL``, except for the following:

* :c:data:`Py_tp_doc`
* :c:data:`Py_tp_token` (for clarity, prefer :c:data:`Py_TP_USE_SPEC`
  rather than ``NULL``)

.. versionchanged:: 3.9
   Slots in :c:type:`PyBufferProcs` may be set in the unlimited API.

.. versionchanged:: 3.11
   :c:member:`~PyBufferProcs.bf_getbuffer` and
   :c:member:`~PyBufferProcs.bf_releasebuffer` are now available
   under the :ref:`limited API <limited-c-api>`.

.. versionchanged:: 3.14
   The field :c:member:`~PyTypeObject.tp_vectorcall` can now be set
   using :c:data:`Py_tp_vectorcall`.  See the field's documentation
   for details.

The following slots correspond to fields in the underlying type structure,
but need extra remarks for use as slots:

.. c:macro:: Py_tp_name

   :c:member:`Slot ID <PySlot.sl_id>` for the name of the type,
   used to set :c:member:`PyTypeObject.tp_name`.

   This slot (or :c:func:`PyType_Spec.name`) is required to create a type.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use :c:func:`PyType_Spec.name` instead.

   .. impl-detail::

      CPython processes slots in order.
      It is recommended to put ``Py_tp_name`` at the beginning of the slots
      array, so that if processing of a later slots fails, error messages
      can include the name.

   .. versionadded:: next

.. c:macro:: Py_tp_basicsize

   :c:member:`Slot ID <PySlot.sl_id>` for the size of the instance in bytes.
   It is used to set :c:member:`PyTypeObject.tp_basicsize`.

   The value must be positive.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use :c:func:`PyType_Spec.basicsize` instead.

   This slot may not be used with :c:func:`PyType_GetSlot`.
   Use :c:member:`PyTypeObject.tp_basicsize` instead if needed, but be aware
   that a type's size is often considered an implementation detail.

   .. versionadded:: next

.. c:macro:: Py_tp_extra_basicsize

   :c:member:`Slot ID <PySlot.sl_id>` for type data size in bytes, that is,
   how much space instances of the class need *in addition*
   to space needed for superclasses.

   The value is used, together with the size of superclasses, to set
   :c:member:`PyTypeObject.tp_basicsize`.
   Python will insert padding as needed to meet
   :c:member:`!tp_basicsize`'s alignment requirements.

   Use :c:func:`PyObject_GetTypeData` to get a pointer to subclass-specific
   memory reserved this way.

   The value must be positive.
   To specify that instances need no additional size (that is, size should be
   inherited), omit the :c:macro:`!Py_tp_extra_basicsize` slot rather than
   set it to zero.

   Specifying both :c:macro:`Py_tp_basicsize` and
   :c:macro:`!Py_tp_extra_basicsize` is an error.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use negative :c:func:`PyType_Spec.basicsize` instead.

   This slot may not be used with :c:func:`PyType_GetSlot`.

   .. versionadded:: next

.. c:macro:: Py_tp_itemsize

   :c:member:`Slot ID <PySlot.sl_id>` for the size of one element of a
   variable-size type, in bytes.
   Used to set :c:member:`PyTypeObject.tp_itemsize`.
   See :c:member:`!tp_itemsize` documentation for caveats.

   The value must be positive.

   If this slot is missing, :c:member:`~PyTypeObject.tp_itemsize` is inherited.
   Extending arbitrary variable-sized classes is dangerous,
   since some types use a fixed offset for variable-sized memory,
   which can then overlap fixed-sized memory used by a subclass.
   To help prevent mistakes, inheriting ``itemsize`` is only possible
   in the following situations:

   - The base is not variable-sized (its
     :c:member:`~PyTypeObject.tp_itemsize`).
   - The requested :c:member:`PyType_Spec.basicsize` is positive,
     suggesting that the memory layout of the base class is known.
   - The requested :c:member:`PyType_Spec.basicsize` is zero,
     suggesting that the subclass does not access the instance's memory
     directly.
   - With the :c:macro:`Py_TPFLAGS_ITEMS_AT_END` flag.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use :c:func:`PyType_Spec.itemsize` instead.

   This slot may not be used with :c:func:`PyType_GetSlot`.

   .. versionadded:: next

.. c:macro:: Py_tp_flags

   :c:member:`Slot ID <PySlot.sl_id>` for type flags, used to set
   :c:member:`PyTypeObject.tp_flags`.

   The ``Py_TPFLAGS_HEAPTYPE`` flag is not set,
   :c:func:`PyType_FromSpecWithBases` sets it automatically.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use negative :c:func:`PyType_Spec.basicsize` instead.

   This slot may not be used with :c:func:`PyType_GetSlot`.
   Use :c:func:`PyType_GetFlags` instead.

   .. versionadded:: next

The following slots do not correspond to public fields in the
underlying structures:

.. c:macro:: Py_tp_metaclass

   :c:member:`Slot ID <PySlot.sl_id>` for the metaclass used to construct
   the resulting type object.
   When omitted the metaclass is derived from bases
   (:c:macro:`Py_tp_bases` or the *bases* argument of
   :c:func:`PyType_FromMetaclass`).

   Metaclasses that override :c:member:`~PyTypeObject.tp_new` are not
   supported, except if ``tp_new`` is ``NULL``.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use :c:func:`PyType_FromMetaclass` to specify a metaclass with
   :c:type:`!PyType_Spec`.

   This slot may not be used with :c:func:`PyType_GetSlot`.
   Use :c:func:`Py_TYPE` on the type object instead.

   .. versionadded:: next

.. c:macro:: Py_tp_module

   :c:member:`Slot ID <PySlot.sl_id>` for recording the module in which
   the new class is defined.

   The value must be a module object.
   The module is associated with the new type and can later be
   retrieved with :c:func:`PyType_GetModule`.
   The associated module is not inherited by subclasses; it must be specified
   for each class individually.

   This may not be used in :c:member:`PyType_Spec.slots`.
   Use :c:func:`PyType_FromMetaclass` to specify a module with
   :c:type:`!PyType_Spec`.

   This slot may not be used with :c:func:`PyType_GetSlot`.
   Use :c:func:`PyType_GetModule` instead.

   .. versionadded:: next

.. c:macro:: Py_tp_token

   :c:member:`Slot ID <PySlot.sl_id>` for recording a static memory layout ID
   for a class.

   If the class is defined using a :c:type:`PyType_Spec`, and that spec is
   statically allocated, the token can be set to the spec using the special
   value :c:data:`Py_TP_USE_SPEC`:

   .. code-block:: c

      static PyType_Slot foo_slots[] = {
         {Py_tp_token, Py_TP_USE_SPEC},

   It can also be set to an arbitrary pointer, but you must ensure that:

   * The pointer outlives the class, so it's not reused for something else
     while the class exists.
   * It "belongs" to the extension module where the class lives, so it will not
     clash with other extensions.

   Use :c:func:`PyType_GetBaseByToken` to check if a class's superclass has
   a given token -- that is, check whether the memory layout is compatible.

   To get the token for a given class (without considering superclasses),
   use :c:func:`PyType_GetSlot` with ``Py_tp_token``.

   .. versionadded:: 3.14

   .. c:namespace:: NULL

   .. c:macro:: Py_TP_USE_SPEC

      Used as a value with :c:data:`Py_tp_token` to set the token to the
      class's :c:type:`PyType_Spec`.
      May only be used for classes defined using :c:type:`!PyType_Spec`.

      Expands to ``NULL``.

      .. versionadded:: 3.14

.. c:macro:: Py_tp_slots

   :c:member:`Slot ID <PySlot.sl_id>` that works like
   :c:macro:`Py_slot_subslots`, except it specifies an array of
   :c:type:`PyType_Slot` structures.

   .. versionadded:: next


Soft-deprecated API
-------------------

The following functions are :term:`soft deprecated`.
They will continue to work, but new features will be added as slots for
:c:func:`PyType_FromSlots`, not as arguments to new ``PyType_From*`` functions.

.. c:function:: PyObject* PyType_FromMetaclass(PyTypeObject *metaclass, PyObject *module, PyType_Spec *spec, PyObject *bases)

   Create and return a :ref:`heap type <heap-types>` from the *spec*
   (see :c:macro:`Py_TPFLAGS_HEAPTYPE`).

   A non-``NULL`` *metaclass* argument corresponds to the
   :c:macro:`Py_tp_metaclass` slot.

   A non-``NULL`` *bases* argument corresponds to the :c:data:`Py_tp_bases`
   slot, and takes precedence over :c:data:`Py_tp_bases` and
   :c:data:`Py_tp_bases` slots.

   A non-``NULL`` *module* argument corresponds to the
   :c:macro:`Py_tp_module` slot.

   This function calls :c:func:`PyType_Ready` on the new type.

   Note that this function does *not* fully match the behavior of
   calling :py:class:`type() <type>` or using the :keyword:`class` statement.
   See the note in :c:func:`PyType_FromSlots` documentation for details.

   .. versionadded:: 3.12

   .. soft-deprecated:: next

      Prefer :c:func:`PyType_FromSlots` in new code.


.. c:function:: PyObject* PyType_FromModuleAndSpec(PyObject *module, PyType_Spec *spec, PyObject *bases)

   Equivalent to ``PyType_FromMetaclass(NULL, module, spec, bases)``.

   .. versionadded:: 3.9

   .. versionchanged:: 3.10

      The function now accepts a single class as the *bases* argument and
      ``NULL`` as the ``tp_doc`` slot.

   .. versionchanged:: 3.12

      The function now finds and uses a metaclass corresponding to the provided
      base classes.  Previously, only :class:`type` instances were returned.

      The :c:member:`~PyTypeObject.tp_new` of the metaclass is *ignored*.
      which may result in incomplete initialization.
      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is deprecated.

   .. versionchanged:: 3.14

      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is no longer allowed.

   .. soft-deprecated:: next

      Prefer :c:func:`PyType_FromSlots` in new code.


.. c:function:: PyObject* PyType_FromSpecWithBases(PyType_Spec *spec, PyObject *bases)

   Equivalent to ``PyType_FromMetaclass(NULL, NULL, spec, bases)``.

   .. versionadded:: 3.3

   .. versionchanged:: 3.12

      The function now finds and uses a metaclass corresponding to the provided
      base classes.  Previously, only :class:`type` instances were returned.

      The :c:member:`~PyTypeObject.tp_new` of the metaclass is *ignored*.
      which may result in incomplete initialization.
      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is deprecated.

   .. versionchanged:: 3.14

      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is no longer allowed.

   .. soft-deprecated:: next

      Prefer :c:func:`PyType_FromSlots` in new code.


.. c:function:: PyObject* PyType_FromSpec(PyType_Spec *spec)

   Equivalent to ``PyType_FromMetaclass(NULL, NULL, spec, NULL)``.

   .. versionchanged:: 3.12

      The function now finds and uses a metaclass corresponding to the
      base classes provided in *Py_tp_base[s]* slots.
      Previously, only :class:`type` instances were returned.

      The :c:member:`~PyTypeObject.tp_new` of the metaclass is *ignored*.
      which may result in incomplete initialization.
      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is deprecated.

   .. versionchanged:: 3.14

      Creating classes whose metaclass overrides
      :c:member:`~PyTypeObject.tp_new` is no longer allowed.

   .. soft-deprecated:: next

      Prefer :c:func:`PyType_FromSlots` in new code.

.. raw:: html

   <!-- Keep old URL fragments working (see gh-97908) -->
   <span id='c.PyType_Spec.PyType_Spec.name'></span>
   <span id='c.PyType_Spec.PyType_Spec.basicsize'></span>
   <span id='c.PyType_Spec.PyType_Spec.itemsize'></span>
   <span id='c.PyType_Spec.PyType_Spec.flags'></span>
   <span id='c.PyType_Spec.PyType_Spec.slots'></span>

.. c:type:: PyType_Spec

   Structure defining a type's behavior, used for soft-deprecated functions
   like :c:func:`PyType_FromMetaclass`.

   This structure contains several members that can instead be specified
   as :ref:`slots <pyslot_type_slot_ids>` for :c:func:`PyType_FromSlots`,
   and an array of slot entries with a simpler structure.

   .. c:member:: const char* name

      Corresponds to :c:macro:`Py_tp_name`.

   .. c:member:: int basicsize

      If positive, corresponds to :c:macro:`Py_tp_basicsize`.

      If negative, corresponds to :c:macro:`Py_tp_extra_basicsize` set to
      the absolute value.

      .. versionchanged:: 3.12

         Previously, this field could not be negative.

   .. c:member:: int itemsize

      Corresponds to :c:macro:`Py_tp_itemsize`.

   .. c:member:: unsigned int flags

      Corresponds to :c:macro:`Py_tp_flags`.

   .. c:member:: PyType_Slot *slots

      Array of :c:type:`PyType_Slot` (not :c:type:`PySlot`) structures.

      Terminated by the special slot value ``{0, NULL}``.
      Each slot ID should be specified at most once.

      .. c:namespace:: NULL

      .. raw:: html

         <!-- Keep old URL fragments working (see gh-97908) -->
         <span id='c.PyType_Slot.PyType_Slot.slot'></span>
         <span id='c.PyType_Slot.PyType_Slot.pfunc'></span>

      .. c:type:: PyType_Slot

         Structure defining optional functionality of a type, used for
         soft-deprecated functions like :c:func:`PyType_FromMetaclass`.

         Note that a :c:type:`!PyType_Slot` array may be included in a
         :c:type:`!PySlot` array using :c:macro:`Py_tp_slots`,
         and vice versa using :c:macro:`Py_slot_subslots`.

         Each :c:type:`!PyType_Slot` structure ``tpslot`` is interpreted
         as the following :c:type:`PySlot` structure::

            (PySlot){
               .sl_id=tpslot.slot,
               .sl_flags=PySlot_INTPTR | sub_static,
               .sl_ptr=tpslot.func
            }

         where ``sub_static`` is ``PySlot_STATIC`` if the slot requires
         the flag (such as for :c:macro:`Py_tp_methods`), or if this flag
         is present on the "parent" :c:macro:`!Py_tp_slots` slot (if any).

         .. c:member:: int slot

            Corresponds to :c:member:`PySlot.sl_id`.

         .. c:member:: void *pfunc

            Corresponds to :c:member:`PySlot.sl_ptr`.
