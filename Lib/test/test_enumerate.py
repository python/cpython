import unittest
from sets import Set

from test import test_support

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

class E:
    'Test propagation of exceptions'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self
    def next(self):
        3/0

class N:
    'Iterator missing next()'
    def __init__(self, seqn):
        self.seqn = seqn
        self.i = 0
    def __iter__(self):
        return self

class EnumerateTestCase(unittest.TestCase):

    enum = enumerate
    seq, res = 'abc', [(0,'a'), (1,'b'), (2,'c')]

    def test_basicfunction(self):
        self.assertEqual(type(self.enum(self.seq)), self.enum)
        e = self.enum(self.seq)
        self.assertEqual(iter(e), e)
        self.assertEqual(list(self.enum(self.seq)), self.res)
        self.enum.__doc__

    def test_getitemseqn(self):
        self.assertEqual(list(self.enum(G(self.seq))), self.res)
        e = self.enum(G(''))
        self.assertRaises(StopIteration, e.next)

    def test_iteratorseqn(self):
        self.assertEqual(list(self.enum(I(self.seq))), self.res)
        e = self.enum(I(''))
        self.assertRaises(StopIteration, e.next)

    def test_iteratorgenerator(self):
        self.assertEqual(list(self.enum(Ig(self.seq))), self.res)
        e = self.enum(Ig(''))
        self.assertRaises(StopIteration, e.next)

    def test_noniterable(self):
        self.assertRaises(TypeError, self.enum, X(self.seq))

    def test_illformediterable(self):
        self.assertRaises(TypeError, list, self.enum(N(self.seq)))

    def test_exception_propagation(self):
        self.assertRaises(ZeroDivisionError, list, self.enum(E(self.seq)))

    def test_argumentcheck(self):
        self.assertRaises(TypeError, self.enum) # no arguments
        self.assertRaises(TypeError, self.enum, 1) # wrong type (not iterable)
        self.assertRaises(TypeError, self.enum, 'abc', 2) # too many arguments

    def test_tuple_reuse(self):
        # Tests an implementation detail where tuple is reused
        # whenever nothing else holds a reference to it
        self.assertEqual(len(Set(map(id, list(enumerate(self.seq))))), len(self.seq))
        self.assertEqual(len(Set(map(id, enumerate(self.seq)))), min(1,len(self.seq)))

class MyEnum(enumerate):
    pass

class SubclassTestCase(EnumerateTestCase):

    enum = MyEnum

class TestEmpty(EnumerateTestCase):

    seq, res = '', []

class TestBig(EnumerateTestCase):

    seq = range(10,20000,2)
    res = zip(range(20000), seq)


def test_main(verbose=None):
    testclasses = (EnumerateTestCase, SubclassTestCase, TestEmpty, TestBig)
    test_support.run_unittest(*testclasses)

    # verify reference counting
    import sys
    if verbose and hasattr(sys, "gettotalrefcount"):
        counts = [None] * 5
        for i in xrange(len(counts)):
            test_support.run_unittest(*testclasses)
            counts[i] = sys.gettotalrefcount()
        print counts

if __name__ == "__main__":
    test_main(verbose=True)
