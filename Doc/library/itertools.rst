
:mod:`itertools` --- Functions creating iterators for efficient looping
=======================================================================

.. module:: itertools
   :synopsis: Functions creating iterators for efficient looping.
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>


.. testsetup::

   from itertools import *


This module implements a number of :term:`iterator` building blocks inspired by
constructs from the Haskell and SML programming languages.  Each has been recast
in a form suitable for Python.

The module standardizes a core set of fast, memory efficient tools that are
useful by themselves or in combination.  Standardization helps avoid the
readability and reliability problems which arise when many different individuals
create their own slightly varying implementations, each with their own quirks
and naming conventions.

The tools are designed to combine readily with one another.  This makes it easy
to construct more specialized tools succinctly and efficiently in pure Python.

For instance, SML provides a tabulation tool: ``tabulate(f)`` which produces a
sequence ``f(0), f(1), ...``.  But, this effect can be achieved in Python
by combining :func:`map` and :func:`count` to form ``map(f, count())``.

Likewise, the functional tools are designed to work well with the high-speed
functions provided by the :mod:`operator` module.

The module author welcomes suggestions for other basic building blocks to be
added to future versions of the module.

Whether cast in pure python form or compiled code, tools that use iterators are
more memory efficient (and faster) than their list based counterparts. Adopting
the principles of just-in-time manufacturing, they create data when and where
needed instead of consuming memory with the computer equivalent of "inventory".

The performance advantage of iterators becomes more acute as the number of
elements increases -- at some point, lists grow large enough to severely impact
memory cache performance and start running slowly.


.. seealso::

   The Standard ML Basis Library, `The Standard ML Basis Library
   <http://www.standardml.org/Basis/>`_.

   Haskell, A Purely Functional Language, `Definition of Haskell and the Standard
   Libraries <http://www.haskell.org/definition/>`_.


.. _itertools-functions:

Itertool functions
------------------

The following module functions all construct and return iterators. Some provide
streams of infinite length, so they should only be accessed by functions or
loops that truncate the stream.


.. function:: chain(*iterables)

   Make an iterator that returns elements from the first iterable until it is
   exhausted, then proceeds to the next iterable, until all of the iterables are
   exhausted.  Used for treating consecutive sequences as a single sequence.
   Equivalent to::

      def chain(*iterables):
          # chain('ABC', 'DEF') --> A B C D E F
          for it in iterables:
              for element in it:
                  yield element


.. function:: itertools.chain.from_iterable(iterable)

   Alternate constructor for :func:`chain`.  Gets chained inputs from a 
   single iterable argument that is evaluated lazily.  Equivalent to::

      @classmethod
      def from_iterable(iterables):
          # chain.from_iterable(['ABC', 'DEF']) --> A B C D E F
          for it in iterables:
              for element in it:
                  yield element

   .. versionadded:: 2.6


.. function:: combinations(iterable, r)

   Return *r* length subsequences of elements from the input *iterable*.

   Combinations are emitted in lexicographic sort order.  So, if the 
   input *iterable* is sorted, the combination tuples will be produced
   in sorted order.  

   Elements are treated as unique based on their position, not on their
   value.  So if the input elements are unique, there will be no repeat
   values in each combination.

   Equivalent to::

        def combinations(iterable, r):
            # combinations('ABCD', 2) --> AB AC AD BC BD CD
            # combinations(range(4), 3) --> 012 013 023 123
            pool = tuple(iterable)
            n = len(pool)
            indices = range(r)
            yield tuple(pool[i] for i in indices)
            while 1:
                for i in reversed(range(r)):
                    if indices[i] != i + n - r:
                        break
                else:
                    return
                indices[i] += 1
                for j in range(i+1, r):
                    indices[j] = indices[j-1] + 1
                yield tuple(pool[i] for i in indices)

   The code for :func:`combinations` can be also expressed as a subsequence
   of :func:`permutations` after filtering entries where the elements are not
   in sorted order (according to their position in the input pool)::

        def combinations(iterable, r):
            pool = tuple(iterable)
            n = len(pool)
            for indices in permutations(range(n), r):
                if sorted(indices) == list(indices):
                    yield tuple(pool[i] for i in indices)

   .. versionadded:: 2.6

