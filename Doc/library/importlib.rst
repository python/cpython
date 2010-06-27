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
        syntactic sugar.

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

    :pep:`3120`
        Using UTF-8 as the Default Source Encoding

    :pep:`3147`
        PYC Repository Directories


Functions
---------

.. function:: __import__(name, globals={}, locals={}, fromlist=list(), level=0)

    An implementation of the built-in :func:`__import__` function. See the
    built-in function's documentation for usage instructions.

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
        cannot be found, :keyword:`None` is returned.


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
        :keyword:`None` is returned if the module does not have a code object
        (e.g. built-in module).  :exc:`ImportError` is raised if loader cannot
        find the requested module.

    .. method:: get_source(fullname)

        An abstract method to return the source of a module. It is returned as
        a text string with universal newlines. Returns :keyword:`None` if no
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

        An abstract method that is to return the value for :attr:`__file__` for
        the specified module. If no path is available, :exc:`ImportError` is
        raised.


.. class:: PyLoader

    An abstract base class inheriting from
    :class:`importlib.abc.ExecutionLoader` and
    :class:`importlib.abc.ResourceLoader` designed to ease the loading of
    Python source modules (bytecode is not handled; see
    :class:`importlib.abc.PyPycLoader` for a source/bytecode ABC). A subclass
    implementing this ABC will only need to worry about exposing how the source
    code is stored; all other details for loading Python source code will be
    handled by the concrete implementations of key methods.

    .. method:: source_path(fullname)

        An abstract method that returns the path to the source code for a
        module. Should return :keyword:`None` if there is no source code.
        Raises :exc:`ImportError` if the loader knows it cannot handle the
        module.

    .. method:: get_filename(fullname)

        A concrete implementation of
        :meth:`importlib.abc.ExecutionLoader.get_filename` that
        relies on :meth:`source_path`. If :meth:`source_path` returns
        :keyword:`None`, then :exc:`ImportError` is raised.

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

    An abstract base class inheriting from :class:`importlib.abc.PyLoader`.
    This ABC is meant to help in creating loaders that support both Python
    source and bytecode.

    .. method:: source_mtime(fullname)

        An abstract method which returns the modification time for the source
        code of the specified module. The modification time should be an
        integer. If there is no source code, return :keyword:`None`. If the
        module cannot be found then :exc:`ImportError` is raised.

    .. method:: bytecode_path(fullname)

        An abstract method which returns the path to the bytecode for the
        specified module, if it exists. It returns :keyword:`None`
        if no bytecode exists (yet).
        Raises :exc:`ImportError` if the loader knows it cannot handle the
        module.

    .. method:: get_filename(fullname)

        A concrete implementation of
        :meth:`importlib.abc.ExecutionLoader.get_filename` that relies on
        :meth:`importlib.abc.PyLoader.source_path` and :meth:`bytecode_path`.
        If :meth:`source_path` returns a path, then that value is returned.
        Else if :meth:`bytecode_path` returns a path, that path will be
        returned. If a path is not available from both methods,
        :exc:`ImportError` is raised.

    .. method:: write_bytecode(fullname, bytecode)

        An abstract method which has the loader write *bytecode* for future
        use. If the bytecode is written, return :keyword:`True`. Return
        :keyword:`False` if the bytecode could not be written. This method
        should not be called if :data:`sys.dont_write_bytecode` is true.
        The *bytecode* argument should be a bytes string or bytes array.


:mod:`importlib.machinery` -- Importers and path hooks
------------------------------------------------------

.. module:: importlib.machinery
    :synopsis: Importers and path hooks

This module contains the various objects that help :keyword:`import`
find and load modules.

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
    simplification of the class and its semantics.

    Only class methods are defined by this class to alleviate the need for
    instantiation.

    .. classmethod:: find_module(fullname, path=None)

        Class method that attempts to find a :term:`loader` for the module
        specified by *fullname* on :data:`sys.path` or, if defined, on
        *path*. For each path entry that is searched,
        :data:`sys.path_importer_cache` is checked. If an non-false object is
        found then it is used as the :term:`finder` to look for the module
        being searched for. If no entry is found in
        :data:`sys.path_importer_cache`, then :data:`sys.path_hooks` is
        searched for a finder for the path entry and, if found, is stored in
        :data:`sys.path_importer_cache` along with being queried about the
        module. If no finder is ever found then :keyword:`None` is returned.


