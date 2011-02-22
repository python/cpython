:mod:`collections.abc` --- Abstract Base Classes for Containers
===============================================================

.. module:: collections.abc
   :synopsis: Abstract base classes for containers
.. moduleauthor:: Raymond Hettinger <python at rcn.com>
.. sectionauthor:: Raymond Hettinger <python at rcn.com>

.. testsetup:: *

   from collections import *
   import itertools
   __name__ = '<doctest>'

**Source code:** :source:`Lib/collections/abc.py`

--------------

This module provides :term:`abstract base classes <abstract base class>` that
can be used to test whether a class provides a particular interface; for
example, whether it is hashable or whether it is a mapping.

.. versionchanged:: 3.3
   Formerly, this module was part of the :mod:`collections` module.

.. _abstract-base-classes:

Collections Abstract Base Classes
---------------------------------

The collections module offers the following ABCs:

=========================  =====================  ======================  ====================================================
ABC                        Inherits               Abstract Methods        Mixin Methods
=========================  =====================  ======================  ====================================================
:class:`Container`                                ``__contains__``
:class:`Hashable`                                 ``__hash__``
:class:`Iterable`                                 ``__iter__``
:class:`Iterator`          :class:`Iterable`      ``__next__``            ``__iter__``
:class:`Sized`                                    ``__len__``
:class:`Callable`                                 ``__call__``

:class:`Sequence`          :class:`Sized`,        ``__getitem__``         ``__contains__``, ``__iter__``, ``__reversed__``,
                           :class:`Iterable`,                             ``index``, and ``count``
                           :class:`Container`

:class:`MutableSequence`   :class:`Sequence`      ``__setitem__``         Inherited Sequence methods and
                                                  ``__delitem__``,        ``append``, ``reverse``, ``extend``, ``pop``,
                                                  and ``insert``          ``remove``, and ``__iadd__``

:class:`Set`               :class:`Sized`,                                ``__le__``, ``__lt__``, ``__eq__``, ``__ne__``,
                           :class:`Iterable`,                             ``__gt__``, ``__ge__``, ``__and__``, ``__or__``,
                           :class:`Container`                             ``__sub__``, ``__xor__``, and ``isdisjoint``

:class:`MutableSet`        :class:`Set`           ``add`` and             Inherited Set methods and
                                                  ``discard``             ``clear``, ``pop``, ``remove``, ``__ior__``,
                                                                          ``__iand__``, ``__ixor__``, and ``__isub__``

:class:`Mapping`           :class:`Sized`,        ``__getitem__``         ``__contains__``, ``keys``, ``items``, ``values``,
                           :class:`Iterable`,                             ``get``, ``__eq__``, and ``__ne__``
                           :class:`Container`

:class:`MutableMapping`    :class:`Mapping`       ``__setitem__`` and     Inherited Mapping methods and
                                                  ``__delitem__``         ``pop``, ``popitem``, ``clear``, ``update``,
                                                                          and ``setdefault``


:class:`MappingView`       :class:`Sized`                                 ``__len__``
:class:`KeysView`          :class:`MappingView`,                          ``__contains__``,
                           :class:`Set`                                   ``__iter__``
:class:`ItemsView`         :class:`MappingView`,                          ``__contains__``,
                           :class:`Set`                                   ``__iter__``
:class:`ValuesView`        :class:`MappingView`                           ``__contains__``, ``__iter__``
=========================  =====================  ======================  ====================================================

These ABCs allow us to ask classes or instances if they provide
particular functionality, for example::

    size = None
    if isinstance(myvar, collections.Sized):
        size = len(myvar)

Several of the ABCs are also useful as mixins that make it easier to develop
classes supporting container APIs.  For example, to write a class supporting
the full :class:`Set` API, it only necessary to supply the three underlying
abstract methods: :meth:`__contains__`, :meth:`__iter__`, and :meth:`__len__`.
The ABC supplies the remaining methods such as :meth:`__and__` and
:meth:`isdisjoint` ::

    class ListBasedSet(collections.Set):
         ''' Alternate set implementation favoring space over speed
             and not requiring the set elements to be hashable. '''
         def __init__(self, iterable):
             self.elements = lst = []
             for value in iterable:
                 if value not in lst:
                     lst.append(value)
         def __iter__(self):
             return iter(self.elements)
         def __contains__(self, value):
             return value in self.elements
         def __len__(self):
             return len(self.elements)

    s1 = ListBasedSet('abcdef')
    s2 = ListBasedSet('defghi')
    overlap = s1 & s2            # The __and__() method is supported automatically

Notes on using :class:`Set` and :class:`MutableSet` as a mixin:

(1)
   Since some set operations create new sets, the default mixin methods need
   a way to create new instances from an iterable. The class constructor is
   assumed to have a signature in the form ``ClassName(iterable)``.
   That assumption is factored-out to an internal classmethod called
   :meth:`_from_iterable` which calls ``cls(iterable)`` to produce a new set.
   If the :class:`Set` mixin is being used in a class with a different
   constructor signature, you will need to override :meth:`from_iterable`
   with a classmethod that can construct new instances from
   an iterable argument.

(2)
   To override the comparisons (presumably for speed, as the
   semantics are fixed), redefine :meth:`__le__` and
   then the other operations will automatically follow suit.

(3)
   The :class:`Set` mixin provides a :meth:`_hash` method to compute a hash value
   for the set; however, :meth:`__hash__` is not defined because not all sets
   are hashable or immutable.  To add set hashabilty using mixins,
   inherit from both :meth:`Set` and :meth:`Hashable`, then define
   ``__hash__ = Set._hash``.

.. seealso::

  * `OrderedSet recipe <http://code.activestate.com/recipes/576694/>`_ that uses
    :class:`MutableSet`.

  * For more about ABCs, see the :mod:`abc` module and :pep:`3119`.
