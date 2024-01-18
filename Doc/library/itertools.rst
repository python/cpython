:mod:`itertools` --- Functions creating iterators for efficient looping
=======================================================================

.. module:: itertools
   :synopsis: Functions creating iterators for efficient looping.

.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. sectionauthor:: Raymond Hettinger <python@rcn.com>

.. testsetup::

   from itertools import *
   import collections
   import math
   import operator
   import random

--------------

This module implements a number of :term:`iterator` building blocks inspired
by constructs from APL, Haskell, and SML.  Each has been recast in a form
suitable for Python.

The module standardizes a core set of fast, memory efficient tools that are
useful by themselves or in combination.  Together, they form an "iterator
algebra" making it possible to construct specialized tools succinctly and
efficiently in pure Python.

For instance, SML provides a tabulation tool: ``tabulate(f)`` which produces a
sequence ``f(0), f(1), ...``.  The same effect can be achieved in Python
by combining :func:`map` and :func:`count` to form ``map(f, count())``.

These tools and their built-in counterparts also work well with the high-speed
functions in the :mod:`operator` module.  For example, the multiplication
operator can be mapped across two vectors to form an efficient dot-product:
``sum(starmap(operator.mul, zip(vec1, vec2, strict=True)))``.


**Infinite iterators:**

==================  =================       =================================================               =========================================
Iterator            Arguments               Results                                                         Example
==================  =================       =================================================               =========================================
:func:`count`       [start[, step]]         start, start+step, start+2*step, ...                            ``count(10) --> 10 11 12 13 14 ...``
:func:`cycle`       p                       p0, p1, ... plast, p0, p1, ...                                  ``cycle('ABCD') --> A B C D A B C D ...``
:func:`repeat`      elem [,n]               elem, elem, elem, ... endlessly or up to n times                ``repeat(10, 3) --> 10 10 10``
==================  =================       =================================================               =========================================

**Iterators terminating on the shortest input sequence:**

============================    ============================    =================================================   =============================================================
Iterator                        Arguments                       Results                                             Example
============================    ============================    =================================================   =============================================================
:func:`accumulate`              p [,func]                       p0, p0+p1, p0+p1+p2, ...                            ``accumulate([1,2,3,4,5]) --> 1 3 6 10 15``
:func:`batched`                 p, n                            (p0, p1, ..., p_n-1), ...                           ``batched('ABCDEFG', n=3) --> ABC DEF G``
:func:`chain`                   p, q, ...                       p0, p1, ... plast, q0, q1, ...                      ``chain('ABC', 'DEF') --> A B C D E F``
:func:`chain.from_iterable`     iterable                        p0, p1, ... plast, q0, q1, ...                      ``chain.from_iterable(['ABC', 'DEF']) --> A B C D E F``
:func:`compress`                data, selectors                 (d[0] if s[0]), (d[1] if s[1]), ...                 ``compress('ABCDEF', [1,0,1,0,1,1]) --> A C E F``
:func:`dropwhile`               pred, seq                       seq[n], seq[n+1], starting when pred fails          ``dropwhile(lambda x: x<5, [1,4,6,4,1]) --> 6 4 1``
:func:`filterfalse`             pred, seq                       elements of seq where pred(elem) is false           ``filterfalse(lambda x: x%2, range(10)) --> 0 2 4 6 8``
:func:`groupby`                 iterable[, key]                 sub-iterators grouped by value of key(v)
:func:`islice`                  seq, [start,] stop [, step]     elements from seq[start:stop:step]                  ``islice('ABCDEFG', 2, None) --> C D E F G``
:func:`pairwise`                iterable                        (p[0], p[1]), (p[1], p[2])                          ``pairwise('ABCDEFG') --> AB BC CD DE EF FG``
:func:`starmap`                 func, seq                       func(\*seq[0]), func(\*seq[1]), ...                 ``starmap(pow, [(2,5), (3,2), (10,3)]) --> 32 9 1000``
:func:`takewhile`               pred, seq                       seq[0], seq[1], until pred fails                    ``takewhile(lambda x: x<5, [1,4,6,4,1]) --> 1 4``
:func:`tee`                     it, n                           it1, it2, ... itn  splits one iterator into n
:func:`zip_longest`             p, q, ...                       (p[0], q[0]), (p[1], q[1]), ...                     ``zip_longest('ABCD', 'xy', fillvalue='-') --> Ax By C- D-``
============================    ============================    =================================================   =============================================================

**Combinatoric iterators:**

==============================================   ====================       =============================================================
Iterator                                         Arguments                  Results
==============================================   ====================       =============================================================
:func:`product`                                  p, q, ... [repeat=1]       cartesian product, equivalent to a nested for-loop
:func:`permutations`                             p[, r]                     r-length tuples, all possible orderings, no repeated elements
:func:`combinations`                             p, r                       r-length tuples, in sorted order, no repeated elements
:func:`combinations_with_replacement`            p, r                       r-length tuples, in sorted order, with repeated elements
==============================================   ====================       =============================================================

==============================================   =============================================================
Examples                                         Results
==============================================   =============================================================
``product('ABCD', repeat=2)``                    ``AA AB AC AD BA BB BC BD CA CB CC CD DA DB DC DD``
``permutations('ABCD', 2)``                      ``AB AC AD BA BC BD CA CB CD DA DB DC``
``combinations('ABCD', 2)``                      ``AB AC AD BC BD CD``
``combinations_with_replacement('ABCD', 2)``      ``AA AB AC AD BB BC BD CC CD DD``
==============================================   =============================================================


.. _itertools-functions:

Itertool functions
------------------

The following module functions all construct and return iterators. Some provide
streams of infinite length, so they should only be accessed by functions or
loops that truncate the stream.

.. function:: accumulate(iterable[, func, *, initial=None])

    Make an iterator that returns accumulated sums, or accumulated
    results of other binary functions (specified via the optional
    *func* argument).

    If *func* is supplied, it should be a function
    of two arguments. Elements of the input *iterable* may be any type
    that can be accepted as arguments to *func*. (For example, with
    the default operation of addition, elements may be any addable
    type including :class:`~decimal.Decimal` or
    :class:`~fractions.Fraction`.)

    Usually, the number of elements output matches the input iterable.
    However, if the keyword argument *initial* is provided, the
    accumulation leads off with the *initial* value so that the output
    has one more element than the input iterable.

    Roughly equivalent to::

        def accumulate(iterable, func=operator.add, *, initial=None):
            'Return running totals'
            # accumulate([1,2,3,4,5]) --> 1 3 6 10 15
            # accumulate([1,2,3,4,5], initial=100) --> 100 101 103 106 110 115
            # accumulate([1,2,3,4,5], operator.mul) --> 1 2 6 24 120
            it = iter(iterable)
            total = initial
            if initial is None:
                try:
                    total = next(it)
                except StopIteration:
                    return
            yield total
            for element in it:
                total = func(total, element)
                yield total

    There are a number of uses for the *func* argument.  It can be set to
    :func:`min` for a running minimum, :func:`max` for a running maximum, or
    :func:`operator.mul` for a running product.  Amortization tables can be
    built by accumulating interest and applying payments:

    .. doctest::

      >>> data = [3, 4, 6, 2, 1, 9, 0, 7, 5, 8]
      >>> list(accumulate(data, operator.mul))     # running product
      [3, 12, 72, 144, 144, 1296, 0, 0, 0, 0]
      >>> list(accumulate(data, max))              # running maximum
      [3, 4, 6, 6, 6, 9, 9, 9, 9, 9]

      # Amortize a 5% loan of 1000 with 10 annual payments of 90
      >>> account_update = lambda bal, pmt: round(bal * 1.05) + pmt
      >>> list(accumulate(repeat(-90, 10), account_update, initial=1_000))
      [1000, 960, 918, 874, 828, 779, 728, 674, 618, 559, 497]

    See :func:`functools.reduce` for a similar function that returns only the
    final accumulated value.

    .. versionadded:: 3.2

    .. versionchanged:: 3.3
       Added the optional *func* parameter.

    .. versionchanged:: 3.8
       Added the optional *initial* parameter.


