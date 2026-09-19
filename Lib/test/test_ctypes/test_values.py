"""
A testcase which accesses *values* in a dll.
"""

import _imp
import ctypes
import importlib.util
import sys
import unittest
from ctypes import (Structure, CDLL, POINTER, pythonapi,
                    c_ubyte, c_char_p, c_int)
from test import support
from test.support import import_helper, thread_unsafe


class LibffiVersionTest(unittest.TestCase):

    def _test_libffi_version(self, v, string):
        self.assertIsInstance(v[:], tuple)
        self.assertEqual(len(v), 3)
        self.assertIsInstance(v[0], int)
        self.assertIsInstance(v[1], int)
        self.assertIsInstance(v[2], int)
        self.assertIsInstance(v.major, int)
        self.assertIsInstance(v.minor, int)
        self.assertIsInstance(v.patch, int)
        self.assertEqual(v[0], v.major)
        self.assertEqual(v[1], v.minor)
        self.assertEqual(v[2], v.patch)
        self.assertGreaterEqual(v.major, 3)
        self.assertGreaterEqual(v.minor, 0)
        self.assertGreaterEqual(v.patch, 0)
        self.assertEqual(string, '%d.%d.%d' % v)

    @unittest.skipUnless(hasattr(ctypes, 'LIBFFI_VERSION_INFO'),
                         'requires libffi >= 3.5')
    def test_libffi_version(self):
        if support.verbose:
            print(f'LIBFFI_VERSION = {ctypes.LIBFFI_VERSION}', flush=True)
            print(f'libffi_version = {ctypes.libffi_version}', flush=True)
            print(f'LIBFFI_VERSION_INFO = {ctypes.LIBFFI_VERSION_INFO}', flush=True)
            print(f'libffi_version_info = {ctypes.libffi_version_info}', flush=True)
        self._test_libffi_version(ctypes.LIBFFI_VERSION_INFO, ctypes.LIBFFI_VERSION)
        self._test_libffi_version(ctypes.libffi_version_info, ctypes.libffi_version)
        self.assertEqual(ctypes.LIBFFI_VERSION_INFO[0], ctypes.libffi_version_info[0])


class ValuesTestCase(unittest.TestCase):

    def setUp(self):
        _ctypes_test = import_helper.import_module("_ctypes_test")
        self.ctdll = CDLL(_ctypes_test.__file__)

    @thread_unsafe("static global variables aren't thread-safe")
    def test_an_integer(self):
        # This test checks and changes an integer stored inside the
        # _ctypes_test dll/shared lib.
        ctdll = self.ctdll
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
        self.assertRaises(ValueError, c_int.in_dll, self.ctdll, "Undefined_Symbol")


class PythonValuesTestCase(unittest.TestCase):
    """This test only works when python itself is a dll/shared library"""

    def test_optimizeflag(self):
        # This test accesses the Py_OptimizeFlag integer, which is
        # exported by the Python dll and should match the sys.flags value

        opt = c_int.in_dll(pythonapi, "Py_OptimizeFlag").value
        self.assertEqual(opt, sys.flags.optimize)

    @thread_unsafe('overrides frozen modules')
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
                        ("size", c_int),
                        ("is_package", c_int),
                        ]
        FrozenTable = POINTER(struct_frozen)

        modules = []
        for group in ["Bootstrap", "Stdlib", "Test"]:
            ft = FrozenTable.in_dll(pythonapi, f"_PyImport_Frozen{group}")
            # ft is a pointer to the struct_frozen entries:
            for entry in ft:
                # This is dangerous. We *can* iterate over a pointer, but
                # the loop will not terminate (maybe with an access
                # violation;-) because the pointer instance has no size.
                if entry.name is None:
                    break
                modname = entry.name.decode("ascii")
                modules.append(modname)
                with self.subTest(modname):
                    if entry.size != 0:
                        # Do a sanity check on entry.size and entry.code.
                        self.assertGreater(abs(entry.size), 10)
                        self.assertTrue([entry.code[i] for i in range(abs(entry.size))])
                    # Check the module's package-ness.
                    with import_helper.frozen_modules():
                        spec = importlib.util.find_spec(modname)
                    if entry.is_package:
                        # It's a package.
                        self.assertIsNotNone(spec.submodule_search_locations)
                    else:
                        self.assertIsNone(spec.submodule_search_locations)

        with import_helper.frozen_modules():
            expected = _imp._frozen_module_names()
        self.maxDiff = None
        self.assertEqual(modules, expected,
                         "_PyImport_FrozenBootstrap example "
                         "in Doc/library/ctypes.rst may be out of date")

    def test_undefined(self):
        self.assertRaises(ValueError, c_int.in_dll, pythonapi,
                          "Undefined_Symbol")


if __name__ == '__main__':
    unittest.main()
