:mod:`unittest.mock` --- getting started
========================================

.. moduleauthor:: Michael Foord <michael@python.org>
.. currentmodule:: unittest.mock

.. versionadded:: 3.3


.. _getting-started:

Using Mock
----------

Mock Patching Methods
~~~~~~~~~~~~~~~~~~~~~

Common uses for :class:`Mock` objects include:

* Patching methods
* Recording method calls on objects

You might want to replace a method on an object to check that
it is called with the correct arguments by another part of the system:

    >>> real = SomeClass()
    >>> real.method = MagicMock(name='method')
    >>> real.method(3, 4, 5, key='value')
    <MagicMock name='method()' id='...'>

Once our mock has been used (`real.method` in this example) it has methods
and attributes that allow you to make assertions about how it has been used.

.. note::

    In most of these examples the :class:`Mock` and :class:`MagicMock` classes
    are interchangeable. As the `MagicMock` is the more capable class it makes
    a sensible one to use by default.

Once the mock has been called its :attr:`~Mock.called` attribute is set to
`True`. More importantly we can use the :meth:`~Mock.assert_called_with` or
:meth:`~Mock.assert_called_once_with` method to check that it was called with
the correct arguments.

This example tests that calling `ProductionClass().method` results in a call to
the `something` method:

    >>> class ProductionClass:
    ...     def method(self):
    ...         self.something(1, 2, 3)
    ...     def something(self, a, b, c):
    ...         pass
    ...
    >>> real = ProductionClass()
    >>> real.something = MagicMock()
    >>> real.method()
    >>> real.something.assert_called_once_with(1, 2, 3)



Mock for Method Calls on an Object
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In the last example we patched a method directly on an object to check that it
was called correctly. Another common use case is to pass an object into a
method (or some part of the system under test) and then check that it is used
in the correct way.

The simple `ProductionClass` below has a `closer` method. If it is called with
an object then it calls `close` on it.

    >>> class ProductionClass:
    ...     def closer(self, something):
    ...         something.close()
    ...

So to test it we need to pass in an object with a `close` method and check
that it was called correctly.

    >>> real = ProductionClass()
    >>> mock = Mock()
    >>> real.closer(mock)
    >>> mock.close.assert_called_with()

We don't have to do any work to provide the 'close' method on our mock.
Accessing close creates it. So, if 'close' hasn't already been called then
accessing it in the test will create it, but :meth:`~Mock.assert_called_with`
will raise a failure exception.


Mocking Classes
~~~~~~~~~~~~~~~

A common use case is to mock out classes instantiated by your code under test.
When you patch a class, then that class is replaced with a mock. Instances
are created by *calling the class*. This means you access the "mock instance"
by looking at the return value of the mocked class.

In the example below we have a function `some_function` that instantiates `Foo`
and calls a method on it. The call to `patch` replaces the class `Foo` with a
mock. The `Foo` instance is the result of calling the mock, so it is configured
by modifying the mock :attr:`~Mock.return_value`.

    >>> def some_function():
    ...     instance = module.Foo()
    ...     return instance.method()
    ...
    >>> with patch('module.Foo') as mock:
    ...     instance = mock.return_value
    ...     instance.method.return_value = 'the result'
    ...     result = some_function()
    ...     assert result == 'the result'


Naming your mocks
~~~~~~~~~~~~~~~~~

It can be useful to give your mocks a name. The name is shown in the repr of
the mock and can be helpful when the mock appears in test failure messages. The
name is also propagated to attributes or methods of the mock:

    >>> mock = MagicMock(name='foo')
    >>> mock
    <MagicMock name='foo' id='...'>
    >>> mock.method
    <MagicMock name='foo.method' id='...'>


Tracking all Calls
~~~~~~~~~~~~~~~~~~

Often you want to track more than a single call to a method. The
:attr:`~Mock.mock_calls` attribute records all calls
to child attributes of the mock - and also to their children.

    >>> mock = MagicMock()
    >>> mock.method()
    <MagicMock name='mock.method()' id='...'>
    >>> mock.attribute.method(10, x=53)
    <MagicMock name='mock.attribute.method()' id='...'>
    >>> mock.mock_calls
    [call.method(), call.attribute.method(10, x=53)]

If you make an assertion about `mock_calls` and any unexpected methods
have been called, then the assertion will fail. This is useful because as well
as asserting that the calls you expected have been made, you are also checking
that they were made in the right order and with no additional calls:

You use the :data:`call` object to construct lists for comparing with
`mock_calls`:

    >>> expected = [call.method(), call.attribute.method(10, x=53)]
    >>> mock.mock_calls == expected
    True


Setting Return Values and Attributes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Setting the return values on a mock object is trivially easy:

    >>> mock = Mock()
    >>> mock.return_value = 3
    >>> mock()
    3

Of course you can do the same for methods on the mock:

    >>> mock = Mock()
    >>> mock.method.return_value = 3
    >>> mock.method()
    3

The return value can also be set in the constructor:

    >>> mock = Mock(return_value=3)
    >>> mock()
    3

If you need an attribute setting on your mock, just do it:

    >>> mock = Mock()
    >>> mock.x = 3
    >>> mock.x
    3

Sometimes you want to mock up a more complex situation, like for example
`mock.connection.cursor().execute("SELECT 1")`. If we wanted this call to
return a list, then we have to configure the result of the nested call.

