import unittest
from test import test_support
from itertools import *
import sys
import operator

def onearg(x):
    'Test function of one argument'
    return x

def errfunc(*args):
    'Test function that raises an error'
    raise ValueError

def gen3():
    'Non-restartable source sequence'
    for i in (0, 1, 2):
        yield i

def isEven(x):
    'Test predicate'
    return x%2==0

class StopNow:
    'Class emulating an empty iterable.'
    def __iter__(self):
        return self
    def next(self):
        raise StopIteration

class TestBasicOps(unittest.TestCase):
    def test_chain(self):
        self.assertEqual(list(chain('abc', 'def')), list('abcdef'))
        self.assertEqual(list(chain('abc')), list('abc'))
        self.assertEqual(list(chain('')), [])
        self.assertRaises(TypeError, chain, 2, 3)

    def test_count(self):
        self.assertEqual(zip('abc',count()), [('a', 0), ('b', 1), ('c', 2)])
        self.assertEqual(zip('abc',count(3)), [('a', 3), ('b', 4), ('c', 5)])
        self.assertRaises(TypeError, count, 2, 3)
        self.assertRaises(TypeError, count, 'a')
        c = count(sys.maxint-2)   # verify that rollover doesn't crash
        c.next(); c.next(); c.next(); c.next(); c.next()

    def test_cycle(self):
        self.assertEqual(list(islice(cycle('abc'),10)), list('abcabcabca'))
        self.assertEqual(list(cycle('')), [])
        self.assertRaises(TypeError, cycle)
        self.assertRaises(TypeError, cycle, 5)
        self.assertEqual(list(islice(cycle(gen3()),10)), [0,1,2,0,1,2,0,1,2,0])

    def test_ifilter(self):
        self.assertEqual(list(ifilter(isEven, range(6))), [0,2,4])
        self.assertEqual(list(ifilter(None, [0,1,0,2,0])), [1,2])
        self.assertRaises(TypeError, ifilter)
        self.assertRaises(TypeError, ifilter, lambda x:x)
        self.assertRaises(TypeError, ifilter, lambda x:x, range(6), 7)
        self.assertRaises(TypeError, ifilter, isEven, 3)
        self.assertRaises(TypeError, ifilter(range(6), range(6)).next)

    def test_ifilterfalse(self):
        self.assertEqual(list(ifilterfalse(isEven, range(6))), [1,3,5])
        self.assertEqual(list(ifilterfalse(None, [0,1,0,2,0])), [0,0,0])
        self.assertRaises(TypeError, ifilterfalse)
        self.assertRaises(TypeError, ifilterfalse, lambda x:x)
        self.assertRaises(TypeError, ifilterfalse, lambda x:x, range(6), 7)
        self.assertRaises(TypeError, ifilterfalse, isEven, 3)
        self.assertRaises(TypeError, ifilterfalse(range(6), range(6)).next)

    def test_izip(self):
        ans = [(x,y) for x, y in izip('abc',count())]
        self.assertEqual(ans, [('a', 0), ('b', 1), ('c', 2)])
        self.assertEqual(list(izip('abc', range(6))), zip('abc', range(6)))
        self.assertEqual(list(izip('abcdef', range(3))), zip('abcdef', range(3)))
        self.assertEqual(list(izip('abcdef')), zip('abcdef'))
        self.assertRaises(TypeError, izip)
        self.assertRaises(TypeError, izip, 3)
        self.assertRaises(TypeError, izip, range(3), 3)
        # Check tuple re-use (implementation detail)
        self.assertEqual([tuple(list(pair)) for pair in izip('abc', 'def')],
                         zip('abc', 'def'))
        ids = map(id, izip('abc', 'def'))
        self.assertEqual(min(ids), max(ids))

    def test_repeat(self):
        self.assertEqual(zip(xrange(3),repeat('a')),
                         [(0, 'a'), (1, 'a'), (2, 'a')])
        self.assertEqual(list(repeat('a', 3)), ['a', 'a', 'a'])
        self.assertEqual(list(repeat('a', 0)), [])
        self.assertEqual(list(repeat('a', -3)), [])
        self.assertRaises(TypeError, repeat)
        self.assertRaises(TypeError, repeat, None, 3, 4)
        self.assertRaises(TypeError, repeat, None, 'a')

    def test_imap(self):
        self.assertEqual(list(imap(operator.pow, range(3), range(1,7))),
                         [0**1, 1**2, 2**3])
        self.assertEqual(list(imap(None, 'abc', range(5))),
                         [('a',0),('b',1),('c',2)])
        self.assertEqual(list(imap(operator.pow, [])), [])
        self.assertRaises(TypeError, imap)
        self.assertRaises(TypeError, imap, operator.neg)
        self.assertRaises(TypeError, imap(10, range(5)).next)
        self.assertRaises(ValueError, imap(errfunc, [4], [5]).next)
        self.assertRaises(TypeError, imap(onearg, [4], [5]).next)

    def test_starmap(self):
        self.assertEqual(list(starmap(operator.pow, zip(range(3), range(1,7)))),
                         [0**1, 1**2, 2**3])
        self.assertEqual(list(starmap(operator.pow, [])), [])
        self.assertRaises(TypeError, list, starmap(operator.pow, [[4,5]]))
        self.assertRaises(TypeError, starmap)
        self.assertRaises(TypeError, starmap, operator.pow, [(4,5)], 'extra')
        self.assertRaises(TypeError, starmap(10, [(4,5)]).next)
        self.assertRaises(ValueError, starmap(errfunc, [(4,5)]).next)
        self.assertRaises(TypeError, starmap(onearg, [(4,5)]).next)

    def test_islice(self):
        for args in [          # islice(args) should agree with range(args)
                (10, 20, 3),
                (10, 3, 20),
                (10, 20),
                (10, 3),
                (20,)
                ]:
            self.assertEqual(list(islice(xrange(100), *args)), range(*args))

        for args, tgtargs in [  # Stop when seqn is exhausted
                ((10, 110, 3), ((10, 100, 3))),
                ((10, 110), ((10, 100))),
                ((110,), (100,))
                ]:
            self.assertEqual(list(islice(xrange(100), *args)), range(*tgtargs))

        # Test stop=None
        self.assertEqual(list(islice(xrange(10), None)), range(10))
        self.assertEqual(list(islice(xrange(10), 2, None)), range(2, 10))
        self.assertEqual(list(islice(xrange(10), 1, None, 2)), range(1, 10, 2))

        # Test invalid arguments
        self.assertRaises(TypeError, islice, xrange(10))
        self.assertRaises(TypeError, islice, xrange(10), 1, 2, 3, 4)
        self.assertRaises(ValueError, islice, xrange(10), -5, 10, 1)
        self.assertRaises(ValueError, islice, xrange(10), 1, -5, -1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 10, -1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 10, 0)
        self.assertRaises(ValueError, islice, xrange(10), 'a')
        self.assertRaises(ValueError, islice, xrange(10), 'a', 1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 'a')
        self.assertRaises(ValueError, islice, xrange(10), 'a', 1, 1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 'a', 1)
        self.assertEqual(len(list(islice(count(), 1, 10, sys.maxint))), 1)

    def test_takewhile(self):
        data = [1, 3, 5, 20, 2, 4, 6, 8]
        underten = lambda x: x<10
        self.assertEqual(list(takewhile(underten, data)), [1, 3, 5])
        self.assertEqual(list(takewhile(underten, [])), [])
        self.assertRaises(TypeError, takewhile)
        self.assertRaises(TypeError, takewhile, operator.pow)
        self.assertRaises(TypeError, takewhile, operator.pow, [(4,5)], 'extra')
        self.assertRaises(TypeError, takewhile(10, [(4,5)]).next)
        self.assertRaises(ValueError, takewhile(errfunc, [(4,5)]).next)

    def test_dropwhile(self):
        data = [1, 3, 5, 20, 2, 4, 6, 8]
        underten = lambda x: x<10
        self.assertEqual(list(dropwhile(underten, data)), [20, 2, 4, 6, 8])
        self.assertEqual(list(dropwhile(underten, [])), [])
        self.assertRaises(TypeError, dropwhile)
        self.assertRaises(TypeError, dropwhile, operator.pow)
        self.assertRaises(TypeError, dropwhile, operator.pow, [(4,5)], 'extra')
        self.assertRaises(TypeError, dropwhile(10, [(4,5)]).next)
        self.assertRaises(ValueError, dropwhile(errfunc, [(4,5)]).next)

    def test_StopIteration(self):
        for f in (chain, cycle, izip):
            self.assertRaises(StopIteration, f([]).next)
            self.assertRaises(StopIteration, f(StopNow()).next)

        self.assertRaises(StopIteration, islice([], None).next)
        self.assertRaises(StopIteration, islice(StopNow(), None).next)

        self.assertRaises(StopIteration, repeat(None, 0).next)

        for f in (ifilter, ifilterfalse, imap, takewhile, dropwhile, starmap):
            self.assertRaises(StopIteration, f(lambda x:x, []).next)
            self.assertRaises(StopIteration, f(lambda x:x, StopNow()).next)


