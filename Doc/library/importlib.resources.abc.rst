:mod:`!importlib.resources.abc` -- Abstract base classes for resources
----------------------------------------------------------------------

.. module:: importlib.resources.abc
    :synopsis: Abstract base classes for resources

**Source code:** :source:`Lib/importlib/resources/abc.py`

--------------

.. versionadded:: 3.11

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

    .. deprecated-removed:: 3.12 3.14
       Use :class:`importlib.resources.abc.TraversableResources` instead.

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


.. class:: Traversable

    An object with a subset of :class:`pathlib.Path` methods suitable for
    traversing directories and opening files.

    For a representation of the object on the file-system, use
    :meth:`importlib.resources.as_file`.

    .. attribute:: name

       Abstract. The base name of this object without any parent references.

    .. abstractmethod:: iterdir()

       Yield Traversable objects in self.

    .. abstractmethod:: is_dir()

       Return ``True`` if self is a directory.

    .. abstractmethod:: is_file()

       Return ``True`` if self is a file.

    .. abstractmethod:: joinpath(*pathsegments)

       Traverse directories according to *pathsegments* and return
       the result as :class:`!Traversable`.

       Each *pathsegments* argument may contain multiple names separated by
       forward slashes (``/``, ``posixpath.sep`` ).
       For example, the following are equivalent::

           files.joinpath('subdir', 'subsuddir', 'file.txt')
           files.joinpath('subdir/subsuddir/file.txt')

       Note that some :class:`!Traversable` implementations
       might not be updated to the latest version of the protocol.
       For compatibility with such implementations, provide a single argument
       without path separators to each call to ``joinpath``. For example::

           files.joinpath('subdir').joinpath('subsubdir').joinpath('file.txt')

       .. versionchanged:: 3.11

          ``joinpath`` accepts multiple *pathsegments*, and these segments
          may contain forward slashes as path separators.
          Previously, only a single *child* argument was accepted.

    .. abstractmethod:: __truediv__(child)

       Return Traversable child in self.
       Equivalent to ``joinpath(child)``.

    .. abstractmethod:: open(mode='r', *args, **kwargs)

       *mode* may be 'r' or 'rb' to open as text or binary. Return a handle
       suitable for reading (same as :attr:`pathlib.Path.open`).

       When opening as text, accepts encoding parameters such as those
       accepted by :class:`io.TextIOWrapper`.

    .. method:: read_bytes()

       Read contents of self as bytes.

    .. method:: read_text(encoding=None)

       Read contents of self as text.


.. class:: TraversableResources

    An abstract base class for resource readers capable of serving
    the :meth:`importlib.resources.files` interface. Subclasses
    :class:`ResourceReader` and provides
    concrete implementations of the :class:`!ResourceReader`'s
    abstract methods. Therefore, any loader supplying
    :class:`!TraversableResources` also supplies :class:`!ResourceReader`.

    Loaders that wish to support resource reading are expected to
    implement this interface.

    .. abstractmethod:: files()

       Returns a :class:`importlib.resources.abc.Traversable` object for the loaded
       package.
