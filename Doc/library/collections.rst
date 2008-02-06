
:mod:`collections` --- High-performance container datatypes
===========================================================

.. module:: collections
   :synopsis: High-performance datatypes
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>


This module implements high-performance container datatypes.  Currently,
there are two datatypes, :class:`deque` and :class:`defaultdict`, and
one datatype factory function, :func:`namedtuple`. 

The specialized containers provided in this module provide alternatives
to Python's general purpose built-in containers, :class:`dict`, 
:class:`list`, :class:`set`, and :class:`tuple`.

Besides the containers provided here, the optional :mod:`bsddb`
module offers the ability to create in-memory or file based ordered 
dictionaries with string keys using the :meth:`bsddb.btopen` method.

In addition to containers, the collections module provides some ABCs
(abstract base classes) that can be used to test whether a class 
provides a particular interface, for example, is it hashable or
a mapping. 

ABCs - abstract base classes
----------------------------

The collections module offers the following ABCs:

=====================================  ================================================================================
ABC                                    Notes
=====================================  ================================================================================
:class:`collections.Container`         Defines ``__contains__()``
:class:`collections.Hashable`          Defines ``__hash__()``
:class:`collections.Iterable`          Defines ``__iter__()``
:class:`collections.Iterator`          Derived from :class:`Iterable` and in
                                       addition defines ``__next__()``
:class:`collections.Sized`             Defines ``__len__()``
:class:`collections.Mapping`           Derived from :class:`Container`,
                                       :class:`Iterable`,
                                       and :class:`Sized`, and in addition
                                       defines ``__getitem__()``, ``get()``,
                                       ``__contains__()``, ``__len__()``,
                                       ``__eq__()``, ``__ne__()``,
                                       ``__iter__()``, ``keys()``,
                                       ``items()``, and ``values()``
:class:`collections.MutableMapping`    Derived from :class:`Mapping`
:class:`collections.Sequence`          Derived from :class:`Container`,
                                       :class:`Iterable`, and :class:`Sized`,
                                       and in addition defines
                                       ``__getitem__()``
:class:`collections.MutableSequence`   Derived from :class:`Sequence`
:class:`collections.Set`               Derived from :class:`Container`,
                                       :class:`Iterable`, and :class:`Sized`,
                                       add in addition defines
				       ``__le__()``, ``__lt__()``,
                                       ``__eq__()``, ``__and__()``,
                                       ``__or__()``, ``__sub__()``,
                                       ``__xor__()``, and ``isdisjoint()``,
:class:`collections.MutableSet`        Derived from :class:`Set` and in
                                       addition defines ``add()``,
                                       ``clear()``, ``discard()``, ``pop()``,
                                       ``remove()``, ``__ior__()``,
                                       ``__iand__()``, ``__ixor__()``, and
                                       ``__isub__()``
=====================================  ================================================================================

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
         'Alternate set implementation favoring space over speed'
         def __init__(self, iterable):
	     self.elements = list(set(iterable))
         def __iter__(self):
             return iter(self.elements)
         def __contains__(self, value):
             return value in self.elements
         def __len__(self):
             return len(self.elements)

    s1 = ListBasedSet('abcdef')
    s2 = ListBasedSet('defghi')
    overlap = s1 & s2            # The __and__() method is supported automatically


(For more about ABCs, see the :mod:`abc` module and :pep:`3119`.)



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


   If *maxlen* is not specified or is *None*, deques may grow to an
   arbitrary length.  Otherwise, the deque is bounded to the specified maximum
   length.  Once a bounded length deque is full, when new items are added, a
   corresponding number of items are discarded from the opposite end.  Bounded
   length deques provide functionality similar to the ``tail`` filter in
   Unix. They are also useful for tracking transactions and other pools of data
   where only the most recent activity is of interest.


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
   ...     print(elem.upper())
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
   >>> print(maketree('abcdefgh'))
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
is to use a lambda function which can supply any constant value (not just
zero)::

   >>> def constant_factory(value):
   ...     return lambda: value
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
----------------------------------------------------------------

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
   and/or commas, for example ``'x y'`` or ``'x, y'``.  Alternatively, *fieldnames*
   can be a sequence of strings such as ``['x', 'y']``.

   Any valid Python identifier may be used for a fieldname except for names
   starting with an underscore.  Valid identifiers consist of letters, digits,
   and underscores but do not start with a digit or underscore and cannot be
   a :mod:`keyword` such as *class*, *for*, *return*, *global*, *pass*,
   or *raise*.

   If *verbose* is true, the class definition is printed just before being built.

   Named tuple instances do not have per-instance dictionaries, so they are
   lightweight and require no more memory than regular tuples.

