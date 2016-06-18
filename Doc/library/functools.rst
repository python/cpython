:mod:`functools` --- Higher-order functions and operations on callable objects
==============================================================================

.. module:: functools
   :synopsis: Higher-order functions and operations on callable objects.
.. moduleauthor:: Peter Harris <scav@blueyonder.co.uk>
.. moduleauthor:: Raymond Hettinger <python@rcn.com>
.. moduleauthor:: Nick Coghlan <ncoghlan@gmail.com>
.. sectionauthor:: Peter Harris <scav@blueyonder.co.uk>

.. versionadded:: 2.5

**Source code:** :source:`Lib/functools.py`

--------------

The :mod:`functools` module is for higher-order functions: functions that act on
or return other functions. In general, any callable object can be treated as a
function for the purposes of this module.

The :mod:`functools` module defines the following functions:

..  function:: cmp_to_key(func)

   Transform an old-style comparison function to a :term:`key function`.  Used
   with tools that accept key functions (such as :func:`sorted`, :func:`min`,
   :func:`max`, :func:`heapq.nlargest`, :func:`heapq.nsmallest`,
   :func:`itertools.groupby`).  This function is primarily used as a transition
   tool for programs being converted to Python 3 where comparison functions are
   no longer supported.

   A comparison function is any callable that accept two arguments, compares them,
   and returns a negative number for less-than, zero for equality, or a positive
   number for greater-than.  A key function is a callable that accepts one
   argument and returns another value to be used as the sort key.

   Example::

       sorted(iterable, key=cmp_to_key(locale.strcoll))  # locale-aware sort order

   For sorting examples and a brief sorting tutorial, see :ref:`sortinghowto`.


   .. versionadded:: 2.7

.. function:: total_ordering(cls)

   Given a class defining one or more rich comparison ordering methods, this
   class decorator supplies the rest.  This simplifies the effort involved
   in specifying all of the possible rich comparison operations:

   The class must define one of :meth:`__lt__`, :meth:`__le__`,
   :meth:`__gt__`, or :meth:`__ge__`.
   In addition, the class should supply an :meth:`__eq__` method.

   For example::

       @total_ordering
       class Student:
           def __eq__(self, other):
               return ((self.lastname.lower(), self.firstname.lower()) ==
                       (other.lastname.lower(), other.firstname.lower()))
           def __lt__(self, other):
               return ((self.lastname.lower(), self.firstname.lower()) <
                       (other.lastname.lower(), other.firstname.lower()))

   .. versionadded:: 2.7

.. function:: reduce(function, iterable[, initializer])

   This is the same function as :func:`reduce`.  It is made available in this module
   to allow writing code more forward-compatible with Python 3.

   .. versionadded:: 2.6


.. function:: partial(func[,*args][, **keywords])

   Return a new :class:`partial` object which when called will behave like *func*
   called with the positional arguments *args* and keyword arguments *keywords*. If
   more arguments are supplied to the call, they are appended to *args*. If
   additional keyword arguments are supplied, they extend and override *keywords*.
   Roughly equivalent to::

      def partial(func, *args, **keywords):
          def newfunc(*fargs, **fkeywords):
              newkeywords = keywords.copy()
              newkeywords.update(fkeywords)
              return func(*(args + fargs), **newkeywords)
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


.. function:: update_wrapper(wrapper, wrapped[, assigned][, updated])

   Update a *wrapper* function to look like the *wrapped* function. The optional
   arguments are tuples to specify which attributes of the original function are
   assigned directly to the matching attributes on the wrapper function and which
   attributes of the wrapper function are updated with the corresponding attributes
   from the original function. The default values for these arguments are the
   module level constants *WRAPPER_ASSIGNMENTS* (which assigns to the wrapper
   function's *__name__*, *__module__* and *__doc__*, the documentation string) and
   *WRAPPER_UPDATES* (which updates the wrapper function's *__dict__*, i.e. the
   instance dictionary).

   The main intended use for this function is in :term:`decorator` functions which
   wrap the decorated function and return the wrapper. If the wrapper function is
   not updated, the metadata of the returned function will reflect the wrapper
   definition rather than the original function definition, which is typically less
   than helpful.


.. function:: wraps(wrapped[, assigned][, updated])

   This is a convenience function for invoking :func:`update_wrapper` as a
   function decorator when defining a wrapper function.  It is equivalent to
   ``partial(update_wrapper, wrapped=wrapped, assigned=assigned, updated=updated)``.
   For example::

      >>> from functools import wraps
      >>> def my_decorator(f):
      ...     @wraps(f)
      ...     def wrapper(*args, **kwds):
      ...         print 'Calling decorated function'
      ...         return f(*args, **kwds)
      ...     return wrapper
      ...
      >>> @my_decorator
      ... def example():
      ...     """Docstring"""
      ...     print 'Called example function'
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

