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


User documentation and API reference are provided in another document:

.. seealso::

   :ref:`distutils-index`
      The manual for developers and packagers of Python modules.  This describes
      how to prepare :mod:`distutils`\ -based packages so that they may be
      easily installed into an existing Python installation.  If also contains
      instructions for end-users wanting to install a distutils-based package,
      :ref:`install-index`.