Example::

   >>> Point = namedtuple('Point', 'x y', verbose=True)
   class Point(tuple):
           'Point(x, y)'

           __slots__ = ()

           _fields = ('x', 'y')

           def __new__(cls, x, y):
               return tuple.__new__(cls, (x, y))

           @classmethod
           def _make(cls, iterable):
               'Make a new Point object from a sequence or iterable'
               result = tuple.__new__(cls, iterable)
               if len(result) != 2:
                   raise TypeError('Expected 2 arguments, got %d' % len(result))
               return result

           def __repr__(self):
               return 'Point(x=%r, y=%r)' % self

           def _asdict(t):
               'Return a new dict which maps field names to their values'
               return {'x': t[0], 'y': t[1]}

           def _replace(self, **kwds):
               'Return a new Point object replacing specified fields with new values'
               result = self._make(map(kwds.pop, ('x', 'y'), self))
               if kwds:
                   raise ValueError('Got unexpected field names: %r' % kwds.keys())
               return result

           x = property(itemgetter(0))
           y = property(itemgetter(1))

   >>> p = Point(11, y=22)     # instantiate with positional or keyword arguments
   >>> p[0] + p[1]             # indexable like the plain tuple (11, 22)
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

   import csv
   for emp in map(EmployeeRecord._make, csv.reader(open("employees.csv", "rb"))):
       print(emp.name, emp.title)

   import sqlite3
   conn = sqlite3.connect('/companydata')
   cursor = conn.cursor()
   cursor.execute('SELECT name, age, title, department, paygrade FROM employees')
   for emp in map(EmployeeRecord._make, cursor.fetchall()):
       print(emp.name, emp.title)

In addition to the methods inherited from tuples, named tuples support
three additional methods and one attribute.  To prevent conflicts with
field names, the method and attribute names start with an underscore.

.. method:: somenamedtuple._make(iterable)

   Class method that makes a new instance from an existing sequence or iterable.

::

      >>> t = [11, 22]
      >>> Point._make(t)
      Point(x=11, y=22)

.. method:: somenamedtuple._asdict()

   Return a new dict which maps field names to their corresponding values:

::

      >>> p._asdict()
      {'x': 11, 'y': 22}
      
.. method:: somenamedtuple._replace(kwargs)

   Return a new instance of the named tuple replacing specified fields with new values:

::

      >>> p = Point(x=11, y=22)
      >>> p._replace(x=33)
      Point(x=33, y=22)

      >>> for partnum, record in inventory.items():
      ...     inventory[partnum] = record._replace(price=newprices[partnum], timestamp=time.now())

.. attribute:: somenamedtuple._fields

   Tuple of strings listing the field names.  Useful for introspection
   and for creating new named tuple types from existing named tuples.

::

      >>> p._fields            # view the field names
      ('x', 'y')

      >>> Color = namedtuple('Color', 'red green blue')
      >>> Pixel = namedtuple('Pixel', Point._fields + Color._fields)
      >>> Pixel(11, 22, 128, 255, 0)
      Pixel(x=11, y=22, red=128, green=255, blue=0)

To retrieve a field whose name is stored in a string, use the :func:`getattr`
function::

    >>> getattr(p, 'x')
    11

To convert a dictionary to a named tuple, use the double-star-operator [#]_::

   >>> d = {'x': 11, 'y': 22}
   >>> Point(**d)
   Point(x=11, y=22)

Since a named tuple is a regular Python class, it is easy to add or change
functionality with a subclass.  Here is how to add a calculated field and
a fixed-width print format::

    >>> class Point(namedtuple('Point', 'x y')):
    ...     __slots__ = ()
    ...     @property
    ...     def hypot(self):
    ...         return (self.x ** 2 + self.y ** 2) ** 0.5
    ...     def __str__(self):
    ...         return 'Point: x=%6.3f  y=%6.3f  hypot=%6.3f' % (self.x, self.y, self.hypot)

    >>> for p in Point(3, 4), Point(14, 5/7.):
    ...     print(p)

    Point: x= 3.000  y= 4.000  hypot= 5.000
    Point: x=14.000  y= 0.714  hypot=14.018

The subclass shown above sets ``__slots__`` to an empty tuple.  This keeps
keep memory requirements low by preventing the creation of instance dictionaries.


Subclassing is not useful for adding new, stored fields.  Instead, simply
create a new named tuple type from the :attr:`_fields` attribute::

    >>> Point3D = namedtuple('Point3D', Point._fields + ('z',))

Default values can be implemented by using :meth:`_replace` to
customize a prototype instance::

    >>> Account = namedtuple('Account', 'owner balance transaction_count')
    >>> default_account = Account('<owner name>', 0.0, 0)
    >>> johns_account = default_account._replace(owner='John')

.. rubric:: Footnotes

.. [#] For information on the double-star-operator see
   :ref:`tut-unpacking-arguments` and :ref:`calls`.



:class:`UserDict` objects
-------------------------

The class, :class:`UserDict` acts as a wrapper around dictionary objects.  
The need for this class has been partially supplanted by the ability to 
subclass directly from :class:`dict`; however, this class can be easier
to work with because the underlying dictionary is accessible as an
attribute.

.. class:: UserDict([initialdata])

   Class that simulates a dictionary.  The instance's contents are kept in a
   regular dictionary, which is accessible via the :attr:`data` attribute of
   :class:`UserDict` instances.  If *initialdata* is provided, :attr:`data` is
   initialized with its contents; note that a reference to *initialdata* will not
   be kept, allowing it be used for other purposes.

In addition to supporting the methods and operations of mappings, 
:class:`UserDict` instances provide the following attribute:

.. attribute:: UserDict.data

   A real dictionary used to store the contents of the :class:`UserDict` class.
