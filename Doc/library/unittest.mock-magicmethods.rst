:mod:`unittest.mock` --- MagicMock and magic method support
===========================================================

.. module:: unittest.mock
   :synopsis: Mock object library.
.. moduleauthor:: Michael Foord <michael@python.org>
.. currentmodule:: unittest.mock

.. versionadded:: 3.3


.. _magic-methods:

Mocking Magic Methods
---------------------

:class:`Mock` supports mocking the Python protocol methods, also known as
"magic methods". This allows mock objects to replace containers or other
objects that implement Python protocols.

Because magic methods are looked up differently from normal methods [#]_, this
support has been specially implemented. This means that only specific magic
methods are supported. The supported list includes *almost* all of them. If
there are any missing that you need please let us know.

You mock magic methods by setting the method you are interested in to a function
or a mock instance. If you are using a function then it *must* take ``self`` as
the first argument [#]_.

   >>> def __str__(self):
   ...     return 'fooble'
   ...
   >>> mock = Mock()
   >>> mock.__str__ = __str__
   >>> str(mock)
   'fooble'

   >>> mock = Mock()
   >>> mock.__str__ = Mock()
   >>> mock.__str__.return_value = 'fooble'
   >>> str(mock)
   'fooble'

   >>> mock = Mock()
   >>> mock.__iter__ = Mock(return_value=iter([]))
   >>> list(mock)
   []

One use case for this is for mocking objects used as context managers in a
`with` statement:

   >>> mock = Mock()
   >>> mock.__enter__ = Mock(return_value='foo')
   >>> mock.__exit__ = Mock(return_value=False)
   >>> with mock as m:
   ...     assert m == 'foo'
   ...
   >>> mock.__enter__.assert_called_with()
   >>> mock.__exit__.assert_called_with(None, None, None)

Calls to magic methods do not appear in :attr:`~Mock.method_calls`, but they
are recorded in :attr:`~Mock.mock_calls`.

.. note::

   If you use the `spec` keyword argument to create a mock then attempting to
   set a magic method that isn't in the spec will raise an `AttributeError`.

The full list of supported magic methods is:

* ``__hash__``, ``__sizeof__``, ``__repr__`` and ``__str__``
* ``__dir__``, ``__format__`` and ``__subclasses__``
* ``__floor__``, ``__trunc__`` and ``__ceil__``
* Comparisons: ``__cmp__``, ``__lt__``, ``__gt__``, ``__le__``, ``__ge__``,
  ``__eq__`` and ``__ne__``
* Container methods: ``__getitem__``, ``__setitem__``, ``__delitem__``,
  ``__contains__``, ``__len__``, ``__iter__``, ``__getslice__``,
  ``__setslice__``, ``__reversed__`` and ``__missing__``
* Context manager: ``__enter__`` and ``__exit__``
* Unary numeric methods: ``__neg__``, ``__pos__`` and ``__invert__``
* The numeric methods (including right hand and in-place variants):
  ``__add__``, ``__sub__``, ``__mul__``, ``__div__``,
  ``__floordiv__``, ``__mod__``, ``__divmod__``, ``__lshift__``,
  ``__rshift__``, ``__and__``, ``__xor__``, ``__or__``, and ``__pow__``
* Numeric conversion methods: ``__complex__``, ``__int__``, ``__float__``,
  ``__index__`` and ``__coerce__``
* Descriptor methods: ``__get__``, ``__set__`` and ``__delete__``
* Pickling: ``__reduce__``, ``__reduce_ex__``, ``__getinitargs__``,
  ``__getnewargs__``, ``__getstate__`` and ``__setstate__``


The following methods exist but are *not* supported as they are either in use
by mock, can't be set dynamically, or can cause problems:

* ``__getattr__``, ``__setattr__``, ``__init__`` and ``__new__``
* ``__prepare__``, ``__instancecheck__``, ``__subclasscheck__``, ``__del__``



Magic Mock
----------

There are two `MagicMock` variants: `MagicMock` and `NonCallableMagicMock`.


.. class:: MagicMock(*args, **kw)

   ``MagicMock`` is a subclass of :class:`Mock` with default implementations
   of most of the magic methods. You can use ``MagicMock`` without having to
   configure the magic methods yourself.

   The constructor parameters have the same meaning as for :class:`Mock`.

   If you use the `spec` or `spec_set` arguments then *only* magic methods
   that exist in the spec will be created.


.. class:: NonCallableMagicMock(*args, **kw)

    A non-callable version of `MagicMock`.

    The constructor parameters have the same meaning as for
    :class:`MagicMock`, with the exception of `return_value` and
    `side_effect` which have no meaning on a non-callable mock.

The magic methods are setup with `MagicMock` objects, so you can configure them
and use them in the usual way:

   >>> mock = MagicMock()
   >>> mock[3] = 'fish'
   >>> mock.__setitem__.assert_called_with(3, 'fish')
   >>> mock.__getitem__.return_value = 'result'
   >>> mock[2]
   'result'

By default many of the protocol methods are required to return objects of a
specific type. These methods are preconfigured with a default return value, so
that they can be used without you having to do anything if you aren't interested
in the return value. You can still *set* the return value manually if you want
to change the default.

Methods and their defaults:

* ``__lt__``: NotImplemented
* ``__gt__``: NotImplemented
* ``__le__``: NotImplemented
* ``__ge__``: NotImplemented
* ``__int__`` : 1
* ``__contains__`` : False
* ``__len__`` : 1
* ``__iter__`` : iter([])
* ``__exit__`` : False
* ``__complex__`` : 1j
* ``__float__`` : 1.0
* ``__bool__`` : True
* ``__index__`` : 1
* ``__hash__`` : default hash for the mock
* ``__str__`` : default str for the mock
* ``__sizeof__``: default sizeof for the mock

For example:

   >>> mock = MagicMock()
   >>> int(mock)
   1
   >>> len(mock)
   0
   >>> list(mock)
   []
   >>> object() in mock
   False

The two equality method, `__eq__` and `__ne__`, are special.
They do the default equality comparison on identity, using a side
effect, unless you change their return value to return something else:

   >>> MagicMock() == 3
   False
   >>> MagicMock() != 3
   True
   >>> mock = MagicMock()
   >>> mock.__eq__.return_value = True
   >>> mock == 3
   True

The return value of `MagicMock.__iter__` can be any iterable object and isn't
required to be an iterator:

   >>> mock = MagicMock()
   >>> mock.__iter__.return_value = ['a', 'b', 'c']
   >>> list(mock)
   ['a', 'b', 'c']
   >>> list(mock)
   ['a', 'b', 'c']

If the return value *is* an iterator, then iterating over it once will consume
it and subsequent iterations will result in an empty list:

   >>> mock.__iter__.return_value = iter(['a', 'b', 'c'])
   >>> list(mock)
   ['a', 'b', 'c']
   >>> list(mock)
   []

``MagicMock`` has all of the supported magic methods configured except for some
of the obscure and obsolete ones. You can still set these up if you want.

Magic methods that are supported but not setup by default in ``MagicMock`` are:

* ``__subclasses__``
* ``__dir__``
* ``__format__``
* ``__get__``, ``__set__`` and ``__delete__``
* ``__reversed__`` and ``__missing__``
* ``__reduce__``, ``__reduce_ex__``, ``__getinitargs__``, ``__getnewargs__``,
  ``__getstate__`` and ``__setstate__``
* ``__getformat__`` and ``__setformat__``



.. [#] Magic methods *should* be looked up on the class rather than the
   instance. Different versions of Python are inconsistent about applying this
   rule. The supported protocol methods should work with all supported versions
   of Python.
.. [#] The function is basically hooked up to the class, but each ``Mock``
   instance is kept isolated from the others.
