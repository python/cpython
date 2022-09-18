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
