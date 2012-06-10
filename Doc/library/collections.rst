:mod:`collections` --- Container datatypes
==========================================

.. module:: collections
    :synopsis: Container datatypes
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>

.. testsetup:: *

    from collections import *
    import itertools
    __name__ = '<doctest>'

**Source code:** :source:`Lib/collections/__init__.py`

--------------

This module implements specialized container datatypes providing alternatives to
Python's general purpose built-in containers, :class:`dict`, :class:`list`,
:class:`set`, and :class:`tuple`.

=====================   ====================================================================
:func:`namedtuple`      factory function for creating tuple subclasses with named fields
:class:`deque`          list-like container with fast appends and pops on either end
:class:`ChainMap`       dict-like class for creating a single view of multiple mappings
:class:`Counter`        dict subclass for counting hashable objects
:class:`OrderedDict`    dict subclass that remembers the order entries were added
:class:`defaultdict`    dict subclass that calls a factory function to supply missing values
:class:`UserDict`       wrapper around dictionary objects for easier dict subclassing
:class:`UserList`       wrapper around list objects for easier list subclassing
:class:`UserString`     wrapper around string objects for easier string subclassing
=====================   ====================================================================

.. versionchanged:: 3.3
    Moved :ref:`collections-abstract-base-classes` to the :mod:`collections.abc` module.
    For backwards compatibility, they continue to be visible in this module
    as well.


:class:`ChainMap` objects
-------------------------

.. versionadded:: 3.3

A :class:`ChainMap` class is provided for quickly linking a number of mappings
so they can be treated as a single unit.  It is often much faster than creating
a new dictionary and running multiple :meth:`~dict.update` calls.

The class can be used to simulate nested scopes and is useful in templating.

.. class:: ChainMap(*maps)

    A :class:`ChainMap` groups multiple dicts or other mappings together to
    create a single, updateable view.  If no *maps* are specified, a single empty
    dictionary is provided so that a new chain always has at least one mapping.

    The underlying mappings are stored in a list.  That list is public and can
    accessed or updated using the *maps* attribute.  There is no other state.

    Lookups search the underlying mappings successively until a key is found.  In
    contrast, writes, updates, and deletions only operate on the first mapping.

    A :class:`ChainMap` incorporates the underlying mappings by reference.  So, if
    one of the underlying mappings gets updated, those changes will be reflected
    in :class:`ChainMap`.

    All of the usual dictionary methods are supported.  In addition, there is a
    *maps* attribute, a method for creating new subcontexts, and a property for
    accessing all but the first mapping:

    .. attribute:: maps

        A user updateable list of mappings.  The list is ordered from
        first-searched to last-searched.  It is the only stored state and can
        be modified to change which mappings are searched.  The list should
        always contain at least one mapping.

    .. method:: new_child()

        Returns a new :class:`ChainMap` containing a new :class:`dict` followed by
        all of the maps in the current instance.  A call to ``d.new_child()`` is
        equivalent to: ``ChainMap({}, *d.maps)``.  This method is used for
        creating subcontexts that can be updated without altering values in any
        of the parent mappings.

    .. method:: parents()

        Returns a new :class:`ChainMap` containing all of the maps in the current
        instance except the first one.  This is useful for skipping the first map
        in the search.  The use-cases are similar to those for the
        :keyword:`nonlocal` keyword used in :term:`nested scopes <nested scope>`.
        The use-cases also parallel those for the builtin :func:`super` function.
        A reference to  ``d.parents`` is equivalent to: ``ChainMap(*d.maps[1:])``.

    Example of simulating Python's internal lookup chain::

        import builtins
        pylookup = ChainMap(locals(), globals(), vars(builtins))

    Example of letting user specified values take precedence over environment
    variables which in turn take precedence over default values::

        import os, argparse
        defaults = {'color': 'red', 'user': guest}
        parser = argparse.ArgumentParser()
        parser.add_argument('-u', '--user')
        parser.add_argument('-c', '--color')
        user_specified = vars(parser.parse_args())
        combined = ChainMap(user_specified, os.environ, defaults)

    Example patterns for using the :class:`ChainMap` class to simulate nested
    contexts::

        c = ChainMap()        # Create root context
        d = c.new_child()     # Create nested child context
        e = c.new_child()     # Child of c, independent from d
        e.maps[0]             # Current context dictionary -- like Python's locals()
        e.maps[-1]            # Root context -- like Python's globals()
        e.parents             # Enclosing context chain -- like Python's nonlocals

        d['x']                # Get first key in the chain of contexts
        d['x'] = 1            # Set value in current context
        del['x']              # Delete from current context
        list(d)               # All nested values
        k in d                # Check all nested values
        len(d)                # Number of nested values
        d.items()             # All nested items
        dict(d)               # Flatten into a regular dictionary

    .. seealso::

        * The `MultiContext class
            <http://svn.enthought.com/svn/enthought/CodeTools/trunk/enthought/contexts/multi_context.py>`_
            in the Enthought `CodeTools package
            <https://github.com/enthought/codetools>`_ has options to support
            writing to any mapping in the chain.

        * Django's `Context class
            <http://code.djangoproject.com/browser/django/trunk/django/template/context.py>`_
            for templating is a read-only chain of mappings.  It also features
            pushing and popping of contexts similar to the
            :meth:`~collections.ChainMap.new_child` method and the
            :meth:`~collections.ChainMap.parents` property.

        * The `Nested Contexts recipe
            <http://code.activestate.com/recipes/577434/>`_ has options to control
            whether writes and other mutations apply only to the first mapping or to
            any mapping in the chain.

        * A `greatly simplified read-only version of Chainmap
            <http://code.activestate.com/recipes/305268/>`_.


