import unittest

import test_support

seq, res = 'abc', [(0,'a'), (1,'b'), (2,'c')]

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

    def test_basicfunction(self):
        self.assertEqual(type(self.enum(seq)), self.enum)
        e = self.enum(seq)
        self.assertEqual(iter(e), e)
        self.assertEqual(list(self.enum(seq)), res)
        self.enum.__doc__

    def test_getitemseqn(self):
        self.assertEqual(list(self.enum(G(seq))), res)
        e = self.enum(G(''))
        self.assertRaises(StopIteration, e.next)

    def test_iteratorseqn(self):
        self.assertEqual(list(self.enum(I(seq))), res)
        e = self.enum(I(''))
        self.assertRaises(StopIteration, e.next)

    def test_iteratorgenerator(self):
        self.assertEqual(list(self.enum(Ig(seq))), res)
        e = self.enum(Ig(''))
        self.assertRaises(StopIteration, e.next)

    def test_noniterable(self):
        self.assertRaises(TypeError, self.enum, X(seq))

    def test_illformediterable(self):
        self.assertRaises(TypeError, list, self.enum(N(seq)))

    def test_exception_propagation(self):
        self.assertRaises(ZeroDivisionError, list, self.enum(E(seq)))

class MyEnum(enumerate):
    pass

class SubclassTestCase(EnumerateTestCase):

    enum = MyEnum

def suite():
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(EnumerateTestCase))
    suite.addTest(unittest.makeSuite(SubclassTestCase))
    return suite

def test_main():
    test_support.run_suite(suite())

if __name__ == "__main__":
    test_main()
