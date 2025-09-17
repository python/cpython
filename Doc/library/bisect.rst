:mod:`!bisect` --- Array bisection algorithm
============================================

.. module:: bisect
   :synopsis: Array bisection algorithms for binary searching.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. sectionauthor:: Raymond Hettinger <python at rcn.com>
.. example based on the PyModules FAQ entry by Aaron Watters <arw@pythonpros.com>

**Source code:** :source:`Lib/bisect.py`

--------------

This module provides support for maintaining a list in sorted order without
having to sort the list after each insertion.  For long lists of items with
expensive comparison operations, this can be an improvement over
linear searches or frequent resorting.

The module is called :mod:`bisect` because it uses a basic bisection
algorithm to do its work.  Unlike other bisection tools that search for a
specific value, the functions in this module are designed to locate an
insertion point. Accordingly, the functions never call an :meth:`~object.__eq__`
method to determine whether a value has been found.  Instead, the
functions only call the :meth:`~object.__lt__` method and will return an insertion
point between values in an array.

.. note::

   The functions in this module are not thread-safe. If multiple threads
   concurrently use :mod:`bisect` functions on the same sequence, this
   may result in undefined behaviour. Likewise, if the provided sequence
   is mutated by a different thread while a :mod:`bisect` function
   is operating on it, the result is undefined. For example, using
   :py:func:`~bisect.insort_left` on the same list from multiple threads
   may result in the list becoming unsorted.

.. _bisect functions:

The following functions are provided:


.. function:: bisect_left(a, x, lo=0, hi=len(a), *, key=None)

   Locate the insertion point for *x* in *a* to maintain sorted order.
   The parameters *lo* and *hi* may be used to specify a subset of the list
   which should be considered; by default the entire list is used.  If *x* is
   already present in *a*, the insertion point will be before (to the left of)
   any existing entries.  The return value is suitable for use as the first
   parameter to ``list.insert()`` assuming that *a* is already sorted.

   The returned insertion point *ip* partitions the array *a* into two
   slices such that ``all(elem < x for elem in a[lo : ip])`` is true for the
   left slice and ``all(elem >= x for elem in a[ip : hi])`` is true for the
   right slice.

   *key* specifies a :term:`key function` of one argument that is used to
   extract a comparison key from each element in the array.  To support
   searching complex records, the key function is not applied to the *x* value.

   If *key* is ``None``, the elements are compared directly and
   no key function is called.

   .. versionchanged:: 3.10
      Added the *key* parameter.


.. function:: bisect_right(a, x, lo=0, hi=len(a), *, key=None)
              bisect(a, x, lo=0, hi=len(a), *, key=None)

   Similar to :py:func:`~bisect.bisect_left`, but returns an insertion point which comes
   after (to the right of) any existing entries of *x* in *a*.

   The returned insertion point *ip* partitions the array *a* into two slices
   such that ``all(elem <= x for elem in a[lo : ip])`` is true for the left slice and
   ``all(elem > x for elem in a[ip : hi])`` is true for the right slice.

   .. versionchanged:: 3.10
      Added the *key* parameter.


.. function:: insort_left(a, x, lo=0, hi=len(a), *, key=None)

   Insert *x* in *a* in sorted order.

   This function first runs :py:func:`~bisect.bisect_left` to locate an insertion point.
   Next, it runs the :meth:`~sequence.insert` method on *a* to insert *x* at the
   appropriate position to maintain sort order.

   To support inserting records in a table, the *key* function (if any) is
   applied to *x* for the search step but not for the insertion step.

   Keep in mind that the *O*\ (log *n*) search is dominated by the slow *O*\ (*n*)
   insertion step.

   .. versionchanged:: 3.10
      Added the *key* parameter.


.. function:: insort_right(a, x, lo=0, hi=len(a), *, key=None)
              insort(a, x, lo=0, hi=len(a), *, key=None)

   Similar to :py:func:`~bisect.insort_left`, but inserting *x* in *a* after any existing
   entries of *x*.

   This function first runs :py:func:`~bisect.bisect_right` to locate an insertion point.
   Next, it runs the :meth:`~sequence.insert` method on *a* to insert *x* at the
   appropriate position to maintain sort order.

   To support inserting records in a table, the *key* function (if any) is
   applied to *x* for the search step but not for the insertion step.

   Keep in mind that the *O*\ (log *n*) search is dominated by the slow *O*\ (*n*)
   insertion step.

   .. versionchanged:: 3.10
      Added the *key* parameter.


Performance Notes
-----------------

When writing time sensitive code using *bisect()* and *insort()*, keep these
thoughts in mind:

* Bisection is effective for searching ranges of values.
  For locating specific values, dictionaries are more performant.

* The *insort()* functions are *O*\ (*n*) because the logarithmic search step
  is dominated by the linear time insertion step.

