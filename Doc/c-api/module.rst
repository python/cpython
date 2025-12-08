.. highlight:: c

.. _moduleobjects:

Module Objects
==============

.. index:: pair: object; module

.. c:var:: PyTypeObject PyModule_Type

   .. index:: single: ModuleType (in module types)

   This instance of :c:type:`PyTypeObject` represents the Python module type.  This
   is exposed to Python programs as :py:class:`types.ModuleType`.


.. c:function:: int PyModule_Check(PyObject *p)

   Return true if *p* is a module object, or a subtype of a module object.
   This function always succeeds.


.. c:function:: int PyModule_CheckExact(PyObject *p)

   Return true if *p* is a module object, but not a subtype of
   :c:data:`PyModule_Type`.  This function always succeeds.


.. c:function:: PyObject* PyModule_NewObject(PyObject *name)

   .. index::
      single: __name__ (module attribute)
      single: __doc__ (module attribute)
      single: __file__ (module attribute)
      single: __package__ (module attribute)
      single: __loader__ (module attribute)

   Return a new module object with :attr:`module.__name__` set to *name*.
   The module's :attr:`!__name__`, :attr:`~module.__doc__`,
   :attr:`~module.__package__` and :attr:`~module.__loader__` attributes are
   filled in (all but :attr:`!__name__` are set to ``None``). The caller is
   responsible for setting a :attr:`~module.__file__` attribute.

   Return ``NULL`` with an exception set on error.

   .. versionadded:: 3.3

   .. versionchanged:: 3.4
      :attr:`~module.__package__` and :attr:`~module.__loader__` are now set to
      ``None``.


.. c:function:: PyObject* PyModule_New(const char *name)

   Similar to :c:func:`PyModule_NewObject`, but the name is a UTF-8 encoded
   string instead of a Unicode object.


.. c:function:: PyObject* PyModule_GetDict(PyObject *module)

   .. index:: single: __dict__ (module attribute)

   Return the dictionary object that implements *module*'s namespace; this object
   is the same as the :attr:`~object.__dict__` attribute of the module object.
   If *module* is not a module object (or a subtype of a module object),
   :exc:`SystemError` is raised and ``NULL`` is returned.

   It is recommended extensions use other ``PyModule_*`` and
   ``PyObject_*`` functions rather than directly manipulate a module's
   :attr:`~object.__dict__`.

   The returned reference is borrowed from the module; it is valid until
   the module is destroyed.


.. c:function:: PyObject* PyModule_GetNameObject(PyObject *module)

   .. index::
      single: __name__ (module attribute)
      single: SystemError (built-in exception)

   Return *module*'s :attr:`~module.__name__` value.  If the module does not
   provide one, or if it is not a string, :exc:`SystemError` is raised and
   ``NULL`` is returned.

   .. versionadded:: 3.3


.. c:function:: const char* PyModule_GetName(PyObject *module)

   Similar to :c:func:`PyModule_GetNameObject` but return the name encoded to
   ``'utf-8'``.

   The returned buffer is only valid until the module is renamed or destroyed.
   Note that Python code may rename a module by setting its :py:attr:`~module.__name__`
   attribute.

.. c:function:: PyModuleDef* PyModule_GetDef(PyObject *module)

   Return a pointer to the :c:type:`PyModuleDef` struct from which the module was
   created, or ``NULL`` if the module wasn't created from a definition.

   On error, return ``NULL`` with an exception set.
   Use :c:func:`PyErr_Occurred` to tell this case apart from a missing
   :c:type:`!PyModuleDef`.


.. c:function:: PyObject* PyModule_GetFilenameObject(PyObject *module)

   .. index::
      single: __file__ (module attribute)
      single: SystemError (built-in exception)

   Return the name of the file from which *module* was loaded using *module*'s
   :attr:`~module.__file__` attribute.  If this is not defined, or if it is not a
   string, raise :exc:`SystemError` and return ``NULL``; otherwise return
   a reference to a Unicode object.

   .. versionadded:: 3.2


.. c:function:: const char* PyModule_GetFilename(PyObject *module)

   Similar to :c:func:`PyModule_GetFilenameObject` but return the filename
   encoded to 'utf-8'.

   The returned buffer is only valid until the module's :py:attr:`~module.__file__` attribute
   is reassigned or the module is destroyed.

   .. deprecated:: 3.2
      :c:func:`PyModule_GetFilename` raises :exc:`UnicodeEncodeError` on
      unencodable filenames, use :c:func:`PyModule_GetFilenameObject` instead.


