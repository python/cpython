:mod:`copy_reg` --- Register :mod:`pickle` support functions
============================================================

.. module:: copy_reg
   :synopsis: Register pickle support functions.

.. note::
   The :mod:`copy_reg` module has been renamed to :mod:`copyreg` in Python 3.
   The :term:`2to3` tool will automatically adapt imports when converting your
   sources to Python 3.

.. index::
   module: pickle
   module: cPickle
   module: copy

The :mod:`copy_reg` module offers a way to define fuctions used while pickling
specific objects.  The :mod:`pickle`, :mod:`cPickle`, and :mod:`copy` modules
use those functions when pickling/copying those objects.  The module provides
configuration information about object constructors which are not classes.
Such constructors may be factory functions or class instances.


.. function:: constructor(object)

   Declares *object* to be a valid constructor.  If *object* is not callable (and
   hence not valid as a constructor), raises :exc:`TypeError`.


.. function:: pickle(type, function[, constructor])

   Declares that *function* should be used as a "reduction" function for objects of
   type *type*; *type* must not be a "classic" class object.  (Classic classes are
   handled differently; see the documentation for the :mod:`pickle` module for
   details.)  *function* should return either a string or a tuple containing two or
   three elements.

   The optional *constructor* parameter, if provided, is a callable object which
   can be used to reconstruct the object when called with the tuple of arguments
   returned by *function* at pickling time.  :exc:`TypeError` will be raised if
   *object* is a class or *constructor* is not callable.

   See the :mod:`pickle` module for more details on the interface expected of
   *function* and *constructor*.

Example
-------

The example below would like to show how to register a pickle function and how
it will be used:

   >>> import copy_reg, copy, pickle
   >>> class C(object):
   ...     def __init__(self, a):
   ...         self.a = a
   ...
   >>> def pickle_c(c):
   ...     print("pickling a C instance...")
   ...     return C, (c.a,)
   ...
   >>> copy_reg.pickle(C, pickle_c)
   >>> c = C(1)
   >>> d = copy.copy(c)
   pickling a C instance...
   >>> p = pickle.dumps(c)
   pickling a C instance...
