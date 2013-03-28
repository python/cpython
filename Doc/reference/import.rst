
.. _importsystem:

*****************
The import system
*****************

.. index:: single: import machinery

Python code in one :term:`module` gains access to the code in another module
by the process of :term:`importing` it.  The :keyword:`import` statement is
the most common way of invoking the import machinery, but it is not the only
way.  Functions such as :func:`importlib.import_module` and built-in
:func:`__import__` can also be used to invoke the import machinery.

The :keyword:`import` statement combines two operations; it searches for the
named module, then it binds the results of that search to a name in the local
scope.  The search operation of the :keyword:`import` statement is defined as
a call to the :func:`__import__` function, with the appropriate arguments.
The return value of :func:`__import__` is used to perform the name
binding operation of the :keyword:`import` statement.  See the
:keyword:`import` statement for the exact details of that name binding
operation.

A direct call to :func:`__import__` performs only the module search and, if
found, the module creation operation.  While certain side-effects may occur,
such as the importing of parent packages, and the updating of various caches
(including :data:`sys.modules`), only the :keyword:`import` statement performs
a name binding operation.

When calling :func:`__import__` as part of an import statement, the
import system first checks the module global namespace for a function by
that name. If it is not found, then the standard builtin :func:`__import__`
is called. Other mechanisms for invoking the import system (such as
:func:`importlib.import_module`) do not perform this check and will always
use the standard import system.

