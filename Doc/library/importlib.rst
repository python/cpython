:mod:`importlib` -- An implementation of :keyword:`import`
==========================================================

.. module:: importlib
   :synopsis: An implementation of the import machinery.

.. moduleauthor:: Brett Cannon <brett@python.org>
.. sectionauthor:: Brett Cannon <brett@python.org>

.. versionadded:: 3.1


Introduction
------------

The purpose of the :mod:`importlib` package is two-fold. One is to provide an
implementation of the :keyword:`import` statement (and thus, by extension, the
:func:`__import__` function) in Python source code. This provides an
implementation of :keyword:`import` which is portable to any Python
interpreter. This also provides a reference implementation which is easier to
comprehend than one implemented in a programming language other than Python.

Two, the components to implement :keyword:`import` are exposed in this
package, making it easier for users to create their own custom objects (known
generically as an :term:`importer`) to participate in the import process.
Details on custom importers can be found in :pep:`302`.

.. seealso::

    :ref:`import`
        The language reference for the :keyword:`import` statement.

    `Packages specification <http://www.python.org/doc/essays/packages.html>`__
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

    :pep:`3120`
        Using UTF-8 as the Default Source Encoding

    :pep:`3147`
        PYC Repository Directories


Functions
---------

.. function:: __import__(name, globals={}, locals={}, fromlist=list(), level=0)

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
    is that :func:`import_module` returns the most nested package or module
    that was imported (e.g. ``pkg.mod``), while :func:`__import__` returns the
    top-level package or module (e.g. ``pkg``).

.. function:: find_loader(name, path=None)

   Find the loader for a module, optionally within the specified *path*. If the
   module is in :attr:`sys.modules`, then ``sys.modules[name].__loader__`` is
   returned (unless the loader would be ``None``, in which case
   :exc:`ValueError` is raised). Otherwise a search using :attr:`sys.meta_path`
   is done. ``None`` is returned if no loader is found.

   A dotted name does not have its parent's implicitly imported. If that is
   desired (although not nessarily required to find the loader, it will most
   likely be needed if the loader actually is used to load the module), then
   you will have to import the packages containing the module prior to calling
   this function.

.. function:: invalidate_caches()

   Invalidate the internal caches of the finders stored at
   :data:`sys.path_importer_cache`. If a finder implements
   :meth:`abc.Finder.invalidate_caches()` then it will be called to perform the
   invalidation.  This function may be needed if some modules are installed
   while your program is running and you expect the program to notice the
   changes.

   .. versionadded:: 3.3


:mod:`importlib.abc` -- Abstract base classes related to import
---------------------------------------------------------------

.. module:: importlib.abc
    :synopsis: Abstract base classes related to import

The :mod:`importlib.abc` module contains all of the core abstract base classes
used by :keyword:`import`. Some subclasses of the core abstract base classes
are also provided to help in implementing the core ABCs.


.. class:: Finder

    An abstract base class representing a :term:`finder`.
    See :pep:`302` for the exact definition for a finder.

    .. method:: find_module(fullname, path=None)

        An abstract method for finding a :term:`loader` for the specified
        module. If the :term:`finder` is found on :data:`sys.meta_path` and the
        module to be searched for is a subpackage or module then *path* will
        be the value of :attr:`__path__` from the parent package. If a loader
        cannot be found, ``None`` is returned.

   .. method:: invalidate_caches()

        An optional method which, when called, should invalidate any internal
        cache used by the finder. Used by :func:`invalidate_caches()` when
        invalidating the caches of all cached finders.


