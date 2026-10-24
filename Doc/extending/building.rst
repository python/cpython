..
   The guides to building C and C++ extensions are fairly out of date
   and would benefit from re-writing in terms of a simple primer,
   with specific reference to sysconfig, and how a modules can be build
   'by hand', rather than via setuptools, CMake, meson, etc.
   If you're reading this and would like to have a go, please do!
   See https://github.com/python/cpython/issues/108064 for discussion.

.. highlight:: c

.. _building:

*****************************
Building C and C++ Extensions
*****************************

A C extension for CPython is a shared library (for example, a ``.so`` file on
Linux, ``.pyd`` on Windows), which exports an *initialization function*.

See :ref:`extension-modules` for details.


.. highlight:: c

.. _building-on-windows:
.. _install-index:
.. _setuptools-index:

Building C and C++ Extensions with setuptools
=============================================


Building, packaging and distributing extension modules is best done with
third-party tools, and is out of scope of this document.
One suitable tool is Setuptools, whose documentation can be found at
https://setuptools.pypa.io/en/latest/setuptools.html.

The :mod:`distutils` module, which was included in the standard library
until Python 3.12, is now maintained as part of Setuptools.


.. c:macro:: Py_NO_LINK_LIB

   Turn off the implicit, ``#pragma``-based linkage with the Python
   library, performed inside CPython header files.

   .. versionadded:: 3.14
