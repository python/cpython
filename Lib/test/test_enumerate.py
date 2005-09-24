import unittest
import sys

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
        3 // 0

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
        self.assertEqual(len(set(map(id, list(enumerate(self.seq))))), len(self.seq))
        self.assertEqual(len(set(map(id, enumerate(self.seq)))), min(1,len(self.seq)))

class MyEnum(enumerate):
    pass

class SubclassTestCase(EnumerateTestCase):

    enum = MyEnum

class TestEmpty(EnumerateTestCase):

    seq, res = '', []

class TestBig(EnumerateTestCase):

    seq = range(10,20000,2)
    res = zip(range(20000), seq)

class TestReversed(unittest.TestCase):

    def test_simple(self):
        class A:
            def __getitem__(self, i):
                if i < 5:
                    return str(i)
                raise StopIteration
            def __len__(self):
                return 5
        for data in 'abc', range(5), tuple(enumerate('abc')), A(), xrange(1,17,5):
            self.assertEqual(list(data)[::-1], list(reversed(data)))
        self.assertRaises(TypeError, reversed, {})

    def test_xrange_optimization(self):
        x = xrange(1)
        self.assertEqual(type(reversed(x)), type(iter(x)))

    def test_len(self):
        # This is an implementation detail, not an interface requirement
        from test.test_iterlen import len
        for s in ('hello', tuple('hello'), list('hello'), xrange(5)):
            self.assertEqual(len(reversed(s)), len(s))
            r = reversed(s)
            list(r)
            self.assertEqual(len(r), 0)
        class SeqWithWeirdLen:
            called = False
            def __len__(self):
                if not self.called:
                    self.called = True
                    return 10
                raise ZeroDivisionError
            def __getitem__(self, index):
                return index
        r = reversed(SeqWithWeirdLen())
        self.assertRaises(ZeroDivisionError, len, r)


    def test_gc(self):
        class Seq:
            def __len__(self):
                return 10
            def __getitem__(self, index):
                return index
        s = Seq()
        r = reversed(s)
        s.r = r

    def test_args(self):
        self.assertRaises(TypeError, reversed)
        self.assertRaises(TypeError, reversed, [], 'extra')

    def test_bug1229429(self):
        # this bug was never in reversed, it was in
        # PyObject_CallMethod, and reversed_new calls that sometimes.
        if not hasattr(sys, "getrefcount"):
            return
        def f():
            pass
        r = f.__reversed__ = object()
        rc = sys.getrefcount(r)
        for i in range(10):
            try:
                reversed(f)
            except TypeError:
                pass
            else:
                self.fail("non-callable __reversed__ didn't raise!")
        self.assertEqual(rc, sys.getrefcount(r))


def test_main(verbose=None):
    testclasses = (EnumerateTestCase, SubclassTestCase, TestEmpty, TestBig,
                   TestReversed)
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
