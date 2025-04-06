:mod:`!importlib.resources` -- Package resource reading, opening and access
---------------------------------------------------------------------------

.. module:: importlib.resources
    :synopsis: Package resource reading, opening, and access

**Source code:** :source:`Lib/importlib/resources/__init__.py`

--------------

.. versionadded:: 3.7

This module leverages Python's import system to provide access to *resources*
within *packages*.

"Resources" are file-like resources associated with a module or package in
Python. The resources may be contained directly in a package, within a
subdirectory contained in that package, or adjacent to modules outside a
package. Resources may be text or binary. As a result, Python module sources
(.py) of a package and compilation artifacts (pycache) are technically
de-facto resources of that package. In practice, however, resources are
primarily those non-Python artifacts exposed specifically by the package
author.

Resources can be opened or read in either binary or text mode.

Resources are roughly akin to files inside directories, though it's important
to keep in mind that this is just a metaphor.  Resources and packages **do
not** have to exist as physical files and directories on the file system:
for example, a package and its resources can be imported from a zip file using
:py:mod:`zipimport`.

.. note::

   This module provides functionality similar to `pkg_resources
   <https://setuptools.readthedocs.io/en/latest/pkg_resources.html>`_ `Basic
   Resource Access
   <https://setuptools.readthedocs.io/en/latest/pkg_resources.html#basic-resource-access>`_
   without the performance overhead of that package.  This makes reading
   resources included in packages easier, with more stable and consistent
   semantics.

   The standalone backport of this module provides more information
   on `using importlib.resources
   <https://importlib-resources.readthedocs.io/en/latest/using.html>`_ and
   `migrating from pkg_resources to importlib.resources
   <https://importlib-resources.readthedocs.io/en/latest/migration.html>`_.

:class:`Loaders <importlib.abc.Loader>` that wish to support resource reading should implement a
``get_resource_reader(fullname)`` method as specified by
:class:`importlib.resources.abc.ResourceReader`.

.. class:: Anchor

    Represents an anchor for resources, either a :class:`module object
    <types.ModuleType>` or a module name as a string. Defined as
    ``Union[str, ModuleType]``.

.. function:: files(anchor: Optional[Anchor] = None)

    Returns a :class:`~importlib.resources.abc.Traversable` object
    representing the resource container (think directory) and its resources
    (think files). A Traversable may contain other containers (think
    subdirectories).

    *anchor* is an optional :class:`Anchor`. If the anchor is a
    package, resources are resolved from that package. If a module,
    resources are resolved adjacent to that module (in the same package
    or the package root). If the anchor is omitted, the caller's module
    is used.

    .. versionadded:: 3.9

    .. versionchanged:: 3.12
       *package* parameter was renamed to *anchor*. *anchor* can now
       be a non-package module and if omitted will default to the caller's
       module. *package* is still accepted for compatibility but will raise
       a :exc:`DeprecationWarning`. Consider passing the anchor positionally or
       using ``importlib_resources >= 5.10`` for a compatible interface
       on older Pythons.

.. function:: as_file(traversable)

    Given a :class:`~importlib.resources.abc.Traversable` object representing
    a file or directory, typically from :func:`importlib.resources.files`,
    return a context manager for use in a :keyword:`with` statement.
    The context manager provides a :class:`pathlib.Path` object.

    Exiting the context manager cleans up any temporary file or directory
    created when the resource was extracted from e.g. a zip file.

    Use ``as_file`` when the Traversable methods
    (``read_text``, etc) are insufficient and an actual file or directory on
    the file system is required.

    .. versionadded:: 3.9

    .. versionchanged:: 3.12
       Added support for *traversable* representing a directory.


Deprecated functions
^^^^^^^^^^^^^^^^^^^^

An older, deprecated set of functions is still available, but is
scheduled for removal in a future version of Python.
The main drawback of these functions is that they do not support
directories: they assume all resources are located directly within a *package*.

.. data:: Package

    Whenever a function accepts a ``Package`` argument, you can pass in
    either a :class:`module object <types.ModuleType>` or a module name
    as a string.  You can only pass module objects whose
    ``__spec__.submodule_search_locations`` is not ``None``.

    The ``Package`` type is defined as ``Union[str, ModuleType]``.

   .. deprecated:: 3.12


.. data:: Resource

    For *resource* arguments of the functions below, you can pass in
    the name of a resource as a string or
    a :class:`path-like object <os.PathLike>`.

    The ``Resource`` type is defined as ``Union[str, os.PathLike]``.


.. function:: open_binary(package, resource)

    Open for binary reading the *resource* within *package*.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.  *resource* is the name of the resource to open
    within *package*; it may not contain path separators and it may not have
    sub-resources (i.e. it cannot be a directory).  This function returns a
    ``typing.BinaryIO`` instance, a binary I/O stream open for reading.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          files(package).joinpath(resource).open('rb')


.. function:: open_text(package, resource, encoding='utf-8', errors='strict')

    Open for text reading the *resource* within *package*.  By default, the
    resource is opened for reading as UTF-8.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.  *resource* is the name of the resource to open
    within *package*; it may not contain path separators and it may not have
    sub-resources (i.e. it cannot be a directory).  *encoding* and *errors*
    have the same meaning as with built-in :func:`open`.

    This function returns a ``typing.TextIO`` instance, a text I/O stream open
    for reading.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          files(package).joinpath(resource).open('r', encoding=encoding)


.. function:: read_binary(package, resource)

    Read and return the contents of the *resource* within *package* as
    ``bytes``.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.  *resource* is the name of the resource to open
    within *package*; it may not contain path separators and it may not have
    sub-resources (i.e. it cannot be a directory).  This function returns the
    contents of the resource as :class:`bytes`.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          files(package).joinpath(resource).read_bytes()


.. function:: read_text(package, resource, encoding='utf-8', errors='strict')

    Read and return the contents of *resource* within *package* as a ``str``.
    By default, the contents are read as strict UTF-8.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.  *resource* is the name of the resource to open
    within *package*; it may not contain path separators and it may not have
    sub-resources (i.e. it cannot be a directory).  *encoding* and *errors*
    have the same meaning as with built-in :func:`open`.  This function
    returns the contents of the resource as :class:`str`.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          files(package).joinpath(resource).read_text(encoding=encoding)


.. function:: path(package, resource)

    Return the path to the *resource* as an actual file system path.  This
    function returns a context manager for use in a :keyword:`with` statement.
    The context manager provides a :class:`pathlib.Path` object.

    Exiting the context manager cleans up any temporary file created when the
    resource needs to be extracted from e.g. a zip file.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.  *resource* is the name of the resource to open
    within *package*; it may not contain path separators and it may not have
    sub-resources (i.e. it cannot be a directory).

    .. deprecated:: 3.11

       Calls to this function can be replaced using :func:`as_file`::

          as_file(files(package).joinpath(resource))


.. function:: is_resource(package, name)

    Return ``True`` if there is a resource named *name* in the package,
    otherwise ``False``.
    This function does not consider directories to be resources.
    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          files(package).joinpath(resource).is_file()


.. function:: contents(package)

    Return an iterable over the named items within the package.  The iterable
    returns :class:`str` resources (e.g. files) and non-resources
    (e.g. directories).  The iterable does not recurse into subdirectories.

    *package* is either a name or a module object which conforms to the
    ``Package`` requirements.

    .. deprecated:: 3.11

       Calls to this function can be replaced by::

          (resource.name for resource in files(package).iterdir() if resource.is_file())
