:mod:`functools` --- Higher-order functions and operations on callable objects
==============================================================================

.. module:: functools
   :synopsis: Higher-order functions and operations on callable objects.

.. moduleauthor:: Peter Harris <scav@blueyonder.co.uk>
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. moduleauthor:: Nick Coghlan <ncoghlan@gmail.com>
.. moduleauthor:: Łukasz Langa <lukasz@langa.pl>
.. moduleauthor:: Pablo Galindo <pablogsal@gmail.com>
.. sectionauthor:: Peter Harris <scav@blueyonder.co.uk>

**Source code:** :source:`Lib/functools.py`

.. testsetup:: default

   import functools
   from functools import *

--------------

The :mod:`functools` module is for higher-order functions: functions that act on
or return other functions. In general, any callable object can be treated as a
function for the purposes of this module.

The :mod:`functools` module defines the following functions:

.. decorator:: cache(user_function)

   Simple lightweight unbounded function cache.  Sometimes called
   `"memoize" <https://en.wikipedia.org/wiki/Memoization>`_.

   Returns the same as ``lru_cache(maxsize=None)``, creating a thin
   wrapper around a dictionary lookup for the function arguments.  Because it
   never needs to evict old values, this is smaller and faster than
   :func:`lru_cache()` with a size limit.

   For example::

        @cache
        def factorial(n):
            return n * factorial(n-1) if n else 1

        >>> factorial(10)      # no previously cached result, makes 11 recursive calls
        3628800
        >>> factorial(5)       # just looks up cached value result
        120
        >>> factorial(12)      # makes two new recursive calls, the other 10 are cached
        479001600

   .. versionadded:: 3.9


