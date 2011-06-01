:mod:`packaging.install` --- Installation tools
===============================================

.. module:: packaging.install
   :synopsis: Download and installation building blocks


Packaging provides a set of tools to deal with downloads and installation of
distributions.  Their role is to download the distribution from indexes, resolve
the dependencies, and provide a safe way to install distributions.  An operation
that fails will cleanly roll back, not leave half-installed distributions on the
system.  Here's the basic process followed:

#. Move all distributions that will be removed to a temporary location.

#. Install all the distributions that will be installed in a temporary location.

#. If the installation fails, move the saved distributions back to their
   location and delete the installed distributions.

#. Otherwise, move the installed distributions to the right location and delete
   the temporary locations.

This is a higher-level module built on :mod:`packaging.database` and
:mod:`packaging.pypi`.


Public functions
----------------

.. function:: get_infos(requirements, index=None, installed=None, \
                        prefer_final=True)

   Return information about what's going to be installed and upgraded.
   *requirements* is a string string containing the requirements for this
   project, for example ``'FooBar 1.1'`` or ``'BarBaz (<1.2)'``.

   .. XXX are requirements comma-separated?

   If you want to use another index than the main PyPI, give its URI as *index*
   argument.

   *installed* is a list of already installed distributions used to find
   satisfied dependencies, obsoleted distributions and eventual conflicts.

   By default, alpha, beta and candidate versions are not picked up.  Set
   *prefer_final* to false to accept them too.

   The results are returned in a dictionary containing all the information
   needed to perform installation of the requirements with the
   :func:`install_from_infos` function:

   >>> get_install_info("FooBar (<=1.2)")
   {'install': [<FooBar 1.1>], 'remove': [], 'conflict': []}

   .. TODO should return tuple or named tuple, not dict
   .. TODO use "predicate" or "requirement" consistently in version and here
   .. FIXME "info" cannot be plural in English, s/infos/info/


.. function:: install(project)


.. function:: install_dists(dists, path, paths=None)

   Safely install all distributions provided in *dists* into *path*.  *paths* is
   a list of paths where already-installed distributions will be looked for to
   find satisfied dependencies and conflicts (default: :data:`sys.path`).
   Returns a list of installed dists.

   .. FIXME dists are instances of what?


.. function:: install_from_infos(install_path=None, install=[], remove=[], \
                                 conflicts=[], paths=None)

   Safely install and remove given distributions.  This function is designed to
   work with the return value of :func:`get_infos`: *install*, *remove* and
   *conflicts* should be list of distributions returned by :func:`get_infos`.
   If *install* is not empty, *install_path* must be given to specify the path
   where the distributions should be installed.  *paths* is a list of paths
   where already-installed distributions will be looked for (default:
   :data:`sys.path`).

   This function is a very basic installer; if *conflicts* is not empty, the
   system will be in a conflicting state after the function completes.  It is a
   building block for more sophisticated installers with conflict resolution
   systems.

   .. TODO document typical value for install_path
   .. TODO document integration with default schemes, esp. user site-packages


.. function:: install_local_project(path)

   Install a distribution from a source directory, which must contain either a
   Packaging-compliant :file:`setup.cfg` file or a legacy Distutils
   :file:`setup.py` script (in which case Distutils will be used under the hood
   to perform the installation).


.. function::  remove(project_name, paths=None, auto_confirm=True)

   Remove one distribution from the system.

   .. FIXME this is the only function using "project" instead of dist/release

..
   Example usage
   --------------

   Get the scheme of what's gonna be installed if we install "foobar":