We can use :data:`call` to construct the set of calls in a "chained call" like
this for easy assertion afterwards:

    >>> mock = Mock()
    >>> cursor = mock.connection.cursor.return_value
    >>> cursor.execute.return_value = ['foo']
    >>> mock.connection.cursor().execute("SELECT 1")
    ['foo']
    >>> expected = call.connection.cursor().execute("SELECT 1").call_list()
    >>> mock.mock_calls
    [call.connection.cursor(), call.connection.cursor().execute('SELECT 1')]
    >>> mock.mock_calls == expected
    True

It is the call to `.call_list()` that turns our call object into a list of
calls representing the chained calls.


Raising exceptions with mocks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

A useful attribute is :attr:`~Mock.side_effect`. If you set this to an
exception class or instance then the exception will be raised when the mock
is called.

    >>> mock = Mock(side_effect=Exception('Boom!'))
    >>> mock()
    Traceback (most recent call last):
      ...
    Exception: Boom!


Side effect functions and iterables
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

`side_effect` can also be set to a function or an iterable. The use case for
`side_effect` as an iterable is where your mock is going to be called several
times, and you want each call to return a different value. When you set
`side_effect` to an iterable every call to the mock returns the next value
from the iterable:

    >>> mock = MagicMock(side_effect=[4, 5, 6])
    >>> mock()
    4
    >>> mock()
    5
    >>> mock()
    6


For more advanced use cases, like dynamically varying the return values
depending on what the mock is called with, `side_effect` can be a function.
The function will be called with the same arguments as the mock. Whatever the
function returns is what the call returns:

    >>> vals = {(1, 2): 1, (2, 3): 2}
    >>> def side_effect(*args):
    ...     return vals[args]
    ...
    >>> mock = MagicMock(side_effect=side_effect)
    >>> mock(1, 2)
    1
    >>> mock(2, 3)
    2


Creating a Mock from an Existing Object
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

One problem with over use of mocking is that it couples your tests to the
implementation of your mocks rather than your real code. Suppose you have a
class that implements `some_method`. In a test for another class, you
provide a mock of this object that *also* provides `some_method`. If later
you refactor the first class, so that it no longer has `some_method` - then
your tests will continue to pass even though your code is now broken!

`Mock` allows you to provide an object as a specification for the mock,
using the `spec` keyword argument. Accessing methods / attributes on the
mock that don't exist on your specification object will immediately raise an
attribute error. If you change the implementation of your specification, then
tests that use that class will start failing immediately without you having to
instantiate the class in those tests.

    >>> mock = Mock(spec=SomeClass)
    >>> mock.old_method()
    Traceback (most recent call last):
       ...
    AttributeError: object has no attribute 'old_method'

Using a specification also enables a smarter matching of calls made to the
mock, regardless of whether some parameters were passed as positional or
named arguments::

   >>> def f(a, b, c): pass
   ...
   >>> mock = Mock(spec=f)
   >>> mock(1, 2, 3)
   <Mock name='mock()' id='140161580456576'>
   >>> mock.assert_called_with(a=1, b=2, c=3)

If you want this smarter matching to also work with method calls on the mock,
you can use :ref:`auto-speccing <auto-speccing>`.

If you want a stronger form of specification that prevents the setting
of arbitrary attributes as well as the getting of them then you can use
`spec_set` instead of `spec`.



Patch Decorators
----------------

.. note::

   With `patch` it matters that you patch objects in the namespace where they
   are looked up. This is normally straightforward, but for a quick guide
   read :ref:`where to patch <where-to-patch>`.


A common need in tests is to patch a class attribute or a module attribute,
for example patching a builtin or patching a class in a module to test that it
is instantiated. Modules and classes are effectively global, so patching on
them has to be undone after the test or the patch will persist into other
tests and cause hard to diagnose problems.

mock provides three convenient decorators for this: `patch`, `patch.object` and
`patch.dict`. `patch` takes a single string, of the form
`package.module.Class.attribute` to specify the attribute you are patching. It
also optionally takes a value that you want the attribute (or class or
whatever) to be replaced with. 'patch.object' takes an object and the name of
the attribute you would like patched, plus optionally the value to patch it
with.

`patch.object`:

    >>> original = SomeClass.attribute
    >>> @patch.object(SomeClass, 'attribute', sentinel.attribute)
    ... def test():
    ...     assert SomeClass.attribute == sentinel.attribute
    ...
    >>> test()
    >>> assert SomeClass.attribute == original

    >>> @patch('package.module.attribute', sentinel.attribute)
    ... def test():
    ...     from package.module import attribute
    ...     assert attribute is sentinel.attribute
    ...
    >>> test()

If you are patching a module (including :mod:`builtins`) then use `patch`
instead of `patch.object`:

    >>> mock = MagicMock(return_value=sentinel.file_handle)
    >>> with patch('builtins.open', mock):
    ...     handle = open('filename', 'r')
    ...
    >>> mock.assert_called_with('filename', 'r')
    >>> assert handle == sentinel.file_handle, "incorrect file handle returned"

The module name can be 'dotted', in the form `package.module` if needed:

    >>> @patch('package.module.ClassName.attribute', sentinel.attribute)
    ... def test():
    ...     from package.module import ClassName
    ...     assert ClassName.attribute == sentinel.attribute
    ...
    >>> test()

