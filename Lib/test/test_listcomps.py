import doctest
import unittest


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

Test the idiom for temporary variable assignment in comprehensions.

    >>> [j*j for i in range(4) for j in [i+1]]
    [1, 4, 9, 16]
    >>> [j*k for i in range(4) for j in [i+1] for k in [j+1]]
    [2, 6, 12, 20]
    >>> [j*k for i in range(4) for j, k in [(i+1, i+2)]]
    [2, 6, 12, 20]

Not assignment

    >>> [i*i for i in [*range(4)]]
    [0, 1, 4, 9]
    >>> [i*i for i in (*range(4),)]
    [0, 1, 4, 9]

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

A comprehension's iteration var, if in a cell, doesn't stomp on a previous value:

    >>> def test_func():
    ...     y = 10
    ...     items = [(lambda: y) for y in range(5)]
    ...     x = y
    ...     y = 20
    ...     return x, [z() for z in items]
    >>> test_func()
    (10, [4, 4, 4, 4, 4])

A comprehension's iteration var doesn't shadow implicit globals for sibling scopes:

    >>> g = -1
    >>> def test_func():
    ...     def inner():
    ...         return g
    ...     [g for g in range(5)]
    ...     return inner
    >>> test_func()()
    -1

A modification to a closed-over variable is visible in the outer scope:

    >>> def test_func():
    ...     x = -1
    ...     items = [(x:=y) for y in range(3)]
    ...     return x
    >>> test_func()
    2

Comprehensions' scopes don't interact with each other:

    >>> def test_func(lst):
    ...     ret = [lambda: x for x in lst]
    ...     inc = [x + 1 for x in lst]
    ...     [x for x in inc]
    ...     return ret
    >>> test_func(range(3))[0]()
    2

    >>> def test_func(lst):
    ...     x = -1
    ...     funcs = [lambda: x for x in lst]
    ...     items = [x + 1 for x in lst]
    ...     return x
    >>> test_func(range(3))
    -1

"""


class ListComprehensionTest(unittest.TestCase):
    def test_unbound_local_after_comprehension(self):
        def f():
            if False:
                x = 0
            [x for x in [1]]
            return x

        with self.assertRaises(UnboundLocalError):
            f()


__test__ = {'doctests' : doctests}

def load_tests(loader, tests, pattern):
    tests.addTest(doctest.DocTestSuite())
    return tests


if __name__ == "__main__":
    unittest.main()