.. _pymoduledef_slot:

Module definition
-----------------

Modules created using the C API are typically defined using an
array of :dfn:`slots`.
The slots provide a "description" of how a module should be created.

.. versionchanged:: next

   Previously, a :c:type:`PyModuleDef` struct was necessary to define modules.
   The older way of defining modules is still available: consult either the
   :ref:`pymoduledef` section or earlier versions of this documentation
   if you plan to support earlier Python versions.

The slots array is usually used to define an extension module's “main”
module object (see :ref:`extension-modules` for details).
It can also be used to
:ref:`create extension modules dynamically <module-from-slots>`.

Unless specified otherwise, the same slot ID may not be repeated
in an array of slots.


.. c:type:: PyModuleDef_Slot

   .. c:member:: int slot

      A slot ID, chosen from the available ``Py_mod_*`` values explained below.

      An ID of 0 marks the end of a :c:type:`!PyModuleDef_Slot` array.

   .. c:member:: void* value

      Value of the slot, whose meaning depends on the slot ID.

      The value may not be NULL.
      To leave a slot out, omit the :c:type:`PyModuleDef_Slot` entry entirely.

   .. versionadded:: 3.5


Metadata slots
..............

.. c:macro:: Py_mod_name

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for the name of the new module,
   as a NUL-terminated UTF8-encoded ``const char *``.

   Note that modules are typically created using a
   :py:class:`~importlib.machinery.ModuleSpec`, and when they are, the
   name from the spec will be used instead of :c:data:`!Py_mod_name`.
   However, it is still recommended to include this slot for introspection
   and debugging purposes.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_name` instead to support previous versions.

.. c:macro:: Py_mod_doc

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for the docstring of the new
   module, as a NUL-terminated UTF8-encoded ``const char *``.

   Usually it is set to a variable created with :c:macro:`PyDoc_STRVAR`.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_doc` instead to support previous versions.


Feature slots
.............

.. c:macro:: Py_mod_abi

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` whose value points to
   a :c:struct:`PyABIInfo` structure describing the ABI that
   the extension is using.

   A suitable :c:struct:`!PyABIInfo` variable can be defined using the
   :c:macro:`PyABIInfo_VAR` macro, as in:

   .. code-block:: c

      PyABIInfo_VAR(abi_info);

      static PyModuleDef_Slot mymodule_slots[] = {
         {Py_mod_abi, &abi_info},
         ...
      };

   When creating a module, Python checks the value of this slot
   using :c:func:`PyABIInfo_Check`.

   .. versionadded:: 3.15

.. c:macro:: Py_mod_multiple_interpreters

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` whose value is one of:

   .. c:namespace:: NULL

   .. c:macro:: Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED

      The module does not support being imported in subinterpreters.

   .. c:macro:: Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED

      The module supports being imported in subinterpreters,
      but only when they share the main interpreter's GIL.
      (See :ref:`isolating-extensions-howto`.)

   .. c:macro:: Py_MOD_PER_INTERPRETER_GIL_SUPPORTED

      The module supports being imported in subinterpreters,
      even when they have their own GIL.
      (See :ref:`isolating-extensions-howto`.)

   This slot determines whether or not importing this module
   in a subinterpreter will fail.

   If ``Py_mod_multiple_interpreters`` is not specified, the import
   machinery defaults to ``Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED``.

   .. versionadded:: 3.12

