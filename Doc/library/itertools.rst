:mod:`!itertools` --- Functions creating iterators for efficient looping
========================================================================

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


**Infinite iterators:**

==================  =================       =================================================               =========================================
Iterator            Arguments               Results                                                         Example
==================  =================       =================================================               =========================================
:func:`count`       [start[, step]]         start, start+step, start+2*step, ...                            ``count(10) → 10 11 12 13 14 ...``
:func:`cycle`       p                       p0, p1, ... plast, p0, p1, ...                                  ``cycle('ABCD') → A B C D A B C D ...``
:func:`repeat`      elem [,n]               elem, elem, elem, ... endlessly or up to n times                ``repeat(10, 3) → 10 10 10``
==================  =================       =================================================               =========================================

**Iterators terminating on the shortest input sequence:**

============================    ============================    =================================================   =============================================================
Iterator                        Arguments                       Results                                             Example
============================    ============================    =================================================   =============================================================
:func:`accumulate`              p [,func]                       p0, p0+p1, p0+p1+p2, ...                            ``accumulate([1,2,3,4,5]) → 1 3 6 10 15``
:func:`batched`                 p, n                            (p0, p1, ..., p_n-1), ...                           ``batched('ABCDEFG', n=3) → ABC DEF G``
:func:`chain`                   p, q, ...                       p0, p1, ... plast, q0, q1, ...                      ``chain('ABC', 'DEF') → A B C D E F``
:func:`chain.from_iterable`     iterable                        p0, p1, ... plast, q0, q1, ...                      ``chain.from_iterable(['ABC', 'DEF']) → A B C D E F``
:func:`compress`                data, selectors                 (d[0] if s[0]), (d[1] if s[1]), ...                 ``compress('ABCDEF', [1,0,1,0,1,1]) → A C E F``
:func:`dropwhile`               predicate, seq                  seq[n], seq[n+1], starting when predicate fails     ``dropwhile(lambda x: x<5, [1,4,6,3,8]) → 6 3 8``
:func:`filterfalse`             predicate, seq                  elements of seq where predicate(elem) fails         ``filterfalse(lambda x: x<5, [1,4,6,3,8]) → 6 8``
:func:`groupby`                 iterable[, key]                 sub-iterators grouped by value of key(v)            ``groupby(['A','B','DEF'], len) → (1, A B) (3, DEF)``
:func:`islice`                  seq, [start,] stop [, step]     elements from seq[start:stop:step]                  ``islice('ABCDEFG', 2, None) → C D E F G``
:func:`pairwise`                iterable                        (p[0], p[1]), (p[1], p[2])                          ``pairwise('ABCDEFG') → AB BC CD DE EF FG``
:func:`starmap`                 func, seq                       func(\*seq[0]), func(\*seq[1]), ...                 ``starmap(pow, [(2,5), (3,2), (10,3)]) → 32 9 1000``
:func:`takewhile`               predicate, seq                  seq[0], seq[1], until predicate fails               ``takewhile(lambda x: x<5, [1,4,6,3,8]) → 1 4``
:func:`tee`                     it, n                           it1, it2, ... itn  splits one iterator into n       ``tee('ABC', 2) → A B C, A B C``
:func:`zip_longest`             p, q, ...                       (p[0], q[0]), (p[1], q[1]), ...                     ``zip_longest('ABCD', 'xy', fillvalue='-') → Ax By C- D-``
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
``combinations_with_replacement('ABCD', 2)``     ``AA AB AC AD BB BC BD CC CD DD``
==============================================   =============================================================


.. _itertools-functions:

Itertool Functions
------------------

The following functions all construct and return iterators. Some provide
streams of infinite length, so they should only be accessed by functions or
loops that truncate the stream.


.. function:: accumulate(iterable[, function, *, initial=None])

    Make an iterator that returns accumulated sums or accumulated
    results from other binary functions.

    The *function* defaults to addition.  The *function* should accept
    two arguments, an accumulated total and a value from the *iterable*.

    If an *initial* value is provided, the accumulation will start with
    that value and the output will have one more element than the input
    iterable.

    Roughly equivalent to::

        def accumulate(iterable, function=operator.add, *, initial=None):
            'Return running totals'
            # accumulate([1,2,3,4,5]) → 1 3 6 10 15
            # accumulate([1,2,3,4,5], initial=100) → 100 101 103 106 110 115
            # accumulate([1,2,3,4,5], operator.mul) → 1 2 6 24 120

            iterator = iter(iterable)
            total = initial
            if initial is None:
                try:
                    total = next(iterator)
                except StopIteration:
                    return

            yield total
            for element in iterator:
                total = function(total, element)
                yield total

    To compute a running minimum, set *function* to :func:`min`.
    For a running maximum, set *function* to :func:`max`.
    Or for a running product, set *function* to :func:`operator.mul`.
    To build an `amortization table
    <https://www.ramseysolutions.com/real-estate/amortization-schedule>`_,
    accumulate the interest and apply payments:

    .. doctest::

      >>> data = [3, 4, 6, 2, 1, 9, 0, 7, 5, 8]
      >>> list(accumulate(data, max))              # running maximum
      [3, 4, 6, 6, 6, 9, 9, 9, 9, 9]
      >>> list(accumulate(data, operator.mul))     # running product
      [3, 12, 72, 144, 144, 1296, 0, 0, 0, 0]

      # Amortize a 5% loan of 1000 with 10 annual payments of 90
      >>> update = lambda balance, payment: round(balance * 1.05) - payment
      >>> list(accumulate(repeat(90, 10), update, initial=1_000))
      [1000, 960, 918, 874, 828, 779, 728, 674, 618, 559, 497]

    See :func:`functools.reduce` for a similar function that returns only the
    final accumulated value.

    .. versionadded:: 3.2

    .. versionchanged:: 3.3
       Added the optional *function* parameter.

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

   Roughly equivalent to::

      def batched(iterable, n, *, strict=False):
          # batched('ABCDEFG', 3) → ABC DEF G
          if n < 1:
              raise ValueError('n must be at least one')
          iterator = iter(iterable)
          while batch := tuple(islice(iterator, n)):
              if strict and len(batch) != n:
                  raise ValueError('batched(): incomplete batch')
              yield batch

   .. versionadded:: 3.12

   .. versionchanged:: 3.13
      Added the *strict* option.


