:mod:`copyreg` --- Register :mod:`pickle` support functions
===========================================================

.. module:: copyreg
   :synopsis: Register pickle support functions.


.. index::
   module: pickle
   module: copy

The :mod:`copyreg` module provides support for the :mod:`pickle` module.  The
:mod:`copy` module is likely to use this in the future as well.  It provides
configuration information about object constructors which are not classes.
Such constructors may be factory functions or class instances.


.. function:: constructor(object)

   Declares *object* to be a valid constructor.  If *object* is not callable (and
   hence not valid as a constructor), raises :exc:`TypeError`.


.. function:: pickle(type, function, constructor=None)

   Declares that *function* should be used as a "reduction" function for objects
   of type *type*.  *function* should return either a string or a tuple
   containing two or three elements.

   The optional *constructor* parameter, if provided, is a callable object which
   can be used to reconstruct the object when called with the tuple of arguments
   returned by *function* at pickling time.  :exc:`TypeError` will be raised if
   *object* is a class or *constructor* is not callable.

   See the :mod:`pickle` module for more details on the interface expected of
   *function* and *constructor*.

