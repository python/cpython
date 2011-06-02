.. _packaging-index:

##############################
 Distributing Python Projects
##############################

:Authors: The Fellowship of The Packaging
:Email: distutils-sig@python.org
:Release: |version|
:Date: |today|

This document describes Packaging for Python authors, describing how to use the
module to make Python applications, packages or modules easily available to a
wider audience with very little overhead for build/release/install mechanics.

.. toctree::
   :maxdepth: 2
   :numbered:

   tutorial
   setupcfg
   introduction
   setupscript
   configfile
   sourcedist
   builtdist
   packageindex
   uploading
   examples
   extending
   commandhooks
   commandref


.. seealso::

   :ref:`packaging-install-index`
      A user-centered manual which includes information on adding projects
      into an existing Python installation.  You do not need to be a Python
      programmer to read this manual.

   :mod:`packaging`
      A library reference for developers of packaging tools wanting to use
      standalone building blocks like :mod:`~packaging.version` or
      :mod:`~packaging.metadata`, or extend Packaging itself.