.. class:: Loader

    An abstract base class for a :term:`loader`.
    See :pep:`302` for the exact definition for a loader.

    .. method:: load_module(fullname)

        An abstract method for loading a module. If the module cannot be
        loaded, :exc:`ImportError` is raised, otherwise the loaded module is
        returned.

        If the requested module already exists in :data:`sys.modules`, that
        module should be used and reloaded.
        Otherwise the loader should create a new module and insert it into
        :data:`sys.modules` before any loading begins, to prevent recursion
        from the import. If the loader inserted a module and the load fails, it
        must be removed by the loader from :data:`sys.modules`; modules already
        in :data:`sys.modules` before the loader began execution should be left
        alone. The :func:`importlib.util.module_for_loader` decorator handles
        all of these details.

        The loader should set several attributes on the module.
        (Note that some of these attributes can change when a module is
        reloaded.)

        - :attr:`__name__`
            The name of the module.

        - :attr:`__file__`
            The path to where the module data is stored (not set for built-in
            modules).

        - :attr:`__path__`
            A list of strings specifying the search path within a
            package. This attribute is not set on modules.

        - :attr:`__package__`
            The parent package for the module/package. If the module is
            top-level then it has a value of the empty string. The
            :func:`importlib.util.set_package` decorator can handle the details
            for :attr:`__package__`.

        - :attr:`__loader__`
            The loader used to load the module.
            (This is not set by the built-in import machinery,
            but it should be set whenever a :term:`loader` is used.)


.. class:: ResourceLoader

    An abstract base class for a :term:`loader` which implements the optional
    :pep:`302` protocol for loading arbitrary resources from the storage
    back-end.

    .. method:: get_data(path)

        An abstract method to return the bytes for the data located at *path*.
        Loaders that have a file-like storage back-end
        that allows storing arbitrary data
        can implement this abstract method to give direct access
        to the data stored. :exc:`IOError` is to be raised if the *path* cannot
        be found. The *path* is expected to be constructed using a module's
        :attr:`__file__` attribute or an item from a package's :attr:`__path__`.


.. class:: InspectLoader

    An abstract base class for a :term:`loader` which implements the optional
    :pep:`302` protocol for loaders that inspect modules.

    .. method:: get_code(fullname)

        An abstract method to return the :class:`code` object for a module.
        ``None`` is returned if the module does not have a code object
        (e.g. built-in module).  :exc:`ImportError` is raised if loader cannot
        find the requested module.

    .. method:: get_source(fullname)

        An abstract method to return the source of a module. It is returned as
        a text string with universal newlines. Returns ``None`` if no
        source is available (e.g. a built-in module). Raises :exc:`ImportError`
        if the loader cannot find the module specified.

    .. method:: is_package(fullname)

        An abstract method to return a true value if the module is a package, a
        false value otherwise. :exc:`ImportError` is raised if the
        :term:`loader` cannot find the module.


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


.. class:: FileLoader(fullname, path)

   An abstract base class which inherits from :class:`ResourceLoader` and
   :class:`ExecutionLoader`, providing concreate implementations of
   :meth:`ResourceLoader.get_data` and :meth:`ExecutionLoader.get_filename`.

   The *fullname* argument is a fully resolved name of the module the loader is
   to handle. The *path* argument is the path to the file for the module.

   .. versionadded:: 3.3

   .. attribute:: name

      The name of the module the loader can handle.

   .. attribute:: path

      Path to the file of the module.

   .. method:: load_module(fullname=None)

      Calls
      ``super().load_module(fullname if fullname is not None else self.name)``.

   .. method:: get_filename(fullname)

      Returns :attr:`path` when ``fullname`` equals :attr:`name` or ``None``.

   .. method:: get_data(path)

      Returns the open, binary file for *path*.