.. decorator:: cached_property(func)

   Transform a method of a class into a property whose value is computed once
   and then cached as a normal attribute for the life of the instance. Similar
   to :func:`property`, with the addition of caching. Useful for expensive
   computed properties of instances that are otherwise effectively immutable.

   Example::

       class DataSet:
           def __init__(self, sequence_of_numbers):
               self._data = sequence_of_numbers

           @cached_property
           def stdev(self):
               return statistics.stdev(self._data)

           @cached_property
           def variance(self):
               return statistics.variance(self._data)

   .. versionadded:: 3.8

   .. note::

      This decorator requires that the ``__dict__`` attribute on each instance
      be a mutable mapping. This means it will not work with some types, such as
      metaclasses (since the ``__dict__`` attributes on type instances are
      read-only proxies for the class namespace), and those that specify
      ``__slots__`` without including ``__dict__`` as one of the defined slots
      (as such classes don't provide a ``__dict__`` attribute at all).


.. function:: cmp_to_key(func)

   Transform an old-style comparison function to a :term:`key function`.  Used
   with tools that accept key functions (such as :func:`sorted`, :func:`min`,
   :func:`max`, :func:`heapq.nlargest`, :func:`heapq.nsmallest`,
   :func:`itertools.groupby`).  This function is primarily used as a transition
   tool for programs being converted from Python 2 which supported the use of
   comparison functions.

   A comparison function is any callable that accept two arguments, compares them,
   and returns a negative number for less-than, zero for equality, or a positive
   number for greater-than.  A key function is a callable that accepts one
   argument and returns another value to be used as the sort key.

   Example::

       sorted(iterable, key=cmp_to_key(locale.strcoll))  # locale-aware sort order

   For sorting examples and a brief sorting tutorial, see :ref:`sortinghowto`.

   .. versionadded:: 3.2


.. decorator:: lru_cache(user_function)
               lru_cache(maxsize=128, typed=False)

   Decorator to wrap a function with a memoizing callable that saves up to the
   *maxsize* most recent calls.  It can save time when an expensive or I/O bound
   function is periodically called with the same arguments.

   Since a dictionary is used to cache results, the positional and keyword
   arguments to the function must be hashable.

   Distinct argument patterns may be considered to be distinct calls with
   separate cache entries.  For example, `f(a=1, b=2)` and `f(b=2, a=1)`
   differ in their keyword argument order and may have two separate cache
   entries.

   If *user_function* is specified, it must be a callable. This allows the
   *lru_cache* decorator to be applied directly to a user function, leaving
   the *maxsize* at its default value of 128::

       @lru_cache
       def count_vowels(sentence):
           sentence = sentence.casefold()
           return sum(sentence.count(vowel) for vowel in 'aeiou')

   If *maxsize* is set to ``None``, the LRU feature is disabled and the cache can
   grow without bound.

   If *typed* is set to true, function arguments of different types will be
   cached separately.  For example, ``f(3)`` and ``f(3.0)`` will be treated
   as distinct calls with distinct results.

   The wrapped function is instrumented with a :func:`cache_parameters`
   function that returns a new :class:`dict` showing the values for *maxsize*
   and *typed*.  This is for information purposes only.  Mutating the values
   has no effect.

   To help measure the effectiveness of the cache and tune the *maxsize*
   parameter, the wrapped function is instrumented with a :func:`cache_info`
   function that returns a :term:`named tuple` showing *hits*, *misses*,
   *maxsize* and *currsize*.  In a multi-threaded environment, the hits
   and misses are approximate.

   The decorator also provides a :func:`cache_clear` function for clearing or
   invalidating the cache.

   The original underlying function is accessible through the
   :attr:`__wrapped__` attribute.  This is useful for introspection, for
   bypassing the cache, or for rewrapping the function with a different cache.

   An `LRU (least recently used) cache
   <https://en.wikipedia.org/wiki/Cache_replacement_policies#Least_recently_used_(LRU)>`_
   works best when the most recent calls are the best predictors of upcoming
   calls (for example, the most popular articles on a news server tend to
   change each day).  The cache's size limit assures that the cache does not
   grow without bound on long-running processes such as web servers.

   In general, the LRU cache should only be used when you want to reuse
   previously computed values.  Accordingly, it doesn't make sense to cache
   functions with side-effects, functions that need to create distinct mutable
   objects on each call, or impure functions such as time() or random().

   Example of an LRU cache for static web content::

        @lru_cache(maxsize=32)
        def get_pep(num):
            'Retrieve text of a Python Enhancement Proposal'
            resource = 'http://www.python.org/dev/peps/pep-%04d/' % num
            try:
                with urllib.request.urlopen(resource) as s:
                    return s.read()
            except urllib.error.HTTPError:
                return 'Not Found'

        >>> for n in 8, 290, 308, 320, 8, 218, 320, 279, 289, 320, 9991:
        ...     pep = get_pep(n)
        ...     print(n, len(pep))

        >>> get_pep.cache_info()
        CacheInfo(hits=3, misses=8, maxsize=32, currsize=8)

   Example of efficiently computing
   `Fibonacci numbers <https://en.wikipedia.org/wiki/Fibonacci_number>`_
   using a cache to implement a
   `dynamic programming <https://en.wikipedia.org/wiki/Dynamic_programming>`_
   technique::

        @lru_cache(maxsize=None)
        def fib(n):
            if n < 2:
                return n
            return fib(n-1) + fib(n-2)

        >>> [fib(n) for n in range(16)]
        [0, 1, 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610]

        >>> fib.cache_info()
        CacheInfo(hits=28, misses=16, maxsize=None, currsize=16)

   .. versionadded:: 3.2

   .. versionchanged:: 3.3
      Added the *typed* option.

   .. versionchanged:: 3.8
      Added the *user_function* option.

   .. versionadded:: 3.9
      Added the function :func:`cache_parameters`

.. decorator:: total_ordering

   Given a class defining one or more rich comparison ordering methods, this
   class decorator supplies the rest.  This simplifies the effort involved
   in specifying all of the possible rich comparison operations:

   The class must define one of :meth:`__lt__`, :meth:`__le__`,
   :meth:`__gt__`, or :meth:`__ge__`.
   In addition, the class should supply an :meth:`__eq__` method.

   For example::

       @total_ordering
       class Student:
           def _is_valid_operand(self, other):
               return (hasattr(other, "lastname") and
                       hasattr(other, "firstname"))
           def __eq__(self, other):
               if not self._is_valid_operand(other):
                   return NotImplemented
               return ((self.lastname.lower(), self.firstname.lower()) ==
                       (other.lastname.lower(), other.firstname.lower()))
           def __lt__(self, other):
               if not self._is_valid_operand(other):
                   return NotImplemented
               return ((self.lastname.lower(), self.firstname.lower()) <
                       (other.lastname.lower(), other.firstname.lower()))

   .. note::

      While this decorator makes it easy to create well behaved totally
      ordered types, it *does* come at the cost of slower execution and
      more complex stack traces for the derived comparison methods. If
      performance benchmarking indicates this is a bottleneck for a given
      application, implementing all six rich comparison methods instead is
      likely to provide an easy speed boost.

   .. versionadded:: 3.2

   .. versionchanged:: 3.4
      Returning NotImplemented from the underlying comparison function for
      unrecognised types is now supported.

.. function:: partial(func, /, *args, **keywords)

   Return a new :ref:`partial object<partial-objects>` which when called
   will behave like *func* called with the positional arguments *args*
   and keyword arguments *keywords*. If more arguments are supplied to the
   call, they are appended to *args*. If additional keyword arguments are
   supplied, they extend and override *keywords*.
   Roughly equivalent to::

      def partial(func, /, *args, **keywords):
          def newfunc(*fargs, **fkeywords):
              newkeywords = {**keywords, **fkeywords}
              return func(*args, *fargs, **newkeywords)
          newfunc.func = func
          newfunc.args = args
          newfunc.keywords = keywords
          return newfunc

   The :func:`partial` is used for partial function application which "freezes"
   some portion of a function's arguments and/or keywords resulting in a new object
   with a simplified signature.  For example, :func:`partial` can be used to create
   a callable that behaves like the :func:`int` function where the *base* argument
   defaults to two:

      >>> from functools import partial
      >>> basetwo = partial(int, base=2)
      >>> basetwo.__doc__ = 'Convert base 2 string to an int.'
      >>> basetwo('10010')
      18


.. class:: partialmethod(func, /, *args, **keywords)

   Return a new :class:`partialmethod` descriptor which behaves
   like :class:`partial` except that it is designed to be used as a method
   definition rather than being directly callable.

   *func* must be a :term:`descriptor` or a callable (objects which are both,
   like normal functions, are handled as descriptors).

   When *func* is a descriptor (such as a normal Python function,
   :func:`classmethod`, :func:`staticmethod`, :func:`abstractmethod` or
   another instance of :class:`partialmethod`), calls to ``__get__`` are
   delegated to the underlying descriptor, and an appropriate
   :ref:`partial object<partial-objects>` returned as the result.

   When *func* is a non-descriptor callable, an appropriate bound method is
   created dynamically. This behaves like a normal Python function when
   used as a method: the *self* argument will be inserted as the first
   positional argument, even before the *args* and *keywords* supplied to
   the :class:`partialmethod` constructor.

   Example::

      >>> class Cell:
      ...     def __init__(self):
      ...         self._alive = False
      ...     @property
      ...     def alive(self):
      ...         return self._alive
      ...     def set_state(self, state):
      ...         self._alive = bool(state)
      ...     set_alive = partialmethod(set_state, True)
      ...     set_dead = partialmethod(set_state, False)
      ...
      >>> c = Cell()
      >>> c.alive
      False
      >>> c.set_alive()
      >>> c.alive
      True

   .. versionadded:: 3.4


.. function:: reduce(function, iterable[, initializer])

   Apply *function* of two arguments cumulatively to the items of *iterable*, from
   left to right, so as to reduce the iterable to a single value.  For example,
   ``reduce(lambda x, y: x+y, [1, 2, 3, 4, 5])`` calculates ``((((1+2)+3)+4)+5)``.
   The left argument, *x*, is the accumulated value and the right argument, *y*, is
   the update value from the *iterable*.  If the optional *initializer* is present,
   it is placed before the items of the iterable in the calculation, and serves as
   a default when the iterable is empty.  If *initializer* is not given and
   *iterable* contains only one item, the first item is returned.

   Roughly equivalent to::

      def reduce(function, iterable, initializer=None):
          it = iter(iterable)
          if initializer is None:
              value = next(it)
          else:
              value = initializer
          for element in it:
              value = function(value, element)
          return value

   See :func:`itertools.accumulate` for an iterator that yields all intermediate
   values.

.. decorator:: singledispatch

   Transform a function into a :term:`single-dispatch <single
   dispatch>` :term:`generic function`.

   To define a generic function, decorate it with the ``@singledispatch``
   decorator. Note that the dispatch happens on the type of the first argument,
   create your function accordingly::

     >>> from functools import singledispatch
     >>> @singledispatch
     ... def fun(arg, verbose=False):
     ...     if verbose:
     ...         print("Let me just say,", end=" ")
     ...     print(arg)

   To add overloaded implementations to the function, use the :func:`register`
   attribute of the generic function.  It is a decorator.  For functions
   annotated with types, the decorator will infer the type of the first
   argument automatically::

     >>> @fun.register
     ... def _(arg: int, verbose=False):
     ...     if verbose:
     ...         print("Strength in numbers, eh?", end=" ")
     ...     print(arg)
     ...
     >>> @fun.register
     ... def _(arg: list, verbose=False):
     ...     if verbose:
     ...         print("Enumerate this:")
     ...     for i, elem in enumerate(arg):
     ...         print(i, elem)

   For code which doesn't use type annotations, the appropriate type
   argument can be passed explicitly to the decorator itself::

     >>> @fun.register(complex)
     ... def _(arg, verbose=False):
     ...     if verbose:
     ...         print("Better than complicated.", end=" ")
     ...     print(arg.real, arg.imag)
     ...


   To enable registering lambdas and pre-existing functions, the
   :func:`register` attribute can be used in a functional form::

     >>> def nothing(arg, verbose=False):
     ...     print("Nothing.")
     ...
     >>> fun.register(type(None), nothing)

   The :func:`register` attribute returns the undecorated function which
   enables decorator stacking, pickling, as well as creating unit tests for
   each variant independently::

     >>> @fun.register(float)
     ... @fun.register(Decimal)
     ... def fun_num(arg, verbose=False):
     ...     if verbose:
     ...         print("Half of your number:", end=" ")
     ...     print(arg / 2)
     ...
     >>> fun_num is fun
     False

   When called, the generic function dispatches on the type of the first
   argument::

     >>> fun("Hello, world.")
     Hello, world.
     >>> fun("test.", verbose=True)
     Let me just say, test.
     >>> fun(42, verbose=True)
     Strength in numbers, eh? 42
     >>> fun(['spam', 'spam', 'eggs', 'spam'], verbose=True)
     Enumerate this:
     0 spam
     1 spam
     2 eggs
     3 spam
     >>> fun(None)
     Nothing.
     >>> fun(1.23)
     0.615

   Where there is no registered implementation for a specific type, its
   method resolution order is used to find a more generic implementation.
   The original function decorated with ``@singledispatch`` is registered
   for the base ``object`` type, which means it is used if no better
   implementation is found.

   If an implementation registered to :term:`abstract base class`, virtual
   subclasses will be dispatched to that implementation::

     >>> from collections.abc import Mapping
     >>> @fun.register
     ... def _(arg: Mapping, verbose=False):
     ...     if verbose:
     ...         print("Keys & Values")
     ...     for key, value in arg.items():
     ...         print(key, "=>", value)
     ...
     >>> fun({"a": "b"})
     a => b

   To check which implementation will the generic function choose for
   a given type, use the ``dispatch()`` attribute::

     >>> fun.dispatch(float)
     <function fun_num at 0x1035a2840>
     >>> fun.dispatch(dict)    # note: default implementation
     <function fun at 0x103fe0000>

   To access all registered implementations, use the read-only ``registry``
   attribute::

    >>> fun.registry.keys()
    dict_keys([<class 'NoneType'>, <class 'int'>, <class 'object'>,
              <class 'decimal.Decimal'>, <class 'list'>,
              <class 'float'>])
    >>> fun.registry[float]
    <function fun_num at 0x1035a2840>
    >>> fun.registry[object]
    <function fun at 0x103fe0000>

   .. versionadded:: 3.4

   .. versionchanged:: 3.7
      The :func:`register` attribute supports using type annotations.


.. class:: singledispatchmethod(func)

   Transform a method into a :term:`single-dispatch <single
   dispatch>` :term:`generic function`.

   To define a generic method, decorate it with the ``@singledispatchmethod``
   decorator. Note that the dispatch happens on the type of the first non-self
   or non-cls argument, create your function accordingly::

    class Negator:
        @singledispatchmethod
        def neg(self, arg):
            raise NotImplementedError("Cannot negate a")

        @neg.register
        def _(self, arg: int):
            return -arg

        @neg.register
        def _(self, arg: bool):
            return not arg

   ``@singledispatchmethod`` supports nesting with other decorators such as
   ``@classmethod``. Note that to allow for ``dispatcher.register``,
   ``singledispatchmethod`` must be the *outer most* decorator. Here is the
   ``Negator`` class with the ``neg`` methods being class bound::

    class Negator:
        @singledispatchmethod
        @classmethod
        def neg(cls, arg):
            raise NotImplementedError("Cannot negate a")

        @neg.register
        @classmethod
        def _(cls, arg: int):
            return -arg

        @neg.register
        @classmethod
        def _(cls, arg: bool):
            return not arg

   The same pattern can be used for other similar decorators: ``staticmethod``,
   ``abstractmethod``, and others.

   .. versionadded:: 3.8


.. class:: TopologicalSorter(graph=None)

   Provides functionality to topologically sort a graph of hashable nodes.

   A topological order is a linear ordering of the vertices in a graph such that
   for every directed edge u -> v from vertex u to vertex v, vertex u comes
   before vertex v in the ordering. For instance, the vertices of the graph may
   represent tasks to be performed, and the edges may represent constraints that
   one task must be performed before another; in this example, a topological
   ordering is just a valid sequence for the tasks. A complete topological
   ordering is possible if and only if the graph has no directed cycles, that
   is, if it is a directed acyclic graph.

   If the optional *graph* argument is provided it must be a dictionary
   representing a directed acyclic graph where the keys are nodes and the values
   are iterables of all predecessors of that node in the graph (the nodes that
   have edges that point to the value in the key). Additional nodes can be added
   to the graph using the :meth:`~TopologicalSorter.add` method.

   In the general case, the steps required to perform the sorting of a given
   graph are as follows:

         * Create an instance of the :class:`TopologicalSorter` with an optional
           initial graph.
         * Add additional nodes to the graph.
         * Call :meth:`~TopologicalSorter.prepare` on the graph.
         * While :meth:`~TopologicalSorter.is_active` is ``True``, iterate over
           the nodes returned by :meth:`~TopologicalSorter.get_ready` and
           process them. Call :meth:`~TopologicalSorter.done` on each node as it
           finishes processing.

   In case just an immediate sorting of the nodes in the graph is required and
   no parallelism is involved, the convenience method
   :meth:`TopologicalSorter.static_order` can be used directly:

   .. doctest::

       >>> graph = {"D": {"B", "C"}, "C": {"A"}, "B": {"A"}}
       >>> ts = TopologicalSorter(graph)
       >>> tuple(ts.static_order())
       ('A', 'C', 'B', 'D')

   The class is designed to easily support parallel processing of the nodes as
   they become ready. For instance::

       topological_sorter = TopologicalSorter()

       # Add nodes to 'topological_sorter'...

       topological_sorter.prepare()
       while topological_sorter.is_active():
           for node in topological_sorter.get_ready():
               # Worker threads or processes take nodes to work on off the
               # 'task_queue' queue.
               task_queue.put(node)

           # When the work for a node is done, workers put the node in
           # 'finalized_tasks_queue' so we can get more nodes to work on.
           # The definition of 'is_active()' guarantees that, at this point, at
           # least one node has been placed on 'task_queue' that hasn't yet
           # been passed to 'done()', so this blocking 'get()' must (eventually)
           # succeed.  After calling 'done()', we loop back to call 'get_ready()'
           # again, so put newly freed nodes on 'task_queue' as soon as
           # logically possible.
           node = finalized_tasks_queue.get()
           topological_sorter.done(node)

   .. method:: add(node, *predecessors)

      Add a new node and its predecessors to the graph. Both the *node* and all
      elements in *predecessors* must be hashable.

      If called multiple times with the same node argument, the set of
      dependencies will be the union of all dependencies passed in.

      It is possible to add a node with no dependencies (*predecessors* is not
      provided) or to provide a dependency twice. If a node that has not been
      provided before is included among *predecessors* it will be automatically
      added to the graph with no predecessors of its own.

      Raises :exc:`ValueError` if called after :meth:`~TopologicalSorter.prepare`.

   .. method:: prepare()

      Mark the graph as finished and check for cycles in the graph. If any cycle
      is detected, :exc:`CycleError` will be raised, but
      :meth:`~TopologicalSorter.get_ready` can still be used to obtain as many
      nodes as possible until cycles block more progress. After a call to this
      function, the graph cannot be modified, and therefore no more nodes can be
      added using :meth:`~TopologicalSorter.add`.

   .. method:: is_active()

      Returns ``True`` if more progress can be made and ``False`` otherwise.
      Progress can be made if cycles do not block the resolution and either
      there are still nodes ready that haven't yet been returned by
      :meth:`TopologicalSorter.get_ready` or the number of nodes marked
      :meth:`TopologicalSorter.done` is less than the number that have been
      returned by :meth:`TopologicalSorter.get_ready`.

      The :meth:`~TopologicalSorter.__bool__` method of this class defers to
      this function, so instead of::

          if ts.is_active():
              ...

      if possible to simply do::

          if ts:
              ...

      Raises :exc:`ValueError` if called without calling
      :meth:`~TopologicalSorter.prepare` previously.

   .. method:: done(*nodes)

      Marks a set of nodes returned by :meth:`TopologicalSorter.get_ready` as
      processed, unblocking any successor of each node in *nodes* for being
      returned in the future by a call to :meth:`TopologicalSorter.get_ready`.

      Raises :exc:`ValueError` if any node in *nodes* has already been marked as
      processed by a previous call to this method or if a node was not added to
      the graph by using :meth:`TopologicalSorter.add`, if called without
      calling :meth:`~TopologicalSorter.prepare` or if node has not yet been
      returned by :meth:`~TopologicalSorter.get_ready`.

   .. method:: get_ready()

      Returns a ``tuple`` with all the nodes that are ready. Initially it
      returns all nodes with no predecessors, and once those are marked as
      processed by calling :meth:`TopologicalSorter.done`, further calls will
      return all new nodes that have all their predecessors already processed.
      Once no more progress can be made, empty tuples are returned.

      Raises :exc:`ValueError` if called without calling
      :meth:`~TopologicalSorter.prepare` previously.

   .. method:: static_order()

      Returns an iterable of nodes in a topological order. Using this method
      does not require to call :meth:`TopologicalSorter.prepare` or
      :meth:`TopologicalSorter.done`. This method is equivalent to::

          def static_order(self):
              self.prepare()
              while self.is_active():
                  node_group = self.get_ready()
                  yield from node_group
                  self.done(*node_group)

      The particular order that is returned may depend on the specific order in
      which the items were inserted in the graph. For example:

      .. doctest::

          >>> ts = TopologicalSorter()
          >>> ts.add(3, 2, 1)
          >>> ts.add(1, 0)
          >>> print([*ts.static_order()])
          [2, 0, 1, 3]

          >>> ts2 = TopologicalSorter()
          >>> ts2.add(1, 0)
          >>> ts2.add(3, 2, 1)
          >>> print([*ts2.static_order()])
          [0, 2, 1, 3]

      This is due to the fact that "0" and "2" are in the same level in the
      graph (they would have been returned in the same call to
      :meth:`~TopologicalSorter.get_ready`) and the order between them is
      determined by the order of insertion.


      If any cycle is detected, :exc:`CycleError` will be raised.

   .. versionadded:: 3.9


.. function:: update_wrapper(wrapper, wrapped, assigned=WRAPPER_ASSIGNMENTS, updated=WRAPPER_UPDATES)

   Update a *wrapper* function to look like the *wrapped* function. The optional
   arguments are tuples to specify which attributes of the original function are
   assigned directly to the matching attributes on the wrapper function and which
   attributes of the wrapper function are updated with the corresponding attributes
   from the original function. The default values for these arguments are the
   module level constants ``WRAPPER_ASSIGNMENTS`` (which assigns to the wrapper
   function's ``__module__``, ``__name__``, ``__qualname__``, ``__annotations__``
   and ``__doc__``, the documentation string) and ``WRAPPER_UPDATES`` (which
   updates the wrapper function's ``__dict__``, i.e. the instance dictionary).

   To allow access to the original function for introspection and other purposes
   (e.g. bypassing a caching decorator such as :func:`lru_cache`), this function
   automatically adds a ``__wrapped__`` attribute to the wrapper that refers to
   the function being wrapped.

   The main intended use for this function is in :term:`decorator` functions which
   wrap the decorated function and return the wrapper. If the wrapper function is
   not updated, the metadata of the returned function will reflect the wrapper
   definition rather than the original function definition, which is typically less
   than helpful.

   :func:`update_wrapper` may be used with callables other than functions. Any
   attributes named in *assigned* or *updated* that are missing from the object
   being wrapped are ignored (i.e. this function will not attempt to set them
   on the wrapper function). :exc:`AttributeError` is still raised if the
   wrapper function itself is missing any attributes named in *updated*.

   .. versionadded:: 3.2
      Automatic addition of the ``__wrapped__`` attribute.

   .. versionadded:: 3.2
      Copying of the ``__annotations__`` attribute by default.

   .. versionchanged:: 3.2
      Missing attributes no longer trigger an :exc:`AttributeError`.

   .. versionchanged:: 3.4
      The ``__wrapped__`` attribute now always refers to the wrapped
      function, even if that function defined a ``__wrapped__`` attribute.
      (see :issue:`17482`)


.. decorator:: wraps(wrapped, assigned=WRAPPER_ASSIGNMENTS, updated=WRAPPER_UPDATES)

   This is a convenience function for invoking :func:`update_wrapper` as a
   function decorator when defining a wrapper function.  It is equivalent to
   ``partial(update_wrapper, wrapped=wrapped, assigned=assigned, updated=updated)``.
   For example::

      >>> from functools import wraps
      >>> def my_decorator(f):
      ...     @wraps(f)
      ...     def wrapper(*args, **kwds):
      ...         print('Calling decorated function')
      ...         return f(*args, **kwds)
      ...     return wrapper
      ...
      >>> @my_decorator
      ... def example():
      ...     """Docstring"""
      ...     print('Called example function')
      ...
      >>> example()
      Calling decorated function
      Called example function
      >>> example.__name__
      'example'
      >>> example.__doc__
      'Docstring'

   Without the use of this decorator factory, the name of the example function
   would have been ``'wrapper'``, and the docstring of the original :func:`example`
   would have been lost.


.. _partial-objects:

:class:`partial` Objects
------------------------

:class:`partial` objects are callable objects created by :func:`partial`. They
have three read-only attributes:


.. attribute:: partial.func

   A callable object or function.  Calls to the :class:`partial` object will be
   forwarded to :attr:`func` with new arguments and keywords.


.. attribute:: partial.args

   The leftmost positional arguments that will be prepended to the positional
   arguments provided to a :class:`partial` object call.


.. attribute:: partial.keywords

   The keyword arguments that will be supplied when the :class:`partial` object is
   called.

:class:`partial` objects are like :class:`function` objects in that they are
callable, weak referencable, and can have attributes.  There are some important
differences.  For instance, the :attr:`~definition.__name__` and :attr:`__doc__` attributes
are not created automatically.  Also, :class:`partial` objects defined in
classes behave like static methods and do not transform into bound methods
during instance attribute look-up.


Exceptions
----------
The :mod:`functools` module defines the following exception classes:

.. exception:: CycleError

   Subclass of :exc:`ValueError` raised by :meth:`TopologicalSorter.prepare` if cycles exist
   in the working graph. If multiple cycles exist, only one undefined choice among them will
   be reported and included in the exception.

   The detected cycle can be accessed via the second element in the :attr:`~CycleError.args`
   attribute of the exception instance and consists in a list of nodes, such that each node is,
   in the graph, an immediate predecessor of the next node in the list. In the reported list,
   the first and the last node will be the same, to make it clear that it is cyclic.