.. function:: chain(*iterables)

   Make an iterator that returns elements from the first iterable until
   it is exhausted, then proceeds to the next iterable, until all of the
   iterables are exhausted.  This combines multiple data sources into a
   single iterator.  Roughly equivalent to::

      def chain(*iterables):
          # chain('ABC', 'DEF') → A B C D E F
          for iterable in iterables:
              yield from iterable


.. classmethod:: chain.from_iterable(iterable)

   Alternate constructor for :func:`chain`.  Gets chained inputs from a
   single iterable argument that is evaluated lazily.  Roughly equivalent to::

      def from_iterable(iterables):
          # chain.from_iterable(['ABC', 'DEF']) → A B C D E F
          for iterable in iterables:
              yield from iterable


.. function:: combinations(iterable, r)

   Return *r* length subsequences of elements from the input *iterable*.

   The output is a subsequence of :func:`product` keeping only entries that
   are subsequences of the *iterable*.  The length of the output is given
   by :func:`math.comb` which computes ``n! / r! / (n - r)!`` when ``0 ≤ r
   ≤ n`` or zero when ``r > n``.

   The combination tuples are emitted in lexicographic order according to
   the order of the input *iterable*. If the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  If the input elements are unique, there will be no repeated
   values within each combination.

   Roughly equivalent to::

        def combinations(iterable, r):
            # combinations('ABCD', 2) → AB AC AD BC BD CD
            # combinations(range(4), 3) → 012 013 023 123

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


.. function:: combinations_with_replacement(iterable, r)

   Return *r* length subsequences of elements from the input *iterable*
   allowing individual elements to be repeated more than once.

   The output is a subsequence of :func:`product` that keeps only entries
   that are subsequences (with possible repeated elements) of the
   *iterable*.  The number of subsequence returned is ``(n + r - 1)! / r! /
   (n - 1)!`` when ``n > 0``.

   The combination tuples are emitted in lexicographic order according to
   the order of the input *iterable*. if the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  If the input elements are unique, the generated combinations
   will also be unique.

   Roughly equivalent to::

        def combinations_with_replacement(iterable, r):
            # combinations_with_replacement('ABC', 2) → AA AB AC BB BC CC

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

   .. versionadded:: 3.1


.. function:: compress(data, selectors)

   Make an iterator that returns elements from *data* where the
   corresponding element in *selectors* is true.  Stops when either the
   *data* or *selectors* iterables have been exhausted.  Roughly
   equivalent to::

       def compress(data, selectors):
           # compress('ABCDEF', [1,0,1,0,1,1]) → A C E F
           return (datum for datum, selector in zip(data, selectors) if selector)

   .. versionadded:: 3.1


.. function:: count(start=0, step=1)

   Make an iterator that returns evenly spaced values beginning with
   *start*. Can be used with :func:`map` to generate consecutive data
   points or with :func:`zip` to add sequence numbers.  Roughly
   equivalent to::

      def count(start=0, step=1):
          # count(10) → 10 11 12 13 14 ...
          # count(2.5, 0.5) → 2.5 3.0 3.5 ...
          n = start
          while True:
              yield n
              n += step

   When counting with floating-point numbers, better accuracy can sometimes be
   achieved by substituting multiplicative code such as: ``(start + step * i
   for i in count())``.

   .. versionchanged:: 3.1
      Added *step* argument and allowed non-integer arguments.


.. function:: cycle(iterable)

   Make an iterator returning elements from the *iterable* and saving a
   copy of each.  When the iterable is exhausted, return elements from
   the saved copy.  Repeats indefinitely.  Roughly equivalent to::

      def cycle(iterable):
          # cycle('ABCD') → A B C D A B C D A B C D ...

          saved = []
          for element in iterable:
              yield element
              saved.append(element)

          while saved:
              for element in saved:
                  yield element

   This itertool may require significant auxiliary storage (depending on
   the length of the iterable).


.. function:: dropwhile(predicate, iterable)

   Make an iterator that drops elements from the *iterable* while the
   *predicate* is true and afterwards returns every element.  Roughly
   equivalent to::

      def dropwhile(predicate, iterable):
          # dropwhile(lambda x: x<5, [1,4,6,3,8]) → 6 3 8

          iterator = iter(iterable)
          for x in iterator:
              if not predicate(x):
                  yield x
                  break

          for x in iterator:
              yield x

   Note this does not produce *any* output until the predicate first
   becomes false, so this itertool may have a lengthy start-up time.


