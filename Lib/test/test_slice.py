# tests for slice objects; in particular the indices method.

import unittest
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
        self.assertRaises(TypeError, slice(5).__hash__)

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
        self.assert_(s.stop is obj)

    def test_indices(self):
        self.assertEqual(slice(None           ).indices(10), (0, 10,  1))
        self.assertEqual(slice(None,  None,  2).indices(10), (0, 10,  2))
        self.assertEqual(slice(1,     None,  2).indices(10), (1, 10,  2))
        self.assertEqual(slice(None,  None, -1).indices(10), (9, -1, -1))
        self.assertEqual(slice(None,  None, -2).indices(10), (9, -1, -2))
        self.assertEqual(slice(3,     None, -2).indices(10), (3, -1, -2))
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

def test_main():
    test_support.run_unittest(SliceTest)

if __name__ == "__main__":
    test_main()
