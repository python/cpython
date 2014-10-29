:mod:`importlib` -- The implementation of :keyword:`import`
===========================================================

.. module:: importlib
   :synopsis: The implementation of the import machinery.

.. moduleauthor:: Brett Cannon <brett@python.org>
.. sectionauthor:: Brett Cannon <brett@python.org>

.. versionadded:: 3.1


Introduction
------------

The purpose of the :mod:`importlib` package is two-fold. One is to provide the
implementation of the :keyword:`import` statement (and thus, by extension, the
:func:`__import__` function) in Python source code. This provides an
implementation of :keyword:`import` which is portable to any Python
interpreter. This also provides an implementation which is easier to
comprehend than one implemented in a programming language other than Python.

Two, the components to implement :keyword:`import` are exposed in this
package, making it easier for users to create their own custom objects (known
generically as an :term:`importer`) to participate in the import process.

.. seealso::

    :ref:`import`
        The language reference for the :keyword:`import` statement.

    `Packages specification <http://legacy.python.org/doc/essays/packages.html>`__
        Original specification of packages. Some semantics have changed since
        the writing of this document (e.g. redirecting based on ``None``
        in :data:`sys.modules`).

    The :func:`.__import__` function
        The :keyword:`import` statement is syntactic sugar for this function.

    :pep:`235`
        Import on Case-Insensitive Platforms

    :pep:`263`
        Defining Python Source Code Encodings

    :pep:`302`
        New Import Hooks

    :pep:`328`
        Imports: Multi-Line and Absolute/Relative

    :pep:`366`
        Main module explicit relative imports

    :pep:`451`
        A ModuleSpec Type for the Import System

    :pep:`3120`
        Using UTF-8 as the Default Source Encoding

    :pep:`3147`
        PYC Repository Directories


Functions
---------

.. function:: __import__(name, globals=None, locals=None, fromlist=(), level=0)

    An implementation of the built-in :func:`__import__` function.

.. function:: import_module(name, package=None)

    Import a module. The *name* argument specifies what module to
    import in absolute or relative terms
    (e.g. either ``pkg.mod`` or ``..mod``). If the name is
    specified in relative terms, then the *package* argument must be set to
    the name of the package which is to act as the anchor for resolving the
    package name (e.g. ``import_module('..mod', 'pkg.subpkg')`` will import
    ``pkg.mod``).

    The :func:`import_module` function acts as a simplifying wrapper around
    :func:`importlib.__import__`. This means all semantics of the function are
    derived from :func:`importlib.__import__`, including requiring the package
    from which an import is occurring to have been previously imported
    (i.e., *package* must already be imported). The most important difference
    is that :func:`import_module` returns the specified package or module
    (e.g. ``pkg.mod``), while :func:`__import__` returns the
    top-level package or module (e.g. ``pkg``).

    .. versionchanged:: 3.3
       Parent packages are automatically imported.

.. function:: find_loader(name, path=None)

   Find the loader for a module, optionally within the specified *path*. If the
   module is in :attr:`sys.modules`, then ``sys.modules[name].__loader__`` is
   returned (unless the loader would be ``None`` or is not set, in which case
   :exc:`ValueError` is raised). Otherwise a search using :attr:`sys.meta_path`
   is done. ``None`` is returned if no loader is found.

   A dotted name does not have its parent's implicitly imported as that requires
   loading them and that may not be desired. To properly import a submodule you
   will need to import all parent packages of the submodule and use the correct
   argument to *path*.

   .. versionadded:: 3.3

   .. versionchanged:: 3.4
      If ``__loader__`` is not set, raise :exc:`ValueError`, just like when the
      attribute is set to ``None``.

   .. deprecated:: 3.4
      Use :func:`importlib.util.find_spec` instead.

.. function:: invalidate_caches()

   Invalidate the internal caches of finders stored at
   :data:`sys.meta_path`. If a finder implements ``invalidate_caches()`` then it
   will be called to perform the invalidation.  This function should be called
   if any modules are created/installed while your program is running to
   guarantee all finders will notice the new module's existence.

   .. versionadded:: 3.3