.. function:: filterfalse(predicate, iterable)

   Make an iterator that filters elements from the *iterable* returning
   only those for which the *predicate* returns a false value.  If
   *predicate* is ``None``, returns the items that are false.  Roughly
   equivalent to::

      def filterfalse(predicate, iterable):
          # filterfalse(lambda x: x<5, [1,4,6,3,8]) → 6 8

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

      def groupby(iterable, key=None):
          # [k for k, g in groupby('AAAABBBCCDAABBB')] → A B C D A B
          # [list(g) for k, g in groupby('AAAABBBCCD')] → AAAA BBB CC D

          keyfunc = (lambda x: x) if key is None else key
          iterator = iter(iterable)
          exhausted = False

          def _grouper(target_key):
              nonlocal curr_value, curr_key, exhausted
              yield curr_value
              for curr_value in iterator:
                  curr_key = keyfunc(curr_value)
                  if curr_key != target_key:
                      return
                  yield curr_value
              exhausted = True

          try:
              curr_value = next(iterator)
          except StopIteration:
              return
          curr_key = keyfunc(curr_value)

          while not exhausted:
              target_key = curr_key
              curr_group = _grouper(target_key)
              yield curr_key, curr_group
              if curr_key == target_key:
                  for _ in curr_group:
                      pass


.. function:: islice(iterable, stop)
              islice(iterable, start, stop[, step])

   Make an iterator that returns selected elements from the iterable.
   Works like sequence slicing but does not support negative values for
   *start*, *stop*, or *step*.

   If *start* is zero or ``None``, iteration starts at zero.  Otherwise,
   elements from the iterable are skipped until *start* is reached.

   If *stop* is ``None``, iteration continues until the input is
   exhausted, if at all.  Otherwise, it stops at the specified position.

   If *step* is ``None``, the step defaults to one.  Elements are returned
   consecutively unless *step* is set higher than one which results in
   items being skipped.

   Roughly equivalent to::

      def islice(iterable, *args):
          # islice('ABCDEFG', 2) → A B
          # islice('ABCDEFG', 2, 4) → C D
          # islice('ABCDEFG', 2, None) → C D E F G
          # islice('ABCDEFG', 0, None, 2) → A C E G

          s = slice(*args)
          start = 0 if s.start is None else s.start
          stop = s.stop
          step = 1 if s.step is None else s.step
          if start < 0 or (stop is not None and stop < 0) or step <= 0:
              raise ValueError

          indices = count() if stop is None else range(max(start, stop))
          next_i = start
          for i, element in zip(indices, iterable):
              if i == next_i:
                  yield element
                  next_i += step

   If the input is an iterator, then fully consuming the *islice*
   advances the input iterator by ``max(start, stop)`` steps regardless
   of the *step* value.


.. function:: pairwise(iterable)

   Return successive overlapping pairs taken from the input *iterable*.

   The number of 2-tuples in the output iterator will be one fewer than the
   number of inputs.  It will be empty if the input iterable has fewer than
   two values.

   Roughly equivalent to::

        def pairwise(iterable):
            # pairwise('ABCDEFG') → AB BC CD DE EF FG

            iterator = iter(iterable)
            a = next(iterator, None)

            for b in iterator:
                yield a, b
                a = b

   .. versionadded:: 3.10


.. function:: permutations(iterable, r=None)

   Return successive *r* length `permutations of elements
   <https://www.britannica.com/science/permutation>`_ from the *iterable*.

   If *r* is not specified or is ``None``, then *r* defaults to the length
   of the *iterable* and all possible full-length permutations
   are generated.

   The output is a subsequence of :func:`product` where entries with
   repeated elements have been filtered out.  The length of the output is
   given by :func:`math.perm` which computes ``n! / (n - r)!`` when
   ``0 ≤ r ≤ n`` or zero when ``r > n``.

   The permutation tuples are emitted in lexicographic order according to
   the order of the input *iterable*.  If the input *iterable* is sorted,
   the output tuples will be produced in sorted order.

   Elements are treated as unique based on their position, not on their
   value.  If the input elements are unique, there will be no repeated
   values within a permutation.

   Roughly equivalent to::

        def permutations(iterable, r=None):
            # permutations('ABCD', 2) → AB AC AD BA BC BD CA CB CD DA DB DC
            # permutations(range(3)) → 012 021 102 120 201 210

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


.. function:: product(*iterables, repeat=1)

   `Cartesian product <https://en.wikipedia.org/wiki/Cartesian_product>`_
   of the input iterables.

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

       def product(*iterables, repeat=1):
           # product('ABCD', 'xy') → Ax Ay Bx By Cx Cy Dx Dy
           # product(range(2), repeat=3) → 000 001 010 011 100 101 110 111

           if repeat < 0:
               raise ValueError('repeat argument cannot be negative')
           pools = [tuple(pool) for pool in iterables] * repeat

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
          # repeat(10, 3) → 10 10 10
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

   Make an iterator that computes the *function* using arguments obtained
   from the *iterable*.  Used instead of :func:`map` when argument
   parameters have already been "pre-zipped" into tuples.

   The difference between :func:`map` and :func:`starmap` parallels the
   distinction between ``function(a,b)`` and ``function(*c)``. Roughly
   equivalent to::

      def starmap(function, iterable):
          # starmap(pow, [(2,5), (3,2), (10,3)]) → 32 9 1000
          for args in iterable:
              yield function(*args)