A nice pattern is to actually decorate test methods themselves:

    >>> class MyTest(unittest2.TestCase):
    ...     @patch.object(SomeClass, 'attribute', sentinel.attribute)
    ...     def test_something(self):
    ...         self.assertEqual(SomeClass.attribute, sentinel.attribute)
    ...
    >>> original = SomeClass.attribute
    >>> MyTest('test_something').test_something()
    >>> assert SomeClass.attribute == original

If you want to patch with a Mock, you can use `patch` with only one argument
(or `patch.object` with two arguments). The mock will be created for you and
passed into the test function / method:

    >>> class MyTest(unittest2.TestCase):
    ...     @patch.object(SomeClass, 'static_method')
    ...     def test_something(self, mock_method):
    ...         SomeClass.static_method()
    ...         mock_method.assert_called_with()
    ...
    >>> MyTest('test_something').test_something()

You can stack up multiple patch decorators using this pattern:

    >>> class MyTest(unittest2.TestCase):
    ...     @patch('package.module.ClassName1')
    ...     @patch('package.module.ClassName2')
    ...     def test_something(self, MockClass2, MockClass1):
    ...         self.assertIs(package.module.ClassName1, MockClass1)
    ...         self.assertIs(package.module.ClassName2, MockClass2)
    ...
    >>> MyTest('test_something').test_something()

When you nest patch decorators the mocks are passed in to the decorated
function in the same order they applied (the normal *python* order that
decorators are applied). This means from the bottom up, so in the example
above the mock for `test_module.ClassName2` is passed in first.

There is also :func:`patch.dict` for setting values in a dictionary just
during a scope and restoring the dictionary to its original state when the test
ends:

   >>> foo = {'key': 'value'}
   >>> original = foo.copy()
   >>> with patch.dict(foo, {'newkey': 'newvalue'}, clear=True):
   ...     assert foo == {'newkey': 'newvalue'}
   ...
   >>> assert foo == original

`patch`, `patch.object` and `patch.dict` can all be used as context managers.

Where you use `patch` to create a mock for you, you can get a reference to the
mock using the "as" form of the with statement:

    >>> class ProductionClass:
    ...     def method(self):
    ...         pass
    ...
    >>> with patch.object(ProductionClass, 'method') as mock_method:
    ...     mock_method.return_value = None
    ...     real = ProductionClass()
    ...     real.method(1, 2, 3)
    ...
    >>> mock_method.assert_called_with(1, 2, 3)


As an alternative `patch`, `patch.object` and `patch.dict` can be used as
class decorators. When used in this way it is the same as applying the
decorator individually to every method whose name starts with "test".


.. _further-examples:

Further Examples
----------------


Here are some more examples for some slightly more advanced scenarios.


Mocking chained calls
~~~~~~~~~~~~~~~~~~~~~

Mocking chained calls is actually straightforward with mock once you
understand the :attr:`~Mock.return_value` attribute. When a mock is called for
the first time, or you fetch its `return_value` before it has been called, a
new `Mock` is created.

This means that you can see how the object returned from a call to a mocked
object has been used by interrogating the `return_value` mock:

    >>> mock = Mock()
    >>> mock().foo(a=2, b=3)
    <Mock name='mock().foo()' id='...'>
    >>> mock.return_value.foo.assert_called_with(a=2, b=3)

From here it is a simple step to configure and then make assertions about
chained calls. Of course another alternative is writing your code in a more
testable way in the first place...

So, suppose we have some code that looks a little bit like this:

    >>> class Something:
    ...     def __init__(self):
    ...         self.backend = BackendProvider()
    ...     def method(self):
    ...         response = self.backend.get_endpoint('foobar').create_call('spam', 'eggs').start_call()
    ...         # more code

Assuming that `BackendProvider` is already well tested, how do we test
`method()`? Specifically, we want to test that the code section `# more
code` uses the response object in the correct way.

As this chain of calls is made from an instance attribute we can monkey patch
the `backend` attribute on a `Something` instance. In this particular case
we are only interested in the return value from the final call to
`start_call` so we don't have much configuration to do. Let's assume the
object it returns is 'file-like', so we'll ensure that our response object
uses the builtin `open` as its `spec`.

To do this we create a mock instance as our mock backend and create a mock
response object for it. To set the response as the return value for that final
`start_call` we could do this:

    `mock_backend.get_endpoint.return_value.create_call.return_value.start_call.return_value = mock_response`.

We can do that in a slightly nicer way using the :meth:`~Mock.configure_mock`
method to directly set the return value for us:

    >>> something = Something()
    >>> mock_response = Mock(spec=open)
    >>> mock_backend = Mock()
    >>> config = {'get_endpoint.return_value.create_call.return_value.start_call.return_value': mock_response}
    >>> mock_backend.configure_mock(**config)

With these we monkey patch the "mock backend" in place and can make the real
call:

    >>> something.backend = mock_backend
    >>> something.method()

Using :attr:`~Mock.mock_calls` we can check the chained call with a single
assert. A chained call is several calls in one line of code, so there will be
several entries in `mock_calls`. We can use :meth:`call.call_list` to create
this list of calls for us:

    >>> chained = call.get_endpoint('foobar').create_call('spam', 'eggs').start_call()
    >>> call_list = chained.call_list()
    >>> assert mock_backend.mock_calls == call_list


Partial mocking
~~~~~~~~~~~~~~~

