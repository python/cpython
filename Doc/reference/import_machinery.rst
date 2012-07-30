
.. _importmachinery:

****************
Import machinery
****************

.. index:: single: import machinery

Python code in one :term:`module` gains access to the code in another module
by the process of :term:`importing` it.  The :keyword:`import` statement is
the most common way of invoking the import machinery, but it is not the only
way.  Functions such as :func:`importlib.import_module` and built-in
:func:`__import__` can also be used to invoke the import machinery.

The :keyword:`import` statement combines two operations; it searches for the
named module, then it binds the results of that search to a name in the local
scope.  The search operation of the :keyword:`import` statement is defined as
a call to the built-in :func:`__import__` function, with the appropriate
arguments.  The return value of :func:`__import__` is used to perform the name
binding operation of the :keyword:`import` statement.  See the
:keyword:`import` statement for the exact details of that name binding
operation.

A direct call to :func:`__import__` performs only the module search and, if
found, the module creation operation.  While certain side-effects may occur,
such as the importing of parent packages, and the updating of various caches
(including :data:`sys.modules`), only the :keyword:`import` statement performs
a name binding operation.

When a module is first imported, Python searches for the module and if found,
it creates a module object [#fnmo]_, initializing it.  If the named module
cannot be found, an :exc:`ImportError` is raised.  Python implements various
strategies to search for the named module when the import machinery is
invoked.  These strategies can be modified and extended by using various hooks
described in the sections below.  The entire import machinery itself can be
overridden by replacing built-in :func:`__import__`.


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
module.  Specifically, any module that contains an ``__path__`` attribute is
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
``__init__.py`` file is implicitly imported, and the objects it defines are
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

Importing ``parent.one`` will implicitly import ``parent/__init__.py`` and
``parent/one/__init__.py``.  Subsequent imports of ``parent.two`` or
``parent.three`` will import ``parent/two/__init__.py`` and
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

:data:`sys.modules` is writable.  Deleting a key will not destroy the
associated module, but it will invalidate the cache entry for the named
module, causing Python to search anew for the named module upon its next
import.  Beware though, because if you keep a reference to the module object,
invalidate its cache entry in :data:`sys.modules`, and then re-import the
named module, the two module objects will *not* be the same.  The key can also
be assigned to ``None``, forcing the next import of the module to result in an
:exc:`ImportError`.


Finders and loaders
-------------------

.. index::
    single: finder
    single: loader

If the named module is not found in :data:`sys.modules` then Python's import
protocol is invoked to find and load the module.  As this implies, the import
protocol consists of two conceptual objects, :term:`finders <finder>` and
:term:`loaders <loader>`.  A finder's job is to determine whether it can find
the named module using whatever strategy it knows about.  For example, there
is a file system finder which know how to search the file system for the named
module.  Other finders may know how to search a zip file, a web page, or a
database to find the named module.  The import machinery is extensible, so new
finders can be added to extend the range and scope of module searching.

Finders do not actually load modules.  If they can find the named module, they
return a loader, which the import machinery later invokes to load the module
and create the corresponding module object.

There are actually two types of finders, and two different but related APIs
for finders, depending on whether it is a :term:`meta path finder` or a
:term:`sys path finder`.  Meta path processing occurs at the beginning of
import processing, while sys path processing happens later, by the :term:`path
importer`.

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
hooks* and *path hooks*.

Meta hooks are called at the start of import processing, before any other
import processing has occurred.  This allows meta hooks to override
:data:`sys.path` processing, frozen modules, or even built-in modules.  Meta
hooks are registered by adding new finder objects to :data:`sys.meta_path`, as
described below.

Path hooks are called as part of :data:`sys.path` (or ``package.__path__``)
processing, at the point where their associated path item is encountered.
Path hooks are registered by adding new callables to :data:`sys.path_hooks` as
described below.


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
:meth:`find_module()` which takes two arguments, a name and a path.  The meta
path finder can use any strategy it wants to determine whether it can handle
the named module or not.

If the meta path finder knows how to handle the named module, it returns a
loader object.  If it cannot handle the named module, it returns ``None``.  If
:data:`sys.meta_path` processing reaches the end of its list without returning
a loader, then an :exc:`ImportError` is raised.  Any other exceptions raised
are simply propagated up, aborting the import process.

The :meth:`find_module()` method of meta path finders is called with two
arguments.  The first is the fully qualified name of the module being
imported, for example ``foo.bar.baz``.  The second argument is the relative
path for the module search.  For top-level modules, the second argument is
``None``, but for submodules or subpackages, the second argument is the value
of the parent package's ``__path__`` attribute, which must exist or an
:exc:`ImportError` is raised.

Python's default :data:`sys.meta_path` has three meta path finders, one that
knows how to import built-in modules, one that knows how to import frozen
modules, and one that knows how to import modules from the file system
(i.e. the :term:`path importer`).


Meta path loaders
-----------------

Once a loader is found via a meta path finder, the loader's
:meth:`~importlib.abc.Loader.load_module` method is called, with a single
argument, the fully qualified name of the module being imported.  This method
has several responsibilities, and should return the module object it has
loaded [#fnlo]_.  If it cannot load the module, it should raise an
:exc:`ImportError`, although any other exception raised during
:meth:`load_module()` will be propagated.

In many cases, the meta path finder and loader can be the same object,
e.g. :meth:`finder.find_module()` would just return ``self``.

Loaders must satisfy the following requirements:

 * If there is an existing module object with the given name in
   :data:`sys.modules`, the loader must use that existing module.  (Otherwise,
   the :func:`imp.reload` will not work correctly.)  If the named module does
   not exist in :data:`sys.modules`, the loader must create a new module
   object and add it to :data:`sys.modules`.

   Note that the module *must* exist in :data:`sys.modules` before the loader
   executes the module code.  This is crucial because the module code may
   (directly or indirectly) import itself; adding it to :data:`sys.modules`
   beforehand prevents unbounded recursion in the worst case and multiple
   loading in the best.

   If the load fails, the loader needs to remove any modules it may have
   inserted into ``sys.modules``.  If the module was already in
   ``sys.modules`` then the loader should leave it alone.

 * The loader may set the ``__file__`` attribute of the module.  If set, this
   attribute's value must be a string.  The loader may opt to leave
   ``__file__`` unset if it has no semantic meaning (e.g. a module loaded from
   a database).

 * The loader may set the ``__name__`` attribute of the module.  While not
   required, setting this attribute is highly recommended so that the
   :meth:`repr()` of the module is more informative.

 * If module is a package (either regular or namespace), the loader must set
   the module object's ``__path__`` attribute.  The value must be a list, but
   may be empty if ``__path__`` has no further significance to the importer.
   More details on the semantics of ``__path__`` are given below.

 * The ``__loader__`` attribute must be set to the loader object that loaded
   the module.  This is mostly for introspection and reloading, but can be
   used for additional importer-specific functionality, for example getting
   data associated with an importer.

 * The module's ``__package__`` attribute should be set.  Its value must be a
   string, but it can be the same value as its ``__name__``.  This is the
   recommendation when the module is a package.  When the module is not a
   package, ``__package__`` should be set to the parent package's
   name [#fnpk]_.

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

 * If the module has an ``__loader__`` and that loader has a
   :meth:`module_repr()` method, call it with a single argument, which is the
   module object.  The value returned is used as the module's repr.

 * If an exception occurs in :meth:`module_repr()`, the exception is caught
   and discarded, and the calculation of the module's repr continues as if
   :meth:`module_repr()` did not exist.

 * If the module has an ``__file__`` attribute, this is used as part of the
   module's repr.

 * If the module has no ``__file__`` but does have an ``__loader__``, then the
   loader's repr is used as part of the module's repr.

 * Otherwise, just use the module's ``__name__`` in the repr.

This example, from :pep:`420` shows how a loader can craft its own module
repr::

    class NamespaceLoader:
        @classmethod
        def module_repr(cls, module):
            return "<module '{}' (namespace)>".format(module.__name__)


module.__path__
---------------

By definition, if a module has an ``__path__`` attribute, it is a package,
regardless of its value.

A package's ``__path__`` attribute is used during imports of its subpackages.
Within the import machinery, it functions much the same as :data:`sys.path`,
i.e. providing a list of locations to search for modules during import.
However, ``__path__`` is typically much more constrained than
:data:`sys.path`.

``__path__`` must be a list, but it may be empty.  The same rules used for
:data:`sys.path` also apply to a package's ``__path__``, and
:data:`sys.path_hooks` (described below) is consulted when traversing a
package's ``__path__``.

A package's ``__init__.py`` file may set or alter the package's ``__path__``
attribute, and this was typically the way namespace packages were implemented
prior to :pep:`420`.  With the adoption of :pep:`420`, namespace packages no
longer need to supply ``__init__.py`` files containing only ``__path__``
manipulation code; the namespace loader automatically sets ``__path__``
correctly for the namespace package.


The Path Importer
=================

.. index::
    single: path importer

As mentioned previously, Python comes with several default meta path finders.
One of these, called the :term:`path importer`, knows how to provide
traditional file system imports.  It implements all the semantics for finding
modules on the file system, handling special file types such as Python source
code (``.py`` files), Python byte code (``.pyc`` and ``.pyo`` files) and
shared libraries (e.g. ``.so`` files).

In addition to being able to find such modules, there is built-in support for
loading these modules.  To accomplish these two related tasks, additional
hooks and protocols are provided so that you can extend and customize the path
importer semantics.

A word of warning: this section and the previous both use the term *finder*,
distinguishing between them by using the terms :term:`meta path finder` and
:term:`sys path finder`.  Meta path finders and sys path finders are very
similar, support similar protocols, and function in similar ways during the
import process, but it's important to keep in mind that they are subtly
different.  In particular, meta path finders operate at the beginning of the
import process, as keyed off the :data:`sys.meta_path` traversal.

On the other hand, sys path finders are in a sense an implementation detail of
the path importer, and in fact, if the path importer were to be removed from
:data:`sys.meta_path`, none of the sys path finder semantics would be invoked.


sys path finders
----------------

.. index::
    single: sys.path
    single: sys.path_hooks
    single: sys.path_importer_cache
    single: PYTHONPATH

The path importer is responsible for finding and loading Python modules and
packages from the file system.  As a meta path finder, it implements the
:meth:`find_module()` protocol previously described, however it exposes
additional hooks that can be used to customize how modules are found and
loaded from the file system.

Three variables are used during file system import, :data:`sys.path`,
:data:`sys.path_hooks` and :data:`sys.path_importer_cache`.  These provide
additional ways that the import machinery can be customized, in this case
specifically during file system path import.

:data:`sys.path` contains a list of strings providing search locations for
modules and packages.  It is initialized from the :data:`PYTHONPATH`
environment variable and various other installation- and
implementation-specific defaults.  Entries in :data:`sys.path` can name
directories on the file system, zip files, and potentially other "locations"
(see the :mod:`site` module) that should be searched for modules.

The path importer is a meta path finder, so the import machinery begins file
system search by calling the path importer's :meth:`find_module()` method as
described previously.  When the ``path`` argument to :meth:`find_module()` is
given, it will be a list of string paths to traverse.  If not,
:data:`sys.path` is used.

The path importer iterates over every entry in the search path, and for each
of these, searches for an appropriate sys path finder for the path entry.
Because this can be an expensive operation (e.g. there are `stat()` call
overheads for this search), the path importer maintains a cache mapping path
entries to sys path finders.  This cache is maintained in
:data:`sys.path_importer_cache`.  In this way, the expensive search for a
particular path location's sys path finder need only be done once.  User code
is free to remove cache entries from :data:`sys.path_importer_cache` forcing
the path importer to perform the path search again [#fnpic]_.

If the path entry is not present in the cache, the path importer iterates over
every callable in :data:`sys.path_hooks`.  Each entry in this list is called
with a single argument, the path entry being searched.  This callable may
either return a sys path finder that can handle the path entry, or it may
raise :exc:`ImportError`.  An :exc:`ImportError` is used by the path importer
to signal that the hook cannot find a sys path finder for that path entry.
The exception is ignored and :data:`sys.path_hooks` iteration continues.

If :data:`sys.path_hooks` iteration ends with no sys path finder being
returned then the path importer's :meth:`find_module()` method will return
``None`` and an :exc:`ImportError` will be raised.

If a sys path finder *is* returned by one of the callables on
:data:`sys.path_hooks`, then the following protocol is used to ask the sys
path finder for a module loader, which is then used to load the module as
previously described (i.e. its :meth:`load_module()` method is called).


sys path finder protocol
------------------------

sys path finders support the same, traditional :meth:`find_module()` method
that meta path finders support, however sys path finder :meth:`find_module()`
methods are never called with a ``path`` argument.

The :meth:`find_module()` method on sys path finders is deprecated though, and
instead sys path finders should implement the :meth:`find_loader()` method.
If it exists on the sys path finder, :meth:`find_loader()` will always be
called instead of :meth:`find_module()`.

:meth:`find_loader()` takes one argument, the fully qualified name of the
module being imported.  :meth:`find_loader()` returns a 2-tuple where the
first item is the loader and the second item is a namespace :term:`portion`.
When the first item (i.e. the loader) is ``None``, this means that while the
sys path finder does not have a loader for the named module, it knows that the
path entry contributes to a namespace portion for the named module.  This will
almost always be the case where Python is asked to import a namespace package
that has no physical presence on the file system.  When a sys path finder
returns ``None`` for the loader, the second item of the 2-tuple return value
must be a sequence, although it can be empty.

If :meth:`find_loader()` returns a non-``None`` loader value, the portion is
ignored and the loader is returned from the path importer, terminating the
:data:`sys.path` search.


Open issues
===========

XXX Find a better term than "path importer" for class PathFinder and update
the glossary.

XXX In the glossary, "though I'd change ":term:`finder` / :term:`loader`" to
"metapath importer".

XXX Find a better term than "sys path finder".

XXX It would be really nice to have a diagram.

XXX * (import_machinery.rst) how about a section devoted just to the
attributes of modules and packages, perhaps expanding upon or supplanting the
related entries in the data model reference page?

XXX * (import_machinery.rst) Meta path loaders, end of paragraph 2: "The
finder could also be a classmethod that returns an instance of the class."

XXX * (import_machinery.rst) Meta path loaders: "If the load fails, the loader
needs to remove any modules..." is a pretty exceptional case, since the
modules is not in charge of its parent or children, nor of import statements
executed for it.  Is this a new requirement?

XXX Module reprs: how does module.__qualname__ fit in?


References
==========

The import machinery has evolved considerably since Python's early days.  The
original `specification for packages
<http://www.python.org/doc/essays/packages.html>`_ is still available to read,
although some details have changed since the writing of that document.

The original specification for :data:`sys.meta_path` was :pep:`302`, with
subsequent extension in :pep:`420`, which also introduced namespace packages
without ``__init__.py`` files in Python 3.3.  :pep:`420` also introduced the
:meth:`find_loader` protocol as an alternative to :meth:`find_module`.

:pep:`366` describes the addition of the ``__package__`` attribute for
explicit relative imports in main modules.


Footnotes
=========

.. [#fnmo] See :class:`types.ModuleType`.

.. [#fnlo] The importlib implementation appears not to use the return value
   directly. Instead, it gets the module object by looking the module name up
   in :data:`sys.modules`.)  The indirect effect of this is that an imported
   module may replace itself in :data:`sys.modules`.  This is
   implementation-specific behavior that is not guaranteed to work in other
   Python implementations.

.. [#fnpk] In practice, within CPython there is little consistency in the
   values of ``__package__`` for top-level modules.  In some, such as in the
   :mod:`email` package, both the ``__name__`` and ``__package__`` are set to
   "email".  In other top-level modules (non-packages), ``__package__`` may be
   set to ``None`` or the empty string.  The recommendation for top-level
   non-package modules is to set ``__package__`` to the empty string.

.. [#fnpic] In legacy code, it is possible to find instances of
   :class:`imp.NullImporter` in the :data:`sys.path_importer_cache`.  It
   recommended that code be changed to use ``None`` instead.  See
   :ref:`portingpythoncode` for more details.
