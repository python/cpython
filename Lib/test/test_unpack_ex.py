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

Now for some failures

Unpacking non-sequence

    >>> a, *b = 7
    Traceback (most recent call last):
      ...
    TypeError: 'int' object is not iterable

Unpacking sequence too short

    >>> a, *b, c, d, e = Seq()
    Traceback (most recent call last):
      ...
    ValueError: need more than 3 values to unpack

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
    SyntaxError: can use starred expression only as assignment target

    >>> *1 # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: can use starred expression only as assignment target

    >>> x = *a # doctest:+ELLIPSIS
    Traceback (most recent call last):
      ...
    SyntaxError: can use starred expression only as assignment target

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
    import sys
    from test import support
    from test import test_unpack_ex
    support.run_doctest(test_unpack_ex, verbose)

if __name__ == "__main__":
    test_main(verbose=True)
