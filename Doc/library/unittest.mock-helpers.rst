:mod:`unittest.mock` --- helpers
================================

.. module:: unittest.mock
   :synopsis: Mock object library.
.. moduleauthor:: Michael Foord <michael@python.org>
.. currentmodule:: unittest.mock

.. versionadded:: 3.3


sentinel
--------

.. data:: sentinel

    The ``sentinel`` object provides a convenient way of providing unique
    objects for your tests.

    Attributes are created on demand when you access them by name. Accessing
    the same attribute will always return the same object. The objects
    returned have a sensible repr so that test failure messages are readable.

Sometimes when testing you need to test that a specific object is passed as an
argument to another method, or returned. It can be common to create named
sentinel objects to test this. `sentinel` provides a convenient way of
creating and testing the identity of objects like this.

In this example we monkey patch `method` to return `sentinel.some_object`:

    >>> real = ProductionClass()
    >>> real.method = Mock(name="method")
    >>> real.method.return_value = sentinel.some_object
    >>> result = real.method()
    >>> assert result is sentinel.some_object
    >>> sentinel.some_object
    sentinel.some_object


DEFAULT
-------


.. data:: DEFAULT

    The `DEFAULT` object is a pre-created sentinel (actually
    `sentinel.DEFAULT`). It can be used by :attr:`~Mock.side_effect`
    functions to indicate that the normal return value should be used.



call
----

.. function:: call(*args, **kwargs)

    `call` is a helper object for making simpler assertions, for comparing
    with :attr:`~Mock.call_args`, :attr:`~Mock.call_args_list`,
    :attr:`~Mock.mock_calls` and:attr: `~Mock.method_calls`. `call` can also be
    used with :meth:`~Mock.assert_has_calls`.

        >>> m = MagicMock(return_value=None)
        >>> m(1, 2, a='foo', b='bar')
        >>> m()
        >>> m.call_args_list == [call(1, 2, a='foo', b='bar'), call()]
        True

.. method:: call.call_list()

    For a call object that represents multiple calls, `call_list`
    returns a list of all the intermediate calls as well as the
    final call.

`call_list` is particularly useful for making assertions on "chained calls". A
chained call is multiple calls on a single line of code. This results in
multiple entries in :attr:`~Mock.mock_calls` on a mock. Manually constructing
the sequence of calls can be tedious.

:meth:`~call.call_list` can construct the sequence of calls from the same
chained call:

    >>> m = MagicMock()
    >>> m(1).method(arg='foo').other('bar')(2.0)
    <MagicMock name='mock().method().other()()' id='...'>
    >>> kall = call(1).method(arg='foo').other('bar')(2.0)
    >>> kall.call_list()
    [call(1),
     call().method(arg='foo'),
     call().method().other('bar'),
     call().method().other()(2.0)]
    >>> m.mock_calls == kall.call_list()
    True

.. _calls-as-tuples:

A `call` object is either a tuple of (positional args, keyword args) or
(name, positional args, keyword args) depending on how it was constructed. When
you construct them yourself this isn't particularly interesting, but the `call`
objects that are in the :attr:`Mock.call_args`, :attr:`Mock.call_args_list` and
:attr:`Mock.mock_calls` attributes can be introspected to get at the individual
arguments they contain.

The `call` objects in :attr:`Mock.call_args` and :attr:`Mock.call_args_list`
are two-tuples of (positional args, keyword args) whereas the `call` objects
in :attr:`Mock.mock_calls`, along with ones you construct yourself, are
three-tuples of (name, positional args, keyword args).

You can use their "tupleness" to pull out the individual arguments for more
complex introspection and assertions. The positional arguments are a tuple
(an empty tuple if there are no positional arguments) and the keyword
arguments are a dictionary:

    >>> m = MagicMock(return_value=None)
    >>> m(1, 2, 3, arg='one', arg2='two')
    >>> kall = m.call_args
    >>> args, kwargs = kall
    >>> args
    (1, 2, 3)
    >>> kwargs
    {'arg2': 'two', 'arg': 'one'}
    >>> args is kall[0]
    True
    >>> kwargs is kall[1]
    True

    >>> m = MagicMock()
    >>> m.foo(4, 5, 6, arg='two', arg2='three')
    <MagicMock name='mock.foo()' id='...'>
    >>> kall = m.mock_calls[0]
    >>> name, args, kwargs = kall
    >>> name
    'foo'
    >>> args
    (4, 5, 6)
    >>> kwargs
    {'arg2': 'three', 'arg': 'two'}
    >>> name is m.mock_calls[0][0]
    True