.. function:: takewhile(predicate, iterable)

   Make an iterator that returns elements from the *iterable* as long as
   the *predicate* is true.  Roughly equivalent to::

      def takewhile(predicate, iterable):
          # takewhile(lambda x: x<5, [1,4,6,3,8]) → 1 4
          for x in iterable:
              if not predicate(x):
                  break
              yield x

   Note, the element that first fails the predicate condition is
   consumed from the input iterator and there is no way to access it.
   This could be an issue if an application wants to further consume the
   input iterator after *takewhile* has been run to exhaustion.  To work
   around this problem, consider using `more-itertools before_and_after()
   <https://more-itertools.readthedocs.io/en/stable/api.html#more_itertools.before_and_after>`_
   instead.


.. function:: tee(iterable, n=2)

   Return *n* independent iterators from a single iterable.

   Roughly equivalent to::

        def tee(iterable, n=2):
            if n < 0:
                raise ValueError
            if n == 0:
                return ()
            iterator = _tee(iterable)
            result = [iterator]
            for _ in range(n - 1):
                result.append(_tee(iterator))
            return tuple(result)

        class _tee:

            def __init__(self, iterable):
                it = iter(iterable)
                if isinstance(it, _tee):
                    self.iterator = it.iterator
                    self.link = it.link
                else:
                    self.iterator = it
                    self.link = [None, None]

            def __iter__(self):
                return self

            def __next__(self):
                link = self.link
                if link[1] is None:
                    link[0] = next(self.iterator)
                    link[1] = [None, None]
                value, self.link = link
                return value

   When the input *iterable* is already a tee iterator object, all
   members of the return tuple are constructed as if they had been
   produced by the upstream :func:`tee` call.  This "flattening step"
   allows nested :func:`tee` calls to share the same underlying data
   chain and to have a single update step rather than a chain of calls.

   The flattening property makes tee iterators efficiently peekable:

   .. testcode::

      def lookahead(tee_iterator):
           "Return the next value without moving the input forward"
           [forked_iterator] = tee(tee_iterator, 1)
           return next(forked_iterator)

   .. doctest::

      >>> iterator = iter('abcdef')
      >>> [iterator] = tee(iterator, 1)   # Make the input peekable
      >>> next(iterator)                  # Move the iterator forward
      'a'
      >>> lookahead(iterator)             # Check next value
      'b'
      >>> next(iterator)                  # Continue moving forward
      'b'

   ``tee`` iterators are not threadsafe. A :exc:`RuntimeError` may be
   raised when simultaneously using iterators returned by the same :func:`tee`
   call, even if the original *iterable* is threadsafe.

   This itertool may require significant auxiliary storage (depending on how
   much temporary data needs to be stored). In general, if one iterator uses
   most or all of the data before another iterator starts, it is faster to use
   :func:`list` instead of :func:`tee`.


.. function:: zip_longest(*iterables, fillvalue=None)

   Make an iterator that aggregates elements from each of the
   *iterables*.

   If the iterables are of uneven length, missing values are filled-in
   with *fillvalue*.  If not specified, *fillvalue* defaults to ``None``.

   Iteration continues until the longest iterable is exhausted.

   Roughly equivalent to::

      def zip_longest(*iterables, fillvalue=None):
          # zip_longest('ABCD', 'xy', fillvalue='-') → Ax By C- D-

          iterators = list(map(iter, iterables))
          num_active = len(iterators)
          if not num_active:
              return

          while True:
              values = []
              for i, iterator in enumerate(iterators):
                  try:
                      value = next(iterator)
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
   (for example :func:`islice` or :func:`takewhile`).


.. _itertools-recipes:

Itertools Recipes
-----------------

This section shows recipes for creating an extended toolset using the existing
itertools as building blocks.

The primary purpose of the itertools recipes is educational.  The recipes show
various ways of thinking about individual tools — for example, that
``chain.from_iterable`` is related to the concept of flattening.  The recipes
also give ideas about ways that the tools can be combined — for example, how
``starmap()`` and ``repeat()`` can work together.  The recipes also show patterns
for using itertools with the :mod:`operator` and :mod:`collections` modules as
well as with the built-in itertools such as ``map()``, ``filter()``,
``reversed()``, and ``enumerate()``.

A secondary purpose of the recipes is to serve as an incubator.  The
``accumulate()``, ``compress()``, and ``pairwise()`` itertools started out as
recipes.  Currently, the ``sliding_window()``, ``iter_index()``, and ``sieve()``
recipes are being tested to see whether they prove their worth.

Substantially all of these recipes and many, many others can be installed from
the :pypi:`more-itertools` project found
on the Python Package Index::

    python -m pip install more-itertools

Many of the recipes offer the same high performance as the underlying toolset.
Superior memory performance is kept by processing elements one at a time rather
than bringing the whole iterable into memory all at once. Code volume is kept
small by linking the tools together in a `functional style
<https://www.cs.kent.ac.uk/people/staff/dat/miranda/whyfp90.pdf>`_.  High speed
is retained by preferring "vectorized" building blocks over the use of for-loops
and :term:`generators <generator>` which incur interpreter overhead.

