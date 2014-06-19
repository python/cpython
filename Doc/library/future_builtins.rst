:mod:`future_builtins` --- Python 3 builtins
============================================

.. module:: future_builtins
.. sectionauthor:: Georg Brandl
.. versionadded:: 2.6

This module provides functions that exist in 2.x, but have different behavior in
Python 3, so they cannot be put into the 2.x builtins namespace.

Instead, if you want to write code compatible with Python 3 builtins, import
them from this module, like this::

   from future_builtins import map, filter

   ... code using Python 3-style map and filter ...

The :term:`2to3` tool that ports Python 2 code to Python 3 will recognize
this usage and leave the new builtins alone.

.. note::

   The Python 3 :func:`print` function is already in the builtins, but cannot be
   accessed from Python 2 code unless you use the appropriate future statement::

      from __future__ import print_function


Available builtins are:

.. function:: ascii(object)

   Returns the same as :func:`repr`.  In Python 3, :func:`repr` will return
   printable Unicode characters unescaped, while :func:`ascii` will always
   backslash-escape them.  Using :func:`future_builtins.ascii` instead of
   :func:`repr` in 2.6 code makes it clear that you need a pure ASCII return
   value.

.. function:: filter(function, iterable)

   Works like :func:`itertools.ifilter`.

.. function:: hex(object)

   Works like the built-in :func:`hex`, but instead of :meth:`__hex__` it will
   use the :meth:`__index__` method on its argument to get an integer that is
   then converted to hexadecimal.

.. function:: map(function, iterable, ...)

   Works like :func:`itertools.imap`.

   .. note::

      In Python 3, :func:`map` does not accept ``None`` for the
      function argument.

.. function:: oct(object)

   Works like the built-in :func:`oct`, but instead of :meth:`__oct__` it will
   use the :meth:`__index__` method on its argument to get an integer that is
   then converted to octal.

.. function:: zip(*iterables)

   Works like :func:`itertools.izip`.