:class:`Counter` objects
------------------------

A counter tool is provided to support convenient and rapid tallies.
For example::

    >>> # Tally occurrences of words in a list
    >>> cnt = Counter()
    >>> for word in ['red', 'blue', 'red', 'green', 'blue', 'blue']:
    ...     cnt[word] += 1
    >>> cnt
    Counter({'blue': 3, 'red': 2, 'green': 1})

    >>> # Find the ten most common words in Hamlet
    >>> import re
    >>> words = re.findall('\w+', open('hamlet.txt').read().lower())
    >>> Counter(words).most_common(10)
    [('the', 1143), ('and', 966), ('to', 762), ('of', 669), ('i', 631),
     ('you', 554),  ('a', 546), ('my', 514), ('hamlet', 471), ('in', 451)]

.. class:: Counter([iterable-or-mapping])

    A :class:`Counter` is a :class:`dict` subclass for counting hashable objects.
    It is an unordered collection where elements are stored as dictionary keys
    and their counts are stored as dictionary values.  Counts are allowed to be
    any integer value including zero or negative counts.  The :class:`Counter`
    class is similar to bags or multisets in other languages.

    Elements are counted from an *iterable* or initialized from another
    *mapping* (or counter):

        >>> c = Counter()                           # a new, empty counter
        >>> c = Counter('gallahad')                 # a new counter from an iterable
        >>> c = Counter({'red': 4, 'blue': 2})      # a new counter from a mapping
        >>> c = Counter(cats=4, dogs=8)             # a new counter from keyword args

    Counter objects have a dictionary interface except that they return a zero
    count for missing items instead of raising a :exc:`KeyError`:

        >>> c = Counter(['eggs', 'ham'])
        >>> c['bacon']                              # count of a missing element is zero
        0

    Setting a count to zero does not remove an element from a counter.
    Use ``del`` to remove it entirely:

        >>> c['sausage'] = 0                        # counter entry with a zero count
        >>> del c['sausage']                        # del actually removes the entry

    .. versionadded:: 3.1


    Counter objects support three methods beyond those available for all
    dictionaries:

    .. method:: elements()

        Return an iterator over elements repeating each as many times as its
        count.  Elements are returned in arbitrary order.  If an element's count
        is less than one, :meth:`elements` will ignore it.

            >>> c = Counter(a=4, b=2, c=0, d=-2)
            >>> list(c.elements())
            ['a', 'a', 'a', 'a', 'b', 'b']

    .. method:: most_common([n])

        Return a list of the *n* most common elements and their counts from the
        most common to the least.  If *n* is not specified, :func:`most_common`
        returns *all* elements in the counter.  Elements with equal counts are
        ordered arbitrarily:

            >>> Counter('abracadabra').most_common(3)
            [('a', 5), ('r', 2), ('b', 2)]

    .. method:: subtract([iterable-or-mapping])

        Elements are subtracted from an *iterable* or from another *mapping*
        (or counter).  Like :meth:`dict.update` but subtracts counts instead
        of replacing them.  Both inputs and outputs may be zero or negative.

            >>> c = Counter(a=4, b=2, c=0, d=-2)
            >>> d = Counter(a=1, b=2, c=3, d=4)
            >>> c.subtract(d)
            Counter({'a': 3, 'b': 0, 'c': -3, 'd': -6})

        .. versionadded:: 3.2

    The usual dictionary methods are available for :class:`Counter` objects
    except for two which work differently for counters.

    .. method:: fromkeys(iterable)

        This class method is not implemented for :class:`Counter` objects.

    .. method:: update([iterable-or-mapping])

        Elements are counted from an *iterable* or added-in from another
        *mapping* (or counter).  Like :meth:`dict.update` but adds counts
        instead of replacing them.  Also, the *iterable* is expected to be a
        sequence of elements, not a sequence of ``(key, value)`` pairs.