.. function:: reload(module)

   Reload a previously imported *module*.  The argument must be a module object,
   so it must have been successfully imported before.  This is useful if you
   have edited the module source file using an external editor and want to try
   out the new version without leaving the Python interpreter.  The return value
   is the module object (which can be different if re-importing causes a
   different object to be placed in :data:`sys.modules`).

   When :func:`reload` is executed:

   * Python module's code is recompiled and the module-level code re-executed,
     defining a new set of objects which are bound to names in the module's
     dictionary by reusing the :term:`loader` which originally loaded the
     module.  The ``init`` function of extension modules is not called a second
     time.

   * As with all other objects in Python the old objects are only reclaimed
     after their reference counts drop to zero.

   * The names in the module namespace are updated to point to any new or
     changed objects.

   * Other references to the old objects (such as names external to the module) are
     not rebound to refer to the new objects and must be updated in each namespace
     where they occur if that is desired.

   There are a number of other caveats:

   If a module is syntactically correct but its initialization fails, the first
   :keyword:`import` statement for it does not bind its name locally, but does
   store a (partially initialized) module object in ``sys.modules``.  To reload
   the module you must first :keyword:`import` it again (this will bind the name
   to the partially initialized module object) before you can :func:`reload` it.

   When a module is reloaded, its dictionary (containing the module's global
   variables) is retained.  Redefinitions of names will override the old
   definitions, so this is generally not a problem.  If the new version of a
   module does not define a name that was defined by the old version, the old
   definition remains.  This feature can be used to the module's advantage if it
   maintains a global table or cache of objects --- with a :keyword:`try`
   statement it can test for the table's presence and skip its initialization if
   desired::

      try:
          cache
      except NameError:
          cache = {}

   It is legal though generally not very useful to reload built-in or
   dynamically loaded modules (this is not true for e.g. :mod:`sys`,
   :mod:`__main__`, :mod:`builtins` and other key modules where reloading is
   frowned upon). In many cases, however, extension modules are not designed to
   be initialized more than once, and may fail in arbitrary ways when reloaded.

   If a module imports objects from another module using :keyword:`from` ...
   :keyword:`import` ..., calling :func:`reload` for the other module does not
   redefine the objects imported from it --- one way around this is to
   re-execute the :keyword:`from` statement, another is to use :keyword:`import`
   and qualified names (*module.name*) instead.

   If a module instantiates instances of a class, reloading the module that
   defines the class does not affect the method definitions of the instances ---
   they continue to use the old class definition.  The same is true for derived
   classes.

   .. versionadded:: 3.4


:mod:`importlib.abc` -- Abstract base classes related to import
---------------------------------------------------------------

.. module:: importlib.abc
    :synopsis: Abstract base classes related to import

The :mod:`importlib.abc` module contains all of the core abstract base classes
used by :keyword:`import`. Some subclasses of the core abstract base classes
are also provided to help in implementing the core ABCs.

ABC hierarchy::

    object
     +-- Finder (deprecated)
     |    +-- MetaPathFinder
     |    +-- PathEntryFinder
     +-- Loader
          +-- ResourceLoader --------+
          +-- InspectLoader          |
               +-- ExecutionLoader --+
                                     +-- FileLoader
                                     +-- SourceLoader


.. class:: Finder

   An abstract base class representing a :term:`finder`.

   .. deprecated:: 3.3
      Use :class:`MetaPathFinder` or :class:`PathEntryFinder` instead.

   .. method:: find_module(fullname, path=None)

      An abstact method for finding a :term:`loader` for the specified
      module.  Originally specified in :pep:`302`, this method was meant
      for use in :data:`sys.meta_path` and in the path-based import subsystem.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of raising
         :exc:`NotImplementedError`.


