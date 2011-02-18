
:mod:`sets` --- Unordered collections of unique elements
========================================================

.. module:: sets
   :synopsis: Implementation of sets of unique elements.
   :deprecated:
.. moduleauthor:: Greg V. Wilson <gvwilson@nevex.com>
.. moduleauthor:: Alex Martelli <aleax@aleax.it>
.. moduleauthor:: Guido van Rossum <guido@python.org>
.. sectionauthor:: Raymond D. Hettinger <python@rcn.com>


.. versionadded:: 2.3

.. deprecated:: 2.6
   The built-in :class:`set`/:class:`frozenset` types replace this module.

The :mod:`sets` module provides classes for constructing and manipulating
unordered collections of unique elements.  Common uses include membership
testing, removing duplicates from a sequence, and computing standard math
operations on sets such as intersection, union, difference, and symmetric
difference.

Like other collections, sets support ``x in set``, ``len(set)``, and ``for x in
set``.  Being an unordered collection, sets do not record element position or
order of insertion.  Accordingly, sets do not support indexing, slicing, or
other sequence-like behavior.

Most set applications use the :class:`Set` class which provides every set method
except for :meth:`__hash__`. For advanced applications requiring a hash method,
the :class:`ImmutableSet` class adds a :meth:`__hash__` method but omits methods
which alter the contents of the set. Both :class:`Set` and :class:`ImmutableSet`
derive from :class:`BaseSet`, an abstract class useful for determining whether
something is a set: ``isinstance(obj, BaseSet)``.

The set classes are implemented using dictionaries.  Accordingly, the
requirements for set elements are the same as those for dictionary keys; namely,
that the element defines both :meth:`__eq__` and :meth:`__hash__`. As a result,
sets cannot contain mutable elements such as lists or dictionaries. However,
they can contain immutable collections such as tuples or instances of
:class:`ImmutableSet`.  For convenience in implementing sets of sets, inner sets
are automatically converted to immutable form, for example,
``Set([Set(['dog'])])`` is transformed to ``Set([ImmutableSet(['dog'])])``.


.. class:: Set([iterable])

   Constructs a new empty :class:`Set` object.  If the optional *iterable*
   parameter is supplied, updates the set with elements obtained from iteration.
   All of the elements in *iterable* should be immutable or be transformable to an
   immutable using the protocol described in section :ref:`immutable-transforms`.


.. class:: ImmutableSet([iterable])

   Constructs a new empty :class:`ImmutableSet` object.  If the optional *iterable*
   parameter is supplied, updates the set with elements obtained from iteration.
   All of the elements in *iterable* should be immutable or be transformable to an
   immutable using the protocol described in section :ref:`immutable-transforms`.

   Because :class:`ImmutableSet` objects provide a :meth:`__hash__` method, they
   can be used as set elements or as dictionary keys.  :class:`ImmutableSet`
   objects do not have methods for adding or removing elements, so all of the
   elements must be known when the constructor is called.


.. _set-objects:

Set Objects
-----------

Instances of :class:`Set` and :class:`ImmutableSet` both provide the following
operations:

+-------------------------------+------------+---------------------------------+
| Operation                     | Equivalent | Result                          |
+===============================+============+=================================+
| ``len(s)``                    |            | cardinality of set *s*          |
+-------------------------------+------------+---------------------------------+
| ``x in s``                    |            | test *x* for membership in *s*  |
+-------------------------------+------------+---------------------------------+
| ``x not in s``                |            | test *x* for non-membership in  |
|                               |            | *s*                             |
+-------------------------------+------------+---------------------------------+
| ``s.issubset(t)``             | ``s <= t`` | test whether every element in   |
|                               |            | *s* is in *t*                   |
+-------------------------------+------------+---------------------------------+
| ``s.issuperset(t)``           | ``s >= t`` | test whether every element in   |
|                               |            | *t* is in *s*                   |
+-------------------------------+------------+---------------------------------+
| ``s.union(t)``                | ``s | t``  | new set with elements from both |
|                               |            | *s* and *t*                     |
+-------------------------------+------------+---------------------------------+
| ``s.intersection(t)``         | ``s & t``  | new set with elements common to |
|                               |            | *s* and *t*                     |
+-------------------------------+------------+---------------------------------+
| ``s.difference(t)``           | ``s - t``  | new set with elements in *s*    |
|                               |            | but not in *t*                  |
+-------------------------------+------------+---------------------------------+
| ``s.symmetric_difference(t)`` | ``s ^ t``  | new set with elements in either |
|                               |            | *s* or *t* but not both         |
+-------------------------------+------------+---------------------------------+
| ``s.copy()``                  |            | new set with a shallow copy of  |
|                               |            | *s*                             |
+-------------------------------+------------+---------------------------------+

