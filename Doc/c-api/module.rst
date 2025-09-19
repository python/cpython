.. highlight:: c

.. _moduleobjects:

Module Objects
--------------

.. index:: pair: object; module


.. c:var:: PyTypeObject PyModule_Type

   .. index:: single: ModuleType (in module types)

   This instance of :c:type:`PyTypeObject` represents the Python module type.  This
   is exposed to Python programs as ``types.ModuleType``.


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

.. c:function:: void* PyModule_GetState(PyObject *module)

   Return the "state" of the module, that is, a pointer to the block of memory
   allocated at module creation time, or ``NULL``.  See
   :c:member:`PyModuleDef.m_size`.


.. c:function:: PyModuleDef* PyModule_GetDef(PyObject *module)

   Return a pointer to the :c:type:`PyModuleDef` struct from which the module was
   created, or ``NULL`` if the module wasn't created from a definition.


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

   .. deprecated:: 3.2
      :c:func:`PyModule_GetFilename` raises :exc:`UnicodeEncodeError` on
      unencodable filenames, use :c:func:`PyModule_GetFilenameObject` instead.


.. _pymoduledef:

Module definitions
------------------

The functions in the previous section work on any module object, including
modules imported from Python code.

Modules defined using the C API typically use a *module definition*,
:c:type:`PyModuleDef` -- a statically allocated, constant “description" of
how a module should be created.

The definition is usually used to define an extension's “main” module object
(see :ref:`extension-modules` for details).
It is also used to
:ref:`create extension modules dynamically <moduledef-dynamic>`.

Unlike :c:func:`PyModule_New`, the definition allows management of
*module state* -- a piece of memory that is allocated and cleared together
with the module object.
Unlike the module's Python attributes, Python code cannot replace or delete
data stored in module state.

.. c:type:: PyModuleDef

   The module definition struct, which holds all information needed to create
   a module object.
   This structure must be statically allocated (or be otherwise guaranteed
   to be valid while any modules created from it exist).
   Usually, there is only one variable of this type for each extension module.

   .. c:member:: PyModuleDef_Base m_base

      Always initialize this member to :c:macro:`PyModuleDef_HEAD_INIT`.

   .. c:member:: const char *m_name

      Name for the new module.

   .. c:member:: const char *m_doc

      Docstring for the module; usually a docstring variable created with
      :c:macro:`PyDoc_STRVAR` is used.

   .. c:member:: Py_ssize_t m_size

      Module state may be kept in a per-module memory area that can be
      retrieved with :c:func:`PyModule_GetState`, rather than in static globals.
      This makes modules safe for use in multiple sub-interpreters.

      This memory area is allocated based on *m_size* on module creation,
      and freed when the module object is deallocated, after the
      :c:member:`~PyModuleDef.m_free` function has been called, if present.

      Setting it to a non-negative value means that the module can be
      re-initialized and specifies the additional amount of memory it requires
      for its state.

      Setting ``m_size`` to ``-1`` means that the module does not support
      sub-interpreters, because it has global state.
      Negative ``m_size`` is only allowed when using
      :ref:`legacy single-phase initialization <single-phase-initialization>`
      or when :ref:`creating modules dynamically <moduledef-dynamic>`.

      See :PEP:`3121` for more details.

   .. c:member:: PyMethodDef* m_methods

      A pointer to a table of module-level functions, described by
      :c:type:`PyMethodDef` values.  Can be ``NULL`` if no functions are present.

   .. c:member:: PyModuleDef_Slot* m_slots

      An array of slot definitions for multi-phase initialization, terminated by
      a ``{0, NULL}`` entry.
      When using legacy single-phase initialization, *m_slots* must be ``NULL``.

      .. versionchanged:: 3.5

         Prior to version 3.5, this member was always set to ``NULL``,
         and was defined as:

           .. c:member:: inquiry m_reload

   .. c:member:: traverseproc m_traverse

      A traversal function to call during GC traversal of the module object, or
      ``NULL`` if not needed.

      This function is not called if the module state was requested but is not
      allocated yet. This is the case immediately after the module is created
      and before the module is executed (:c:data:`Py_mod_exec` function). More
      precisely, this function is not called if :c:member:`~PyModuleDef.m_size` is greater
      than 0 and the module state (as returned by :c:func:`PyModule_GetState`)
      is ``NULL``.

      .. versionchanged:: 3.9
         No longer called before the module state is allocated.

   .. c:member:: inquiry m_clear

      A clear function to call during GC clearing of the module object, or
      ``NULL`` if not needed.

      This function is not called if the module state was requested but is not
      allocated yet. This is the case immediately after the module is created
      and before the module is executed (:c:data:`Py_mod_exec` function). More
      precisely, this function is not called if :c:member:`~PyModuleDef.m_size` is greater
      than 0 and the module state (as returned by :c:func:`PyModule_GetState`)
      is ``NULL``.

      Like :c:member:`PyTypeObject.tp_clear`, this function is not *always*
      called before a module is deallocated. For example, when reference
      counting is enough to determine that an object is no longer used,
      the cyclic garbage collector is not involved and
      :c:member:`~PyModuleDef.m_free` is called directly.

      .. versionchanged:: 3.9
         No longer called before the module state is allocated.

   .. c:member:: freefunc m_free

      A function to call during deallocation of the module object, or ``NULL``
      if not needed.

      This function is not called if the module state was requested but is not
      allocated yet. This is the case immediately after the module is created
      and before the module is executed (:c:data:`Py_mod_exec` function). More
      precisely, this function is not called if :c:member:`~PyModuleDef.m_size` is greater
      than 0 and the module state (as returned by :c:func:`PyModule_GetState`)
      is ``NULL``.

      .. versionchanged:: 3.9
         No longer called before the module state is allocated.


