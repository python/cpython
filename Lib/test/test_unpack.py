import doctest
import textwrap
import traceback
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

Unpack dict

    >>> d = {4: 'four', 5: 'five', 6: 'six'}
    >>> a, b, c = d
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
    ValueError: too many values to unpack (expected 2, got 3)

Unpacking tuple of wrong size

    >>> a, b = l
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 2, got 3)

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
    ValueError: too many values to unpack (expected 0, got 1)

Unpacking to an empty iterable should raise ValueError, but it won't consume the
iterable if it doesn't have a pre-determined length

    >>> it = iter(range(100))
    >>> x, y, z = it
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3)
    >>> next(it)
    4

Unpacking unbalanced dict

    >>> d = {4: 'four', 5: 'five', 6: 'six', 7: 'seven'}
    >>> a, b, c = d
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3, got 4)

Ensure that `__len__()` is respected when showing the error message

    >>> class LengthTooLong:
    ...     def __len__(self):
    ...         return 5
    ...     def __getitem__(self, i):
    ...         return i*2
    ...
    >>> x, y, z = LengthTooLong()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3, got 5)

If `__len__()` is too low, fallback to not showing the got count

    >>> class BadLength:
    ...     def __len__(self):
    ...         return 1
    ...     def __getitem__(self, i):
    ...         return i*2
    ...
    >>> x, y, z = BadLength()
    Traceback (most recent call last):
      ...
    ValueError: too many values to unpack (expected 3)
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

    def test_exception_context_when_len_fails(self):
        """When `__len__()` raises an Exception, preserve exception context"""
        code = textwrap.dedent(
            f"""
            class CustomSeq:
                def __len__(self):
                    raise RuntimeError

                def __getitem__(self, i):
                    return i*2

            x, y, z = CustomSeq()
            """
        )
        with self.assertRaises(ValueError) as cm:
            exec(code)

        traceback_text = ''.join(traceback.format_exception(cm.exception))
        self.assertIn(
            "RuntimeError\n\n"
            "During handling of the above exception, another exception occurred:\n\n"
            "ValueError: too many values to unpack (expected 3)\n",
            traceback_text,
        )

    def test_baseexception_propagation_when_len_fails(self):
        """if `__len__()` raises a `BaseException`, propagate that as-is"""
        code = textwrap.dedent(
            """
            class C:
                def __len__(self):
                    raise KeyboardInterrupt
                def __getitem__(self, i):
                    return i

            x, y, z = C()
            """
        )
        with self.assertRaises(ValueError) as cm:
            exec(code)

        with self.assertRaises(KeyboardInterrupt):
            traceback.format_exception(cm.exception)

if __name__ == "__main__":
    unittest.main()
