:mod:`!copy` --- Shallow and deep copy operations
=================================================

.. module:: copy
   :synopsis: Shallow and deep copy operations.

**Source code:** :source:`Lib/copy.py`

--------------

Assignment statements in Python do not copy objects, they create bindings
between a target and an object. For collections that are mutable or contain
mutable items, a copy is sometimes needed so one can change one copy without
changing the other. This module provides generic shallow and deep copy
operations (explained below), plus a replace (copy-with-modifications)
operation that's implemented for certain immutable types.


``copy`` and ``deepcopy``
-------------------------

.. function:: copy(obj)

   Return a shallow copy of *obj*.


.. function:: deepcopy(obj[, memo])

   Return a deep copy of *obj*.


.. _shallow_vs_deep_copy:

.. tip:: Shallow vs. Deep Copy

   The difference between shallow and deep copying is only relevant for compound
   objects (objects that contain other objects, like lists or class instances):

   * A *shallow copy* constructs a new compound object and then (to the extent
     possible) inserts *references* into it to the objects found in the original.

   * A *deep copy* constructs a new compound object and then, recursively,
     inserts *copies* into it of the objects found in the original.

Two problems often exist with deep copy operations that don't exist with shallow
copy operations:

* Recursive objects (compound objects that, directly or indirectly, contain a
  reference to themselves) may cause a recursive loop.

* Because deep copy copies everything it may copy too much, such as data
  which is intended to be shared between copies.

The :func:`deepcopy` function avoids these problems by:

* keeping a ``memo`` dictionary of objects already copied during the current
  copying pass; and

* letting user-defined classes override the copying operation or the set of
  components copied.

This module does not copy types like module, method, stack trace, stack frame,
file, socket, window, or any similar types.  It does "copy" functions and
classes (shallow and deeply), by returning the original object unchanged; this
is compatible with the way these are treated by the :mod:`pickle` module.

Shallow copies of dictionaries can be made using :meth:`dict.copy`, and
of lists by assigning a slice of the entire list, for example,
``copied_list = original_list[:]``.


Pickling
~~~~~~~~

.. index:: pair: module; pickle

Classes can use the same interfaces to control copying that they use to control
pickling.  See the description of module :mod:`pickle` for information on these
methods.  In fact, the :mod:`copy` module uses the registered
pickle functions from the :mod:`copyreg` module.

.. seealso::

   Module :mod:`pickle`
      Discussion of the special methods used to support object state retrieval and
      restoration.


The copy protocol
~~~~~~~~~~~~~~~~~

.. index::
   single: __copy__() (copy protocol)
   single: __deepcopy__() (copy protocol)

.. currentmodule:: None

In order for a class to define its own copy implementation, it can define
special methods :meth:`~object.__copy__` and :meth:`~object.__deepcopy__`.

.. method:: object.__copy__(self)
   :noindexentry:

   Called to implement the shallow copy operation;
   no additional arguments are passed.

.. method:: object.__deepcopy__(self, memo)
   :noindexentry:

   Called to implement the deep copy operation; it is passed one
   argument, the *memo* dictionary.  If the ``__deepcopy__`` implementation needs
   to make a deep copy of a component, it should call the :func:`!copy.deepcopy` function
   with the component as first argument and the *memo* dictionary as second argument.
   The *memo* dictionary should be treated as an opaque object.


``replace``
-----------

.. function:: copy.replace(obj, /, **changes)

   Return a new object of the same type as *obj*, replacing fields with values
   from *changes*.

   Function :func:`!copy.replace` is more limited
   than :func:`~copy.copy` and :func:`~copy.deepcopy`,
   and only supports named tuples created by :func:`~collections.namedtuple`,
   :mod:`dataclasses`, and other classes which implement the replace protocol
   (i.e. define a method :meth:`~object.__replace__`).

   .. versionadded:: 3.13


The replace protocol
~~~~~~~~~~~~~~~~~~~~

.. index::
   single: __replace__() (replace protocol)

In order for a class to define its own replace implementation, it can define
the special method :meth:`~object.__replace__`.


.. method:: object.__replace__(self, /, **changes)
   :noindexentry:

   Called to implement the replace operation; it is passed a set of changes
   as keyword arguments. This method should create a new object of the same type,
   replacing fields with values from *changes*.

   .. versionadded:: 3.13


Exceptions
----------

.. exception:: copy.Error

   Raised for module specific errors.
