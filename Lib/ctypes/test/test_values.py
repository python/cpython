"""
A testcase which accesses *values* in a dll.
"""

import unittest
import sys
from ctypes import *

import _ctypes_test

class ValuesTestCase(unittest.TestCase):

    def test_an_integer(self):
        ctdll = CDLL(_ctypes_test.__file__)
        an_integer = c_int.in_dll(ctdll, "an_integer")
        x = an_integer.value
        self.assertEqual(x, ctdll.get_an_integer())
        an_integer.value *= 2
        self.assertEqual(x*2, ctdll.get_an_integer())

    def test_undefined(self):
        ctdll = CDLL(_ctypes_test.__file__)
        self.assertRaises(ValueError, c_int.in_dll, ctdll, "Undefined_Symbol")

@unittest.skipUnless(sys.platform == 'win32', 'Windows-specific test')
class Win_ValuesTestCase(unittest.TestCase):
    """This test only works when python itself is a dll/shared library"""

    def test_optimizeflag(self):
        # This test accesses the Py_OptimizeFlag intger, which is
        # exported by the Python dll.

        # It's value is set depending on the -O and -OO flags:
        # if not given, it is 0 and __debug__ is 1.
        # If -O is given, the flag is 1, for -OO it is 2.
        # docstrings are also removed in the latter case.
        opt = c_int.in_dll(pythonapi, "Py_OptimizeFlag").value
        if __debug__:
            self.assertEqual(opt, 0)
        elif ValuesTestCase.__doc__ is not None:
            self.assertEqual(opt, 1)
        else:
            self.assertEqual(opt, 2)

    def test_frozentable(self):
        # Python exports a PyImport_FrozenModules symbol. This is a
        # pointer to an array of struct _frozen entries.  The end of the
        # array is marked by an entry containing a NULL name and zero
        # size.

        # In standard Python, this table contains a __hello__
        # module, and a __phello__ package containing a spam
        # module.
        class struct_frozen(Structure):
            _fields_ = [("name", c_char_p),
                        ("code", POINTER(c_ubyte)),
                        ("size", c_int)]
        FrozenTable = POINTER(struct_frozen)

        ft = FrozenTable.in_dll(pythonapi, "PyImport_FrozenModules")
        # ft is a pointer to the struct_frozen entries:
        items = []
        for entry in ft:
            # This is dangerous. We *can* iterate over a pointer, but
            # the loop will not terminate (maybe with an access
            # violation;-) because the pointer instance has no size.
            if entry.name is None:
                break
            items.append((entry.name, entry.size))

        expected = [("__hello__", 104),
                    ("__phello__", -104),
                    ("__phello__.spam", 104)]
        self.assertEqual(items, expected)

        from ctypes import _pointer_type_cache
        del _pointer_type_cache[struct_frozen]

    def test_undefined(self):
        self.assertRaises(ValueError, c_int.in_dll, pythonapi,
                          "Undefined_Symbol")

if __name__ == '__main__':
    unittest.main()
