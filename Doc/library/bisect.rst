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


.. function:: bisect_left(a, x, lo=0, hi=len(a))

   Locate the proper insertion point for *x* in *a* to maintain sorted order.
   The parameters *lo* and *hi* may be used to specify a subset of the list
   which should be considered; by default the entire list is used.  If *x* is
   already present in *a*, the insertion point will be before (to the left of)
   any existing entries.  The return value is suitable for use as the first
   parameter to ``list.insert()``.  This assumes that *a* is already sorted.


.. function:: bisect_right(a, x, lo=0, hi=len(a))
              bisect(a, x, lo=0, hi=len(a))

   Similar to :func:`bisect_left`, but returns an insertion point which comes
   after (to the right of) any existing entries of *x* in *a*.


.. function:: insort_left(a, x, lo=0, hi=len(a))

   Insert *x* in *a* in sorted order.  This is equivalent to
   ``a.insert(bisect.bisect_left(a, x, lo, hi), x)``.  This assumes that *a* is
   already sorted.


.. function:: insort_right(a, x, lo=0, hi=len(a))
              insort(a, x, lo=0, hi=len(a))

   Similar to :func:`insort_left`, but inserting *x* in *a* after any existing
   entries of *x*.


Examples
--------

.. _bisect-example:

The :func:`bisect` function is generally useful for categorizing numeric data.
This example uses :func:`bisect` to look up a letter grade for an exam total
(say) based on a set of ordered numeric breakpoints: 85 and up is an 'A', 75..84
is a 'B', etc.

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


