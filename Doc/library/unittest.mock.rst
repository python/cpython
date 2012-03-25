:mod:`unittest.mock` --- mock object library
============================================

.. module:: unittest.mock
   :synopsis: Mock object library.
.. moduleauthor:: Michael Foord <michael@python.org>
.. currentmodule:: unittest.mock

.. versionadded:: 3.3

:mod:`unittest.mock` is a library for testing in Python. It allows you to
replace parts of your system under test with mock objects and make assertions
about how they have been used.

`unittest.mock` provides a core :class:`Mock` class removing the need to
create a host of stubs throughout your test suite. After performing an
action, you can make assertions about which methods / attributes were used
and arguments they were called with. You can also specify return values and
set needed attributes in the normal way.

Additionally, mock provides a :func:`patch` decorator that handles patching
module and class level attributes within the scope of a test, along with
:const:`sentinel` for creating unique objects. See the `quick guide`_ for
some examples of how to use :class:`Mock`, :class:`MagicMock` and
:func:`patch`.

Mock is very easy to use and is designed for use with :mod:`unittest`. Mock
is based on the 'action -> assertion' pattern instead of `'record -> replay'`
used by many mocking frameworks.

There is a backport of `unittest.mock` for earlier versions of Python,
available as `mock on PyPI <http://pypi.python.org/pypi/mock>`_.

**Source code:** :source:`Lib/unittest/mock.py`


Quick Guide
-----------

:class:`Mock` and :class:`MagicMock` objects create all attributes and
methods as you access them and store details of how they have been used. You
can configure them, to specify return values or limit what attributes are
available, and then make assertions about how they have been used:

    >>> from unittest.mock import MagicMock
    >>> thing = ProductionClass()
    >>> thing.method = MagicMock(return_value=3)
    >>> thing.method(3, 4, 5, key='value')
    3
    >>> thing.method.assert_called_with(3, 4, 5, key='value')

:attr:`side_effect` allows you to perform side effects, including raising an
exception when a mock is called:

   >>> mock = Mock(side_effect=KeyError('foo'))
   >>> mock()
   Traceback (most recent call last):
    ...
   KeyError: 'foo'

   >>> values = {'a': 1, 'b': 2, 'c': 3}
   >>> def side_effect(arg):
   ...     return values[arg]
   ...
   >>> mock.side_effect = side_effect
   >>> mock('a'), mock('b'), mock('c')
   (1, 2, 3)
   >>> mock.side_effect = [5, 4, 3, 2, 1]
   >>> mock(), mock(), mock()
   (5, 4, 3)

Mock has many other ways you can configure it and control its behaviour. For
example the `spec` argument configures the mock to take its specification
from another object. Attempting to access attributes or methods on the mock
that don't exist on the spec will fail with an `AttributeError`.

The :func:`patch` decorator / context manager makes it easy to mock classes or
objects in a module under test. The object you specify will be replaced with a
mock (or other object) during the test and restored when the test ends:

    >>> from unittest.mock import patch
    >>> @patch('module.ClassName2')
    ... @patch('module.ClassName1')
    ... def test(MockClass1, MockClass2):
    ...     module.ClassName1()
    ...     module.ClassName2()

    ...     assert MockClass1 is module.ClassName1
    ...     assert MockClass2 is module.ClassName2
    ...     assert MockClass1.called
    ...     assert MockClass2.called
    ...
    >>> test()

.. note::

   When you nest patch decorators the mocks are passed in to the decorated
   function in the same order they applied (the normal *python* order that
   decorators are applied). This means from the bottom up, so in the example
   above the mock for `module.ClassName1` is passed in first.

   With `patch` it matters that you patch objects in the namespace where they
   are looked up. This is normally straightforward, but for a quick guide
   read :ref:`where to patch <where-to-patch>`.

As well as a decorator `patch` can be used as a context manager in a with
statement:

    >>> with patch.object(ProductionClass, 'method', return_value=None) as mock_method:
    ...     thing = ProductionClass()
    ...     thing.method(1, 2, 3)
    ...
    >>> mock_method.assert_called_once_with(1, 2, 3)