Common patterns for working with :class:`Counter` objects::

    sum(c.values())                 # total of all counts
    c.clear()                       # reset all counts
    list(c)                         # list unique elements
    set(c)                          # convert to a set
    dict(c)                         # convert to a regular dictionary
    c.items()                       # convert to a list of (elem, cnt) pairs
    Counter(dict(list_of_pairs))    # convert from a list of (elem, cnt) pairs
    c.most_common()[:-n:-1]         # n least common elements
    +c                              # remove zero and negative counts

Several mathematical operations are provided for combining :class:`Counter`
objects to produce multisets (counters that have counts greater than zero).
Addition and subtraction combine counters by adding or subtracting the counts
of corresponding elements.  Intersection and union return the minimum and
maximum of corresponding counts.  Each operation can accept inputs with signed
counts, but the output will exclude results with counts of zero or less.

    >>> c = Counter(a=3, b=1)
    >>> d = Counter(a=1, b=2)
    >>> c + d                       # add two counters together:  c[x] + d[x]
    Counter({'a': 4, 'b': 3})
    >>> c - d                       # subtract (keeping only positive counts)
    Counter({'a': 2})
    >>> c & d                       # intersection:  min(c[x], d[x])
    Counter({'a': 1, 'b': 1})
    >>> c | d                       # union:  max(c[x], d[x])
    Counter({'a': 3, 'b': 2})

Unary addition and substraction are shortcuts for adding an empty counter
or subtracting from an empty counter.

    >>> c = Counter(a=2, b=-4)
    >>> +c
    Counter({'a': 2})
    >>> -c
    Counter({'b': 4})

.. versionadded:: 3.3
    Added support for unary plus, unary minus, and in-place multiset operations.

.. note::

    Counters were primarily designed to work with positive integers to represent
    running counts; however, care was taken to not unnecessarily preclude use
    cases needing other types or negative values.  To help with those use cases,
    this section documents the minimum range and type restrictions.

    * The :class:`Counter` class itself is a dictionary subclass with no
        restrictions on its keys and values.  The values are intended to be numbers
        representing counts, but you *could* store anything in the value field.

    * The :meth:`most_common` method requires only that the values be orderable.

    * For in-place operations such as ``c[key] += 1``, the value type need only
        support addition and subtraction.  So fractions, floats, and decimals would
        work and negative values are supported.  The same is also true for
        :meth:`update` and :meth:`subtract` which allow negative and zero values
        for both inputs and outputs.

    * The multiset methods are designed only for use cases with positive values.
        The inputs may be negative or zero, but only outputs with positive values
        are created.  There are no type restrictions, but the value type needs to
        support addition, subtraction, and comparison.

    * The :meth:`elements` method requires integer counts.  It ignores zero and
        negative counts.

