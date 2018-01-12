:mod:`collections.abc` --- Abstract Base Classes for Containers
===============================================================

.. module:: collections.abc
   :synopsis: Abstract base classes for containers

.. moduleauthor:: Raymond Hettinger <python at rcn.com>
.. sectionauthor:: Raymond Hettinger <python at rcn.com>

.. versionadded:: 3.3
   Formerly, this module was part of the :mod:`collections` module.

**Source code:** :source:`Lib/_collections_abc.py`

.. testsetup:: *

   from collections import *
   import itertools
   __name__ = '<doctest>'

--------------

This module provides :term:`abstract base classes <abstract base class>` that
can be used to test whether a class provides a particular interface; for
example, whether it is hashable or whether it is a mapping.


.. _collections-abstract-base-classes:

Collections Abstract Base Classes
---------------------------------

The collections module offers the following :term:`ABCs <abstract base class>`:

.. tabularcolumns:: |l|L|L|L|

========================== ====================== ======================= ====================================================
ABC                        Inherits from          Abstract Methods        Mixin Methods
========================== ====================== ======================= ====================================================
:class:`Container`                                ``__contains__``
:class:`Hashable`                                 ``__hash__``
:class:`Iterable`                                 ``__iter__``
:class:`Iterator`          :class:`Iterable`      ``__next__``            ``__iter__``
:class:`Reversible`        :class:`Iterable`      ``__reversed__``
:class:`Generator`         :class:`Iterator`      ``send``, ``throw``     ``close``, ``__iter__``, ``__next__``
:class:`Sized`                                    ``__len__``
:class:`Callable`                                 ``__call__``
:class:`Collection`        :class:`Sized`,        ``__contains__``,
                           :class:`Iterable`,     ``__iter__``,
                           :class:`Container`     ``__len__``

:class:`Sequence`          :class:`Reversible`,   ``__getitem__``,        ``__contains__``, ``__iter__``, ``__reversed__``,
                           :class:`Collection`    ``__len__``             ``index``, and ``count``

:class:`MutableSequence`   :class:`Sequence`      ``__getitem__``,        Inherited :class:`Sequence` methods and
                                                  ``__setitem__``,        ``append``, ``reverse``, ``extend``, ``pop``,
                                                  ``__delitem__``,        ``remove``, and ``__iadd__``
                                                  ``__len__``,
                                                  ``insert``

:class:`ByteString`        :class:`Sequence`      ``__getitem__``,        Inherited :class:`Sequence` methods
                                                  ``__len__``

:class:`Set`               :class:`Collection`    ``__contains__``,       ``__le__``, ``__lt__``, ``__eq__``, ``__ne__``,
                                                  ``__iter__``,           ``__gt__``, ``__ge__``, ``__and__``, ``__or__``,
                                                  ``__len__``             ``__sub__``, ``__xor__``, and ``isdisjoint``

:class:`MutableSet`        :class:`Set`           ``__contains__``,       Inherited :class:`Set` methods and
                                                  ``__iter__``,           ``clear``, ``pop``, ``remove``, ``__ior__``,
                                                  ``__len__``,            ``__iand__``, ``__ixor__``, and ``__isub__``
                                                  ``add``,
                                                  ``discard``

:class:`Mapping`           :class:`Collection`    ``__getitem__``,        ``__contains__``, ``keys``, ``items``, ``values``,
                                                  ``__iter__``,           ``get``, ``__eq__``, and ``__ne__``
                                                  ``__len__``

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
:class:`ValuesView`        :class:`MappingView`,                          ``__contains__``, ``__iter__``
                           :class:`Collection`
:class:`Awaitable`                                ``__await__``
:class:`Coroutine`         :class:`Awaitable`     ``send``, ``throw``     ``close``
:class:`AsyncIterable`                            ``__aiter__``
:class:`AsyncIterator`     :class:`AsyncIterable` ``__anext__``           ``__aiter__``
:class:`AsyncGenerator`    :class:`AsyncIterator` ``asend``, ``athrow``   ``aclose``, ``__aiter__``, ``__anext__``
========================== ====================== ======================= ====================================================


.. class:: Container
           Hashable
           Sized
           Callable

   ABCs for classes that provide respectively the methods :meth:`__contains__`,
   :meth:`__hash__`, :meth:`__len__`, and :meth:`__call__`.

