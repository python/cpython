doctests = """

Test simple loop with conditional

    >>> sum(i*i for i in range(100) if i&1 == 1)
    166650

Test simple nesting

    >>> list((i,j) for i in range(3) for j in range(4) )
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]

Test nesting with the inner expression dependent on the outer

    >>> list((i,j) for i in range(4) for j in range(i) )
    [(1, 0), (2, 0), (2, 1), (3, 0), (3, 1), (3, 2)]

Make sure the induction variable is not exposed

    >>> i = 20
    >>> sum(i*i for i in range(100))
    328350
    >>> i
    20

Test first class

    >>> g = (i*i for i in range(4))
    >>> type(g)
    <type 'generator'>
    >>> list(g)
    [0, 1, 4, 9]

Test direct calls to next()

    >>> g = (i*i for i in range(3))
    >>> g.next()
    0
    >>> g.next()
    1
    >>> g.next()
    4
    >>> g.next()
    Traceback (most recent call last):
      File "<pyshell#21>", line 1, in -toplevel-
        g.next()
    StopIteration

Does it stay stopped?

    >>> g.next()
    Traceback (most recent call last):
      File "<pyshell#21>", line 1, in -toplevel-
        g.next()
    StopIteration
    >>> list(g)
    []

Test running gen when defining function is out of scope

    >>> def f(n):
    ...     return (i*i for i in xrange(n))
    >>> list(f(10))
    [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

    >>> def f(n):
    ...     return ((i,j) for i in xrange(3) for j in xrange(n))
    >>> list(f(4))
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]
    >>> def f(n):
    ...     return ((i,j) for i in xrange(3) for j in xrange(4) if j in xrange(n))
    >>> list(f(4))
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]
    >>> list(f(2))
    [(0, 0), (0, 1), (1, 0), (1, 1), (2, 0), (2, 1)]

Verify that parenthesis are required in a statement

    >>> def f(n):
    ...     return i*i for i in xrange(n)
    Traceback (most recent call last):
       ...
    SyntaxError: invalid syntax

Verify early binding for the outermost for-expression

    >>> x=10
    >>> g = (i*i for i in range(x))
    >>> x = 5
    >>> list(g)
    [0, 1, 4, 9, 16, 25, 36, 49, 64, 81]

Verify that the outermost for-expression makes an immediate check
for iterability

    >>> (i for i in 6)
    Traceback (most recent call last):
      File "<pyshell#4>", line 1, in -toplevel-
        (i for i in 6)
    TypeError: iteration over non-sequence

Verify late binding for the outermost if-expression

    >>> include = (2,4,6,8)
    >>> g = (i*i for i in range(10) if i in include)
    >>> include = (1,3,5,7,9)
    >>> list(g)
    [1, 9, 25, 49, 81]

Verify late binding for the innermost for-expression

    >>> g = ((i,j) for i in range(3) for j in range(x))
    >>> x = 4
    >>> list(g)
    [(0, 0), (0, 1), (0, 2), (0, 3), (1, 0), (1, 1), (1, 2), (1, 3), (2, 0), (2, 1), (2, 2), (2, 3)]

Verify re-use of tuples (a side benefit of using genexps over listcomps)

    >>> tupleids = map(id, ((i,i) for i in xrange(10)))
    >>> max(tupleids) - min(tupleids)
    0

Verify that syntax error's are raised for genexps used as lvalues

    >>> (y for y in (1,2)) = 10
    Traceback (most recent call last):
       ...
    SyntaxError: assign to generator expression not possible

    >>> (y for y in (1,2)) += 10
    Traceback (most recent call last):
       ...
    SyntaxError: augmented assign to tuple literal or generator expression not possible



########### Tests borrowed from or inspired by test_generators.py ############

Make a generator that acts like range()

    >>> yrange = lambda n:  (i for i in xrange(n))
    >>> list(yrange(10))
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

Generators always return to the most recent caller:

    >>> def creator():
    ...     r = yrange(5)
    ...     print "creator", r.next()
    ...     return r
    >>> def caller():
    ...     r = creator()
    ...     for i in r:
    ...             print "caller", i
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
    >>> list(zrange(5))
    [0, 1, 2, 3, 4]


Verify that a gen exp cannot be resumed while it is actively running:

    >>> g = (me.next() for i in xrange(10))
    >>> me = g
    >>> me.next()
    Traceback (most recent call last):
      File "<pyshell#30>", line 1, in -toplevel-
        me.next()
      File "<pyshell#28>", line 1, in <generator expression>
        g = (me.next() for i in xrange(10))
    ValueError: generator already executing

Verify exception propagation

    >>> g = (10 // i for i in (5, 0, 2))
    >>> g.next()
    2
    >>> g.next()
    Traceback (most recent call last):
      File "<pyshell#37>", line 1, in -toplevel-
        g.next()
      File "<pyshell#35>", line 1, in <generator expression>
        g = (10 // i for i in (5, 0, 2))
    ZeroDivisionError: integer division or modulo by zero
    >>> g.next()
    Traceback (most recent call last):
      File "<pyshell#38>", line 1, in -toplevel-
        g.next()
    StopIteration

Make sure that None is a valid return value

    >>> list(None for i in xrange(10))
    [None, None, None, None, None, None, None, None, None, None]

Check that generator attributes are present

    >>> g = (i*i for i in range(3))
    >>> expected = set(['gi_frame', 'gi_running', 'next'])
    >>> set(attr for attr in dir(g) if not attr.startswith('__')) >= expected
    True

    >>> print g.next.__doc__
    x.next() -> the next value, or raise StopIteration
    >>> import types
    >>> isinstance(g, types.GeneratorType)
    True

Check the __iter__ slot is defined to return self

    >>> iter(g) is g
    True

Verify that the running flag is set properly

    >>> g = (me.gi_running for i in (0,1))
    >>> me = g
    >>> me.gi_running
    0
    >>> me.next()
    1
    >>> me.gi_running
    0

Verify that genexps are weakly referencable

    >>> import weakref
    >>> g = (i*i for i in range(4))
    >>> wr = weakref.ref(g)
    >>> wr() is g
    True
    >>> p = weakref.proxy(g)
    >>> list(p)
    [0, 1, 4, 9]


"""


__test__ = {'doctests' : doctests}

def test_main(verbose=None):
    import sys
    from test import test_support
    from test import test_genexps
    test_support.run_doctest(test_genexps, verbose)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_doctest(test_genexps, verbose)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
