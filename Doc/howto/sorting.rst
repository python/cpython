.. _sortinghowto:

Sorting HOW TO
**************

:Author: Andrew Dalke and Raymond Hettinger
:Release: 0.1


Python lists have a built-in :meth:`list.sort` method that modifies the list
in-place.  There is also a :func:`sorted` built-in function that builds a new
sorted list from an iterable.

In this document, we explore the various techniques for sorting data using Python.


Sorting Basics
==============

A simple ascending sort is very easy: just call the :func:`sorted` function. It
returns a new sorted list:

.. doctest::

    >>> sorted([5, 2, 3, 1, 4])
    [1, 2, 3, 4, 5]

You can also use the :meth:`list.sort` method. It modifies the list
in-place (and returns ``None`` to avoid confusion). Usually it's less convenient
than :func:`sorted` - but if you don't need the original list, it's slightly
more efficient.

.. doctest::

    >>> a = [5, 2, 3, 1, 4]
    >>> a.sort()
    >>> a
    [1, 2, 3, 4, 5]

Another difference is that the :meth:`list.sort` method is only defined for
lists. In contrast, the :func:`sorted` function accepts any iterable.

.. doctest::

    >>> sorted({1: 'D', 2: 'B', 3: 'B', 4: 'E', 5: 'A'})
    [1, 2, 3, 4, 5]

Key Functions
=============

Both :meth:`list.sort` and :func:`sorted` have a *key* parameter to specify a
function (or other callable) to be called on each list element prior to making
comparisons.

For example, here's a case-insensitive string comparison:

.. doctest::

    >>> sorted("This is a test string from Andrew".split(), key=str.lower)
    ['a', 'Andrew', 'from', 'is', 'string', 'test', 'This']

The value of the *key* parameter should be a function (or other callable) that
takes a single argument and returns a key to use for sorting purposes. This
technique is fast because the key function is called exactly once for each
input record.

A common pattern is to sort complex objects using some of the object's indices
as keys. For example:

.. doctest::

    >>> student_tuples = [
    ...     ('john', 'A', 15),
    ...     ('jane', 'B', 12),
    ...     ('dave', 'B', 10),
    ... ]
    >>> sorted(student_tuples, key=lambda student: student[2])   # sort by age
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

The same technique works for objects with named attributes. For example:

.. doctest::

    >>> class Student:
    ...     def __init__(self, name, grade, age):
    ...         self.name = name
    ...         self.grade = grade
    ...         self.age = age
    ...     def __repr__(self):
    ...         return repr((self.name, self.grade, self.age))

    >>> student_objects = [
    ...     Student('john', 'A', 15),
    ...     Student('jane', 'B', 12),
    ...     Student('dave', 'B', 10),
    ... ]
    >>> sorted(student_objects, key=lambda student: student.age)   # sort by age
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

Operator Module Functions
=========================

The key-function patterns shown above are very common, so Python provides
convenience functions to make accessor functions easier and faster. The
:mod:`operator` module has :func:`~operator.itemgetter`,
:func:`~operator.attrgetter`, and a :func:`~operator.methodcaller` function.

Using those functions, the above examples become simpler and faster:

.. doctest::

    >>> from operator import itemgetter, attrgetter

    >>> sorted(student_tuples, key=itemgetter(2))
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

    >>> sorted(student_objects, key=attrgetter('age'))
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

The operator module functions allow multiple levels of sorting. For example, to
sort by *grade* then by *age*:

.. doctest::

    >>> sorted(student_tuples, key=itemgetter(1,2))
    [('john', 'A', 15), ('dave', 'B', 10), ('jane', 'B', 12)]

    >>> sorted(student_objects, key=attrgetter('grade', 'age'))
    [('john', 'A', 15), ('dave', 'B', 10), ('jane', 'B', 12)]

Ascending and Descending
========================

Both :meth:`list.sort` and :func:`sorted` accept a *reverse* parameter with a
boolean value. This is used to flag descending sorts. For example, to get the
student data in reverse *age* order:

.. doctest::

    >>> sorted(student_tuples, key=itemgetter(2), reverse=True)
    [('john', 'A', 15), ('jane', 'B', 12), ('dave', 'B', 10)]

    >>> sorted(student_objects, key=attrgetter('age'), reverse=True)
    [('john', 'A', 15), ('jane', 'B', 12), ('dave', 'B', 10)]

Sort Stability and Complex Sorts
================================

Sorts are guaranteed to be `stable
<https://en.wikipedia.org/wiki/Sorting_algorithm#Stability>`_\. That means that
when multiple records have the same key, their original order is preserved.

.. doctest::

    >>> data = [('red', 1), ('blue', 1), ('red', 2), ('blue', 2)]
    >>> sorted(data, key=itemgetter(0))
    [('blue', 1), ('blue', 2), ('red', 1), ('red', 2)]

Notice how the two records for *blue* retain their original order so that
``('blue', 1)`` is guaranteed to precede ``('blue', 2)``.

