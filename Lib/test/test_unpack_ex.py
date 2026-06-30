# Tests for extended unpacking, starred expressions.

import doctest
import unittest


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

Unpack nested implied tuple

    >>> [*[*a]] = [[7,8,9]]
    >>> a == [[7,8,9]]
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

    >>> class OnlyKeys:
    ...     def keys(self):
    ...         return ['key']
    >>> {**OnlyKeys()}
    Traceback (most recent call last):
      ...
    TypeError: 'OnlyKeys' object is not subscriptable

    >>> class BrokenKeys:
    ...     def keys(self):
    ...         return 1
    >>> {**BrokenKeys()}
    Traceback (most recent call last):
      ...
    TypeError: test.test_unpack_ex.BrokenKeys.keys() must return an iterable, not int

    >>> len(eval("{" + ", ".join("**{{{}: {}}}".format(i, i)
    ...                          for i in range(1000)) + "}"))
    1000

    >>> {0:1, **{0:2}, 0:3, 0:4}
    {0: 4}

Comprehension element unpacking

    >>> a, b, c = [0, 1, 2], 3, 4
    >>> [*a, b, c]
    [0, 1, 2, 3, 4]

    >>> l = [a, (3, 4), {5}, {6: None}, (i for i in range(7, 10))]
    >>> [*item for item in l]
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

    >>> [*[0, 1] for i in range(5)]
    [0, 1, 0, 1, 0, 1, 0, 1, 0, 1]

    >>> [*'a' for i in range(5)]
    ['a', 'a', 'a', 'a', 'a']

    >>> [*[] for i in range(10)]
    []

    >>> [*(x*2) for x in [[1, 2, 3], [], 'cat']]
    [1, 2, 3, 1, 2, 3, 'c', 'a', 't', 'c', 'a', 't']

    >>> {**{} for a in [1]}
    {}

    >>> {**{7: i} for i in range(10)}
    {7: 9}

    >>> dicts = [{1: 2}, {3: 4}, {5: 6, 7: 8}, {}, {9: 10}, {1: 0}]
    >>> {**d for d in dicts}
    {1: 0, 3: 4, 5: 6, 7: 8, 9: 10}

    >>> gen = (*(0, 1) for i in range(5))
    >>> next(gen)
    0
    >>> list(gen)
    [1, 0, 1, 0, 1, 0, 1, 0, 1]

Comprehension unpacking with conditionals and double loops

    >>> [*[i, i+1] for i in range(5) if i % 2 == 0]
    [0, 1, 2, 3, 4, 5]

    >>> [*y for x in [[[0], [1, 2, 3], [], [4, 5]], [[6, 7]]] for y in x]
    [0, 1, 2, 3, 4, 5, 6, 7]

    >>> [*y for x in [[[0], [1, 2, 3], [], [4, 5]], [[6, 7]]] for y in x if y and y[0]>0]
    [1, 2, 3, 4, 5, 6, 7]

    >>> dicts = [{1: 2}, {3: 4}, {5: 6, 7: 8}, {}, {9: 10}, {1: 0}]
    >>> {**d for d in dicts if len(d) != 2}
    {1: 0, 3: 4, 9: 10}

Scoping of assignment expressions in comprehensions

    >>> [*((y := i**2), 2*y) for i in range(4)]
    [0, 0, 1, 2, 4, 8, 9, 18]
    >>> y
    9

    >>> [*(y := [i, i+1, i+2]) for i in range(4)]
    [0, 1, 2, 1, 2, 3, 2, 3, 4, 3, 4, 5]
    >>> y
    [3, 4, 5]

    >>> g = (*(z := [i, i+1, i+2]) for i in range(4))
    >>> z
    Traceback (most recent call last):
    ...
    NameError: name 'z' is not defined
    >>> next(g)
    0
    >>> z
    [0, 1, 2]
    >>> next(g)
    1
    >>> z
    [0, 1, 2]
    >>> next(g)
    2
    >>> z
    [0, 1, 2]
    >>> next(g)
    1
    >>> z
    [1, 2, 3]

    >>> x = [1, 2, 3]
    >>> y = [4, 5, 6]
    >>> def f(*args):
    ...     print(args)

    >>> f(*x if x else y)
    (1, 2, 3)