:mod:`importlib.util` -- Utility code for importers
---------------------------------------------------

.. module:: importlib.util
    :synopsis: Importers and path hooks

This module contains the various objects that help in the construction of
an :term:`importer`.

.. function:: module_for_loader(method)

    A :term:`decorator` for a :term:`loader` method,
    to handle selecting the proper
    module object to load with. The decorated method is expected to have a call
    signature taking two positional arguments
    (e.g. ``load_module(self, module)``) for which the second argument
    will be the module **object** to be used by the loader.
    Note that the decorator
    will not work on static methods because of the assumption of two
    arguments.

    The decorated method will take in the **name** of the module to be loaded
    as expected for a :term:`loader`. If the module is not found in
    :data:`sys.modules` then a new one is constructed with its
    :attr:`__name__` attribute set. Otherwise the module found in
    :data:`sys.modules` will be passed into the method. If an
    exception is raised by the decorated method and a module was added to
    :data:`sys.modules` it will be removed to prevent a partially initialized
    module from being in left in :data:`sys.modules`. If the module was already
    in :data:`sys.modules` then it is left alone.

    Use of this decorator handles all the details of which module object a
    loader should initialize as specified by :pep:`302`.

.. function:: set_loader(fxn)

    A :term:`decorator` for a :term:`loader` method,
    to set the :attr:`__loader__`
    attribute on loaded modules. If the attribute is already set the decorator
    does nothing. It is assumed that the first positional argument to the
    wrapped method is what :attr:`__loader__` should be set to.

.. function:: set_package(fxn)

    A :term:`decorator` for a :term:`loader` to set the :attr:`__package__`
    attribute on the module returned by the loader. If :attr:`__package__` is
    set and has a value other than :keyword:`None` it will not be changed.
    Note that the module returned by the loader is what has the attribute
    set on and not the module found in :data:`sys.modules`.

    Reliance on this decorator is discouraged when it is possible to set
    :attr:`__package__` before the execution of the code is possible. By
    setting it before the code for the module is executed it allows the
    attribute to be used at the global level of the module during
    initialization.


Example
-------

Below is an example meta path importer that uses a dict for back-end storage
for source code. While not an optimal solution -- manipulations of
:attr:`__path__` on packages does not influence import -- it does illustrate
what little is required to implement an importer.

.. testcode::

    """An importer where source is stored in a dict."""
    from importlib import abc


    class DictImporter(abc.Finder, abc.PyLoader):

        """A meta path importer that stores source code in a dict.

        The keys are the module names -- packages must end in ``.__init__``.
        The values must be something that can be passed to 'bytes'.

        """

        def __init__(self, memory):
            """Store the dict."""
            self.memory = memory

        def contains(self, name):
            """See if a module or package is in the dict."""
            if name in self.memory:
                return name
            package_name = '{}.__init__'.format(name)
            if  package_name in self.memory:
                return package_name
            return False

        __contains__ = contains  # Convenience.

        def find_module(self, fullname, path=None):
            """Find the module in the dict."""
            if fullname in self:
                return self
            return None

        def source_path(self, fullname):
            """Return the module name if the module is in the dict."""
            if not fullname in self:
                raise ImportError
            return fullname

        def get_data(self, path):
            """Return the bytes for the source.

            The value found in the dict is passed through 'bytes' before being
            returned.

            """
            name = self.contains(path)
            if not name:
                raise IOError
            return bytes(self.memory[name])

        def is_package(self, fullname):
            """Tell if module is a package based on whether the dict contains the
            name with ``.__init__`` appended to it."""
            if fullname not in self:
                raise ImportError
            if fullname in self.memory:
                return False
            # If name is in this importer but not as it is then it must end in
            # ``__init__``.
            else:
                return True

.. testcode::
    :hide:

    import importlib
    import sys


    # Build the dict; keys of name, value of __package__.
    names = {'_top_level': '', '_pkg.__init__': '_pkg', '_pkg.mod': '_pkg'}
    source = {name: "name = {!r}".format(name).encode() for name in names}

    # Register the meta path importer.
    importer = DictImporter(source)
    sys.meta_path.append(importer)

    # Sanity check.
    for name in names:
        module = importlib.import_module(name)
        assert module.__name__ == name
        assert getattr(module, 'name') == name
        assert module.__loader__ is importer
        assert module.__package__ == names[name]
