.. highlight:: c

.. _importing:

Importing Modules
=================


.. c:function:: PyObject* PyImport_ImportModule(const char *name)

   .. index::
      single: package variable; __all__
      single: __all__ (package variable)
      single: modules (in module sys)

   This is a wrapper around :c:func:`PyImport_Import()` which takes a
   :c:expr:`const char *` as an argument instead of a :c:expr:`PyObject *`.


.. c:function:: PyObject* PyImport_ImportModuleEx(const char *name, PyObject *globals, PyObject *locals, PyObject *fromlist)

   .. index:: pair: built-in function; __import__

   Import a module.  This is best described by referring to the built-in Python
   function :func:`__import__`.

   The return value is a new reference to the imported module or top-level
   package, or ``NULL`` with an exception set on failure.  Like for
   :func:`__import__`, the return value when a submodule of a package was
   requested is normally the top-level package, unless a non-empty *fromlist*
   was given.

   Failing imports remove incomplete module objects, like with
   :c:func:`PyImport_ImportModule`.


.. c:function:: PyObject* PyImport_ImportModuleLevelObject(PyObject *name, PyObject *globals, PyObject *locals, PyObject *fromlist, int level)

   Import a module.  This is best described by referring to the built-in Python
   function :func:`__import__`, as the standard :func:`__import__` function calls
   this function directly.

   The return value is a new reference to the imported module or top-level package,
   or ``NULL`` with an exception set on failure.  Like for :func:`__import__`,
   the return value when a submodule of a package was requested is normally the
   top-level package, unless a non-empty *fromlist* was given.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyImport_ImportModuleLevel(const char *name, PyObject *globals, PyObject *locals, PyObject *fromlist, int level)

   Similar to :c:func:`PyImport_ImportModuleLevelObject`, but the name is a
   UTF-8 encoded string instead of a Unicode object.

   .. versionchanged:: 3.3
         Negative values for *level* are no longer accepted.

.. c:function:: PyObject* PyImport_Import(PyObject *name)

   This is a higher-level interface that calls the current "import hook
   function" (with an explicit *level* of 0, meaning absolute import).  It
   invokes the :func:`__import__` function from the ``__builtins__`` of the
   current globals.  This means that the import is done using whatever import
   hooks are installed in the current environment.

   This function always uses absolute imports.


.. c:function:: PyObject* PyImport_ReloadModule(PyObject *m)

   Reload a module.  Return a new reference to the reloaded module, or ``NULL`` with
   an exception set on failure (the module still exists in this case).


.. c:function:: PyObject* PyImport_AddModuleRef(const char *name)

   Return the module object corresponding to a module name.

   The *name* argument may be of the form ``package.module``. First check the
   modules dictionary if there's one there, and if not, create a new one and
   insert it in the modules dictionary.

   Return a :term:`strong reference` to the module on success. Return ``NULL``
   with an exception set on failure.

   The module name *name* is decoded from UTF-8.

   This function does not load or import the module; if the module wasn't
   already loaded, you will get an empty module object. Use
   :c:func:`PyImport_ImportModule` or one of its variants to import a module.
   Package structures implied by a dotted name for *name* are not created if
   not already present.

   .. versionadded:: 3.13


.. c:function:: PyObject* PyImport_AddModuleObject(PyObject *name)

   Similar to :c:func:`PyImport_AddModuleRef`, but return a :term:`borrowed
   reference` and *name* is a Python :class:`str` object.

   .. versionadded:: 3.3


.. c:function:: PyObject* PyImport_AddModule(const char *name)

   Similar to :c:func:`PyImport_AddModuleRef`, but return a :term:`borrowed
   reference`.


