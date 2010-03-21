# -*- coding: utf-8 -*-

"""Doctest for method/function calls.

We're going the use these types for extra testing

    >>> from UserList import UserList
    >>> from UserDict import UserDict

We're defining four helper functions

    >>> def e(a,b):
    ...     print a, b

    >>> def f(*a, **k):
    ...     print a, test_support.sortdict(k)

    >>> def g(x, *y, **z):
    ...     print x, y, test_support.sortdict(z)

    >>> def h(j=1, a=2, h=3):
    ...     print j, a, h

Argument list examples

    >>> f()
    () {}
    >>> f(1)
    (1,) {}
    >>> f(1, 2)
    (1, 2) {}
    >>> f(1, 2, 3)
    (1, 2, 3) {}
    >>> f(1, 2, 3, *(4, 5))
    (1, 2, 3, 4, 5) {}
    >>> f(1, 2, 3, *[4, 5])
    (1, 2, 3, 4, 5) {}
    >>> f(1, 2, 3, *UserList([4, 5]))
    (1, 2, 3, 4, 5) {}

Here we add keyword arguments

    >>> f(1, 2, 3, **{'a':4, 'b':5})
    (1, 2, 3) {'a': 4, 'b': 5}
    >>> f(1, 2, 3, *[4, 5], **{'a':6, 'b':7})
    (1, 2, 3, 4, 5) {'a': 6, 'b': 7}
    >>> f(1, 2, 3, x=4, y=5, *(6, 7), **{'a':8, 'b': 9})
    (1, 2, 3, 6, 7) {'a': 8, 'b': 9, 'x': 4, 'y': 5}

    >>> f(1, 2, 3, **UserDict(a=4, b=5))
    (1, 2, 3) {'a': 4, 'b': 5}
    >>> f(1, 2, 3, *(4, 5), **UserDict(a=6, b=7))
    (1, 2, 3, 4, 5) {'a': 6, 'b': 7}
    >>> f(1, 2, 3, x=4, y=5, *(6, 7), **UserDict(a=8, b=9))
    (1, 2, 3, 6, 7) {'a': 8, 'b': 9, 'x': 4, 'y': 5}

Examples with invalid arguments (TypeErrors). We're also testing the function
names in the exception messages.

Verify clearing of SF bug #733667

    >>> e(c=4)
    Traceback (most recent call last):
      ...
    TypeError: e() got an unexpected keyword argument 'c'

    >>> g()
    Traceback (most recent call last):
      ...
    TypeError: g() takes at least 1 argument (0 given)

    >>> g(*())
    Traceback (most recent call last):
      ...
    TypeError: g() takes at least 1 argument (0 given)

    >>> g(*(), **{})
    Traceback (most recent call last):
      ...
    TypeError: g() takes at least 1 argument (0 given)

    >>> g(1)
    1 () {}
    >>> g(1, 2)
    1 (2,) {}
    >>> g(1, 2, 3)
    1 (2, 3) {}
    >>> g(1, 2, 3, *(4, 5))
    1 (2, 3, 4, 5) {}

    >>> class Nothing: pass
    ...
    >>> g(*Nothing())
    Traceback (most recent call last):
      ...
    TypeError: g() argument after * must be a sequence, not instance

    >>> class Nothing:
    ...     def __len__(self): return 5
    ...

    >>> g(*Nothing())
    Traceback (most recent call last):
      ...
    TypeError: g() argument after * must be a sequence, not instance

    >>> class Nothing():
    ...     def __len__(self): return 5
    ...     def __getitem__(self, i):
    ...         if i<3: return i
    ...         else: raise IndexError(i)
    ...

    >>> g(*Nothing())
    0 (1, 2) {}

    >>> class Nothing:
    ...     def __init__(self): self.c = 0
    ...     def __iter__(self): return self
    ...     def next(self):
    ...         if self.c == 4:
    ...             raise StopIteration
    ...         c = self.c
    ...         self.c += 1
    ...         return c
    ...

    >>> g(*Nothing())
    0 (1, 2, 3) {}

Make sure that the function doesn't stomp the dictionary

    >>> d = {'a': 1, 'b': 2, 'c': 3}
    >>> d2 = d.copy()
    >>> g(1, d=4, **d)
    1 () {'a': 1, 'b': 2, 'c': 3, 'd': 4}
    >>> d == d2
    True

What about willful misconduct?

    >>> def saboteur(**kw):
    ...     kw['x'] = 'm'
    ...     return kw

    >>> d = {}
    >>> kw = saboteur(a=1, **d)
    >>> d
    {}


    >>> g(1, 2, 3, **{'x': 4, 'y': 5})
    Traceback (most recent call last):
      ...
    TypeError: g() got multiple values for keyword argument 'x'

    >>> f(**{1:2})
    Traceback (most recent call last):
      ...
    TypeError: f() keywords must be strings

    >>> h(**{'e': 2})
    Traceback (most recent call last):
      ...
    TypeError: h() got an unexpected keyword argument 'e'

    >>> h(*h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after * must be a sequence, not function

    >>> dir(*h)
    Traceback (most recent call last):
      ...
    TypeError: dir() argument after * must be a sequence, not function

    >>> None(*h)
    Traceback (most recent call last):
      ...
    TypeError: NoneType object argument after * must be a sequence, \
not function

    >>> h(**h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not function

    >>> dir(**h)
    Traceback (most recent call last):
      ...
    TypeError: dir() argument after ** must be a mapping, not function

    >>> None(**h)
    Traceback (most recent call last):
      ...
    TypeError: NoneType object argument after ** must be a mapping, \
not function

    >>> dir(b=1, **{'b': 1})
    Traceback (most recent call last):
      ...
    TypeError: dir() got multiple values for keyword argument 'b'

Another helper function

    >>> def f2(*a, **b):
    ...     return a, b


    >>> d = {}
    >>> for i in xrange(512):
    ...     key = 'k%d' % i
    ...     d[key] = i
    >>> a, b = f2(1, *(2,3), **d)
    >>> len(a), len(b), b == d
    (3, 512, True)

    >>> class Foo:
    ...     def method(self, arg1, arg2):
    ...         return arg1+arg2

    >>> x = Foo()
    >>> Foo.method(*(x, 1, 2))
    3
    >>> Foo.method(x, *(1, 2))
    3
    >>> Foo.method(*(1, 2, 3))
    Traceback (most recent call last):
      ...
    TypeError: unbound method method() must be called with Foo instance as \
first argument (got int instance instead)

    >>> Foo.method(1, *[2, 3])
    Traceback (most recent call last):
      ...
    TypeError: unbound method method() must be called with Foo instance as \
first argument (got int instance instead)

A PyCFunction that takes only positional parameters shoud allow an
empty keyword dictionary to pass without a complaint, but raise a
TypeError if te dictionary is not empty

    >>> try:
    ...     silence = id(1, *{})
    ...     True
    ... except:
    ...     False
    True

    >>> id(1, **{'foo': 1})
    Traceback (most recent call last):
      ...
    TypeError: id() takes no keyword arguments

A corner case of keyword dictionary items being deleted during
the function call setup. See <http://bugs.python.org/issue2016>.

    >>> class Name(str):
    ...     def __eq__(self, other):
    ...         try:
    ...              del x[self]
    ...         except KeyError:
    ...              pass
    ...         return str.__eq__(self, other)
    ...     def __hash__(self):
    ...         return str.__hash__(self)

    >>> x = {Name("a"):1, Name("b"):2}
    >>> def f(a, b):
    ...     print a,b
    >>> f(**x)
    1 2

A obscure message:

    >>> def f(a, b):
    ...    pass
    >>> f(b=1)
    Traceback (most recent call last):
      ...
    TypeError: f() takes exactly 2 arguments (1 given)

The number of arguments passed in includes keywords:

    >>> def f(a):
    ...    pass
    >>> f(6, a=4, *(1, 2, 3))
    Traceback (most recent call last):
      ...
    TypeError: f() takes exactly 1 argument (5 given)
"""

import unittest
import sys
from test import test_support


class ExtCallTest(unittest.TestCase):

    def test_unicode_keywords(self):
        def f(a):
            return a
        self.assertEqual(f(**{u'a': 4}), 4)
        self.assertRaises(TypeError, f, **{u'stören': 4})
        self.assertRaises(TypeError, f, **{u'someLongString':2})
        try:
            f(a=4, **{u'a': 4})
        except TypeError:
            pass
        else:
            self.fail("duplicate arguments didn't raise")


def test_main():
    test_support.run_doctest(sys.modules[__name__], True)
    test_support.run_unittest(ExtCallTest)

if __name__ == '__main__':
    test_main()