Malformed comperehension element unpacking

    >>> [*x for x in [1, 2, 3]]
    Traceback (most recent call last):
    ...
    [*x for x in [1, 2, 3]]
     ^^
    TypeError: Value after * must be an iterable, not int


Error messages for specific failure modes of unpacking

    >>> [*x if x else y for x in z]
    Traceback (most recent call last):
    ...
    [*x if x else y for x in z]
     ^^^^^^^^^^^^^^
    SyntaxError: invalid starred expression. Did you forget to wrap the conditional expression in parentheses?

    >>> [*x if x else y]
    Traceback (most recent call last):
    ...
    [*x if x else y]
     ^^^^^^^^^^^^^^
    SyntaxError: invalid starred expression. Did you forget to wrap the conditional expression in parentheses?

    >>> [x if x else *y for x in z]
    Traceback (most recent call last):
    ...
    [x if x else *y for x in z]
                 ^
    SyntaxError: cannot unpack only part of a conditional expression

    >>> [x if x else *y]
    Traceback (most recent call last):
    ...
    [x if x else *y]
                 ^
    SyntaxError: cannot unpack only part of a conditional expression

    >>> {**x if x else y}
    Traceback (most recent call last):
    ...
    {**x if x else y}
     ^^^^^^^^^^^^^^^^
    SyntaxError: invalid double starred expression. Did you forget to wrap the conditional expression in parentheses?
    >>> {x if x else **y}
    Traceback (most recent call last):
    ...
    {x if x else **y}
                 ^^
    SyntaxError: cannot use dict unpacking on only part of a conditional expression

    >>> [**x for x in [{1: 2}]]
    Traceback (most recent call last):
    ...
    [**x for x in [{1: 2}]]
     ^^^
    SyntaxError: cannot use dict unpacking in list comprehension

    >>> (**x for x in [{1:2}])
    Traceback (most recent call last):
    ...
        (**x for x in [{1:2}])
         ^^^
    SyntaxError: cannot use dict unpacking in generator expression

    >>> dict(**x for x in [{1:2}])
    Traceback (most recent call last):
    ...
        dict(**x for x in [{1:2}])
             ^^^
    SyntaxError: cannot use dict unpacking in generator expression

    >>> {*a: b for a, b in {1: 2}.items()}
    Traceback (most recent call last):
    ...
    {*a: b for a, b in {1: 2}.items()}
     ^^
    SyntaxError: cannot use a starred expression in a dictionary key

    >>> {**a: b for a, b in {1: 2}.items()}
    Traceback (most recent call last):
    ...
    {**a: b for a, b in {1: 2}.items()}
     ^^^
    SyntaxError: cannot use dict unpacking in a dictionary key

    >>> {a: *b for a, b in {1: 2}.items()}
    Traceback (most recent call last):
    ...
    {a: *b for a, b in {1: 2}.items()}
        ^^
    SyntaxError: cannot use a starred expression in a dictionary value

    >>> {a: **b for a, b in {1: 2}.items()}
    Traceback (most recent call last):
    ...
    {a: **b for a, b in {1: 2}.items()}
        ^^^
    SyntaxError: cannot use dict unpacking in a dictionary value


# Generator expression in function arguments

    >>> list(*x for x in (range(5) for i in range(3)))
    [0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4]

    >>> def f(arg):
    ...     print(type(arg), list(arg), list(arg))
    >>> f(*x for x in [[1,2,3]])
    <class 'generator'> [1, 2, 3] []

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
    SyntaxError: multiple starred expressions in assignment

    >>> [*b, *c] = range(10) # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: multiple starred expressions in assignment

    >>> a,*b,*c,*d = range(4) # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: multiple starred expressions in assignment

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

    >>> (*x),y = 1, 2 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: cannot use starred expression here

    >>> (((*x))),y = 1, 2 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: cannot use starred expression here

    >>> z,(*x),y = 1, 2, 4 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: cannot use starred expression here

    >>> z,(*x) = 1, 2 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: cannot use starred expression here

    >>> ((*x),y) = 1, 2 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: cannot use starred expression here

