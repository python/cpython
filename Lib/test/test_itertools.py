import unittest
from test import test_support
from itertools import *
import sys
import operator

def onearg(x):
    'Test function of one argument'
    return 2*x

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

def isOdd(x):
    'Test predicate'
    return x%2==1

class StopNow:
    'Class emulating an empty iterable.'
    def __iter__(self):
        return self
    def next(self):
        raise StopIteration

def take(n, seq):
    'Convenience function for partially consuming a long of infinite iterable'
    return list(islice(seq, n))

class TestBasicOps(unittest.TestCase):
    def test_chain(self):
        self.assertEqual(list(chain('abc', 'def')), list('abcdef'))
        self.assertEqual(list(chain('abc')), list('abc'))
        self.assertEqual(list(chain('')), [])
        self.assertEqual(take(4, chain('abc', 'def')), list('abcd'))
        self.assertRaises(TypeError, chain, 2, 3)

    def test_count(self):
        self.assertEqual(zip('abc',count()), [('a', 0), ('b', 1), ('c', 2)])
        self.assertEqual(zip('abc',count(3)), [('a', 3), ('b', 4), ('c', 5)])
        self.assertEqual(take(2, zip('abc',count(3))), [('a', 3), ('b', 4)])
        self.assertRaises(TypeError, count, 2, 3)
        self.assertRaises(TypeError, count, 'a')
        c = count(sys.maxint-2)   # verify that rollover doesn't crash
        c.next(); c.next(); c.next(); c.next(); c.next()

    def test_cycle(self):
        self.assertEqual(take(10, cycle('abc')), list('abcabcabca'))
        self.assertEqual(list(cycle('')), [])
        self.assertRaises(TypeError, cycle)
        self.assertRaises(TypeError, cycle, 5)
        self.assertEqual(list(islice(cycle(gen3()),10)), [0,1,2,0,1,2,0,1,2,0])

    def test_ifilter(self):
        self.assertEqual(list(ifilter(isEven, range(6))), [0,2,4])
        self.assertEqual(list(ifilter(None, [0,1,0,2,0])), [1,2])
        self.assertEqual(take(4, ifilter(isEven, count())), [0,2,4,6])
        self.assertRaises(TypeError, ifilter)
        self.assertRaises(TypeError, ifilter, lambda x:x)
        self.assertRaises(TypeError, ifilter, lambda x:x, range(6), 7)
        self.assertRaises(TypeError, ifilter, isEven, 3)
        self.assertRaises(TypeError, ifilter(range(6), range(6)).next)

    def test_ifilterfalse(self):
        self.assertEqual(list(ifilterfalse(isEven, range(6))), [1,3,5])
        self.assertEqual(list(ifilterfalse(None, [0,1,0,2,0])), [0,0,0])
        self.assertEqual(take(4, ifilterfalse(isEven, count())), [1,3,5,7])
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
        self.assertEqual(take(3,izip('abcdef', count())), zip('abcdef', range(3)))
        self.assertEqual(list(izip('abcdef')), zip('abcdef'))
        self.assertEqual(list(izip()), [])
        self.assertRaises(TypeError, izip, 3)
        self.assertRaises(TypeError, izip, range(3), 3)
        # Check tuple re-use (implementation detail)
        self.assertEqual([tuple(list(pair)) for pair in izip('abc', 'def')],
                         zip('abc', 'def'))
        self.assertEqual([pair for pair in izip('abc', 'def')],
                         zip('abc', 'def'))
        ids = map(id, izip('abc', 'def'))
        self.assertEqual(min(ids), max(ids))
        ids = map(id, list(izip('abc', 'def')))
        self.assertEqual(len(dict.fromkeys(ids)), len(ids))

    def test_repeat(self):
        self.assertEqual(zip(xrange(3),repeat('a')),
                         [(0, 'a'), (1, 'a'), (2, 'a')])
        self.assertEqual(list(repeat('a', 3)), ['a', 'a', 'a'])
        self.assertEqual(take(3, repeat('a')), ['a', 'a', 'a'])
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
        self.assertEqual(list(imap(None, 'abc', count())),
                         [('a',0),('b',1),('c',2)])
        self.assertEqual(take(2,imap(None, 'abc', count())),
                         [('a',0),('b',1)])
        self.assertEqual(list(imap(operator.pow, [])), [])
        self.assertRaises(TypeError, imap)
        self.assertRaises(TypeError, imap, operator.neg)
        self.assertRaises(TypeError, imap(10, range(5)).next)
        self.assertRaises(ValueError, imap(errfunc, [4], [5]).next)
        self.assertRaises(TypeError, imap(onearg, [4], [5]).next)

    def test_starmap(self):
        self.assertEqual(list(starmap(operator.pow, zip(range(3), range(1,7)))),
                         [0**1, 1**2, 2**3])
        self.assertEqual(take(3, starmap(operator.pow, izip(count(), count(1)))),
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
        self.assertRaises(StopIteration, izip().next)

        for f in (chain, cycle, izip):
            self.assertRaises(StopIteration, f([]).next)
            self.assertRaises(StopIteration, f(StopNow()).next)

        self.assertRaises(StopIteration, islice([], None).next)
        self.assertRaises(StopIteration, islice(StopNow(), None).next)

        self.assertRaises(StopIteration, repeat(None, 0).next)

        for f in (ifilter, ifilterfalse, imap, takewhile, dropwhile, starmap):
            self.assertRaises(StopIteration, f(lambda x:x, []).next)
            self.assertRaises(StopIteration, f(lambda x:x, StopNow()).next)

def R(seqn):
    'Regular generator'
    for i in seqn:
        yield i

class G:
    'Sequence using __getitem__'
    def __init__(self, seqn):
        self.seqn = seqn
    def __getitem__(self, i):
        return self.seqn[i]

class I:
    'Sequence using iterator protocol'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class Ig:
    'Sequence using iterator protocol defined with a generator'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        for val in self.seqn:
            yield val

class X:
    'Missing __getitem__ and __iter__'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def next(self):
        if self.i >= len(self.seqn): raise StopIteration
        v = self.seqn[self.i]
        self.i += 1
        return v

class N:
    'Iterator missing next()'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self

class E:
    'Test propagation of exceptions'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        3/0

class S:
    'Test immediate stop'
    def __init__(self, seqn):
        pass
    def __iter__(self):
        return self
    def next(self):
        raise StopIteration

def L(seqn):
    'Test multiple tiers of iterators'
    return chain(imap(lambda x:x, R(Ig(G(seqn)))))

class TestGC(unittest.TestCase):

    def makecycle(self, iterator, container):
        container.append(iterator)
        iterator.next()
        del container, iterator

    def test_chain(self):
        a = []
        self.makecycle(chain(a), a)

    def test_cycle(self):
        a = []
        self.makecycle(cycle([a]*2), a)

    def test_ifilter(self):
        a = []
        self.makecycle(ifilter(lambda x:True, [a]*2), a)

    def test_ifilterfalse(self):
        a = []
        self.makecycle(ifilterfalse(lambda x:False, a), a)

    def test_izip(self):
        a = []
        self.makecycle(izip([a]*2, [a]*3), a)

    def test_imap(self):
        a = []
        self.makecycle(imap(lambda x:x, [a]*2), a)

    def test_islice(self):
        a = []
        self.makecycle(islice([a]*2, None), a)

    def test_starmap(self):
        a = []
        self.makecycle(starmap(lambda *t: t, [(a,a)]*2), a)


class TestVariousIteratorArgs(unittest.TestCase):

    def test_chain(self):
        for s in ("123", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(chain(g(s))), list(g(s)))
                self.assertEqual(list(chain(g(s), g(s))), list(g(s))+list(g(s)))
            self.assertRaises(TypeError, chain, X(s))
            self.assertRaises(TypeError, list, chain(N(s)))
            self.assertRaises(ZeroDivisionError, list, chain(E(s)))

    def test_cycle(self):
        for s in ("123", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                tgtlen = len(s) * 3
                expected = list(g(s))*3
                actual = list(islice(cycle(g(s)), tgtlen))
                self.assertEqual(actual, expected)
            self.assertRaises(TypeError, cycle, X(s))
            self.assertRaises(TypeError, list, cycle(N(s)))
            self.assertRaises(ZeroDivisionError, list, cycle(E(s)))

    def test_ifilter(self):
        for s in (range(10), range(0), range(1000), (7,11), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(ifilter(isEven, g(s))), filter(isEven, g(s)))
            self.assertRaises(TypeError, ifilter, isEven, X(s))
            self.assertRaises(TypeError, list, ifilter(isEven, N(s)))
            self.assertRaises(ZeroDivisionError, list, ifilter(isEven, E(s)))

    def test_ifilterfalse(self):
        for s in (range(10), range(0), range(1000), (7,11), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(ifilterfalse(isEven, g(s))), filter(isOdd, g(s)))
            self.assertRaises(TypeError, ifilterfalse, isEven, X(s))
            self.assertRaises(TypeError, list, ifilterfalse(isEven, N(s)))
            self.assertRaises(ZeroDivisionError, list, ifilterfalse(isEven, E(s)))

    def test_izip(self):
        for s in ("123", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(izip(g(s))), zip(g(s)))
                self.assertEqual(list(izip(g(s), g(s))), zip(g(s), g(s)))
            self.assertRaises(TypeError, izip, X(s))
            self.assertRaises(TypeError, list, izip(N(s)))
            self.assertRaises(ZeroDivisionError, list, izip(E(s)))

    def test_imap(self):
        for s in (range(10), range(0), range(100), (7,11), xrange(20,50,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(imap(onearg, g(s))), map(onearg, g(s)))
                self.assertEqual(list(imap(operator.pow, g(s), g(s))), map(operator.pow, g(s), g(s)))
            self.assertRaises(TypeError, imap, onearg, X(s))
            self.assertRaises(TypeError, list, imap(onearg, N(s)))
            self.assertRaises(ZeroDivisionError, list, imap(onearg, E(s)))

    def test_islice(self):
        for s in ("12345", "", range(1000), ('do', 1.2), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                self.assertEqual(list(islice(g(s),1,None,2)), list(g(s))[1::2])
            self.assertRaises(TypeError, islice, X(s), 10)
            self.assertRaises(TypeError, list, islice(N(s), 10))
            self.assertRaises(ZeroDivisionError, list, islice(E(s), 10))

    def test_starmap(self):
        for s in (range(10), range(0), range(100), (7,11), xrange(20,50,5)):
            for g in (G, I, Ig, S, L, R):
                ss = zip(s, s)
                self.assertEqual(list(starmap(operator.pow, g(ss))), map(operator.pow, g(s), g(s)))
            self.assertRaises(TypeError, starmap, operator.pow, X(ss))
            self.assertRaises(TypeError, list, starmap(operator.pow, N(ss)))
            self.assertRaises(ZeroDivisionError, list, starmap(operator.pow, E(ss)))

    def test_takewhile(self):
        for s in (range(10), range(0), range(1000), (7,11), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                tgt = []
                for elem in g(s):
                    if not isEven(elem): break
                    tgt.append(elem)
                self.assertEqual(list(takewhile(isEven, g(s))), tgt)
            self.assertRaises(TypeError, takewhile, isEven, X(s))
            self.assertRaises(TypeError, list, takewhile(isEven, N(s)))
            self.assertRaises(ZeroDivisionError, list, takewhile(isEven, E(s)))

    def test_dropwhile(self):
        for s in (range(10), range(0), range(1000), (7,11), xrange(2000,2200,5)):
            for g in (G, I, Ig, S, L, R):
                tgt = []
                for elem in g(s):
                    if not tgt and isOdd(elem): continue
                    tgt.append(elem)
                self.assertEqual(list(dropwhile(isOdd, g(s))), tgt)
            self.assertRaises(TypeError, dropwhile, isOdd, X(s))
            self.assertRaises(TypeError, list, dropwhile(isOdd, N(s)))
            self.assertRaises(ZeroDivisionError, list, dropwhile(isOdd, E(s)))

class RegressionTests(unittest.TestCase):

    def test_sf_793826(self):
        # Fix Armin Rigo's successful efforts to wreak havoc

        def mutatingtuple(tuple1, f, tuple2):
            # this builds a tuple t which is a copy of tuple1,
            # then calls f(t), then mutates t to be equal to tuple2
            # (needs len(tuple1) == len(tuple2)).
            def g(value, first=[1]):
                if first:
                    del first[:]
                    f(z.next())
                return value
            items = list(tuple2)
            items[1:1] = list(tuple1)
            gen = imap(g, items)
            z = izip(*[gen]*len(tuple1))
            z.next()

        def f(t):
            global T
            T = t
            first[:] = list(T)

        first = []
        mutatingtuple((1,2,3), f, (4,5,6))
        second = list(T)
        self.assertEqual(first, second)


    def test_sf_950057(self):
        # Make sure that chain() and cycle() catch exceptions immediately
        # rather than when shifting between input sources

        def gen1():
            hist.append(0)
            yield 1
            hist.append(1)
            assert False
            hist.append(2)

        def gen2(x):
            hist.append(3)
            yield 2
            hist.append(4)
            if x:
                raise StopIteration

        hist = []
        self.assertRaises(AssertionError, list, chain(gen1(), gen2(False)))
        self.assertEqual(hist, [0,1])

        hist = []
        self.assertRaises(AssertionError, list, chain(gen1(), gen2(True)))
        self.assertEqual(hist, [0,1])

        hist = []
        self.assertRaises(AssertionError, list, cycle(gen1()))
        self.assertEqual(hist, [0,1])

libreftest = """ Doctest for examples in the library reference: libitertools.tex


>>> amounts = [120.15, 764.05, 823.14]
>>> for checknum, amount in izip(count(1200), amounts):
...     print 'Check %d is for $%.2f' % (checknum, amount)
...
Check 1200 is for $120.15
Check 1201 is for $764.05
Check 1202 is for $823.14

>>> import operator
>>> for cube in imap(operator.pow, xrange(1,4), repeat(3)):
...    print cube
...
1
8
27

>>> reportlines = ['EuroPython', 'Roster', '', 'alex', '', 'laura', '', 'martin', '', 'walter', '', 'samuele']
>>> for name in islice(reportlines, 3, None, 2):
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

>>> def all(seq, pred=bool):
...     "Returns True if pred(x) is True for every element in the iterable"
...     return False not in imap(pred, seq)

>>> def any(seq, pred=bool):
...     "Returns True if pred(x) is True for at least one element in the iterable"
...     return True in imap(pred, seq)

>>> def no(seq, pred=bool):
...     "Returns True if pred(x) is False for every element in the iterable"
...     return True not in imap(pred, seq)

>>> def quantify(seq, pred=bool):
...     "Count how many times the predicate is True in the sequence"
...     return sum(imap(pred, seq))

>>> def padnone(seq):
...     "Returns the sequence elements and then returns None indefinitely"
...     return chain(seq, repeat(None))

>>> def ncycles(seq, n):
...     "Returns the sequence elements n times"
...     return chain(*repeat(seq, n))

>>> def dotproduct(vec1, vec2):
...     return sum(imap(operator.mul, vec1, vec2))

>>> def window(seq, n=2):
...     "Returns a sliding window (of width n) over data from the iterable"
...     "   s -> (s0,s1,...s[n-1]), (s1,s2,...,sn), ...                   "
...     it = iter(seq)
...     result = tuple(islice(it, n))
...     if len(result) == n:
...         yield result
...     for elem in it:
...         result = result[1:] + (elem,)
...         yield result

>>> def take(n, seq):
...     return list(islice(seq, n))

This is not part of the examples but it tests to make sure the definitions
perform as purported.

>>> list(enumerate('abc'))
[(0, 'a'), (1, 'b'), (2, 'c')]

>>> list(islice(tabulate(lambda x: 2*x), 4))
[0, 2, 4, 6]

>>> nth('abcde', 3)
['d']

>>> all([2, 4, 6, 8], lambda x: x%2==0)
True

>>> all([2, 3, 6, 8], lambda x: x%2==0)
False

>>> any([2, 4, 6, 8], lambda x: x%2==0)
True

>>> any([1, 3, 5, 9], lambda x: x%2==0,)
False

>>> no([1, 3, 5, 9], lambda x: x%2==0)
True

>>> no([1, 2, 5, 9], lambda x: x%2==0)
False

>>> quantify(xrange(99), lambda x: x%2==0)
50

>>> list(window('abc'))
[('a', 'b'), ('b', 'c')]

>>> list(window('abc',5))
[]

>>> list(islice(padnone('abc'), 0, 6))
['a', 'b', 'c', None, None, None]

>>> list(ncycles('abc', 3))
['a', 'b', 'c', 'a', 'b', 'c', 'a', 'b', 'c']

>>> dotproduct([1,2,3], [4,5,6])
32

>>> take(10, count())
[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]

"""

__test__ = {'libreftest' : libreftest}

def test_main(verbose=None):
    test_classes = (TestBasicOps, TestVariousIteratorArgs, TestGC,
                    RegressionTests)
    test_support.run_unittest(*test_classes)

    # verify reference counting
    if verbose and hasattr(sys, "gettotalrefcount"):
        import gc
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*test_classes)
            gc.collect()
            counts[i] = sys.gettotalrefcount()
        print counts

    # doctest the examples in the library reference
    test_support.run_doctest(sys.modules[__name__], verbose)

if __name__ == "__main__":
    test_main(verbose=True)