.. c:macro:: Py_mod_gil

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` whose value is one of:

   .. c:namespace:: NULL

   .. c:macro:: Py_MOD_GIL_USED

      The module depends on the presence of the global interpreter lock (GIL),
      and may access global state without synchronization.

   .. c:macro:: Py_MOD_GIL_NOT_USED

      The module is safe to run without an active GIL.

   This slot is ignored by Python builds not configured with
   :option:`--disable-gil`.  Otherwise, it determines whether or not importing
   this module will cause the GIL to be automatically enabled. See
   :ref:`whatsnew313-free-threaded-cpython` for more detail.

   If ``Py_mod_gil`` is not specified, the import machinery defaults to
   ``Py_MOD_GIL_USED``.

   .. versionadded:: 3.13


Creation and initialization slots
.................................

.. c:macro:: Py_mod_create

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a function that creates
   the module object itself.
   The function must have the signature:

   .. c:function:: PyObject* create_module(PyObject *spec, PyModuleDef *def)
      :no-index-entry:
      :no-contents-entry:

   The function will be called with:

   - *spec*: a ``ModuleSpec``-like object, meaning that any attributes defined
     for :py:class:`importlib.machinery.ModuleSpec` have matching semantics.
     However, any of the attributes may be missing.
   - *def*: ``NULL``, or the module definition if the module is created from one.

   The function should return a new module object, or set an error
   and return ``NULL``.

   This function should be kept minimal. In particular, it should not
   call arbitrary Python code, as trying to import the same module again may
   result in an infinite loop.

   If ``Py_mod_create`` is not specified, the import machinery will create
   a normal module object using :c:func:`PyModule_New`. The name is taken from
   *spec*, not the definition, to allow extension modules to dynamically adjust
   to their place in the module hierarchy and be imported under different
   names through symlinks, all while sharing a single module definition.

   There is no requirement for the returned object to be an instance of
   :c:type:`PyModule_Type`.
   However, some slots may only be used with
   :c:type:`!PyModule_Type` instances; in particular:

   - :c:macro:`Py_mod_exec`,
   - :ref:`module state slots <ext-module-state-slots>` (``Py_mod_state_*``),
   - :c:macro:`Py_mod_token`.

   .. versionadded:: 3.5

   .. versionchanged:: next

      The *slots* argument may be a ``ModuleSpec``-like object, rather than
      a true :py:class:`~importlib.machinery.ModuleSpec` instance.
      Note that previous versions of CPython did not enforce this.

      The *def* argument may now be ``NULL``, since modules are not necessarily
      made from definitions.

.. c:macro:: Py_mod_exec

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a function that will
   :dfn:`execute`, or initialize, the module.
   This function does the equivalent to executing the code of a Python module:
   typically, it adds classes and constants to the module.
   The signature of the function is:

   .. c:function:: int exec_module(PyObject* module)
      :no-index-entry:
      :no-contents-entry:

   See the :ref:`capi-module-support-functions` section for some useful
   functions to call.

   For backwards compatibility, the :c:type:`PyModuleDef.m_slots` array may
   contain multiple :c:macro:`!Py_mod_exec` slots; these are processed in the
   order they appear in the array.
   Elsewhere (that is, in arguments to :c:func:`PyModule_FromSlotsAndSpec`
   and in return values of :samp:`PyModExport_{<name>}`), repeating the slot
   is not allowed.

   .. versionadded:: 3.5

   .. versionchanged:: next

      Repeated ``Py_mod_exec`` slots are disallowed, except in
      :c:type:`PyModuleDef.m_slots`.

.. c:macro:: Py_mod_methods

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a table of module-level
   functions, as an array of :c:type:`PyMethodDef` values suitable as the
   *functions* argument to :c:func:`PyModule_AddFunctions`.

   Like other slot IDs, a slots array may only contain one
   :c:macro:`!Py_mod_methods` entry.
   To add functions from multiple :c:type:`PyMethodDef` arrays, call
   :c:func:`PyModule_AddFunctions` in the :c:macro:`Py_mod_exec` function.

   The table must be statically allocated (or otherwise guaranteed to outlive
   the module object).

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_methods` instead to support previous versions.

.. _ext-module-state:

Module state
------------

Extension modules can have *module state* -- a
piece of memory that is allocated on module creation,
and freed when the module object is deallocated.
The module state is specified using :ref:`dedicated slots <ext-module-state-slots>`.

A typical use of module state is storing an exception type -- or indeed *any*
type object defined by the module --

Unlike the module's Python attributes, Python code cannot replace or delete
data stored in module state.

Keeping per-module information in attributes and module state, rather than in
static globals, makes module objects *isolated* and safer for use in
multiple sub-interpreters.
It also helps Python do an orderly clean-up when it shuts down.

Extensions that keep references to Python objects as part of module state must
implement :c:macro:`Py_mod_state_traverse` and :c:macro:`Py_mod_state_clear`
functions to avoid reference leaks.

To retrieve the state from a given module, use the following functions:

.. c:function:: void* PyModule_GetState(PyObject *module)

   Return the "state" of the module, that is, a pointer to the block of memory
   allocated at module creation time, or ``NULL``.  See
   :c:macro:`Py_mod_state_size`.

   On error, return ``NULL`` with an exception set.
   Use :c:func:`PyErr_Occurred` to tell this case apart from missing
   module state.