Note, the non-operator versions of :meth:`union`, :meth:`intersection`,
:meth:`difference`, and :meth:`symmetric_difference` will accept any iterable as
an argument. In contrast, their operator based counterparts require their
arguments to be sets.  This precludes error-prone constructions like
``Set('abc') & 'cbs'`` in favor of the more readable
``Set('abc').intersection('cbs')``.

.. versionchanged:: 2.3.1
   Formerly all arguments were required to be sets.

In addition, both :class:`Set` and :class:`ImmutableSet` support set to set
comparisons.  Two sets are equal if and only if every element of each set is
contained in the other (each is a subset of the other). A set is less than
another set if and only if the first set is a proper subset of the second set
(is a subset, but is not equal). A set is greater than another set if and only
if the first set is a proper superset of the second set (is a superset, but is
not equal).

The subset and equality comparisons do not generalize to a complete ordering
function.  For example, any two disjoint sets are not equal and are not subsets
of each other, so *all* of the following return ``False``:  ``a<b``, ``a==b``,
or ``a>b``. Accordingly, sets do not implement the :meth:`__cmp__` method.

Since sets only define partial ordering (subset relationships), the output of
the :meth:`list.sort` method is undefined for lists of sets.

The following table lists operations available in :class:`ImmutableSet` but not
found in :class:`Set`:

+-------------+------------------------------+
| Operation   | Result                       |
+=============+==============================+
| ``hash(s)`` | returns a hash value for *s* |
+-------------+------------------------------+

The following table lists operations available in :class:`Set` but not found in
:class:`ImmutableSet`:

+--------------------------------------+-------------+---------------------------------+
| Operation                            | Equivalent  | Result                          |
+======================================+=============+=================================+
| ``s.update(t)``                      | *s* \|= *t* | return set *s* with elements    |
|                                      |             | added from *t*                  |
+--------------------------------------+-------------+---------------------------------+
| ``s.intersection_update(t)``         | *s* &= *t*  | return set *s* keeping only     |
|                                      |             | elements also found in *t*      |
+--------------------------------------+-------------+---------------------------------+
| ``s.difference_update(t)``           | *s* -= *t*  | return set *s* after removing   |
|                                      |             | elements found in *t*           |
+--------------------------------------+-------------+---------------------------------+
| ``s.symmetric_difference_update(t)`` | *s* ^= *t*  | return set *s* with elements    |
|                                      |             | from *s* or *t* but not both    |
+--------------------------------------+-------------+---------------------------------+
| ``s.add(x)``                         |             | add element *x* to set *s*      |
+--------------------------------------+-------------+---------------------------------+
| ``s.remove(x)``                      |             | remove *x* from set *s*; raises |
|                                      |             | :exc:`KeyError` if not present  |
+--------------------------------------+-------------+---------------------------------+
| ``s.discard(x)``                     |             | removes *x* from set *s* if     |
|                                      |             | present                         |
+--------------------------------------+-------------+---------------------------------+
| ``s.pop()``                          |             | remove and return an arbitrary  |
|                                      |             | element from *s*; raises        |
|                                      |             | :exc:`KeyError` if empty        |
+--------------------------------------+-------------+---------------------------------+
| ``s.clear()``                        |             | remove all elements from set    |
|                                      |             | *s*                             |
+--------------------------------------+-------------+---------------------------------+

Note, the non-operator versions of :meth:`update`, :meth:`intersection_update`,
:meth:`difference_update`, and :meth:`symmetric_difference_update` will accept
any iterable as an argument.

.. versionchanged:: 2.3.1
   Formerly all arguments were required to be sets.

