
:mod:`bisect` --- Array bisection algorithm
===========================================

.. module:: bisect
   :synopsis: Array bisection algorithms for binary searching.
.. sectionauthor:: Fred L. Drake, Jr. <fdrake@acm.org>
.. example based on the PyModules FAQ entry by Aaron Watters <arw@pythonpros.com>

This module provides support for maintaining a list in sorted order without
having to sort the list after each insertion.  For long lists of items with
expensive comparison operations, this can be an improvement over the more common
approach.  The module is called :mod:`bisect` because it uses a basic bisection
algorithm to do its work.  The source code may be most useful as a working
example of the algorithm (the boundary conditions are already right!).

The following functions are provided:


.. function:: bisect_left(list, item[, lo[, hi]])

   Locate the proper insertion point for *item* in *list* to maintain sorted order.
   The parameters *lo* and *hi* may be used to specify a subset of the list which
   should be considered; by default the entire list is used.  If *item* is already
   present in *list*, the insertion point will be before (to the left of) any
   existing entries.  The return value is suitable for use as the first parameter
   to ``list.insert()``.  This assumes that *list* is already sorted.


.. function:: bisect_right(list, item[, lo[, hi]])

   Similar to :func:`bisect_left`, but returns an insertion point which comes after
   (to the right of) any existing entries of *item* in *list*.


.. function:: bisect(...)

   Alias for :func:`bisect_right`.


.. function:: insort_left(list, item[, lo[, hi]])

   Insert *item* in *list* in sorted order.  This is equivalent to
   ``list.insert(bisect.bisect_left(list, item, lo, hi), item)``.  This assumes
   that *list* is already sorted.


.. function:: insort_right(list, item[, lo[, hi]])

   Similar to :func:`insort_left`, but inserting *item* in *list* after any
   existing entries of *item*.


.. function:: insort(...)

   Alias for :func:`insort_right`.


Examples
--------

.. _bisect-example:

The :func:`bisect` function is generally useful for categorizing numeric data.
This example uses :func:`bisect` to look up a letter grade for an exam total
(say) based on a set of ordered numeric breakpoints: 85 and up is an 'A', 75..84
is a 'B', etc. ::

   >>> grades = "FEDCBA"
   >>> breakpoints = [30, 44, 66, 75, 85]
   >>> from bisect import bisect
   >>> def grade(total):
   ...           return grades[bisect(breakpoints, total)]
   ...
   >>> grade(66)
   'C'
   >>> map(grade, [33, 99, 77, 44, 12, 88])
   ['E', 'A', 'B', 'D', 'F', 'A']