.. class:: SourceLoader

    An abstract base class for implementing source (and optionally bytecode)
    file loading. The class inherits from both :class:`ResourceLoader` and
    :class:`ExecutionLoader`, requiring the implementation of:

    * :meth:`ResourceLoader.get_data`
    * :meth:`ExecutionLoader.get_filename`
          Should only return the path to the source file; sourceless
          loading is not supported.

    The abstract methods defined by this class are to add optional bytecode
    file support. Not implementing these optional methods causes the loader to
    only work with source code. Implementing the methods allows the loader to
    work with source *and* bytecode files; it does not allow for *sourceless*
    loading where only bytecode is provided.  Bytecode files are an
    optimization to speed up loading by removing the parsing step of Python's
    compiler, and so no bytecode-specific API is exposed.

    .. method:: path_stats(self, path)

        Optional abstract method which returns a :class:`dict` containing
        metadata about the specifed path.  Supported dictionary keys are:

        - ``'mtime'`` (mandatory): an integer or floating-point number
          representing the modification time of the source code;
        - ``'size'`` (optional): the size in bytes of the source code.

        Any other keys in the dictionary are ignored, to allow for future
        extensions.

        .. versionadded:: 3.3

    .. method:: path_mtime(self, path)

        Optional abstract method which returns the modification time for the
        specified path.

        .. deprecated:: 3.3
           This method is deprecated in favour of :meth:`path_stats`.  You don't
           have to implement it, but it is still available for compatibility
           purposes.

    .. method:: set_data(self, path, data)

        Optional abstract method which writes the specified bytes to a file
        path. Any intermediate directories which do not exist are to be created
        automatically.

        When writing to the path fails because the path is read-only
        (:attr:`errno.EACCES`), do not propagate the exception.

    .. method:: get_code(self, fullname)

        Concrete implementation of :meth:`InspectLoader.get_code`.

    .. method:: load_module(self, fullname)

        Concrete implementation of :meth:`Loader.load_module`.

    .. method:: get_source(self, fullname)

        Concrete implementation of :meth:`InspectLoader.get_source`.

    .. method:: is_package(self, fullname)

        Concrete implementation of :meth:`InspectLoader.is_package`. A module
        is determined to be a package if its file path is a file named
        ``__init__`` when the file extension is removed.


.. class:: PyLoader

    An abstract base class inheriting from
    :class:`ExecutionLoader` and
    :class:`ResourceLoader` designed to ease the loading of
    Python source modules (bytecode is not handled; see
    :class:`SourceLoader` for a source/bytecode ABC). A subclass
    implementing this ABC will only need to worry about exposing how the source
    code is stored; all other details for loading Python source code will be
    handled by the concrete implementations of key methods.

    .. deprecated:: 3.2
        This class has been deprecated in favor of :class:`SourceLoader` and is
        slated for removal in Python 3.4. See below for how to create a
        subclass that is compatible with Python 3.1 onwards.

    If compatibility with Python 3.1 is required, then use the following idiom
    to implement a subclass that will work with Python 3.1 onwards (make sure
    to implement :meth:`ExecutionLoader.get_filename`)::

        try:
            from importlib.abc import SourceLoader
        except ImportError:
            from importlib.abc import PyLoader as SourceLoader


        class CustomLoader(SourceLoader):
            def get_filename(self, fullname):
                """Return the path to the source file."""
                # Implement ...

            def source_path(self, fullname):
                """Implement source_path in terms of get_filename."""
                try:
                    return self.get_filename(fullname)
                except ImportError:
                    return None

            def is_package(self, fullname):
                """Implement is_package by looking for an __init__ file
                name as returned by get_filename."""
                filename = os.path.basename(self.get_filename(fullname))
                return os.path.splitext(filename)[0] == '__init__'


    .. method:: source_path(fullname)

        An abstract method that returns the path to the source code for a
        module. Should return ``None`` if there is no source code.
        Raises :exc:`ImportError` if the loader knows it cannot handle the
        module.

    .. method:: get_filename(fullname)

        A concrete implementation of
        :meth:`importlib.abc.ExecutionLoader.get_filename` that
        relies on :meth:`source_path`. If :meth:`source_path` returns
        ``None``, then :exc:`ImportError` is raised.

    .. method:: load_module(fullname)

        A concrete implementation of :meth:`importlib.abc.Loader.load_module`
        that loads Python source code. All needed information comes from the
        abstract methods required by this ABC. The only pertinent assumption
        made by this method is that when loading a package
        :attr:`__path__` is set to ``[os.path.dirname(__file__)]``.

    .. method:: get_code(fullname)

        A concrete implementation of
        :meth:`importlib.abc.InspectLoader.get_code` that creates code objects
        from Python source code, by requesting the source code (using
        :meth:`source_path` and :meth:`get_data`) and compiling it with the
        built-in :func:`compile` function.

    .. method:: get_source(fullname)

        A concrete implementation of
        :meth:`importlib.abc.InspectLoader.get_source`. Uses
        :meth:`importlib.abc.ResourceLoader.get_data` and :meth:`source_path`
        to get the source code.  It tries to guess the source encoding using
        :func:`tokenize.detect_encoding`.


