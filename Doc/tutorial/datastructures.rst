.. _tut-structures:

***************
Data Structures
***************

This chapter describes some things you've learned about already in more detail,
and adds some new things as well.


.. _tut-morelists:

More on Lists
=============

The list data type has some more methods.  Here are all of the methods of list
objects:


.. method:: list.append(x)
   :noindex:

   Add an item to the end of the list; equivalent to ``a[len(a):] = [x]``.


.. method:: list.extend(L)
   :noindex:

   Extend the list by appending all the items in the given list; equivalent to
   ``a[len(a):] = L``.


.. method:: list.insert(i, x)
   :noindex:

   Insert an item at a given position.  The first argument is the index of the
   element before which to insert, so ``a.insert(0, x)`` inserts at the front of
   the list, and ``a.insert(len(a), x)`` is equivalent to ``a.append(x)``.


.. method:: list.remove(x)
   :noindex:

   Remove the first item from the list whose value is *x*. It is an error if there
   is no such item.


.. method:: list.pop([i])
   :noindex:

   Remove the item at the given position in the list, and return it.  If no index
   is specified, ``a.pop()`` removes and returns the last item in the list.  (The
   square brackets around the *i* in the method signature denote that the parameter
   is optional, not that you should type square brackets at that position.  You
   will see this notation frequently in the Python Library Reference.)


.. method:: list.index(x)
   :noindex:

   Return the index in the list of the first item whose value is *x*. It is an
   error if there is no such item.


.. method:: list.count(x)
   :noindex:

   Return the number of times *x* appears in the list.


.. method:: list.sort()
   :noindex:

   Sort the items of the list, in place.


.. method:: list.reverse()
   :noindex:

   Reverse the elements of the list, in place.

An example that uses most of the list methods::

   >>> a = [66.25, 333, 333, 1, 1234.5]
   >>> print a.count(333), a.count(66.25), a.count('x')
   2 1 0
   >>> a.insert(2, -1)
   >>> a.append(333)
   >>> a
   [66.25, 333, -1, 333, 1, 1234.5, 333]
   >>> a.index(333)
   1
   >>> a.remove(333)
   >>> a
   [66.25, -1, 333, 1, 1234.5, 333]
   >>> a.reverse()
   >>> a
   [333, 1234.5, 1, 333, -1, 66.25]
   >>> a.sort()
   >>> a
   [-1, 1, 66.25, 333, 333, 1234.5]


.. _tut-lists-as-stacks:

Using Lists as Stacks
---------------------

.. sectionauthor:: Ka-Ping Yee <ping@lfw.org>


The list methods make it very easy to use a list as a stack, where the last
element added is the first element retrieved ("last-in, first-out").  To add an
item to the top of the stack, use :meth:`append`.  To retrieve an item from the
top of the stack, use :meth:`pop` without an explicit index.  For example::

   >>> stack = [3, 4, 5]
   >>> stack.append(6)
   >>> stack.append(7)
   >>> stack
   [3, 4, 5, 6, 7]
   >>> stack.pop()
   7
   >>> stack
   [3, 4, 5, 6]
   >>> stack.pop()
   6
   >>> stack.pop()
   5
   >>> stack
   [3, 4]


.. _tut-lists-as-queues:

Using Lists as Queues
---------------------

.. sectionauthor:: Ka-Ping Yee <ping@lfw.org>

It is also possible to use a list as a queue, where the first element added is
the first element retrieved ("first-in, first-out"); however, lists are not
efficient for this purpose.  While appends and pops from the end of list are
fast, doing inserts or pops from the beginning of a list is slow (because all
of the other elements have to be shifted by one).

To implement a queue, use :class:`collections.deque` which was designed to
have fast appends and pops from both ends.  For example::

   >>> from collections import deque
   >>> queue = deque(["Eric", "John", "Michael"])
   >>> queue.append("Terry")           # Terry arrives
   >>> queue.append("Graham")          # Graham arrives
   >>> queue.popleft()                 # The first to arrive now leaves
   'Eric'
   >>> queue.popleft()                 # The second to arrive now leaves
   'John'
   >>> queue                           # Remaining queue in order of arrival
   deque(['Michael', 'Terry', 'Graham'])