.. c:function:: int PyModule_GetStateSize(PyObject *, Py_ssize_t *result)

   Set *\*result* to the size of the module's state, as specified using
   :c:macro:`Py_mod_state_size` (or :c:member:`PyModuleDef.m_size`),
   and return 0.

   On error, set *\*result* to -1, and return -1 with an exception set.

   .. versionadded:: next



.. _ext-module-state-slots:

Slots for defining module state
...............................

The following :c:member:`PyModuleDef_Slot.slot` IDs are available for
defining the module state.

.. c:macro:: Py_mod_state_size

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for the size of the module state,
   in bytes.

   Setting the value to a non-negative value means that the module can be
   re-initialized and specifies the additional amount of memory it requires
   for its state.

   See :PEP:`3121` for more details.

   Use :c:func:`PyModule_GetStateSize` to retrieve the size of a given module.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_size` instead to support previous versions.

.. c:macro:: Py_mod_state_traverse

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a traversal function to call
   during GC traversal of the module object.

   The signature of the function, and meanings of the arguments,
   is similar as for :c:member:`PyTypeObject.tp_traverse`:

   .. c:function:: int traverse_module_state(PyObject *module, visitproc visit, void *arg)
      :no-index-entry:
      :no-contents-entry:

   This function is not called if the module state was requested but is not
   allocated yet. This is the case immediately after the module is created
   and before the module is executed (:c:data:`Py_mod_exec` function). More
   precisely, this function is not called if the state size
   (:c:data:`Py_mod_state_size`) is greater than 0 and the module state
   (as returned by :c:func:`PyModule_GetState`) is ``NULL``.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_size` instead to support previous versions.

