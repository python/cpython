:mod:`!copyreg` --- Register :mod:`!pickle` support functions
=============================================================

.. module:: copyreg
   :synopsis: Register pickle support functions.

**Source code:** :source:`Lib/copyreg.py`

.. index::
   pair: module; pickle
   pair: module; copy

--------------

The :mod:`!copyreg` module offers a way to define functions used while pickling
specific objects.  The :mod:`pickle` and :mod:`copy` modules use those functions
when pickling/copying those objects.  The module provides configuration
information about object constructors which are not classes.
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


.. class:: Proxy(obj)

   A wrapper for an object which is pickled weakly.
   Reduction functions can use it to pickle objects
   to which the reconstructed object only holds weak references.
   For example, a weak reference is reduced to
   ``(weakref.ref, (Proxy(referent),))``.

   A proxy is pickled as the wrapped object *obj*,
   which is unpickled in its place.
   *obj* must also be strongly referenced elsewhere in the same pickle;
   otherwise pickling raises :exc:`pickle.PicklingError`.

   :func:`copy.copy` of a proxy returns *obj*,
   and :func:`copy.deepcopy` returns the deep copy of *obj*.

   .. attribute:: obj

      The wrapped object.

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