.. seealso::

    * `Counter class <http://code.activestate.com/recipes/576611/>`_
        adapted for Python 2.5 and an early `Bag recipe
        <http://code.activestate.com/recipes/259174/>`_ for Python 2.4.

    * `Bag class <http://www.gnu.org/software/smalltalk/manual-base/html_node/Bag.html>`_
        in Smalltalk.

    * Wikipedia entry for `Multisets <http://en.wikipedia.org/wiki/Multiset>`_.

    * `C++ multisets <http://www.demo2s.com/Tutorial/Cpp/0380__set-multiset/Catalog0380__set-multiset.htm>`_
        tutorial with examples.

    * For mathematical operations on multisets and their use cases, see
        *Knuth, Donald. The Art of Computer Programming Volume II,
        Section 4.6.3, Exercise 19*.

    * To enumerate all distinct multisets of a given size over a given set of
        elements, see :func:`itertools.combinations_with_replacement`.

            map(Counter, combinations_with_replacement('ABC', 2)) --> AA AB AC BB BC CC


:class:`deque` objects
----------------------

.. class:: deque([iterable, [maxlen]])

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

    .. method:: append(x)

        Add *x* to the right side of the deque.


    .. method:: appendleft(x)

        Add *x* to the left side of the deque.


    .. method:: clear()

        Remove all elements from the deque leaving it with length 0.


    .. method:: count(x)

        Count the number of deque elements equal to *x*.

        .. versionadded:: 3.2


    .. method:: extend(iterable)

        Extend the right side of the deque by appending elements from the iterable
        argument.


    .. method:: extendleft(iterable)

        Extend the left side of the deque by appending elements from *iterable*.
        Note, the series of left appends results in reversing the order of
        elements in the iterable argument.


    .. method:: pop()

        Remove and return an element from the right side of the deque. If no
        elements are present, raises an :exc:`IndexError`.


    .. method:: popleft()

        Remove and return an element from the left side of the deque. If no
        elements are present, raises an :exc:`IndexError`.


    .. method:: remove(value)

        Removed the first occurrence of *value*.  If not found, raises a
        :exc:`ValueError`.


    .. method:: reverse()

        Reverse the elements of the deque in-place and then return ``None``.

        .. versionadded:: 3.2


    .. method:: rotate(n)

        Rotate the deque *n* steps to the right.  If *n* is negative, rotate to
        the left.  Rotating one step to the right is equivalent to:
        ``d.appendleft(d.pop())``.


    Deque objects also provide one read-only attribute:

    .. attribute:: maxlen

        Maximum size of a deque or *None* if unbounded.

        .. versionadded:: 3.1


In addition to the above, deques support iteration, pickling, ``len(d)``,
``reversed(d)``, ``copy.copy(d)``, ``copy.deepcopy(d)``, membership testing with
the :keyword:`in` operator, and subscript references such as ``d[-1]``.  Indexed
access is O(1) at both ends but slows to O(n) in the middle.  For fast random
access, use lists instead.

Example:

.. doctest::

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


:class:`deque` Recipes
^^^^^^^^^^^^^^^^^^^^^^

This section shows various approaches to working with deques.

Bounded length deques provide functionality similar to the ``tail`` filter
in Unix::

    def tail(filename, n=10):
        'Return the last n lines of a file'
        with open(filename) as f:
            return deque(f, n)

Another approach to using deques is to maintain a sequence of recently
added elements by appending to the right and popping to the left::

    def moving_average(iterable, n=3):
        # moving_average([40, 30, 50, 46, 39, 44]) --> 40.0 42.0 45.0 43.0
        # http://en.wikipedia.org/wiki/Moving_average
        it = iter(iterable)
        d = deque(itertools.islice(it, n-1))
        d.appendleft(0)
        s = sum(d)
        for elem in it:
            s += elem - d.popleft()
            d.append(elem)
            yield s / n

The :meth:`rotate` method provides a way to implement :class:`deque` slicing and
deletion.  For example, a pure Python implementation of ``del d[n]`` relies on
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


:class:`defaultdict` objects
----------------------------

