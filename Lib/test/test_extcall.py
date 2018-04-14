
"""Doctest for method/function calls.

We're going the use these types for extra testing

    >>> from collections import UserList
    >>> from collections import UserDict

We're defining four helper functions

    >>> def e(a,b):
    ...     print(a, b)

    >>> def f(*a, **k):
    ...     print(a, support.sortdict(k))

    >>> def g(x, *y, **z):
    ...     print(x, y, support.sortdict(z))

    >>> def h(j=1, a=2, h=3):
    ...     print(j, a, h)

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
    >>> f(*[1, 2, 3], 4, 5)
    (1, 2, 3, 4, 5) {}
    >>> f(1, 2, 3, *UserList([4, 5]))
    (1, 2, 3, 4, 5) {}
    >>> f(1, 2, 3, *[4, 5], *[6, 7])
    (1, 2, 3, 4, 5, 6, 7) {}
    >>> f(1, *[2, 3], 4, *[5, 6], 7)
    (1, 2, 3, 4, 5, 6, 7) {}
    >>> f(*UserList([1, 2]), *UserList([3, 4]), 5, *UserList([6, 7]))
    (1, 2, 3, 4, 5, 6, 7) {}

Here we add keyword arguments

    >>> f(1, 2, 3, **{'a':4, 'b':5})
    (1, 2, 3) {'a': 4, 'b': 5}
    >>> f(1, 2, **{'a': -1, 'b': 5}, **{'a': 4, 'c': 6})
    Traceback (most recent call last):
        ...
    TypeError: f() got multiple values for keyword argument 'a'
    >>> f(1, 2, **{'a': -1, 'b': 5}, a=4, c=6)
    Traceback (most recent call last):
        ...
    TypeError: f() got multiple values for keyword argument 'a'
    >>> f(1, 2, a=3, **{'a': 4}, **{'a': 5})
    Traceback (most recent call last):
        ...
    TypeError: f() got multiple values for keyword argument 'a'
    >>> f(1, 2, 3, *[4, 5], **{'a':6, 'b':7})
    (1, 2, 3, 4, 5) {'a': 6, 'b': 7}
    >>> f(1, 2, 3, x=4, y=5, *(6, 7), **{'a':8, 'b': 9})
    (1, 2, 3, 6, 7) {'a': 8, 'b': 9, 'x': 4, 'y': 5}
    >>> f(1, 2, 3, *[4, 5], **{'c': 8}, **{'a':6, 'b':7})
    (1, 2, 3, 4, 5) {'a': 6, 'b': 7, 'c': 8}
    >>> f(1, 2, 3, *(4, 5), x=6, y=7, **{'a':8, 'b': 9})
    (1, 2, 3, 4, 5) {'a': 8, 'b': 9, 'x': 6, 'y': 7}

    >>> f(1, 2, 3, **UserDict(a=4, b=5))
    (1, 2, 3) {'a': 4, 'b': 5}
    >>> f(1, 2, 3, *(4, 5), **UserDict(a=6, b=7))
    (1, 2, 3, 4, 5) {'a': 6, 'b': 7}
    >>> f(1, 2, 3, x=4, y=5, *(6, 7), **UserDict(a=8, b=9))
    (1, 2, 3, 6, 7) {'a': 8, 'b': 9, 'x': 4, 'y': 5}
    >>> f(1, 2, 3, *(4, 5), x=6, y=7, **UserDict(a=8, b=9))
    (1, 2, 3, 4, 5) {'a': 8, 'b': 9, 'x': 6, 'y': 7}

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
    TypeError: g() missing 1 required positional argument: 'x'

    >>> g(*())
    Traceback (most recent call last):
      ...
    TypeError: g() missing 1 required positional argument: 'x'

    >>> g(*(), **{})
    Traceback (most recent call last):
      ...
    TypeError: g() missing 1 required positional argument: 'x'

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
    TypeError: g() argument after * must be an iterable, not Nothing

    >>> class Nothing:
    ...     def __len__(self): return 5
    ...

    >>> g(*Nothing())
    Traceback (most recent call last):
      ...
    TypeError: g() argument after * must be an iterable, not Nothing

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
    ...     def __next__(self):
    ...         if self.c == 4:
    ...             raise StopIteration
    ...         c = self.c
    ...         self.c += 1
    ...         return c
    ...

    >>> g(*Nothing())
    0 (1, 2, 3) {}

Check for issue #4806: Does a TypeError in a generator get propagated with the
right error message? (Also check with other iterables.)

    >>> def broken(): raise TypeError("myerror")
    ...

    >>> g(*(broken() for i in range(1)))
    Traceback (most recent call last):
      ...
    TypeError: myerror
    >>> g(*range(1), *(broken() for i in range(1)))
    Traceback (most recent call last):
      ...
    TypeError: myerror

    >>> class BrokenIterable1:
    ...     def __iter__(self):
    ...         raise TypeError('myerror')
    ...
    >>> g(*BrokenIterable1())
    Traceback (most recent call last):
      ...
    TypeError: myerror
    >>> g(*range(1), *BrokenIterable1())
    Traceback (most recent call last):
      ...
    TypeError: myerror

    >>> class BrokenIterable2:
    ...     def __iter__(self):
    ...         yield 0
    ...         raise TypeError('myerror')
    ...
    >>> g(*BrokenIterable2())
    Traceback (most recent call last):
      ...
    TypeError: myerror
    >>> g(*range(1), *BrokenIterable2())
    Traceback (most recent call last):
      ...
    TypeError: myerror

    >>> class BrokenSequence:
    ...     def __getitem__(self, idx):
    ...         raise TypeError('myerror')
    ...
    >>> g(*BrokenSequence())
    Traceback (most recent call last):
      ...
    TypeError: myerror
    >>> g(*range(1), *BrokenSequence())
    Traceback (most recent call last):
      ...
    TypeError: myerror

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
    TypeError: g() got multiple values for argument 'x'

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
    TypeError: h() argument after * must be an iterable, not function

    >>> h(1, *h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after * must be an iterable, not function

    >>> h(*[1], *h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after * must be an iterable, not function

    >>> dir(*h)
    Traceback (most recent call last):
      ...
    TypeError: dir() argument after * must be an iterable, not function

    >>> None(*h)
    Traceback (most recent call last):
      ...
    TypeError: NoneType object argument after * must be an iterable, \
not function

    >>> h(**h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not function

    >>> h(**[])
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not list

    >>> h(a=1, **h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not function

    >>> h(a=1, **[])
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not list

    >>> h(**{'a': 1}, **h)
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not function

    >>> h(**{'a': 1}, **[])
    Traceback (most recent call last):
      ...
    TypeError: h() argument after ** must be a mapping, not list

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
    >>> for i in range(512):
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
    5
    >>> Foo.method(1, *[2, 3])
    5

A PyCFunction that takes only positional parameters should allow an
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
    ...     print(a,b)
    >>> f(**x)
    1 2

Too many arguments:

    >>> def f(): pass
    >>> f(1)
    Traceback (most recent call last):
      ...
    TypeError: f() takes 0 positional arguments but 1 was given
    >>> def f(a): pass
    >>> f(1, 2)
    Traceback (most recent call last):
      ...
    TypeError: f() takes 1 positional argument but 2 were given
    >>> def f(a, b=1): pass
    >>> f(1, 2, 3)
    Traceback (most recent call last):
      ...
    TypeError: f() takes from 1 to 2 positional arguments but 3 were given
    >>> def f(*, kw): pass
    >>> f(1, kw=3)
    Traceback (most recent call last):
      ...
    TypeError: f() takes 0 positional arguments but 1 positional argument (and 1 keyword-only argument) were given
    >>> def f(*, kw, b): pass
    >>> f(1, 2, 3, b=3, kw=3)
    Traceback (most recent call last):
      ...
    TypeError: f() takes 0 positional arguments but 3 positional arguments (and 2 keyword-only arguments) were given
    >>> def f(a, b=2, *, kw): pass
    >>> f(2, 3, 4, kw=4)
    Traceback (most recent call last):
      ...
    TypeError: f() takes from 1 to 2 positional arguments but 3 positional arguments (and 1 keyword-only argument) were given

Too few and missing arguments:

    >>> def f(a): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 1 required positional argument: 'a'
    >>> def f(a, b): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 2 required positional arguments: 'a' and 'b'
    >>> def f(a, b, c): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 3 required positional arguments: 'a', 'b', and 'c'
    >>> def f(a, b, c, d, e): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 5 required positional arguments: 'a', 'b', 'c', 'd', and 'e'
    >>> def f(a, b=4, c=5, d=5): pass
    >>> f(c=12, b=9)
    Traceback (most recent call last):
      ...
    TypeError: f() missing 1 required positional argument: 'a'

Same with keyword only args:

    >>> def f(*, w): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 1 required keyword-only argument: 'w'
    >>> def f(*, a, b, c, d, e): pass
    >>> f()
    Traceback (most recent call last):
      ...
    TypeError: f() missing 5 required keyword-only arguments: 'a', 'b', 'c', 'd', and 'e'

"""

import sys
from test import support

def test_main():
    support.run_doctest(sys.modules[__name__], True)

if __name__ == '__main__':
    test_main()