.. testcode::

   from collections import Counter, deque
   from contextlib import suppress
   from functools import reduce
   from math import comb, prod, sumprod, isqrt
   from operator import itemgetter, getitem, mul, neg

   def take(n, iterable):
       "Return first n items of the iterable as a list."
       return list(islice(iterable, n))

   def prepend(value, iterable):
       "Prepend a single value in front of an iterable."
       # prepend(1, [2, 3, 4]) → 1 2 3 4
       return chain([value], iterable)

   def tabulate(function, start=0):
       "Return function(0), function(1), ..."
       return map(function, count(start))

   def repeatfunc(function, times=None, *args):
       "Repeat calls to a function with specified arguments."
       if times is None:
           return starmap(function, repeat(args))
       return starmap(function, repeat(args, times))

   def flatten(list_of_lists):
       "Flatten one level of nesting."
       return chain.from_iterable(list_of_lists)

   def ncycles(iterable, n):
       "Returns the sequence elements n times."
       return chain.from_iterable(repeat(tuple(iterable), n))

   def loops(n):
       "Loop n times. Like range(n) but without creating integers."
       # for _ in loops(100): ...
       return repeat(None, n)

   def tail(n, iterable):
       "Return an iterator over the last n items."
       # tail(3, 'ABCDEFG') → E F G
       return iter(deque(iterable, maxlen=n))

   def consume(iterator, n=None):
       "Advance the iterator n-steps ahead. If n is None, consume entirely."
       # Use functions that consume iterators at C speed.
       if n is None:
           deque(iterator, maxlen=0)
       else:
           next(islice(iterator, n, n), None)

   def nth(iterable, n, default=None):
       "Returns the nth item or a default value."
       return next(islice(iterable, n, None), default)

   def quantify(iterable, predicate=bool):
       "Given a predicate that returns True or False, count the True results."
       return sum(map(predicate, iterable))

   def first_true(iterable, default=False, predicate=None):
       "Returns the first true value or the *default* if there is no true value."
       # first_true([a,b,c], x) → a or b or c or x
       # first_true([a,b], x, f) → a if f(a) else b if f(b) else x
       return next(filter(predicate, iterable), default)

   def all_equal(iterable, key=None):
       "Returns True if all the elements are equal to each other."
       # all_equal('4٤௪౪໔', key=int) → True
       return len(take(2, groupby(iterable, key))) <= 1

   def unique_justseen(iterable, key=None):
       "Yield unique elements, preserving order. Remember only the element just seen."
       # unique_justseen('AAAABBBCCDAABBB') → A B C D A B
       # unique_justseen('ABBcCAD', str.casefold) → A B c A D
       if key is None:
           return map(itemgetter(0), groupby(iterable))
       return map(next, map(itemgetter(1), groupby(iterable, key)))

   def unique_everseen(iterable, key=None):
       "Yield unique elements, preserving order. Remember all elements ever seen."
       # unique_everseen('AAAABBBCCDAABBB') → A B C D
       # unique_everseen('ABBcCAD', str.casefold) → A B c D
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

   def unique(iterable, key=None, reverse=False):
      "Yield unique elements in sorted order. Supports unhashable inputs."
      # unique([[1, 2], [3, 4], [1, 2]]) → [1, 2] [3, 4]
      sequenced = sorted(iterable, key=key, reverse=reverse)
      return unique_justseen(sequenced, key=key)

   def sliding_window(iterable, n):
       "Collect data into overlapping fixed-length chunks or blocks."
       # sliding_window('ABCDEFG', 4) → ABCD BCDE CDEF DEFG
       iterator = iter(iterable)
       window = deque(islice(iterator, n - 1), maxlen=n)
       for x in iterator:
           window.append(x)
           yield tuple(window)

   def grouper(iterable, n, *, incomplete='fill', fillvalue=None):
       "Collect data into non-overlapping fixed-length chunks or blocks."
       # grouper('ABCDEFG', 3, fillvalue='x') → ABC DEF Gxx
       # grouper('ABCDEFG', 3, incomplete='strict') → ABC DEF ValueError
       # grouper('ABCDEFG', 3, incomplete='ignore') → ABC DEF
       iterators = [iter(iterable)] * n
       match incomplete:
           case 'fill':
               return zip_longest(*iterators, fillvalue=fillvalue)
           case 'strict':
               return zip(*iterators, strict=True)
           case 'ignore':
               return zip(*iterators)
           case _:
               raise ValueError('Expected fill, strict, or ignore')

   def roundrobin(*iterables):
       "Visit input iterables in a cycle until each is exhausted."
       # roundrobin('ABC', 'D', 'EF') → A D E B F C
       # Algorithm credited to George Sakkis
       iterators = map(iter, iterables)
       for num_active in range(len(iterables), 0, -1):
           iterators = cycle(islice(iterators, num_active))
           yield from map(next, iterators)

   def subslices(seq):
       "Return all contiguous non-empty subslices of a sequence."
       # subslices('ABCD') → A AB ABC ABCD B BC BCD C CD D
       slices = starmap(slice, combinations(range(len(seq) + 1), 2))
       return map(getitem, repeat(seq), slices)

   def iter_index(iterable, value, start=0, stop=None):
       "Return indices where a value occurs in a sequence or iterable."
       # iter_index('AABCADEAF', 'A') → 0 1 4 7
       seq_index = getattr(iterable, 'index', None)
       if seq_index is None:
           iterator = islice(iterable, start, stop)
           for i, element in enumerate(iterator, start):
               if element is value or element == value:
                   yield i
       else:
           stop = len(iterable) if stop is None else stop
           i = start
           with suppress(ValueError):
               while True:
                   yield (i := seq_index(value, i, stop))
                   i += 1

   def iter_except(function, exception, first=None):
       "Convert a call-until-exception interface to an iterator interface."
       # iter_except(d.popitem, KeyError) → non-blocking dictionary iterator
       with suppress(exception):
           if first is not None:
               yield first()
           while True:
               yield function()