* The search functions are stateless and discard key function results after
  they are used.  Consequently, if the search functions are used in a loop,
  the key function may be called again and again on the same array elements.
  If the key function isn't fast, consider wrapping it with
  :py:func:`functools.cache` to avoid duplicate computations.  Alternatively,
  consider searching an array of precomputed keys to locate the insertion
  point (as shown in the examples section below).

.. seealso::

   * `Sorted Collections
     <https://grantjenks.com/docs/sortedcollections/>`_ is a high performance
     module that uses *bisect* to managed sorted collections of data.

   * The `SortedCollection recipe
     <https://code.activestate.com/recipes/577197-sortedcollection/>`_ uses
     bisect to build a full-featured collection class with straight-forward search
     methods and support for a key-function.  The keys are precomputed to save
     unnecessary calls to the key function during searches.


Searching Sorted Lists
----------------------

The above `bisect functions`_ are useful for finding insertion points but
can be tricky or awkward to use for common searching tasks. The following five
functions show how to transform them into the standard lookups for sorted
lists::

    def index(a, x):
        'Locate the leftmost value exactly equal to x'
        i = bisect_left(a, x)
        if i != len(a) and a[i] == x:
            return i
        raise ValueError

    def find_lt(a, x):
        'Find rightmost value less than x'
        i = bisect_left(a, x)
        if i:
            return a[i-1]
        raise ValueError

    def find_le(a, x):
        'Find rightmost value less than or equal to x'
        i = bisect_right(a, x)
        if i:
            return a[i-1]
        raise ValueError

    def find_gt(a, x):
        'Find leftmost value greater than x'
        i = bisect_right(a, x)
        if i != len(a):
            return a[i]
        raise ValueError

    def find_ge(a, x):
        'Find leftmost item greater than or equal to x'
        i = bisect_left(a, x)
        if i != len(a):
            return a[i]
        raise ValueError


Examples
--------

.. _bisect-example:

The :py:func:`~bisect.bisect` function can be useful for numeric table lookups. This
example uses :py:func:`~bisect.bisect` to look up a letter grade for an exam score (say)
based on a set of ordered numeric breakpoints: 90 and up is an 'A', 80 to 89 is
a 'B', and so on::

   >>> def grade(score, breakpoints=[60, 70, 80, 90], grades='FDCBA'):
   ...     i = bisect(breakpoints, score)
   ...     return grades[i]
   ...
   >>> [grade(score) for score in [33, 99, 77, 70, 89, 90, 100]]
   ['F', 'A', 'C', 'C', 'B', 'A', 'A']

The :py:func:`~bisect.bisect` and :py:func:`~bisect.insort` functions also work with
lists of tuples.  The *key* argument can serve to extract the field used for ordering
records in a table::

    >>> from collections import namedtuple
    >>> from operator import attrgetter
    >>> from bisect import bisect, insort
    >>> from pprint import pprint

    >>> Movie = namedtuple('Movie', ('name', 'released', 'director'))

    >>> movies = [
    ...     Movie('Jaws', 1975, 'Spielberg'),
    ...     Movie('Titanic', 1997, 'Cameron'),
    ...     Movie('The Birds', 1963, 'Hitchcock'),
    ...     Movie('Aliens', 1986, 'Cameron')
    ... ]

    >>> # Find the first movie released after 1960
    >>> by_year = attrgetter('released')
    >>> movies.sort(key=by_year)
    >>> movies[bisect(movies, 1960, key=by_year)]
    Movie(name='The Birds', released=1963, director='Hitchcock')

    >>> # Insert a movie while maintaining sort order
    >>> romance = Movie('Love Story', 1970, 'Hiller')
    >>> insort(movies, romance, key=by_year)
    >>> pprint(movies)
    [Movie(name='The Birds', released=1963, director='Hitchcock'),
     Movie(name='Love Story', released=1970, director='Hiller'),
     Movie(name='Jaws', released=1975, director='Spielberg'),
     Movie(name='Aliens', released=1986, director='Cameron'),
     Movie(name='Titanic', released=1997, director='Cameron')]

If the key function is expensive, it is possible to avoid repeated function
calls by searching a list of precomputed keys to find the index of a record::

    >>> data = [('red', 5), ('blue', 1), ('yellow', 8), ('black', 0)]
    >>> data.sort(key=lambda r: r[1])       # Or use operator.itemgetter(1).
    >>> keys = [r[1] for r in data]         # Precompute a list of keys.
    >>> data[bisect_left(keys, 0)]
    ('black', 0)
    >>> data[bisect_left(keys, 1)]
    ('blue', 1)
    >>> data[bisect_left(keys, 5)]
    ('red', 5)
    >>> data[bisect_left(keys, 8)]
    ('yellow', 8)
