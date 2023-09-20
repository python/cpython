import doctest
import unittest


doctests = """

Unpack tuple

    >>> t = (1, 2, 3)
    >>> a, b, c = t
    >>> a == 1 and b == 2 and c == 3
    True

Unpack list

    >>> l = [4, 5, 6]
    >>> a, b, c = l
    >>> a == 4 and b == 5 and c == 6
    True

Unpack implied tuple

    >>> a, b, c = 7, 8, 9
    >>> a == 7 and b == 8 and c == 9
    True

Unpack string... fun!

    >>> a, b, c = 'one'
    >>> a == 'o' and b == 'n' and c == 'e'
    True

Unpack generic sequence

    >>> class Seq:
    ...     def __getitem__(self, i):
    ...         if i >= 0 and i < 3: return i
    ...         raise IndexError
    ...
    >>> a, b, c = Seq()
    >>> a == 0 and b == 1 and c == 2
    True

Single element unpacking, with extra syntax

    >>> st = (99,)
    >>> sl = [100]
    >>> a, = st
    >>> a
    99
    >>> b, = sl
    >>> b
    100

Now for some failures

Unpacking non-sequence

    >>> a, b, c = 7
    Traceback (most recent call last):
      ...
    TypeError: cannot unpack non-iterable int object

Unpacking tuple of wrong size

    >>> a, b = t
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2)

Unpacking tuple of wrong size

    >>> a, b = l
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2)

Unpacking sequence too short

    >>> a, b, c, d = Seq()
    Traceback (most recent call last):
      ...
    ValueError: not enough values to unpack (expected 4, got 3)

Unpacking sequence too long

    >>> a, b = Seq()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2)

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

    >>> a, b, c, d, e = BadSeq()
    Traceback (most recent call last):
      ...
    test.test_unpack.BozoError

Trigger code while expecting an IndexError (unpack sequence too short, wrong
error)

    >>> a, b, c = BadSeq()
    Traceback (most recent call last):
      ...
    test.test_unpack.BozoError

Allow unpacking empty iterables

    >>> () = []
    >>> [] = ()
    >>> [] = []
    >>> () = ()

Unpacking non-iterables should raise TypeError

    >>> () = 42
    Traceback (most recent call last):
      ...
    TypeError: cannot unpack non-iterable int object

Unpacking to an empty iterable should raise ValueError

    >>> () = [42]
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 0)

"""

__test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


class TestCornerCases(unittest.TestCase):
    def test_extended_oparg_not_ignored(self):
        # https://github.com/python/cpython/issues/91625
        target = "(" + "y,"*400 + ")"
        code = f"""def unpack_400(x):
            {target} = x
            return y
        """
        ns = {}
        exec(code, ns)
        unpack_400 = ns["unpack_400"]
        # Warm up the function for quickening (PEP 659)
        for _ in range(30):
            y = unpack_400(range(400))
            self.assertEqual(y, 399)

if __name__ == "__main__":
    unittest.main()