.. c:function:: PyObject* PyImport_ExecCodeModule(const char *name, PyObject *co)

   .. index:: pair: built-in function; compile

   Given a module name (possibly of the form ``package.module``) and a code object
   read from a Python bytecode file or obtained from the built-in function
   :func:`compile`, load the module.  Return a new reference to the module object,
   or ``NULL`` with an exception set if an error occurred.  *name*
   is removed from :data:`sys.modules` in error cases, even if *name* was already
   in :data:`sys.modules` on entry to :c:func:`PyImport_ExecCodeModule`.  Leaving
   incompletely initialized modules in :data:`sys.modules` is dangerous, as imports of
   such modules have no way to know that the module object is an unknown (and
   probably damaged with respect to the module author's intents) state.

   The module's :attr:`~module.__spec__` and :attr:`~module.__loader__` will be
   set, if not set already, with the appropriate values.  The spec's loader
   will be set to the module's :attr:`!__loader__` (if set) and to an instance
   of :class:`~importlib.machinery.SourceFileLoader` otherwise.

   The module's :attr:`~module.__file__` attribute will be set to the code
   object's :attr:`~codeobject.co_filename`.

   This function will reload the module if it was already imported.  See
   :c:func:`PyImport_ReloadModule` for the intended way to reload a module.

   If *name* points to a dotted name of the form ``package.module``, any package
   structures not already created will still not be created.

   See also :c:func:`PyImport_ExecCodeModuleEx` and
   :c:func:`PyImport_ExecCodeModuleWithPathnames`.

   .. versionchanged:: 3.12
      The setting of ``__cached__`` and :attr:`~module.__loader__`
      is deprecated. See :class:`~importlib.machinery.ModuleSpec` for
      alternatives.

   .. versionchanged:: 3.15
      ``__cached__`` is no longer set.


.. c:function:: PyObject* PyImport_ExecCodeModuleEx(const char *name, PyObject *co, const char *pathname)

   Like :c:func:`PyImport_ExecCodeModule`, but the :attr:`~module.__file__`
   attribute of the module object is set to *pathname* if it is non-``NULL``.

   See also :c:func:`PyImport_ExecCodeModuleWithPathnames`.


.. c:function:: PyObject* PyImport_ExecCodeModuleObject(PyObject *name, PyObject *co, PyObject *pathname, PyObject *cpathname)

   Like :c:func:`PyImport_ExecCodeModuleEx`, but the path to any compiled file
   via *cpathname* is used appropriately when non-``NULL``.  Of the three
   functions, this is the preferred one to use.

   .. versionadded:: 3.3

   .. versionchanged:: 3.12
      Setting ``__cached__`` is deprecated. See
      :class:`~importlib.machinery.ModuleSpec` for alternatives.

   .. versionchanged:: 3.15
      ``__cached__`` no longer set.


.. c:function:: PyObject* PyImport_ExecCodeModuleWithPathnames(const char *name, PyObject *co, const char *pathname, const char *cpathname)

   Like :c:func:`PyImport_ExecCodeModuleObject`, but *name*, *pathname* and
   *cpathname* are UTF-8 encoded strings. Attempts are also made to figure out
   what the value for *pathname* should be from *cpathname* if the former is
   set to ``NULL``.

   .. versionadded:: 3.2
   .. versionchanged:: 3.3
      Uses :func:`!imp.source_from_cache` in calculating the source path if
      only the bytecode path is provided.
   .. versionchanged:: 3.12
      No longer uses the removed :mod:`!imp` module.


.. c:function:: long PyImport_GetMagicNumber()

   Return the magic number for Python bytecode files (a.k.a. :file:`.pyc` file).
   The magic number should be present in the first four bytes of the bytecode
   file, in little-endian byte order. Returns ``-1`` on error.

   .. versionchanged:: 3.3
      Return value of ``-1`` upon failure.


.. c:function:: const char * PyImport_GetMagicTag()

   Return the magic tag string for :pep:`3147` format Python bytecode file
   names.  Keep in mind that the value at ``sys.implementation.cache_tag`` is
   authoritative and should be used instead of this function.

   .. versionadded:: 3.2

.. c:function:: PyObject* PyImport_GetModuleDict()

   Return the dictionary used for the module administration (a.k.a.
   ``sys.modules``).  Note that this is a per-interpreter variable.

.. c:function:: PyObject* PyImport_GetModule(PyObject *name)

   Return the already imported module with the given name.  If the
   module has not been imported yet then returns ``NULL`` but does not set
   an error.  Returns ``NULL`` and sets an error if the lookup failed.

   .. versionadded:: 3.7

.. c:function:: PyObject* PyImport_GetImporter(PyObject *path)

   Return a finder object for a :data:`sys.path`/:attr:`!pkg.__path__` item
   *path*, possibly by fetching it from the :data:`sys.path_importer_cache`
   dict.  If it wasn't yet cached, traverse :data:`sys.path_hooks` until a hook
   is found that can handle the path item.  Return ``None`` if no hook could;
   this tells our caller that the :term:`path based finder` could not find a
   finder for this path item. Cache the result in :data:`sys.path_importer_cache`.
   Return a new reference to the finder object.


.. c:function:: int PyImport_ImportFrozenModuleObject(PyObject *name)

   Load a frozen module named *name*.  Return ``1`` for success, ``0`` if the
   module is not found, and ``-1`` with an exception set if the initialization
   failed.  To access the imported module on a successful load, use
   :c:func:`PyImport_ImportModule`.  (Note the misnomer --- this function would
   reload the module if it was already imported.)

   .. versionadded:: 3.3

   .. versionchanged:: 3.4
      The ``__file__`` attribute is no longer set on the module.


.. c:function:: int PyImport_ImportFrozenModule(const char *name)

   Similar to :c:func:`PyImport_ImportFrozenModuleObject`, but the name is a
   UTF-8 encoded string instead of a Unicode object.


.. c:struct:: _frozen

   .. index:: single: freeze utility

   This is the structure type definition for frozen module descriptors, as
   generated by the :program:`freeze` utility (see :file:`Tools/freeze/` in the
   Python source distribution).  Its definition, found in :file:`Include/import.h`,
   is::

      struct _frozen {
          const char *name;
          const unsigned char *code;
          int size;
          bool is_package;
      };

   .. versionchanged:: 3.11
      The new ``is_package`` field indicates whether the module is a package or not.
      This replaces setting the ``size`` field to a negative value.

.. c:var:: const struct _frozen* PyImport_FrozenModules

   This pointer is initialized to point to an array of :c:struct:`_frozen`
   records, terminated by one whose members are all ``NULL`` or zero.  When a frozen
   module is imported, it is searched in this table.  Third-party code could play
   tricks with this to provide a dynamically created collection of frozen modules.


.. c:function:: int PyImport_AppendInittab(const char *name, PyObject* (*initfunc)(void))

   Add a single module to the existing table of built-in modules.  This is a
   convenience wrapper around :c:func:`PyImport_ExtendInittab`, returning ``-1`` if
   the table could not be extended.  The new module can be imported by the name
   *name*, and uses the function *initfunc* as the initialization function called
   on the first attempted import.  This should be called before
   :c:func:`Py_Initialize`.


.. c:struct:: _inittab

   Structure describing a single entry in the list of built-in modules.
   Programs which
   embed Python may use an array of these structures in conjunction with
   :c:func:`PyImport_ExtendInittab` to provide additional built-in modules.
   The structure consists of two members:

   .. c:member:: const char *name

      The module name, as an ASCII encoded string.

   .. c:member:: PyObject* (*initfunc)(void)

      Initialization function for a module built into the interpreter.


.. c:function:: int PyImport_ExtendInittab(struct _inittab *newtab)

   Add a collection of modules to the table of built-in modules.  The *newtab*
   array must end with a sentinel entry which contains ``NULL`` for the :c:member:`~_inittab.name`
   field; failure to provide the sentinel value can result in a memory fault.
   Returns ``0`` on success or ``-1`` if insufficient memory could be allocated to
   extend the internal table.  In the event of failure, no modules are added to the
   internal table.  This must be called before :c:func:`Py_Initialize`.

   If Python is initialized multiple times, :c:func:`PyImport_AppendInittab` or
   :c:func:`PyImport_ExtendInittab` must be called before each Python
   initialization.


.. c:var:: struct _inittab *PyImport_Inittab

   The table of built-in modules used by Python initialization. Do not use this directly;
   use :c:func:`PyImport_AppendInittab` and :c:func:`PyImport_ExtendInittab`
   instead.


.. c:function:: PyObject* PyImport_ImportModuleAttr(PyObject *mod_name, PyObject *attr_name)

   Import the module *mod_name* and get its attribute *attr_name*.

   Names must be Python :class:`str` objects.

   Helper function combining :c:func:`PyImport_Import` and
   :c:func:`PyObject_GetAttr`. For example, it can raise :exc:`ImportError` if
   the module is not found, and :exc:`AttributeError` if the attribute doesn't
   exist.

   .. versionadded:: 3.14

.. c:function:: PyObject* PyImport_ImportModuleAttrString(const char *mod_name, const char *attr_name)

   Similar to :c:func:`PyImport_ImportModuleAttr`, but names are UTF-8 encoded
   strings instead of Python :class:`str` objects.

   .. versionadded:: 3.14

.. c:function:: PyImport_LazyImportsMode PyImport_GetLazyImportsMode()

   Gets the current lazy imports mode.

   .. versionadded:: next

.. c:function:: PyObject* PyImport_GetLazyImportsFilter()

   Return a :term:`strong reference` to the current lazy imports filter,
   or ``NULL`` if none exists. This function always succeeds.

   .. versionadded:: next

.. c:function:: int PyImport_SetLazyImportsMode(PyImport_LazyImportsMode mode)

   Similar to :c:func:`PyImport_ImportModuleAttr`, but names are UTF-8 encoded
   strings instead of Python :class:`str` objects.

   This function always returns ``0``.

   .. versionadded:: next

.. c:function:: int PyImport_SetLazyImportsFilter(PyObject *filter)

   Sets the current lazy imports filter. The *filter* should be a callable that
   will receive ``(importing_module_name, imported_module_name, [fromlist])``
   when an import can potentially be lazy and that must return ``True`` if
   the import should be lazy and ``False`` otherwise.

   Return ``0`` on success and ``-1`` with an exception set otherwise.

   .. versionadded:: next

.. c:type:: PyImport_LazyImportsMode

   Enumeration of possible lazy import modes.

   .. c:enumerator:: PyImport_LAZY_NORMAL

      Respect the ``lazy`` keyword in source code. This is the default mode.

   .. c:enumerator:: PyImport_LAZY_ALL

      Make all imports lazy by default.

   .. c:enumerator:: PyImport_LAZY_NONE

      Disable lazy imports entirely. Even explicit ``lazy`` statements become
      eager imports.

   .. versionadded:: next

.. c:function:: PyObject* PyImport_CreateModuleFromInitfunc(PyObject *spec, PyObject* (*initfunc)(void))

   This function is a building block that enables embedders to implement
   the :py:meth:`~importlib.abc.Loader.create_module` step of custom
   static extension importers (e.g. of statically-linked extensions).

   *spec* must be a :class:`~importlib.machinery.ModuleSpec` object.

   *initfunc* must be an :ref:`initialization function <extension-export-hook>`,
   the same as for :c:func:`PyImport_AppendInittab`.

   On success, create and return a module object.
   This module will not be initialized; call :c:func:`PyModule_Exec`
   to initialize it.
   (Custom importers should do this in their
   :py:meth:`~importlib.abc.Loader.exec_module` method.)

   On error, return NULL with an exception set.

   .. versionadded:: 3.15