libreftest = """ Doctest for examples in the library reference, libitertools.tex


>>> amounts = [120.15, 764.05, 823.14]
>>> for checknum, amount in izip(count(1200), amounts):
...     print 'Check %d is for $%.2f' % (checknum, amount)
...
Check 1200 is for $120.15
Check 1201 is for $764.05
Check 1202 is for $823.14

>>> import operator
>>> import operator
>>> for cube in imap(operator.pow, xrange(1,4), repeat(3)):
...    print cube
...
1
8
27

>>> reportlines = ['EuroPython', 'Roster', '', 'alex', '', 'laura', '', 'martin', '', 'walter', '', 'samuele']
>>> for name in islice(reportlines, 3, len(reportlines), 2):
...    print name.title()
...
Alex
Laura
Martin
Walter
Samuele

>>> def enumerate(iterable):
...     return izip(count(), iterable)

>>> def tabulate(function):
...     "Return function(0), function(1), ..."
...     return imap(function, count())

>>> def iteritems(mapping):
...     return izip(mapping.iterkeys(), mapping.itervalues())

>>> def nth(iterable, n):
...     "Returns the nth item"
...     return list(islice(iterable, n, n+1))

>>> def all(pred, seq):
...     "Returns True if pred(x) is True for every element in the iterable"
...     return not nth(ifilterfalse(pred, seq), 0)

>>> def some(pred, seq):
...     "Returns True if pred(x) is True at least one element in the iterable"
...     return bool(nth(ifilter(pred, seq), 0))

>>> def no(pred, seq):
...     "Returns True if pred(x) is False for every element in the iterable"
...     return not nth(ifilter(pred, seq), 0)

>>> def pairwise(seq):
...     "s -> (s0,s1), (s1,s2), (s2, s3), ..."
...     return izip(seq, islice(seq,1,len(seq)))

>>> def padnone(seq):
...     "Returns the sequence elements and then returns None indefinitely"
...     return chain(seq, repeat(None))

>>> def ncycles(seq, n):
...     "Returns the sequence elements n times"
...     return chain(*repeat(seq, n))

>>> def dotproduct(vec1, vec2):
...     return sum(imap(operator.mul, vec1, vec2))


This is not part of the examples but it tests to make sure the definitions
perform as purported.

>>> list(enumerate('abc'))
[(0, 'a'), (1, 'b'), (2, 'c')]

>>> list(islice(tabulate(lambda x: 2*x), 4))
[0, 2, 4, 6]

>>> nth('abcde', 3)
['d']

>>> all(lambda x: x%2==0, [2, 4, 6, 8])
True

>>> all(lambda x: x%2==0, [2, 3, 6, 8])
False

>>> some(lambda x: x%2==0, [2, 4, 6, 8])
True

>>> some(lambda x: x%2==0, [1, 3, 5, 9])
False

>>> no(lambda x: x%2==0, [1, 3, 5, 9])
True

>>> no(lambda x: x%2==0, [1, 2, 5, 9])
False

>>> list(pairwise('abc'))
[('a', 'b'), ('b', 'c')]

>>> list(islice(padnone('abc'), 0, 6))
['a', 'b', 'c', None, None, None]

>>> list(ncycles('abc', 3))
['a', 'b', 'c', 'a', 'b', 'c', 'a', 'b', 'c']

>>> dotproduct([1,2,3], [4,5,6])
32


"""

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    test_support.run_unittest(TestBasicOps)

    # verify reference counting
    import sys
    if verbose and hasattr(sys, "gettotalrefcount"):
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(TestBasicOps)
            counts[i] = sys.gettotalrefcount()
        print counts

    # doctest the examples in the library reference
    import doctest
    doctest.testmod(sys.modules[__name__])

if __name__ == "__main__":
    test_main(verbose=True)