.. function:: count([n])

   Make an iterator that returns consecutive integers starting with *n*. If not
   specified *n* defaults to zero.   Often used as an argument to :func:`map` to
   generate consecutive data points. Also, used with :func:`zip` to add sequence
   numbers.  Equivalent to::

      def count(n=0):
          # count(10) --> 10 11 12 13 14 ...
          while True:
              yield n
              n += 1


.. function:: cycle(iterable)

   Make an iterator returning elements from the iterable and saving a copy of each.
   When the iterable is exhausted, return elements from the saved copy.  Repeats
   indefinitely.  Equivalent to::

      def cycle(iterable):
          # cycle('ABCD') --> A B C D A B C D A B C D ...
          saved = []
          for element in iterable:
              yield element
              saved.append(element)
          while saved:
              for element in saved:
                    yield element

   Note, this member of the toolkit may require significant auxiliary storage
   (depending on the length of the iterable).


.. function:: dropwhile(predicate, iterable)

   Make an iterator that drops elements from the iterable as long as the predicate
   is true; afterwards, returns every element.  Note, the iterator does not produce
   *any* output until the predicate first becomes false, so it may have a lengthy
   start-up time.  Equivalent to::

      def dropwhile(predicate, iterable):
          # dropwhile(lambda x: x<5, [1,4,6,4,1]) --> 6 4 1
          iterable = iter(iterable)
          for x in iterable:
              if not predicate(x):
                  yield x
                  break
          for x in iterable:
              yield x


.. function:: groupby(iterable[, key])

   Make an iterator that returns consecutive keys and groups from the *iterable*.
   The *key* is a function computing a key value for each element.  If not
   specified or is ``None``, *key* defaults to an identity function and returns
   the element unchanged.  Generally, the iterable needs to already be sorted on
   the same key function.

   The operation of :func:`groupby` is similar to the ``uniq`` filter in Unix.  It
   generates a break or new group every time the value of the key function changes
   (which is why it is usually necessary to have sorted the data using the same key
   function).  That behavior differs from SQL's GROUP BY which aggregates common
   elements regardless of their input order.

   The returned group is itself an iterator that shares the underlying iterable
   with :func:`groupby`.  Because the source is shared, when the :func:`groupby`
   object is advanced, the previous group is no longer visible.  So, if that data
   is needed later, it should be stored as a list::

      groups = []
      uniquekeys = []
      data = sorted(data, key=keyfunc)
      for k, g in groupby(data, keyfunc):
          groups.append(list(g))      # Store group iterator as a list
          uniquekeys.append(k)

   :func:`groupby` is equivalent to::

      class groupby(object):
          # [k for k, g in groupby('AAAABBBCCDAABBB')] --> A B C D A B
          # [(list(g)) for k, g in groupby('AAAABBBCCD')] --> AAAA BBB CC D
          def __init__(self, iterable, key=None):
              if key is None:
                  key = lambda x: x
              self.keyfunc = key
              self.it = iter(iterable)
              self.tgtkey = self.currkey = self.currvalue = object()
          def __iter__(self):
              return self
          def __next__(self):
              while self.currkey == self.tgtkey:
                  self.currvalue = next(self.it) # Exit on StopIteration
                  self.currkey = self.keyfunc(self.currvalue)
              self.tgtkey = self.currkey
              return (self.currkey, self._grouper(self.tgtkey))
          def _grouper(self, tgtkey):
              while self.currkey == tgtkey:
                  yield self.currvalue
                  self.currvalue = next(self.it) # Exit on StopIteration
                  self.currkey = self.keyfunc(self.currvalue)


.. function:: filterfalse(predicate, iterable)

   Make an iterator that filters elements from iterable returning only those for
   which the predicate is ``False``. If *predicate* is ``None``, return the items
   that are false. Equivalent to::

      def filterfalse(predicate, iterable):
          # filterfalse(lambda x: x%2, range(10)) --> 0 2 4 6 8
          if predicate is None:
              predicate = bool
          for x in iterable:
              if not predicate(x):
                  yield x