There is also :func:`patch.dict` for setting values in a dictionary just
during a scope and restoring the dictionary to its original state when the test
ends:

   >>> foo = {'key': 'value'}
   >>> original = foo.copy()
   >>> with patch.dict(foo, {'newkey': 'newvalue'}, clear=True):
   ...     assert foo == {'newkey': 'newvalue'}
   ...
   >>> assert foo == original

Mock supports the mocking of Python :ref:`magic methods <magic-methods>`. The
easiest way of using magic methods is with the :class:`MagicMock` class. It
allows you to do things like:

    >>> mock = MagicMock()
    >>> mock.__str__.return_value = 'foobarbaz'
    >>> str(mock)
    'foobarbaz'
    >>> mock.__str__.assert_called_with()

Mock allows you to assign functions (or other Mock instances) to magic methods
and they will be called appropriately. The `MagicMock` class is just a Mock
variant that has all of the magic methods pre-created for you (well, all the
useful ones anyway).

The following is an example of using magic methods with the ordinary Mock
class:

    >>> mock = Mock()
    >>> mock.__str__ = Mock(return_value='wheeeeee')
    >>> str(mock)
    'wheeeeee'

For ensuring that the mock objects in your tests have the same api as the
objects they are replacing, you can use :ref:`auto-speccing <auto-speccing>`.
Auto-speccing can be done through the `autospec` argument to patch, or the
:func:`create_autospec` function. Auto-speccing creates mock objects that
have the same attributes and methods as the objects they are replacing, and
any functions and methods (including constructors) have the same call
signature as the real object.

This ensures that your mocks will fail in the same way as your production
code if they are used incorrectly:

   >>> from unittest.mock import create_autospec
   >>> def function(a, b, c):
   ...     pass
   ...
   >>> mock_function = create_autospec(function, return_value='fishy')
   >>> mock_function(1, 2, 3)
   'fishy'
   >>> mock_function.assert_called_once_with(1, 2, 3)
   >>> mock_function('wrong arguments')
   Traceback (most recent call last):
    ...
   TypeError: <lambda>() takes exactly 3 arguments (1 given)

`create_autospec` can also be used on classes, where it copies the signature of
the `__init__` method, and on callable objects where it copies the signature of
the `__call__` method.



The Mock Class
--------------


