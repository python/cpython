# Test specifically-sized containers.

import unittest
from ctypes import *

class SizesTestCase(unittest.TestCase):
    def test_8(self):
        self.failUnlessEqual(1, sizeof(c_int8))
        self.failUnlessEqual(1, sizeof(c_uint8))

    def test_16(self):
        self.failUnlessEqual(2, sizeof(c_int16))
        self.failUnlessEqual(2, sizeof(c_uint16))

    def test_32(self):
        self.failUnlessEqual(4, sizeof(c_int32))
        self.failUnlessEqual(4, sizeof(c_uint32))

    def test_64(self):
        self.failUnlessEqual(8, sizeof(c_int64))
        self.failUnlessEqual(8, sizeof(c_uint64))

    def test_size_t(self):
        self.failUnlessEqual(sizeof(c_void_p), sizeof(c_size_t))

if __name__ == "__main__":
    unittest.main()