.. function:: islice(iterable, [start,] stop [, step])

   Make an iterator that returns selected elements from the iterable. If *start* is
   non-zero, then elements from the iterable are skipped until start is reached.
   Afterward, elements are returned consecutively unless *step* is set higher than
   one which results in items being skipped.  If *stop* is ``None``, then iteration
   continues until the iterator is exhausted, if at all; otherwise, it stops at the
   specified position.  Unlike regular slicing, :func:`islice` does not support
   negative values for *start*, *stop*, or *step*.  Can be used to extract related
   fields from data where the internal structure has been flattened (for example, a
   multi-line report may list a name field on every third line).  Equivalent to::

      def islice(iterable, *args):
          # islice('ABCDEFG', 2) --> A B
          # islice('ABCDEFG', 2, 4) --> C D
          # islice('ABCDEFG', 2, None) --> C D E F G
          # islice('ABCDEFG', 0, None, 2) --> A C E G
          s = slice(*args)
          it = range(s.start or 0, s.stop or sys.maxsize, s.step or 1)
          nexti = next(it)
          for i, element in enumerate(iterable):
              if i == nexti:
                  yield element
                  nexti = next(it)

   If *start* is ``None``, then iteration starts at zero. If *step* is ``None``,
   then the step defaults to one.


.. function:: zip_longest(*iterables[, fillvalue])

   Make an iterator that aggregates elements from each of the iterables. If the
   iterables are of uneven length, missing values are filled-in with *fillvalue*.
   Iteration continues until the longest iterable is exhausted.  Equivalent to::

      def zip_longest(*args, fillvalue=None):
          # zip_longest('ABCD', 'xy', fillvalue='-') --> Ax By C- D-
          def sentinel(counter = ([fillvalue]*(len(args)-1)).pop):
              yield counter()         # yields the fillvalue, or raises IndexError
          fillers = repeat(fillvalue)
          iters = [chain(it, sentinel(), fillers) for it in args]
          try:
              for tup in zip(*iters):
                  yield tup
          except IndexError:
              pass

   If one of the iterables is potentially infinite, then the :func:`zip_longest`
   function should be wrapped with something that limits the number of calls (for
   example :func:`islice` or :func:`takewhile`).


.. function:: permutations(iterable[, r])

   Return successive *r* length permutations of elements in the *iterable*.

   If *r* is not specified or is ``None``, then *r* defaults to the length
   of the *iterable* and all possible full-length permutations 
   are generated.

   Permutations are emitted in lexicographic sort order.  So, if the 
   input *iterable* is sorted, the permutation tuples will be produced
   in sorted order.  

   Elements are treated as unique based on their position, not on their
   value.  So if the input elements are unique, there will be no repeat
   values in each permutation.

   Equivalent to::

        def permutations(iterable, r=None):
            # permutations('ABCD', 2) --> AB AC AD BA BC BD CA CB CD DA DB DC
            # permutations(range(3)) --> 012 021 102 120 201 210
            pool = tuple(iterable)
            n = len(pool)
            r = n if r is None else r
            indices = range(n)
            cycles = range(n, n-r, -1)
            yield tuple(pool[i] for i in indices[:r])
            while n:
                for i in reversed(range(r)):
                    cycles[i] -= 1
                    if cycles[i] == 0:
                        indices[i:] = indices[i+1:] + indices[i:i+1]
                        cycles[i] = n - i
                    else:
                        j = cycles[i]
                        indices[i], indices[-j] = indices[-j], indices[i]
                        yield tuple(pool[i] for i in indices[:r])
                        break
                else:
                    return

   The code for :func:`permutations` can be also expressed as a subsequence of 
   :func:`product`, filtered to exclude entries with repeated elements (those
   from the same position in the input pool)::

        def permutations(iterable, r=None):
            pool = tuple(iterable)
            n = len(pool)
            r = n if r is None else r
            for indices in product(range(n), repeat=r):
                if len(set(indices)) == r:
                    yield tuple(pool[i] for i in indices)

   .. versionadded:: 2.6

