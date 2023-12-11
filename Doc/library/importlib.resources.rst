:mod:`importlib.resources` -- Package resource reading, opening and access
--------------------------------------------------------------------------

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