.. class:: MetaPathFinder

   An abstract base class representing a :term:`meta path finder`. For
   compatibility, this is a subclass of :class:`Finder`.

   .. versionadded:: 3.3

   .. method:: find_spec(fullname, path, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  If this is a top-level import, *path* will
      be ``None``.  Otherwise, this is a search for a subpackage or
      module and *path* will be the value of :attr:`__path__` from the
      parent package. If a spec cannot be found, ``None`` is returned.
      When passed in, ``target`` is a module object that the finder may
      use to make a more educated about what spec to return.

      .. versionadded:: 3.4

   .. method:: find_module(fullname, path)

      A legacy method for finding a :term:`loader` for the specified
      module.  If this is a top-level import, *path* will be ``None``.
      Otherwise, this is a search for a subpackage or module and *path*
      will be the value of :attr:`__path__` from the parent
      package. If a loader cannot be found, ``None`` is returned.

      If :meth:`find_spec` is defined, backwards-compatible functionality is
      provided.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of raising
         :exc:`NotImplementedError`. Can use :meth:`find_spec` to provide
         functionality.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: invalidate_caches()

      An optional method which, when called, should invalidate any internal
      cache used by the finder. Used by :func:`importlib.invalidate_caches`
      when invalidating the caches of all finders on :data:`sys.meta_path`.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of ``NotImplemented``.


.. class:: PathEntryFinder

   An abstract base class representing a :term:`path entry finder`.  Though
   it bears some similarities to :class:`MetaPathFinder`, ``PathEntryFinder``
   is meant for use only within the path-based import subsystem provided
   by :class:`PathFinder`. This ABC is a subclass of :class:`Finder` for
   compatibility reasons only.

   .. versionadded:: 3.3

   .. method:: find_spec(fullname, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  The finder will search for the module only
      within the :term:`path entry` to which it is assigned.  If a spec
      cannot be found, ``None`` is returned.  When passed in, ``target``
      is a module object that the finder may use to make a more educated
      about what spec to return.

      .. versionadded:: 3.4

   .. method:: find_loader(fullname)

      A legacy method for finding a :term:`loader` for the specified
      module.  Returns a 2-tuple of ``(loader, portion)`` where ``portion``
      is a sequence of file system locations contributing to part of a namespace
      package. The loader may be ``None`` while specifying ``portion`` to
      signify the contribution of the file system locations to a namespace
      package. An empty list can be used for ``portion`` to signify the loader
      is not part of a namespace package. If ``loader`` is ``None`` and
      ``portion`` is the empty list then no loader or location for a namespace
      package were found (i.e. failure to find anything for the module).

      If :meth:`find_spec` is defined then backwards-compatible functionality is
      provided.

      .. versionchanged:: 3.4
         Returns ``(None, [])`` instead of raising :exc:`NotImplementedError`.
         Uses :meth:`find_spec` when available to provide functionality.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: find_module(fullname)

      A concrete implementation of :meth:`Finder.find_module` which is
      equivalent to ``self.find_loader(fullname)[0]``.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. method:: invalidate_caches()

      An optional method which, when called, should invalidate any internal
      cache used by the finder. Used by :meth:`PathFinder.invalidate_caches`
      when invalidating the caches of all cached finders.


.. class:: Loader

    An abstract base class for a :term:`loader`.
    See :pep:`302` for the exact definition for a loader.

    .. method:: create_module(spec)

       An optional method that returns the module object to use when
       importing a module.  create_module() may also return ``None``,
       indicating that the default module creation should take place
       instead.

       .. versionadded:: 3.4

    .. method:: exec_module(module)

       An abstract method that executes the module in its own namespace
       when a module is imported or reloaded.  The module should already
       be initialized when exec_module() is called.

       .. versionadded:: 3.4

    .. method:: load_module(fullname)

        A legacy method for loading a module. If the module cannot be
        loaded, :exc:`ImportError` is raised, otherwise the loaded module is
        returned.

        If the requested module already exists in :data:`sys.modules`, that
        module should be used and reloaded.
        Otherwise the loader should create a new module and insert it into
        :data:`sys.modules` before any loading begins, to prevent recursion
        from the import. If the loader inserted a module and the load fails, it
        must be removed by the loader from :data:`sys.modules`; modules already
        in :data:`sys.modules` before the loader began execution should be left
        alone (see :func:`importlib.util.module_for_loader`).

        The loader should set several attributes on the module.
        (Note that some of these attributes can change when a module is
        reloaded):

        - :attr:`__name__`
            The name of the module.

        - :attr:`__file__`
            The path to where the module data is stored (not set for built-in
            modules).

        - :attr:`__cached__`
            The path to where a compiled version of the module is/should be
            stored (not set when the attribute would be inappropriate).

        - :attr:`__path__`
            A list of strings specifying the search path within a
            package. This attribute is not set on modules.

        - :attr:`__package__`
            The parent package for the module/package. If the module is
            top-level then it has a value of the empty string. The
            :func:`importlib.util.module_for_loader` decorator can handle the
            details for :attr:`__package__`.

        - :attr:`__loader__`
            The loader used to load the module. The
            :func:`importlib.util.module_for_loader` decorator can handle the
            details for :attr:`__package__`.

        When :meth:`exec_module` is available then backwards-compatible
        functionality is provided.

        .. versionchanged:: 3.4
           Raise :exc:`ImportError` when called instead of
           :exc:`NotImplementedError`. Functionality provided when
           :meth:`exec_module` is available.

        .. deprecated:: 3.4
           The recommended API for loading a module is :meth:`exec_module`
           (and optionally :meth:`create_module`).  Loaders should implement
           it instead of load_module().  The import machinery takes care of
           all the other responsibilities of load_module() when exec_module()
           is implemented.

    .. method:: module_repr(module)

        A legacy method which when implemented calculates and returns the
        given module's repr, as a string. The module type's default repr() will
        use the result of this method as appropriate.

        .. versionadded:: 3.3

        .. versionchanged:: 3.4
           Made optional instead of an abstractmethod.

        .. deprecated:: 3.4
           The import machinery now takes care of this automatically.


.. class:: ResourceLoader

    An abstract base class for a :term:`loader` which implements the optional
    :pep:`302` protocol for loading arbitrary resources from the storage
    back-end.

    .. method:: get_data(path)

        An abstract method to return the bytes for the data located at *path*.
        Loaders that have a file-like storage back-end
        that allows storing arbitrary data
        can implement this abstract method to give direct access
        to the data stored. :exc:`OSError` is to be raised if the *path* cannot
        be found. The *path* is expected to be constructed using a module's
        :attr:`__file__` attribute or an item from a package's :attr:`__path__`.

        .. versionchanged:: 3.4
           Raises :exc:`OSError` instead of :exc:`NotImplementedError`.


.. class:: InspectLoader

    An abstract base class for a :term:`loader` which implements the optional
    :pep:`302` protocol for loaders that inspect modules.

    .. method:: get_code(fullname)

        Return the code object for a module, or ``None`` if the module does not
        have a code object (as would be the case, for example, for a built-in
        module).  Raise an :exc:`ImportError` if loader cannot find the
        requested module.

        .. note::
           While the method has a default implementation, it is suggested that
           it be overridden if possible for performance.

        .. index::
           single: universal newlines; importlib.abc.InspectLoader.get_source method

        .. versionchanged:: 3.4
           No longer abstract and a concrete implementation is provided.

    .. method:: get_source(fullname)

        An abstract method to return the source of a module. It is returned as
        a text string using :term:`universal newlines`, translating all
        recognized line separators into ``'\n'`` characters.  Returns ``None``
        if no source is available (e.g. a built-in module). Raises
        :exc:`ImportError` if the loader cannot find the module specified.

        .. versionchanged:: 3.4
           Raises :exc:`ImportError` instead of :exc:`NotImplementedError`.

    .. method:: is_package(fullname)

        An abstract method to return a true value if the module is a package, a
        false value otherwise. :exc:`ImportError` is raised if the
        :term:`loader` cannot find the module.

        .. versionchanged:: 3.4
           Raises :exc:`ImportError` instead of :exc:`NotImplementedError`.

    .. method:: source_to_code(data, path='<string>')

        Create a code object from Python source.

        The *data* argument can be whatever the :func:`compile` function
        supports (i.e. string or bytes). The *path* argument should be
        the "path" to where the source code originated from, which can be an
        abstract concept (e.g. location in a zip file).

        .. versionadded:: 3.4

    .. method:: exec_module(module)

       Implementation of :meth:`Loader.exec_module`.

       .. versionadded:: 3.4

    .. method:: load_module(fullname)

       Implementation of :meth:`Loader.load_module`.

       .. deprecated:: 3.4
          use :meth:`exec_module` instead.


.. class:: ExecutionLoader

    An abstract base class which inherits from :class:`InspectLoader` that,
    when implemented, helps a module to be executed as a script. The ABC
    represents an optional :pep:`302` protocol.

    .. method:: get_filename(fullname)

        An abstract method that is to return the value of :attr:`__file__` for
        the specified module. If no path is available, :exc:`ImportError` is
        raised.

        If source code is available, then the method should return the path to
        the source file, regardless of whether a bytecode was used to load the
        module.

        .. versionchanged:: 3.4
           Raises :exc:`ImportError` instead of :exc:`NotImplementedError`.


.. class:: FileLoader(fullname, path)

   An abstract base class which inherits from :class:`ResourceLoader` and
   :class:`ExecutionLoader`, providing concrete implementations of
   :meth:`ResourceLoader.get_data` and :meth:`ExecutionLoader.get_filename`.

   The *fullname* argument is a fully resolved name of the module the loader is
   to handle. The *path* argument is the path to the file for the module.

   .. versionadded:: 3.3

   .. attribute:: name

      The name of the module the loader can handle.

   .. attribute:: path

      Path to the file of the module.

   .. method:: load_module(fullname)

      Calls super's ``load_module()``.

      .. deprecated:: 3.4
         Use :meth:`Loader.exec_module` instead.

   .. method:: get_filename(fullname)

      Returns :attr:`path`.

   .. method:: get_data(path)

      Reads *path* as a binary file and returns the bytes from it.


.. class:: SourceLoader

    An abstract base class for implementing source (and optionally bytecode)
    file loading. The class inherits from both :class:`ResourceLoader` and
    :class:`ExecutionLoader`, requiring the implementation of:

    * :meth:`ResourceLoader.get_data`
    * :meth:`ExecutionLoader.get_filename`
          Should only return the path to the source file; sourceless
          loading is not supported.

    The abstract methods defined by this class are to add optional bytecode
    file support. Not implementing these optional methods (or causing them to
    raise :exc:`NotImplementedError`) causes the loader to
    only work with source code. Implementing the methods allows the loader to
    work with source *and* bytecode files; it does not allow for *sourceless*
    loading where only bytecode is provided.  Bytecode files are an
    optimization to speed up loading by removing the parsing step of Python's
    compiler, and so no bytecode-specific API is exposed.

    .. method:: path_stats(path)

        Optional abstract method which returns a :class:`dict` containing
        metadata about the specifed path.  Supported dictionary keys are:

        - ``'mtime'`` (mandatory): an integer or floating-point number
          representing the modification time of the source code;
        - ``'size'`` (optional): the size in bytes of the source code.

        Any other keys in the dictionary are ignored, to allow for future
        extensions. If the path cannot be handled, :exc:`OSError` is raised.

        .. versionadded:: 3.3

        .. versionchanged:: 3.4
           Raise :exc:`OSError` instead of :exc:`NotImplementedError`.

    .. method:: path_mtime(path)

        Optional abstract method which returns the modification time for the
        specified path.

        .. deprecated:: 3.3
           This method is deprecated in favour of :meth:`path_stats`.  You don't
           have to implement it, but it is still available for compatibility
           purposes. Raise :exc:`OSError` if the path cannot be handled.

        .. versionchanged:: 3.4
           Raise :exc:`OSError` instead of :exc:`NotImplementedError`.

    .. method:: set_data(path, data)

        Optional abstract method which writes the specified bytes to a file
        path. Any intermediate directories which do not exist are to be created
        automatically.

        When writing to the path fails because the path is read-only
        (:attr:`errno.EACCES`/:exc:`PermissionError`), do not propagate the
        exception.

        .. versionchanged:: 3.4
           No longer raises :exc:`NotImplementedError` when called.

    .. method:: get_code(fullname)

        Concrete implementation of :meth:`InspectLoader.get_code`.

    .. method:: exec_module(module)

       Concrete implementation of :meth:`Loader.exec_module`.

      .. versionadded:: 3.4

    .. method:: load_module(fullname)

       Concrete implementation of :meth:`Loader.load_module`.

       .. deprecated:: 3.4
          Use :meth:`exec_module` instead.

    .. method:: get_source(fullname)

        Concrete implementation of :meth:`InspectLoader.get_source`.

    .. method:: is_package(fullname)

        Concrete implementation of :meth:`InspectLoader.is_package`. A module
        is determined to be a package if its file path (as provided by
        :meth:`ExecutionLoader.get_filename`) is a file named
        ``__init__`` when the file extension is removed **and** the module name
        itself does not end in ``__init__``.


:mod:`importlib.machinery` -- Importers and path hooks
------------------------------------------------------

.. module:: importlib.machinery
    :synopsis: Importers and path hooks

This module contains the various objects that help :keyword:`import`
find and load modules.

.. attribute:: SOURCE_SUFFIXES

   A list of strings representing the recognized file suffixes for source
   modules.

   .. versionadded:: 3.3

.. attribute:: DEBUG_BYTECODE_SUFFIXES

   A list of strings representing the file suffixes for non-optimized bytecode
   modules.

   .. versionadded:: 3.3

.. attribute:: OPTIMIZED_BYTECODE_SUFFIXES

   A list of strings representing the file suffixes for optimized bytecode
   modules.

   .. versionadded:: 3.3

.. attribute:: BYTECODE_SUFFIXES

   A list of strings representing the recognized file suffixes for bytecode
   modules. Set to either :attr:`DEBUG_BYTECODE_SUFFIXES` or
   :attr:`OPTIMIZED_BYTECODE_SUFFIXES` based on whether ``__debug__`` is true.

   .. versionadded:: 3.3

.. attribute:: EXTENSION_SUFFIXES

   A list of strings representing the recognized file suffixes for
   extension modules.

   .. versionadded:: 3.3

.. function:: all_suffixes()

   Returns a combined list of strings representing all file suffixes for
   modules recognized by the standard import machinery. This is a
   helper for code which simply needs to know if a filesystem path
   potentially refers to a module without needing any details on the kind
   of module (for example, :func:`inspect.getmodulename`)

   .. versionadded:: 3.3


.. class:: BuiltinImporter

    An :term:`importer` for built-in modules. All known built-in modules are
    listed in :data:`sys.builtin_module_names`. This class implements the
    :class:`importlib.abc.MetaPathFinder` and
    :class:`importlib.abc.InspectLoader` ABCs.

    Only class methods are defined by this class to alleviate the need for
    instantiation.

    .. note::
       Due to limitations in the extension module C-API, for now
       BuiltinImporter does not implement :meth:`Loader.exec_module`.


.. class:: FrozenImporter

    An :term:`importer` for frozen modules. This class implements the
    :class:`importlib.abc.MetaPathFinder` and
    :class:`importlib.abc.InspectLoader` ABCs.

    Only class methods are defined by this class to alleviate the need for
    instantiation.


.. class:: WindowsRegistryFinder

   :term:`Finder` for modules declared in the Windows registry.  This class
   implements the :class:`importlib.abc.Finder` ABC.

   Only class methods are defined by this class to alleviate the need for
   instantiation.

   .. versionadded:: 3.3


.. class:: PathFinder

   A :term:`Finder` for :data:`sys.path` and package ``__path__`` attributes.
   This class implements the :class:`importlib.abc.MetaPathFinder` ABC.

   Only class methods are defined by this class to alleviate the need for
   instantiation.

   .. classmethod:: find_spec(fullname, path=None, target=None)

      Class method that attempts to find a :term:`spec <module spec>`
      for the module specified by *fullname* on :data:`sys.path` or, if
      defined, on *path*. For each path entry that is searched,
      :data:`sys.path_importer_cache` is checked. If a non-false object
      is found then it is used as the :term:`path entry finder` to look
      for the module being searched for. If no entry is found in
      :data:`sys.path_importer_cache`, then :data:`sys.path_hooks` is
      searched for a finder for the path entry and, if found, is stored
      in :data:`sys.path_importer_cache` along with being queried about
      the module. If no finder is ever found then ``None`` is both
      stored in the cache and returned.

      .. versionadded:: 3.4

   .. classmethod:: find_module(fullname, path=None)

      A legacy wrapper around :meth:`find_spec`.

      .. deprecated:: 3.4
         Use :meth:`find_spec` instead.

   .. classmethod:: invalidate_caches()

      Calls :meth:`importlib.abc.PathEntryFinder.invalidate_caches` on all
      finders stored in :attr:`sys.path_importer_cache`.

   .. versionchanged:: 3.4
      Calls objects in :data:`sys.path_hooks` with the current working
      directory for ``''`` (i.e. the empty string).


.. class:: FileFinder(path, \*loader_details)

   A concrete implementation of :class:`importlib.abc.PathEntryFinder` which
   caches results from the file system.

   The *path* argument is the directory for which the finder is in charge of
   searching.

   The *loader_details* argument is a variable number of 2-item tuples each
   containing a loader and a sequence of file suffixes the loader recognizes.
   The loaders are expected to be callables which accept two arguments of
   the module's name and the path to the file found.

   The finder will cache the directory contents as necessary, making stat calls
   for each module search to verify the cache is not outdated. Because cache
   staleness relies upon the granularity of the operating system's state
   information of the file system, there is a potential race condition of
   searching for a module, creating a new file, and then searching for the
   module the new file represents. If the operations happen fast enough to fit
   within the granularity of stat calls, then the module search will fail. To
   prevent this from happening, when you create a module dynamically, make sure
   to call :func:`importlib.invalidate_caches`.

   .. versionadded:: 3.3

   .. attribute:: path

      The path the finder will search in.

   .. method:: find_spec(fullname, target=None)

      Attempt to find the spec to handle *fullname* within :attr:`path`.

      .. versionadded:: 3.4

   .. method:: find_loader(fullname)

      Attempt to find the loader to handle *fullname* within :attr:`path`.

   .. method:: invalidate_caches()

      Clear out the internal cache.

   .. classmethod:: path_hook(\*loader_details)

      A class method which returns a closure for use on :attr:`sys.path_hooks`.
      An instance of :class:`FileFinder` is returned by the closure using the
      path argument given to the closure directly and *loader_details*
      indirectly.

      If the argument to the closure is not an existing directory,
      :exc:`ImportError` is raised.


.. class:: SourceFileLoader(fullname, path)

   A concrete implementation of :class:`importlib.abc.SourceLoader` by
   subclassing :class:`importlib.abc.FileLoader` and providing some concrete
   implementations of other methods.

   .. versionadded:: 3.3

   .. attribute:: name

      The name of the module that this loader will handle.

   .. attribute:: path

      The path to the source file.

   .. method:: is_package(fullname)

      Return true if :attr:`path` appears to be for a package.

   .. method:: path_stats(path)

      Concrete implementation of :meth:`importlib.abc.SourceLoader.path_stats`.

   .. method:: set_data(path, data)

      Concrete implementation of :meth:`importlib.abc.SourceLoader.set_data`.

   .. method:: load_module(name=None)

      Concrete implementation of :meth:`importlib.abc.Loader.load_module` where
      specifying the name of the module to load is optional.


.. class:: SourcelessFileLoader(fullname, path)

   A concrete implementation of :class:`importlib.abc.FileLoader` which can
   import bytecode files (i.e. no source code files exist).

   Please note that direct use of bytecode files (and thus not source code
   files) inhibits your modules from being usable by all Python
   implementations or new versions of Python which change the bytecode
   format.

   .. versionadded:: 3.3

   .. attribute:: name

      The name of the module the loader will handle.

   .. attribute:: path

      The path to the bytecode file.

   .. method:: is_package(fullname)

      Determines if the module is a package based on :attr:`path`.

   .. method:: get_code(fullname)

      Returns the code object for :attr:`name` created from :attr:`path`.

   .. method:: get_source(fullname)

      Returns ``None`` as bytecode files have no source when this loader is
      used.

   .. method:: load_module(name=None)

   Concrete implementation of :meth:`importlib.abc.Loader.load_module` where
   specifying the name of the module to load is optional.


.. class:: ExtensionFileLoader(fullname, path)

   A concrete implementation of :class:`importlib.abc.ExecutionLoader` for
   extension modules.

   The *fullname* argument specifies the name of the module the loader is to
   support. The *path* argument is the path to the extension module's file.

   .. versionadded:: 3.3

   .. attribute:: name

      Name of the module the loader supports.

   .. attribute:: path

      Path to the extension module.

   .. method:: load_module(name=None)

      Loads the extension module if and only if *fullname* is the same as
      :attr:`name` or is ``None``.

      .. note::
         Due to limitations in the extension module C-API, for now
         ExtensionFileLoader does not implement :meth:`Loader.exec_module`.

   .. method:: is_package(fullname)

      Returns ``True`` if the file path points to a package's ``__init__``
      module based on :attr:`EXTENSION_SUFFIXES`.

   .. method:: get_code(fullname)

      Returns ``None`` as extension modules lack a code object.

   .. method:: get_source(fullname)

      Returns ``None`` as extension modules do not have source code.

   .. method:: get_filename(fullname)

      Returns :attr:`path`.

      .. versionadded:: 3.4


.. class:: ModuleSpec(name, loader, *, origin=None, loader_state=None, is_package=None)

   A specification for a module's import-system-related state.

   .. versionadded:: 3.4

   .. attribute:: name

   (``__name__``)

   A string for the fully-qualified name of the module.

   .. attribute:: loader

   (``__loader__``)

   The loader to use for loading.  For namespace packages this should be
   set to None.

   .. attribute:: origin

   (``__file__``)

   Name of the place from which the module is loaded, e.g. "builtin" for
   built-in modules and the filename for modules loaded from source.
   Normally "origin" should be set, but it may be None (the default)
   which indicates it is unspecified.

   .. attribute:: submodule_search_locations

   (``__path__``)

   List of strings for where to find submodules, if a package (None
   otherwise).

   .. attribute:: loader_state

   Container of extra module-specific data for use during loading (or
   None).

   .. attribute:: cached

   (``__cached__``)

   String for where the compiled module should be stored (or None).

   .. attribute:: parent

   (``__package__``)

   (Read-only) Fully-qualified name of the package to which the module
   belongs as a submodule (or None).

   .. attribute:: has_location

   Boolean indicating whether or not the module's "origin"
   attribute refers to a loadable location.

:mod:`importlib.util` -- Utility code for importers
---------------------------------------------------

.. module:: importlib.util
    :synopsis: Utility code for importers

This module contains the various objects that help in the construction of
an :term:`importer`.

.. attribute:: MAGIC_NUMBER

   The bytes which represent the bytecode version number. If you need help with
   loading/writing bytecode then consider :class:`importlib.abc.SourceLoader`.

   .. versionadded:: 3.4

.. function:: cache_from_source(path, debug_override=None)

   Return the :pep:`3147` path to the byte-compiled file associated with the
   source *path*.  For example, if *path* is ``/foo/bar/baz.py`` the return
   value would be ``/foo/bar/__pycache__/baz.cpython-32.pyc`` for Python 3.2.
   The ``cpython-32`` string comes from the current magic tag (see
   :func:`get_tag`; if :attr:`sys.implementation.cache_tag` is not defined then
   :exc:`NotImplementedError` will be raised).  The returned path will end in
   ``.pyc`` when ``__debug__`` is ``True`` or ``.pyo`` for an optimized Python
   (i.e. ``__debug__`` is ``False``).  By passing in ``True`` or ``False`` for
   *debug_override* you can override the system's value for ``__debug__`` for
   extension selection.

   *path* need not exist.

   .. versionadded:: 3.4


.. function:: source_from_cache(path)

   Given the *path* to a :pep:`3147` file name, return the associated source code
   file path.  For example, if *path* is
   ``/foo/bar/__pycache__/baz.cpython-32.pyc`` the returned path would be
   ``/foo/bar/baz.py``.  *path* need not exist, however if it does not conform
   to :pep:`3147` format, a ``ValueError`` is raised. If
   :attr:`sys.implementation.cache_tag` is not defined,
   :exc:`NotImplementedError` is raised.

   .. versionadded:: 3.4

.. function:: decode_source(source_bytes)

   Decode the given bytes representing source code and return it as a string
   with universal newlines (as required by
   :meth:`importlib.abc.InspectLoader.get_source`).

   .. versionadded:: 3.4

.. function:: resolve_name(name, package)

   Resolve a relative module name to an absolute one.

   If  **name** has no leading dots, then **name** is simply returned. This
   allows for usage such as
   ``importlib.util.resolve_name('sys', __package__)`` without doing a
   check to see if the **package** argument is needed.

   :exc:`ValueError` is raised if **name** is a relative module name but
   package is a false value (e.g. ``None`` or the empty string).
   :exc:`ValueError` is also raised a relative name would escape its containing
   package (e.g. requesting ``..bacon`` from within the ``spam`` package).

   .. versionadded:: 3.3

.. function:: find_spec(name, package=None)

   Find the :term:`spec <module spec>` for a module, optionally relative to
   the specified **package** name. If the module is in :attr:`sys.modules`,
   then ``sys.modules[name].__spec__`` is returned (unless the spec would be
   ``None`` or is not set, in which case :exc:`ValueError` is raised).
   Otherwise a search using :attr:`sys.meta_path` is done. ``None`` is
   returned if no spec is found.

   If **name** is for a submodule (contains a dot), the parent module is
   automatically imported.

   **name** and **package** work the same as for :func:`import_module`.

   .. versionadded:: 3.4

.. decorator:: module_for_loader

    A :term:`decorator` for :meth:`importlib.abc.Loader.load_module`
    to handle selecting the proper
    module object to load with. The decorated method is expected to have a call
    signature taking two positional arguments
    (e.g. ``load_module(self, module)``) for which the second argument
    will be the module **object** to be used by the loader.
    Note that the decorator will not work on static methods because of the
    assumption of two arguments.

    The decorated method will take in the **name** of the module to be loaded
    as expected for a :term:`loader`. If the module is not found in
    :data:`sys.modules` then a new one is constructed. Regardless of where the
    module came from, :attr:`__loader__` set to **self** and :attr:`__package__`
    is set based on what :meth:`importlib.abc.InspectLoader.is_package` returns
    (if available). These attributes are set unconditionally to support
    reloading.

    If an exception is raised by the decorated method and a module was added to
    :data:`sys.modules`, then the module will be removed to prevent a partially
    initialized module from being in left in :data:`sys.modules`. If the module
    was already in :data:`sys.modules` then it is left alone.

    .. versionchanged:: 3.3
       :attr:`__loader__` and :attr:`__package__` are automatically set
       (when possible).

    .. versionchanged:: 3.4
       Set :attr:`__name__`, :attr:`__loader__` :attr:`__package__`
       unconditionally to support reloading.

    .. deprecated:: 3.4
       The import machinery now directly performs all the functionality
       provided by this function.

.. decorator:: set_loader

   A :term:`decorator` for :meth:`importlib.abc.Loader.load_module`
   to set the :attr:`__loader__`
   attribute on the returned module. If the attribute is already set the
   decorator does nothing. It is assumed that the first positional argument to
   the wrapped method (i.e. ``self``) is what :attr:`__loader__` should be set
   to.

   .. versionchanged:: 3.4
      Set ``__loader__`` if set to ``None``, as if the attribute does not
      exist.

   .. deprecated:: 3.4
      The import machinery takes care of this automatically.

.. decorator:: set_package

   A :term:`decorator` for :meth:`importlib.abc.Loader.load_module` to set the :attr:`__package__` attribute on the returned module. If :attr:`__package__`
   is set and has a value other than ``None`` it will not be changed.

   .. deprecated:: 3.4
      The import machinery takes care of this automatically.

.. function:: spec_from_loader(name, loader, *, origin=None, is_package=None)

   A factory function for creating a :class:`ModuleSpec` instance based
   on a loader.  The parameters have the same meaning as they do for
   ModuleSpec.  The function uses available :term:`loader` APIs, such as
   :meth:`InspectLoader.is_package`, to fill in any missing
   information on the spec.

   .. versionadded:: 3.4

.. function:: spec_from_file_location(name, location, *, loader=None, submodule_search_locations=None)

   A factory function for creating a :class:`ModuleSpec` instance based
   on the path to a file.  Missing information will be filled in on the
   spec by making use of loader APIs and by the implication that the
   module will be file-based.

   .. versionadded:: 3.4