.. function:: product(*iterables[, repeat])

   Cartesian product of input iterables.

   Equivalent to nested for-loops in a generator expression. For example,
   ``product(A, B)`` returns the same as ``((x,y) for x in A for y in B)``.

   The nested loops cycle like an odometer with the rightmost element advancing
   on every iteration.  This pattern creates a lexicographic ordering so that if
   the input's iterables are sorted, the product tuples are emitted in sorted
   order.

   To compute the product of an iterable with itself, specify the number of
   repetitions with the optional *repeat* keyword argument.  For example,
   ``product(A, repeat=4)`` means the same as ``product(A, A, A, A)``.

   This function is equivalent to the following code, except that the
   actual implementation does not build up intermediate results in memory::

       def product(*args, repeat=1):
           # product('ABCD', 'xy') --> Ax Ay Bx By Cx Cy Dx Dy
           # product(range(2), repeat=3) --> 000 001 010 011 100 101 110 111
           pools = map(tuple, args) * repeat
           result = [[]]
           for pool in pools:
               result = [x+[y] for x in result for y in pool]
           for prod in result:
               yield tuple(prod)


.. function:: repeat(object[, times])

   Make an iterator that returns *object* over and over again. Runs indefinitely
   unless the *times* argument is specified. Used as argument to :func:`map` for
   invariant parameters to the called function.  Also used with :func:`zip` to
   create an invariant part of a tuple record.  Equivalent to::

      def repeat(object, times=None):
          # repeat(10, 3) --> 10 10 10
          if times is None:
              while True:
                  yield object
          else:
              for i in range(times):
                  yield object


.. function:: starmap(function, iterable)

   Make an iterator that computes the function using arguments obtained from
   the iterable.  Used instead of :func:`map` when argument parameters are already
   grouped in tuples from a single iterable (the data has been "pre-zipped").  The
   difference between :func:`map` and :func:`starmap` parallels the distinction
   between ``function(a,b)`` and ``function(*c)``. Equivalent to::

      def starmap(function, iterable):
          # starmap(pow, [(2,5), (3,2), (10,3)]) --> 32 9 1000
          for args in iterable:
              yield function(*args)

   .. versionchanged:: 2.6
      Previously, :func:`starmap` required the function arguments to be tuples.
      Now, any iterable is allowed.


.. function:: takewhile(predicate, iterable)

   Make an iterator that returns elements from the iterable as long as the
   predicate is true.  Equivalent to::

      def takewhile(predicate, iterable):
          # takewhile(lambda x: x<5, [1,4,6,4,1]) --> 1 4
          for x in iterable:
              if predicate(x):
                  yield x
              else:
                  break


.. function:: tee(iterable[, n=2])

   Return *n* independent iterators from a single iterable. The case where ``n==2``
   is equivalent to::

      def tee(iterable):
          def gen(next, data={}):
              for i in count():
                  if i in data:
                      yield data.pop(i)
                  else:
                      data[i] = next()
                      yield data[i]
          it = iter(iterable)
          return (gen(it.__next__), gen(it.__next__))

   Note, once :func:`tee` has made a split, the original *iterable* should not be
   used anywhere else; otherwise, the *iterable* could get advanced without the tee
   objects being informed.

   Note, this member of the toolkit may require significant auxiliary storage
   (depending on how much temporary data needs to be stored). In general, if one
   iterator is going to use most or all of the data before the other iterator, it
   is faster to use :func:`list` instead of :func:`tee`.


.. _itertools-example:

Examples
--------

The following examples show common uses for each tool and demonstrate ways they
can be combined.

.. doctest::

   # Show a dictionary sorted and grouped by value
   >>> from operator import itemgetter
   >>> d = dict(a=1, b=2, c=1, d=2, e=1, f=2, g=3)
   >>> di = sorted(d.items(), key=itemgetter(1))
   >>> for k, g in groupby(di, key=itemgetter(1)):
   ...     print(k, map(itemgetter(0), g))
   ...
   1 ['a', 'c', 'e']
   2 ['b', 'd', 'f']
   3 ['g']

   # Find runs of consecutive numbers using groupby.  The key to the solution
   # is differencing with a range so that consecutive numbers all appear in
   # same group.
   >>> data = [ 1,  4,5,6, 10, 15,16,17,18, 22, 25,26,27,28]
   >>> for k, g in groupby(enumerate(data), lambda t:t[0]-t[1]):
   ...     print(map(operator.itemgetter(1), g))
   ... 
   [1]
   [4, 5, 6]
   [10]
   [15, 16, 17, 18]
   [22]
   [25, 26, 27, 28]