.. function:: batched(iterable, n, *, strict=False)

   Batch data from the *iterable* into tuples of length *n*. The last
   batch may be shorter than *n*.

   If *strict* is true, will raise a :exc:`ValueError` if the final
   batch is shorter than *n*.

   Loops over the input iterable and accumulates data into tuples up to
   size *n*.  The input is consumed lazily, just enough to fill a batch.
   The result is yielded as soon as the batch is full or when the input
   iterable is exhausted:

   .. doctest::

      >>> flattened_data = ['roses', 'red', 'violets', 'blue', 'sugar', 'sweet']
      >>> unflattened = list(batched(flattened_data, 2))
      >>> unflattened
      [('roses', 'red'), ('violets', 'blue'), ('sugar', 'sweet')]

      >>> for batch in batched('ABCDEFG', 3):
      ...     print(batch)
      ...
      ('A', 'B', 'C')
      ('D', 'E', 'F')
      ('G',)

   Roughly equivalent to::

      def batched(iterable, n, *, strict=False):
          # batched('ABCDEFG', 3) --> ABC DEF G
          if n < 1:
              raise ValueError('n must be at least one')
          it = iter(iterable)
          while batch := tuple(islice(it, n)):
              if strict and len(batch) != n:
                  raise ValueError('batched(): incomplete batch')
              yield batch

   .. versionadded:: 3.12

   .. versionchanged:: 3.13
      Added the *strict* option.


.. function:: chain(*iterables)

   Make an iterator that returns elements from the first iterable until it is
   exhausted, then proceeds to the next iterable, until all of the iterables are
   exhausted.  Used for treating consecutive sequences as a single sequence.
   Roughly equivalent to::

      def chain(*iterables):
          # chain('ABC', 'DEF') --> A B C D E F
          for it in iterables:
              for element in it:
                  yield element


.. classmethod:: chain.from_iterable(iterable)

   Alternate constructor for :func:`chain`.  Gets chained inputs from a
   single iterable argument that is evaluated lazily.  Roughly equivalent to::

      def from_iterable(iterables):
          # chain.from_iterable(['ABC', 'DEF']) --> A B C D E F
          for it in iterables:
              for element in it:
                  yield element


.. function:: combinations(iterable, r)

   Return *r* length subsequences of elements from the input *iterable*.

   The combination tuples are emitted in lexicographic ordering according to
   the order of the input *iterable*. So, if the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  So if the input elements are unique, there will be no repeated
   values in each combination.

   Roughly equivalent to::

        def combinations(iterable, r):
            # combinations('ABCD', 2) --> AB AC AD BC BD CD
            # combinations(range(4), 3) --> 012 013 023 123
            pool = tuple(iterable)
            n = len(pool)
            if r > n:
                return
            indices = list(range(r))
            yield tuple(pool[i] for i in indices)
            while True:
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

   The number of items returned is ``n! / r! / (n-r)!`` when ``0 <= r <= n``
   or zero when ``r > n``.

.. function:: combinations_with_replacement(iterable, r)

   Return *r* length subsequences of elements from the input *iterable*
   allowing individual elements to be repeated more than once.

   The combination tuples are emitted in lexicographic ordering according to
   the order of the input *iterable*. So, if the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  So if the input elements are unique, the generated combinations
   will also be unique.

   Roughly equivalent to::

        def combinations_with_replacement(iterable, r):
            # combinations_with_replacement('ABC', 2) --> AA AB AC BB BC CC
            pool = tuple(iterable)
            n = len(pool)
            if not n and r:
                return
            indices = [0] * r
            yield tuple(pool[i] for i in indices)
            while True:
                for i in reversed(range(r)):
                    if indices[i] != n - 1:
                        break
                else:
                    return
                indices[i:] = [indices[i] + 1] * (r - i)
                yield tuple(pool[i] for i in indices)

   The code for :func:`combinations_with_replacement` can be also expressed as
   a subsequence of :func:`product` after filtering entries where the elements
   are not in sorted order (according to their position in the input pool)::

        def combinations_with_replacement(iterable, r):
            pool = tuple(iterable)
            n = len(pool)
            for indices in product(range(n), repeat=r):
                if sorted(indices) == list(indices):
                    yield tuple(pool[i] for i in indices)

   The number of items returned is ``(n+r-1)! / r! / (n-1)!`` when ``n > 0``.

   .. versionadded:: 3.1


.. function:: compress(data, selectors)

   Make an iterator that filters elements from *data* returning only those that
   have a corresponding element in *selectors* that evaluates to ``True``.
   Stops when either the *data* or *selectors* iterables has been exhausted.
   Roughly equivalent to::

       def compress(data, selectors):
           # compress('ABCDEF', [1,0,1,0,1,1]) --> A C E F
           return (d for d, s in zip(data, selectors) if s)

   .. versionadded:: 3.1


.. function:: count(start=0, step=1)

   Make an iterator that returns evenly spaced values starting with number *start*. Often
   used as an argument to :func:`map` to generate consecutive data points.
   Also, used with :func:`zip` to add sequence numbers.  Roughly equivalent to::

      def count(start=0, step=1):
          # count(10) --> 10 11 12 13 14 ...
          # count(2.5, 0.5) --> 2.5 3.0 3.5 ...
          n = start
          while True:
              yield n
              n += step

   When counting with floating point numbers, better accuracy can sometimes be
   achieved by substituting multiplicative code such as: ``(start + step * i
   for i in count())``.

   .. versionchanged:: 3.1
      Added *step* argument and allowed non-integer arguments.

.. function:: cycle(iterable)

   Make an iterator returning elements from the iterable and saving a copy of each.
   When the iterable is exhausted, return elements from the saved copy.  Repeats
   indefinitely.  Roughly equivalent to::

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
   start-up time.  Roughly equivalent to::

      def dropwhile(predicate, iterable):
          # dropwhile(lambda x: x<5, [1,4,6,4,1]) --> 6 4 1
          iterable = iter(iterable)
          for x in iterable:
              if not predicate(x):
                  yield x
                  break
          for x in iterable:
              yield x

.. function:: filterfalse(predicate, iterable)

   Make an iterator that filters elements from iterable returning only those for
   which the predicate is false. If *predicate* is ``None``, return the items
   that are false. Roughly equivalent to::

      def filterfalse(predicate, iterable):
          # filterfalse(lambda x: x%2, range(10)) --> 0 2 4 6 8
          if predicate is None:
              predicate = bool
          for x in iterable:
              if not predicate(x):
                  yield x


