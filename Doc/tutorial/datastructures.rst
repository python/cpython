.. _tut-structures:

***************
Data Structures
***************

This chapter describes some things you've learned about already in more detail,
and adds some new things as well.

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

This is called, appropriately enough, *sequence unpacking*. Sequence unpacking
requires the list of variables on the left to have the same number of elements
as the length of the sequence.  Note that multiple assignment is really just a
combination of tuple packing and sequence unpacking!

There is a small bit of asymmetry here:  packing multiple values always creates
a tuple, and unpacking works for any sequence.

.. % XXX Add a bit on the difference between tuples and lists.


.. _tut-morelists:

More on Lists
=============

The list data type has some more methods.  Here are all of the methods of list
objects:


.. method:: list.append(x)

   Add an item to the end of the list; equivalent to ``a[len(a):] = [x]``.


.. method:: list.extend(L)

   Extend the list by appending all the items in the given list; equivalent to
   ``a[len(a):] = L``.


.. method:: list.insert(i, x)

   Insert an item at a given position.  The first argument is the index of the
   element before which to insert, so ``a.insert(0, x)`` inserts at the front of
   the list, and ``a.insert(len(a), x)`` is equivalent to ``a.append(x)``.


.. method:: list.remove(x)

   Remove the first item from the list whose value is *x*. It is an error if there
   is no such item.


.. method:: list.pop([i])

   Remove the item at the given position in the list, and return it.  If no index
   is specified, ``a.pop()`` removes and returns the last item in the list.  (The
   square brackets around the *i* in the method signature denote that the parameter
   is optional, not that you should type square brackets at that position.  You
   will see this notation frequently in the Python Library Reference.)


.. method:: list.index(x)

   Return the index in the list of the first item whose value is *x*. It is an
   error if there is no such item.


.. method:: list.count(x)

   Return the number of times *x* appears in the list.


.. method:: list.sort()

   Sort the items of the list, in place.


.. method:: list.reverse()

   Reverse the elements of the list, in place.

An example that uses most of the list methods::

   >>> a = [66.25, 333, 333, 1, 1234.5]
   >>> print(a.count(333), a.count(66.25), a.count('x'))
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


You can also use a list conveniently as a queue, where the first element added
is the first element retrieved ("first-in, first-out").  To add an item to the
back of the queue, use :meth:`append`.  To retrieve an item from the front of
the queue, use :meth:`pop` with ``0`` as the index.  For example::

   >>> queue = ["Eric", "John", "Michael"]
   >>> queue.append("Terry")           # Terry arrives
   >>> queue.append("Graham")          # Graham arrives
   >>> queue.pop(0)
   'Eric'
   >>> queue.pop(0)
   'John'
   >>> queue
   ['Michael', 'Terry', 'Graham']


List Comprehensions
-------------------

List comprehensions provide a concise way to create lists from sequences.
Common applications are to make lists where each element is the result of
some operations applied to each member of the sequence, or to create a 
subsequence of those elements that satisfy a certain condition.


Each list comprehension consists of an expression followed by a :keyword:`for`
clause, then zero or more :keyword:`for` or :keyword:`if` clauses.  The result
will be a list resulting from evaluating the expression in the context of the
:keyword:`for` and :keyword:`if` clauses which follow it.  If the expression
would evaluate to a tuple, it must be parenthesized. 

Here we take a list of numbers and return a list of three times each number::

   >>> vec = [2, 4, 6]
   >>> [3*x for x in vec]
   [6, 12, 18]

Now we get a little fancier::

   >>> [[x, x**2] for x in vec]
   [[2, 4], [4, 16], [6, 36]]

Here we apply a method call to each item in a sequence::

   >>> freshfruit = ['  banana', '  loganberry ', 'passion fruit  ']
   >>> [weapon.strip() for weapon in freshfruit]
   ['banana', 'loganberry', 'passion fruit']

Using the :keyword:`if` clause we can filter the stream::

   >>> [3*x for x in vec if x > 3]
   [12, 18]
   >>> [3*x for x in vec if x < 2]
   []

Tuples can often be created without their parentheses, but not here::

   >>> [x, x**2 for x in vec]	# error - parens required for tuples
     File "<stdin>", line 1, in ?
       [x, x**2 for x in vec]
                  ^
   SyntaxError: invalid syntax
   >>> [(x, x**2) for x in vec]
   [(2, 4), (4, 16), (6, 36)]

