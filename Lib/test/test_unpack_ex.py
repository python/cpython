# Tests for extended unpacking, starred expressions.

doctests = """

Unpack tuple

    >>> t = (1, 2, 3)
    >>> a, *b, c = t
    >>> a == 1 and b == [2] and c == 3
    True

Unpack list

    >>> l = [4, 5, 6]
    >>> a, *b = l
    >>> a == 4 and b == [5, 6]
    True

Unpack implied tuple

    >>> *a, = 7, 8, 9
    >>> a == [7, 8, 9]
    True

Unpack string... fun!

    >>> a, *b = 'one'
    >>> a == 'o' and b == ['n', 'e']
    True

Unpack long sequence

    >>> a, b, c, *d, e, f, g = range(10)
    >>> (a, b, c, d, e, f, g) == (0, 1, 2, [3, 4, 5, 6], 7, 8, 9)
    True

Unpack short sequence

    >>> a, *b, c = (1, 2)
    >>> a == 1 and c == 2 and b == []
    True

Unpack generic sequence

    >>> class Seq:
    ...     def __getitem__(self, i):
    ...         if i >= 0 and i < 3: return i
    ...         raise IndexError
    ...
    >>> a, *b = Seq()
    >>> a == 0 and b == [1, 2]
    True

Unpack in for statement

    >>> for a, *b, c in [(1,2,3), (4,5,6,7)]:
    ...     print(a, b, c)
    ...
    1 [2] 3
    4 [5, 6] 7

Unpack in list

    >>> [a, *b, c] = range(5)
    >>> a == 0 and b == [1, 2, 3] and c == 4
    True

Multiple targets

    >>> a, *b, c = *d, e = range(5)
    >>> a == 0 and b == [1, 2, 3] and c == 4 and d == [0, 1, 2, 3] and e == 4
    True

Assignment unpacking

    >>> a, b, *c = range(5)
    >>> a, b, c
    (0, 1, [2, 3, 4])
    >>> *a, b, c = a, b, *c
    >>> a, b, c
    ([0, 1, 2], 3, 4)

Set display element unpacking

    >>> a = [1, 2, 3]
    >>> sorted({1, *a, 0, 4})
    [0, 1, 2, 3, 4]

    >>> {1, *1, 0, 4}
    Traceback (most recent call last):
      ...
    TypeError: 'int' object is not iterable

Dict display element unpacking

    >>> kwds = {'z': 0, 'w': 12}
    >>> sorted({'x': 1, 'y': 2, **kwds}.items())
    [('w', 12), ('x', 1), ('y', 2), ('z', 0)]

    >>> sorted({**{'x': 1}, 'y': 2, **{'z': 3}}.items())
    [('x', 1), ('y', 2), ('z', 3)]

    >>> sorted({**{'x': 1}, 'y': 2, **{'x': 3}}.items())
    [('x', 3), ('y', 2)]

    >>> sorted({**{'x': 1}, **{'x': 3}, 'x': 4}.items())
    [('x', 4)]

    >>> {**{}}
    {}

    >>> a = {}
    >>> {**a}[0] = 1
    >>> a
    {}

    >>> {**1}
    Traceback (most recent call last):
    ...
    TypeError: 'int' object is not a mapping

    >>> {**[]}
    Traceback (most recent call last):
    ...
    TypeError: 'list' object is not a mapping

    >>> len(eval("{" + ", ".join("**{{{}: {}}}".format(i, i)
    ...                          for i in range(1000)) + "}"))
    1000

    >>> {0:1, **{0:2}, 0:3, 0:4}
    {0: 4}

List comprehension element unpacking

    >>> a, b, c = [0, 1, 2], 3, 4
    >>> [*a, b, c]
    [0, 1, 2, 3, 4]

    >>> l = [a, (3, 4), {5}, {6: None}, (i for i in range(7, 10))]
    >>> [*item for item in l]
    Traceback (most recent call last):
    ...
    SyntaxError: iterable unpacking cannot be used in comprehension

    >>> [*[0, 1] for i in range(10)]
    Traceback (most recent call last):
    ...
    SyntaxError: iterable unpacking cannot be used in comprehension

    >>> [*'a' for i in range(10)]
    Traceback (most recent call last):
    ...
    SyntaxError: iterable unpacking cannot be used in comprehension

    >>> [*[] for i in range(10)]
    Traceback (most recent call last):
    ...
    SyntaxError: iterable unpacking cannot be used in comprehension

Generator expression in function arguments

    >>> list(*x for x in (range(5) for i in range(3)))
    Traceback (most recent call last):
    ...
        list(*x for x in (range(5) for i in range(3)))
                  ^
    SyntaxError: invalid syntax

    >>> dict(**x for x in [{1:2}])
    Traceback (most recent call last):
    ...
        dict(**x for x in [{1:2}])
                   ^
    SyntaxError: invalid syntax

Iterable argument unpacking

    >>> print(*[1], *[2], 3)
    1 2 3

Make sure that they don't corrupt the passed-in dicts.

    >>> def f(x, y):
    ...     print(x, y)
    ...
    >>> original_dict = {'x': 1}
    >>> f(**original_dict, y=2)
    1 2
    >>> original_dict
    {'x': 1}

Now for some failures

Make sure the raised errors are right for keyword argument unpackings

    >>> from collections.abc import MutableMapping
    >>> class CrazyDict(MutableMapping):
    ...     def __init__(self):
    ...         self.d = {}
    ...
    ...     def __iter__(self):
    ...         for x in self.d.__iter__():
    ...             if x == 'c':
    ...                 self.d['z'] = 10
    ...             yield x
    ...
    ...     def __getitem__(self, k):
    ...         return self.d[k]
    ...
    ...     def __len__(self):
    ...         return len(self.d)
    ...
    ...     def __setitem__(self, k, v):
    ...         self.d[k] = v
    ...
    ...     def __delitem__(self, k):
    ...         del self.d[k]
    ...
    >>> d = CrazyDict()
    >>> d.d = {chr(ord('a') + x): x for x in range(5)}
    >>> e = {**d}
    Traceback (most recent call last):
    ...
    RuntimeError: dictionary changed size during iteration

    >>> d.d = {chr(ord('a') + x): x for x in range(5)}
    >>> def f(**kwargs): print(kwargs)
    >>> f(**d)
    Traceback (most recent call last):
    ...
    RuntimeError: dictionary changed size during iteration

Overridden parameters

    >>> f(x=5, **{'x': 3}, y=2)
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.f() got multiple values for keyword argument 'x'

    >>> f(**{'x': 3}, x=5, y=2)
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.f() got multiple values for keyword argument 'x'

    >>> f(**{'x': 3}, **{'x': 5}, y=2)
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.f() got multiple values for keyword argument 'x'

    >>> f(x=5, **{'x': 3}, **{'x': 2})
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.f() got multiple values for keyword argument 'x'

    >>> f(**{1: 3}, **{1: 5})
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.f() got multiple values for keyword argument '1'

Unpacking non-sequence

    >>> a, *b = 7
    Traceback (most recent call last):
      ...
    TypeError: cannot unpack non-iterable int object

Unpacking sequence too short

    >>> a, *b, c, d, e = Seq()
    Traceback (most recent call last):
      ...
    ValueError: not enough values to unpack (expected at least 4, got 3)

Unpacking sequence too short and target appears last

    >>> a, b, c, d, *e = Seq()
    Traceback (most recent call last):
      ...
    ValueError: not enough values to unpack (expected at least 4, got 3)

Unpacking a sequence where the test for too long raises a different kind of
error

    >>> class BozoError(Exception):
    ...     pass
    ...
    >>> class BadSeq:
    ...     def __getitem__(self, i):
    ...         if i >= 0 and i < 3:
    ...             return i
    ...         elif i == 3:
    ...             raise BozoError
    ...         else:
    ...             raise IndexError
    ...

Trigger code while not expecting an IndexError (unpack sequence too long, wrong
error)

    >>> a, *b, c, d, e = BadSeq()
    Traceback (most recent call last):
      ...
    test.test_unpack_ex.BozoError

Now some general starred expressions (all fail).

    >>> a, *b, c, *d, e = range(10) # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: two starred expressions in assignment

    >>> [*b, *c] = range(10) # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: two starred expressions in assignment

    >>> *a = range(10) # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: starred assignment target must be in a list or tuple

    >>> *a # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: can't use starred expression here

    >>> *1 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: can't use starred expression here

    >>> x = *a # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: can't use starred expression here

Some size constraints (all fail.)

    >>> s = ", ".join("a%d" % i for i in range(1<<8)) + ", *rest = range(1<<8 + 1)"
    >>> compile(s, 'test', 'exec') # doctest:+ELLIPSIS
    Traceback (most recent call last):
     ...
    SyntaxError: too many expressions in star-unpacking assignment

    >>> s = ", ".join("a%d" % i for i in range(1<<8 + 1)) + ", *rest = range(1<<8 + 2)"
    >>> compile(s, 'test', 'exec') # doctest:+ELLIPSIS
    Traceback (most recent call last):
     ...
    SyntaxError: too many expressions in star-unpacking assignment

(there is an additional limit, on the number of expressions after the
'*rest', but it's 1<<24 and testing it takes too much memory.)

"""

__test__ = {'doctests' : doctests}

def test_main(verbose=False):
    from test import support
    from test import test_unpack_ex
    support.run_doctest(test_unpack_ex, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
