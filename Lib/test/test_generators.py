simple_tests = """
Let's try a simple generator:

    >>> def f():
    ...    yield 1
    ...    yield 2

    >>> g = f()
    >>> g.next()
    1
    >>> g.next()
    2
    >>> g.next()
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
      File "<stdin>", line 2, in g
    StopIteration

"return" stops the generator:

    >>> def f():
    ...     yield 1
    ...     return
    ...     yield 2 # never reached
    ...
    >>> g = f()
    >>> g.next()
    1
    >>> g.next()
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
      File "<stdin>", line 3, in f
    StopIteration
    >>> g.next() # once stopped, can't be resumed
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
    StopIteration

"raise StopIteration" stops the generator too:

    >>> def f():
    ...     yield 1
    ...     return
    ...     yield 2 # never reached
    ...
    >>> g = f()
    >>> g.next()
    1
    >>> g.next()
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
    StopIteration
    >>> g.next()
    Traceback (most recent call last):
      File "<stdin>", line 1, in ?
    StopIteration

However, they are not exactly equivalent:

    >>> def g1():
    ...     try:
    ...         return
    ...     except:
    ...         yield 1
    ...
    >>> list(g1())
    []

    >>> def g2():
    ...     try:
    ...         raise StopIteration
    ...     except:
    ...         yield 42
    >>> print list(g2())
    [42]

This may be surprising at first:

    >>> def g3():
    ...     try:
    ...         return
    ...     finally:
    ...         yield 1
    ...
    >>> list(g3())
    [1]

Let's create an alternate range() function implemented as a generator:

    >>> def yrange(n):
    ...     for i in range(n):
    ...         yield i
    ...
    >>> list(yrange(5))
    [0, 1, 2, 3, 4]

Generators always return to the most recent caller:

    >>> def creator():
    ...     r = yrange(5)
    ...     print "creator", r.next()
    ...     return r
    ...
    >>> def caller():
    ...     r = creator()
    ...     for i in r:
    ...             print "caller", i
    ...
    >>> caller()
    creator 0
    caller 1
    caller 2
    caller 3
    caller 4

Generators can call other generators:

    >>> def zrange(n):
    ...     for i in yrange(n):
    ...         yield i
    ...
    >>> list(zrange(5))
    [0, 1, 2, 3, 4]

"""

__test__ = {"simple": simple_tests}

# Magic test name that regrtest.py invokes *after* importing this module.
# This worms around a bootstrap problem.
# Note that doctest and regrtest both look in sys.argv for a "-v" argument,
# so this works as expected in both ways of running regrtest.
def test_main():
    import doctest, test_generators
    doctest.testmod(test_generators)

# This part isn't needed for regrtest, but for running the test directly.
if __name__ == "__main__":
    test_main()
