# Test specifically-sized containers.

from ctypes import *

import unittest


class SizesTestCase(unittest.TestCase):
    def test_8(self):
        self.assertEqual(1, sizeof(c_int8))
        self.assertEqual(1, sizeof(c_uint8))

    def test_16(self):
        self.assertEqual(2, sizeof(c_int16))
        self.assertEqual(2, sizeof(c_uint16))

    def test_32(self):
        self.assertEqual(4, sizeof(c_int32))
        self.assertEqual(4, sizeof(c_uint32))

    def test_64(self):
        self.assertEqual(8, sizeof(c_int64))
        self.assertEqual(8, sizeof(c_uint64))

    def test_size_t(self):
        self.assertEqual(sizeof(c_void_p), sizeof(c_size_t))

    def test_ssize_t(self):
        self.assertEqual(sizeof(c_void_p), sizeof(c_ssize_t))

    def test_time_t(self):
        self.assertEqual(sizeof(c_time_t), SIZEOF_TIME_T)


if __name__ == "__main__":
    unittest.main()