create_autospec
---------------

.. function:: create_autospec(spec, spec_set=False, instance=False, **kwargs)

    Create a mock object using another object as a spec. Attributes on the
    mock will use the corresponding attribute on the `spec` object as their
    spec.

    Functions or methods being mocked will have their arguments checked to
    ensure that they are called with the correct signature.

    If `spec_set` is `True` then attempting to set attributes that don't exist
    on the spec object will raise an `AttributeError`.

    If a class is used as a spec then the return value of the mock (the
    instance of the class) will have the same spec. You can use a class as the
    spec for an instance object by passing `instance=True`. The returned mock
    will only be callable if instances of the mock are callable.

    `create_autospec` also takes arbitrary keyword arguments that are passed to
    the constructor of the created mock.

See :ref:`auto-speccing` for examples of how to use auto-speccing with
`create_autospec` and the `autospec` argument to :func:`patch`.


ANY
---

.. data:: ANY

Sometimes you may need to make assertions about *some* of the arguments in a
call to mock, but either not care about some of the arguments or want to pull
them individually out of :attr:`~Mock.call_args` and make more complex
assertions on them.

To ignore certain arguments you can pass in objects that compare equal to
*everything*. Calls to :meth:`~Mock.assert_called_with` and
:meth:`~Mock.assert_called_once_with` will then succeed no matter what was
passed in.

    >>> mock = Mock(return_value=None)
    >>> mock('foo', bar=object())
    >>> mock.assert_called_once_with('foo', bar=ANY)

`ANY` can also be used in comparisons with call lists like
:attr:`~Mock.mock_calls`:

    >>> m = MagicMock(return_value=None)
    >>> m(1)
    >>> m(1, 2)
    >>> m(object())
    >>> m.mock_calls == [call(1), call(1, 2), ANY]
    True



FILTER_DIR
----------

.. data:: FILTER_DIR

`FILTER_DIR` is a module level variable that controls the way mock objects
respond to `dir` (only for Python 2.6 or more recent). The default is `True`,
which uses the filtering described below, to only show useful members. If you
dislike this filtering, or need to switch it off for diagnostic purposes, then
set `mock.FILTER_DIR = False`.

