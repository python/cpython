import unittest
from test import test_support
import operator

class oldstyle:
    def __index__(self):
        return self.ind

class newstyle(object):
    def __index__(self):
        return self.ind

class ListTestCase(unittest.TestCase):
    def setUp(self):
        self.seq = [0,10,20,30,40,50]
        self.o = oldstyle()
        self.n = newstyle()
        self.o2 = oldstyle()
        self.n2 = newstyle()
    
    def test_basic(self):
        self.o.ind = -2
        self.n.ind = 2
        assert(self.seq[self.n] == 20)
        assert(self.seq[self.o] == 40)
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

class TupleTestCase(unittest.TestCase):
    def setUp(self):
        self.seq = (0,10,20,30,40,50)
        self.o = oldstyle()
        self.n = newstyle()
        self.o2 = oldstyle()
        self.n2 = newstyle()
        
    
    def test_basic(self):
        self.o.ind = -2
        self.n.ind = 2
        assert(self.seq[self.n] == 20)
        assert(self.seq[self.o] == 40)
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

class StringTestCase(unittest.TestCase):
    def setUp(self):
        self.seq = "this is a test"
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


class UnicodeTestCase(unittest.TestCase):
    def setUp(self):
        self.seq = u"this is a test"
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


def test_main():
    test_support.run_unittest(
        ListTestCase,
        TupleTestCase,
        StringTestCase,
        UnicodeTestCase
    )

if __name__ == "__main__":
    test_main()