In some tests I wanted to mock out a call to `datetime.date.today()
<http://docs.python.org/library/datetime.html#datetime.date.today>`_ to return
a known date, but I didn't want to prevent the code under test from
creating new date objects. Unfortunately `datetime.date` is written in C, and
so I couldn't just monkey-patch out the static `date.today` method.

I found a simple way of doing this that involved effectively wrapping the date
class with a mock, but passing through calls to the constructor to the real
class (and returning real instances).

The :func:`patch decorator <patch>` is used here to
mock out the `date` class in the module under test. The :attr:`side_effect`
attribute on the mock date class is then set to a lambda function that returns
a real date. When the mock date class is called a real date will be
constructed and returned by `side_effect`.

    >>> from datetime import date
    >>> with patch('mymodule.date') as mock_date:
    ...     mock_date.today.return_value = date(2010, 10, 8)
    ...     mock_date.side_effect = lambda *args, **kw: date(*args, **kw)
    ...
    ...     assert mymodule.date.today() == date(2010, 10, 8)
    ...     assert mymodule.date(2009, 6, 8) == date(2009, 6, 8)
    ...

Note that we don't patch `datetime.date` globally, we patch `date` in the
module that *uses* it. See :ref:`where to patch <where-to-patch>`.

When `date.today()` is called a known date is returned, but calls to the
`date(...)` constructor still return normal dates. Without this you can find
yourself having to calculate an expected result using exactly the same
algorithm as the code under test, which is a classic testing anti-pattern.

Calls to the date constructor are recorded in the `mock_date` attributes
(`call_count` and friends) which may also be useful for your tests.

An alternative way of dealing with mocking dates, or other builtin classes,
is discussed in `this blog entry
<http://williamjohnbert.com/2011/07/how-to-unit-testing-in-django-with-mocking-and-patching/>`_.


Mocking a Generator Method
~~~~~~~~~~~~~~~~~~~~~~~~~~

