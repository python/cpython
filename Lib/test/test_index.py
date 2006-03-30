import unittest
from test import test_support
import operator

class oldstyle:
    def __index__(self):
        return self.ind

class newstyle(object):
    def __index__(self):
        return self.ind

class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self.o = oldstyle()
        self.n = newstyle()
        self.o2 = oldstyle()
        self.n2 = newstyle()

    def test_basic(self):
        self.o.ind = -2
        self.n.ind = 2
        assert(self.seq[self.n] == self.seq[2])
        assert(self.seq[self.o] == self.seq[-2])
        assert(operator.index(self.o) == -2)
        assert(operator.index(self.n) == 2)

    def test_error(self):
        self.o.ind = 'dumb'
        self.n.ind = 'bad'
        myfunc = lambda x, obj: obj.seq[x]
        self.failUnlessRaises(TypeError, operator.index, self.o)
        self.failUnlessRaises(TypeError, operator.index, self.n)
        self.failUnlessRaises(TypeError, myfunc, self.o, self)
        self.failUnlessRaises(TypeError, myfunc, self.n, self)

    def test_slice(self):
        self.o.ind = 1
        self.o2.ind = 3
        self.n.ind = 2
        self.n2.ind = 4
        assert(self.seq[self.o:self.o2] == self.seq[1:3])
        assert(self.seq[self.n:self.n2] == self.seq[2:4])

    def test_repeat(self):
        self.o.ind = 3
        self.n.ind = 2
        assert(self.seq * self.o == self.seq * 3)
        assert(self.seq * self.n == self.seq * 2)
        assert(self.o * self.seq == self.seq * 3)
        assert(self.n * self.seq == self.seq * 2)

    def test_wrappers(self):
        n = self.n
        n.ind = 5
        assert n.__index__() == 5
        assert 6 .__index__() == 6
        assert -7L.__index__() == -7
        assert self.seq.__getitem__(n) == self.seq[5]
        assert self.seq.__mul__(n) == self.seq * 5
        assert self.seq.__rmul__(n) == self.seq * 5

    def test_infinite_recusion(self):
        class Trap1(int):
            def __index__(self):
                return self
        class Trap2(long):
            def __index__(self):
                return self
        self.failUnlessRaises(TypeError, operator.getitem, self.seq, Trap1())
        self.failUnlessRaises(TypeError, operator.getitem, self.seq, Trap2())


class ListTestCase(BaseTestCase):
    seq = [0,10,20,30,40,50]

    def test_setdelitem(self):
        self.o.ind = -2
        self.n.ind = 2
        lst = list('ab!cdefghi!j')
        del lst[self.o]
        del lst[self.n]
        lst[self.o] = 'X'
        lst[self.n] = 'Y'
        assert lst == list('abYdefghXj')

        lst = [5, 6, 7, 8, 9, 10, 11]
        lst.__setitem__(self.n, "here")
        assert lst == [5, 6, "here", 8, 9, 10, 11]
        lst.__delitem__(self.n)
        assert lst == [5, 6, 8, 9, 10, 11]

    def test_inplace_repeat(self):
        self.o.ind = 2
        self.n.ind = 3
        lst = [6, 4]
        lst *= self.o
        assert lst == [6, 4, 6, 4]
        lst *= self.n
        assert lst == [6, 4, 6, 4] * 3

        lst = [5, 6, 7, 8, 9, 11]
        l2 = lst.__imul__(self.n)
        assert l2 is lst
        assert lst == [5, 6, 7, 8, 9, 11] * 3


class TupleTestCase(BaseTestCase):
    seq = (0,10,20,30,40,50)

class StringTestCase(BaseTestCase):
    seq = "this is a test"

class UnicodeTestCase(BaseTestCase):
    seq = u"this is a test"


class XRangeTestCase(unittest.TestCase):

    def test_xrange(self):
        n = newstyle()
        n.ind = 5
        assert xrange(1, 20)[n] == 6
        assert xrange(1, 20).__getitem__(n) == 6


def test_main():
    test_support.run_unittest(
        ListTestCase,
        TupleTestCase,
        StringTestCase,
        UnicodeTestCase,
        XRangeTestCase,
    )

if __name__ == "__main__":
    test_main()
