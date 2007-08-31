
:mod:`fpformat` --- Floating point conversions
==============================================

.. module:: fpformat
   :synopsis: General floating point formatting functions.
.. sectionauthor:: Moshe Zadka <moshez@zadka.site.co.il>


The :mod:`fpformat` module defines functions for dealing with floating point
numbers representations in 100% pure Python.

.. note::

   This module is unnecessary: everything here can be done using the string
   formatting functions described in the :ref:`string-formatting` section.

The :mod:`fpformat` module defines the following functions and an exception:


.. function:: fix(x, digs)

   Format *x* as ``[-]ddd.ddd`` with *digs* digits after the point and at least one
   digit before. If ``digs <= 0``, the decimal point is suppressed.

   *x* can be either a number or a string that looks like one. *digs* is an
   integer.

   Return value is a string.


.. function:: sci(x, digs)

   Format *x* as ``[-]d.dddE[+-]ddd`` with *digs* digits after the  point and
   exactly one digit before. If ``digs <= 0``, one digit is kept and the point is
   suppressed.

   *x* can be either a real number, or a string that looks like one. *digs* is an
   integer.

   Return value is a string.


.. exception:: NotANumber

   Exception raised when a string passed to :func:`fix` or :func:`sci` as the *x*
   parameter does not look like a number. This is a subclass of :exc:`ValueError`
   when the standard exceptions are strings.  The exception value is the improperly
   formatted string that caused the exception to be raised.

Example::

   >>> import fpformat
   >>> fpformat.fix(1.23, 1)
   '1.2'

