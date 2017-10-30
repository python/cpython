:mod:`copyreg` --- Register :mod:`pickle` support functions
===========================================================

.. module:: copyreg
   :synopsis: Register pickle support functions.

**Source code:** :source:`Lib/copyreg.py`

.. index::
   module: pickle
   module: copy

--------------

The :mod:`copyreg` module offers a way to define functions used while pickling
specific objects.  The :mod:`pickle` and :mod:`copy` modules use those functions
when pickling/copying those objects.  The module provides configuration
information about object constructors which are not classes.
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

   See the :mod:`pickle` module for more details on the interface
   expected of *function* and *constructor*.  Note that the
   :attr:`~pickle.Pickler.dispatch_table` attribute of a pickler
   object or subclass of :class:`pickle.Pickler` can also be used for
   declaring reduction functions.

Example
-------

The example below would like to show how to register a pickle function and how
it will be used:

   >>> import copyreg, copy, pickle
   >>> class C(object):
   ...     def __init__(self, a):
   ...         self.a = a
   ...
   >>> def pickle_c(c):
   ...     print("pickling a C instance...")
   ...     return C, (c.a,)
   ...
   >>> copyreg.pickle(C, pickle_c)
   >>> c = C(1)
   >>> d = copy.copy(c)  # doctest: +SKIP
   pickling a C instance...
   >>> p = pickle.dumps(c)  # doctest: +SKIP
   pickling a C instance...