Module slots
............

.. c:type:: PyModuleDef_Slot

   .. c:member:: int slot

      A slot ID, chosen from the available values explained below.

   .. c:member:: void* value

      Value of the slot, whose meaning depends on the slot ID.

   .. versionadded:: 3.5

The available slot types are:

.. c:macro:: Py_mod_create

   Specifies a function that is called to create the module object itself.
   The *value* pointer of this slot must point to a function of the signature:

   .. c:function:: PyObject* create_module(PyObject *spec, PyModuleDef *def)
      :no-index-entry:
      :no-contents-entry:

   The function receives a :py:class:`~importlib.machinery.ModuleSpec`
   instance, as defined in :PEP:`451`, and the module definition.
   It should return a new module object, or set an error
   and return ``NULL``.

   This function should be kept minimal. In particular, it should not
   call arbitrary Python code, as trying to import the same module again may
   result in an infinite loop.

   Multiple ``Py_mod_create`` slots may not be specified in one module
   definition.

   If ``Py_mod_create`` is not specified, the import machinery will create
   a normal module object using :c:func:`PyModule_New`. The name is taken from
   *spec*, not the definition, to allow extension modules to dynamically adjust
   to their place in the module hierarchy and be imported under different
   names through symlinks, all while sharing a single module definition.

   There is no requirement for the returned object to be an instance of
   :c:type:`PyModule_Type`. Any type can be used, as long as it supports
   setting and getting import-related attributes.
   However, only ``PyModule_Type`` instances may be returned if the
   ``PyModuleDef`` has non-``NULL`` ``m_traverse``, ``m_clear``,
   ``m_free``; non-zero ``m_size``; or slots other than ``Py_mod_create``.

.. c:macro:: Py_mod_exec

   Specifies a function that is called to *execute* the module.
   This is equivalent to executing the code of a Python module: typically,
   this function adds classes and constants to the module.
   The signature of the function is:

   .. c:function:: int exec_module(PyObject* module)
      :no-index-entry:
      :no-contents-entry:

   If multiple ``Py_mod_exec`` slots are specified, they are processed in the
   order they appear in the *m_slots* array.

.. c:macro:: Py_mod_multiple_interpreters

   Specifies one of the following values:

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

   Multiple ``Py_mod_multiple_interpreters`` slots may not be specified
   in one module definition.

   If ``Py_mod_multiple_interpreters`` is not specified, the import
   machinery defaults to ``Py_MOD_MULTIPLE_INTERPRETERS_SUPPORTED``.

   .. versionadded:: 3.12

.. c:macro:: Py_mod_gil

   Specifies one of the following values:

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

   Multiple ``Py_mod_gil`` slots may not be specified in one module definition.

   If ``Py_mod_gil`` is not specified, the import machinery defaults to
   ``Py_MOD_GIL_USED``.

   .. versionadded:: 3.13


.. _moduledef-dynamic:

Creating extension modules dynamically
--------------------------------------

The following functions may be used to create a module outside of an
extension's :ref:`initialization function <extension-export-hook>`.
They are also used in
:ref:`single-phase initialization <single-phase-initialization>`.

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


Support functions
-----------------

The following functions are provided to help initialize a module
state.
They are intended for a module's execution slots (:c:data:`Py_mod_exec`),
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