When a module is first imported, Python searches for the module and if found,
it creates a module object [#fnmo]_, initializing it.  If the named module
cannot be found, an :exc:`ImportError` is raised.  Python implements various
strategies to search for the named module when the import machinery is
invoked.  These strategies can be modified and extended by using various hooks
described in the sections below.

.. versionchanged:: 3.3
   The import system has been updated to fully implement the second phase
   of :pep:`302`. There is no longer any implicit import machinery - the full
   import system is exposed through :data:`sys.meta_path`. In addition,
   native namespace package support has been implemented (see :pep:`420`).


:mod:`importlib`
================

The :mod:`importlib` module provides a rich API for interacting with the
import system.  For example :func:`importlib.import_module` provides a
recommended, simpler API than built-in :func:`__import__` for invoking the
import machinery.  Refer to the :mod:`importlib` library documentation for
additional detail.



Packages
========

.. index::
    single: package

Python has only one type of module object, and all modules are of this type,
regardless of whether the module is implemented in Python, C, or something
else.  To help organize modules and provide a naming hierarchy, Python has a
concept of :term:`packages <package>`.

You can think of packages as the directories on a file system and modules as
files within directories, but don't take this analogy too literally since
packages and modules need not originate from the file system.  For the
purposes of this documentation, we'll use this convenient analogy of
directories and files.  Like file system directories, packages are organized
hierarchically, and packages may themselves contain subpackages, as well as
regular modules.

It's important to keep in mind that all packages are modules, but not all
modules are packages.  Or put another way, packages are just a special kind of
module.  Specifically, any module that contains a ``__path__`` attribute is
considered a package.

All modules have a name.  Subpackage names are separated from their parent
package name by dots, akin to Python's standard attribute access syntax.  Thus
you might have a module called :mod:`sys` and a package called :mod:`email`,
which in turn has a subpackage called :mod:`email.mime` and a module within
that subpackage called :mod:`email.mime.text`.


Regular packages
----------------

.. index::
    pair: package; regular

Python defines two types of packages, :term:`regular packages <regular
package>` and :term:`namespace packages <namespace package>`.  Regular
packages are traditional packages as they existed in Python 3.2 and earlier.
A regular package is typically implemented as a directory containing an
``__init__.py`` file.  When a regular package is imported, this
``__init__.py`` file is implicitly executed, and the objects it defines are
bound to names in the package's namespace.  The ``__init__.py`` file can
contain the same Python code that any other module can contain, and Python
will add some additional attributes to the module when it is imported.

For example, the following file system layout defines a top level ``parent``
package with three subpackages::

    parent/
        __init__.py
        one/
            __init__.py
        two/
            __init__.py
        three/
            __init__.py

Importing ``parent.one`` will implicitly execute ``parent/__init__.py`` and
``parent/one/__init__.py``.  Subsequent imports of ``parent.two`` or
``parent.three`` will execute ``parent/two/__init__.py`` and
``parent/three/__init__.py`` respectively.


Namespace packages
------------------

.. index::
    pair:: package; namespace
    pair:: package; portion

A namespace package is a composite of various :term:`portions <portion>`,
where each portion contributes a subpackage to the parent package.  Portions
may reside in different locations on the file system.  Portions may also be
found in zip files, on the network, or anywhere else that Python searches
during import.  Namespace packages may or may not correspond directly to
objects on the file system; they may be virtual modules that have no concrete
representation.

Namespace packages do not use an ordinary list for their ``__path__``
attribute. They instead use a custom iterable type which will automatically
perform a new search for package portions on the next import attempt within
that package if the path of their parent package (or :data:`sys.path` for a
top level package) changes.

With namespace packages, there is no ``parent/__init__.py`` file.  In fact,
there may be multiple ``parent`` directories found during import search, where
each one is provided by a different portion.  Thus ``parent/one`` may not be
physically located next to ``parent/two``.  In this case, Python will create a
namespace package for the top-level ``parent`` package whenever it or one of
its subpackages is imported.

See also :pep:`420` for the namespace package specification.


Searching
=========

To begin the search, Python needs the :term:`fully qualified <qualified name>`
name of the module (or package, but for the purposes of this discussion, the
difference is immaterial) being imported.  This name may come from various
arguments to the :keyword:`import` statement, or from the parameters to the
:func:`importlib.import_module` or :func:`__import__` functions.

This name will be used in various phases of the import search, and it may be
the dotted path to a submodule, e.g. ``foo.bar.baz``.  In this case, Python
first tries to import ``foo``, then ``foo.bar``, and finally ``foo.bar.baz``.
If any of the intermediate imports fail, an :exc:`ImportError` is raised.


The module cache
----------------

.. index::
    single: sys.modules

The first place checked during import search is :data:`sys.modules`.  This
mapping serves as a cache of all modules that have been previously imported,
including the intermediate paths.  So if ``foo.bar.baz`` was previously
imported, :data:`sys.modules` will contain entries for ``foo``, ``foo.bar``,
and ``foo.bar.baz``.  Each key will have as its value the corresponding module
object.

During import, the module name is looked up in :data:`sys.modules` and if
present, the associated value is the module satisfying the import, and the
process completes.  However, if the value is ``None``, then an
:exc:`ImportError` is raised.  If the module name is missing, Python will
continue searching for the module.

:data:`sys.modules` is writable.  Deleting a key may not destroy the
associated module (as other modules may hold references to it),
but it will invalidate the cache entry for the named module, causing
Python to search anew for the named module upon its next
import. The key can also be assigned to ``None``, forcing the next import
of the module to result in an :exc:`ImportError`.

Beware though, as if you keep a reference to the module object,
invalidate its cache entry in :data:`sys.modules`, and then re-import the
named module, the two module objects will *not* be the same. By contrast,
:func:`imp.reload` will reuse the *same* module object, and simply
reinitialise the module contents by rerunning the module's code.


Finders and loaders
-------------------

.. index::
    single: finder
    single: loader

If the named module is not found in :data:`sys.modules`, then Python's import
protocol is invoked to find and load the module.  This protocol consists of
two conceptual objects, :term:`finders <finder>` and :term:`loaders <loader>`.
A finder's job is to determine whether it can find the named module using
whatever strategy it knows about. Objects that implement both of these
interfaces are referred to as :term:`importers <importer>` - they return
themselves when they find that they can load the requested module.

Python includes a number of default finders and importers.  The first one
knows how to locate built-in modules, and the second knows how to locate
frozen modules.  A third default finder searches an :term:`import path`
for modules.  The :term:`import path` is a list of locations that may
name file system paths or zip files.  It can also be extended to search
for any locatable resource, such as those identified by URLs.

The import machinery is extensible, so new finders can be added to extend the
range and scope of module searching.

Finders do not actually load modules.  If they can find the named module, they
return a :term:`loader`, which the import machinery then invokes to load the
module and create the corresponding module object.

The following sections describe the protocol for finders and loaders in more
detail, including how you can create and register new ones to extend the
import machinery.


Import hooks
------------

.. index::
   single: import hooks
   single: meta hooks
   single: path hooks
   pair: hooks; import
   pair: hooks; meta
   pair: hooks; path

The import machinery is designed to be extensible; the primary mechanism for
this are the *import hooks*.  There are two types of import hooks: *meta
hooks* and *import path hooks*.

Meta hooks are called at the start of import processing, before any other
import processing has occurred, other than :data:`sys.modules` cache look up.
This allows meta hooks to override :data:`sys.path` processing, frozen
modules, or even built-in modules.  Meta hooks are registered by adding new
finder objects to :data:`sys.meta_path`, as described below.

Import path hooks are called as part of :data:`sys.path` (or
``package.__path__``) processing, at the point where their associated path
item is encountered.  Import path hooks are registered by adding new callables
to :data:`sys.path_hooks` as described below.


The meta path
-------------

.. index::
    single: sys.meta_path
    pair: finder; find_module
    pair: finder; find_loader

When the named module is not found in :data:`sys.modules`, Python next
searches :data:`sys.meta_path`, which contains a list of meta path finder
objects.  These finders are queried in order to see if they know how to handle
the named module.  Meta path finders must implement a method called
:meth:`find_module()` which takes two arguments, a name and an import path.
The meta path finder can use any strategy it wants to determine whether it can
handle the named module or not.

If the meta path finder knows how to handle the named module, it returns a
loader object.  If it cannot handle the named module, it returns ``None``.  If
:data:`sys.meta_path` processing reaches the end of its list without returning
a loader, then an :exc:`ImportError` is raised.  Any other exceptions raised
are simply propagated up, aborting the import process.

The :meth:`find_module()` method of meta path finders is called with two
arguments.  The first is the fully qualified name of the module being
imported, for example ``foo.bar.baz``.  The second argument is the path
entries to use for the module search.  For top-level modules, the second
argument is ``None``, but for submodules or subpackages, the second
argument is the value of the parent package's ``__path__`` attribute. If
the appropriate ``__path__`` attribute cannot be accessed, an
:exc:`ImportError` is raised.

The meta path may be traversed multiple times for a single import request.
For example, assuming none of the modules involved has already been cached,
importing ``foo.bar.baz`` will first perform a top level import, calling
``mpf.find_module("foo", None)`` on each meta path finder (``mpf``). After
``foo`` has been imported, ``foo.bar`` will be imported by traversing the
meta path a second time, calling
``mpf.find_module("foo.bar", foo.__path__)``. Once ``foo.bar`` has been
imported, the final traversal will call
``mpf.find_module("foo.bar.baz", foo.bar.__path__)``.

Some meta path finders only support top level imports. These importers will
always return ``None`` when anything other than ``None`` is passed as the
second argument.

Python's default :data:`sys.meta_path` has three meta path finders, one that
knows how to import built-in modules, one that knows how to import frozen
modules, and one that knows how to import modules from an :term:`import path`
(i.e. the :term:`path based finder`).


Loaders
=======

If and when a module loader is found its
:meth:`~importlib.abc.Loader.load_module` method is called, with a single
argument, the fully qualified name of the module being imported.  This method
has several responsibilities, and should return the module object it has
loaded [#fnlo]_.  If it cannot load the module, it should raise an
:exc:`ImportError`, although any other exception raised during
:meth:`load_module()` will be propagated.

In many cases, the finder and loader can be the same object; in such cases the
:meth:`finder.find_module()` would just return ``self``.

Loaders must satisfy the following requirements:

 * If there is an existing module object with the given name in
   :data:`sys.modules`, the loader must use that existing module.  (Otherwise,
   :func:`imp.reload` will not work correctly.)  If the named module does
   not exist in :data:`sys.modules`, the loader must create a new module
   object and add it to :data:`sys.modules`.

   Note that the module *must* exist in :data:`sys.modules` before the loader
   executes the module code.  This is crucial because the module code may
   (directly or indirectly) import itself; adding it to :data:`sys.modules`
   beforehand prevents unbounded recursion in the worst case and multiple
   loading in the best.

   If loading fails, the loader must remove any modules it has inserted into
   :data:`sys.modules`, but it must remove **only** the failing module, and
   only if the loader itself has loaded it explicitly.  Any module already in
   the :data:`sys.modules` cache, and any module that was successfully loaded
   as a side-effect, must remain in the cache.

 * The loader may set the ``__file__`` attribute of the module.  If set, this
   attribute's value must be a string.  The loader may opt to leave
   ``__file__`` unset if it has no semantic meaning (e.g. a module loaded from
   a database).

 * The loader may set the ``__name__`` attribute of the module.  While not
   required, setting this attribute is highly recommended so that the
   :meth:`repr()` of the module is more informative.

 * If the module is a package (either regular or namespace), the loader must
   set the module object's ``__path__`` attribute.  The value must be
   iterable, but may be empty if ``__path__`` has no further significance
   to the loader. If ``__path__`` is not empty, it must produce strings
   when iterated over. More details on the semantics of ``__path__`` are
   given :ref:`below <package-path-rules>`.

 * The ``__loader__`` attribute must be set to the loader object that loaded
   the module.  This is mostly for introspection and reloading, but can be
   used for additional loader-specific functionality, for example getting
   data associated with a loader. If the attribute is missing or set to ``None``
   then the import machinery will automatically set it **after** the module has
   been imported.

 * The module's ``__package__`` attribute must be set.  Its value must be a
   string, but it can be the same value as its ``__name__``.  If the attribute
   is set to ``None`` or is missing, the import system will fill it in with a
   more appropriate value **after** the module has been imported.
   When the module is a package, its ``__package__`` value should be set to its
   ``__name__``.  When the module is not a package, ``__package__`` should be
   set to the empty string for top-level modules, or for submodules, to the
   parent package's name.  See :pep:`366` for further details.

   This attribute is used instead of ``__name__`` to calculate explicit
   relative imports for main modules, as defined in :pep:`366`.

 * If the module is a Python module (as opposed to a built-in module or a
   dynamically loaded extension), the loader should execute the module's code
   in the module's global name space (``module.__dict__``).


Module reprs
------------

By default, all modules have a usable repr, however depending on the
attributes set above, and hooks in the loader, you can more explicitly control
the repr of module objects.

Loaders may implement a :meth:`module_repr()` method which takes a single
argument, the module object.  When ``repr(module)`` is called for a module
with a loader supporting this protocol, whatever is returned from
``module.__loader__.module_repr(module)`` is returned as the module's repr
without further processing.  This return value must be a string.

If the module has no ``__loader__`` attribute, or the loader has no
:meth:`module_repr()` method, then the module object implementation itself
will craft a default repr using whatever information is available.  It will
try to use the ``module.__name__``, ``module.__file__``, and
``module.__loader__`` as input into the repr, with defaults for whatever
information is missing.

Here are the exact rules used:

 * If the module has a ``__loader__`` and that loader has a
   :meth:`module_repr()` method, call it with a single argument, which is the
   module object.  The value returned is used as the module's repr.

 * If an exception occurs in :meth:`module_repr()`, the exception is caught
   and discarded, and the calculation of the module's repr continues as if
   :meth:`module_repr()` did not exist.

 * If the module has a ``__file__`` attribute, this is used as part of the
   module's repr.

 * If the module has no ``__file__`` but does have a ``__loader__``, then the
   loader's repr is used as part of the module's repr.

 * Otherwise, just use the module's ``__name__`` in the repr.

This example, from :pep:`420` shows how a loader can craft its own module
repr::

    class NamespaceLoader:
        @classmethod
        def module_repr(cls, module):
            return "<module '{}' (namespace)>".format(module.__name__)


.. _package-path-rules:

module.__path__
---------------

By definition, if a module has an ``__path__`` attribute, it is a package,
regardless of its value.

A package's ``__path__`` attribute is used during imports of its subpackages.
Within the import machinery, it functions much the same as :data:`sys.path`,
i.e. providing a list of locations to search for modules during import.
However, ``__path__`` is typically much more constrained than
:data:`sys.path`.

``__path__`` must be an iterable of strings, but it may be empty.
The same rules used for :data:`sys.path` also apply to a package's
``__path__``, and :data:`sys.path_hooks` (described below) are
consulted when traversing a package's ``__path__``.

A package's ``__init__.py`` file may set or alter the package's ``__path__``
attribute, and this was typically the way namespace packages were implemented
prior to :pep:`420`.  With the adoption of :pep:`420`, namespace packages no
longer need to supply ``__init__.py`` files containing only ``__path__``
manipulation code; the namespace loader automatically sets ``__path__``
correctly for the namespace package.


The Path Based Finder
=====================

.. index::
    single: path based finder

As mentioned previously, Python comes with several default meta path finders.
One of these, called the :term:`path based finder`, searches an :term:`import
path`, which contains a list of :term:`path entries <path entry>`.  Each path
entry names a location to search for modules.

The path based finder itself doesn't know how to import anything. Instead, it
traverses the individual path entries, associating each of them with a
path entry finder that knows how to handle that particular kind of path.

The default set of path entry finders implement all the semantics for finding
modules on the file system, handling special file types such as Python source
code (``.py`` files), Python byte code (``.pyc`` and ``.pyo`` files) and
shared libraries (e.g. ``.so`` files). When supported by the :mod:`zipimport`
module in the standard library, the default path entry finders also handle
loading all of these file types (other than shared libraries) from zipfiles.

Path entries need not be limited to file system locations.  They can refer to
URLs, database queries, or any other location that can be specified as a
string.

The path based finder provides additional hooks and protocols so that you
can extend and customize the types of searchable path entries.  For example,
if you wanted to support path entries as network URLs, you could write a hook
that implements HTTP semantics to find modules on the web.  This hook (a
callable) would return a :term:`path entry finder` supporting the protocol
described below, which was then used to get a loader for the module from the
web.

A word of warning: this section and the previous both use the term *finder*,
distinguishing between them by using the terms :term:`meta path finder` and
:term:`path entry finder`.  These two types of finders are very similar,
support similar protocols, and function in similar ways during the import
process, but it's important to keep in mind that they are subtly different.
In particular, meta path finders operate at the beginning of the import
process, as keyed off the :data:`sys.meta_path` traversal.

By contrast, path entry finders are in a sense an implementation detail
of the path based finder, and in fact, if the path based finder were to be
removed from :data:`sys.meta_path`, none of the path entry finder semantics
would be invoked.


Path entry finders
------------------

.. index::
    single: sys.path
    single: sys.path_hooks
    single: sys.path_importer_cache
    single: PYTHONPATH

The :term:`path based finder` is responsible for finding and loading Python
modules and packages whose location is specified with a string :term:`path
entry`.  Most path entries name locations in the file system, but they need
not be limited to this.

As a meta path finder, the :term:`path based finder` implements the
:meth:`find_module()` protocol previously described, however it exposes
additional hooks that can be used to customize how modules are found and
loaded from the :term:`import path`.

Three variables are used by the :term:`path based finder`, :data:`sys.path`,
:data:`sys.path_hooks` and :data:`sys.path_importer_cache`.  The ``__path__``
attributes on package objects are also used.  These provide additional ways
that the import machinery can be customized.

:data:`sys.path` contains a list of strings providing search locations for
modules and packages.  It is initialized from the :data:`PYTHONPATH`
environment variable and various other installation- and
implementation-specific defaults.  Entries in :data:`sys.path` can name
directories on the file system, zip files, and potentially other "locations"
(see the :mod:`site` module) that should be searched for modules, such as
URLs, or database queries.  Only strings and bytes should be present on
:data:`sys.path`; all other data types are ignored.  The encoding of bytes
entries is determined by the individual :term:`path entry finders <path entry
finder>`.

The :term:`path based finder` is a :term:`meta path finder`, so the import
machinery begins the :term:`import path` search by calling the path
based finder's :meth:`find_module()` method as described previously.  When
the ``path`` argument to :meth:`find_module()` is given, it will be a
list of string paths to traverse - typically a package's ``__path__``
attribute for an import within that package.  If the ``path`` argument
is ``None``, this indicates a top level import and :data:`sys.path` is used.

The path based finder iterates over every entry in the search path, and
for each of these, looks for an appropriate :term:`path entry finder` for the
path entry.  Because this can be an expensive operation (e.g. there may be
`stat()` call overheads for this search), the path based finder maintains
a cache mapping path entries to path entry finders.  This cache is maintained
in :data:`sys.path_importer_cache` (despite the name, this cache actually
stores finder objects rather than being limited to :term:`importer` objects).
In this way, the expensive search for a particular :term:`path entry`
location's :term:`path entry finder` need only be done once.  User code is
free to remove cache entries from :data:`sys.path_importer_cache` forcing
the path based finder to perform the path entry search again [#fnpic]_.

If the path entry is not present in the cache, the path based finder iterates
over every callable in :data:`sys.path_hooks`.  Each of the :term:`path entry
hooks <path entry hook>` in this list is called with a single argument, the
path entry to be searched.  This callable may either return a :term:`path
entry finder` that can handle the path entry, or it may raise
:exc:`ImportError`.  An :exc:`ImportError` is used by the path based finder to
signal that the hook cannot find a :term:`path entry finder` for that
:term:`path entry`.  The exception is ignored and :term:`import path`
iteration continues.  The hook should expect either a string or bytes object;
the encoding of bytes objects is up to the hook (e.g. it may be a file system
encoding, UTF-8, or something else), and if the hook cannot decode the
argument, it should raise :exc:`ImportError`.

If :data:`sys.path_hooks` iteration ends with no :term:`path entry finder`
being returned, then the path based finder's :meth:`find_module()` method
will store ``None`` in :data:`sys.path_importer_cache` (to indicate that
there is no finder for this path entry) and return ``None``, indicating that
this :term:`meta path finder` could not find the module.

If a :term:`path entry finder` *is* returned by one of the :term:`path entry
hook` callables on :data:`sys.path_hooks`, then the following protocol is used
to ask the finder for a module loader, which is then used to load the module.


Path entry finder protocol
--------------------------

In order to support imports of modules and initialized packages and also to
contribute portions to namespace packages, path entry finders must implement
the :meth:`find_loader()` method.

:meth:`find_loader()` takes one argument, the fully qualified name of the
module being imported.  :meth:`find_loader()` returns a 2-tuple where the
first item is the loader and the second item is a namespace :term:`portion`.
When the first item (i.e. the loader) is ``None``, this means that while the
path entry finder does not have a loader for the named module, it knows that the
path entry contributes to a namespace portion for the named module.  This will
almost always be the case where Python is asked to import a namespace package
that has no physical presence on the file system.  When a path entry finder
returns ``None`` for the loader, the second item of the 2-tuple return value
must be a sequence, although it can be empty.

If :meth:`find_loader()` returns a non-``None`` loader value, the portion is
ignored and the loader is returned from the path based finder, terminating
the search through the path entries.

For backwards compatibility with other implementations of the import
protocol, many path entry finders also support the same,
traditional :meth:`find_module()` method that meta path finders support.
However path entry finder :meth:`find_module()` methods are never called
with a ``path`` argument (they are expected to record the appropriate
path information from the initial call to the path hook).

The :meth:`find_module()` method on path entry finders is deprecated,
as it does not allow the path entry finder to contribute portions to
namespace packages. Instead path entry finders should implement the
:meth:`find_loader()` method as described above. If it exists on the path
entry finder, the import system will always call :meth:`find_loader()`
in preference to :meth:`find_module()`.


Replacing the standard import system
====================================

The most reliable mechanism for replacing the entire import system is to
delete the default contents of :data:`sys.meta_path`, replacing them
entirely with a custom meta path hook.

If it is acceptable to only alter the behaviour of import statements
without affecting other APIs that access the import system, then replacing
the builtin :func:`__import__` function may be sufficient. This technique
may also be employed at the module level to only alter the behaviour of
import statements within that module.

To selectively prevent import of some modules from a hook early on the
meta path (rather than disabling the standard import system entirely),
it is sufficient to raise :exc:`ImportError` directly from
:meth:`find_module` instead of returning ``None``. The latter indicates
that the meta path search should continue. while raising an exception
terminates it immediately.


Open issues
===========

XXX It would be really nice to have a diagram.

XXX * (import_machinery.rst) how about a section devoted just to the
attributes of modules and packages, perhaps expanding upon or supplanting the
related entries in the data model reference page?

XXX runpy, pkgutil, et al in the library manual should all get "See Also"
links at the top pointing to the new import system section.


References
==========

The import machinery has evolved considerably since Python's early days.  The
original `specification for packages
<http://www.python.org/doc/essays/packages.html>`_ is still available to read,
although some details have changed since the writing of that document.

The original specification for :data:`sys.meta_path` was :pep:`302`, with
subsequent extension in :pep:`420`.

:pep:`420` introduced :term:`namespace packages <namespace package>` for
Python 3.3.  :pep:`420` also introduced the :meth:`find_loader` protocol as an
alternative to :meth:`find_module`.

:pep:`366` describes the addition of the ``__package__`` attribute for
explicit relative imports in main modules.

:pep:`328` introduced absolute and explicit relative imports and initially
proposed ``__name__`` for semantics :pep:`366` would eventually specify for
``__package__``.

:pep:`338` defines executing modules as scripts.


.. rubric:: Footnotes

.. [#fnmo] See :class:`types.ModuleType`.

.. [#fnlo] The importlib implementation avoids using the return value
   directly. Instead, it gets the module object by looking the module name up
   in :data:`sys.modules`.  The indirect effect of this is that an imported
   module may replace itself in :data:`sys.modules`.  This is
   implementation-specific behavior that is not guaranteed to work in other
   Python implementations.

.. [#fnpic] In legacy code, it is possible to find instances of
   :class:`imp.NullImporter` in the :data:`sys.path_importer_cache`.  It
   is recommended that code be changed to use ``None`` instead.  See
   :ref:`portingpythoncode` for more details.
