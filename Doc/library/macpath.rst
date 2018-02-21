:mod:`macpath` --- Mac OS 9 path manipulation functions
=======================================================

.. module:: macpath
   :synopsis: Mac OS 9 path manipulation functions.

**Source code:** :source:`Lib/macpath.py`

.. deprecated-removed:: 3.7 3.8

--------------

This module is the Mac OS 9 (and earlier) implementation of the :mod:`os.path`
module. It can be used to manipulate old-style Macintosh pathnames on Mac OS X
(or any other platform).

The following functions are available in this module: :func:`normcase`,
:func:`normpath`, :func:`isabs`, :func:`join`, :func:`split`, :func:`isdir`,
:func:`isfile`, :func:`walk`, :func:`exists`. For other functions available in
:mod:`os.path` dummy counterparts are available.

