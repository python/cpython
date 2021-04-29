"""
A testcase which accesses *values* in a dll.
"""

import unittest
import sys
from ctypes import *

import _ctypes_test

class ValuesTestCase(unittest.TestCase):

    def test_an_integer(self):
        # This test checks and changes an integer stored inside the
        # _ctypes_test dll/shared lib.
        ctdll = CDLL(_ctypes_test.__file__)
        an_integer = c_int.in_dll(ctdll, "an_integer")
        x = an_integer.value
        self.assertEqual(x, ctdll.get_an_integer())
        an_integer.value *= 2
        self.assertEqual(x*2, ctdll.get_an_integer())
        # To avoid test failures when this test is repeated several
        # times the original value must be restored
        an_integer.value = x
        self.assertEqual(x, ctdll.get_an_integer())

    def test_undefined(self):
        ctdll = CDLL(_ctypes_test.__file__)
        self.assertRaises(ValueError, c_int.in_dll, ctdll, "Undefined_Symbol")

class PythonValuesTestCase(unittest.TestCase):
    """This test only works when python itself is a dll/shared library"""

    def test_optimizeflag(self):
        # This test accesses the Py_OptimizeFlag integer, which is
        # exported by the Python dll and should match the sys.flags value

        opt = c_int.in_dll(pythonapi, "Py_OptimizeFlag").value
        self.assertEqual(opt, sys.flags.optimize)

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
        # _frozen_importlib changes size whenever importlib._bootstrap
        # changes, so it gets a special case.  We should make sure it's
        # found, but don't worry about its size too much.  The same
        # applies to _frozen_importlib_external.
        bootstrap_seen = []
        bootstrap_expected = [
                b'_frozen_importlib',
                b'_frozen_importlib_external',
                b'zipimport',
                ]
        for entry in ft:
            # This is dangerous. We *can* iterate over a pointer, but
            # the loop will not terminate (maybe with an access
            # violation;-) because the pointer instance has no size.
            if entry.name is None:
                break

            if entry.name in bootstrap_expected:
                bootstrap_seen.append(entry.name)
                self.assertTrue(entry.size,
                    "{!r} was reported as having no size".format(entry.name))
                continue
            items.append((entry.name.decode("ascii"), entry.size))

        expected = [("__hello__", 137),
                    ("__phello__", -137),
                    ("__phello__.spam", 137),
                    ]
        self.assertEqual(items, expected, "PyImport_FrozenModules example "
            "in Doc/library/ctypes.rst may be out of date")

        self.assertEqual(sorted(bootstrap_seen), bootstrap_expected,
            "frozen bootstrap modules did not match PyImport_FrozenModules")

        from ctypes import _pointer_type_cache
        del _pointer_type_cache[struct_frozen]

    def test_undefined(self):
        self.assertRaises(ValueError, c_int.in_dll, pythonapi,
                          "Undefined_Symbol")

if __name__ == '__main__':
    unittest.main()