.. class:: Iterable

   ABC for classes that provide the :meth:`__iter__` method.

   Checking ``isinstance(obj, Iterable)`` detects classes that are registered
   as :class:`Iterable` or that have an :meth:`__iter__` method, but it does
   not detect classes that iterate with the :meth:`__getitem__` method.
   The only reliable way to determine whether an object is :term:`iterable`
   is to call ``iter(obj)``.

.. class:: Collection

   ABC for sized iterable container classes.

   .. versionadded:: 3.6

.. class:: Iterator

   ABC for classes that provide the :meth:`~iterator.__iter__` and
   :meth:`~iterator.__next__` methods.  See also the definition of
   :term:`iterator`.

.. class:: Reversible

   ABC for iterable classes that also provide the :meth:`__reversed__`
   method.

   .. versionadded:: 3.6

.. class:: Generator

   ABC for generator classes that implement the protocol defined in
   :pep:`342` that extends iterators with the :meth:`~generator.send`,
   :meth:`~generator.throw` and :meth:`~generator.close` methods.
   See also the definition of :term:`generator`.

   .. versionadded:: 3.5

.. class:: Sequence
           MutableSequence
           ByteString

   ABCs for read-only and mutable :term:`sequences <sequence>`.

   Implementation note: Some of the mixin methods, such as
   :meth:`__iter__`, :meth:`__reversed__` and :meth:`index`, make
   repeated calls to the underlying :meth:`__getitem__` method.
   Consequently, if :meth:`__getitem__` is implemented with constant
   access speed, the mixin methods will have linear performance;
   however, if the underlying method is linear (as it would be with a
   linked list), the mixins will have quadratic performance and will
   likely need to be overridden.

   .. versionchanged:: 3.5
      The index() method added support for *stop* and *start*
      arguments.

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

   ABCs for mapping, items, keys, and values :term:`views <dictionary view>`.

.. class:: Awaitable

   ABC for :term:`awaitable` objects, which can be used in :keyword:`await`
   expressions.  Custom implementations must provide the :meth:`__await__`
   method.

   :term:`Coroutine` objects and instances of the
   :class:`~collections.abc.Coroutine` ABC are all instances of this ABC.

   .. note::
      In CPython, generator-based coroutines (generators decorated with
      :func:`types.coroutine` or :func:`asyncio.coroutine`) are
      *awaitables*, even though they do not have an :meth:`__await__` method.
      Using ``isinstance(gencoro, Awaitable)`` for them will return ``False``.
      Use :func:`inspect.isawaitable` to detect them.

   .. versionadded:: 3.5

.. class:: Coroutine

   ABC for coroutine compatible classes.  These implement the
   following methods, defined in :ref:`coroutine-objects`:
   :meth:`~coroutine.send`, :meth:`~coroutine.throw`, and
   :meth:`~coroutine.close`.  Custom implementations must also implement
   :meth:`__await__`.  All :class:`Coroutine` instances are also instances of
   :class:`Awaitable`.  See also the definition of :term:`coroutine`.

   .. note::
      In CPython, generator-based coroutines (generators decorated with
      :func:`types.coroutine` or :func:`asyncio.coroutine`) are
      *awaitables*, even though they do not have an :meth:`__await__` method.
      Using ``isinstance(gencoro, Coroutine)`` for them will return ``False``.
      Use :func:`inspect.isawaitable` to detect them.

   .. versionadded:: 3.5

.. class:: AsyncIterable

   ABC for classes that provide ``__aiter__`` method.  See also the
   definition of :term:`asynchronous iterable`.

   .. versionadded:: 3.5

.. class:: AsyncIterator

   ABC for classes that provide ``__aiter__`` and ``__anext__``
   methods.  See also the definition of :term:`asynchronous iterator`.

   .. versionadded:: 3.5

.. class:: AsyncGenerator

   ABC for asynchronous generator classes that implement the protocol
   defined in :pep:`525` and :pep:`492`.

   .. versionadded:: 3.6


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
   are hashable or immutable.  To add set hashability using mixins,
   inherit from both :meth:`Set` and :meth:`Hashable`, then define
   ``__hash__ = Set._hash``.

.. seealso::

   * `OrderedSet recipe <https://code.activestate.com/recipes/576694/>`_ for an
     example built on :class:`MutableSet`.

   * For more about ABCs, see the :mod:`abc` module and :pep:`3119`.