`Mock` is a flexible mock object intended to replace the use of stubs and
test doubles throughout your code. Mocks are callable and create attributes as
new mocks when you access them [#]_. Accessing the same attribute will always
return the same mock. Mocks record how you use them, allowing you to make
assertions about what your code has done to them.

:class:`MagicMock` is a subclass of `Mock` with all the magic methods
pre-created and ready to use. There are also non-callable variants, useful
when you are mocking out objects that aren't callable:
:class:`NonCallableMock` and :class:`NonCallableMagicMock`

The :func:`patch` decorators makes it easy to temporarily replace classes
in a particular module with a `Mock` object. By default `patch` will create
a `MagicMock` for you. You can specify an alternative class of `Mock` using
the `new_callable` argument to `patch`.


.. class:: Mock(spec=None, side_effect=None, return_value=DEFAULT, wraps=None, name=None, spec_set=None, **kwargs)

    Create a new `Mock` object. `Mock` takes several optional arguments
    that specify the behaviour of the Mock object:

    * `spec`: This can be either a list of strings or an existing object (a
      class or instance) that acts as the specification for the mock object. If
      you pass in an object then a list of strings is formed by calling dir on
      the object (excluding unsupported magic attributes and methods).
      Accessing any attribute not in this list will raise an `AttributeError`.

      If `spec` is an object (rather than a list of strings) then
      :attr:`__class__` returns the class of the spec object. This allows mocks
      to pass `isinstance` tests.

    * `spec_set`: A stricter variant of `spec`. If used, attempting to *set*
      or get an attribute on the mock that isn't on the object passed as
      `spec_set` will raise an `AttributeError`.

    * `side_effect`: A function to be called whenever the Mock is called. See
      the :attr:`~Mock.side_effect` attribute. Useful for raising exceptions or
      dynamically changing return values. The function is called with the same
      arguments as the mock, and unless it returns :data:`DEFAULT`, the return
      value of this function is used as the return value.

      Alternatively `side_effect` can be an exception class or instance. In
      this case the exception will be raised when the mock is called.

      If `side_effect` is an iterable then each call to the mock will return
      the next value from the iterable.

      A `side_effect` can be cleared by setting it to `None`.

    * `return_value`: The value returned when the mock is called. By default
      this is a new Mock (created on first access). See the
      :attr:`return_value` attribute.

    * `wraps`: Item for the mock object to wrap. If `wraps` is not None then
      calling the Mock will pass the call through to the wrapped object
      (returning the real result and ignoring `return_value`). Attribute access
      on the mock will return a Mock object that wraps the corresponding
      attribute of the wrapped object (so attempting to access an attribute
      that doesn't exist will raise an `AttributeError`).

      If the mock has an explicit `return_value` set then calls are not passed
      to the wrapped object and the `return_value` is returned instead.

    * `name`: If the mock has a name then it will be used in the repr of the
      mock. This can be useful for debugging. The name is propagated to child
      mocks.

    Mocks can also be called with arbitrary keyword arguments. These will be
    used to set attributes on the mock after it is created. See the
    :meth:`configure_mock` method for details.


    .. method:: assert_called_with(*args, **kwargs)

        This method is a convenient way of asserting that calls are made in a
        particular way:

            >>> mock = Mock()
            >>> mock.method(1, 2, 3, test='wow')
            <Mock name='mock.method()' id='...'>
            >>> mock.method.assert_called_with(1, 2, 3, test='wow')


    .. method:: assert_called_once_with(*args, **kwargs)

       Assert that the mock was called exactly once and with the specified
       arguments.

            >>> mock = Mock(return_value=None)
            >>> mock('foo', bar='baz')
            >>> mock.assert_called_once_with('foo', bar='baz')
            >>> mock('foo', bar='baz')
            >>> mock.assert_called_once_with('foo', bar='baz')
            Traceback (most recent call last):
              ...
            AssertionError: Expected to be called once. Called 2 times.


    .. method:: assert_any_call(*args, **kwargs)

        assert the mock has been called with the specified arguments.

        The assert passes if the mock has *ever* been called, unlike
        :meth:`assert_called_with` and :meth:`assert_called_once_with` that
        only pass if the call is the most recent one.

            >>> mock = Mock(return_value=None)
            >>> mock(1, 2, arg='thing')
            >>> mock('some', 'thing', 'else')
            >>> mock.assert_any_call(1, 2, arg='thing')


    .. method:: assert_has_calls(calls, any_order=False)

        assert the mock has been called with the specified calls.
        The `mock_calls` list is checked for the calls.

        If `any_order` is False (the default) then the calls must be
        sequential. There can be extra calls before or after the
        specified calls.

        If `any_order` is True then the calls can be in any order, but
        they must all appear in :attr:`mock_calls`.

            >>> mock = Mock(return_value=None)
            >>> mock(1)
            >>> mock(2)
            >>> mock(3)
            >>> mock(4)
            >>> calls = [call(2), call(3)]
            >>> mock.assert_has_calls(calls)
            >>> calls = [call(4), call(2), call(3)]
            >>> mock.assert_has_calls(calls, any_order=True)


    .. method:: reset_mock()

        The reset_mock method resets all the call attributes on a mock object:

            >>> mock = Mock(return_value=None)
            >>> mock('hello')
            >>> mock.called
            True
            >>> mock.reset_mock()
            >>> mock.called
            False

        This can be useful where you want to make a series of assertions that
        reuse the same object. Note that `reset_mock` *doesn't* clear the
        return value, :attr:`side_effect` or any child attributes you have
        set using normal assignment. Child mocks and the return value mock
        (if any) are reset as well.


    .. method:: mock_add_spec(spec, spec_set=False)

        Add a spec to a mock. `spec` can either be an object or a
        list of strings. Only attributes on the `spec` can be fetched as
        attributes from the mock.

        If `spec_set` is `True` then only attributes on the spec can be set.


    .. method:: attach_mock(mock, attribute)

        Attach a mock as an attribute of this one, replacing its name and
        parent. Calls to the attached mock will be recorded in the
        :attr:`method_calls` and :attr:`mock_calls` attributes of this one.


    .. method:: configure_mock(**kwargs)

        Set attributes on the mock through keyword arguments.

        Attributes plus return values and side effects can be set on child
        mocks using standard dot notation and unpacking a dictionary in the
        method call:

            >>> mock = Mock()
            >>> attrs = {'method.return_value': 3, 'other.side_effect': KeyError}
            >>> mock.configure_mock(**attrs)
            >>> mock.method()
            3
            >>> mock.other()
            Traceback (most recent call last):
              ...
            KeyError

        The same thing can be achieved in the constructor call to mocks:

            >>> attrs = {'method.return_value': 3, 'other.side_effect': KeyError}
            >>> mock = Mock(some_attribute='eggs', **attrs)
            >>> mock.some_attribute
            'eggs'
            >>> mock.method()
            3
            >>> mock.other()
            Traceback (most recent call last):
              ...
            KeyError

        `configure_mock` exists to make it easier to do configuration
        after the mock has been created.


    .. method:: __dir__()

        `Mock` objects limit the results of `dir(some_mock)` to useful results.
        For mocks with a `spec` this includes all the permitted attributes
        for the mock.

        See :data:`FILTER_DIR` for what this filtering does, and how to
        switch it off.


    .. method:: _get_child_mock(**kw)

        Create the child mocks for attributes and return value.
        By default child mocks will be the same type as the parent.
        Subclasses of Mock may want to override this to customize the way
        child mocks are made.

        For non-callable mocks the callable variant will be used (rather than
        any custom subclass).


    .. attribute:: called

        A boolean representing whether or not the mock object has been called:

            >>> mock = Mock(return_value=None)
            >>> mock.called
            False
            >>> mock()
            >>> mock.called
            True

    .. attribute:: call_count

        An integer telling you how many times the mock object has been called:

            >>> mock = Mock(return_value=None)
            >>> mock.call_count
            0
            >>> mock()
            >>> mock()
            >>> mock.call_count
            2


    .. attribute:: return_value

        Set this to configure the value returned by calling the mock:

            >>> mock = Mock()
            >>> mock.return_value = 'fish'
            >>> mock()
            'fish'

        The default return value is a mock object and you can configure it in
        the normal way:

            >>> mock = Mock()
            >>> mock.return_value.attribute = sentinel.Attribute
            >>> mock.return_value()
            <Mock name='mock()()' id='...'>
            >>> mock.return_value.assert_called_with()

        `return_value` can also be set in the constructor:

            >>> mock = Mock(return_value=3)
            >>> mock.return_value
            3
            >>> mock()
            3


    .. attribute:: side_effect

        This can either be a function to be called when the mock is called,
        or an exception (class or instance) to be raised.

        If you pass in a function it will be called with same arguments as the
        mock and unless the function returns the :data:`DEFAULT` singleton the
        call to the mock will then return whatever the function returns. If the
        function returns :data:`DEFAULT` then the mock will return its normal
        value (from the :attr:`return_value`.

        An example of a mock that raises an exception (to test exception
        handling of an API):

            >>> mock = Mock()
            >>> mock.side_effect = Exception('Boom!')
            >>> mock()
            Traceback (most recent call last):
              ...
            Exception: Boom!

        Using `side_effect` to return a sequence of values:

            >>> mock = Mock()
            >>> mock.side_effect = [3, 2, 1]
            >>> mock(), mock(), mock()
            (3, 2, 1)

        The `side_effect` function is called with the same arguments as the
        mock (so it is wise for it to take arbitrary args and keyword
        arguments) and whatever it returns is used as the return value for
        the call. The exception is if `side_effect` returns :data:`DEFAULT`,
        in which case the normal :attr:`return_value` is used.

            >>> mock = Mock(return_value=3)
            >>> def side_effect(*args, **kwargs):
            ...     return DEFAULT
            ...
            >>> mock.side_effect = side_effect
            >>> mock()
            3

        `side_effect` can be set in the constructor. Here's an example that
        adds one to the value the mock is called with and returns it:

            >>> side_effect = lambda value: value + 1
            >>> mock = Mock(side_effect=side_effect)
            >>> mock(3)
            4
            >>> mock(-8)
            -7

        Setting `side_effect` to `None` clears it:

            >>> m = Mock(side_effect=KeyError, return_value=3)
            >>> m()
            Traceback (most recent call last):
             ...
            KeyError
            >>> m.side_effect = None
            >>> m()
            3


    .. attribute:: call_args

        This is either `None` (if the mock hasn't been called), or the
        arguments that the mock was last called with. This will be in the
        form of a tuple: the first member is any ordered arguments the mock
        was called with (or an empty tuple) and the second member is any
        keyword arguments (or an empty dictionary).

            >>> mock = Mock(return_value=None)
            >>> print mock.call_args
            None
            >>> mock()
            >>> mock.call_args
            call()
            >>> mock.call_args == ()
            True
            >>> mock(3, 4)
            >>> mock.call_args
            call(3, 4)
            >>> mock.call_args == ((3, 4),)
            True
            >>> mock(3, 4, 5, key='fish', next='w00t!')
            >>> mock.call_args
            call(3, 4, 5, key='fish', next='w00t!')

        `call_args`, along with members of the lists :attr:`call_args_list`,
        :attr:`method_calls` and :attr:`mock_calls` are :data:`call` objects.
        These are tuples, so they can be unpacked to get at the individual
        arguments and make more complex assertions. See
        :ref:`calls as tuples <calls-as-tuples>`.


    .. attribute:: call_args_list

        This is a list of all the calls made to the mock object in sequence
        (so the length of the list is the number of times it has been
        called). Before any calls have been made it is an empty list. The
        :data:`call` object can be used for conveniently constructing lists of
        calls to compare with `call_args_list`.

            >>> mock = Mock(return_value=None)
            >>> mock()
            >>> mock(3, 4)
            >>> mock(key='fish', next='w00t!')
            >>> mock.call_args_list
            [call(), call(3, 4), call(key='fish', next='w00t!')]
            >>> expected = [(), ((3, 4),), ({'key': 'fish', 'next': 'w00t!'},)]
            >>> mock.call_args_list == expected
            True

        Members of `call_args_list` are :data:`call` objects. These can be
        unpacked as tuples to get at the individual arguments. See
        :ref:`calls as tuples <calls-as-tuples>`.


    .. attribute:: method_calls

        As well as tracking calls to themselves, mocks also track calls to
        methods and attributes, and *their* methods and attributes:

            >>> mock = Mock()
            >>> mock.method()
            <Mock name='mock.method()' id='...'>
            >>> mock.property.method.attribute()
            <Mock name='mock.property.method.attribute()' id='...'>
            >>> mock.method_calls
            [call.method(), call.property.method.attribute()]

        Members of `method_calls` are :data:`call` objects. These can be
        unpacked as tuples to get at the individual arguments. See
        :ref:`calls as tuples <calls-as-tuples>`.


    .. attribute:: mock_calls

        `mock_calls` records *all* calls to the mock object, its methods, magic
        methods *and* return value mocks.

            >>> mock = MagicMock()
            >>> result = mock(1, 2, 3)
            >>> mock.first(a=3)
            <MagicMock name='mock.first()' id='...'>
            >>> mock.second()
            <MagicMock name='mock.second()' id='...'>
            >>> int(mock)
            1
            >>> result(1)
            <MagicMock name='mock()()' id='...'>
            >>> expected = [call(1, 2, 3), call.first(a=3), call.second(),
            ... call.__int__(), call()(1)]
            >>> mock.mock_calls == expected
            True

        Members of `mock_calls` are :data:`call` objects. These can be
        unpacked as tuples to get at the individual arguments. See
        :ref:`calls as tuples <calls-as-tuples>`.


    .. attribute:: __class__

        Normally the `__class__` attribute of an object will return its type.
        For a mock object with a `spec` `__class__` returns the spec class
        instead. This allows mock objects to pass `isinstance` tests for the
        object they are replacing / masquerading as:

            >>> mock = Mock(spec=3)
            >>> isinstance(mock, int)
            True

        `__class__` is assignable to, this allows a mock to pass an
        `isinstance` check without forcing you to use a spec:

            >>> mock = Mock()
            >>> mock.__class__ = dict
            >>> isinstance(mock, dict)
            True

.. class:: NonCallableMock(spec=None, wraps=None, name=None, spec_set=None, **kwargs)

    A non-callable version of `Mock`. The constructor parameters have the same
    meaning of `Mock`, with the exception of `return_value` and `side_effect`
    which have no meaning on a non-callable mock.

Mock objects that use a class or an instance as a `spec` or `spec_set` are able
to pass `isintance` tests:

    >>> mock = Mock(spec=SomeClass)
    >>> isinstance(mock, SomeClass)
    True
    >>> mock = Mock(spec_set=SomeClass())
    >>> isinstance(mock, SomeClass)
    True

The `Mock` classes have support for mocking magic methods. See :ref:`magic
methods <magic-methods>` for the full details.

The mock classes and the :func:`patch` decorators all take arbitrary keyword
arguments for configuration. For the `patch` decorators the keywords are
passed to the constructor of the mock being created. The keyword arguments
are for configuring attributes of the mock:

        >>> m = MagicMock(attribute=3, other='fish')
        >>> m.attribute
        3
        >>> m.other
        'fish'

The return value and side effect of child mocks can be set in the same way,
using dotted notation. As you can't use dotted names directly in a call you
have to create a dictionary and unpack it using `**`:

    >>> attrs = {'method.return_value': 3, 'other.side_effect': KeyError}
    >>> mock = Mock(some_attribute='eggs', **attrs)
    >>> mock.some_attribute
    'eggs'
    >>> mock.method()
    3
    >>> mock.other()
    Traceback (most recent call last):
      ...
    KeyError


.. class:: PropertyMock(*args, **kwargs)

   A mock intended to be used as a property, or other descriptor, on a class.
   `PropertyMock` provides `__get__` and `__set__` methods so you can specify
   a return value when it is fetched.

   Fetching a `PropertyMock` instance from an object calls the mock, with
   no args. Setting it calls the mock with the value being set.

        >>> class Foo(object):
        ...     @property
        ...     def foo(self):
        ...         return 'something'
        ...     @foo.setter
        ...     def foo(self, value):
        ...         pass
        ...
        >>> with patch('__main__.Foo.foo', new_callable=PropertyMock) as mock_foo:
        ...     mock_foo.return_value = 'mockity-mock'
        ...     this_foo = Foo()
        ...     print this_foo.foo
        ...     this_foo.foo = 6
        ...
        mockity-mock
        >>> mock_foo.mock_calls
        [call(), call(6)]


Calling
~~~~~~~

Mock objects are callable. The call will return the value set as the
:attr:`~Mock.return_value` attribute. The default return value is a new Mock
object; it is created the first time the return value is accessed (either
explicitly or by calling the Mock) - but it is stored and the same one
returned each time.

Calls made to the object will be recorded in the attributes
like :attr:`~Mock.call_args` and :attr:`~Mock.call_args_list`.

If :attr:`~Mock.side_effect` is set then it will be called after the call has
been recorded, so if `side_effect` raises an exception the call is still
recorded.

The simplest way to make a mock raise an exception when called is to make
:attr:`~Mock.side_effect` an exception class or instance:

        >>> m = MagicMock(side_effect=IndexError)
        >>> m(1, 2, 3)
        Traceback (most recent call last):
          ...
        IndexError
        >>> m.mock_calls
        [call(1, 2, 3)]
        >>> m.side_effect = KeyError('Bang!')
        >>> m('two', 'three', 'four')
        Traceback (most recent call last):
          ...
        KeyError: 'Bang!'
        >>> m.mock_calls
        [call(1, 2, 3), call('two', 'three', 'four')]

If `side_effect` is a function then whatever that function returns is what
calls to the mock return. The `side_effect` function is called with the
same arguments as the mock. This allows you to vary the return value of the
call dynamically, based on the input:

        >>> def side_effect(value):
        ...     return value + 1
        ...
        >>> m = MagicMock(side_effect=side_effect)
        >>> m(1)
        2
        >>> m(2)
        3
        >>> m.mock_calls
        [call(1), call(2)]

If you want the mock to still return the default return value (a new mock), or
any set return value, then there are two ways of doing this. Either return
`mock.return_value` from inside `side_effect`, or return :data:`DEFAULT`:

        >>> m = MagicMock()
        >>> def side_effect(*args, **kwargs):
        ...     return m.return_value
        ...
        >>> m.side_effect = side_effect
        >>> m.return_value = 3
        >>> m()
        3
        >>> def side_effect(*args, **kwargs):
        ...     return DEFAULT
        ...
        >>> m.side_effect = side_effect
        >>> m()
        3

To remove a `side_effect`, and return to the default behaviour, set the
`side_effect` to `None`:

        >>> m = MagicMock(return_value=6)
        >>> def side_effect(*args, **kwargs):
        ...     return 3
        ...
        >>> m.side_effect = side_effect
        >>> m()
        3
        >>> m.side_effect = None
        >>> m()
        6

The `side_effect` can also be any iterable object. Repeated calls to the mock
will return values from the iterable (until the iterable is exhausted and
a `StopIteration` is raised):

        >>> m = MagicMock(side_effect=[1, 2, 3])
        >>> m()
        1
        >>> m()
        2
        >>> m()
        3
        >>> m()
        Traceback (most recent call last):
          ...
        StopIteration


.. _deleting-attributes:

Deleting Attributes
~~~~~~~~~~~~~~~~~~~

Mock objects create attributes on demand. This allows them to pretend to be
objects of any type.

You may want a mock object to return `False` to a `hasattr` call, or raise an
`AttributeError` when an attribute is fetched. You can do this by providing
an object as a `spec` for a mock, but that isn't always convenient.

You "block" attributes by deleting them. Once deleted, accessing an attribute
will raise an `AttributeError`.

    >>> mock = MagicMock()
    >>> hasattr(mock, 'm')
    True
    >>> del mock.m
    >>> hasattr(mock, 'm')
    False
    >>> del mock.f
    >>> mock.f
    Traceback (most recent call last):
        ...
    AttributeError: f


Attaching Mocks as Attributes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When you attach a mock as an attribute of another mock (or as the return
value) it becomes a "child" of that mock. Calls to the child are recorded in
the :attr:`~Mock.method_calls` and :attr:`~Mock.mock_calls` attributes of the
parent. This is useful for configuring child mocks and then attaching them to
the parent, or for attaching mocks to a parent that records all calls to the
children and allows you to make assertions about the order of calls between
mocks:

    >>> parent = MagicMock()
    >>> child1 = MagicMock(return_value=None)
    >>> child2 = MagicMock(return_value=None)
    >>> parent.child1 = child1
    >>> parent.child2 = child2
    >>> child1(1)
    >>> child2(2)
    >>> parent.mock_calls
    [call.child1(1), call.child2(2)]

The exception to this is if the mock has a name. This allows you to prevent
the "parenting" if for some reason you don't want it to happen.

    >>> mock = MagicMock()
    >>> not_a_child = MagicMock(name='not-a-child')
    >>> mock.attribute = not_a_child
    >>> mock.attribute()
    <MagicMock name='not-a-child()' id='...'>
    >>> mock.mock_calls
    []

Mocks created for you by :func:`patch` are automatically given names. To
attach mocks that have names to a parent you use the :meth:`~Mock.attach_mock`
method:

    >>> thing1 = object()
    >>> thing2 = object()
    >>> parent = MagicMock()
    >>> with patch('__main__.thing1', return_value=None) as child1:
    ...     with patch('__main__.thing2', return_value=None) as child2:
    ...         parent.attach_mock(child1, 'child1')
    ...         parent.attach_mock(child2, 'child2')
    ...         child1('one')
    ...         child2('two')
    ...
    >>> parent.mock_calls
    [call.child1('one'), call.child2('two')]


.. [#] The only exceptions are magic methods and attributes (those that have
       leading and trailing double underscores). Mock doesn't create these but
       instead of raises an ``AttributeError``. This is because the interpreter
       will often implicitly request these methods, and gets *very* confused to
       get a new Mock object when it expects a magic method. If you need magic
       method support see :ref:`magic methods <magic-methods>`.
