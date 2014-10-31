:mod:`collections.abc` --- Abstract Base Classes for Containers
===============================================================

.. module:: collections.abc
   :synopsis: Abstract base classes for containers
.. moduleauthor:: Raymond Hettinger <python at rcn.com>
.. sectionauthor:: Raymond Hettinger <python at rcn.com>

.. versionadded:: 3.3
   Formerly, this module was part of the :mod:`collections` module.

.. testsetup:: *

   from collections import *
   import itertools
   __name__ = '<doctest>'

**Source code:** :source:`Lib/_collections_abc.py`

--------------

This module provides :term:`abstract base classes <abstract base class>` that
can be used to test whether a class provides a particular interface; for
example, whether it is hashable or whether it is a mapping.


.. _collections-abstract-base-classes:

Collections Abstract Base Classes
---------------------------------

The collections module offers the following :term:`ABCs <abstract base class>`:

.. tabularcolumns:: |l|L|L|L|

=========================  =====================  ======================  ====================================================
ABC                        Inherits from          Abstract Methods        Mixin Methods
=========================  =====================  ======================  ====================================================
:class:`Container`                                ``__contains__``
:class:`Hashable`                                 ``__hash__``
:class:`Iterable`                                 ``__iter__``
:class:`Iterator`          :class:`Iterable`      ``__next__``            ``__iter__``
:class:`Sized`                                    ``__len__``
:class:`Callable`                                 ``__call__``

:class:`Sequence`          :class:`Sized`,        ``__getitem__``,        ``__contains__``, ``__iter__``, ``__reversed__``,
                           :class:`Iterable`,     ``__len__``             ``index``, and ``count``
                           :class:`Container`

:class:`MutableSequence`   :class:`Sequence`      ``__getitem__``,        Inherited :class:`Sequence` methods and
                                                  ``__setitem__``,        ``append``, ``reverse``, ``extend``, ``pop``,
                                                  ``__delitem__``,        ``remove``, and ``__iadd__``
                                                  ``__len__``,
                                                  ``insert``

:class:`Set`               :class:`Sized`,        ``__contains__``,       ``__le__``, ``__lt__``, ``__eq__``, ``__ne__``,
                           :class:`Iterable`,     ``__iter__``,           ``__gt__``, ``__ge__``, ``__and__``, ``__or__``,
                           :class:`Container`     ``__len__``             ``__sub__``, ``__xor__``, and ``isdisjoint``

:class:`MutableSet`        :class:`Set`           ``__contains__``,       Inherited :class:`Set` methods and
                                                  ``__iter__``,           ``clear``, ``pop``, ``remove``, ``__ior__``,
                                                  ``__len__``,            ``__iand__``, ``__ixor__``, and ``__isub__``
                                                  ``add``,
                                                  ``discard``

:class:`Mapping`           :class:`Sized`,        ``__getitem__``,        ``__contains__``, ``keys``, ``items``, ``values``,
                           :class:`Iterable`,     ``__iter__``,           ``get``, ``__eq__``, and ``__ne__``
                           :class:`Container`     ``__len__``

:class:`MutableMapping`    :class:`Mapping`       ``__getitem__``,        Inherited :class:`Mapping` methods and
                                                  ``__setitem__``,        ``pop``, ``popitem``, ``clear``, ``update``,
                                                  ``__delitem__``,        and ``setdefault``
                                                  ``__iter__``,
                                                  ``__len__``


:class:`MappingView`       :class:`Sized`                                 ``__len__``
:class:`ItemsView`         :class:`MappingView`,                          ``__contains__``,
                           :class:`Set`                                   ``__iter__``
:class:`KeysView`          :class:`MappingView`,                          ``__contains__``,
                           :class:`Set`                                   ``__iter__``
:class:`ValuesView`        :class:`MappingView`                           ``__contains__``, ``__iter__``
=========================  =====================  ======================  ====================================================


.. class:: Container
           Hashable
           Sized
           Callable

   ABCs for classes that provide respectively the methods :meth:`__contains__`,
   :meth:`__hash__`, :meth:`__len__`, and :meth:`__call__`.

.. class:: Iterable

   ABC for classes that provide the :meth:`__iter__` method.
   See also the definition of :term:`iterable`.

.. class:: Iterator

   ABC for classes that provide the :meth:`~iterator.__iter__` and
   :meth:`~iterator.__next__` methods.  See also the definition of
   :term:`iterator`.

.. class:: Sequence
           MutableSequence

   ABCs for read-only and mutable :term:`sequences <sequence>`.

.. class:: Set
           MutableSet

   ABCs for read-only and mutable sets.

.. class:: Mapping
           MutableMapping

   ABCs for read-only and mutable :term:`mappings <mapping>`.

.. class:: MappingView
           ItemsView
           KeysView
           ValuesView

   ABCs for mapping, items, keys, and values :term:`views <view>`.


These ABCs allow us to ask classes or instances if they provide
particular functionality, for example::

    size = None
    if isinstance(myvar, collections.abc.Sized):
        size = len(myvar)

Several of the ABCs are also useful as mixins that make it easier to develop
classes supporting container APIs.  For example, to write a class supporting
the full :class:`Set` API, it is only necessary to supply the three underlying
abstract methods: :meth:`__contains__`, :meth:`__iter__`, and :meth:`__len__`.
The ABC supplies the remaining methods such as :meth:`__and__` and
:meth:`isdisjoint`::

    class ListBasedSet(collections.abc.Set):
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
   constructor signature, you will need to override :meth:`_from_iterable`
   with a classmethod that can construct new instances from
   an iterable argument.

(2)
   To override the comparisons (presumably for speed, as the
   semantics are fixed), redefine :meth:`__le__` and :meth:`__ge__`,
   then the other operations will automatically follow suit.

(3)
   The :class:`Set` mixin provides a :meth:`_hash` method to compute a hash value
   for the set; however, :meth:`__hash__` is not defined because not all sets
   are hashable or immutable.  To add set hashabilty using mixins,
   inherit from both :meth:`Set` and :meth:`Hashable`, then define
   ``__hash__ = Set._hash``.

.. seealso::

   * `OrderedSet recipe <http://code.activestate.com/recipes/576694/>`_ for an
     example built on :class:`MutableSet`.

   * For more about ABCs, see the :mod:`abc` module and :pep:`3119`.