.. _tut-functional:

Functional Programming Tools
----------------------------

There are three built-in functions that are very useful when used with lists:
:func:`filter`, :func:`map`, and :func:`reduce`.

``filter(function, sequence)`` returns a sequence consisting of those items from
the sequence for which ``function(item)`` is true. If *sequence* is a
:class:`string` or :class:`tuple`, the result will be of the same type;
otherwise, it is always a :class:`list`. For example, to compute a sequence of
numbers not divisible by 2 and 3::

   >>> def f(x): return x % 2 != 0 and x % 3 != 0
   ...
   >>> filter(f, range(2, 25))
   [5, 7, 11, 13, 17, 19, 23]

``map(function, sequence)`` calls ``function(item)`` for each of the sequence's
items and returns a list of the return values.  For example, to compute some
cubes::

   >>> def cube(x): return x*x*x
   ...
   >>> map(cube, range(1, 11))
   [1, 8, 27, 64, 125, 216, 343, 512, 729, 1000]

More than one sequence may be passed; the function must then have as many
arguments as there are sequences and is called with the corresponding item from
each sequence (or ``None`` if some sequence is shorter than another).  For
example::

   >>> seq = range(8)
   >>> def add(x, y): return x+y
   ...
   >>> map(add, seq, seq)
   [0, 2, 4, 6, 8, 10, 12, 14]

``reduce(function, sequence)`` returns a single value constructed by calling the
binary function *function* on the first two items of the sequence, then on the
result and the next item, and so on.  For example, to compute the sum of the
numbers 1 through 10::

   >>> def add(x,y): return x+y
   ...
   >>> reduce(add, range(1, 11))
   55

If there's only one item in the sequence, its value is returned; if the sequence
is empty, an exception is raised.

A third argument can be passed to indicate the starting value.  In this case the
starting value is returned for an empty sequence, and the function is first
applied to the starting value and the first sequence item, then to the result
and the next item, and so on.  For example, ::

   >>> def sum(seq):
   ...     def add(x,y): return x+y
   ...     return reduce(add, seq, 0)
   ...
   >>> sum(range(1, 11))
   55
   >>> sum([])
   0

Don't use this example's definition of :func:`sum`: since summing numbers is
such a common need, a built-in function ``sum(sequence)`` is already provided,
and works exactly like this.

.. versionadded:: 2.3


List Comprehensions
-------------------

List comprehensions provide a concise way to create lists.
Common applications are to make new lists where each element is the result of
some operations applied to each member of another sequence or iterable, or to
create a subsequence of those elements that satisfy a certain condition.

