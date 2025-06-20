.. highlight:: c

.. _building:

*****************************
Building C and C++ Extensions
*****************************

A C extension for CPython is a shared library (for example, a ``.so`` file on
Linux, ``.pyd`` on Windows), which exports an *initialization function*.

See :ref:`extension-modules` for details.


.. highlight:: c

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