.. function:: groupby(iterable, key=None)

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

   :func:`groupby` is roughly equivalent to::

      class groupby:
          # [k for k, g in groupby('AAAABBBCCDAABBB')] --> A B C D A B
          # [list(g) for k, g in groupby('AAAABBBCCD')] --> AAAA BBB CC D

          def __init__(self, iterable, key=None):
              if key is None:
                  key = lambda x: x
              self.keyfunc = key
              self.it = iter(iterable)
              self.tgtkey = self.currkey = self.currvalue = object()

          def __iter__(self):
              return self

          def __next__(self):
              self.id = object()
              while self.currkey == self.tgtkey:
                  self.currvalue = next(self.it)    # Exit on StopIteration
                  self.currkey = self.keyfunc(self.currvalue)
              self.tgtkey = self.currkey
              return (self.currkey, self._grouper(self.tgtkey, self.id))

          def _grouper(self, tgtkey, id):
              while self.id is id and self.currkey == tgtkey:
                  yield self.currvalue
                  try:
                      self.currvalue = next(self.it)
                  except StopIteration:
                      return
                  self.currkey = self.keyfunc(self.currvalue)


.. function:: islice(iterable, stop)
              islice(iterable, start, stop[, step])

   Make an iterator that returns selected elements from the iterable. If *start* is
   non-zero, then elements from the iterable are skipped until start is reached.
   Afterward, elements are returned consecutively unless *step* is set higher than
   one which results in items being skipped.  If *stop* is ``None``, then iteration
   continues until the iterator is exhausted, if at all; otherwise, it stops at the
   specified position.

   If *start* is ``None``, then iteration starts at zero. If *step* is ``None``,
   then the step defaults to one.

   Unlike regular slicing, :func:`islice` does not support negative values for
   *start*, *stop*, or *step*.  Can be used to extract related fields from
   data where the internal structure has been flattened (for example, a
   multi-line report may list a name field on every third line).

   Roughly equivalent to::

      def islice(iterable, *args):
          # islice('ABCDEFG', 2) --> A B
          # islice('ABCDEFG', 2, 4) --> C D
          # islice('ABCDEFG', 2, None) --> C D E F G
          # islice('ABCDEFG', 0, None, 2) --> A C E G
          s = slice(*args)
          start, stop, step = s.start or 0, s.stop or sys.maxsize, s.step or 1
          it = iter(range(start, stop, step))
          try:
              nexti = next(it)
          except StopIteration:
              # Consume *iterable* up to the *start* position.
              for i, element in zip(range(start), iterable):
                  pass
              return
          try:
              for i, element in enumerate(iterable):
                  if i == nexti:
                      yield element
                      nexti = next(it)
          except StopIteration:
              # Consume to *stop*.
              for i, element in zip(range(i + 1, stop), iterable):
                  pass


.. function:: pairwise(iterable)

   Return successive overlapping pairs taken from the input *iterable*.

   The number of 2-tuples in the output iterator will be one fewer than the
   number of inputs.  It will be empty if the input iterable has fewer than
   two values.

   Roughly equivalent to::

        def pairwise(iterable):
            # pairwise('ABCDEFG') --> AB BC CD DE EF FG
            a, b = tee(iterable)
            next(b, None)
            return zip(a, b)

   .. versionadded:: 3.10


.. function:: permutations(iterable, r=None)

   Return successive *r* length permutations of elements in the *iterable*.

   If *r* is not specified or is ``None``, then *r* defaults to the length
   of the *iterable* and all possible full-length permutations
   are generated.

   The permutation tuples are emitted in lexicographic order according to
   the order of the input *iterable*. So, if the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  So if the input elements are unique, there will be no repeated
   values within a permutation.

   Roughly equivalent to::

        def permutations(iterable, r=None):
            # permutations('ABCD', 2) --> AB AC AD BA BC BD CA CB CD DA DB DC
            # permutations(range(3)) --> 012 021 102 120 201 210
            pool = tuple(iterable)
            n = len(pool)
            r = n if r is None else r
            if r > n:
                return
            indices = list(range(n))
            cycles = list(range(n, n-r, -1))
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

   The number of items returned is ``n! / (n-r)!`` when ``0 <= r <= n``
   or zero when ``r > n``.

.. function:: product(*iterables, repeat=1)

   Cartesian product of input iterables.

   Roughly equivalent to nested for-loops in a generator expression. For example,
   ``product(A, B)`` returns the same as ``((x,y) for x in A for y in B)``.

   The nested loops cycle like an odometer with the rightmost element advancing
   on every iteration.  This pattern creates a lexicographic ordering so that if
   the input's iterables are sorted, the product tuples are emitted in sorted
   order.

   To compute the product of an iterable with itself, specify the number of
   repetitions with the optional *repeat* keyword argument.  For example,
   ``product(A, repeat=4)`` means the same as ``product(A, A, A, A)``.

   This function is roughly equivalent to the following code, except that the
   actual implementation does not build up intermediate results in memory::

       def product(*args, repeat=1):
           # product('ABCD', 'xy') --> Ax Ay Bx By Cx Cy Dx Dy
           # product(range(2), repeat=3) --> 000 001 010 011 100 101 110 111
           pools = [tuple(pool) for pool in args] * repeat
           result = [[]]
           for pool in pools:
               result = [x+[y] for x in result for y in pool]
           for prod in result:
               yield tuple(prod)

   Before :func:`product` runs, it completely consumes the input iterables,
   keeping pools of values in memory to generate the products.  Accordingly,
   it is only useful with finite inputs.