Also note, the module also includes a :meth:`union_update` method which is an
alias for :meth:`update`.  The method is included for backwards compatibility.
Programmers should prefer the :meth:`update` method because it is supported by
the built-in :class:`set()` and :class:`frozenset()` types.


.. _set-example:

Example
-------

   >>> from sets import Set
   >>> engineers = Set(['John', 'Jane', 'Jack', 'Janice'])
   >>> programmers = Set(['Jack', 'Sam', 'Susan', 'Janice'])
   >>> managers = Set(['Jane', 'Jack', 'Susan', 'Zack'])
   >>> employees = engineers | programmers | managers           # union
   >>> engineering_management = engineers & managers            # intersection
   >>> fulltime_management = managers - engineers - programmers # difference
   >>> engineers.add('Marvin')                                  # add element
   >>> print engineers # doctest: +SKIP
   Set(['Jane', 'Marvin', 'Janice', 'John', 'Jack'])
   >>> employees.issuperset(engineers)     # superset test
   False
   >>> employees.update(engineers)         # update from another set
   >>> employees.issuperset(engineers)
   True
   >>> for group in [engineers, programmers, managers, employees]: # doctest: +SKIP
   ...     group.discard('Susan')          # unconditionally remove element
   ...     print group
   ...
   Set(['Jane', 'Marvin', 'Janice', 'John', 'Jack'])
   Set(['Janice', 'Jack', 'Sam'])
   Set(['Jane', 'Zack', 'Jack'])
   Set(['Jack', 'Sam', 'Jane', 'Marvin', 'Janice', 'John', 'Zack'])


.. _immutable-transforms:

Protocol for automatic conversion to immutable
----------------------------------------------

Sets can only contain immutable elements.  For convenience, mutable :class:`Set`
objects are automatically copied to an :class:`ImmutableSet` before being added
as a set element.

The mechanism is to always add a :term:`hashable` element, or if it is not
hashable, the element is checked to see if it has an :meth:`__as_immutable__`
method which returns an immutable equivalent.

Since :class:`Set` objects have a :meth:`__as_immutable__` method returning an
instance of :class:`ImmutableSet`, it is possible to construct sets of sets.

A similar mechanism is needed by the :meth:`__contains__` and :meth:`remove`
methods which need to hash an element to check for membership in a set.  Those
methods check an element for hashability and, if not, check for a
:meth:`__as_temporarily_immutable__` method which returns the element wrapped by
a class that provides temporary methods for :meth:`__hash__`, :meth:`__eq__`,
and :meth:`__ne__`.

The alternate mechanism spares the need to build a separate copy of the original
mutable object.

:class:`Set` objects implement the :meth:`__as_temporarily_immutable__` method
which returns the :class:`Set` object wrapped by a new class
:class:`_TemporarilyImmutableSet`.

The two mechanisms for adding hashability are normally invisible to the user;
however, a conflict can arise in a multi-threaded environment where one thread
is updating a set while another has temporarily wrapped it in
:class:`_TemporarilyImmutableSet`.  In other words, sets of mutable sets are not
thread-safe.


.. _comparison-to-builtin-set:

Comparison to the built-in :class:`set` types
---------------------------------------------

The built-in :class:`set` and :class:`frozenset` types were designed based on
lessons learned from the :mod:`sets` module.  The key differences are:

* :class:`Set` and :class:`ImmutableSet` were renamed to :class:`set` and
  :class:`frozenset`.

* There is no equivalent to :class:`BaseSet`.  Instead, use ``isinstance(x,
  (set, frozenset))``.

* The hash algorithm for the built-ins performs significantly better (fewer
  collisions) for most datasets.

* The built-in versions have more space efficient pickles.

* The built-in versions do not have a :meth:`union_update` method. Instead, use
  the :meth:`update` method which is equivalent.

* The built-in versions do not have a ``_repr(sorted=True)`` method.
  Instead, use the built-in :func:`repr` and :func:`sorted` functions:
  ``repr(sorted(s))``.

* The built-in version does not have a protocol for automatic conversion to
  immutable.  Many found this feature to be confusing and no one in the community
  reported having found real uses for it.