.. c:macro:: Py_mod_state_clear

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a clear function to call
   during GC clearing of the module object.

   The signature of the function is:

   .. c:function:: int clear_module_state(PyObject* module)
      :no-index-entry:
      :no-contents-entry:

   This function is not called if the module state was requested but is not
   allocated yet. This is the case immediately after the module is created
   and before the module is executed (:c:data:`Py_mod_exec` function). More
   precisely, this function is not called if the state size
   (:c:data:`Py_mod_state_size`) is greater than 0 and the module state
   (as returned by :c:func:`PyModule_GetState`) is ``NULL``.

   Like :c:member:`PyTypeObject.tp_clear`, this function is not *always*
   called before a module is deallocated. For example, when reference
   counting is enough to determine that an object is no longer used,
   the cyclic garbage collector is not involved and
   the :c:macro:`Py_mod_state_free` function is called directly.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_clear` instead to support previous versions.

.. c:macro:: Py_mod_state_free

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for a function to call during
   deallocation of the module object.

   The signature of the function is:

   .. c:function:: int free_module_state(PyObject* module)
      :no-index-entry:
      :no-contents-entry:

   This function is not called if the module state was requested but is not
   allocated yet. This is the case immediately after the module is created
   and before the module is executed (:c:data:`Py_mod_exec` function). More
   precisely, this function is not called if the state size
   (:c:data:`Py_mod_state_size`) is greater than 0 and the module state
   (as returned by :c:func:`PyModule_GetState`) is ``NULL``.

   .. versionadded:: next

      Use :c:member:`PyModuleDef.m_free` instead to support previous versions.


.. _ext-module-token:

Module token
............

Each module may have an associated *token*: a pointer-sized value intended to
identify of the module state's memory layout.
This means that if you have a module object, but you are not sure if it
“belongs” to your extension, you can check using code like this:

.. code-block:: c

   PyObject *module = <the module in question>

   void *module_token;
   if (PyModule_GetToken(module, &module_token) < 0) {
       return NULL;
   }
   if (module_token != your_token) {
       PyErr_SetString(PyExc_ValueError, "unexpected module")
       return NULL;
   }

   // This module's state has the expected memory layout; it's safe to cast
   struct my_state state = (struct my_state*)PyModule_GetState(module)

A module's token -- and the *your_token* value to use in the above code -- is:

- For modules created with :c:type:`PyModuleDef`: the address of that
  :c:type:`PyModuleDef`;
- For modules defined with the :c:macro:`Py_mod_token` slot: the value
  of that slot;
- For modules created from an ``PyModExport_*``
  :ref:`export hook <extension-export-hook>`: the slots array that the export
  hook returned (unless overriden with :c:macro:`Py_mod_token`).

.. c:macro:: Py_mod_token

   :c:type:`Slot ID <PyModuleDef_Slot.slot>` for the module token.

   If you use this slot to set the module token (rather than rely on the
   default), you must ensure that:

   * The pointer outlives the class, so it's not reused for something else
     while the class exists.
   * It "belongs" to the extension module where the class lives, so it will not
     clash with other extensions.
   * If the token points to a :c:type:`PyModuleDef` struct, the module should
     behave as if it was created from that :c:type:`PyModuleDef`.
     In particular, the module state must have matching layout and semantics.

   Modules created from :c:type:`PyModuleDef` allways use the address of
   the :c:type:`PyModuleDef` as the token.
   This means that :c:macro:`!Py_mod_token` cannot be used in
   :c:member:`PyModuleDef.m_slots`.

   .. versionadded:: next

.. c:function:: int PyModule_GetToken(PyObject *module, void** result)

   Set *\*result* to the module's token and return 0.

   On error, set *\*result* to NULL, and return -1 with an exception set.

   .. versionadded:: next

See also :c:func:`PyType_GetModuleByToken`.


.. _module-from-slots:

Creating extension modules dynamically
--------------------------------------

The following functions may be used to create an extension module dynamically,
rather than from an extension's :ref:`export hook <extension-export-hook>`.

.. c:function:: PyObject *PyModule_FromSlotsAndSpec(const PyModuleDef_Slot *slots, PyObject *spec)

   Create a new module object, given an array of :ref:`slots <pymoduledef_slot>`
   and the :py:class:`~importlib.machinery.ModuleSpec` *spec*.

   The *slots* argument must point to an array of :c:type:`PyModuleDef_Slot`
   structures, terminated by an entry slot with slot ID of 0
   (typically written as ``{0}`` or ``{0, NULL}`` in C).
   The *slots* argument may not be ``NULL``.

   The *spec* argument may be any ``ModuleSpec``-like object, as described
   in :c:macro:`Py_mod_create` documentation.
   Currently, the *spec* must have a ``name`` attribute.

   On success, return the new module.
   On error, return ``NULL`` with an exception set.

   Note that this does not process the module's execution slot
   (:c:data:`Py_mod_exec`).
   Both :c:func:`!PyModule_FromSlotsAndSpec` and :c:func:`PyModule_Exec`
   must be called to fully initialize a module.
   (See also :ref:`multi-phase-initialization`.)

   The *slots* array only needs to be valid for the duration of the
   :c:func:`!PyModule_FromSlotsAndSpec` call.
   In particular, it may be heap-allocated.

   .. versionadded:: next

.. c:function:: int PyModule_Exec(PyObject *module)

   Execute the :c:data:`Py_mod_exec` slot(s) of the given *module*.

   On success, return 0.
   On error, return -1 with an exception set.

   For clarity: If *module* has no slots, for example if it uses
   :ref:`legacy single-phase initialization <single-phase-initialization>`,
   this function does nothing and returns 0.

   .. versionadded:: next



.. _pymoduledef:

Module definition struct
------------------------

Traditionally, extension modules were defined using a *module definition*
as the “description" of how a module should be created.
Rather than using an array of :ref:`slots <pymoduledef_slot>` directly,
the definition has dedicated members for most common functionality,
and allows additional slots as an extension mechanism.

This way of defining modules is still available and there are no plans to
remove it.

.. c:type:: PyModuleDef

   The module definition struct, which holds information needed to create
   a module object.

   This structure must be statically allocated (or be otherwise guaranteed
   to be valid while any modules created from it exist).
   Usually, there is only one variable of this type for each extension module
   defined this way.

   .. c:member:: PyModuleDef_Base m_base

      Always initialize this member to :c:macro:`PyModuleDef_HEAD_INIT`:

      .. c:namespace:: NULL

      .. c:type:: PyModuleDef_Base

         The type of :c:member:`!PyModuleDef.m_base`.

      .. c:macro:: PyModuleDef_HEAD_INIT

         The required initial value for :c:member:`!PyModuleDef.m_base`.

   .. c:member:: const char *m_name

      Corresponds to the :c:macro:`Py_mod_name` slot.

   .. c:member:: const char *m_doc

      These members correspond to the :c:macro:`Py_mod_doc` slot.
      Setting this to NULL is equivalent to omitting the slot.

   .. c:member:: Py_ssize_t m_size

      Corresponds to the :c:macro:`Py_mod_state_size` slot.
      Setting this to zero is equivalent to omitting the slot.

      When using :ref:`legacy single-phase initialization <single-phase-initialization>`
      or when creating modules dynamically using :c:func:`PyModule_Create`
      or :c:func:`PyModule_Create2`, :c:member:`!m_size` may be set to -1.
      This indicates that the module does not support sub-interpreters,
      because it has global state.

   .. c:member:: PyMethodDef *m_methods

      Corresponds to the :c:macro:`Py_mod_methods` slot.
      Setting this to NULL is equivalent to omitting the slot.

   .. c:member:: PyModuleDef_Slot* m_slots

      An array of additional slots, terminated by a ``{0, NULL}`` entry.

      This array may not contain slots corresponding to :c:type:`PyModuleDef`
      members.
      For example, you cannot use :c:macro:`Py_mod_name` in :c:member:`!m_slots`;
      the module name must be given as :c:member:`PyModuleDef.m_name`.

      .. versionchanged:: 3.5

         Prior to version 3.5, this member was always set to ``NULL``,
         and was defined as:

           .. c:member:: inquiry m_reload

   .. c:member:: traverseproc m_traverse
                 inquiry m_clear
                 freefunc m_free

      These members correspond to the :c:macro:`Py_mod_state_traverse`,
      :c:macro:`Py_mod_state_clear`, and :c:macro:`Py_mod_state_free` slots,
      respectively.

      Setting these members to NULL is equivalent to omitting the
      corresponding slots.

      .. versionchanged:: 3.9

         :c:member:`m_traverse`, :c:member:`m_clear` and :c:member:`m_free`
         functions are longer called before the module state is allocated.


.. _moduledef-dynamic:

The following API can be used to create modules from a :c:type:`!PyModuleDef`
struct:

.. c:function:: PyObject* PyModule_Create(PyModuleDef *def)

   Create a new module object, given the definition in *def*.
   This is a macro that calls :c:func:`PyModule_Create2` with
   *module_api_version* set to :c:macro:`PYTHON_API_VERSION`, or
   to :c:macro:`PYTHON_ABI_VERSION` if using the
   :ref:`limited API <limited-c-api>`.

.. c:function:: PyObject* PyModule_Create2(PyModuleDef *def, int module_api_version)

   Create a new module object, given the definition in *def*, assuming the
   API version *module_api_version*.  If that version does not match the version
   of the running interpreter, a :exc:`RuntimeWarning` is emitted.

   Return ``NULL`` with an exception set on error.

   This function does not support slots.
   The :c:member:`~PyModuleDef.m_slots` member of *def* must be ``NULL``.


   .. note::

      Most uses of this function should be using :c:func:`PyModule_Create`
      instead; only use this if you are sure you need it.

.. c:function:: PyObject * PyModule_FromDefAndSpec(PyModuleDef *def, PyObject *spec)

   This macro calls :c:func:`PyModule_FromDefAndSpec2` with
   *module_api_version* set to :c:macro:`PYTHON_API_VERSION`, or
   to :c:macro:`PYTHON_ABI_VERSION` if using the
   :ref:`limited API <limited-c-api>`.

   .. versionadded:: 3.5

.. c:function:: PyObject * PyModule_FromDefAndSpec2(PyModuleDef *def, PyObject *spec, int module_api_version)

   Create a new module object, given the definition in *def* and the
   ModuleSpec *spec*, assuming the API version *module_api_version*.
   If that version does not match the version of the running interpreter,
   a :exc:`RuntimeWarning` is emitted.

   Return ``NULL`` with an exception set on error.

   Note that this does not process execution slots (:c:data:`Py_mod_exec`).
   Both ``PyModule_FromDefAndSpec`` and ``PyModule_ExecDef`` must be called
   to fully initialize a module.

   .. note::

      Most uses of this function should be using :c:func:`PyModule_FromDefAndSpec`
      instead; only use this if you are sure you need it.

   .. versionadded:: 3.5

.. c:function:: int PyModule_ExecDef(PyObject *module, PyModuleDef *def)

   Process any execution slots (:c:data:`Py_mod_exec`) given in *def*.

   .. versionadded:: 3.5

.. c:macro:: PYTHON_API_VERSION

   The C API version. Defined for backwards compatibility.

   Currently, this constant is not updated in new Python versions, and is not
   useful for versioning. This may change in the future.

.. c:macro:: PYTHON_ABI_VERSION

   Defined as ``3`` for backwards compatibility.

   Currently, this constant is not updated in new Python versions, and is not
   useful for versioning. This may change in the future.


.. _capi-module-support-functions:

Support functions
-----------------

The following functions are provided to help initialize a module object.
They are intended for a module's execution slot (:c:data:`Py_mod_exec`),
the initialization function for legacy :ref:`single-phase initialization <single-phase-initialization>`,
or code that creates modules dynamically.

.. c:function:: int PyModule_AddObjectRef(PyObject *module, const char *name, PyObject *value)

   Add an object to *module* as *name*.  This is a convenience function which
   can be used from the module's initialization function.

   On success, return ``0``. On error, raise an exception and return ``-1``.

   Example usage::

       static int
       add_spam(PyObject *module, int value)
       {
           PyObject *obj = PyLong_FromLong(value);
           if (obj == NULL) {
               return -1;
           }
           int res = PyModule_AddObjectRef(module, "spam", obj);
           Py_DECREF(obj);
           return res;
        }

   To be convenient, the function accepts ``NULL`` *value* with an exception
   set. In this case, return ``-1`` and just leave the raised exception
   unchanged.

   The example can also be written without checking explicitly if *obj* is
   ``NULL``::

       static int
       add_spam(PyObject *module, int value)
       {
           PyObject *obj = PyLong_FromLong(value);
           int res = PyModule_AddObjectRef(module, "spam", obj);
           Py_XDECREF(obj);
           return res;
        }

   Note that ``Py_XDECREF()`` should be used instead of ``Py_DECREF()`` in
   this case, since *obj* can be ``NULL``.

   The number of different *name* strings passed to this function
   should be kept small, usually by only using statically allocated strings
   as *name*.
   For names that aren't known at compile time, prefer calling
   :c:func:`PyUnicode_FromString` and :c:func:`PyObject_SetAttr` directly.
   For more details, see :c:func:`PyUnicode_InternFromString`, which may be
   used internally to create a key object.

   .. versionadded:: 3.10


.. c:function:: int PyModule_Add(PyObject *module, const char *name, PyObject *value)

   Similar to :c:func:`PyModule_AddObjectRef`, but "steals" a reference
   to *value*.
   It can be called with a result of function that returns a new reference
   without bothering to check its result or even saving it to a variable.

   Example usage::

        if (PyModule_Add(module, "spam", PyBytes_FromString(value)) < 0) {
            goto error;
        }

   .. versionadded:: 3.13


.. c:function:: int PyModule_AddObject(PyObject *module, const char *name, PyObject *value)

   Similar to :c:func:`PyModule_AddObjectRef`, but steals a reference to
   *value* on success (if it returns ``0``).

   The new :c:func:`PyModule_Add` or :c:func:`PyModule_AddObjectRef`
   functions are recommended, since it is
   easy to introduce reference leaks by misusing the
   :c:func:`PyModule_AddObject` function.

   .. note::

      Unlike other functions that steal references, ``PyModule_AddObject()``
      only releases the reference to *value* **on success**.

      This means that its return value must be checked, and calling code must
      :c:func:`Py_XDECREF` *value* manually on error.

   Example usage::

        PyObject *obj = PyBytes_FromString(value);
        if (PyModule_AddObject(module, "spam", obj) < 0) {
            // If 'obj' is not NULL and PyModule_AddObject() failed,
            // 'obj' strong reference must be deleted with Py_XDECREF().
            // If 'obj' is NULL, Py_XDECREF() does nothing.
            Py_XDECREF(obj);
            goto error;
        }
        // PyModule_AddObject() stole a reference to obj:
        // Py_XDECREF(obj) is not needed here.

   .. deprecated:: 3.13

      :c:func:`PyModule_AddObject` is :term:`soft deprecated`.


.. c:function:: int PyModule_AddIntConstant(PyObject *module, const char *name, long value)

   Add an integer constant to *module* as *name*.  This convenience function can be
   used from the module's initialization function.
   Return ``-1`` with an exception set on error, ``0`` on success.

   This is a convenience function that calls :c:func:`PyLong_FromLong` and
   :c:func:`PyModule_AddObjectRef`; see their documentation for details.


.. c:function:: int PyModule_AddStringConstant(PyObject *module, const char *name, const char *value)

   Add a string constant to *module* as *name*.  This convenience function can be
   used from the module's initialization function.  The string *value* must be
   ``NULL``-terminated.
   Return ``-1`` with an exception set on error, ``0`` on success.

   This is a convenience function that calls
   :c:func:`PyUnicode_InternFromString` and :c:func:`PyModule_AddObjectRef`;
   see their documentation for details.


.. c:macro:: PyModule_AddIntMacro(module, macro)

   Add an int constant to *module*. The name and the value are taken from
   *macro*. For example ``PyModule_AddIntMacro(module, AF_INET)`` adds the int
   constant *AF_INET* with the value of *AF_INET* to *module*.
   Return ``-1`` with an exception set on error, ``0`` on success.


.. c:macro:: PyModule_AddStringMacro(module, macro)

   Add a string constant to *module*.

.. c:function:: int PyModule_AddType(PyObject *module, PyTypeObject *type)

   Add a type object to *module*.
   The type object is finalized by calling internally :c:func:`PyType_Ready`.
   The name of the type object is taken from the last component of
   :c:member:`~PyTypeObject.tp_name` after dot.
   Return ``-1`` with an exception set on error, ``0`` on success.

   .. versionadded:: 3.9

.. c:function:: int PyModule_AddFunctions(PyObject *module, PyMethodDef *functions)

   Add the functions from the ``NULL`` terminated *functions* array to *module*.
   Refer to the :c:type:`PyMethodDef` documentation for details on individual
   entries (due to the lack of a shared module namespace, module level
   "functions" implemented in C typically receive the module as their first
   parameter, making them similar to instance methods on Python classes).

   This function is called automatically when creating a module from
   ``PyModuleDef`` (such as when using :ref:`multi-phase-initialization`,
   ``PyModule_Create``, or ``PyModule_FromDefAndSpec``).
   Some module authors may prefer defining functions in multiple
   :c:type:`PyMethodDef` arrays; in that case they should call this function
   directly.

   The *functions* array must be statically allocated (or otherwise guaranteed
   to outlive the module object).

   .. versionadded:: 3.5

.. c:function:: int PyModule_SetDocString(PyObject *module, const char *docstring)

   Set the docstring for *module* to *docstring*.
   This function is called automatically when creating a module from
   ``PyModuleDef`` (such as when using :ref:`multi-phase-initialization`,
   ``PyModule_Create``, or ``PyModule_FromDefAndSpec``).

   .. versionadded:: 3.5

.. c:function:: int PyUnstable_Module_SetGIL(PyObject *module, void *gil)

   Indicate that *module* does or does not support running without the global
   interpreter lock (GIL), using one of the values from
   :c:macro:`Py_mod_gil`. It must be called during *module*'s initialization
   function when using :ref:`single-phase-initialization`.
   If this function is not called during module initialization, the
   import machinery assumes the module does not support running without the
   GIL. This function is only available in Python builds configured with
   :option:`--disable-gil`.
   Return ``-1`` with an exception set on error, ``0`` on success.

   .. versionadded:: 3.13


Module lookup (single-phase initialization)
...........................................

The legacy :ref:`single-phase initialization <single-phase-initialization>`
initialization scheme creates singleton modules that can be looked up
in the context of the current interpreter. This allows the module object to be
retrieved later with only a reference to the module definition.

These functions will not work on modules created using multi-phase initialization,
since multiple such modules can be created from a single definition.

.. c:function:: PyObject* PyState_FindModule(PyModuleDef *def)

   Returns the module object that was created from *def* for the current interpreter.
   This method requires that the module object has been attached to the interpreter state with
   :c:func:`PyState_AddModule` beforehand. In case the corresponding module object is not
   found or has not been attached to the interpreter state yet, it returns ``NULL``.

.. c:function:: int PyState_AddModule(PyObject *module, PyModuleDef *def)

   Attaches the module object passed to the function to the interpreter state. This allows
   the module object to be accessible via :c:func:`PyState_FindModule`.

   Only effective on modules created using single-phase initialization.

   Python calls ``PyState_AddModule`` automatically after importing a module
   that uses :ref:`single-phase initialization <single-phase-initialization>`,
   so it is unnecessary (but harmless) to call it from module initialization
   code. An explicit call is needed only if the module's own init code
   subsequently calls ``PyState_FindModule``.
   The function is mainly intended for implementing alternative import
   mechanisms (either by calling it directly, or by referring to its
   implementation for details of the required state updates).

   If a module was attached previously using the same *def*, it is replaced
   by the new *module*.

   The caller must have an :term:`attached thread state`.

   Return ``-1`` with an exception set on error, ``0`` on success.

   .. versionadded:: 3.3

.. c:function:: int PyState_RemoveModule(PyModuleDef *def)

   Removes the module object created from *def* from the interpreter state.
   Return ``-1`` with an exception set on error, ``0`` on success.

   The caller must have an :term:`attached thread state`.

   .. versionadded:: 3.3
