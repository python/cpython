
:mod:`collections` --- High-performance container datatypes
===========================================================

.. module:: collections
   :synopsis: High-performance datatypes
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>


.. versionadded:: 2.4

This module implements high-performance container datatypes.  Currently,
there are two datatypes, :class:`deque` and :class:`defaultdict`, and
one datatype factory function, :func:`namedtuple`. Python already
includes built-in containers, :class:`dict`, :class:`list`,
:class:`set`, and :class:`tuple`. In addition, the optional :mod:`bsddb`
module has a :meth:`bsddb.btopen` method that can be used to create in-memory
or file based ordered dictionaries with string keys.

Future editions of the standard library may include balanced trees and
ordered dictionaries.

.. versionchanged:: 2.5
   Added :class:`defaultdict`.

.. versionchanged:: 2.6
   Added :func:`namedtuple`.


.. _deque-objects:

:class:`deque` objects
----------------------


.. class:: deque([iterable[, maxlen]])

   Returns a new deque object initialized left-to-right (using :meth:`append`) with
   data from *iterable*.  If *iterable* is not specified, the new deque is empty.

   Deques are a generalization of stacks and queues (the name is pronounced "deck"
   and is short for "double-ended queue").  Deques support thread-safe, memory
   efficient appends and pops from either side of the deque with approximately the
   same O(1) performance in either direction.

   Though :class:`list` objects support similar operations, they are optimized for
   fast fixed-length operations and incur O(n) memory movement costs for
   ``pop(0)`` and ``insert(0, v)`` operations which change both the size and
   position of the underlying data representation.

   .. versionadded:: 2.4

   If *maxlen* is not specified or is *None*, deques may grow to an
   arbitrary length.  Otherwise, the deque is bounded to the specified maximum
   length.  Once a bounded length deque is full, when new items are added, a
   corresponding number of items are discarded from the opposite end.  Bounded
   length deques provide functionality similar to the ``tail`` filter in
   Unix. They are also useful for tracking transactions and other pools of data
   where only the most recent activity is of interest.

   .. versionchanged:: 2.6
      Added *maxlen*

Deque objects support the following methods:


.. method:: deque.append(x)

   Add *x* to the right side of the deque.


.. method:: deque.appendleft(x)

   Add *x* to the left side of the deque.


.. method:: deque.clear()

   Remove all elements from the deque leaving it with length 0.


.. method:: deque.extend(iterable)

   Extend the right side of the deque by appending elements from the iterable
   argument.


.. method:: deque.extendleft(iterable)

   Extend the left side of the deque by appending elements from *iterable*.  Note,
   the series of left appends results in reversing the order of elements in the
   iterable argument.


.. method:: deque.pop()

   Remove and return an element from the right side of the deque. If no elements
   are present, raises an :exc:`IndexError`.


.. method:: deque.popleft()

   Remove and return an element from the left side of the deque. If no elements are
   present, raises an :exc:`IndexError`.


.. method:: deque.remove(value)

   Removed the first occurrence of *value*.  If not found, raises a
   :exc:`ValueError`.

   .. versionadded:: 2.5


.. method:: deque.rotate(n)

   Rotate the deque *n* steps to the right.  If *n* is negative, rotate to the
   left.  Rotating one step to the right is equivalent to:
   ``d.appendleft(d.pop())``.

In addition to the above, deques support iteration, pickling, ``len(d)``,
``reversed(d)``, ``copy.copy(d)``, ``copy.deepcopy(d)``, membership testing with
the :keyword:`in` operator, and subscript references such as ``d[-1]``.