.. class:: defaultdict([default_factory[, ...]])

    Returns a new dictionary-like object.  :class:`defaultdict` is a subclass of the
    built-in :class:`dict` class.  It overrides one method and adds one writable
    instance variable.  The remaining functionality is the same as for the
    :class:`dict` class and is not documented here.

    The first argument provides the initial value for the :attr:`default_factory`
    attribute; it defaults to ``None``. All remaining arguments are treated the same
    as if they were passed to the :class:`dict` constructor, including keyword
    arguments.


    :class:`defaultdict` objects support the following method in addition to the
    standard :class:`dict` operations:

    .. method:: __missing__(key)

        If the :attr:`default_factory` attribute is ``None``, this raises a
        :exc:`KeyError` exception with the *key* as argument.

        If :attr:`default_factory` is not ``None``, it is called without arguments
        to provide a default value for the given *key*, this value is inserted in
        the dictionary for the *key*, and returned.

        If calling :attr:`default_factory` raises an exception this exception is
        propagated unchanged.

        This method is called by the :meth:`__getitem__` method of the
        :class:`dict` class when the requested key is not found; whatever it
        returns or raises is then returned or raised by :meth:`__getitem__`.

        Note that :meth:`__missing__` is *not* called for any operations besides
        :meth:`__getitem__`. This means that :meth:`get` will, like normal
        dictionaries, return ``None`` as a default rather than using
        :attr:`default_factory`.


    :class:`defaultdict` objects support the following instance variable:


    .. attribute:: default_factory

        This attribute is used by the :meth:`__missing__` method; it is
        initialized from the first argument to the constructor, if present, or to
        ``None``, if absent.


:class:`defaultdict` Examples
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Using :class:`list` as the :attr:`default_factory`, it is easy to group a
sequence of key-value pairs into a dictionary of lists:

    >>> s = [('yellow', 1), ('blue', 2), ('yellow', 3), ('blue', 4), ('red', 1)]
    >>> d = defaultdict(list)
    >>> for k, v in s:
    ...     d[k].append(v)
    ...
    >>> list(d.items())
    [('blue', [2, 4]), ('red', [1]), ('yellow', [1, 3])]

When each key is encountered for the first time, it is not already in the
mapping; so an entry is automatically created using the :attr:`default_factory`
function which returns an empty :class:`list`.  The :meth:`list.append`
operation then attaches the value to the new list.  When keys are encountered
again, the look-up proceeds normally (returning the list for that key) and the
:meth:`list.append` operation adds another value to the list. This technique is
simpler and faster than an equivalent technique using :meth:`dict.setdefault`:

    >>> d = {}
    >>> for k, v in s:
    ...     d.setdefault(k, []).append(v)
    ...
    >>> list(d.items())
    [('blue', [2, 4]), ('red', [1]), ('yellow', [1, 3])]

Setting the :attr:`default_factory` to :class:`int` makes the
:class:`defaultdict` useful for counting (like a bag or multiset in other
languages):

    >>> s = 'mississippi'
    >>> d = defaultdict(int)
    >>> for k in s:
    ...     d[k] += 1
    ...
    >>> list(d.items())
    [('i', 4), ('p', 2), ('s', 4), ('m', 1)]

When a letter is first encountered, it is missing from the mapping, so the
:attr:`default_factory` function calls :func:`int` to supply a default count of
zero.  The increment operation then builds up the count for each letter.

The function :func:`int` which always returns zero is just a special case of
constant functions.  A faster and more flexible way to create constant functions
is to use a lambda function which can supply any constant value (not just
zero):

    >>> def constant_factory(value):
    ...     return lambda: value
    >>> d = defaultdict(constant_factory('<missing>'))
    >>> d.update(name='John', action='ran')
    >>> '%(name)s %(action)s to %(object)s' % d
    'John ran to <missing>'

Setting the :attr:`default_factory` to :class:`set` makes the
:class:`defaultdict` useful for building a dictionary of sets:

    >>> s = [('red', 1), ('blue', 2), ('red', 3), ('blue', 4), ('red', 1), ('blue', 4)]
    >>> d = defaultdict(set)
    >>> for k, v in s:
    ...     d[k].add(v)
    ...
    >>> list(d.items())
    [('blue', {2, 4}), ('red', {1, 3})]


:func:`namedtuple` Factory Function for Tuples with Named Fields
----------------------------------------------------------------

Named tuples assign meaning to each position in a tuple and allow for more readable,
self-documenting code.  They can be used wherever regular tuples are used, and
they add the ability to access fields by name instead of position index.