Some size constraints (all fail.)

    >>> s = ", ".join("a%d" % i for i in range(1<<8)) + ", *rest = range((1<<8) + 1)"
    >>> compile(s, 'test', 'exec') # doctest:+ELLIPSIS
    Traceback (most recent call last):
     ...
    SyntaxError: too many expressions in star-unpacking assignment

    >>> s = ", ".join("a%d" % i for i in range((1<<8) + 1)) + ", *rest = range((1<<8) + 2)"
    >>> compile(s, 'test', 'exec') # doctest:+ELLIPSIS
    Traceback (most recent call last):
     ...
    SyntaxError: too many expressions in star-unpacking assignment

(there is an additional limit, on the number of expressions after the
'*rest', but it's 1<<24 and testing it takes too much memory.)

"""

def test_errors_in_iter():
    """
    >>> class A:
    ...     def __iter__(self):
    ...         raise exc
    ...
    >>> exc = ZeroDivisionError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> exc = AttributeError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> exc = TypeError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    TypeError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    TypeError: some error
    """

def test_errors_in_next():
    """
    >>> class I:
    ...     def __iter__(self):
    ...         return self
    ...     def __next__(self):
    ...         raise exc
    ...
    >>> class A:
    ...     def __iter__(self):
    ...         return I()
    ...

    >>> exc = ZeroDivisionError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> exc = AttributeError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> exc = TypeError('some error')
    >>> [*A()]
    Traceback (most recent call last):
      ...
    TypeError: some error

    >>> {*A()}
    Traceback (most recent call last):
      ...
    TypeError: some error
    """

def test_errors_in_keys():
    """
    >>> class D:
    ...     def keys(self):
    ...         raise exc
    ...
    >>> exc = ZeroDivisionError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> exc = AttributeError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> exc = KeyError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    KeyError: 'some error'
    """

def test_errors_in_keys_next():
    """
    >>> class I:
    ...     def __iter__(self):
    ...         return self
    ...     def __next__(self):
    ...         raise exc
    ...
    >>> class D:
    ...     def keys(self):
    ...         return I()
    ...
    >>> exc = ZeroDivisionError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> exc = AttributeError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> exc = KeyError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    KeyError: 'some error'
    """

def test_errors_in_getitem():
    """
    >>> class D:
    ...     def keys(self):
    ...         return ['key']
    ...     def __getitem__(self, key):
    ...         raise exc
    ...
    >>> exc = ZeroDivisionError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    ZeroDivisionError: some error

    >>> exc = AttributeError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    AttributeError: some error

    >>> exc = KeyError('some error')
    >>> {**D()}
    Traceback (most recent call last):
      ...
    KeyError: 'some error'
    """

__test__ = {'doctests' : doctests}


class TestGeneratorExpressionDelegation(unittest.TestCase):
    # Unpacking a sub-iterable with ``*`` in a generator expression delegates
    # to the sub-iterable using ``yield from`` semantics, so that values sent
    # to (and exceptions thrown into) the generator are forwarded.

    def test_flatten(self):
        lists = [[1, 2], [3], [], [4, 5]]
        self.assertEqual(list((*sub for sub in lists)), [1, 2, 3, 4, 5])

    def test_yields_from_multiple_iterables(self):
        gen = (*(0, 1) for i in range(3))
        self.assertEqual(list(gen), [0, 1, 0, 1, 0, 1])

    def test_send_is_forwarded(self):
        received = []

        def sub():
            while True:
                received.append((yield 'value'))

        gen = (*sub() for _ in range(1))
        self.assertEqual(next(gen), 'value')
        self.assertEqual(gen.send(42), 'value')
        self.assertEqual(gen.send(7), 'value')
        self.assertEqual(received, [42, 7])

    def test_throw_is_forwarded(self):
        caught = []

        def sub():
            try:
                yield 1
                yield 2
            except ValueError as exc:
                caught.append(str(exc))
                yield 'after-catch'

        gen = (*sub() for _ in range(1))
        self.assertEqual(next(gen), 1)
        self.assertEqual(gen.throw(ValueError('boom')), 'after-catch')
        self.assertEqual(caught, ['boom'])

    def test_close_is_forwarded(self):
        closed = []

        def sub():
            try:
                yield 1
                yield 2
            except GeneratorExit:
                closed.append(True)
                raise

        gen = (*sub() for _ in range(1))
        self.assertEqual(next(gen), 1)
        gen.close()
        self.assertEqual(closed, [True])

    def test_subiterator_return_value_is_discarded(self):
        def sub(n):
            yield n
            return 'ignored'

        self.assertEqual(list((*sub(i) for i in range(3))), [0, 1, 2])


class TestAsyncGeneratorExpressionDelegation(unittest.TestCase):
    # Unpacking a (synchronous) sub-iterable with ``*`` in an asynchronous
    # generator expression also delegates with ``yield from`` semantics:
    # asend() forwards to the sub-iterator's send(), athrow() to throw() and
    # aclose() to close().
    #
    # These are low-level language tests, so (like test_asyncgen) the async
    # generators are driven by hand rather than through an event loop.

    @staticmethod
    async def _aiter(seq):
        for item in seq:
            yield item

    @staticmethod
    def _run(coro):
        # Drive a coroutine that is not expected to await anything real, and
        # return its result.  Any StopAsyncIteration (e.g. an exhausted
        # asend()) is allowed to propagate.
        try:
            coro.send(None)
        except StopIteration as exc:
            return exc.value
        coro.close()
        raise AssertionError("coroutine awaited unexpectedly")

    def _collect(self, agen):
        result = []
        while True:
            try:
                result.append(self._run(agen.asend(None)))
            except StopAsyncIteration:
                break
        return result

    def test_flatten(self):
        lists = [[1, 2], [3], [], [4, 5]]
        agen = (*sub async for sub in self._aiter(lists))
        self.assertEqual(self._collect(agen), [1, 2, 3, 4, 5])

    def test_asend_forwards_to_send(self):
        received = []

        def sub():
            while True:
                received.append((yield 'value'))

        agen = (*sub() async for _ in self._aiter([0]))
        self.assertEqual(self._run(agen.asend(None)), 'value')
        self.assertEqual(self._run(agen.asend(99)), 'value')
        self.assertEqual(self._run(agen.asend(123)), 'value')
        self.assertEqual(received, [99, 123])

    def test_athrow_forwards_to_throw(self):
        caught = []

        def sub():
            try:
                yield 1
                yield 2
            except ValueError as exc:
                caught.append(str(exc))
                yield 'after-catch'

        agen = (*sub() async for _ in self._aiter([0]))
        self.assertEqual(self._run(agen.asend(None)), 1)
        self.assertEqual(self._run(agen.athrow(ValueError('boom'))),
                         'after-catch')
        self.assertEqual(caught, ['boom'])

    def test_aclose_forwards_to_close(self):
        closed = []

        def sub():
            try:
                yield 1
                yield 2
            except GeneratorExit:
                closed.append(True)
                raise

        agen = (*sub() async for _ in self._aiter([0]))
        self.assertEqual(self._run(agen.asend(None)), 1)
        self._run(agen.aclose())
        self.assertEqual(closed, [True])

    def test_unpack_helper_throw_requires_argument(self):
        # The internal sync-iterable wrapper is reachable via ag_await while
        # suspended at the delegation; throw() must validate its arity rather
        # than crash (gh-143055).
        agen = (*[1, 2, 3] async for _ in self._aiter([0]))
        self._run(agen.asend(None))
        wrapper = agen.ag_await
        self.assertIsNotNone(wrapper)
        with self.assertRaises(TypeError):
            wrapper.throw()
        # A valid exception is still accepted and propagates out.
        with self.assertRaises(ValueError):
            wrapper.throw(ValueError('boom'))

    def test_unpacking_async_iterable_is_a_type_error(self):
        # ``*`` unpacking is synchronous; async iterables cannot be unpacked.
        async def agen_fn():
            yield 1

        agen = (*agen_fn() async for _ in self._aiter([0]))
        with self.assertRaises(TypeError):
            self._run(agen.asend(None))


def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
