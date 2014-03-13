:mod:`distutils` --- Building and installing Python modules
===========================================================

.. module:: distutils
   :synopsis: Support for building and installing Python modules into an
              existing Python installation.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>


The :mod:`distutils` package provides support for building and installing
additional modules into a Python installation.  The new modules may be either
100%-pure Python, or may be extension modules written in C, or may be
collections of Python packages which include modules coded in both Python and C.

Most Python users will *not* want to use this module directly, but instead
use the cross-version tools maintained by the Python Packaging Authority.
Refer to the `Python Packaging User Guide <http://packaging.python.org>`_
for more information.

For the benefits of packaging tool authors and users seeking a deeper
understanding of the details of the current packaging and distribution
system, the legacy :mod:`distutils` based user documentation and API
reference remain available:

* :ref:`install-index`
* :ref:`distutils-index`