For example, assume we want to create a list of squares, like::

   >>> squares = []
   >>> for x in range(10):
   ...     squares.append(x**2)
   ...
   >>> squares
   [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

We can obtain the same result with::

   squares = [x**2 for x in range(10)]

This is also equivalent to ``squares = map(lambda x: x**2, range(10))``,
but it's more concise and readable.

A list comprehension consists of brackets containing an expression followed
by a :keyword:`for` clause, then zero or more :keyword:`for` or :keyword:`if`
clauses.  The result will be a new list resulting from evaluating the expression
in the context of the :keyword:`for` and :keyword:`if` clauses which follow it.
For example, this listcomp combines the elements of two lists if they are not
equal::

   >>> [(x, y) for x in [1,2,3] for y in [3,1,4] if x != y]
   [(1, 3), (1, 4), (2, 3), (2, 1), (2, 4), (3, 1), (3, 4)]

and it's equivalent to:

   >>> combs = []
   >>> for x in [1,2,3]:
   ...     for y in [3,1,4]:
   ...         if x != y:
   ...             combs.append((x, y))
   ...
   >>> combs
   [(1, 3), (1, 4), (2, 3), (2, 1), (2, 4), (3, 1), (3, 4)]

Note how the order of the :keyword:`for` and :keyword:`if` statements is the
same in both these snippets.

If the expression is a tuple (e.g. the ``(x, y)`` in the previous example),
it must be parenthesized. ::

   >>> vec = [-4, -2, 0, 2, 4]
   >>> # create a new list with the values doubled
   >>> [x*2 for x in vec]
   [-8, -4, 0, 4, 8]
   >>> # filter the list to exclude negative numbers
   >>> [x for x in vec if x >= 0]
   [0, 2, 4]
   >>> # apply a function to all the elements
   >>> [abs(x) for x in vec]
   [4, 2, 0, 2, 4]
   >>> # call a method on each element
   >>> freshfruit = ['  banana', '  loganberry ', 'passion fruit  ']
   >>> [weapon.strip() for weapon in freshfruit]
   ['banana', 'loganberry', 'passion fruit']
   >>> # create a list of 2-tuples like (number, square)
   >>> [(x, x**2) for x in range(6)]
   [(0, 0), (1, 1), (2, 4), (3, 9), (4, 16), (5, 25)]
   >>> # the tuple must be parenthesized, otherwise an error is raised
   >>> [x, x**2 for x in range(6)]
     File "<stdin>", line 1
       [x, x**2 for x in range(6)]
                  ^
   SyntaxError: invalid syntax
   >>> # flatten a list using a listcomp with two 'for'
   >>> vec = [[1,2,3], [4,5,6], [7,8,9]]
   >>> [num for elem in vec for num in elem]
   [1, 2, 3, 4, 5, 6, 7, 8, 9]

List comprehensions can contain complex expressions and nested functions::

   >>> from math import pi
   >>> [str(round(pi, i)) for i in range(1, 6)]
   ['3.1', '3.14', '3.142', '3.1416', '3.14159']


Nested List Comprehensions
''''''''''''''''''''''''''

The initial expression in a list comprehension can be any arbitrary expression,
including another list comprehension.

Consider the following example of a 3x4 matrix implemented as a list of
3 lists of length 4::

   >>> matrix = [
   ...     [1, 2, 3, 4],
   ...     [5, 6, 7, 8],
   ...     [9, 10, 11, 12],
   ... ]

The following list comprehension will transpose rows and columns::

   >>> [[row[i] for row in matrix] for i in range(4)]
   [[1, 5, 9], [2, 6, 10], [3, 7, 11], [4, 8, 12]]

As we saw in the previous section, the nested listcomp is evaluated in
the context of the :keyword:`for` that follows it, so this example is
equivalent to::

   >>> transposed = []
   >>> for i in range(4):
   ...     transposed.append([row[i] for row in matrix])
   ...
   >>> transposed
   [[1, 5, 9], [2, 6, 10], [3, 7, 11], [4, 8, 12]]

which, in turn, is the same as::

   >>> transposed = []
   >>> for i in range(4):
   ...     # the following 3 lines implement the nested listcomp
   ...     transposed_row = []
   ...     for row in matrix:
   ...         transposed_row.append(row[i])
   ...     transposed.append(transposed_row)
   ...
   >>> transposed
   [[1, 5, 9], [2, 6, 10], [3, 7, 11], [4, 8, 12]]


In the real world, you should prefer built-in functions to complex flow statements.
The :func:`zip` function would do a great job for this use case::

   >>> zip(*matrix)
   [(1, 5, 9), (2, 6, 10), (3, 7, 11), (4, 8, 12)]

See :ref:`tut-unpacking-arguments` for details on the asterisk in this line.

.. _tut-del:

The :keyword:`del` statement
============================

There is a way to remove an item from a list given its index instead of its
value: the :keyword:`del` statement.  This differs from the :meth:`pop` method
which returns a value.  The :keyword:`del` statement can also be used to remove
slices from a list or clear the entire list (which we did earlier by assignment
of an empty list to the slice).  For example::

   >>> a = [-1, 1, 66.25, 333, 333, 1234.5]
   >>> del a[0]
   >>> a
   [1, 66.25, 333, 333, 1234.5]
   >>> del a[2:4]
   >>> a
   [1, 66.25, 1234.5]
   >>> del a[:]
   >>> a
   []

:keyword:`del` can also be used to delete entire variables::

   >>> del a

Referencing the name ``a`` hereafter is an error (at least until another value
is assigned to it).  We'll find other uses for :keyword:`del` later.


.. _tut-tuples:

Tuples and Sequences
====================

We saw that lists and strings have many common properties, such as indexing and
slicing operations.  They are two examples of *sequence* data types (see
:ref:`typesseq`).  Since Python is an evolving language, other sequence data
types may be added.  There is also another standard sequence data type: the
*tuple*.

A tuple consists of a number of values separated by commas, for instance::

   >>> t = 12345, 54321, 'hello!'
   >>> t[0]
   12345
   >>> t
   (12345, 54321, 'hello!')
   >>> # Tuples may be nested:
   ... u = t, (1, 2, 3, 4, 5)
   >>> u
   ((12345, 54321, 'hello!'), (1, 2, 3, 4, 5))

As you see, on output tuples are always enclosed in parentheses, so that nested
tuples are interpreted correctly; they may be input with or without surrounding
parentheses, although often parentheses are necessary anyway (if the tuple is
part of a larger expression).

Tuples have many uses.  For example: (x, y) coordinate pairs, employee records
from a database, etc.  Tuples, like strings, are immutable: it is not possible
to assign to the individual items of a tuple (you can simulate much of the same
effect with slicing and concatenation, though).  It is also possible to create
tuples which contain mutable objects, such as lists.

A special problem is the construction of tuples containing 0 or 1 items: the
syntax has some extra quirks to accommodate these.  Empty tuples are constructed
by an empty pair of parentheses; a tuple with one item is constructed by
following a value with a comma (it is not sufficient to enclose a single value
in parentheses). Ugly, but effective.  For example::

   >>> empty = ()
   >>> singleton = 'hello',    # <-- note trailing comma
   >>> len(empty)
   0
   >>> len(singleton)
   1
   >>> singleton
   ('hello',)

The statement ``t = 12345, 54321, 'hello!'`` is an example of *tuple packing*:
the values ``12345``, ``54321`` and ``'hello!'`` are packed together in a tuple.
The reverse operation is also possible::

   >>> x, y, z = t

This is called, appropriately enough, *sequence unpacking* and works for any
sequence on the right-hand side.  Sequence unpacking requires the list of
variables on the left to have the same number of elements as the length of the
sequence.  Note that multiple assignment is really just a combination of tuple
packing and sequence unpacking.

.. XXX Add a bit on the difference between tuples and lists.


.. _tut-sets:

Sets
====

Python also includes a data type for *sets*.  A set is an unordered collection
with no duplicate elements.  Basic uses include membership testing and
eliminating duplicate entries.  Set objects also support mathematical operations
like union, intersection, difference, and symmetric difference.

Here is a brief demonstration::

   >>> basket = ['apple', 'orange', 'apple', 'pear', 'orange', 'banana']
   >>> fruit = set(basket)               # create a set without duplicates
   >>> fruit
   set(['orange', 'pear', 'apple', 'banana'])
   >>> 'orange' in fruit                 # fast membership testing
   True
   >>> 'crabgrass' in fruit
   False

   >>> # Demonstrate set operations on unique letters from two words
   ...
   >>> a = set('abracadabra')
   >>> b = set('alacazam')
   >>> a                                  # unique letters in a
   set(['a', 'r', 'b', 'c', 'd'])
   >>> a - b                              # letters in a but not in b
   set(['r', 'd', 'b'])
   >>> a | b                              # letters in either a or b
   set(['a', 'c', 'r', 'd', 'b', 'm', 'z', 'l'])
   >>> a & b                              # letters in both a and b
   set(['a', 'c'])
   >>> a ^ b                              # letters in a or b but not both
   set(['r', 'd', 'b', 'm', 'z', 'l'])


.. _tut-dictionaries:

Dictionaries
============

Another useful data type built into Python is the *dictionary* (see
:ref:`typesmapping`). Dictionaries are sometimes found in other languages as
"associative memories" or "associative arrays".  Unlike sequences, which are
indexed by a range of numbers, dictionaries are indexed by *keys*, which can be
any immutable type; strings and numbers can always be keys.  Tuples can be used
as keys if they contain only strings, numbers, or tuples; if a tuple contains
any mutable object either directly or indirectly, it cannot be used as a key.
You can't use lists as keys, since lists can be modified in place using index
assignments, slice assignments, or methods like :meth:`append` and
:meth:`extend`.

It is best to think of a dictionary as an unordered set of *key: value* pairs,
with the requirement that the keys are unique (within one dictionary). A pair of
braces creates an empty dictionary: ``{}``. Placing a comma-separated list of
key:value pairs within the braces adds initial key:value pairs to the
dictionary; this is also the way dictionaries are written on output.

The main operations on a dictionary are storing a value with some key and
extracting the value given the key.  It is also possible to delete a key:value
pair with ``del``. If you store using a key that is already in use, the old
value associated with that key is forgotten.  It is an error to extract a value
using a non-existent key.

The :meth:`keys` method of a dictionary object returns a list of all the keys
used in the dictionary, in arbitrary order (if you want it sorted, just apply
the :func:`sorted` function to it).  To check whether a single key is in the
dictionary, use the :keyword:`in` keyword.

Here is a small example using a dictionary::

   >>> tel = {'jack': 4098, 'sape': 4139}
   >>> tel['guido'] = 4127
   >>> tel
   {'sape': 4139, 'guido': 4127, 'jack': 4098}
   >>> tel['jack']
   4098
   >>> del tel['sape']
   >>> tel['irv'] = 4127
   >>> tel
   {'guido': 4127, 'irv': 4127, 'jack': 4098}
   >>> tel.keys()
   ['guido', 'irv', 'jack']
   >>> 'guido' in tel
   True

The :func:`dict` constructor builds dictionaries directly from lists of
key-value pairs stored as tuples.  When the pairs form a pattern, list
comprehensions can compactly specify the key-value list. ::

   >>> dict([('sape', 4139), ('guido', 4127), ('jack', 4098)])
   {'sape': 4139, 'jack': 4098, 'guido': 4127}
   >>> dict([(x, x**2) for x in (2, 4, 6)])     # use a list comprehension
   {2: 4, 4: 16, 6: 36}

Later in the tutorial, we will learn about Generator Expressions which are even
better suited for the task of supplying key-values pairs to the :func:`dict`
constructor.

When the keys are simple strings, it is sometimes easier to specify pairs using
keyword arguments::

   >>> dict(sape=4139, guido=4127, jack=4098)
   {'sape': 4139, 'jack': 4098, 'guido': 4127}


.. _tut-loopidioms:

Looping Techniques
==================

When looping through dictionaries, the key and corresponding value can be
retrieved at the same time using the :meth:`iteritems` method. ::

   >>> knights = {'gallahad': 'the pure', 'robin': 'the brave'}
   >>> for k, v in knights.iteritems():
   ...     print k, v
   ...
   gallahad the pure
   robin the brave

When looping through a sequence, the position index and corresponding value can
be retrieved at the same time using the :func:`enumerate` function. ::

   >>> for i, v in enumerate(['tic', 'tac', 'toe']):
   ...     print i, v
   ...
   0 tic
   1 tac
   2 toe

To loop over two or more sequences at the same time, the entries can be paired
with the :func:`zip` function. ::

   >>> questions = ['name', 'quest', 'favorite color']
   >>> answers = ['lancelot', 'the holy grail', 'blue']
   >>> for q, a in zip(questions, answers):
   ...     print 'What is your {0}?  It is {1}.'.format(q, a)
   ...
   What is your name?  It is lancelot.
   What is your quest?  It is the holy grail.
   What is your favorite color?  It is blue.

To loop over a sequence in reverse, first specify the sequence in a forward
direction and then call the :func:`reversed` function. ::

   >>> for i in reversed(xrange(1,10,2)):
   ...     print i
   ...
   9
   7
   5
   3
   1

To loop over a sequence in sorted order, use the :func:`sorted` function which
returns a new sorted list while leaving the source unaltered. ::

   >>> basket = ['apple', 'orange', 'apple', 'pear', 'orange', 'banana']
   >>> for f in sorted(set(basket)):
   ...     print f
   ...
   apple
   banana
   orange
   pear


.. _tut-conditions:

More on Conditions
==================

The conditions used in ``while`` and ``if`` statements can contain any
operators, not just comparisons.

The comparison operators ``in`` and ``not in`` check whether a value occurs
(does not occur) in a sequence.  The operators ``is`` and ``is not`` compare
whether two objects are really the same object; this only matters for mutable
objects like lists.  All comparison operators have the same priority, which is
lower than that of all numerical operators.

Comparisons can be chained.  For example, ``a < b == c`` tests whether ``a`` is
less than ``b`` and moreover ``b`` equals ``c``.

Comparisons may be combined using the Boolean operators ``and`` and ``or``, and
the outcome of a comparison (or of any other Boolean expression) may be negated
with ``not``.  These have lower priorities than comparison operators; between
them, ``not`` has the highest priority and ``or`` the lowest, so that ``A and
not B or C`` is equivalent to ``(A and (not B)) or C``. As always, parentheses
can be used to express the desired composition.

The Boolean operators ``and`` and ``or`` are so-called *short-circuit*
operators: their arguments are evaluated from left to right, and evaluation
stops as soon as the outcome is determined.  For example, if ``A`` and ``C`` are
true but ``B`` is false, ``A and B and C`` does not evaluate the expression
``C``.  When used as a general value and not as a Boolean, the return value of a
short-circuit operator is the last evaluated argument.

It is possible to assign the result of a comparison or other Boolean expression
to a variable.  For example, ::

   >>> string1, string2, string3 = '', 'Trondheim', 'Hammer Dance'
   >>> non_null = string1 or string2 or string3
   >>> non_null
   'Trondheim'

Note that in Python, unlike C, assignment cannot occur inside expressions. C
programmers may grumble about this, but it avoids a common class of problems
encountered in C programs: typing ``=`` in an expression when ``==`` was
intended.


.. _tut-comparing:

Comparing Sequences and Other Types
===================================

Sequence objects may be compared to other objects with the same sequence type.
The comparison uses *lexicographical* ordering: first the first two items are
compared, and if they differ this determines the outcome of the comparison; if
they are equal, the next two items are compared, and so on, until either
sequence is exhausted. If two items to be compared are themselves sequences of
the same type, the lexicographical comparison is carried out recursively.  If
all items of two sequences compare equal, the sequences are considered equal.
If one sequence is an initial sub-sequence of the other, the shorter sequence is
the smaller (lesser) one.  Lexicographical ordering for strings uses the ASCII
ordering for individual characters.  Some examples of comparisons between
sequences of the same type::

   (1, 2, 3)              < (1, 2, 4)
   [1, 2, 3]              < [1, 2, 4]
   'ABC' < 'C' < 'Pascal' < 'Python'
   (1, 2, 3, 4)           < (1, 2, 4)
   (1, 2)                 < (1, 2, -1)
   (1, 2, 3)             == (1.0, 2.0, 3.0)
   (1, 2, ('aa', 'ab'))   < (1, 2, ('abc', 'a'), 4)

Note that comparing objects of different types is legal.  The outcome is
deterministic but arbitrary: the types are ordered by their name. Thus, a list
is always smaller than a string, a string is always smaller than a tuple, etc.
[#]_ Mixed numeric types are compared according to their numeric value, so 0
equals 0.0, etc.


.. rubric:: Footnotes

.. [#] The rules for comparing objects of different types should not be relied upon;
   they may change in a future version of the language.