Example::

   >>> from collections import deque
   >>> d = deque('ghi')                 # make a new deque with three items
   >>> for elem in d:                   # iterate over the deque's elements
   ...     print elem.upper()	
   G
   H
   I

   >>> d.append('j')                    # add a new entry to the right side
   >>> d.appendleft('f')                # add a new entry to the left side
   >>> d                                # show the representation of the deque
   deque(['f', 'g', 'h', 'i', 'j'])

   >>> d.pop()                          # return and remove the rightmost item
   'j'
   >>> d.popleft()                      # return and remove the leftmost item
   'f'
   >>> list(d)                          # list the contents of the deque
   ['g', 'h', 'i']
   >>> d[0]                             # peek at leftmost item
   'g'
   >>> d[-1]                            # peek at rightmost item
   'i'

   >>> list(reversed(d))                # list the contents of a deque in reverse
   ['i', 'h', 'g']
   >>> 'h' in d                         # search the deque
   True
   >>> d.extend('jkl')                  # add multiple elements at once
   >>> d
   deque(['g', 'h', 'i', 'j', 'k', 'l'])
   >>> d.rotate(1)                      # right rotation
   >>> d
   deque(['l', 'g', 'h', 'i', 'j', 'k'])
   >>> d.rotate(-1)                     # left rotation
   >>> d
   deque(['g', 'h', 'i', 'j', 'k', 'l'])

   >>> deque(reversed(d))               # make a new deque in reverse order
   deque(['l', 'k', 'j', 'i', 'h', 'g'])
   >>> d.clear()                        # empty the deque
   >>> d.pop()                          # cannot pop from an empty deque
   Traceback (most recent call last):
     File "<pyshell#6>", line 1, in -toplevel-
       d.pop()
   IndexError: pop from an empty deque

   >>> d.extendleft('abc')              # extendleft() reverses the input order
   >>> d
   deque(['c', 'b', 'a'])


.. _deque-recipes:

:class:`deque` Recipes
^^^^^^^^^^^^^^^^^^^^^^

This section shows various approaches to working with deques.

The :meth:`rotate` method provides a way to implement :class:`deque` slicing and
deletion.  For example, a pure python implementation of ``del d[n]`` relies on
the :meth:`rotate` method to position elements to be popped::

   def delete_nth(d, n):
       d.rotate(-n)
       d.popleft()
       d.rotate(n)

To implement :class:`deque` slicing, use a similar approach applying
:meth:`rotate` to bring a target element to the left side of the deque. Remove
old entries with :meth:`popleft`, add new entries with :meth:`extend`, and then
reverse the rotation.
With minor variations on that approach, it is easy to implement Forth style
stack manipulations such as ``dup``, ``drop``, ``swap``, ``over``, ``pick``,
``rot``, and ``roll``.

Multi-pass data reduction algorithms can be succinctly expressed and efficiently
coded by extracting elements with multiple calls to :meth:`popleft`, applying
a reduction function, and calling :meth:`append` to add the result back to the
deque.

For example, building a balanced binary tree of nested lists entails reducing
two adjacent nodes into one by grouping them in a list::

   >>> def maketree(iterable):
   ...     d = deque(iterable)
   ...     while len(d) > 1:
   ...         pair = [d.popleft(), d.popleft()]
   ...         d.append(pair)
   ...     return list(d)
   ...
   >>> print maketree('abcdefgh')
   [[[['a', 'b'], ['c', 'd']], [['e', 'f'], ['g', 'h']]]]

Bounded length deques provide functionality similar to the ``tail`` filter
in Unix::

   def tail(filename, n=10):
       'Return the last n lines of a file'
       return deque(open(filename), n)

.. _defaultdict-objects:

:class:`defaultdict` objects
----------------------------


.. class:: defaultdict([default_factory[, ...]])

   Returns a new dictionary-like object.  :class:`defaultdict` is a subclass of the
   builtin :class:`dict` class.  It overrides one method and adds one writable
   instance variable.  The remaining functionality is the same as for the
   :class:`dict` class and is not documented here.

   The first argument provides the initial value for the :attr:`default_factory`
   attribute; it defaults to ``None``. All remaining arguments are treated the same
   as if they were passed to the :class:`dict` constructor, including keyword
   arguments.

   .. versionadded:: 2.5

:class:`defaultdict` objects support the following method in addition to the
standard :class:`dict` operations:


.. method:: defaultdict.__missing__(key)

   If the :attr:`default_factory` attribute is ``None``, this raises an
   :exc:`KeyError` exception with the *key* as argument.

   If :attr:`default_factory` is not ``None``, it is called without arguments to
   provide a default value for the given *key*, this value is inserted in the
   dictionary for the *key*, and returned.

   If calling :attr:`default_factory` raises an exception this exception is
   propagated unchanged.

   This method is called by the :meth:`__getitem__` method of the :class:`dict`
   class when the requested key is not found; whatever it returns or raises is then
   returned or raised by :meth:`__getitem__`.

