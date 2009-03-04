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
implementaiton of :keyword:`import` which is portable to any Python
interpreter. This also provides a reference implementation which is easier to
comprehend than one in a programming language other than Python.

Two, the components to implement :keyword:`import` can be exposed in this
package, making it easier for users to create their own custom objects (known
generically as an :term:`importer`) to participate in the import process.
Details on providing custom importers can be found in :pep:`302`.

.. seealso::

    :ref:`import`
        The language reference for the :keyword:`import` statement.

    `Packages specification <http://www.python.org/doc/essays/packages.html>`__
        Original specification of packages. Some semantics have changed since
        the writing of this document (e.g. redirecting based on :keyword:`None`
        in :data:`sys.modules`).

    The :func:`.__import__` function
        The built-in function for which the :keyword:`import` statement is
        syntactic sugar for.

    :pep:`235`
        Import on Case-Insensitive Platforms

    :pep:`263`
        Defining Python Source Code Encodings

    :pep:`302`
        New Import Hooks.

    :pep:`328`
        Imports: Multi-Line and Absolute/Relative

    :pep:`366`
        Main module explicit relative imports

    :pep:`3128`
        Using UTF-8 as the Default Source Encoding


Functions
---------

.. function:: __import__(name, globals={}, locals={}, fromlist=list(), level=0)

    An implementation of the built-in :func:`__import__` function. See the
    built-in function's documentation for usage instructions.

.. function:: import_module(name, package=None)

    Import a module. The *name* argument specifies what module to
    import in absolute or relative terms
    (e.g. either ``pkg.mod`` or ``..mod``). If the name is
    specified in relative terms, then the *package* argument must be
    set to the package which is to act as the anchor for resolving the
    package name (e.g. ``import_module('..mod', 'pkg.subpkg')`` will import
    ``pkg.mod``).

    The :func:`import_module` function acts as a simplifying wrapper around
    :func:`__import__`. This means all semantics of the function are derived
    from :func:`__import__`, including requiring the package where an import is
    occuring from to already be imported (i.e., *package* must already be
    imported).

:mod:`importlib.machinery` -- Importers and path hooks
------------------------------------------------------

.. module:: importlib.machinery
    :synopsis: Importers and path hooks

This module contains the various objects that help :keyword:`import`
find and load modules.

.. class:: BuiltinImporter

    :term:`Importer` for built-in modules. All known built-in modules are
    listed in :data:`sys.builtin_module_names`.

    Only class methods are defined by this class to alleviate the need for
    instantiation.

    .. classmethod:: find_module(fullname, path=None)

        Class method that allows this class to be a :term:`finder` for built-in
        modules.

    .. classmethod:: load_module(fullname)

        Class method that allows this class to be a :term:`loader` for built-in
        modules.


.. class:: FrozenImporter

    :term:`Importer` for frozen modules.

    Only class methods are defined by this class to alleviate the need for
    instantiation.

    .. classmethod:: find_module(fullname, path=None)

        Class method that allows this class to be a :term:`finder` for frozen
        modules.

    .. classmethod:: load_module(fullname)

        Class method that allows this class to be a :term:`loader` for frozen
        modules.


.. class:: PathFinder

    :term:`Finder` for :data:`sys.path`.

    This class does not perfectly mirror the semantics of :keyword:`import` in
    terms of :data:`sys.path`. No implicit path hooks are assumed for
    simplification of the class and its semantics.

    Only class method are defined by this class to alleviate the need for
    instantiation.

    .. classmethod:: find_module(fullname, path=None)

        Class method that attempts to find a :term:`loader` for the module
        specified by *fullname* either on :data:`sys.path` or, if defined, on
        *path*. For each path entry that is searched,
        :data:`sys.path_importer_cache` is checked. If an non-false object is
        found then it is used as the :term:`finder` to query for the module
        being searched for. For no entry is found in
        :data:`sys.path_importer_cache`, then :data:`sys.path_hooks` is
        searched for a finder for the path entry and, if found, is stored in
        :data:`sys.path_importer_cache` along with being queried about the
        module.


:mod:`importlib.util` -- Utility code for importers
---------------------------------------------------

.. module:: importlib.util
    :synopsis: Importers and path hooks

This module contains the various objects that help in the construction of
an :term:`importer`.

.. function:: module_for_loader(method)

    A :term:`decorator` for a :term:`loader` which handles selecting the proper
    module object to load with. The decorated method is expected to have a call
    signature of ``method(self, module_object)`` for which the second argument
    will be the module object to be used by the loader (note that the decorator
    will not work on static methods because of the assumption of two
    arguments).

    The decorated method will take in the name of the module to be loaded as
    expected for a :term:`loader`. If the module is not found in
    :data:`sys.modules` then a new one is constructed with its
    :attr:`__name__` attribute set. Otherwise the module found in
    :data:`sys.modules` will be passed into the method. If an
    exception is raised by the decorated method and a module was added to
    :data:`sys.modules` it will be removed to prevent a partially initialized
    module from being in left in :data:`sys.modules`. If the module was already
    in :data:`sys.modules` then it is left alone.

    Use of this decorator handles all the details of what module object a
    loader should initialize as specified by :pep:`302`.


.. function:: set_package(method)

    A :term:`decorator` for a :term:`loader` to set the :attr:`__package__`
    attribute on the module returned by the loader. If :attr:`__package__` is
    set and has a value other than :keyword:`None` it will not be changed.
    Note that the module returned by the loader is what has the attribute
    set on and not the module found in :data:`sys.modules`.
