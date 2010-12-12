doctests = """
########### Tests borrowed from or inspired by test_genexps.py ############

Test simple loop with conditional

    >>> sum([i*i for i in range(100) if i&1 == 1])
    166650

Test simple nesting

    >>> [(i,j) for i in range(3) for j in range(4)]
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]

Test nesting with the inner expression dependent on the outer

    >>> [(i,j) for i in range(4) for j in range(i)]
    [(1, 0), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2)]

Make sure the induction variable is not exposed

    >>> i = 20
    >>> sum([i*i for i in range(100)])
    328350

    >>> i
    20

Verify that syntax error's are raised for listcomps used as lvalues

    >>> [y for y in (1,2)] = 10          # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...

    >>> [y for y in (1,2)] += 10         # doctest: +IGNORE_EXCEPTION_DETAIL
    Traceback (most recent call last):
       ...
    SyntaxError: ...


########### Tests borrowed from or inspired by test_generators.py ############

Make a nested list comprehension that acts like range()

    >>> def frange(n):
    ...     return [i for i in range(n)]
    >>> frange(10)
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Same again, only as a lambda expression instead of a function definition

    >>> lrange = lambda n:  [i for i in range(n)]
    >>> lrange(10)
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Generators can call other generators:

    >>> def grange(n):
    ...     for x in [i for i in range(n)]:
    ...         yield x
    >>> list(grange(5))
    [0, 1, 2, 3, 4]


Make sure that None is a valid return value

    >>> [None for i in range(10)]
    [None, None, None, None, None, None, None, None, None, None]

########### Tests for various scoping corner cases ############

Return lambdas that use the iteration variable as a default argument

    >>> items = [(lambda i=i: i) for i in range(5)]
    >>> [x() for x in items]
    [0, 1, 2, 3, 4]

Same again, only this time as a closure variable

    >>> items = [(lambda: i) for i in range(5)]
    >>> [x() for x in items]
    [4, 4, 4, 4, 4]

Another way to test that the iteration variable is local to the list comp

    >>> items = [(lambda: i) for i in range(5)]
    >>> i = 20
    >>> [x() for x in items]
    [4, 4, 4, 4, 4]

And confirm that a closure can jump over the list comp scope

    >>> items = [(lambda: y) for i in range(5)]
    >>> y = 2
    >>> [x() for x in items]
    [2, 2, 2, 2, 2]

We also repeat each of the above scoping tests inside a function

    >>> def test_func():
    ...     items = [(lambda i=i: i) for i in range(5)]
    ...     return [x() for x in items]
    >>> test_func()
    [0, 1, 2, 3, 4]

    >>> def test_func():
    ...     items = [(lambda: i) for i in range(5)]
    ...     return [x() for x in items]
    >>> test_func()
    [4, 4, 4, 4, 4]

    >>> def test_func():
    ...     items = [(lambda: i) for i in range(5)]
    ...     i = 20
    ...     return [x() for x in items]
    >>> test_func()
    [4, 4, 4, 4, 4]

    >>> def test_func():
    ...     items = [(lambda: y) for i in range(5)]
    ...     y = 2
    ...     return [x() for x in items]
    >>> test_func()
    [2, 2, 2, 2, 2]

"""


__test__ = {'doctests' : doctests}

def test_main(verbose=None):
    import sys
    from test import support
    from test import test_listcomps
    support.run_doctest(test_listcomps, verbose)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in range(len(counts)):
            support.run_doctest(test_genexps, verbose)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print(counts)

if __name__ == "__main__":
    test_main(verbose=True)