.. class:: PyPycLoader

    An abstract base class inheriting from :class:`PyLoader`.
    This ABC is meant to help in creating loaders that support both Python
    source and bytecode.

    .. deprecated:: 3.2
        This class has been deprecated in favor of :class:`SourceLoader` and to
        properly support :pep:`3147`. If compatibility is required with
        Python 3.1, implement both :class:`SourceLoader` and :class:`PyLoader`;
        instructions on how to do so are included in the documentation for
        :class:`PyLoader`. Do note that this solution will not support
        sourceless/bytecode-only loading; only source *and* bytecode loading.

    .. method:: source_mtime(fullname)

        An abstract method which returns the modification time for the source
        code of the specified module. The modification time should be an
        integer. If there is no source code, return ``None``. If the
        module cannot be found then :exc:`ImportError` is raised.

    .. method:: bytecode_path(fullname)

        An abstract method which returns the path to the bytecode for the
        specified module, if it exists. It returns ``None``
        if no bytecode exists (yet).
        Raises :exc:`ImportError` if the loader knows it cannot handle the
        module.

    .. method:: get_filename(fullname)

        A concrete implementation of
        :meth:`ExecutionLoader.get_filename` that relies on
        :meth:`PyLoader.source_path` and :meth:`bytecode_path`.
        If :meth:`source_path` returns a path, then that value is returned.
        Else if :meth:`bytecode_path` returns a path, that path will be
        returned. If a path is not available from both methods,
        :exc:`ImportError` is raised.

    .. method:: write_bytecode(fullname, bytecode)

        An abstract method which has the loader write *bytecode* for future
        use. If the bytecode is written, return ``True``. Return
        ``False`` if the bytecode could not be written. This method
        should not be called if :data:`sys.dont_write_bytecode` is true.
        The *bytecode* argument should be a bytes string or bytes array.


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

   A list of strings representing the the recognized file suffixes for
   extension modules.

   .. versionadded:: 3.3


.. class:: BuiltinImporter

    An :term:`importer` for built-in modules. All known built-in modules are
    listed in :data:`sys.builtin_module_names`. This class implements the
    :class:`importlib.abc.Finder` and :class:`importlib.abc.InspectLoader`
    ABCs.

    Only class methods are defined by this class to alleviate the need for
    instantiation.


.. class:: FrozenImporter

    An :term:`importer` for frozen modules. This class implements the
    :class:`importlib.abc.Finder` and :class:`importlib.abc.InspectLoader`
    ABCs.

    Only class methods are defined by this class to alleviate the need for
    instantiation.


.. class:: PathFinder

    :term:`Finder` for :data:`sys.path`. This class implements the
    :class:`importlib.abc.Finder` ABC.

    This class does not perfectly mirror the semantics of :keyword:`import` in
    terms of :data:`sys.path`. No implicit path hooks are assumed for
    simplification of the class and its semantics. This implies that when
    ``None`` is found in :data:`sys.path_importer_cache` that it is simply
    ignored instead of implying a default finder.

    Only class methods are defined by this class to alleviate the need for
    instantiation.

    .. classmethod:: find_module(fullname, path=None)

        Class method that attempts to find a :term:`loader` for the module
        specified by *fullname* on :data:`sys.path` or, if defined, on
        *path*. For each path entry that is searched,
        :data:`sys.path_importer_cache` is checked. If a non-false object is
        found then it is used as the :term:`finder` to look for the module
        being searched for. If no entry is found in
        :data:`sys.path_importer_cache`, then :data:`sys.path_hooks` is
        searched for a finder for the path entry and, if found, is stored in
        :data:`sys.path_importer_cache` along with being queried about the
        module. If no finder is ever found then ``None`` is returned.


