"""Unit tests for the bytes type."""

import sys
import unittest
import test.test_support


class BytesTest(unittest.TestCase):

    def test_basics(self):
        b = bytes()
        self.assertEqual(type(b), bytes)
        self.assertEqual(b.__class__, bytes)

    def test_empty_sequence(self):
        b = bytes()
        self.assertEqual(len(b), 0)
        self.assertRaises(IndexError, lambda: b[0])
        self.assertRaises(IndexError, lambda: b[1])
        self.assertRaises(IndexError, lambda: b[sys.maxint])
        self.assertRaises(IndexError, lambda: b[sys.maxint+1])
        self.assertRaises(IndexError, lambda: b[10**100])
        self.assertRaises(IndexError, lambda: b[-1])
        self.assertRaises(IndexError, lambda: b[-2])
        self.assertRaises(IndexError, lambda: b[-sys.maxint])
        self.assertRaises(IndexError, lambda: b[-sys.maxint-1])
        self.assertRaises(IndexError, lambda: b[-sys.maxint-2])
        self.assertRaises(IndexError, lambda: b[-10**100])

    def test_from_list(self):
        ints = list(range(256))
        b = bytes(i for i in ints)
        self.assertEqual(len(b), 256)
        self.assertEqual(list(b), ints)

    def test_from_index(self):
        class C:
            def __init__(self, i=0):
                self.i = i
            def __index__(self):
                return self.i
        b = bytes([C(), C(1), C(254), C(255)])
        self.assertEqual(list(b), [0, 1, 254, 255])
        self.assertRaises(ValueError, lambda: bytes([C(-1)]))
        self.assertRaises(ValueError, lambda: bytes([C(256)]))

    def test_constructor_type_errors(self):
        class C:
            pass
        self.assertRaises(TypeError, lambda: bytes(["0"]))
        self.assertRaises(TypeError, lambda: bytes([0.0]))
        self.assertRaises(TypeError, lambda: bytes([None]))
        self.assertRaises(TypeError, lambda: bytes([C()]))

    def test_constructor_value_errors(self):
        self.assertRaises(ValueError, lambda: bytes([-1]))
        self.assertRaises(ValueError, lambda: bytes([-sys.maxint]))
        self.assertRaises(ValueError, lambda: bytes([-sys.maxint-1]))
        self.assertRaises(ValueError, lambda: bytes([-sys.maxint-2]))
        self.assertRaises(ValueError, lambda: bytes([-10**100]))
        self.assertRaises(ValueError, lambda: bytes([256]))
        self.assertRaises(ValueError, lambda: bytes([257]))
        self.assertRaises(ValueError, lambda: bytes([sys.maxint]))
        self.assertRaises(ValueError, lambda: bytes([sys.maxint+1]))
        self.assertRaises(ValueError, lambda: bytes([10**100]))

    def test_repr(self):
        self.assertEqual(repr(bytes()), "bytes()")
        self.assertEqual(repr(bytes([0])), "bytes([0x00])")
        self.assertEqual(repr(bytes([0, 1, 254, 255])), "bytes([0x00, 0x01, 0xfe, 0xff])")

    def test_compare(self):
        b1 = bytes([1, 2, 3])
        b2 = bytes([1, 2, 3])
        b3 = bytes([1, 3])

        self.failUnless(b1 == b2)
        self.failUnless(b2 != b3)
        self.failUnless(b1 <= b2)
        self.failUnless(b1 <= b3)
        self.failUnless(b1 <  b3)
        self.failUnless(b1 >= b2)
        self.failUnless(b3 >= b2)
        self.failUnless(b3 >  b2)

        self.failIf(b1 != b2)
        self.failIf(b2 == b3)
        self.failIf(b1 >  b2)
        self.failIf(b1 >  b3)
        self.failIf(b1 >= b3)
        self.failIf(b1 <  b2)
        self.failIf(b3 <  b2)
        self.failIf(b3 <= b2)

    def test_nohash(self):
        self.assertRaises(TypeError, hash, bytes())

    def test_doc(self):
        self.failUnless(bytes.__doc__ != None)
        self.failUnless(bytes.__doc__.startswith("bytes("))


def test_main():
    test.test_support.run_unittest(BytesTest)


if __name__ == "__main__":
    ##test_main()
    unittest.main()
