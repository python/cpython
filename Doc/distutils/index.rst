.. _distutils-index:

##############################################
  Distributing Python Modules (Legacy version)
##############################################

:Authors: Greg Ward, Anthony Baxter
:Email: distutils-sig@python.org

.. seealso::

   :ref:`distributing-index`
      The up to date module distribution documentations

.. include:: ./_setuptools_disclaimer.rst

.. note::

   This guide only covers the basic tools for building and distributing
   extensions that are provided as part of this version of Python. Third party
   tools offer easier to use and more secure alternatives. Refer to the `quick
   recommendations section <https://packaging.python.org/guides/tool-recommendations/>`__
   in the Python Packaging User Guide for more information.

This document describes the Python Distribution Utilities ("Distutils") from
the module developer's point of view, describing the underlying capabilities
that ``setuptools`` builds on to allow Python developers to make Python modules
and extensions readily available to a wider audience.

.. toctree::
   :maxdepth: 2
   :numbered:

   introduction.rst
   setupscript.rst
   configfile.rst
   sourcedist.rst
   builtdist.rst
   examples.rst
   extending.rst
   commandref.rst
   apiref.rst
