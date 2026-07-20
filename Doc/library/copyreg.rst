.. _copyreg-register-pickle-support-functions:

:mod:`!copyreg` --- Register :mod:`!pickle` and :mod:`!json` support functions
==============================================================================

.. module:: copyreg
   :synopsis: Register pickle and JSON support functions.

**Source code:** :source:`Lib/copyreg.py`

.. index::
   pair: module; pickle
   pair: module; copy
   pair: module; json

--------------

The :mod:`!copyreg` module offers a way to define functions
used while pickling and JSON-serializing specific objects.
The :mod:`pickle`, :mod:`copy` and :mod:`json` modules use those functions
when pickling/copying/serializing those objects.
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


.. function:: json(type, function)

   Declares that *function* should be used
   as the JSON serialization function for objects of type *type*.
   *function* must be callable;
   it is called with the object as its only argument
   and must return a substitute object to be serialized,
   with the same interface as the :meth:`~object.__json__` method.
   Registration is by exact type:
   it does not apply to subclasses of *type*.
   See :ref:`json-protocol` for details.

   .. versionadded:: next


.. data:: json_dispatch_table

   The mapping of types to serialization functions
   filled by :func:`json` and consulted by the :mod:`json` module.
   It can be overridden for a particular encoder
   with the :attr:`json.JSONEncoder.dispatch_table` attribute.

   .. versionadded:: next


.. class:: RawJSON(encoded_json)

   Wrapper for the already encoded JSON string *encoded_json*.
   The JSON encoder outputs it verbatim,
   without validation of its content.
   ``str()`` of the instance returns *encoded_json*.

   For example, it allows serializing :class:`decimal.Decimal`
   as a JSON number with full precision:

      >>> import copyreg, decimal, json
      >>> copyreg.json(decimal.Decimal, lambda d: copyreg.RawJSON(str(d)))
      >>> json.dumps({'price': decimal.Decimal('1.10')})
      '{"price": 1.10}'

   A registration can be undone by removing the entry from
   :data:`json_dispatch_table`:

      >>> del copyreg.json_dispatch_table[decimal.Decimal]

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