This wonderful property lets you build complex sorts in a series of sorting
steps. For example, to sort the student data by descending *grade* and then
ascending *age*, do the *age* sort first and then sort again using *grade*:

.. doctest::

    >>> s = sorted(student_objects, key=attrgetter('age'))     # sort on secondary key
    >>> sorted(s, key=attrgetter('grade'), reverse=True)       # now sort on primary key, descending
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

This can be abstracted out into a wrapper function that can take a list and
tuples of field and order to sort them on multiple passes.

.. doctest::

    >>> def multisort(xs, specs):
    ...     for key, reverse in reversed(specs):
    ...         xs.sort(key=attrgetter(key), reverse=reverse)
    ...     return xs

    >>> multisort(list(student_objects), (('grade', True), ('age', False)))
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

The `Timsort <https://en.wikipedia.org/wiki/Timsort>`_ algorithm used in Python
does multiple sorts efficiently because it can take advantage of any ordering
already present in a dataset.

Decorate-Sort-Undecorate
========================

This idiom is called Decorate-Sort-Undecorate after its three steps:

* First, the initial list is decorated with new values that control the sort order.

* Second, the decorated list is sorted.

* Finally, the decorations are removed, creating a list that contains only the
  initial values in the new order.

For example, to sort the student data by *grade* using the DSU approach:

    >>> decorated = [(student.grade, i, student) for i, student in enumerate(student_objects)]
    >>> decorated.sort()
    >>> [student for grade, i, student in decorated]               # undecorate
    [('john', 'A', 15), ('jane', 'B', 12), ('dave', 'B', 10)]

This idiom works because tuples are compared lexicographically; the first items
are compared; if they are the same then the second items are compared, and so
on.

It is not strictly necessary in all cases to include the index *i* in the
decorated list, but including it gives two benefits:

* The sort is stable -- if two items have the same key, their order will be
  preserved in the sorted list.

* The original items do not have to be comparable because the ordering of the
  decorated tuples will be determined by at most the first two items. So for
  example the original list could contain complex numbers which cannot be sorted
  directly.

Another name for this idiom is
`Schwartzian transform <https://en.wikipedia.org/wiki/Schwartzian_transform>`_\,
after Randal L. Schwartz, who popularized it among Perl programmers.

Now that Python sorting provides key-functions, this technique is not often needed.

Comparison Functions
====================

Unlike key functions that return an absolute value for sorting, a comparison
function computes the relative ordering for two inputs.

For example, a `balance scale
<https://upload.wikimedia.org/wikipedia/commons/1/17/Balance_à_tabac_1850.JPG>`_
compares two samples giving a relative ordering: lighter, equal, or heavier.
Likewise, a comparison function such as ``cmp(a, b)`` will return a negative
value for less-than, zero if the inputs are equal, or a positive value for
greater-than.

It is common to encounter comparison functions when translating algorithms from
other languages.  Also, some libraries provide comparison functions as part of
their API.  For example, :func:`locale.strcoll` is a comparison function.

To accommodate those situations, Python provides
:class:`functools.cmp_to_key` to wrap the comparison function
to make it usable as a key function::

    sorted(words, key=cmp_to_key(strcoll))  # locale-aware sort order

Odds and Ends
=============

* For locale aware sorting, use :func:`locale.strxfrm` for a key function or
  :func:`locale.strcoll` for a comparison function.  This is necessary
  because "alphabetical" sort orderings can vary across cultures even
  if the underlying alphabet is the same.

* The *reverse* parameter still maintains sort stability (so that records with
  equal keys retain the original order). Interestingly, that effect can be
  simulated without the parameter by using the builtin :func:`reversed` function
  twice:

  .. doctest::

    >>> data = [('red', 1), ('blue', 1), ('red', 2), ('blue', 2)]
    >>> standard_way = sorted(data, key=itemgetter(0), reverse=True)
    >>> double_reversed = list(reversed(sorted(reversed(data), key=itemgetter(0))))
    >>> assert standard_way == double_reversed
    >>> standard_way
    [('red', 1), ('red', 2), ('blue', 1), ('blue', 2)]

* The sort routines use ``<`` when making comparisons
  between two objects. So, it is easy to add a standard sort order to a class by
  defining an :meth:`~object.__lt__` method:

  .. doctest::

    >>> Student.__lt__ = lambda self, other: self.age < other.age
    >>> sorted(student_objects)
    [('dave', 'B', 10), ('jane', 'B', 12), ('john', 'A', 15)]

  However, note that ``<`` can fall back to using :meth:`~object.__gt__` if
  :meth:`~object.__lt__` is not implemented (see :func:`object.__lt__`).

* Key functions need not depend directly on the objects being sorted. A key
  function can also access external resources. For instance, if the student grades
  are stored in a dictionary, they can be used to sort a separate list of student
  names:

  .. doctest::

    >>> students = ['dave', 'john', 'jane']
    >>> newgrades = {'john': 'F', 'jane':'A', 'dave': 'C'}
    >>> sorted(students, key=newgrades.__getitem__)
    ['jane', 'dave', 'john']
