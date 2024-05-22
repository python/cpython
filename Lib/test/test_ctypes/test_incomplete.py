import ctypes
import unittest
import warnings
from ctypes import Structure, POINTER, pointer, c_char_p


# The incomplete pointer example from the tutorial
class TestSetPointerType(unittest.TestCase):
    def tearDown(self):
        # to not leak references, we must clean _pointer_type_cache
        ctypes._reset_cache()

    def test_incomplete_example(self):
        lpcell = POINTER("cell")
        class cell(Structure):
            _fields_ = [("name", c_char_p),
                        ("next", lpcell)]

        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            ctypes.SetPointerType(lpcell, cell)

        c1 = cell()
        c1.name = b"foo"
        c2 = cell()
        c2.name = b"bar"

        c1.next = pointer(c2)
        c2.next = pointer(c1)

        p = c1

        result = []
        for i in range(8):
            result.append(p.name)
            p = p.next[0]
        self.assertEqual(result, [b"foo", b"bar"] * 4)

    def test_deprecation(self):
        lpcell = POINTER("cell")
        class cell(Structure):
            _fields_ = [("name", c_char_p),
                        ("next", lpcell)]

        with self.assertWarns(DeprecationWarning):
            ctypes.SetPointerType(lpcell, cell)


if __name__ == '__main__':
    unittest.main()
