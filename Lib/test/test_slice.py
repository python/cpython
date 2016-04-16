# tests for slice objects; in particular the indices method.

import unittest
import weakref

from cPickle import loads, dumps
from test import test_support

import sys

class SliceTest(unittest.TestCase):

    def test_constructor(self):
        self.assertRaises(TypeError, slice)
        self.assertRaises(TypeError, slice, 1, 2, 3, 4)

    def test_repr(self):
        self.assertEqual(repr(slice(1, 2, 3)), "slice(1, 2, 3)")

    def test_hash(self):
        # Verify clearing of SF bug #800796
        self.assertRaises(TypeError, hash, slice(5))
        with self.assertRaises(TypeError):
            slice(5).__hash__()

    def test_cmp(self):
        s1 = slice(1, 2, 3)
        s2 = slice(1, 2, 3)
        s3 = slice(1, 2, 4)
        self.assertEqual(s1, s2)
        self.assertNotEqual(s1, s3)

        class Exc(Exception):
            pass

        class BadCmp(object):
            def __eq__(self, other):
                raise Exc
            __hash__ = None # Silence Py3k warning

        s1 = slice(BadCmp())
        s2 = slice(BadCmp())
        self.assertRaises(Exc, cmp, s1, s2)
        self.assertEqual(s1, s1)

        s1 = slice(1, BadCmp())
        s2 = slice(1, BadCmp())
        self.assertEqual(s1, s1)
        self.assertRaises(Exc, cmp, s1, s2)

        s1 = slice(1, 2, BadCmp())
        s2 = slice(1, 2, BadCmp())
        self.assertEqual(s1, s1)
        self.assertRaises(Exc, cmp, s1, s2)

    def test_members(self):
        s = slice(1)
        self.assertEqual(s.start, None)
        self.assertEqual(s.stop, 1)
        self.assertEqual(s.step, None)

        s = slice(1, 2)
        self.assertEqual(s.start, 1)
        self.assertEqual(s.stop, 2)
        self.assertEqual(s.step, None)

        s = slice(1, 2, 3)
        self.assertEqual(s.start, 1)
        self.assertEqual(s.stop, 2)
        self.assertEqual(s.step, 3)

        class AnyClass:
            pass

        obj = AnyClass()
        s = slice(obj)
        self.assertTrue(s.stop is obj)

    def test_indices(self):
        self.assertEqual(slice(None           ).indices(10), (0, 10,  1))
        self.assertEqual(slice(None,  None,  2).indices(10), (0, 10,  2))
        self.assertEqual(slice(1,     None,  2).indices(10), (1, 10,  2))
        self.assertEqual(slice(None,  None, -1).indices(10), (9, -1, -1))
        self.assertEqual(slice(None,  None, -2).indices(10), (9, -1, -2))
        self.assertEqual(slice(3,     None, -2).indices(10), (3, -1, -2))
        # issue 3004 tests
        self.assertEqual(slice(None, -9).indices(10), (0, 1, 1))
        self.assertEqual(slice(None, -10).indices(10), (0, 0, 1))
        self.assertEqual(slice(None, -11).indices(10), (0, 0, 1))
        self.assertEqual(slice(None, -10, -1).indices(10), (9, 0, -1))
        self.assertEqual(slice(None, -11, -1).indices(10), (9, -1, -1))
        self.assertEqual(slice(None, -12, -1).indices(10), (9, -1, -1))
        self.assertEqual(slice(None, 9).indices(10), (0, 9, 1))
        self.assertEqual(slice(None, 10).indices(10), (0, 10, 1))
        self.assertEqual(slice(None, 11).indices(10), (0, 10, 1))
        self.assertEqual(slice(None, 8, -1).indices(10), (9, 8, -1))
        self.assertEqual(slice(None, 9, -1).indices(10), (9, 9, -1))
        self.assertEqual(slice(None, 10, -1).indices(10), (9, 9, -1))

        self.assertEqual(
            slice(-100,  100     ).indices(10),
            slice(None).indices(10)
        )
        self.assertEqual(
            slice(100,  -100,  -1).indices(10),
            slice(None, None, -1).indices(10)
        )
        self.assertEqual(slice(-100L, 100L, 2L).indices(10), (0, 10,  2))

        self.assertEqual(range(10)[::sys.maxint - 1], [0])

        self.assertRaises(OverflowError, slice(None).indices, 1L<<100)

    def test_setslice_without_getslice(self):
        tmp = []
        class X(object):
            def __setslice__(self, i, j, k):
                tmp.append((i, j, k))

        x = X()
        with test_support.check_py3k_warnings():
            x[1:2] = 42
        self.assertEqual(tmp, [(1, 2, 42)])

    def test_pickle(self):
        s = slice(10, 20, 3)
        for protocol in (0,1,2):
            t = loads(dumps(s, protocol))
            self.assertEqual(s, t)
            self.assertEqual(s.indices(15), t.indices(15))
            self.assertNotEqual(id(s), id(t))

    def test_cycle(self):
        class myobj(): pass
        o = myobj()
        o.s = slice(o)
        w = weakref.ref(o)
        o = None
        test_support.gc_collect()
        self.assertIsNone(w())

def test_main():
    test_support.run_unittest(SliceTest)

if __name__ == "__main__":
    test_main()