The following recipes have a more mathematical flavor:

.. testcode::

   def multinomial(*counts):
       "Number of distinct arrangements of a multiset."
       # Counter('abracadabra').values() → 5 2 2 1 1
       # multinomial(5, 2, 2, 1, 1) → 83160
       return prod(map(comb, accumulate(counts), counts))

   def powerset(iterable):
       "Subsequences of the iterable from shortest to longest."
       # powerset([1,2,3]) → () (1,) (2,) (3,) (1,2) (1,3) (2,3) (1,2,3)
       s = list(iterable)
       return chain.from_iterable(combinations(s, r) for r in range(len(s)+1))

   def sum_of_squares(iterable):
       "Add up the squares of the input values."
       # sum_of_squares([10, 20, 30]) → 1400
       return sumprod(*tee(iterable))

   def reshape(matrix, columns):
       "Reshape a 2-D matrix to have a given number of columns."
       # reshape([(0, 1), (2, 3), (4, 5)], 3) →  (0, 1, 2), (3, 4, 5)
       return batched(chain.from_iterable(matrix), columns, strict=True)

   def transpose(matrix):
       "Swap the rows and columns of a 2-D matrix."
       # transpose([(1, 2, 3), (11, 22, 33)]) → (1, 11) (2, 22) (3, 33)
       return zip(*matrix, strict=True)

   def matmul(m1, m2):
       "Multiply two matrices."
       # matmul([(7, 5), (3, 5)], [(2, 5), (7, 9)]) → (49, 80), (41, 60)
       n = len(m2[0])
       return batched(starmap(sumprod, product(m1, transpose(m2))), n)

   def convolve(signal, kernel):
       """Discrete linear convolution of two iterables.
       Equivalent to polynomial multiplication.

       Convolutions are mathematically commutative; however, the inputs are
       evaluated differently.  The signal is consumed lazily and can be
       infinite. The kernel is fully consumed before the calculations begin.

       Article:  https://betterexplained.com/articles/intuitive-convolution/
       Video:    https://www.youtube.com/watch?v=KuXjwB4LzSA
       """
       # convolve([1, -1, -20], [1, -3]) → 1 -4 -17 60
       # convolve(data, [0.25, 0.25, 0.25, 0.25]) → Moving average (blur)
       # convolve(data, [1/2, 0, -1/2]) → 1st derivative estimate
       # convolve(data, [1, -2, 1]) → 2nd derivative estimate
       kernel = tuple(kernel)[::-1]
       n = len(kernel)
       padded_signal = chain(repeat(0, n-1), signal, repeat(0, n-1))
       windowed_signal = sliding_window(padded_signal, n)
       return map(sumprod, repeat(kernel), windowed_signal)

   def polynomial_from_roots(roots):
       """Compute a polynomial's coefficients from its roots.

          (x - 5) (x + 4) (x - 3)  expands to:   x³ -4x² -17x + 60
       """
       # polynomial_from_roots([5, -4, 3]) → [1, -4, -17, 60]
       factors = zip(repeat(1), map(neg, roots))
       return list(reduce(convolve, factors, [1]))

   def polynomial_eval(coefficients, x):
       """Evaluate a polynomial at a specific value.

       Computes with better numeric stability than Horner's method.
       """
       # Evaluate x³ -4x² -17x + 60 at x = 5
       # polynomial_eval([1, -4, -17, 60], x=5) → 0
       n = len(coefficients)
       if not n:
           return type(x)(0)
       powers = map(pow, repeat(x), reversed(range(n)))
       return sumprod(coefficients, powers)

   def polynomial_derivative(coefficients):
       """Compute the first derivative of a polynomial.

          f(x)  =  x³ -4x² -17x + 60
          f'(x) = 3x² -8x  -17
       """
       # polynomial_derivative([1, -4, -17, 60]) → [3, -8, -17]
       n = len(coefficients)
       powers = reversed(range(1, n))
       return list(map(mul, coefficients, powers))

   def sieve(n):
       "Primes less than n."
       # sieve(30) → 2 3 5 7 11 13 17 19 23 29
       if n > 2:
           yield 2
       data = bytearray((0, 1)) * (n // 2)
       for p in iter_index(data, 1, start=3, stop=isqrt(n) + 1):
           data[p*p : n : p+p] = bytes(len(range(p*p, n, p+p)))
       yield from iter_index(data, 1, start=3)

   def factor(n):
       "Prime factors of n."
       # factor(99) → 3 3 11
       # factor(1_000_000_000_000_007) → 47 59 360620266859
       # factor(1_000_000_000_000_403) → 1000000000000403
       for prime in sieve(isqrt(n) + 1):
           while not n % prime:
               yield prime
               n //= prime
               if n == 1:
                   return
       if n > 1:
           yield n

   def is_prime(n):
       "Return True if n is prime."
       # is_prime(1_000_000_000_000_403) → True
       return n > 1 and next(factor(n)) == n

   def totient(n):
       "Count of natural numbers up to n that are coprime to n."
       # https://mathworld.wolfram.com/TotientFunction.html
       # totient(12) → 4 because len([1, 5, 7, 11]) == 4
       for prime in set(factor(n)):
           n -= n // prime
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
    >>> # Verify that the input is consumed lazily
    >>> it = iter('abcdef')
    >>> take(3, it)
    ['a', 'b', 'c']
    >>> list(it)
    ['d', 'e', 'f']


    >>> list(prepend(1, [2, 3, 4]))
    [1, 2, 3, 4]


    >>> list(enumerate('abc'))
    [(0, 'a'), (1, 'b'), (2, 'c')]


    >>> list(islice(tabulate(lambda x: 2*x), 4))
    [0, 2, 4, 6]


    >>> for _ in loops(5):
    ...     print('hi')
    ...
    hi
    hi
    hi
    hi
    hi


    >>> list(tail(3, 'ABCDEFG'))
    ['E', 'F', 'G']
    >>> # Verify the input is consumed greedily
    >>> input_iterator = iter('ABCDEFG')
    >>> output_iterator = tail(3, input_iterator)
    >>> list(input_iterator)
    []


    >>> it = iter(range(10))
    >>> consume(it, 3)
    >>> # Verify the input is consumed lazily
    >>> next(it)
    3
    >>> # Verify the input is consumed completely
    >>> consume(it)
    >>> next(it, 'Done')
    'Done'


    >>> nth('abcde', 3)
    'd'
    >>> nth('abcde', 9) is None
    True
    >>> # Verify that the input is consumed lazily
    >>> it = iter('abcde')
    >>> nth(it, 2)
    'c'
    >>> list(it)
    ['d', 'e']


    >>> [all_equal(s) for s in ('', 'A', 'AAAA', 'AAAB', 'AAABA')]
    [True, True, True, False, False]
    >>> [all_equal(s, key=str.casefold) for s in ('', 'A', 'AaAa', 'AAAB', 'AAABA')]
    [True, True, True, False, False]
    >>> # Verify that the input is consumed lazily and that only
    >>> # one element of a second equivalence class is used to disprove
    >>> # the assertion that all elements are equal.
    >>> it = iter('aaabbbccc')
    >>> all_equal(it)
    False
    >>> ''.join(it)
    'bbccc'


    >>> quantify(range(99), lambda x: x%2==0)
    50
    >>> quantify([True, False, False, True, True])
    3
    >>> quantify(range(12), predicate=lambda x: x%2==1)
    6


    >>> a = [[1, 2, 3], [4, 5, 6]]
    >>> list(flatten(a))
    [1, 2, 3, 4, 5, 6]


    >>> list(ncycles('abc', 3))
    ['a', 'b', 'c', 'a', 'b', 'c', 'a', 'b', 'c']
    >>> # Verify greedy consumption of input iterator
    >>> input_iterator = iter('abc')
    >>> output_iterator = ncycles(input_iterator, 3)
    >>> list(input_iterator)
    []


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
    >>> # Verify that the inputs are consumed lazily
    >>> input1 = iter([1, 2, 3])
    >>> input2 = iter([11, 22, 33])
    >>> output_iterator = transpose([input1, input2])
    >>> next(output_iterator)
    (1, 11)
    >>> list(zip(input1, input2))
    [(2, 22), (3, 33)]


    >>> list(matmul([(7, 5), (3, 5)], [[2, 5], [7, 9]]))
    [(49, 80), (41, 60)]
    >>> list(matmul([[2, 5], [7, 9], [3, 4]], [[7, 11, 5, 4, 9], [3, 5, 2, 6, 3]]))
    [(29, 47, 20, 38, 33), (76, 122, 53, 82, 90), (33, 53, 23, 36, 39)]


    >>> list(convolve([1, -1, -20], [1, -3])) == [1, -4, -17, 60]
    True
    >>> data = [20, 40, 24, 32, 20, 28, 16]
    >>> list(convolve(data, [0.25, 0.25, 0.25, 0.25]))
    [5.0, 15.0, 21.0, 29.0, 29.0, 26.0, 24.0, 16.0, 11.0, 4.0]
    >>> list(convolve(data, [1, -1]))
    [20, 20, -16, 8, -12, 8, -12, -16]
    >>> list(convolve(data, [1, -2, 1]))
    [20, 0, -36, 24, -20, 20, -20, -4, 16]
    >>> # Verify signal is consumed lazily and the kernel greedily
    >>> signal_iterator = iter([10, 20, 30, 40, 50])
    >>> kernel_iterator = iter([1, 2, 3])
    >>> output_iterator = convolve(signal_iterator, kernel_iterator)
    >>> list(kernel_iterator)
    []
    >>> next(output_iterator)
    10
    >>> next(output_iterator)
    40
    >>> list(signal_iterator)
    [30, 40, 50]


    >>> from fractions import Fraction
    >>> from decimal import Decimal
    >>> polynomial_eval([1, -4, -17, 60], x=5)
    0
    >>> x = 5; x**3 - 4*x**2 -17*x + 60
    0
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
    >>> # Verify that input is consumed lazily
    >>> input_iterator = iter('AABCADEAF')
    >>> output_iterator = iter_index(input_iterator, 'A')
    >>> next(output_iterator)
    0
    >>> next(output_iterator)
    1
    >>> next(output_iterator)
    4
    >>> ''.join(input_iterator)
    'DEAF'


    >>> # Verify that the target value can be a sequence.
    >>> seq = [[10, 20], [30, 40], 30, 40, [30, 40], 50]
    >>> target = [30, 40]
    >>> list(iter_index(seq, target))
    [1, 4]


    >>> # Verify faithfulness to type specific index() method behaviors.
    >>> # For example, bytes and str perform continuous-subsequence searches
    >>> # that do not match the general behavior specified
    >>> # in collections.abc.Sequence.index().
    >>> seq = 'abracadabra'
    >>> target = 'ab'
    >>> list(iter_index(seq, target))
    [0, 7]


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


    >>> small_primes = [2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97]
    >>> list(filter(is_prime, range(-100, 100))) == small_primes
    True
    >>> carmichael = {561, 1105, 1729, 2465, 2821, 6601, 8911}  # https://oeis.org/A002997
    >>> any(map(is_prime, carmichael))
    False
    >>> # https://www.wolframalpha.com/input?i=is+128884753939+prime
    >>> is_prime(128_884_753_939)           # large prime
    True
    >>> is_prime(999953 * 999983)           # large semiprime
    False
    >>> is_prime(1_000_000_000_000_007)     # factor() example
    False
    >>> is_prime(1_000_000_000_000_403)     # factor() example
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


    >>> list(repeatfunc(pow, 5, 2, 3))
    [8, 8, 8, 8, 8]
    >>> take(5, map(int, repeatfunc(random.random)))
    [0, 0, 0, 0, 0]
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
    >>> ranges = [range(5, 1000), range(4, 3000), range(0), range(3, 2000), range(2, 5000), range(1, 3500)]
    >>> collections.Counter(roundrobin(*ranges)) == collections.Counter(chain(*ranges))
    True
    >>> # Verify that the inputs are consumed lazily
    >>> input_iterators = list(map(iter, ['abcd', 'ef', '', 'ghijk', 'l', 'mnopqr']))
    >>> output_iterator = roundrobin(*input_iterators)
    >>> ''.join(islice(output_iterator, 10))
    'aeglmbfhnc'
    >>> ''.join(chain(*input_iterators))
    'dijkopqr'


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
    >>> # Verify that the input is consumed lazily
    >>> input_iterator = iter('AAAABBBCCDAABBB')
    >>> output_iterator = unique_everseen(input_iterator)
    >>> next(output_iterator)
    'A'
    >>> ''.join(input_iterator)
    'AAABBBCCDAABBB'


    >>> list(unique_justseen('AAAABBBCCDAABBB'))
    ['A', 'B', 'C', 'D', 'A', 'B']
    >>> list(unique_justseen('ABBCcAD', str.casefold))
    ['A', 'B', 'C', 'A', 'D']
    >>> list(unique_justseen('ABBcCAD', str.casefold))
    ['A', 'B', 'c', 'A', 'D']
    >>> # Verify that the input is consumed lazily
    >>> input_iterator = iter('AAAABBBCCDAABBB')
    >>> output_iterator = unique_justseen(input_iterator)
    >>> next(output_iterator)
    'A'
    >>> ''.join(input_iterator)
    'AAABBBCCDAABBB'


    >>> list(unique([[1, 2], [3, 4], [1, 2]]))
    [[1, 2], [3, 4]]
    >>> list(unique('ABBcCAD', str.casefold))
    ['A', 'B', 'c', 'D']
    >>> list(unique('ABBcCAD', str.casefold, reverse=True))
    ['D', 'c', 'B', 'A']


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
    >>> # Verify that inputs are consumed lazily
    >>> it = iter('ABC0DEF1')
    >>> first_true(it, predicate=str.isdigit)
    '0'
    >>> ''.join(it)
    'DEF1'

    >>> multinomial(5, 2, 2, 1, 1)
    83160
    >>> word = 'coffee'
    >>> multinomial(*Counter(word).values()) == len(set(permutations(word)))
    True


.. testcode::
    :hide:

    # Old recipes and their tests which are guaranteed to continue to work.

    def old_sumprod_recipe(vec1, vec2):
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
        # triplewise('ABCDEFG') → ABC BCD CDE DEF EFG
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

    def partition(predicate, iterable):
        """Partition entries into false entries and true entries.

        If *predicate* is slow, consider wrapping it with functools.lru_cache().
        """
        # partition(is_odd, range(10)) → 0 2 4 6 8   and  1 3 5 7 9
        t1, t2 = tee(iterable)
        return filterfalse(predicate, t1), filter(predicate, t2)



.. doctest::
    :hide:

    >>> dotproduct([1,2,3], [4,5,6])
    32


    >>> old_sumprod_recipe([1,2,3], [4,5,6])
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
    ...
    >>> iterable = 'abcde'
    >>> r = 3
    >>> combos = list(combinations(iterable, r))
    >>> all(nth_combination(iterable, r, i) == comb for i, comb in enumerate(combos))
    True


    >>> it = iter('ABCdEfGhI')
    >>> all_upper, remainder = before_and_after(str.isupper, it)
    >>> ''.join(all_upper)
    'ABC'
    >>> ''.join(remainder)
    'dEfGhI'


    >>> def is_odd(x):
    ...     return x % 2 == 1
    ...
    >>> evens, odds = partition(is_odd, range(10))
    >>> list(evens)
    [0, 2, 4, 6, 8]
    >>> list(odds)
    [1, 3, 5, 7, 9]
    >>> # Verify that the input is consumed lazily
    >>> input_iterator = iter(range(10))
    >>> evens, odds = partition(is_odd, input_iterator)
    >>> next(odds)
    1
    >>> next(odds)
    3
    >>> next(evens)
    0
    >>> list(input_iterator)
    [4, 5, 6, 7, 8, 9]
