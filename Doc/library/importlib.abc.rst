:mod:`importlib.abc` -- Abstract base classes related to import
---------------------------------------------------------------

.. module:: importlib.abc
    :synopsis: Abstract base classes related to import

**Source code:** :source:`Lib/importlib/abc.py`

--------------


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

   .. abstractmethod:: find_module(fullname, path=None)

      An abstract method for finding a :term:`loader` for the specified
      module.  Originally specified in :pep:`302`, this method was meant
      for use in :data:`sys.meta_path` and in the path-based import subsystem.

      .. versionchanged:: 3.4
         Returns ``None`` when called instead of raising
         :exc:`NotImplementedError`.

      .. deprecated:: 3.10
         Implement :meth:`MetaPathFinder.find_spec` or
         :meth:`PathEntryFinder.find_spec` instead.


.. class:: MetaPathFinder

   An abstract base class representing a :term:`meta path finder`. For
   compatibility, this is a subclass of :class:`Finder`.

   .. versionadded:: 3.3

   .. versionchanged:: 3.10
      No longer a subclass of :class:`Finder`.

   .. method:: find_spec(fullname, path, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  If this is a top-level import, *path* will
      be ``None``.  Otherwise, this is a search for a subpackage or
      module and *path* will be the value of :attr:`__path__` from the
      parent package. If a spec cannot be found, ``None`` is returned.
      When passed in, ``target`` is a module object that the finder may
      use to make a more educated guess about what spec to return.
      :func:`importlib.util.spec_from_loader` may be useful for implementing
      concrete ``MetaPathFinders``.

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
   by :class:`importlib.machinery.PathFinder`.

   .. versionadded:: 3.3

   .. versionchanged:: 3.10
      No longer a subclass of :class:`Finder`.

   .. method:: find_spec(fullname, target=None)

      An abstract method for finding a :term:`spec <module spec>` for
      the specified module.  The finder will search for the module only
      within the :term:`path entry` to which it is assigned.  If a spec
      cannot be found, ``None`` is returned.  When passed in, ``target``
      is a module object that the finder may use to make a more educated
      guess about what spec to return. :func:`importlib.util.spec_from_loader`
      may be useful for implementing concrete ``PathEntryFinders``.

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
      cache used by the finder. Used by
      :meth:`importlib.machinery.PathFinder.invalidate_caches`
      when invalidating the caches of all cached finders.


.. class:: Loader

    An abstract base class for a :term:`loader`.
    See :pep:`302` for the exact definition for a loader.

    Loaders that wish to support resource reading should implement a
    :meth:`get_resource_reader` method as specified by
    :class:`importlib.abc.ResourceReader`.

    .. versionchanged:: 3.7
       Introduced the optional :meth:`get_resource_reader` method.

    .. method:: create_module(spec)

       A method that returns the module object to use when
       importing a module.  This method may return ``None``,
       indicating that default module creation semantics should take place.

       .. versionadded:: 3.4

       .. versionchanged:: 3.5
          Starting in Python 3.6, this method will not be optional when
          :meth:`exec_module` is defined.

    .. method:: exec_module(module)

       An abstract method that executes the module in its own namespace
       when a module is imported or reloaded.  The module should already
       be initialized when :meth:`exec_module` is called.  When this method exists,
       :meth:`create_module` must be defined.

       .. versionadded:: 3.4

       .. versionchanged:: 3.6
          :meth:`create_module` must also be defined.

    .. method:: load_module(fullname)

        A legacy method for loading a module.  If the module cannot be
        loaded, :exc:`ImportError` is raised, otherwise the loaded module is
        returned.

        If the requested module already exists in :data:`sys.modules`, that
        module should be used and reloaded.
        Otherwise the loader should create a new module and insert it into
        :data:`sys.modules` before any loading begins, to prevent recursion
        from the import.  If the loader inserted a module and the load fails, it
        must be removed by the loader from :data:`sys.modules`; modules already
        in :data:`sys.modules` before the loader began execution should be left
        alone (see :func:`importlib.util.module_for_loader`).

        The loader should set several attributes on the module
        (note that some of these attributes can change when a module is
        reloaded):

        - :attr:`__name__`
            The module's fully-qualified name.
            It is ``'__main__'`` for an executed module.

        - :attr:`__file__`
            The location the :term:`loader` used to load the module.
            For example, for modules loaded from a .py file this is the filename.
            It is not set on all modules (e.g. built-in modules).

        - :attr:`__cached__`
            The filename of a compiled version of the module's code.
            It is not set on all modules (e.g. built-in modules).

        - :attr:`__path__`
            The list of locations where the package's submodules will be found.
            Most of the time this is a single directory.
            The import system passes this attribute to ``__import__()`` and to finders
            in the same way as :attr:`sys.path` but just for the package.
            It is not set on non-package modules so it can be used
            as an indicator that the module is a package.

        - :attr:`__package__`
            The fully-qualified name of the package the module is in (or the
            empty string for a top-level module).
            If the module is a package then this is the same as :attr:`__name__`.

        - :attr:`__loader__`
            The :term:`loader` used to load the module.

        When :meth:`exec_module` is available then backwards-compatible
        functionality is provided.

        .. versionchanged:: 3.4
           Raise :exc:`ImportError` when called instead of
           :exc:`NotImplementedError`.  Functionality provided when
           :meth:`exec_module` is available.

        .. deprecated:: 3.4
           The recommended API for loading a module is :meth:`exec_module`
           (and :meth:`create_module`).  Loaders should implement it instead of
           :meth:`load_module`.  The import machinery takes care of all the
           other responsibilities of :meth:`load_module` when
           :meth:`exec_module` is implemented.

    .. method:: module_repr(module)

        A legacy method which when implemented calculates and returns the given
        module's representation, as a string.  The module type's default
        :meth:`__repr__` will use the result of this method as appropriate.

        .. versionadded:: 3.3

        .. versionchanged:: 3.4
           Made optional instead of an abstractmethod.

        .. deprecated:: 3.4
           The import machinery now takes care of this automatically.


.. class:: ResourceReader

    *Superseded by TraversableResources*

    An :term:`abstract base class` to provide the ability to read
    *resources*.

    From the perspective of this ABC, a *resource* is a binary
    artifact that is shipped within a package. Typically this is
    something like a data file that lives next to the ``__init__.py``
    file of the package. The purpose of this class is to help abstract
    out the accessing of such data files so that it does not matter if
    the package and its data file(s) are stored in a e.g. zip file
    versus on the file system.

    For any of methods of this class, a *resource* argument is
    expected to be a :term:`path-like object` which represents
    conceptually just a file name. This means that no subdirectory
    paths should be included in the *resource* argument. This is
    because the location of the package the reader is for, acts as the
    "directory". Hence the metaphor for directories and file
    names is packages and resources, respectively. This is also why
    instances of this class are expected to directly correlate to
    a specific package (instead of potentially representing multiple
    packages or a module).

    Loaders that wish to support resource reading are expected to
    provide a method called ``get_resource_reader(fullname)`` which
    returns an object implementing this ABC's interface. If the module
    specified by fullname is not a package, this method should return
    :const:`None`. An object compatible with this ABC should only be
    returned when the specified module is a package.

    .. versionadded:: 3.7

    .. abstractmethod:: open_resource(resource)

        Returns an opened, :term:`file-like object` for binary reading
        of the *resource*.

        If the resource cannot be found, :exc:`FileNotFoundError` is
        raised.

    .. abstractmethod:: resource_path(resource)

        Returns the file system path to the *resource*.

        If the resource does not concretely exist on the file system,
        raise :exc:`FileNotFoundError`.

    .. abstractmethod:: is_resource(name)

        Returns ``True`` if the named *name* is considered a resource.
        :exc:`FileNotFoundError` is raised if *name* does not exist.

    .. abstractmethod:: contents()

        Returns an :term:`iterable` of strings over the contents of
        the package. Do note that it is not required that all names
        returned by the iterator be actual resources, e.g. it is
        acceptable to return names for which :meth:`is_resource` would
        be false.

        Allowing non-resource names to be returned is to allow for
        situations where how a package and its resources are stored
        are known a priori and the non-resource names would be useful.
        For instance, returning subdirectory names is allowed so that
        when it is known that the package and resources are stored on
        the file system then those subdirectory names can be used
        directly.

        The abstract method returns an iterable of no items.


.. class:: ResourceLoader

    An abstract base class for a :term:`loader` which implements the optional
    :pep:`302` protocol for loading arbitrary resources from the storage
    back-end.

    .. deprecated:: 3.7
       This ABC is deprecated in favour of supporting resource loading
       through :class:`importlib.abc.ResourceReader`.

    .. abstractmethod:: get_data(path)

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

    .. abstractmethod:: get_source(fullname)

        An abstract method to return the source of a module. It is returned as
        a text string using :term:`universal newlines`, translating all
        recognized line separators into ``'\n'`` characters.  Returns ``None``
        if no source is available (e.g. a built-in module). Raises
        :exc:`ImportError` if the loader cannot find the module specified.

        .. versionchanged:: 3.4
           Raises :exc:`ImportError` instead of :exc:`NotImplementedError`.

    .. method:: is_package(fullname)

        An optional method to return a true value if the module is a package, a
        false value otherwise. :exc:`ImportError` is raised if the
        :term:`loader` cannot find the module.

        .. versionchanged:: 3.4
           Raises :exc:`ImportError` instead of :exc:`NotImplementedError`.

    .. staticmethod:: source_to_code(data, path='<string>')

        Create a code object from Python source.

        The *data* argument can be whatever the :func:`compile` function
        supports (i.e. string or bytes). The *path* argument should be
        the "path" to where the source code originated from, which can be an
        abstract concept (e.g. location in a zip file).

        With the subsequent code object one can execute it in a module by
        running ``exec(code, module.__dict__)``.

        .. versionadded:: 3.4

        .. versionchanged:: 3.5
           Made the method static.

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

    .. abstractmethod:: get_filename(fullname)

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

   .. abstractmethod:: get_filename(fullname)

      Returns :attr:`path`.

   .. abstractmethod:: get_data(path)

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
        metadata about the specified path.  Supported dictionary keys are:

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


.. class:: Traversable

    An object with a subset of pathlib.Path methods suitable for
    traversing directories and opening files.

    .. versionadded:: 3.9

    .. attribute:: name

       Abstract. The base name of this object without any parent references.

    .. abstractmethod:: iterdir()

       Yield Traversable objects in self.

    .. abstractmethod:: is_dir()

       Return True if self is a directory.

    .. abstractmethod:: is_file()

       Return True if self is a file.

    .. abstractmethod:: joinpath(child)

       Return Traversable child in self.

    .. abstractmethod:: __truediv__(child)

       Return Traversable child in self.

    .. abstractmethod:: open(mode='r', *args, **kwargs)

       *mode* may be 'r' or 'rb' to open as text or binary. Return a handle
       suitable for reading (same as :attr:`pathlib.Path.open`).

       When opening as text, accepts encoding parameters such as those
       accepted by :attr:`io.TextIOWrapper`.

    .. method:: read_bytes()

       Read contents of self as bytes.

    .. method:: read_text(encoding=None)

       Read contents of self as text.


.. class:: TraversableResources

    An abstract base class for resource readers capable of serving
    the :meth:`importlib.resources.files` interface. Subclasses
    :class:`importlib.abc.ResourceReader` and provides
    concrete implementations of the :class:`importlib.abc.ResourceReader`'s
    abstract methods. Therefore, any loader supplying
    :class:`importlib.abc.TraversableReader` also supplies ResourceReader.

    Loaders that wish to support resource reading are expected to
    implement this interface.

    .. versionadded:: 3.9

    .. abstractmethod:: files()

       Returns a :class:`importlib.abc.Traversable` object for the loaded
       package.
