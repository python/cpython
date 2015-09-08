.. highlightlang:: c

.. _cporting-howto:

*************************************
Porting Extension Modules to Python 3
*************************************

.. topic:: Abstract

   Although changing the C-API was not one of Python 3's objectives,
   the many Python-level changes made leaving Python 2's API intact
   impossible.  In fact, some changes such as :func:`int` and
   :func:`long` unification are more obvious on the C level.  This
   document used to document such incompatibilities and how they can
   be worked around, but was never particularly complete.
   Guides that are much more useful are maintained outside the Python
   documentation.

   This page now lists links to suggested external guides and projects.

We recommend the following resources for porting extension modules to Python 3:

* The `Migrating C extensions`_ chapter from
  *Supporting Python 3: An in-depth guide*, a book on moving from Python 2
  to Python 3 in general, guides the reader through porting an extension
  module.
* The `Porting guide`_ from the *py3c* project provides opinionated
  suggestions with supporting code.
* The `Cython`_ and `CFFI`_ libraries offer abstractions over
  Python's C API.
  Extensions generally need to be re-written to use one of them,
  but the library then handles differences between various Python
  versions and implementations.

.. _Migrating C extensions: http://python3porting.com/cextensions.html
.. _Porting guide: https://py3c.readthedocs.io/en/latest/guide.html
.. _Cython: http://cython.org/
.. _CFFI: https://cffi.readthedocs.io/en/latest/
