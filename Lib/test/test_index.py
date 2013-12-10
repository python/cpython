import unittest
from test import test_support
import operator
from sys import maxint
maxsize = test_support.MAX_Py_ssize_t
minsize = -maxsize-1

class oldstyle:
    def __index__(self):
        return self.ind

class newstyle(object):
    def __index__(self):
        return self.ind

class TrapInt(int):
    def __index__(self):
        return self

class TrapLong(long):
    def __index__(self):
        return self

class BaseTestCase(unittest.TestCase):
    def setUp(self):
        self.o = oldstyle()
        self.n = newstyle()

    def test_basic(self):
        self.o.ind = -2
        self.n.ind = 2
        self.assertEqual(operator.index(self.o), -2)
        self.assertEqual(operator.index(self.n), 2)

    def test_slice(self):
        self.o.ind = 1
        self.n.ind = 2
        slc = slice(self.o, self.o, self.o)
        check_slc = slice(1, 1, 1)
        self.assertEqual(slc.indices(self.o), check_slc.indices(1))
        slc = slice(self.n, self.n, self.n)
        check_slc = slice(2, 2, 2)
        self.assertEqual(slc.indices(self.n), check_slc.indices(2))

    def test_wrappers(self):
        self.o.ind = 4
        self.n.ind = 5
        self.assertEqual(6 .__index__(), 6)
        self.assertEqual(-7L.__index__(), -7)
        self.assertEqual(self.o.__index__(), 4)
        self.assertEqual(self.n.__index__(), 5)
        self.assertEqual(True.__index__(), 1)
        self.assertEqual(False.__index__(), 0)

    def test_subclasses(self):
        r = range(10)
        self.assertEqual(r[TrapInt(5):TrapInt(10)], r[5:10])
        self.assertEqual(r[TrapLong(5):TrapLong(10)], r[5:10])
        self.assertEqual(slice(TrapInt()).indices(0), (0,0,1))
        self.assertEqual(slice(TrapLong(0)).indices(0), (0,0,1))

    def test_error(self):
        self.o.ind = 'dumb'
        self.n.ind = 'bad'
        self.assertRaises(TypeError, operator.index, self.o)
        self.assertRaises(TypeError, operator.index, self.n)
        self.assertRaises(TypeError, slice(self.o).indices, 0)
        self.assertRaises(TypeError, slice(self.n).indices, 0)


class SeqTestCase(unittest.TestCase):
    # This test case isn't run directly. It just defines common tests
    # to the different sequence types below
    def setUp(self):
        self.o = oldstyle()
        self.n = newstyle()
        self.o2 = oldstyle()
        self.n2 = newstyle()

    def test_index(self):
        self.o.ind = -2
        self.n.ind = 2
        self.assertEqual(self.seq[self.n], self.seq[2])
        self.assertEqual(self.seq[self.o], self.seq[-2])

    def test_slice(self):
        self.o.ind = 1
        self.o2.ind = 3
        self.n.ind = 2
        self.n2.ind = 4
        self.assertEqual(self.seq[self.o:self.o2], self.seq[1:3])
        self.assertEqual(self.seq[self.n:self.n2], self.seq[2:4])

    def test_slice_bug7532a(self):
        seqlen = len(self.seq)
        self.o.ind = int(seqlen * 1.5)
        self.n.ind = seqlen + 2
        self.assertEqual(self.seq[self.o:], self.seq[0:0])
        self.assertEqual(self.seq[:self.o], self.seq)
        self.assertEqual(self.seq[self.n:], self.seq[0:0])
        self.assertEqual(self.seq[:self.n], self.seq)

    def test_slice_bug7532b(self):
        if isinstance(self.seq, ClassicSeq):
            self.skipTest('test fails for ClassicSeq')
        # These tests fail for ClassicSeq (see bug #7532)
        seqlen = len(self.seq)
        self.o2.ind = -seqlen - 2
        self.n2.ind = -int(seqlen * 1.5)
        self.assertEqual(self.seq[self.o2:], self.seq)
        self.assertEqual(self.seq[:self.o2], self.seq[0:0])
        self.assertEqual(self.seq[self.n2:], self.seq)
        self.assertEqual(self.seq[:self.n2], self.seq[0:0])

    def test_repeat(self):
        self.o.ind = 3
        self.n.ind = 2
        self.assertEqual(self.seq * self.o, self.seq * 3)
        self.assertEqual(self.seq * self.n, self.seq * 2)
        self.assertEqual(self.o * self.seq, self.seq * 3)
        self.assertEqual(self.n * self.seq, self.seq * 2)

    def test_wrappers(self):
        self.o.ind = 4
        self.n.ind = 5
        self.assertEqual(self.seq.__getitem__(self.o), self.seq[4])
        self.assertEqual(self.seq.__mul__(self.o), self.seq * 4)
        self.assertEqual(self.seq.__rmul__(self.o), self.seq * 4)
        self.assertEqual(self.seq.__getitem__(self.n), self.seq[5])
        self.assertEqual(self.seq.__mul__(self.n), self.seq * 5)
        self.assertEqual(self.seq.__rmul__(self.n), self.seq * 5)

    def test_subclasses(self):
        self.assertEqual(self.seq[TrapInt()], self.seq[0])
        self.assertEqual(self.seq[TrapLong()], self.seq[0])

    def test_error(self):
        self.o.ind = 'dumb'
        self.n.ind = 'bad'
        indexobj = lambda x, obj: obj.seq[x]
        self.assertRaises(TypeError, indexobj, self.o, self)
        self.assertRaises(TypeError, indexobj, self.n, self)
        sliceobj = lambda x, obj: obj.seq[x:]
        self.assertRaises(TypeError, sliceobj, self.o, self)
        self.assertRaises(TypeError, sliceobj, self.n, self)


