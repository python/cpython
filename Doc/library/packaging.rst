:mod:`packaging` --- Packaging support
======================================

.. module:: packaging
   :synopsis: Packaging system and building blocks for other packaging systems.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>, distutils and packaging
                   contributors


The :mod:`packaging` package provides support for building, packaging,
distributing and installing additional projects into a Python installation.
Projects may include Python modules, extension modules, packages and scripts.
:mod:`packaging` also provides building blocks for other packaging systems
that are not tied to the command system.

This manual is the reference documentation for those standalone building
blocks and for extending Packaging. If you're looking for the user-centric
guides to install a project or package your own code, head to `See also`__.


Building blocks
---------------

.. toctree::
   :maxdepth: 2
   :numbered:

   packaging-misc
   packaging.version
   packaging.metadata
   packaging.database
   packaging.depgraph
   packaging.pypi
   packaging.pypi.dist
   packaging.pypi.simple
   packaging.pypi.xmlrpc
   packaging.install


The command machinery
---------------------

.. toctree::
   :maxdepth: 2
   :numbered:

   packaging.dist
   packaging.command
   packaging.compiler
   packaging.fancy_getopt


Other utilities
----------------

.. toctree::
   :maxdepth: 2
   :numbered:

   packaging.util
   packaging.tests.pypi_server

.. XXX missing: compat config create (dir_util) run pypi.{base,mirrors}


.. __:

.. seealso::

   :ref:`packaging-index`
      The manual for developers of Python projects who want to package and
      distribute them. This describes how to use :mod:`packaging` to make
      projects easily found and added to an existing Python installation.

   :ref:`packaging-install-index`
      A user-centered manual which includes information on adding projects
      into an existing Python installation.  You do not need to be a Python
      programmer to read this manual.