.. function:: namedtuple(typename, field_names, verbose=False, rename=False)

    Returns a new tuple subclass named *typename*.  The new subclass is used to
    create tuple-like objects that have fields accessible by attribute lookup as
    well as being indexable and iterable.  Instances of the subclass also have a
    helpful docstring (with typename and field_names) and a helpful :meth:`__repr__`
    method which lists the tuple contents in a ``name=value`` format.

    The *field_names* are a single string with each fieldname separated by whitespace
    and/or commas, for example ``'x y'`` or ``'x, y'``.  Alternatively, *field_names*
    can be a sequence of strings such as ``['x', 'y']``.

    Any valid Python identifier may be used for a fieldname except for names
    starting with an underscore.  Valid identifiers consist of letters, digits,
    and underscores but do not start with a digit or underscore and cannot be
    a :mod:`keyword` such as *class*, *for*, *return*, *global*, *pass*,
    or *raise*.

    If *rename* is true, invalid fieldnames are automatically replaced
    with positional names.  For example, ``['abc', 'def', 'ghi', 'abc']`` is
    converted to ``['abc', '_1', 'ghi', '_3']``, eliminating the keyword
    ``def`` and the duplicate fieldname ``abc``.

    If *verbose* is true, the class definition is printed after it is
    built.  This option is outdated; instead, it is simpler to print the
    :attr:`_source` attribute.

    Named tuple instances do not have per-instance dictionaries, so they are
    lightweight and require no more memory than regular tuples.

    .. versionchanged:: 3.1
        Added support for *rename*.


.. doctest::
    :options: +NORMALIZE_WHITESPACE

    >>> # Basic example
    >>> Point = namedtuple('Point', ['x', 'y'])
    >>> p = Point(11, y=22)     # instantiate with positional or keyword arguments
    >>> p[0] + p[1]             # indexable like the plain tuple (11, 22)
    33
    >>> x, y = p                # unpack like a regular tuple
    >>> x, y
    (11, 22)
    >>> p.x + p.y               # fields also accessible by name
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
three additional methods and two attributes.  To prevent conflicts with
field names, the method and attribute names start with an underscore.

.. classmethod:: somenamedtuple._make(iterable)

    Class method that makes a new instance from an existing sequence or iterable.

.. doctest::

    >>> t = [11, 22]
    >>> Point._make(t)
    Point(x=11, y=22)

.. method:: somenamedtuple._asdict()

    Return a new :class:`OrderedDict` which maps field names to their corresponding
    values.  Note, this method is no longer needed now that the same effect can
    be achieved by using the built-in :func:`vars` function::

        >>> vars(p)
        OrderedDict([('x', 11), ('y', 22)])

    .. versionchanged:: 3.1
        Returns an :class:`OrderedDict` instead of a regular :class:`dict`.

.. method:: somenamedtuple._replace(kwargs)

    Return a new instance of the named tuple replacing specified fields with new
    values:

::

    >>> p = Point(x=11, y=22)
    >>> p._replace(x=33)
    Point(x=33, y=22)

    >>> for partnum, record in inventory.items():
    ...     inventory[partnum] = record._replace(price=newprices[partnum], timestamp=time.now())

.. attribute:: somenamedtuple._source

    A string with the pure Python source code used to create the named
    tuple class.  The source makes the named tuple self-documenting.
    It can be printed, executed using :func:`exec`, or saved to a file
    and imported.

    .. versionadded:: 3.3

.. attribute:: somenamedtuple._fields

    Tuple of strings listing the field names.  Useful for introspection
    and for creating new named tuple types from existing named tuples.

.. doctest::

    >>> p._fields            # view the field names
    ('x', 'y')

    >>> Color = namedtuple('Color', 'red green blue')
    >>> Pixel = namedtuple('Pixel', Point._fields + Color._fields)
    >>> Pixel(11, 22, 128, 255, 0)
    Pixel(x=11, y=22, red=128, green=255, blue=0)

To retrieve a field whose name is stored in a string, use the :func:`getattr`
function:

    >>> getattr(p, 'x')
    11

To convert a dictionary to a named tuple, use the double-star-operator
(as described in :ref:`tut-unpacking-arguments`):

    >>> d = {'x': 11, 'y': 22}
    >>> Point(**d)
    Point(x=11, y=22)