:class:`defaultdict` objects support the following instance variable:


.. attribute:: defaultdict.default_factory

   This attribute is used by the :meth:`__missing__` method; it is initialized from
   the first argument to the constructor, if present, or to ``None``,  if absent.


.. _defaultdict-examples:

:class:`defaultdict` Examples
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Using :class:`list` as the :attr:`default_factory`, it is easy to group a
sequence of key-value pairs into a dictionary of lists::

   >>> s = [('yellow', 1), ('blue', 2), ('yellow', 3), ('blue', 4), ('red', 1)]
   >>> d = defaultdict(list)
   >>> for k, v in s:
   ...     d[k].append(v)
   ...
   >>> d.items()
   [('blue', [2, 4]), ('red', [1]), ('yellow', [1, 3])]

When each key is encountered for the first time, it is not already in the
mapping; so an entry is automatically created using the :attr:`default_factory`
function which returns an empty :class:`list`.  The :meth:`list.append`
operation then attaches the value to the new list.  When keys are encountered
again, the look-up proceeds normally (returning the list for that key) and the
:meth:`list.append` operation adds another value to the list. This technique is
simpler and faster than an equivalent technique using :meth:`dict.setdefault`::

   >>> d = {}
   >>> for k, v in s:
   ...     d.setdefault(k, []).append(v)
   ...
   >>> d.items()
   [('blue', [2, 4]), ('red', [1]), ('yellow', [1, 3])]

Setting the :attr:`default_factory` to :class:`int` makes the
:class:`defaultdict` useful for counting (like a bag or multiset in other
languages)::

   >>> s = 'mississippi'
   >>> d = defaultdict(int)
   >>> for k in s:
   ...     d[k] += 1
   ...
   >>> d.items()
   [('i', 4), ('p', 2), ('s', 4), ('m', 1)]

When a letter is first encountered, it is missing from the mapping, so the
:attr:`default_factory` function calls :func:`int` to supply a default count of
zero.  The increment operation then builds up the count for each letter.

The function :func:`int` which always returns zero is just a special case of
constant functions.  A faster and more flexible way to create constant functions
is to use :func:`itertools.repeat` which can supply any constant value (not just
zero)::

   >>> def constant_factory(value):
   ...     return itertools.repeat(value).next
   >>> d = defaultdict(constant_factory('<missing>'))
   >>> d.update(name='John', action='ran')
   >>> '%(name)s %(action)s to %(object)s' % d
   'John ran to <missing>'

Setting the :attr:`default_factory` to :class:`set` makes the
:class:`defaultdict` useful for building a dictionary of sets::

   >>> s = [('red', 1), ('blue', 2), ('red', 3), ('blue', 4), ('red', 1), ('blue', 4)]
   >>> d = defaultdict(set)
   >>> for k, v in s:
   ...     d[k].add(v)
   ...
   >>> d.items()
   [('blue', set([2, 4])), ('red', set([1, 3]))]


.. _named-tuple-factory:

:func:`namedtuple` Factory Function for Tuples with Named Fields
-----------------------------------------------------------------

Named tuples assign meaning to each position in a tuple and allow for more readable,
self-documenting code.  They can be used wherever regular tuples are used, and
they add the ability to access fields by name instead of position index.

.. function:: namedtuple(typename, fieldnames, [verbose])

   Returns a new tuple subclass named *typename*.  The new subclass is used to
   create tuple-like objects that have fields accessable by attribute lookup as
   well as being indexable and iterable.  Instances of the subclass also have a
   helpful docstring (with typename and fieldnames) and a helpful :meth:`__repr__`
   method which lists the tuple contents in a ``name=value`` format.

   The *fieldnames* are a single string with each fieldname separated by whitespace
   and/or commas (for example 'x y' or 'x, y').  Alternatively, the *fieldnames*
   can be specified as a list of strings (such as ['x', 'y']).

   Any valid Python identifier may be used for a fieldname except for names
   starting and ending with double underscores.  Valid identifiers consist of
   letters, digits, and underscores but do not start with a digit and cannot be
   a :mod:`keyword` such as *class*, *for*, *return*, *global*, *pass*, *print*,
   or *raise*.

   If *verbose* is true, will print the class definition.

   Named tuple instances do not have per-instance dictionaries, so they are
   lightweight and require no more memory than regular tuples.

   .. versionadded:: 2.6

