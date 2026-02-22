import ctypes
import unittest
from ctypes import Structure, POINTER, pointer, c_char_p, c_int

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

        lpcell.set_type(cell)

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

        lpcell.set_type(cell)
        self.assertIs(POINTER(cell), lpcell)

    def test_set_type_updates_format(self):
        # gh-142966: set_type should update StgInfo.format
        # to match the element type's format
        with self.assertWarns(DeprecationWarning):
            lp = POINTER("node")

        class node(Structure):
            _fields_ = [("value", c_int)]

        # Get the expected format before set_type
        node_format = memoryview(node()).format
        expected_format = "&" + node_format

        lp.set_type(node)

        # Create instance to check format via memoryview
        n = node(42)
        p = lp(n)
        actual_format = memoryview(p).format

        # After set_type, the pointer's format should be "&<element_format>"
        self.assertEqual(actual_format, expected_format)


if __name__ == '__main__':
    unittest.main()