class ListTestCase(SeqTestCase):
    seq = [0,10,20,30,40,50]

    def test_setdelitem(self):
        self.o.ind = -2
        self.n.ind = 2
        lst = list('ab!cdefghi!j')
        del lst[self.o]
        del lst[self.n]
        lst[self.o] = 'X'
        lst[self.n] = 'Y'
        self.assertEqual(lst, list('abYdefghXj'))

        lst = [5, 6, 7, 8, 9, 10, 11]
        lst.__setitem__(self.n, "here")
        self.assertEqual(lst, [5, 6, "here", 8, 9, 10, 11])
        lst.__delitem__(self.n)
        self.assertEqual(lst, [5, 6, 8, 9, 10, 11])

    def test_inplace_repeat(self):
        self.o.ind = 2
        self.n.ind = 3
        lst = [6, 4]
        lst *= self.o
        self.assertEqual(lst, [6, 4, 6, 4])
        lst *= self.n
        self.assertEqual(lst, [6, 4, 6, 4] * 3)

        lst = [5, 6, 7, 8, 9, 11]
        l2 = lst.__imul__(self.n)
        self.assertIs(l2, lst)
        self.assertEqual(lst, [5, 6, 7, 8, 9, 11] * 3)


class _BaseSeq:

    def __init__(self, iterable):
        self._list = list(iterable)

    def __repr__(self):
        return repr(self._list)

    def __eq__(self, other):
        return self._list == other

    def __len__(self):
        return len(self._list)

    def __mul__(self, n):
        return self.__class__(self._list*n)
    __rmul__ = __mul__

    def __getitem__(self, index):
        return self._list[index]


class _GetSliceMixin:

    def __getslice__(self, i, j):
        return self._list.__getslice__(i, j)


class ClassicSeq(_BaseSeq): pass
class NewSeq(_BaseSeq, object): pass
class ClassicSeqDeprecated(_GetSliceMixin, ClassicSeq): pass
class NewSeqDeprecated(_GetSliceMixin, NewSeq): pass


class TupleTestCase(SeqTestCase):
    seq = (0,10,20,30,40,50)

class StringTestCase(SeqTestCase):
    seq = "this is a test"

class ByteArrayTestCase(SeqTestCase):
    seq = bytearray("this is a test")

class UnicodeTestCase(SeqTestCase):
    seq = u"this is a test"

class ClassicSeqTestCase(SeqTestCase):
    seq = ClassicSeq((0,10,20,30,40,50))

class NewSeqTestCase(SeqTestCase):
    seq = NewSeq((0,10,20,30,40,50))

class ClassicSeqDeprecatedTestCase(SeqTestCase):
    seq = ClassicSeqDeprecated((0,10,20,30,40,50))

class NewSeqDeprecatedTestCase(SeqTestCase):
    seq = NewSeqDeprecated((0,10,20,30,40,50))


class XRangeTestCase(unittest.TestCase):

    def test_xrange(self):
        n = newstyle()
        n.ind = 5
        self.assertEqual(xrange(1, 20)[n], 6)
        self.assertEqual(xrange(1, 20).__getitem__(n), 6)

class OverflowTestCase(unittest.TestCase):

    def setUp(self):
        self.pos = 2**100
        self.neg = -self.pos

    def test_large_longs(self):
        self.assertEqual(self.pos.__index__(), self.pos)
        self.assertEqual(self.neg.__index__(), self.neg)

    def _getitem_helper(self, base):
        class GetItem(base):
            def __len__(self):
                return maxint # cannot return long here
            def __getitem__(self, key):
                return key
        x = GetItem()
        self.assertEqual(x[self.pos], self.pos)
        self.assertEqual(x[self.neg], self.neg)
        self.assertEqual(x[self.neg:self.pos].indices(maxsize),
                         (0, maxsize, 1))
        self.assertEqual(x[self.neg:self.pos:1].indices(maxsize),
                         (0, maxsize, 1))

    def _getslice_helper_deprecated(self, base):
        class GetItem(base):
            def __len__(self):
                return maxint # cannot return long here
            def __getitem__(self, key):
                return key
            def __getslice__(self, i, j):
                return i, j
        x = GetItem()
        self.assertEqual(x[self.pos], self.pos)
        self.assertEqual(x[self.neg], self.neg)
        self.assertEqual(x[self.neg:self.pos], (maxint+minsize, maxsize))
        self.assertEqual(x[self.neg:self.pos:1].indices(maxsize),
                         (0, maxsize, 1))

    def test_getitem(self):
        self._getitem_helper(object)
        with test_support.check_py3k_warnings():
            self._getslice_helper_deprecated(object)

    def test_getitem_classic(self):
        class Empty: pass
        # XXX This test fails (see bug #7532)
        #self._getitem_helper(Empty)
        with test_support.check_py3k_warnings():
            self._getslice_helper_deprecated(Empty)

    def test_sequence_repeat(self):
        self.assertRaises(OverflowError, lambda: "a" * self.pos)
        self.assertRaises(OverflowError, lambda: "a" * self.neg)


def test_main():
    test_support.run_unittest(
        BaseTestCase,
        ListTestCase,
        TupleTestCase,
        ByteArrayTestCase,
        StringTestCase,
        UnicodeTestCase,
        ClassicSeqTestCase,
        NewSeqTestCase,
        XRangeTestCase,
        OverflowTestCase,
    )
    with test_support.check_py3k_warnings():
        test_support.run_unittest(
            ClassicSeqDeprecatedTestCase,
            NewSeqDeprecatedTestCase,
        )


if __name__ == "__main__":
    test_main()
