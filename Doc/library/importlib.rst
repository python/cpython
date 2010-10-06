:mod:`importlib` -- Convenience wrappers for :func:`__import__`
===============================================================

.. module:: importlib
   :synopsis: Convenience wrappers for __import__

.. moduleauthor:: Brett Cannon <brett@python.org>
.. sectionauthor:: Brett Cannon <brett@python.org>

.. versionadded:: 2.7

This module is a minor subset of what is available in the more full-featured
package of the same name from Python 3.1 that provides a complete
implementation of :keyword:`import`. What is here has been provided to
help ease in transitioning from 2.7 to 3.1.


.. function:: import_module(name, package=None)

    Import a module. The *name* argument specifies what module to
    import in absolute or relative terms
    (e.g. either ``pkg.mod`` or ``..mod``). If the name is
    specified in relative terms, then the *package* argument must be
    specified to the package which is to act as the anchor for resolving the
    package name (e.g. ``import_module('..mod', 'pkg.subpkg')`` will import
    ``pkg.mod``).  The specified module will be inserted into
    :data:`sys.modules` and returned.
