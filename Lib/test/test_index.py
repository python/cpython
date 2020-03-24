import unittest
from test import support
from test.support import FakeIndex
import operator
maxsize = support.MAX_Py_ssize_t

class TrapInt(int):
    def __index__(self):
        return int(self)

class BaseTestCase(unittest.TestCase):
    def test_basic(self):
        self.assertEqual(operator.index(FakeIndex(-2)), -2)

    def test_slice(self):
        slc = slice(FakeIndex(2), FakeIndex(2), FakeIndex(2))
        check_slc = slice(2, 2, 2)
        self.assertEqual(slc.indices(FakeIndex(2)), check_slc.indices(2))

    def test_wrappers(self):
        self.assertEqual(6 .__index__(), 6)
        self.assertEqual(-7 .__index__(), -7)
        self.assertEqual(FakeIndex(5).__index__(), 5)
        self.assertEqual(True.__index__(), 1)
        self.assertEqual(False.__index__(), 0)

    def test_subclasses(self):
        r = list(range(10))
        self.assertEqual(r[TrapInt(5):TrapInt(10)], r[5:10])
        self.assertEqual(slice(TrapInt()).indices(0), (0,0,1))

    def test_error(self):
        self.assertRaises(TypeError, operator.index, FakeIndex('bad'))
        self.assertRaises(TypeError, slice(FakeIndex('bad')).indices, 0)

    def test_int_subclass_with_index(self):
        # __index__ should be used when computing indices, even for int
        # subclasses.  See issue #17576.
        class MyInt(int):
            def __index__(self):
                return int(str(self)) + 1

        my_int = MyInt(7)
        direct_index = my_int.__index__()
        operator_index = operator.index(my_int)
        self.assertEqual(direct_index, 8)
        self.assertEqual(operator_index, 7)
        # Both results should be of exact type int.
        self.assertIs(type(direct_index), int)
        #self.assertIs(type(operator_index), int)

    def test_index_returns_int_subclass(self):
        class BadInt2(int):
            def __index__(self):
                return True

        bad_int = FakeIndex(True)
        with self.assertWarns(DeprecationWarning):
            n = operator.index(bad_int)
        self.assertEqual(n, 1)

        bad_int = BadInt2()
        n = operator.index(bad_int)
        self.assertEqual(n, 0)


class SeqTestCase:
    # This test case isn't run directly. It just defines common tests
    # to the different sequence types below

    def test_index(self):
        self.assertEqual(self.seq[FakeIndex(2)], self.seq[2])
        self.assertEqual(self.seq[FakeIndex(-2)], self.seq[-2])

    def test_slice(self):
        self.assertEqual(self.seq[FakeIndex(1):FakeIndex(3)], self.seq[1:3])

    def test_slice_bug7532(self):
        seqlen = len(self.seq)
        self.assertEqual(self.seq[FakeIndex(seqlen + 2):], self.seq[0:0])
        self.assertEqual(self.seq[:FakeIndex(seqlen + 2)], self.seq)
        self.assertEqual(self.seq[FakeIndex(-seqlen - 2):], self.seq)
        self.assertEqual(self.seq[:FakeIndex(-seqlen - 2)], self.seq[0:0])

    def test_repeat(self):
        self.assertEqual(self.seq * FakeIndex(3), self.seq * 3)
        self.assertEqual(FakeIndex(3) * self.seq, self.seq * 3)

    def test_wrappers(self):
        self.assertEqual(self.seq.__getitem__(FakeIndex(4)), self.seq[4])
        self.assertEqual(self.seq.__mul__(FakeIndex(4)), self.seq * 4)
        self.assertEqual(self.seq.__rmul__(FakeIndex(4)), self.seq * 4)

    def test_subclasses(self):
        self.assertEqual(self.seq[TrapInt()], self.seq[0])

    def test_error(self):
        with self.assertRaises(TypeError):
            self.seq[FakeIndex('bad')]
        with self.assertRaises(TypeError):
            self.seq[FakeIndex('bad'):]


class ListTestCase(SeqTestCase, unittest.TestCase):
    seq = [0,10,20,30,40,50]

    def test_setdelitem(self):
        lst = list('ab!cdefghi!j')
        del lst[FakeIndex(-2)]
        del lst[FakeIndex(2)]
        lst[FakeIndex(-2)] = 'X'
        lst[FakeIndex(2)] = 'Y'
        self.assertEqual(lst, list('abYdefghXj'))

        lst = [5, 6, 7, 8, 9, 10, 11]
        lst.__setitem__(FakeIndex(2), "here")
        self.assertEqual(lst, [5, 6, "here", 8, 9, 10, 11])
        lst.__delitem__(FakeIndex(2))
        self.assertEqual(lst, [5, 6, 8, 9, 10, 11])

    def test_inplace_repeat(self):
        lst = [6, 4]
        lst *= FakeIndex(2)
        self.assertEqual(lst, [6, 4, 6, 4])

        lst = [5, 6, 7, 8, 9, 11]
        l2 = lst.__imul__(FakeIndex(2))
        self.assertIs(l2, lst)
        self.assertEqual(lst, [5, 6, 7, 8, 9, 11] * 2)


class NewSeq:

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


class TupleTestCase(SeqTestCase, unittest.TestCase):
    seq = (0,10,20,30,40,50)

class ByteArrayTestCase(SeqTestCase, unittest.TestCase):
    seq = bytearray(b"this is a test")

class BytesTestCase(SeqTestCase, unittest.TestCase):
    seq = b"this is a test"

class StringTestCase(SeqTestCase, unittest.TestCase):
    seq = "this is a test"

class NewSeqTestCase(SeqTestCase, unittest.TestCase):
    seq = NewSeq((0,10,20,30,40,50))



class RangeTestCase(unittest.TestCase):

    def test_range(self):
        n = FakeIndex(5)
        self.assertEqual(range(1, 20)[n], 6)
        self.assertEqual(range(1, 20).__getitem__(n), 6)


class OverflowTestCase(unittest.TestCase):

    def setUp(self):
        self.pos = 2**100
        self.neg = -self.pos

    def test_large_longs(self):
        self.assertEqual(self.pos.__index__(), self.pos)
        self.assertEqual(self.neg.__index__(), self.neg)

    def test_getitem(self):
        class GetItem:
            def __len__(self):
                assert False, "__len__ should not be invoked"
            def __getitem__(self, key):
                return key
        x = GetItem()
        self.assertEqual(x[self.pos], self.pos)
        self.assertEqual(x[self.neg], self.neg)
        self.assertEqual(x[self.neg:self.pos].indices(maxsize),
                         (0, maxsize, 1))
        self.assertEqual(x[self.neg:self.pos:1].indices(maxsize),
                         (0, maxsize, 1))

    def test_sequence_repeat(self):
        self.assertRaises(OverflowError, lambda: "a" * self.pos)
        self.assertRaises(OverflowError, lambda: "a" * self.neg)


if __name__ == "__main__":
    unittest.main()
