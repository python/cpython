.. _copyreg-register-pickle-support-functions:

:mod:`!copyreg` --- Register :mod:`!pickle` and :mod:`!copy` support functions
==============================================================================

.. module:: copyreg
   :synopsis: Register pickle and copy support functions.

**Source code:** :source:`Lib/copyreg.py`

.. index::
   pair: module; pickle
   pair: module; copy

--------------

The :mod:`!copyreg` module offers a way to define functions
used while pickling and copying specific objects.
The :mod:`pickle` and :mod:`copy` modules use those functions
when pickling/copying those objects.
The module provides configuration information
about object constructors which are not classes.
Such constructors may be factory functions or class instances.


.. function:: constructor(object)

   Declares *object* to be a valid constructor.  If *object* is not callable (and
   hence not valid as a constructor), raises :exc:`TypeError`.


.. function:: pickle(type, function, constructor_ob=None)

   Declares that *function* should be used as a "reduction" function for objects
   of type *type*.  *function* must return either a string or a tuple
   containing between two and six elements. See the :attr:`~pickle.Pickler.dispatch_table`
   for more details on the interface of *function*.

   The *constructor_ob* parameter is a legacy feature and is now ignored, but if
   passed it must be a callable.

   Note that the :attr:`~pickle.Pickler.dispatch_table` attribute of a pickler
   object or subclass of :class:`pickle.Pickler` can also be used for
   declaring reduction functions.


.. function:: copy(type, function)

   Declares that *function* should be used as the shallow copy function
   for objects of type *type*.
   *function* is called with the object as its only argument
   and must return the copy,
   like the :meth:`~object.__copy__` method.
   Registration is by exact type:
   it does not apply to subclasses of *type*.

   :func:`copy.copy` uses the registered function
   in preference to the :meth:`~object.__copy__` method
   and the pickle interfaces.
   Unlike a reduction function registered with :func:`pickle`,
   it affects only shallow copying.
   The registered functions are stored in ``copy_dispatch_table``,
   the same table that holds the handlers for the built-in
   container types, so registering a function for a built-in
   container type overrides its default copying.

   .. versionadded:: next


.. function:: deepcopy(type, function)

   Like :func:`copy`, but registers the deep copy function
   used by :func:`copy.deepcopy`.
   *function* is called with the object and the memo dictionary
   as its two arguments,
   like the :meth:`~object.__deepcopy__` method.
   The registered functions are stored in ``deepcopy_dispatch_table``.

   .. versionadded:: next


Example
-------

The example below would like to show how to register a pickle function and how
it will be used:

   >>> import copyreg, copy, pickle
   >>> class C:
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