A Python generator is a function or method that uses the `yield statement
<http://docs.python.org/reference/simple_stmts.html#the-yield-statement>`_ to
return a series of values when iterated over [#]_.

A generator method / function is called to return the generator object. It is
the generator object that is then iterated over. The protocol method for
iteration is `__iter__
<http://docs.python.org/library/stdtypes.html#container.__iter__>`_, so we can
mock this using a `MagicMock`.

Here's an example class with an "iter" method implemented as a generator:

    >>> class Foo:
    ...     def iter(self):
    ...         for i in [1, 2, 3]:
    ...             yield i
    ...
    >>> foo = Foo()
    >>> list(foo.iter())
    [1, 2, 3]


How would we mock this class, and in particular its "iter" method?

To configure the values returned from the iteration (implicit in the call to
`list`), we need to configure the object returned by the call to `foo.iter()`.

    >>> mock_foo = MagicMock()
    >>> mock_foo.iter.return_value = iter([1, 2, 3])
    >>> list(mock_foo.iter())
    [1, 2, 3]

.. [#] There are also generator expressions and more `advanced uses
    <http://www.dabeaz.com/coroutines/index.html>`_ of generators, but we aren't
    concerned about them here. A very good introduction to generators and how
    powerful they are is: `Generator Tricks for Systems Programmers
    <http://www.dabeaz.com/generators/>`_.


Applying the same patch to every test method
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want several patches in place for multiple test methods the obvious way
is to apply the patch decorators to every method. This can feel like unnecessary
repetition. For Python 2.6 or more recent you can use `patch` (in all its
various forms) as a class decorator. This applies the patches to all test
methods on the class. A test method is identified by methods whose names start
with `test`:

    >>> @patch('mymodule.SomeClass')
    ... class MyTest(TestCase):
    ...
    ...     def test_one(self, MockSomeClass):
    ...         self.assertIs(mymodule.SomeClass, MockSomeClass)
    ...
    ...     def test_two(self, MockSomeClass):
    ...         self.assertIs(mymodule.SomeClass, MockSomeClass)
    ...
    ...     def not_a_test(self):
    ...         return 'something'
    ...
    >>> MyTest('test_one').test_one()
    >>> MyTest('test_two').test_two()
    >>> MyTest('test_two').not_a_test()
    'something'

An alternative way of managing patches is to use the :ref:`start-and-stop`.
These allow you to move the patching into your `setUp` and `tearDown` methods.

    >>> class MyTest(TestCase):
    ...     def setUp(self):
    ...         self.patcher = patch('mymodule.foo')
    ...         self.mock_foo = self.patcher.start()
    ...
    ...     def test_foo(self):
    ...         self.assertIs(mymodule.foo, self.mock_foo)
    ...
    ...     def tearDown(self):
    ...         self.patcher.stop()
    ...
    >>> MyTest('test_foo').run()

If you use this technique you must ensure that the patching is "undone" by
calling `stop`. This can be fiddlier than you might think, because if an
exception is raised in the setUp then tearDown is not called.
:meth:`unittest.TestCase.addCleanup` makes this easier:

    >>> class MyTest(TestCase):
    ...     def setUp(self):
    ...         patcher = patch('mymodule.foo')
    ...         self.addCleanup(patcher.stop)
    ...         self.mock_foo = patcher.start()
    ...
    ...     def test_foo(self):
    ...         self.assertIs(mymodule.foo, self.mock_foo)
    ...
    >>> MyTest('test_foo').run()


Mocking Unbound Methods
~~~~~~~~~~~~~~~~~~~~~~~

Whilst writing tests today I needed to patch an *unbound method* (patching the
method on the class rather than on the instance). I needed self to be passed
in as the first argument because I want to make asserts about which objects
were calling this particular method. The issue is that you can't patch with a
mock for this, because if you replace an unbound method with a mock it doesn't
become a bound method when fetched from the instance, and so it doesn't get
self passed in. The workaround is to patch the unbound method with a real
function instead. The :func:`patch` decorator makes it so simple to
patch out methods with a mock that having to create a real function becomes a
nuisance.

If you pass `autospec=True` to patch then it does the patching with a
*real* function object. This function object has the same signature as the one
it is replacing, but delegates to a mock under the hood. You still get your
mock auto-created in exactly the same way as before. What it means though, is
that if you use it to patch out an unbound method on a class the mocked
function will be turned into a bound method if it is fetched from an instance.
It will have `self` passed in as the first argument, which is exactly what I
wanted:

    >>> class Foo:
    ...   def foo(self):
    ...     pass
    ...
    >>> with patch.object(Foo, 'foo', autospec=True) as mock_foo:
    ...   mock_foo.return_value = 'foo'
    ...   foo = Foo()
    ...   foo.foo()
    ...
    'foo'
    >>> mock_foo.assert_called_once_with(foo)

If we don't use `autospec=True` then the unbound method is patched out
with a Mock instance instead, and isn't called with `self`.


Checking multiple calls with mock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

mock has a nice API for making assertions about how your mock objects are used.

    >>> mock = Mock()
    >>> mock.foo_bar.return_value = None
    >>> mock.foo_bar('baz', spam='eggs')
    >>> mock.foo_bar.assert_called_with('baz', spam='eggs')

If your mock is only being called once you can use the
:meth:`assert_called_once_with` method that also asserts that the
:attr:`call_count` is one.

    >>> mock.foo_bar.assert_called_once_with('baz', spam='eggs')
    >>> mock.foo_bar()
    >>> mock.foo_bar.assert_called_once_with('baz', spam='eggs')
    Traceback (most recent call last):
        ...
    AssertionError: Expected to be called once. Called 2 times.

Both `assert_called_with` and `assert_called_once_with` make assertions about
the *most recent* call. If your mock is going to be called several times, and
you want to make assertions about *all* those calls you can use
:attr:`~Mock.call_args_list`:

    >>> mock = Mock(return_value=None)
    >>> mock(1, 2, 3)
    >>> mock(4, 5, 6)
    >>> mock()
    >>> mock.call_args_list
    [call(1, 2, 3), call(4, 5, 6), call()]

The :data:`call` helper makes it easy to make assertions about these calls. You
can build up a list of expected calls and compare it to `call_args_list`. This
looks remarkably similar to the repr of the `call_args_list`:

    >>> expected = [call(1, 2, 3), call(4, 5, 6), call()]
    >>> mock.call_args_list == expected
    True


Coping with mutable arguments
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Another situation is rare, but can bite you, is when your mock is called with
mutable arguments. `call_args` and `call_args_list` store *references* to the
arguments. If the arguments are mutated by the code under test then you can no
longer make assertions about what the values were when the mock was called.

Here's some example code that shows the problem. Imagine the following functions
defined in 'mymodule'::

    def frob(val):
        pass

    def grob(val):
        "First frob and then clear val"
        frob(val)
        val.clear()

When we try to test that `grob` calls `frob` with the correct argument look
what happens:

    >>> with patch('mymodule.frob') as mock_frob:
    ...     val = set([6])
    ...     mymodule.grob(val)
    ...
    >>> val
    set([])
    >>> mock_frob.assert_called_with(set([6]))
    Traceback (most recent call last):
        ...
    AssertionError: Expected: ((set([6]),), {})
    Called with: ((set([]),), {})

One possibility would be for mock to copy the arguments you pass in. This
could then cause problems if you do assertions that rely on object identity
for equality.

Here's one solution that uses the :attr:`side_effect`
functionality. If you provide a `side_effect` function for a mock then
`side_effect` will be called with the same args as the mock. This gives us an
opportunity to copy the arguments and store them for later assertions. In this
example I'm using *another* mock to store the arguments so that I can use the
mock methods for doing the assertion. Again a helper function sets this up for
me.

    >>> from copy import deepcopy
    >>> from unittest.mock import Mock, patch, DEFAULT
    >>> def copy_call_args(mock):
    ...     new_mock = Mock()
    ...     def side_effect(*args, **kwargs):
    ...         args = deepcopy(args)
    ...         kwargs = deepcopy(kwargs)
    ...         new_mock(*args, **kwargs)
    ...         return DEFAULT
    ...     mock.side_effect = side_effect
    ...     return new_mock
    ...
    >>> with patch('mymodule.frob') as mock_frob:
    ...     new_mock = copy_call_args(mock_frob)
    ...     val = set([6])
    ...     mymodule.grob(val)
    ...
    >>> new_mock.assert_called_with(set([6]))
    >>> new_mock.call_args
    call(set([6]))

`copy_call_args` is called with the mock that will be called. It returns a new
mock that we do the assertion on. The `side_effect` function makes a copy of
the args and calls our `new_mock` with the copy.

.. note::

    If your mock is only going to be used once there is an easier way of
    checking arguments at the point they are called. You can simply do the
    checking inside a `side_effect` function.

        >>> def side_effect(arg):
        ...     assert arg == set([6])
        ...
        >>> mock = Mock(side_effect=side_effect)
        >>> mock(set([6]))
        >>> mock(set())
        Traceback (most recent call last):
            ...
        AssertionError

An alternative approach is to create a subclass of `Mock` or `MagicMock` that
copies (using :func:`copy.deepcopy`) the arguments.
Here's an example implementation:

    >>> from copy import deepcopy
    >>> class CopyingMock(MagicMock):
    ...     def __call__(self, *args, **kwargs):
    ...         args = deepcopy(args)
    ...         kwargs = deepcopy(kwargs)
    ...         return super(CopyingMock, self).__call__(*args, **kwargs)
    ...
    >>> c = CopyingMock(return_value=None)
    >>> arg = set()
    >>> c(arg)
    >>> arg.add(1)
    >>> c.assert_called_with(set())
    >>> c.assert_called_with(arg)
    Traceback (most recent call last):
        ...
    AssertionError: Expected call: mock(set([1]))
    Actual call: mock(set([]))
    >>> c.foo
    <CopyingMock name='mock.foo' id='...'>

When you subclass `Mock` or `MagicMock` all dynamically created attributes,
and the `return_value` will use your subclass automatically. That means all
children of a `CopyingMock` will also have the type `CopyingMock`.


Nesting Patches
~~~~~~~~~~~~~~~

Using patch as a context manager is nice, but if you do multiple patches you
can end up with nested with statements indenting further and further to the
right:

    >>> class MyTest(TestCase):
    ...
    ...     def test_foo(self):
    ...         with patch('mymodule.Foo') as mock_foo:
    ...             with patch('mymodule.Bar') as mock_bar:
    ...                 with patch('mymodule.Spam') as mock_spam:
    ...                     assert mymodule.Foo is mock_foo
    ...                     assert mymodule.Bar is mock_bar
    ...                     assert mymodule.Spam is mock_spam
    ...
    >>> original = mymodule.Foo
    >>> MyTest('test_foo').test_foo()
    >>> assert mymodule.Foo is original

With unittest `cleanup` functions and the :ref:`start-and-stop` we can
achieve the same effect without the nested indentation. A simple helper
method, `create_patch`, puts the patch in place and returns the created mock
for us:

    >>> class MyTest(TestCase):
    ...
    ...     def create_patch(self, name):
    ...         patcher = patch(name)
    ...         thing = patcher.start()
    ...         self.addCleanup(patcher.stop)
    ...         return thing
    ...
    ...     def test_foo(self):
    ...         mock_foo = self.create_patch('mymodule.Foo')
    ...         mock_bar = self.create_patch('mymodule.Bar')
    ...         mock_spam = self.create_patch('mymodule.Spam')
    ...
    ...         assert mymodule.Foo is mock_foo
    ...         assert mymodule.Bar is mock_bar
    ...         assert mymodule.Spam is mock_spam
    ...
    >>> original = mymodule.Foo
    >>> MyTest('test_foo').run()
    >>> assert mymodule.Foo is original


Mocking a dictionary with MagicMock
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may want to mock a dictionary, or other container object, recording all
access to it whilst having it still behave like a dictionary.

We can do this with :class:`MagicMock`, which will behave like a dictionary,
and using :data:`~Mock.side_effect` to delegate dictionary access to a real
underlying dictionary that is under our control.

When the `__getitem__` and `__setitem__` methods of our `MagicMock` are called
(normal dictionary access) then `side_effect` is called with the key (and in
the case of `__setitem__` the value too). We can also control what is returned.

After the `MagicMock` has been used we can use attributes like
:data:`~Mock.call_args_list` to assert about how the dictionary was used:

    >>> my_dict = {'a': 1, 'b': 2, 'c': 3}
    >>> def getitem(name):
    ...      return my_dict[name]
    ...
    >>> def setitem(name, val):
    ...     my_dict[name] = val
    ...
    >>> mock = MagicMock()
    >>> mock.__getitem__.side_effect = getitem
    >>> mock.__setitem__.side_effect = setitem

.. note::

    An alternative to using `MagicMock` is to use `Mock` and *only* provide
    the magic methods you specifically want:

        >>> mock = Mock()
        >>> mock.__setitem__ = Mock(side_effect=getitem)
        >>> mock.__getitem__ = Mock(side_effect=setitem)

    A *third* option is to use `MagicMock` but passing in `dict` as the `spec`
    (or `spec_set`) argument so that the `MagicMock` created only has
    dictionary magic methods available:

        >>> mock = MagicMock(spec_set=dict)
        >>> mock.__getitem__.side_effect = getitem
        >>> mock.__setitem__.side_effect = setitem

With these side effect functions in place, the `mock` will behave like a normal
dictionary but recording the access. It even raises a `KeyError` if you try
to access a key that doesn't exist.

    >>> mock['a']
    1
    >>> mock['c']
    3
    >>> mock['d']
    Traceback (most recent call last):
        ...
    KeyError: 'd'
    >>> mock['b'] = 'fish'
    >>> mock['d'] = 'eggs'
    >>> mock['b']
    'fish'
    >>> mock['d']
    'eggs'

After it has been used you can make assertions about the access using the normal
mock methods and attributes:

    >>> mock.__getitem__.call_args_list
    [call('a'), call('c'), call('d'), call('b'), call('d')]
    >>> mock.__setitem__.call_args_list
    [call('b', 'fish'), call('d', 'eggs')]
    >>> my_dict
    {'a': 1, 'c': 3, 'b': 'fish', 'd': 'eggs'}


Mock subclasses and their attributes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

There are various reasons why you might want to subclass `Mock`. One reason
might be to add helper methods. Here's a silly example:

    >>> class MyMock(MagicMock):
    ...     def has_been_called(self):
    ...         return self.called
    ...
    >>> mymock = MyMock(return_value=None)
    >>> mymock
    <MyMock id='...'>
    >>> mymock.has_been_called()
    False
    >>> mymock()
    >>> mymock.has_been_called()
    True

The standard behaviour for `Mock` instances is that attributes and the return
value mocks are of the same type as the mock they are accessed on. This ensures
that `Mock` attributes are `Mocks` and `MagicMock` attributes are `MagicMocks`
[#]_. So if you're subclassing to add helper methods then they'll also be
available on the attributes and return value mock of instances of your
subclass.

    >>> mymock.foo
    <MyMock name='mock.foo' id='...'>
    >>> mymock.foo.has_been_called()
    False
    >>> mymock.foo()
    <MyMock name='mock.foo()' id='...'>
    >>> mymock.foo.has_been_called()
    True

Sometimes this is inconvenient. For example, `one user
<https://code.google.com/p/mock/issues/detail?id=105>`_ is subclassing mock to
created a `Twisted adaptor
<http://twistedmatrix.com/documents/11.0.0/api/twisted.python.components.html>`_.
Having this applied to attributes too actually causes errors.

`Mock` (in all its flavours) uses a method called `_get_child_mock` to create
these "sub-mocks" for attributes and return values. You can prevent your
subclass being used for attributes by overriding this method. The signature is
that it takes arbitrary keyword arguments (`**kwargs`) which are then passed
onto the mock constructor:

    >>> class Subclass(MagicMock):
    ...     def _get_child_mock(self, **kwargs):
    ...         return MagicMock(**kwargs)
    ...
    >>> mymock = Subclass()
    >>> mymock.foo
    <MagicMock name='mock.foo' id='...'>
    >>> assert isinstance(mymock, Subclass)
    >>> assert not isinstance(mymock.foo, Subclass)
    >>> assert not isinstance(mymock(), Subclass)

.. [#] An exception to this rule are the non-callable mocks. Attributes use the
    callable variant because otherwise non-callable mocks couldn't have callable
    methods.


Mocking imports with patch.dict
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

One situation where mocking can be hard is where you have a local import inside
a function. These are harder to mock because they aren't using an object from
the module namespace that we can patch out.

Generally local imports are to be avoided. They are sometimes done to prevent
circular dependencies, for which there is *usually* a much better way to solve
the problem (refactor the code) or to prevent "up front costs" by delaying the
import. This can also be solved in better ways than an unconditional local
import (store the module as a class or module attribute and only do the import
on first use).

That aside there is a way to use `mock` to affect the results of an import.
Importing fetches an *object* from the `sys.modules` dictionary. Note that it
fetches an *object*, which need not be a module. Importing a module for the
first time results in a module object being put in `sys.modules`, so usually
when you import something you get a module back. This need not be the case
however.

This means you can use :func:`patch.dict` to *temporarily* put a mock in place
in `sys.modules`. Any imports whilst this patch is active will fetch the mock.
When the patch is complete (the decorated function exits, the with statement
body is complete or `patcher.stop()` is called) then whatever was there
previously will be restored safely.

Here's an example that mocks out the 'fooble' module.

    >>> mock = Mock()
    >>> with patch.dict('sys.modules', {'fooble': mock}):
    ...    import fooble
    ...    fooble.blob()
    ...
    <Mock name='mock.blob()' id='...'>
    >>> assert 'fooble' not in sys.modules
    >>> mock.blob.assert_called_once_with()

As you can see the `import fooble` succeeds, but on exit there is no 'fooble'
left in `sys.modules`.

This also works for the `from module import name` form:

    >>> mock = Mock()
    >>> with patch.dict('sys.modules', {'fooble': mock}):
    ...    from fooble import blob
    ...    blob.blip()
    ...
    <Mock name='mock.blob.blip()' id='...'>
    >>> mock.blob.blip.assert_called_once_with()

With slightly more work you can also mock package imports:

    >>> mock = Mock()
    >>> modules = {'package': mock, 'package.module': mock.module}
    >>> with patch.dict('sys.modules', modules):
    ...    from package.module import fooble
    ...    fooble()
    ...
    <Mock name='mock.module.fooble()' id='...'>
    >>> mock.module.fooble.assert_called_once_with()


Tracking order of calls and less verbose call assertions
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The :class:`Mock` class allows you to track the *order* of method calls on
your mock objects through the :attr:`~Mock.method_calls` attribute. This
doesn't allow you to track the order of calls between separate mock objects,
however we can use :attr:`~Mock.mock_calls` to achieve the same effect.

Because mocks track calls to child mocks in `mock_calls`, and accessing an
arbitrary attribute of a mock creates a child mock, we can create our separate
mocks from a parent one. Calls to those child mock will then all be recorded,
in order, in the `mock_calls` of the parent:

    >>> manager = Mock()
    >>> mock_foo = manager.foo
    >>> mock_bar = manager.bar

    >>> mock_foo.something()
    <Mock name='mock.foo.something()' id='...'>
    >>> mock_bar.other.thing()
    <Mock name='mock.bar.other.thing()' id='...'>

    >>> manager.mock_calls
    [call.foo.something(), call.bar.other.thing()]

We can then assert about the calls, including the order, by comparing with
the `mock_calls` attribute on the manager mock:

    >>> expected_calls = [call.foo.something(), call.bar.other.thing()]
    >>> manager.mock_calls == expected_calls
    True

If `patch` is creating, and putting in place, your mocks then you can attach
them to a manager mock using the :meth:`~Mock.attach_mock` method. After
attaching calls will be recorded in `mock_calls` of the manager.

    >>> manager = MagicMock()
    >>> with patch('mymodule.Class1') as MockClass1:
    ...     with patch('mymodule.Class2') as MockClass2:
    ...         manager.attach_mock(MockClass1, 'MockClass1')
    ...         manager.attach_mock(MockClass2, 'MockClass2')
    ...         MockClass1().foo()
    ...         MockClass2().bar()
    ...
    <MagicMock name='mock.MockClass1().foo()' id='...'>
    <MagicMock name='mock.MockClass2().bar()' id='...'>
    >>> manager.mock_calls
    [call.MockClass1(),
     call.MockClass1().foo(),
     call.MockClass2(),
     call.MockClass2().bar()]

If many calls have been made, but you're only interested in a particular
sequence of them then an alternative is to use the
:meth:`~Mock.assert_has_calls` method. This takes a list of calls (constructed
with the :data:`call` object). If that sequence of calls are in
:attr:`~Mock.mock_calls` then the assert succeeds.

    >>> m = MagicMock()
    >>> m().foo().bar().baz()
    <MagicMock name='mock().foo().bar().baz()' id='...'>
    >>> m.one().two().three()
    <MagicMock name='mock.one().two().three()' id='...'>
    >>> calls = call.one().two().three().call_list()
    >>> m.assert_has_calls(calls)

Even though the chained call `m.one().two().three()` aren't the only calls that
have been made to the mock, the assert still succeeds.

Sometimes a mock may have several calls made to it, and you are only interested
in asserting about *some* of those calls. You may not even care about the
order. In this case you can pass `any_order=True` to `assert_has_calls`:

    >>> m = MagicMock()
    >>> m(1), m.two(2, 3), m.seven(7), m.fifty('50')
    (...)
    >>> calls = [call.fifty('50'), call(1), call.seven(7)]
    >>> m.assert_has_calls(calls, any_order=True)


More complex argument matching
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Using the same basic concept as :data:`ANY` we can implement matchers to do more
complex assertions on objects used as arguments to mocks.

Suppose we expect some object to be passed to a mock that by default
compares equal based on object identity (which is the Python default for user
defined classes). To use :meth:`~Mock.assert_called_with` we would need to pass
in the exact same object. If we are only interested in some of the attributes
of this object then we can create a matcher that will check these attributes
for us.

You can see in this example how a 'standard' call to `assert_called_with` isn't
sufficient:

    >>> class Foo:
    ...     def __init__(self, a, b):
    ...         self.a, self.b = a, b
    ...
    >>> mock = Mock(return_value=None)
    >>> mock(Foo(1, 2))
    >>> mock.assert_called_with(Foo(1, 2))
    Traceback (most recent call last):
        ...
    AssertionError: Expected: call(<__main__.Foo object at 0x...>)
    Actual call: call(<__main__.Foo object at 0x...>)

A comparison function for our `Foo` class might look something like this:

    >>> def compare(self, other):
    ...     if not type(self) == type(other):
    ...         return False
    ...     if self.a != other.a:
    ...         return False
    ...     if self.b != other.b:
    ...         return False
    ...     return True
    ...

And a matcher object that can use comparison functions like this for its
equality operation would look something like this:

    >>> class Matcher:
    ...     def __init__(self, compare, some_obj):
    ...         self.compare = compare
    ...         self.some_obj = some_obj
    ...     def __eq__(self, other):
    ...         return self.compare(self.some_obj, other)
    ...

Putting all this together:

    >>> match_foo = Matcher(compare, Foo(1, 2))
    >>> mock.assert_called_with(match_foo)

The `Matcher` is instantiated with our compare function and the `Foo` object
we want to compare against. In `assert_called_with` the `Matcher` equality
method will be called, which compares the object the mock was called with
against the one we created our matcher with. If they match then
`assert_called_with` passes, and if they don't an `AssertionError` is raised:

    >>> match_wrong = Matcher(compare, Foo(3, 4))
    >>> mock.assert_called_with(match_wrong)
    Traceback (most recent call last):
        ...
    AssertionError: Expected: ((<Matcher object at 0x...>,), {})
    Called with: ((<Foo object at 0x...>,), {})

With a bit of tweaking you could have the comparison function raise the
`AssertionError` directly and provide a more useful failure message.

As of version 1.5, the Python testing library `PyHamcrest
<http://pypi.python.org/pypi/PyHamcrest>`_ provides similar functionality,
that may be useful here, in the form of its equality matcher
(`hamcrest.library.integration.match_equality
<http://packages.python.org/PyHamcrest/integration.html#hamcrest.library.integration.match_equality>`_).