Here are some nested for loops and other fancy behavior::

   >>> vec1 = [2, 4, 6]
   >>> vec2 = [4, 3, -9]
   >>> [x*y for x in vec1 for y in vec2]
   [8, 6, -18, 16, 12, -36, 24, 18, -54]
   >>> [x+y for x in vec1 for y in vec2]
   [6, 5, -7, 8, 7, -5, 10, 9, -3]
   >>> [vec1[i]*vec2[i] for i in range(len(vec1))]
   [8, 12, -54]

List comprehensions can be applied to complex expressions and nested functions::

   >>> [str(round(355/113.0, i)) for i in range(1, 6)]
   ['3.1', '3.14', '3.142', '3.1416', '3.14159']


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



.. _tut-sets:

Sets
====

Python also includes a data type for *sets*.  A set is an unordered collection
with no duplicate elements.  Basic uses include membership testing and
eliminating duplicate entries.  Set objects also support mathematical operations
like union, intersection, difference, and symmetric difference.

Curly braces or the :func:`set` function can be use to create sets. Note:
To create an empty set you have to use set(), not {}; the latter creates
an empty dictionary, a data structure that we discuss in the next section.

Here is a brief demonstration::

   >>> basket = {'apple', 'orange', 'apple', 'pear', 'orange', 'banana'}
   >>> print(basket)
   {'orange', 'bananna', 'pear', 'apple'}
   >>> fruit = ['apple', 'orange', 'apple', 'pear', 'orange', 'banana']
   >>> fruit = set(basket)               # create a set without duplicates
   >>> fruit
   {'orange', 'pear', 'apple', 'banana'}
   >>> 'orange' in fruit                 # fast membership testing
   True
   >>> 'crabgrass' in fruit
   False

   >>> # Demonstrate set operations on unique letters from two words
   ...
   >>> a = set('abracadabra')
   >>> b = set('alacazam')
   >>> a                                  # unique letters in a
   {'a', 'r', 'b', 'c', 'd'}
   >>> a - b                              # letters in a but not in b
   {'r', 'd', 'b'}
   >>> a | b                              # letters in either a or b
   {'a', 'c', 'r', 'd', 'b', 'm', 'z', 'l'}
   >>> a & b                              # letters in both a and b
   {'a', 'c'}
   >>> a ^ b                              # letters in a or b but not both
   {'r', 'd', 'b', 'm', 'z', 'l'}




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
the :meth:`sort` method to the list of keys).  To check whether a single key is
in the dictionary, use the :keyword:`in` keyword.

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
   >>> list(tel.keys())
   ['guido', 'irv', 'jack']
   >>> 'guido' in tel
   True
   >>> 'jack' not in tel
   False

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
.. % 
   Find out the right way to do these DUBOIS

Looping Techniques
==================

When looping through dictionaries, the key and corresponding value can be
retrieved at the same time using the :meth:`items` method. ::

   >>> knights = {'gallahad': 'the pure', 'robin': 'the brave'}
   >>> for k, v in knights.items():
   ...     print(k, v)
   ...
   gallahad the pure
   robin the brave

When looping through a sequence, the position index and corresponding value can
be retrieved at the same time using the :func:`enumerate` function. ::

   >>> for i, v in enumerate(['tic', 'tac', 'toe']):
   ...     print(i, v)
   ...
   0 tic
   1 tac
   2 toe

To loop over two or more sequences at the same time, the entries can be paired
with the :func:`zip` function. ::

   >>> questions = ['name', 'quest', 'favorite color']
   >>> answers = ['lancelot', 'the holy grail', 'blue']
   >>> for q, a in zip(questions, answers):
   ...     print('What is your %s?  It is %s.' % (q, a))
   ...	
   What is your name?  It is lancelot.
   What is your quest?  It is the holy grail.
   What is your favorite color?  It is blue.

To loop over a sequence in reverse, first specify the sequence in a forward
direction and then call the :func:`reversed` function. ::

   >>> for i in reversed(range(1, 10, 2)):
   ...     print(i)
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
   ...     print(f)
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