.. _itertools-recipes:

Recipes
-------

This section shows recipes for creating an extended toolset using the existing
itertools as building blocks.

The extended tools offer the same high performance as the underlying toolset.
The superior memory performance is kept by processing elements one at a time
rather than bringing the whole iterable into memory all at once. Code volume is
kept small by linking the tools together in a functional style which helps
eliminate temporary variables.  High speed is retained by preferring
"vectorized" building blocks over the use of for-loops and :term:`generator`\s
which incur interpreter overhead.

.. testcode::

   def take(n, seq):
       return list(islice(seq, n))

   def enumerate(iterable):
       return zip(count(), iterable)

   def tabulate(function):
       "Return function(0), function(1), ..."
       return map(function, count())

   def items(mapping):
       return zip(mapping.keys(), mapping.values())

   def nth(iterable, n):
       "Returns the nth item or raise StopIteration"
       return next(islice(iterable, n, None))

   def all(seq, pred=None):
       "Returns True if pred(x) is true for every element in the iterable"
       for elem in filterfalse(pred, seq):
           return False
       return True

   def any(seq, pred=None):
       "Returns True if pred(x) is true for at least one element in the iterable"
       for elem in filter(pred, seq):
           return True
       return False

   def no(seq, pred=None):
       "Returns True if pred(x) is false for every element in the iterable"
       for elem in filter(pred, seq):
           return False
       return True

   def quantify(seq, pred=None):
       "Count how many times the predicate is true in the sequence"
       return sum(map(pred, seq))

   def padnone(seq):
       """Returns the sequence elements and then returns None indefinitely.

       Useful for emulating the behavior of the built-in map() function.
       """
       return chain(seq, repeat(None))

   def ncycles(seq, n):
       "Returns the sequence elements n times"
       return chain.from_iterable(repeat(seq, n))

   def dotproduct(vec1, vec2):
       return sum(map(operator.mul, vec1, vec2))

   def flatten(listOfLists):
       return list(chain.from_iterable(listOfLists))

   def repeatfunc(func, times=None, *args):
       """Repeat calls to func with specified arguments.

       Example:  repeatfunc(random.random)
       """
       if times is None:
           return starmap(func, repeat(args))
       return starmap(func, repeat(args, times))

   def pairwise(iterable):
       "s -> (s0,s1), (s1,s2), (s2, s3), ..."
       a, b = tee(iterable)
       for elem in b:
           break
       return zip(a, b)

   def grouper(n, iterable, fillvalue=None):
       "grouper(3, 'abcdefg', 'x') --> ('a','b','c'), ('d','e','f'), ('g','x','x')"
       args = [iter(iterable)] * n
       return zip_longest(*args, fillvalue=fillvalue)

   def roundrobin(*iterables):
       "roundrobin('abc', 'd', 'ef') --> 'a', 'd', 'e', 'b', 'f', 'c'"
       # Recipe credited to George Sakkis
       pending = len(iterables)
       nexts = cycle(iter(it).__next__ for it in iterables)
       while pending:
           try:
               for next in nexts:
                   yield next()
           except StopIteration:
               pending -= 1
               nexts = cycle(islice(nexts, pending))

   def powerset(iterable):
       "powerset('ab') --> set([]), set(['a']), set(['b']), set(['a', 'b'])"
       # Recipe credited to Eric Raymond
       pairs = [(2**i, x) for i, x in enumerate(iterable)]
       for n in xrange(2**len(pairs)):
           yield set(x for m, x in pairs if m&n)

   def compress(data, selectors):
       "compress('abcdef', [1,0,1,0,1,1]) --> a c e f"
       for d, s in zip(data, selectors):
           if s:
               yield d

