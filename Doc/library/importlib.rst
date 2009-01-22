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
read than one in a programming language other than Python.

Two, the components to implement :keyword:`import` can be exposed in this
package, making it easier for users to create their own custom objects (known
generically as importers) to participate in the import process. Details on
providing custom importers can be found in :pep:`302`.

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

.. function:: __import__(name, globals={}, locals={}, fromlist=\[\], level=0)

    An implementation of the built-in :func:`__import__` function. See the
    built-in function's documentation for usage instructions.

.. function:: import_module(name, package=None)

    Import a module. The *name* argument specifies what module to
    import in absolute or relative terms
    (e.g. either ``pkg.mod`` or ``..mod``). If the name is
    specified in relative terms, then the *package* argument must be
    specified to the package which is to act as the anchor for resolving the
    package name (e.g. ``import_module('..mod', 'pkg.subpkg')`` will import
    ``pkg.mod``). The specified module will be inserted into
    :data:`sys.modules` and returned.
