import unittest
from test import test_support
from itertools import *
import sys

class TestBasicOps(unittest.TestCase):
    def test_chain(self):
        self.assertEqual(list(chain('abc', 'def')), list('abcdef'))

    def test_count(self):
        self.assertEqual(zip('abc',count()), [('a', 0), ('b', 1), ('c', 2)])
        self.assertEqual(zip('abc',count(3)), [('a', 3), ('b', 4), ('c', 5)])
        self.assertRaises(TypeError, count, 2, 3)

    def test_cycle(self):
        self.assertEqual(list(islice(cycle('abc'),10)), list('abcabcabca'))
        self.assertEqual(list(cycle('')), [])

    def test_ifilter(self):
        def isEven(x):
            return x%2==0
        self.assertEqual(list(ifilter(isEven, range(6))), [0,2,4])
        self.assertEqual(list(ifilter(None, [0,1,0,2,0])), [1,2])
        self.assertRaises(TypeError, ifilter)
        self.assertRaises(TypeError, ifilter, 3)
        self.assertRaises(TypeError, ifilter, isEven, 3)

    def test_ifilterfalse(self):
        def isEven(x):
            return x%2==0
        self.assertEqual(list(ifilterfalse(isEven, range(6))), [1,3,5])
        self.assertEqual(list(ifilterfalse(None, [0,1,0,2,0])), [0,0,0])
        self.assertRaises(TypeError, ifilterfalse)
        self.assertRaises(TypeError, ifilterfalse, 3)
        self.assertRaises(TypeError, ifilterfalse, isEven, 3)

    def test_izip(self):
        ans = [(x,y) for x, y in izip('abc',count())]
        self.assertEqual(ans, [('a', 0), ('b', 1), ('c', 2)])
        self.assertRaises(TypeError, izip)

    def test_repeat(self):
        self.assertEqual(zip(xrange(3),repeat('a')),
                         [(0, 'a'), (1, 'a'), (2, 'a')])
        self.assertEqual(list(repeat('a', 3)), ['a', 'a', 'a'])
        self.assertRaises(TypeError, repeat)

    def test_imap(self):
        import operator
        self.assertEqual(list(imap(operator.pow, range(3), range(1,7))),
                         [0**1, 1**2, 2**3])
        self.assertEqual(list(imap(None, 'abc', range(5))),
                         [('a',0),('b',1),('c',2)])
        self.assertRaises(TypeError, imap)
        self.assertRaises(TypeError, imap, operator.neg)

    def test_starmap(self):
        import operator
        self.assertEqual(list(starmap(operator.pow, zip(range(3), range(1,7)))),
                         [0**1, 1**2, 2**3])
        self.assertRaises(TypeError, list, starmap(operator.pow, [[4,5]]))

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

        self.assertRaises(TypeError, islice, xrange(10))
        self.assertRaises(TypeError, islice, xrange(10), 1, 2, 3, 4)
        self.assertRaises(ValueError, islice, xrange(10), -5, 10, 1)
        self.assertRaises(ValueError, islice, xrange(10), 1, -5, -1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 10, -1)
        self.assertRaises(ValueError, islice, xrange(10), 1, 10, 0)
        self.assertEqual(len(list(islice(count(), 1, 10, sys.maxint))), 1)

    def test_takewhile(self):
        data = [1, 3, 5, 20, 2, 4, 6, 8]
        underten = lambda x: x<10
        self.assertEqual(list(takewhile(underten, data)), [1, 3, 5])

    def test_dropwhile(self):
        data = [1, 3, 5, 20, 2, 4, 6, 8]
        underten = lambda x: x<10
        self.assertEqual(list(dropwhile(underten, data)), [20, 2, 4, 6, 8])

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

"""

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    import test_itertools
    suite = unittest.TestSuite()
    for testclass in (TestBasicOps,
                          ):
        suite.addTest(unittest.makeSuite(testclass))
    test_support.run_suite(suite)
    test_support.run_doctest(test_itertools, verbose)

    # verify reference counting
    import sys
    if verbose and hasattr(sys, "gettotalrefcount"):
        counts = []
        for i in xrange(5):
            test_support.run_suite(suite)
            counts.append(sys.gettotalrefcount()-i)
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
