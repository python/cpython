:mod:`functools` --- Higher-order functions and operations on callable objects
==============================================================================

.. module:: functools
   :synopsis: Higher-order functions and operations on callable objects.

.. moduleauthor:: Peter Harris <scav@blueyonder.co.uk>
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. moduleauthor:: Nick Coghlan <ncoghlan@gmail.com>
.. moduleauthor:: Łukasz Langa <lukasz@langa.pl>
.. sectionauthor:: Peter Harris <scav@blueyonder.co.uk>

**Source code:** :source:`Lib/functools.py`

--------------

The :mod:`functools` module is for higher-order functions: functions that act on
or return other functions. In general, any callable object can be treated as a
function for the purposes of this module.

The :mod:`functools` module defines the following functions:

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


.. decorator:: lru_cache(maxsize=128, typed=False)

   Decorator to wrap a function with a memoizing callable that saves up to the
   *maxsize* most recent calls.  It can save time when an expensive or I/O bound
   function is periodically called with the same arguments.

   Since a dictionary is used to cache results, the positional and keyword
   arguments to the function must be hashable.

   If *maxsize* is set to ``None``, the LRU feature is disabled and the cache can
   grow without bound.  The LRU feature performs best when *maxsize* is a
   power-of-two.

   If *typed* is set to true, function arguments of different types will be
   cached separately.  For example, ``f(3)`` and ``f(3.0)`` will be treated
   as distinct calls with distinct results.

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
   <https://en.wikipedia.org/wiki/Cache_algorithms#Examples>`_ works
   best when the most recent calls are the best predictors of upcoming calls (for
   example, the most popular articles on a news server tend to change each day).
   The cache's size limit assures that the cache does not grow without bound on
   long-running processes such as web servers.

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

.. function:: partial(func, *args, **keywords)

   Return a new :ref:`partial object<partial-objects>` which when called
   will behave like *func* called with the positional arguments *args*
   and keyword arguments *keywords*. If more arguments are supplied to the
   call, they are appended to *args*. If additional keyword arguments are
   supplied, they extend and override *keywords*.
   Roughly equivalent to::

      def partial(func, *args, **keywords):
          def newfunc(*fargs, **fkeywords):
              newkeywords = keywords.copy()
              newkeywords.update(fkeywords)
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


.. class:: partialmethod(func, *args, **keywords)

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

      >>> class Cell(object):
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

   Apply *function* of two arguments cumulatively to the items of *sequence*, from
   left to right, so as to reduce the sequence to a single value.  For example,
   ``reduce(lambda x, y: x+y, [1, 2, 3, 4, 5])`` calculates ``((((1+2)+3)+4)+5)``.
   The left argument, *x*, is the accumulated value and the right argument, *y*, is
   the update value from the *sequence*.  If the optional *initializer* is present,
   it is placed before the items of the sequence in the calculation, and serves as
   a default when the sequence is empty.  If *initializer* is not given and
   *sequence* contains only one item, the first item is returned.

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
   attribute of the generic function.  It is a decorator, taking a type
   parameter and decorating a function implementing the operation for that
   type::

     >>> @fun.register(int)
     ... def _(arg, verbose=False):
     ...     if verbose:
     ...         print("Strength in numbers, eh?", end=" ")
     ...     print(arg)
     ...
     >>> @fun.register(list)
     ... def _(arg, verbose=False):
     ...     if verbose:
     ...         print("Enumerate this:")
     ...     for i, elem in enumerate(arg):
     ...         print(i, elem)

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