Since a named tuple is a regular Python class, it is easy to add or change
functionality with a subclass.  Here is how to add a calculated field and
a fixed-width print format:

    >>> class Point(namedtuple('Point', 'x y')):
        __slots__ = ()
        @property
        def hypot(self):
            return (self.x ** 2 + self.y ** 2) ** 0.5
        def __str__(self):
            return 'Point: x=%6.3f  y=%6.3f  hypot=%6.3f' % (self.x, self.y, self.hypot)

    >>> for p in Point(3, 4), Point(14, 5/7):
        print(p)
    Point: x= 3.000  y= 4.000  hypot= 5.000
    Point: x=14.000  y= 0.714  hypot=14.018

The subclass shown above sets ``__slots__`` to an empty tuple.  This helps
keep memory requirements low by preventing the creation of instance dictionaries.

Subclassing is not useful for adding new, stored fields.  Instead, simply
create a new named tuple type from the :attr:`_fields` attribute:

    >>> Point3D = namedtuple('Point3D', Point._fields + ('z',))

Default values can be implemented by using :meth:`_replace` to
customize a prototype instance:

    >>> Account = namedtuple('Account', 'owner balance transaction_count')
    >>> default_account = Account('<owner name>', 0.0, 0)
    >>> johns_account = default_account._replace(owner='John')
    >>> janes_account = default_account._replace(owner='Jane')

Enumerated constants can be implemented with named tuples, but it is simpler
and more efficient to use a simple class declaration:

    >>> Status = namedtuple('Status', 'open pending closed')._make(range(3))
    >>> Status.open, Status.pending, Status.closed
    (0, 1, 2)
    >>> class Status:
        open, pending, closed = range(3)

.. seealso::

    * `Named tuple recipe <http://code.activestate.com/recipes/500261/>`_
        adapted for Python 2.4.

    * `Recipe for named tuple abstract base class with a metaclass mix-in
        <http://code.activestate.com/recipes/577629-namedtupleabc-abstract-base-class-mix-in-for-named/>`_
        by Jan Kaliszewski.  Besides providing an :term:`abstract base class` for
        named tuples, it also supports an alternate :term:`metaclass`-based
        constructor that is convenient for use cases where named tuples are being
        subclassed.


:class:`OrderedDict` objects
----------------------------

Ordered dictionaries are just like regular dictionaries but they remember the
order that items were inserted.  When iterating over an ordered dictionary,
the items are returned in the order their keys were first added.

.. class:: OrderedDict([items])

    Return an instance of a dict subclass, supporting the usual :class:`dict`
    methods.  An *OrderedDict* is a dict that remembers the order that keys
    were first inserted. If a new entry overwrites an existing entry, the
    original insertion position is left unchanged.  Deleting an entry and
    reinserting it will move it to the end.

    .. versionadded:: 3.1

    .. method:: popitem(last=True)

        The :meth:`popitem` method for ordered dictionaries returns and removes a
        (key, value) pair.  The pairs are returned in LIFO order if *last* is true
        or FIFO order if false.

    .. method:: move_to_end(key, last=True)

        Move an existing *key* to either end of an ordered dictionary.  The item
        is moved to the right end if *last* is true (the default) or to the
        beginning if *last* is false.  Raises :exc:`KeyError` if the *key* does
        not exist::

            >>> d = OrderedDict.fromkeys('abcde')
            >>> d.move_to_end('b')
            >>> ''.join(d.keys())
            'acdeb'
            >>> d.move_to_end('b', last=False)
            >>> ''.join(d.keys())
            'bacde'

        .. versionadded:: 3.2

In addition to the usual mapping methods, ordered dictionaries also support
reverse iteration using :func:`reversed`.

Equality tests between :class:`OrderedDict` objects are order-sensitive
and are implemented as ``list(od1.items())==list(od2.items())``.
Equality tests between :class:`OrderedDict` objects and other
:class:`Mapping` objects are order-insensitive like regular dictionaries.
This allows :class:`OrderedDict` objects to be substituted anywhere a
regular dictionary is used.

The :class:`OrderedDict` constructor and :meth:`update` method both accept
keyword arguments, but their order is lost because Python's function call
semantics pass-in keyword arguments using a regular unordered dictionary.

.. seealso::

    `Equivalent OrderedDict recipe <http://code.activestate.com/recipes/576693/>`_
    that runs on Python 2.4 or later.

