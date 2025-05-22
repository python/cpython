import ctypes
import unittest
import warnings
from ctypes import Structure, POINTER, pointer, c_char_p

# String-based "incomplete pointers" were implemented in ctypes 0.6.3 (2003, when
# ctypes was an external project). They made obsolete by the current
# incomplete *types* (setting `_fields_` late) in 0.9.5 (2005).
# ctypes was added to Python 2.5 (2006), without any mention in docs.

# This tests incomplete pointer example from the old tutorial
# (https://svn.python.org/projects/ctypes/tags/release_0_6_3/ctypes/docs/tutorial.stx)
class TestSetPointerType(unittest.TestCase):
    def tearDown(self):
        ctypes._pointer_type_cache_fallback.clear()

    def test_incomplete_example(self):
        with self.assertWarns(DeprecationWarning):
            lpcell = POINTER("cell")
        class cell(Structure):
            _fields_ = [("name", c_char_p),
                        ("next", lpcell)]

        with warnings.catch_warnings():
            warnings.simplefilter('ignore', DeprecationWarning)
            ctypes.SetPointerType(lpcell, cell)

        self.assertIs(POINTER(cell), lpcell)

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
        with self.assertWarns(DeprecationWarning):
            lpcell = POINTER("cell")
        class cell(Structure):
            _fields_ = [("name", c_char_p),
                        ("next", lpcell)]

        with self.assertWarns(DeprecationWarning):
            ctypes.SetPointerType(lpcell, cell)

        self.assertIs(POINTER(cell), lpcell)

if __name__ == '__main__':
    unittest.main()