.. function:: repeat(object[, times])

   Make an iterator that returns *object* over and over again. Runs indefinitely
   unless the *times* argument is specified.

   Roughly equivalent to::

      def repeat(object, times=None):
          # repeat(10, 3) --> 10 10 10
          if times is None:
              while True:
                  yield object
          else:
              for i in range(times):
                  yield object

   A common use for *repeat* is to supply a stream of constant values to *map*
   or *zip*:

   .. doctest::

      >>> list(map(pow, range(10), repeat(2)))
      [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

.. function:: starmap(function, iterable)

   Make an iterator that computes the function using arguments obtained from
   the iterable.  Used instead of :func:`map` when argument parameters are already
   grouped in tuples from a single iterable (when the data has been
   "pre-zipped").

   The difference between :func:`map` and :func:`starmap` parallels the
   distinction between ``function(a,b)`` and ``function(*c)``. Roughly
   equivalent to::

      def starmap(function, iterable):
          # starmap(pow, [(2,5), (3,2), (10,3)]) --> 32 9 1000
          for args in iterable:
              yield function(*args)


.. function:: takewhile(predicate, iterable)

   Make an iterator that returns elements from the iterable as long as the
   predicate is true.  Roughly equivalent to::

      def takewhile(predicate, iterable):
          # takewhile(lambda x: x<5, [1,4,6,4,1]) --> 1 4
          for x in iterable:
              if predicate(x):
                  yield x
              else:
                  break


.. function:: tee(iterable, n=2)

   Return *n* independent iterators from a single iterable.

   The following Python code helps explain what *tee* does (although the actual
   implementation is more complex and uses only a single underlying
   :abbr:`FIFO (first-in, first-out)` queue)::

        def tee(iterable, n=2):
            it = iter(iterable)
            deques = [collections.deque() for i in range(n)]
            def gen(mydeque):
                while True:
                    if not mydeque:             # when the local deque is empty
                        try:
                            newval = next(it)   # fetch a new value and
                        except StopIteration:
                            return
                        for d in deques:        # load it to all the deques
                            d.append(newval)
                    yield mydeque.popleft()
            return tuple(gen(d) for d in deques)

   Once a :func:`tee` has been created, the original *iterable* should not be
   used anywhere else; otherwise, the *iterable* could get advanced without
   the tee objects being informed.

   ``tee`` iterators are not threadsafe. A :exc:`RuntimeError` may be
   raised when simultaneously using iterators returned by the same :func:`tee`
   call, even if the original *iterable* is threadsafe.

   This itertool may require significant auxiliary storage (depending on how
   much temporary data needs to be stored). In general, if one iterator uses
   most or all of the data before another iterator starts, it is faster to use
   :func:`list` instead of :func:`tee`.


.. function:: zip_longest(*iterables, fillvalue=None)

   Make an iterator that aggregates elements from each of the iterables. If the
   iterables are of uneven length, missing values are filled-in with *fillvalue*.
   Iteration continues until the longest iterable is exhausted.  Roughly equivalent to::

      def zip_longest(*args, fillvalue=None):
          # zip_longest('ABCD', 'xy', fillvalue='-') --> Ax By C- D-
          iterators = [iter(it) for it in args]
          num_active = len(iterators)
          if not num_active:
              return
          while True:
              values = []
              for i, it in enumerate(iterators):
                  try:
                      value = next(it)
                  except StopIteration:
                      num_active -= 1
                      if not num_active:
                          return
                      iterators[i] = repeat(fillvalue)
                      value = fillvalue
                  values.append(value)
              yield tuple(values)

   If one of the iterables is potentially infinite, then the :func:`zip_longest`
   function should be wrapped with something that limits the number of calls
   (for example :func:`islice` or :func:`takewhile`).  If not specified,
   *fillvalue* defaults to ``None``.


.. _itertools-recipes:

Itertools Recipes
-----------------

This section shows recipes for creating an extended toolset using the existing
itertools as building blocks.

The primary purpose of the itertools recipes is educational.  The recipes show
various ways of thinking about individual tools — for example, that
``chain.from_iterable`` is related to the concept of flattening.  The recipes
also give ideas about ways that the tools can be combined — for example, how
``compress()`` and ``range()`` can work together.  The recipes also show patterns
for using itertools with the :mod:`operator` and :mod:`collections` modules as
well as with the built-in itertools such as ``map()``, ``filter()``,
``reversed()``, and ``enumerate()``.

A secondary purpose of the recipes is to serve as an incubator.  The
``accumulate()``, ``compress()``, and ``pairwise()`` itertools started out as
recipes.  Currently, the ``sliding_window()`` and ``iter_index()`` recipes
are being tested to see whether they prove their worth.

Substantially all of these recipes and many, many others can be installed from
the `more-itertools project <https://pypi.org/project/more-itertools/>`_ found
on the Python Package Index::

    python -m pip install more-itertools

Many of the recipes offer the same high performance as the underlying toolset.
Superior memory performance is kept by processing elements one at a time
rather than bringing the whole iterable into memory all at once. Code volume is
kept small by linking the tools together in a functional style which helps
eliminate temporary variables.  High speed is retained by preferring
"vectorized" building blocks over the use of for-loops and :term:`generator`\s
which incur interpreter overhead.

.. testcode::

   import collections
   import functools
   import math
   import operator
   import random

   def take(n, iterable):
       "Return first n items of the iterable as a list."
       return list(islice(iterable, n))

   def prepend(value, iterable):
       "Prepend a single value in front of an iterable."
       # prepend(1, [2, 3, 4]) --> 1 2 3 4
       return chain([value], iterable)

   def tabulate(function, start=0):
       "Return function(0), function(1), ..."
       return map(function, count(start))

   def repeatfunc(func, times=None, *args):
       """Repeat calls to func with specified arguments.

       Example:  repeatfunc(random.random)
       """
       if times is None:
           return starmap(func, repeat(args))
       return starmap(func, repeat(args, times))

   def flatten(list_of_lists):
       "Flatten one level of nesting."
       return chain.from_iterable(list_of_lists)

   def ncycles(iterable, n):
       "Returns the sequence elements n times."
       return chain.from_iterable(repeat(tuple(iterable), n))

   def tail(n, iterable):
       "Return an iterator over the last n items."
       # tail(3, 'ABCDEFG') --> E F G
       return iter(collections.deque(iterable, maxlen=n))

   def consume(iterator, n=None):
       "Advance the iterator n-steps ahead. If n is None, consume entirely."
       # Use functions that consume iterators at C speed.
       if n is None:
           # feed the entire iterator into a zero-length deque
           collections.deque(iterator, maxlen=0)
       else:
           # advance to the empty slice starting at position n
           next(islice(iterator, n, n), None)

   def nth(iterable, n, default=None):
       "Returns the nth item or a default value."
       return next(islice(iterable, n, None), default)

   def quantify(iterable, pred=bool):
       "Given a predicate that returns True or False, count the True results."
       return sum(map(pred, iterable))

   def all_equal(iterable):
       "Returns True if all the elements are equal to each other."
       g = groupby(iterable)
       return next(g, True) and not next(g, False)

   def first_true(iterable, default=False, pred=None):
       """Returns the first true value in the iterable.

       If no true value is found, returns *default*

       If *pred* is not None, returns the first item
       for which pred(item) is true.

       """
       # first_true([a,b,c], x) --> a or b or c or x
       # first_true([a,b], x, f) --> a if f(a) else b if f(b) else x
       return next(filter(pred, iterable), default)

   def unique_everseen(iterable, key=None):
       "List unique elements, preserving order. Remember all elements ever seen."
       # unique_everseen('AAAABBBCCDAABBB') --> A B C D
       # unique_everseen('ABBcCAD', str.casefold) --> A B c D
       seen = set()
       if key is None:
           for element in filterfalse(seen.__contains__, iterable):
               seen.add(element)
               yield element
       else:
           for element in iterable:
               k = key(element)
               if k not in seen:
                   seen.add(k)
                   yield element

   def unique_justseen(iterable, key=None):
       "List unique elements, preserving order. Remember only the element just seen."
       # unique_justseen('AAAABBBCCDAABBB') --> A B C D A B
       # unique_justseen('ABBcCAD', str.casefold) --> A B c A D
       if key is None:
           return map(operator.itemgetter(0), groupby(iterable))
       return map(next, map(operator.itemgetter(1), groupby(iterable, key)))

   def iter_index(iterable, value, start=0, stop=None):
       "Return indices where a value occurs in a sequence or iterable."
       # iter_index('AABCADEAF', 'A') --> 0 1 4 7
       seq_index = getattr(iterable, 'index', None)
       if seq_index is None:
           # Slow path for general iterables
           it = islice(iterable, start, stop)
           for i, element in enumerate(it, start):
               if element is value or element == value:
                   yield i
       else:
           # Fast path for sequences
           stop = len(iterable) if stop is None else stop
           i = start - 1
           try:
               while True:
                   yield (i := seq_index(value, i+1, stop))
           except ValueError:
               pass

   def sliding_window(iterable, n):
       "Collect data into overlapping fixed-length chunks or blocks."
       # sliding_window('ABCDEFG', 4) --> ABCD BCDE CDEF DEFG
       it = iter(iterable)
       window = collections.deque(islice(it, n-1), maxlen=n)
       for x in it:
           window.append(x)
           yield tuple(window)

   def grouper(iterable, n, *, incomplete='fill', fillvalue=None):
       "Collect data into non-overlapping fixed-length chunks or blocks."
       # grouper('ABCDEFG', 3, fillvalue='x') --> ABC DEF Gxx
       # grouper('ABCDEFG', 3, incomplete='strict') --> ABC DEF ValueError
       # grouper('ABCDEFG', 3, incomplete='ignore') --> ABC DEF
       args = [iter(iterable)] * n
       match incomplete:
           case 'fill':
               return zip_longest(*args, fillvalue=fillvalue)
           case 'strict':
               return zip(*args, strict=True)
           case 'ignore':
               return zip(*args)
           case _:
               raise ValueError('Expected fill, strict, or ignore')

   def roundrobin(*iterables):
       "Visit input iterables in a cycle until each is exhausted."
       # roundrobin('ABC', 'D', 'EF') --> A D E B F C
       # Recipe credited to George Sakkis
       num_active = len(iterables)
       nexts = cycle(iter(it).__next__ for it in iterables)
       while num_active:
           try:
               for next in nexts:
                   yield next()
           except StopIteration:
               # Remove the iterator we just exhausted from the cycle.
               num_active -= 1
               nexts = cycle(islice(nexts, num_active))

   def partition(pred, iterable):
       """Partition entries into false entries and true entries.

       If *pred* is slow, consider wrapping it with functools.lru_cache().
       """
       # partition(is_odd, range(10)) --> 0 2 4 6 8   and  1 3 5 7 9
       t1, t2 = tee(iterable)
       return filterfalse(pred, t1), filter(pred, t2)

   def subslices(seq):
       "Return all contiguous non-empty subslices of a sequence."
       # subslices('ABCD') --> A AB ABC ABCD B BC BCD C CD D
       slices = starmap(slice, combinations(range(len(seq) + 1), 2))
       return map(operator.getitem, repeat(seq), slices)

   def iter_except(func, exception, first=None):
       """ Call a function repeatedly until an exception is raised.

       Converts a call-until-exception interface to an iterator interface.
       Like builtins.iter(func, sentinel) but uses an exception instead
       of a sentinel to end the loop.

       Priority queue iterator:
           iter_except(functools.partial(heappop, h), IndexError)

       Non-blocking dictionary iterator:
           iter_except(d.popitem, KeyError)

       Non-blocking deque iterator:
           iter_except(d.popleft, IndexError)

       Non-blocking iterator over a producer Queue:
           iter_except(q.get_nowait, Queue.Empty)

       Non-blocking set iterator:
           iter_except(s.pop, KeyError)

       """
       try:
           if first is not None:
               # For database APIs needing an initial call to db.first()
               yield first()
           while True:
               yield func()
       except exception:
           pass

   def before_and_after(predicate, it):
       """ Variant of takewhile() that allows complete
           access to the remainder of the iterator.

           >>> it = iter('ABCdEfGhI')
           >>> all_upper, remainder = before_and_after(str.isupper, it)
           >>> ''.join(all_upper)
           'ABC'
           >>> ''.join(remainder)     # takewhile() would lose the 'd'
           'dEfGhI'

           Note that the true iterator must be fully consumed
           before the remainder iterator can generate valid results.
       """
       it = iter(it)
       transition = []

       def true_iterator():
           for elem in it:
               if predicate(elem):
                   yield elem
               else:
                   transition.append(elem)
                   return

       return true_iterator(), chain(transition, it)


The following recipes have a more mathematical flavor:

.. testcode::

   def powerset(iterable):
       "powerset([1,2,3]) --> () (1,) (2,) (3,) (1,2) (1,3) (2,3) (1,2,3)"
       s = list(iterable)
       return chain.from_iterable(combinations(s, r) for r in range(len(s)+1))

   def sum_of_squares(it):
       "Add up the squares of the input values."
       # sum_of_squares([10, 20, 30]) -> 1400
       return math.sumprod(*tee(it))

   def reshape(matrix, cols):
       "Reshape a 2-D matrix to have a given number of columns."
       # reshape([(0, 1), (2, 3), (4, 5)], 3) -->  (0, 1, 2), (3, 4, 5)
       return batched(chain.from_iterable(matrix), cols, strict=True)

   def transpose(matrix):
       "Swap the rows and columns of a 2-D matrix."
       # transpose([(1, 2, 3), (11, 22, 33)]) --> (1, 11) (2, 22) (3, 33)
       return zip(*matrix, strict=True)

   def matmul(m1, m2):
       "Multiply two matrices."
       # matmul([(7, 5), (3, 5)], [(2, 5), (7, 9)]) --> (49, 80), (41, 60)
       n = len(m2[0])
       return batched(starmap(math.sumprod, product(m1, transpose(m2))), n)

   def convolve(signal, kernel):
       """Discrete linear convolution of two iterables.

       The kernel is fully consumed before the calculations begin.
       The signal is consumed lazily and can be infinite.

       Convolutions are mathematically commutative.
       If the signal and kernel are swapped,
       the output will be the same.

       Article:  https://betterexplained.com/articles/intuitive-convolution/
       Video:    https://www.youtube.com/watch?v=KuXjwB4LzSA
       """
       # convolve(data, [0.25, 0.25, 0.25, 0.25]) --> Moving average (blur)
       # convolve(data, [1/2, 0, -1/2]) --> 1st derivative estimate
       # convolve(data, [1, -2, 1]) --> 2nd derivative estimate
       kernel = tuple(kernel)[::-1]
       n = len(kernel)
       padded_signal = chain(repeat(0, n-1), signal, repeat(0, n-1))
       windowed_signal = sliding_window(padded_signal, n)
       return map(math.sumprod, repeat(kernel), windowed_signal)

   def polynomial_from_roots(roots):
       """Compute a polynomial's coefficients from its roots.

          (x - 5) (x + 4) (x - 3)  expands to:   x³ -4x² -17x + 60
       """
       # polynomial_from_roots([5, -4, 3]) --> [1, -4, -17, 60]
       factors = zip(repeat(1), map(operator.neg, roots))
       return list(functools.reduce(convolve, factors, [1]))

   def polynomial_eval(coefficients, x):
       """Evaluate a polynomial at a specific value.

       Computes with better numeric stability than Horner's method.
       """
       # Evaluate x³ -4x² -17x + 60 at x = 2.5
       # polynomial_eval([1, -4, -17, 60], x=2.5) --> 8.125
       n = len(coefficients)
       if not n:
           return type(x)(0)
       powers = map(pow, repeat(x), reversed(range(n)))
       return math.sumprod(coefficients, powers)

   def polynomial_derivative(coefficients):
       """Compute the first derivative of a polynomial.

          f(x)  =  x³ -4x² -17x + 60
          f'(x) = 3x² -8x  -17
       """
       # polynomial_derivative([1, -4, -17, 60]) -> [3, -8, -17]
       n = len(coefficients)
       powers = reversed(range(1, n))
       return list(map(operator.mul, coefficients, powers))

   def sieve(n):
       "Primes less than n."
       # sieve(30) --> 2 3 5 7 11 13 17 19 23 29
       if n > 2:
           yield 2
       start = 3
       data = bytearray((0, 1)) * (n // 2)
       limit = math.isqrt(n) + 1
       for p in iter_index(data, 1, start, limit):
           yield from iter_index(data, 1, start, p*p)
           data[p*p : n : p+p] = bytes(len(range(p*p, n, p+p)))
           start = p*p
       yield from iter_index(data, 1, start)

   def factor(n):
       "Prime factors of n."
       # factor(99) --> 3 3 11
       # factor(1_000_000_000_000_007) --> 47 59 360620266859
       # factor(1_000_000_000_000_403) --> 1000000000000403
       for prime in sieve(math.isqrt(n) + 1):
           while not n % prime:
               yield prime
               n //= prime
               if n == 1:
                   return
       if n > 1:
           yield n

   def totient(n):
       "Count of natural numbers up to n that are coprime to n."
       # https://mathworld.wolfram.com/TotientFunction.html
       # totient(12) --> 4 because len([1, 5, 7, 11]) == 4
       for p in unique_justseen(factor(n)):
           n -= n // p
       return n


.. doctest::
    :hide:

    These examples no longer appear in the docs but are guaranteed
    to keep working.

    >>> amounts = [120.15, 764.05, 823.14]
    >>> for checknum, amount in zip(count(1200), amounts):
    ...     print('Check %d is for $%.2f' % (checknum, amount))
    ...
    Check 1200 is for $120.15
    Check 1201 is for $764.05
    Check 1202 is for $823.14

    >>> import operator
    >>> for cube in map(operator.pow, range(1,4), repeat(3)):
    ...    print(cube)
    ...
    1
    8
    27

    >>> reportlines = ['EuroPython', 'Roster', '', 'alex', '', 'laura', '', 'martin', '', 'walter', '', 'samuele']
    >>> for name in islice(reportlines, 3, None, 2):
    ...    print(name.title())
    ...
    Alex
    Laura
    Martin
    Walter
    Samuele

    >>> from operator import itemgetter
    >>> d = dict(a=1, b=2, c=1, d=2, e=1, f=2, g=3)
    >>> di = sorted(sorted(d.items()), key=itemgetter(1))
    >>> for k, g in groupby(di, itemgetter(1)):
    ...     print(k, list(map(itemgetter(0), g)))
    ...
    1 ['a', 'c', 'e']
    2 ['b', 'd', 'f']
    3 ['g']

    # Find runs of consecutive numbers using groupby.  The key to the solution
    # is differencing with a range so that consecutive numbers all appear in
    # same group.
    >>> data = [ 1,  4,5,6, 10, 15,16,17,18, 22, 25,26,27,28]
    >>> for k, g in groupby(enumerate(data), lambda t:t[0]-t[1]):
    ...     print(list(map(operator.itemgetter(1), g)))
    ...
    [1]
    [4, 5, 6]
    [10]
    [15, 16, 17, 18]
    [22]
    [25, 26, 27, 28]

    Now, we test all of the itertool recipes

    >>> take(10, count())
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

    >>> list(prepend(1, [2, 3, 4]))
    [1, 2, 3, 4]

    >>> list(enumerate('abc'))
    [(0, 'a'), (1, 'b'), (2, 'c')]

    >>> list(islice(tabulate(lambda x: 2*x), 4))
    [0, 2, 4, 6]

    >>> list(tail(3, 'ABCDEFG'))
    ['E', 'F', 'G']

    >>> it = iter(range(10))
    >>> consume(it, 3)
    >>> next(it)
    3
    >>> consume(it)
    >>> next(it, 'Done')
    'Done'

    >>> nth('abcde', 3)
    'd'

    >>> nth('abcde', 9) is None
    True

    >>> [all_equal(s) for s in ('', 'A', 'AAAA', 'AAAB', 'AAABA')]
    [True, True, True, False, False]

    >>> quantify(range(99), lambda x: x%2==0)
    50

    >>> quantify([True, False, False, True, True])
    3

    >>> quantify(range(12), pred=lambda x: x%2==1)
    6

    >>> a = [[1, 2, 3], [4, 5, 6]]
    >>> list(flatten(a))
    [1, 2, 3, 4, 5, 6]

    >>> list(repeatfunc(pow, 5, 2, 3))
    [8, 8, 8, 8, 8]

    >>> take(5, map(int, repeatfunc(random.random)))
    [0, 0, 0, 0, 0]

    >>> list(ncycles('abc', 3))
    ['a', 'b', 'c', 'a', 'b', 'c', 'a', 'b', 'c']

    >>> sum_of_squares([10, 20, 30])
    1400

    >>> list(reshape([(0, 1), (2, 3), (4, 5)], 3))
    [(0, 1, 2), (3, 4, 5)]
    >>> M = [(0, 1, 2, 3), (4, 5, 6, 7), (8, 9, 10, 11)]
    >>> list(reshape(M, 1))
    [(0,), (1,), (2,), (3,), (4,), (5,), (6,), (7,), (8,), (9,), (10,), (11,)]
    >>> list(reshape(M, 2))
    [(0, 1), (2, 3), (4, 5), (6, 7), (8, 9), (10, 11)]
    >>> list(reshape(M, 3))
    [(0, 1, 2), (3, 4, 5), (6, 7, 8), (9, 10, 11)]
    >>> list(reshape(M, 4))
    [(0, 1, 2, 3), (4, 5, 6, 7), (8, 9, 10, 11)]
    >>> list(reshape(M, 5))
    Traceback (most recent call last):
    ...
    ValueError: batched(): incomplete batch
    >>> list(reshape(M, 6))
    [(0, 1, 2, 3, 4, 5), (6, 7, 8, 9, 10, 11)]
    >>> list(reshape(M, 12))
    [(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)]

    >>> list(transpose([(1, 2, 3), (11, 22, 33)]))
    [(1, 11), (2, 22), (3, 33)]

    >>> list(matmul([(7, 5), (3, 5)], [[2, 5], [7, 9]]))
    [(49, 80), (41, 60)]
    >>> list(matmul([[2, 5], [7, 9], [3, 4]], [[7, 11, 5, 4, 9], [3, 5, 2, 6, 3]]))
    [(29, 47, 20, 38, 33), (76, 122, 53, 82, 90), (33, 53, 23, 36, 39)]

    >>> data = [20, 40, 24, 32, 20, 28, 16]
    >>> list(convolve(data, [0.25, 0.25, 0.25, 0.25]))
    [5.0, 15.0, 21.0, 29.0, 29.0, 26.0, 24.0, 16.0, 11.0, 4.0]
    >>> list(convolve(data, [1, -1]))
    [20, 20, -16, 8, -12, 8, -12, -16]
    >>> list(convolve(data, [1, -2, 1]))
    [20, 0, -36, 24, -20, 20, -20, -4, 16]

    >>> from fractions import Fraction
    >>> from decimal import Decimal
    >>> polynomial_eval([1, -4, -17, 60], x=2)
    18
    >>> x = 2; x**3 - 4*x**2 -17*x + 60
    18
    >>> polynomial_eval([1, -4, -17, 60], x=2.5)
    8.125
    >>> x = 2.5; x**3 - 4*x**2 -17*x + 60
    8.125
    >>> polynomial_eval([1, -4, -17, 60], x=Fraction(2, 3))
    Fraction(1274, 27)
    >>> x = Fraction(2, 3); x**3 - 4*x**2 -17*x + 60
    Fraction(1274, 27)
    >>> polynomial_eval([1, -4, -17, 60], x=Decimal('1.75'))
    Decimal('23.359375')
    >>> x = Decimal('1.75'); x**3 - 4*x**2 -17*x + 60
    Decimal('23.359375')
    >>> polynomial_eval([], 2)
    0
    >>> polynomial_eval([], 2.5)
    0.0
    >>> polynomial_eval([], Fraction(2, 3))
    Fraction(0, 1)
    >>> polynomial_eval([], Decimal('1.75'))
    Decimal('0')
    >>> polynomial_eval([11], 7) == 11
    True
    >>> polynomial_eval([11, 2], 7) == 11 * 7 + 2
    True

    >>> polynomial_from_roots([5, -4, 3])
    [1, -4, -17, 60]
    >>> factored = lambda x: (x - 5) * (x + 4) * (x - 3)
    >>> expanded = lambda x: x**3 -4*x**2 -17*x + 60
    >>> all(factored(x) == expanded(x) for x in range(-10, 11))
    True

    >>> polynomial_derivative([1, -4, -17, 60])
    [3, -8, -17]

    >>> list(iter_index('AABCADEAF', 'A'))
    [0, 1, 4, 7]
    >>> list(iter_index('AABCADEAF', 'B'))
    [2]
    >>> list(iter_index('AABCADEAF', 'X'))
    []
    >>> list(iter_index('', 'X'))
    []
    >>> list(iter_index('AABCADEAF', 'A', 1))
    [1, 4, 7]
    >>> list(iter_index(iter('AABCADEAF'), 'A', 1))
    [1, 4, 7]
    >>> list(iter_index('AABCADEAF', 'A', 2))
    [4, 7]
    >>> list(iter_index(iter('AABCADEAF'), 'A', 2))
    [4, 7]
    >>> list(iter_index('AABCADEAF', 'A', 10))
    []
    >>> list(iter_index(iter('AABCADEAF'), 'A', 10))
    []
    >>> list(iter_index('AABCADEAF', 'A', 1, 7))
    [1, 4]
    >>> list(iter_index(iter('AABCADEAF'), 'A', 1, 7))
    [1, 4]
    >>> # Verify that ValueErrors not swallowed (gh-107208)
    >>> def assert_no_value(iterable, forbidden_value):
    ...     for item in iterable:
    ...         if item == forbidden_value:
    ...             raise ValueError
    ...         yield item
    ...
    >>> list(iter_index(assert_no_value('AABCADEAF', 'B'), 'A'))
    Traceback (most recent call last):
    ...
    ValueError
    >>> # Verify that both paths can find identical NaN values
    >>> x = float('NaN')
    >>> y = float('NaN')
    >>> list(iter_index([0, x, x, y, 0], x))
    [1, 2]
    >>> list(iter_index(iter([0, x, x, y, 0]), x))
    [1, 2]
    >>> # Test list input. Lists do not support None for the stop argument
    >>> list(iter_index(list('AABCADEAF'), 'A'))
    [0, 1, 4, 7]

    >>> list(sieve(30))
    [2, 3, 5, 7, 11, 13, 17, 19, 23, 29]
    >>> small_primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97]
    >>> all(list(sieve(n)) == [p for p in small_primes if p < n] for n in range(101))
    True
    >>> len(list(sieve(100)))
    25
    >>> len(list(sieve(1_000)))
    168
    >>> len(list(sieve(10_000)))
    1229
    >>> len(list(sieve(100_000)))
    9592
    >>> len(list(sieve(1_000_000)))
    78498
    >>> carmichael = {561, 1105, 1729, 2465, 2821, 6601, 8911}  # https://oeis.org/A002997
    >>> set(sieve(10_000)).isdisjoint(carmichael)
    True

    >>> list(factor(99))                    # Code example 1
    [3, 3, 11]
    >>> list(factor(1_000_000_000_000_007)) # Code example 2
    [47, 59, 360620266859]
    >>> list(factor(1_000_000_000_000_403)) # Code example 3
    [1000000000000403]
    >>> list(factor(0))
    []
    >>> list(factor(1))
    []
    >>> list(factor(2))
    [2]
    >>> list(factor(3))
    [3]
    >>> list(factor(4))
    [2, 2]
    >>> list(factor(5))
    [5]
    >>> list(factor(6))
    [2, 3]
    >>> list(factor(7))
    [7]
    >>> list(factor(8))
    [2, 2, 2]
    >>> list(factor(9))
    [3, 3]
    >>> list(factor(10))
    [2, 5]
    >>> list(factor(128_884_753_939))       # large prime
    [128884753939]
    >>> list(factor(999953 * 999983))       # large semiprime
    [999953, 999983]
    >>> list(factor(6 ** 20)) == [2] * 20 + [3] * 20   # large power
    True
    >>> list(factor(909_909_090_909))       # large multiterm composite
    [3, 3, 7, 13, 13, 751, 113797]
    >>> math.prod([3, 3, 7, 13, 13, 751, 113797])
    909909090909
    >>> all(math.prod(factor(n)) == n for n in range(1, 2_000))
    True
    >>> all(set(factor(n)) <= set(sieve(n+1)) for n in range(2_000))
    True
    >>> all(list(factor(n)) == sorted(factor(n)) for n in range(2_000))
    True

    >>> totient(0)  # https://www.wolframalpha.com/input?i=totient+0
    0
    >>> first_totients = [1, 1, 2, 2, 4, 2, 6, 4, 6, 4, 10, 4, 12, 6, 8, 8, 16, 6,
    ... 18, 8, 12, 10, 22, 8, 20, 12, 18, 12, 28, 8, 30, 16, 20, 16, 24, 12, 36, 18,
    ... 24, 16, 40, 12, 42, 20, 24, 22, 46, 16, 42, 20, 32, 24, 52, 18, 40, 24, 36,
    ... 28, 58, 16, 60, 30, 36, 32, 48, 20, 66, 32, 44]  # https://oeis.org/A000010
    ...
    >>> list(map(totient, range(1, 70))) == first_totients
    True
    >>> reference_totient = lambda n: sum(math.gcd(t, n) == 1 for t in range(1, n+1))
    >>> all(totient(n) == reference_totient(n) for n in range(1000))
    True
    >>> totient(128_884_753_939) == 128_884_753_938  # large prime
    True
    >>> totient(999953 * 999983) == 999952 * 999982  # large semiprime
    True
    >>> totient(6 ** 20) == 1 * 2**19 * 2 * 3**19    # repeated primes
    True

    >>> list(flatten([('a', 'b'), (), ('c', 'd', 'e'), ('f',), ('g', 'h', 'i')]))
    ['a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i']

    >>> random.seed(85753098575309)
    >>> list(repeatfunc(random.random, 3))
    [0.16370491282496968, 0.45889608687313455, 0.3747076837820118]
    >>> list(repeatfunc(chr, 3, 65))
    ['A', 'A', 'A']
    >>> list(repeatfunc(pow, 3, 2, 5))
    [32, 32, 32]

    >>> list(grouper('abcdefg', 3, fillvalue='x'))
    [('a', 'b', 'c'), ('d', 'e', 'f'), ('g', 'x', 'x')]

    >>> it = grouper('abcdefg', 3, incomplete='strict')
    >>> next(it)
    ('a', 'b', 'c')
    >>> next(it)
    ('d', 'e', 'f')
    >>> next(it)
    Traceback (most recent call last):
      ...
    ValueError: zip() argument 2 is shorter than argument 1

    >>> list(grouper('abcdefg', n=3, incomplete='ignore'))
    [('a', 'b', 'c'), ('d', 'e', 'f')]

    >>> list(sliding_window('ABCDEFG', 1))
    [('A',), ('B',), ('C',), ('D',), ('E',), ('F',), ('G',)]
    >>> list(sliding_window('ABCDEFG', 2))
    [('A', 'B'), ('B', 'C'), ('C', 'D'), ('D', 'E'), ('E', 'F'), ('F', 'G')]
    >>> list(sliding_window('ABCDEFG', 3))
    [('A', 'B', 'C'), ('B', 'C', 'D'), ('C', 'D', 'E'), ('D', 'E', 'F'), ('E', 'F', 'G')]
    >>> list(sliding_window('ABCDEFG', 4))
    [('A', 'B', 'C', 'D'), ('B', 'C', 'D', 'E'), ('C', 'D', 'E', 'F'), ('D', 'E', 'F', 'G')]
    >>> list(sliding_window('ABCDEFG', 5))
    [('A', 'B', 'C', 'D', 'E'), ('B', 'C', 'D', 'E', 'F'), ('C', 'D', 'E', 'F', 'G')]
    >>> list(sliding_window('ABCDEFG', 6))
    [('A', 'B', 'C', 'D', 'E', 'F'), ('B', 'C', 'D', 'E', 'F', 'G')]
    >>> list(sliding_window('ABCDEFG', 7))
    [('A', 'B', 'C', 'D', 'E', 'F', 'G')]
    >>> list(sliding_window('ABCDEFG', 8))
    []
    >>> try:
    ...     list(sliding_window('ABCDEFG', -1))
    ... except ValueError:
    ...     'zero or negative n not supported'
    ...
    'zero or negative n not supported'
    >>> try:
    ...     list(sliding_window('ABCDEFG', 0))
    ... except ValueError:
    ...     'zero or negative n not supported'
    ...
    'zero or negative n not supported'

    >>> list(roundrobin('abc', 'd', 'ef'))
    ['a', 'd', 'e', 'b', 'f', 'c']

    >>> def is_odd(x):
    ...     return x % 2 == 1

    >>> evens, odds = partition(is_odd, range(10))
    >>> list(evens)
    [0, 2, 4, 6, 8]
    >>> list(odds)
    [1, 3, 5, 7, 9]

    >>> it = iter('ABCdEfGhI')
    >>> all_upper, remainder = before_and_after(str.isupper, it)
    >>> ''.join(all_upper)
    'ABC'
    >>> ''.join(remainder)
    'dEfGhI'

    >>> list(subslices('ABCD'))
    ['A', 'AB', 'ABC', 'ABCD', 'B', 'BC', 'BCD', 'C', 'CD', 'D']

    >>> list(powerset([1,2,3]))
    [(), (1,), (2,), (3,), (1, 2), (1, 3), (2, 3), (1, 2, 3)]

    >>> all(len(list(powerset(range(n)))) == 2**n for n in range(18))
    True

    >>> list(powerset('abcde')) == sorted(sorted(set(powerset('abcde'))), key=len)
    True

    >>> list(unique_everseen('AAAABBBCCDAABBB'))
    ['A', 'B', 'C', 'D']
    >>> list(unique_everseen('ABBCcAD', str.casefold))
    ['A', 'B', 'C', 'D']
    >>> list(unique_everseen('ABBcCAD', str.casefold))
    ['A', 'B', 'c', 'D']

    >>> list(unique_justseen('AAAABBBCCDAABBB'))
    ['A', 'B', 'C', 'D', 'A', 'B']
    >>> list(unique_justseen('ABBCcAD', str.casefold))
    ['A', 'B', 'C', 'A', 'D']
    >>> list(unique_justseen('ABBcCAD', str.casefold))
    ['A', 'B', 'c', 'A', 'D']

    >>> d = dict(a=1, b=2, c=3)
    >>> it = iter_except(d.popitem, KeyError)
    >>> d['d'] = 4
    >>> next(it)
    ('d', 4)
    >>> next(it)
    ('c', 3)
    >>> next(it)
    ('b', 2)
    >>> d['e'] = 5
    >>> next(it)
    ('e', 5)
    >>> next(it)
    ('a', 1)
    >>> next(it, 'empty')
    'empty'

    >>> first_true('ABC0DEF1', '9', str.isdigit)
    '0'


.. testcode::
    :hide:

    # Old recipes and their tests which are guaranteed to continue to work.

    def sumprod(vec1, vec2):
        "Compute a sum of products."
        return sum(starmap(operator.mul, zip(vec1, vec2, strict=True)))

    def dotproduct(vec1, vec2):
        return sum(map(operator.mul, vec1, vec2))

    def pad_none(iterable):
        """Returns the sequence elements and then returns None indefinitely.

        Useful for emulating the behavior of the built-in map() function.
        """
        return chain(iterable, repeat(None))

    def triplewise(iterable):
        "Return overlapping triplets from an iterable"
        # triplewise('ABCDEFG') --> ABC BCD CDE DEF EFG
        for (a, _), (b, c) in pairwise(pairwise(iterable)):
            yield a, b, c

    def nth_combination(iterable, r, index):
        "Equivalent to list(combinations(iterable, r))[index]"
        pool = tuple(iterable)
        n = len(pool)
        c = math.comb(n, r)
        if index < 0:
            index += c
        if index < 0 or index >= c:
            raise IndexError
        result = []
        while r:
            c, n, r = c*r//n, n-1, r-1
            while index >= c:
                index -= c
                c, n = c*(n-r)//n, n-1
            result.append(pool[-1-n])
        return tuple(result)


.. doctest::
    :hide:

    >>> dotproduct([1,2,3], [4,5,6])
    32

    >>> sumprod([1,2,3], [4,5,6])
    32

    >>> list(islice(pad_none('abc'), 0, 6))
    ['a', 'b', 'c', None, None, None]

    >>> list(triplewise('ABCDEFG'))
    [('A', 'B', 'C'), ('B', 'C', 'D'), ('C', 'D', 'E'), ('D', 'E', 'F'), ('E', 'F', 'G')]

    >>> population = 'ABCDEFGH'
    >>> for r in range(len(population) + 1):
    ...     seq = list(combinations(population, r))
    ...     for i in range(len(seq)):
    ...         assert nth_combination(population, r, i) == seq[i]
    ...     for i in range(-len(seq), 0):
    ...         assert nth_combination(population, r, i) == seq[i]

    >>> iterable = 'abcde'
    >>> r = 3
    >>> combos = list(combinations(iterable, r))
    >>> all(nth_combination(iterable, r, i) == comb for i, comb in enumerate(combos))
    True