:class:`OrderedDict` Examples and Recipes
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Since an ordered dictionary remembers its insertion order, it can be used
in conjuction with sorting to make a sorted dictionary::

    >>> # regular unsorted dictionary
    >>> d = {'banana': 3, 'apple':4, 'pear': 1, 'orange': 2}

    >>> # dictionary sorted by key
    >>> OrderedDict(sorted(d.items(), key=lambda t: t[0]))
    OrderedDict([('apple', 4), ('banana', 3), ('orange', 2), ('pear', 1)])

    >>> # dictionary sorted by value
    >>> OrderedDict(sorted(d.items(), key=lambda t: t[1]))
    OrderedDict([('pear', 1), ('orange', 2), ('banana', 3), ('apple', 4)])

    >>> # dictionary sorted by length of the key string
    >>> OrderedDict(sorted(d.items(), key=lambda t: len(t[0])))
    OrderedDict([('pear', 1), ('apple', 4), ('orange', 2), ('banana', 3)])

The new sorted dictionaries maintain their sort order when entries
are deleted.  But when new keys are added, the keys are appended
to the end and the sort is not maintained.

It is also straight-forward to create an ordered dictionary variant
that the remembers the order the keys were *last* inserted.
If a new entry overwrites an existing entry, the
original insertion position is changed and moved to the end::

    class LastUpdatedOrderedDict(OrderedDict):
        'Store items in the order the keys were last added'

        def __setitem__(self, key, value):
            if key in self:
                del self[key]
            OrderedDict.__setitem__(self, key, value)

An ordered dictionary can be combined with the :class:`Counter` class
so that the counter remembers the order elements are first encountered::

    class OrderedCounter(Counter, OrderedDict):
        'Counter that remembers the order elements are first encountered'

        def __repr__(self):
            return '%s(%r)' % (self.__class__.__name__, OrderedDict(self))

        def __reduce__(self):
            return self.__class__, (OrderedDict(self),)


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

    .. attribute:: data

        A real dictionary used to store the contents of the :class:`UserDict`
        class.



:class:`UserList` objects
-------------------------

This class acts as a wrapper around list objects.  It is a useful base class
for your own list-like classes which can inherit from them and override
existing methods or add new ones.  In this way, one can add new behaviors to
lists.

The need for this class has been partially supplanted by the ability to
subclass directly from :class:`list`; however, this class can be easier
to work with because the underlying list is accessible as an attribute.

.. class:: UserList([list])

    Class that simulates a list.  The instance's contents are kept in a regular
    list, which is accessible via the :attr:`data` attribute of :class:`UserList`
    instances.  The instance's contents are initially set to a copy of *list*,
    defaulting to the empty list ``[]``.  *list* can be any iterable, for
    example a real Python list or a :class:`UserList` object.

    In addition to supporting the methods and operations of mutable sequences,
    :class:`UserList` instances provide the following attribute:

    .. attribute:: data

        A real :class:`list` object used to store the contents of the
        :class:`UserList` class.

**Subclassing requirements:** Subclasses of :class:`UserList` are expect to
offer a constructor which can be called with either no arguments or one
argument.  List operations which return a new sequence attempt to create an
instance of the actual implementation class.  To do so, it assumes that the
constructor can be called with a single parameter, which is a sequence object
used as a data source.

If a derived class does not wish to comply with this requirement, all of the
special methods supported by this class will need to be overridden; please
consult the sources for information about the methods which need to be provided
in that case.

:class:`UserString` objects
---------------------------

The class, :class:`UserString` acts as a wrapper around string objects.
The need for this class has been partially supplanted by the ability to
subclass directly from :class:`str`; however, this class can be easier
to work with because the underlying string is accessible as an
attribute.

.. class:: UserString([sequence])

    Class that simulates a string or a Unicode string object.  The instance's
    content is kept in a regular string object, which is accessible via the
    :attr:`data` attribute of :class:`UserString` instances.  The instance's
    contents are initially set to a copy of *sequence*.  The *sequence* can
    be an instance of :class:`bytes`, :class:`str`, :class:`UserString` (or a
    subclass) or an arbitrary sequence which can be converted into a string using
    the built-in :func:`str` function.