Example::

   >>> Point = namedtuple('Point', 'x y', verbose=True)
   class Point(tuple):
           'Point(x, y)'
           __slots__ = ()
           __fields__ = ('x', 'y')
           def __new__(cls, x, y):
               return tuple.__new__(cls, (x, y))
           def __repr__(self):
               return 'Point(x=%r, y=%r)' % self
           def __asdict__(self):
               'Return a new dict mapping field names to their values'
               return dict(zip(('x', 'y'), self))
           def __replace__(self, field, value):
               'Return a new Point object replacing specified fields with new values'
               return Point(**dict(self.__asdict__().items() + kwds.items()))
           x = property(itemgetter(0))
           y = property(itemgetter(1))

   >>> p = Point(11, y=22)     # instantiate with positional or keyword arguments
   >>> p[0] + p[1]             # indexable like the regular tuple (11, 22)
   33
   >>> x, y = p                # unpack like a regular tuple
   >>> x, y
   (11, 22)
   >>> p.x + p.y               # fields also accessable by name
   33
   >>> p                       # readable __repr__ with a name=value style
   Point(x=11, y=22)

Named tuples are especially useful for assigning field names to result tuples returned
by the :mod:`csv` or :mod:`sqlite3` modules::

   EmployeeRecord = namedtuple('EmployeeRecord', 'name, age, title, department, paygrade')

   from itertools import starmap
   import csv
   for emp in starmap(EmployeeRecord, csv.reader(open("employees.csv", "rb"))):
       print emp.name, emp.title

   import sqlite3
   conn = sqlite3.connect('/companydata')
   cursor = conn.cursor()
   cursor.execute('SELECT name, age, title, department, paygrade FROM employees')
   for emp in starmap(EmployeeRecord, cursor.fetchall()):
       print emp.name, emp.title

When casting a single record to a named tuple, use the star-operator [#]_ to unpack
the values::

   >>> t = [11, 22]
   >>> Point(*t)               # the star-operator unpacks any iterable object
   Point(x=11, y=22)

When casting a dictionary to a named tuple, use the double-star-operator::

   >>> d = {'x': 11, 'y': 22}
   >>> Point(**d)
   Point(x=11, y=22)

In addition to the methods inherited from tuples, named tuples support
two additonal methods and a read-only attribute.

.. method:: somenamedtuple.__asdict__()

   Return a new dict which maps field names to their corresponding values:

::

      >>> p.__asdict__()
      {'x': 11, 'y': 22}
      
.. method:: somenamedtuple.__replace__(kwargs)

   Return a new instance of the named tuple replacing specified fields with new values:

::

      >>> p = Point(x=11, y=22)
      >>> p.__replace__(x=33)
      Point(x=33, y=22)

      >>> for recordnum, record in inventory:
      ...     inventory[recordnum] = record.replace('total', record.price * record.quantity)

.. attribute:: somenamedtuple.__fields__

   Return a tuple of strings listing the field names.  This is useful for introspection
   and for creating new named tuple types from existing named tuples.

::

      >>> p.__fields__                                  # view the field names
      ('x', 'y')

      >>> Color = namedtuple('Color', 'red green blue')
      >>> Pixel = namedtuple('Pixel', Point.__fields__ + Color.__fields__)
      >>> Pixel(11, 22, 128, 255, 0)
      Pixel(x=11, y=22, red=128, green=255, blue=0)'

Since a named tuple is a regular Python class, it is easy to add or change
functionality.  For example, the display format can be changed by overriding
the :meth:`__repr__` method:

::

    >>> Point = namedtuple('Point', 'x y')
    >>> Point.__repr__ = lambda self: 'Point(%.3f, %.3f)' % self
    >>> Point(x=10, y=20)
    Point(10.000, 20.000)

.. rubric:: Footnotes

.. [#] For information on the star-operator see
   :ref:`tut-unpacking-arguments` and :ref:`calls`.