.. class:: FileFinder(path, \*loader_details)

   A concrete implementation of :class:`importlib.abc.Finder` which caches
   results from the file system.

   The *path* argument is the directory for which the finder is in charge of
   searching.

   The *loader_details* argument is a variable number of 3-item tuples each
   containing a loader, file suffixes the loader recognizes, and a boolean
   representing whether the loader handles packages.

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

   .. method:: find_module(fullname)

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


.. class:: ExtensionFileLoader(fullname, path)

   A concrete implementation of :class:`importlib.abc.InspectLoader` for
   extension modules.

   The *fullname* argument specifies the name of the module the loader is to
   support. The *path* argument is the path to the extension module's file.

   .. versionadded:: 3.3

   .. attribute:: name

      Name of the module the loader supports.

   .. attribute:: path

      Path to the extension module.

   .. method:: load_module(fullname=None)

      Loads the extension module if and only if *fullname* is the same as
      :attr:`name` or is ``None``.

   .. method:: is_package(fullname)

      Returns ``False`` as extension modules can never be packages.

   .. method:: get_code(fullname)

      Returns ``None`` as extension modules lack a code object.

   .. method:: get_source(fullname)

      Returns ``None`` as extension modules do not have source code.


:mod:`importlib.util` -- Utility code for importers
---------------------------------------------------

.. module:: importlib.util
    :synopsis: Utility code for importers

This module contains the various objects that help in the construction of
an :term:`importer`.

.. decorator:: module_for_loader

    A :term:`decorator` for a :term:`loader` method,
    to handle selecting the proper
    module object to load with. The decorated method is expected to have a call
    signature taking two positional arguments
    (e.g. ``load_module(self, module)``) for which the second argument
    will be the module **object** to be used by the loader.
    Note that the decorator will not work on static methods because of the
    assumption of two arguments.

    The decorated method will take in the **name** of the module to be loaded
    as expected for a :term:`loader`. If the module is not found in
    :data:`sys.modules` then a new one is constructed with its
    :attr:`__name__` attribute set to **name**, :attr:`__loader__` set to
    **self**, and :attr:`__package__` set if
    :meth:`importlib.abc.InspectLoader.is_package` is defined for **self** and
    does not raise :exc:`ImportError` for **name**. If a new module is not
    needed then the module found in :data:`sys.modules` will be passed into the
    method.

    If an exception is raised by the decorated method and a module was added to
    :data:`sys.modules` it will be removed to prevent a partially initialized
    module from being in left in :data:`sys.modules`. If the module was already
    in :data:`sys.modules` then it is left alone.

    Use of this decorator handles all the details of which module object a
    loader should initialize as specified by :pep:`302` as best as possible.

    .. versionchanged:: 3.3
      :attr:`__loader__` and :attr:`__package__` are automatically set
      (when possible).

.. decorator:: set_loader

    A :term:`decorator` for a :term:`loader` method,
    to set the :attr:`__loader__`
    attribute on loaded modules. If the attribute is already set the decorator
    does nothing. It is assumed that the first positional argument to the
    wrapped method (i.e. ``self``) is what :attr:`__loader__` should be set to.

   .. note::

      It is recommended that :func:`module_for_loader` be used over this
      decorator as it subsumes this functionality.


.. decorator:: set_package

    A :term:`decorator` for a :term:`loader` to set the :attr:`__package__`
    attribute on the module returned by the loader. If :attr:`__package__` is
    set and has a value other than ``None`` it will not be changed.
    Note that the module returned by the loader is what has the attribute
    set on and not the module found in :data:`sys.modules`.

    Reliance on this decorator is discouraged when it is possible to set
    :attr:`__package__` before importing. By
    setting it beforehand the code for the module is executed with the
    attribute set and thus can be used by global level code during
    initialization.

   .. note::

      It is recommended that :func:`module_for_loader` be used over this
      decorator as it subsumes this functionality.