With filtering on, `dir(some_mock)` shows only useful attributes and will
include any dynamically created attributes that wouldn't normally be shown.
If the mock was created with a `spec` (or `autospec` of course) then all the
attributes from the original are shown, even if they haven't been accessed
yet:

    >>> dir(Mock())
    ['assert_any_call',
     'assert_called_once_with',
     'assert_called_with',
     'assert_has_calls',
     'attach_mock',
     ...
    >>> from urllib import request
    >>> dir(Mock(spec=request))
    ['AbstractBasicAuthHandler',
     'AbstractDigestAuthHandler',
     'AbstractHTTPHandler',
     'BaseHandler',
     ...

Many of the not-very-useful (private to `Mock` rather than the thing being
mocked) underscore and double underscore prefixed attributes have been
filtered from the result of calling `dir` on a `Mock`. If you dislike this
behaviour you can switch it off by setting the module level switch
`FILTER_DIR`:

    >>> from unittest import mock
    >>> mock.FILTER_DIR = False
    >>> dir(mock.Mock())
    ['_NonCallableMock__get_return_value',
     '_NonCallableMock__get_side_effect',
     '_NonCallableMock__return_value_doc',
     '_NonCallableMock__set_return_value',
     '_NonCallableMock__set_side_effect',
     '__call__',
     '__class__',
     ...

Alternatively you can just use `vars(my_mock)` (instance members) and
`dir(type(my_mock))` (type members) to bypass the filtering irrespective of
`mock.FILTER_DIR`.


mock_open
---------

.. function:: mock_open(mock=None, read_data=None)

    A helper function to create a mock to replace the use of `open`. It works
    for `open` called directly or used as a context manager.

    The `mock` argument is the mock object to configure. If `None` (the
    default) then a `MagicMock` will be created for you, with the API limited
    to methods or attributes available on standard file handles.

    `read_data` is a string for the `read` method of the file handle to return.
    This is an empty string by default.

Using `open` as a context manager is a great way to ensure your file handles
are closed properly and is becoming common::

    with open('/some/path', 'w') as f:
        f.write('something')

The issue is that even if you mock out the call to `open` it is the
*returned object* that is used as a context manager (and has `__enter__` and
`__exit__` called).

Mocking context managers with a :class:`MagicMock` is common enough and fiddly
enough that a helper function is useful.

    >>> m = mock_open()
    >>> with patch('__main__.open', m, create=True):
    ...     with open('foo', 'w') as h:
    ...         h.write('some stuff')
    ...
    >>> m.mock_calls
    [call('foo', 'w'),
     call().__enter__(),
     call().write('some stuff'),
     call().__exit__(None, None, None)]
    >>> m.assert_called_once_with('foo', 'w')
    >>> handle = m()
    >>> handle.write.assert_called_once_with('some stuff')

And for reading files:

    >>> with patch('__main__.open', mock_open(read_data='bibble'), create=True) as m:
    ...     with open('foo') as h:
    ...         result = h.read()
    ...
    >>> m.assert_called_once_with('foo')
    >>> assert result == 'bibble'


.. _auto-speccing:

Autospeccing
------------

Autospeccing is based on the existing `spec` feature of mock. It limits the
api of mocks to the api of an original object (the spec), but it is recursive
(implemented lazily) so that attributes of mocks only have the same api as
the attributes of the spec. In addition mocked functions / methods have the
same call signature as the original so they raise a `TypeError` if they are
called incorrectly.

Before I explain how auto-speccing works, here's why it is needed.

`Mock` is a very powerful and flexible object, but it suffers from two flaws
when used to mock out objects from a system under test. One of these flaws is
specific to the `Mock` api and the other is a more general problem with using
mock objects.

First the problem specific to `Mock`. `Mock` has two assert methods that are
extremely handy: :meth:`~Mock.assert_called_with` and
:meth:`~Mock.assert_called_once_with`.

    >>> mock = Mock(name='Thing', return_value=None)
    >>> mock(1, 2, 3)
    >>> mock.assert_called_once_with(1, 2, 3)
    >>> mock(1, 2, 3)
    >>> mock.assert_called_once_with(1, 2, 3)
    Traceback (most recent call last):
     ...
    AssertionError: Expected to be called once. Called 2 times.

Because mocks auto-create attributes on demand, and allow you to call them
with arbitrary arguments, if you misspell one of these assert methods then
your assertion is gone:

.. code-block:: pycon

    >>> mock = Mock(name='Thing', return_value=None)
    >>> mock(1, 2, 3)
    >>> mock.assret_called_once_with(4, 5, 6)

Your tests can pass silently and incorrectly because of the typo.

The second issue is more general to mocking. If you refactor some of your
code, rename members and so on, any tests for code that is still using the
*old api* but uses mocks instead of the real objects will still pass. This
means your tests can all pass even though your code is broken.

Note that this is another reason why you need integration tests as well as
unit tests. Testing everything in isolation is all fine and dandy, but if you
don't test how your units are "wired together" there is still lots of room
for bugs that tests might have caught.

`mock` already provides a feature to help with this, called speccing. If you
use a class or instance as the `spec` for a mock then you can only access
attributes on the mock that exist on the real class:

    >>> from urllib import request
    >>> mock = Mock(spec=request.Request)
    >>> mock.assret_called_with
    Traceback (most recent call last):
     ...
    AttributeError: Mock object has no attribute 'assret_called_with'

The spec only applies to the mock itself, so we still have the same issue
with any methods on the mock:

.. code-block:: pycon

    >>> mock.has_data()
    <mock.Mock object at 0x...>
    >>> mock.has_data.assret_called_with()

Auto-speccing solves this problem. You can either pass `autospec=True` to
`patch` / `patch.object` or use the `create_autospec` function to create a
mock with a spec. If you use the `autospec=True` argument to `patch` then the
object that is being replaced will be used as the spec object. Because the
speccing is done "lazily" (the spec is created as attributes on the mock are
accessed) you can use it with very complex or deeply nested objects (like
modules that import modules that import modules) without a big performance
hit.

Here's an example of it in use:

    >>> from urllib import request
    >>> patcher = patch('__main__.request', autospec=True)
    >>> mock_request = patcher.start()
    >>> request is mock_request
    True
    >>> mock_request.Request
    <MagicMock name='request.Request' spec='Request' id='...'>

You can see that `request.Request` has a spec. `request.Request` takes two
arguments in the constructor (one of which is `self`). Here's what happens if
we try to call it incorrectly:

    >>> req = request.Request()
    Traceback (most recent call last):
     ...
    TypeError: <lambda>() takes at least 2 arguments (1 given)

The spec also applies to instantiated classes (i.e. the return value of
specced mocks):

    >>> req = request.Request('foo')
    >>> req
    <NonCallableMagicMock name='request.Request()' spec='Request' id='...'>

`Request` objects are not callable, so the return value of instantiating our
mocked out `request.Request` is a non-callable mock. With the spec in place
any typos in our asserts will raise the correct error:

    >>> req.add_header('spam', 'eggs')
    <MagicMock name='request.Request().add_header()' id='...'>
    >>> req.add_header.assret_called_with
    Traceback (most recent call last):
     ...
    AttributeError: Mock object has no attribute 'assret_called_with'
    >>> req.add_header.assert_called_with('spam', 'eggs')

In many cases you will just be able to add `autospec=True` to your existing
`patch` calls and then be protected against bugs due to typos and api
changes.

As well as using `autospec` through `patch` there is a
:func:`create_autospec` for creating autospecced mocks directly:

    >>> from urllib import request
    >>> mock_request = create_autospec(request)
    >>> mock_request.Request('foo', 'bar')
    <NonCallableMagicMock name='mock.Request()' spec='Request' id='...'>

This isn't without caveats and limitations however, which is why it is not
the default behaviour. In order to know what attributes are available on the
spec object, autospec has to introspect (access attributes) the spec. As you
traverse attributes on the mock a corresponding traversal of the original
object is happening under the hood. If any of your specced objects have
properties or descriptors that can trigger code execution then you may not be
able to use autospec. On the other hand it is much better to design your
objects so that introspection is safe [#]_.

A more serious problem is that it is common for instance attributes to be
created in the `__init__` method and not to exist on the class at all.
`autospec` can't know about any dynamically created attributes and restricts
the api to visible attributes.

    >>> class Something(object):
    ...   def __init__(self):
    ...     self.a = 33
    ...
    >>> with patch('__main__.Something', autospec=True):
    ...   thing = Something()
    ...   thing.a
    ...
    Traceback (most recent call last):
      ...
    AttributeError: Mock object has no attribute 'a'

There are a few different ways of resolving this problem. The easiest, but
not necessarily the least annoying, way is to simply set the required
attributes on the mock after creation. Just because `autospec` doesn't allow
you to fetch attributes that don't exist on the spec it doesn't prevent you
setting them:

    >>> with patch('__main__.Something', autospec=True):
    ...   thing = Something()
    ...   thing.a = 33
    ...

There is a more aggressive version of both `spec` and `autospec` that *does*
prevent you setting non-existent attributes. This is useful if you want to
ensure your code only *sets* valid attributes too, but obviously it prevents
this particular scenario:

    >>> with patch('__main__.Something', autospec=True, spec_set=True):
    ...   thing = Something()
    ...   thing.a = 33
    ...
    Traceback (most recent call last):
     ...
    AttributeError: Mock object has no attribute 'a'

Probably the best way of solving the problem is to add class attributes as
default values for instance members initialised in `__init__`. Note that if
you are only setting default attributes in `__init__` then providing them via
class attributes (shared between instances of course) is faster too. e.g.

.. code-block:: python

    class Something(object):
        a = 33

This brings up another issue. It is relatively common to provide a default
value of `None` for members that will later be an object of a different type.
`None` would be useless as a spec because it wouldn't let you access *any*
attributes or methods on it. As `None` is *never* going to be useful as a
spec, and probably indicates a member that will normally of some other type,
`autospec` doesn't use a spec for members that are set to `None`. These will
just be ordinary mocks (well - `MagicMocks`):

    >>> class Something(object):
    ...     member = None
    ...
    >>> mock = create_autospec(Something)
    >>> mock.member.foo.bar.baz()
    <MagicMock name='mock.member.foo.bar.baz()' id='...'>

If modifying your production classes to add defaults isn't to your liking
then there are more options. One of these is simply to use an instance as the
spec rather than the class. The other is to create a subclass of the
production class and add the defaults to the subclass without affecting the
production class. Both of these require you to use an alternative object as
the spec. Thankfully `patch` supports this - you can simply pass the
alternative object as the `autospec` argument:

    >>> class Something(object):
    ...   def __init__(self):
    ...     self.a = 33
    ...
    >>> class SomethingForTest(Something):
    ...   a = 33
    ...
    >>> p = patch('__main__.Something', autospec=SomethingForTest)
    >>> mock = p.start()
    >>> mock.a
    <NonCallableMagicMock name='Something.a' spec='int' id='...'>


.. [#] This only applies to classes or already instantiated objects. Calling
   a mocked class to create a mock instance *does not* create a real instance.
   It is only attribute lookups - along with calls to `dir` - that are done.
