import _ctypes
import ctypes
import os
import shutil
import subprocess
import sys
import test.support
import unittest
from ctypes import CDLL, cdll, addressof, c_void_p, c_char_p
from ctypes.util import find_library
from test.support import import_helper, os_helper
_ctypes_test = import_helper.import_module("_ctypes_test")


libc_name = None


def setUpModule():
    global libc_name
    if os.name == "nt":
        libc_name = find_library("c")
    elif sys.platform == "cygwin":
        libc_name = "cygwin1.dll"
    else:
        libc_name = find_library("c")

    if test.support.verbose:
        print("libc_name is", libc_name)


class LoaderTest(unittest.TestCase):

    unknowndll = "xxrandomnamexx"

    def test_load(self):
        if libc_name is not None:
            test_lib = libc_name
        else:
            if os.name == "nt":
                test_lib = _ctypes_test.__file__
            else:
                self.skipTest('could not find library to load')
        CDLL(test_lib)
        CDLL(os.path.basename(test_lib))
        CDLL(os_helper.FakePath(test_lib))
        self.assertRaises(OSError, CDLL, self.unknowndll)

    def test_load_version(self):
        if libc_name is None:
            self.skipTest('could not find libc')
        if os.path.basename(libc_name) != 'libc.so.6':
            self.skipTest('wrong libc path for test')
        cdll.LoadLibrary("libc.so.6")
        # linux uses version, libc 9 should not exist
        self.assertRaises(OSError, cdll.LoadLibrary, "libc.so.9")
        self.assertRaises(OSError, cdll.LoadLibrary, self.unknowndll)

    def test_find(self):
        found = False
        for name in ("c", "m"):
            lib = find_library(name)
            if lib:
                found = True
                cdll.LoadLibrary(lib)
                CDLL(lib)
        if not found:
            self.skipTest("Could not find c and m libraries")

    @unittest.skipUnless(os.name == "nt",
                         'test specific to Windows')
    def test_load_library(self):
        # CRT is no longer directly loadable. See issue23606 for the
        # discussion about alternative approaches.
        #self.assertIsNotNone(libc_name)
        if test.support.verbose:
            print(find_library("kernel32"))
            print(find_library("user32"))

        if os.name == "nt":
            ctypes.windll.kernel32.GetModuleHandleW
            ctypes.windll["kernel32"].GetModuleHandleW
            ctypes.windll.LoadLibrary("kernel32").GetModuleHandleW
            ctypes.WinDLL("kernel32").GetModuleHandleW
            # embedded null character
            self.assertRaises(ValueError, ctypes.windll.LoadLibrary, "kernel32\0")

    @unittest.skipUnless(os.name == "nt",
                         'test specific to Windows')
    def test_load_ordinal_functions(self):
        dll = ctypes.WinDLL(_ctypes_test.__file__)
        # We load the same function both via ordinal and name
        func_ord = dll[2]
        func_name = dll.GetString
        # addressof gets the address where the function pointer is stored
        a_ord = addressof(func_ord)
        a_name = addressof(func_name)
        f_ord_addr = c_void_p.from_address(a_ord).value
        f_name_addr = c_void_p.from_address(a_name).value
        self.assertEqual(hex(f_ord_addr), hex(f_name_addr))

        self.assertRaises(AttributeError, dll.__getitem__, 1234)

    @unittest.skipUnless(os.name == "nt", 'Windows-specific test')
    def test_1703286_A(self):
        # On winXP 64-bit, advapi32 loads at an address that does
        # NOT fit into a 32-bit integer.  FreeLibrary must be able
        # to accept this address.

        # These are tests for https://bugs.python.org/issue1703286
        handle = _ctypes.LoadLibrary("advapi32")
        _ctypes.FreeLibrary(handle)

    @unittest.skipUnless(os.name == "nt", 'Windows-specific test')
    def test_1703286_B(self):
        # Since on winXP 64-bit advapi32 loads like described
        # above, the (arbitrarily selected) CloseEventLog function
        # also has a high address.  'call_function' should accept
        # addresses so large.

        advapi32 = ctypes.windll.advapi32
        # Calling CloseEventLog with a NULL argument should fail,
        # but the call should not segfault or so.
        self.assertEqual(0, advapi32.CloseEventLog(None))

        kernel32 = ctypes.windll.kernel32
        kernel32.GetProcAddress.argtypes = c_void_p, c_char_p
        kernel32.GetProcAddress.restype = c_void_p
        proc = kernel32.GetProcAddress(advapi32._handle, b"CloseEventLog")
        self.assertTrue(proc)

        # This is the real test: call the function via 'call_function'
        self.assertEqual(0, _ctypes.call_function(proc, (None,)))

    @unittest.skipUnless(os.name == "nt",
                         'test specific to Windows')
    def test_load_hasattr(self):
        # bpo-34816: shouldn't raise OSError
        self.assertNotHasAttr(ctypes.windll, 'test')

    @unittest.skipUnless(os.name == "nt",
                         'test specific to Windows')
    def test_load_dll_with_flags(self):
        _sqlite3 = import_helper.import_module("_sqlite3")
        src = _sqlite3.__file__
        if os.path.basename(src).partition(".")[0].lower().endswith("_d"):
            ext = "_d.dll"
        else:
            ext = ".dll"

        with os_helper.temp_dir() as tmp:
            # We copy two files and load _sqlite3.dll (formerly .pyd),
            # which has a dependency on sqlite3.dll. Then we test
            # loading it in subprocesses to avoid it starting in memory
            # for each test.
            target = os.path.join(tmp, "_sqlite3.dll")
            shutil.copy(src, target)
            shutil.copy(os.path.join(os.path.dirname(src), "sqlite3" + ext),
                        os.path.join(tmp, "sqlite3" + ext))

            def should_pass(command):
                with self.subTest(command):
                    subprocess.check_output(
                        [sys.executable, "-c",
                         "from ctypes import *; import nt;" + command],
                        cwd=tmp
                    )

            def should_fail(command):
                with self.subTest(command):
                    with self.assertRaises(subprocess.CalledProcessError):
                        subprocess.check_output(
                            [sys.executable, "-c",
                             "from ctypes import *; import nt;" + command],
                            cwd=tmp, stderr=subprocess.STDOUT,
                        )

            # Default load should not find this in CWD
            should_fail("WinDLL('_sqlite3.dll')")

            # Relative path (but not just filename) should succeed
            should_pass("WinDLL('./_sqlite3.dll')")

            # Insecure load flags should succeed
            # Clear the DLL directory to avoid safe search settings propagating
            should_pass("windll.kernel32.SetDllDirectoryW(None); WinDLL('_sqlite3.dll', winmode=0)")

            # Full path load without DLL_LOAD_DIR shouldn't find dependency
            should_fail("WinDLL(nt._getfullpathname('_sqlite3.dll'), " +
                        "winmode=nt._LOAD_LIBRARY_SEARCH_SYSTEM32)")

            # Full path load with DLL_LOAD_DIR should succeed
            should_pass("WinDLL(nt._getfullpathname('_sqlite3.dll'), " +
                        "winmode=nt._LOAD_LIBRARY_SEARCH_SYSTEM32|" +
                        "nt._LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR)")

            # User-specified directory should succeed
            should_pass("import os; p = os.add_dll_directory(os.getcwd());" +
                        "WinDLL('_sqlite3.dll'); p.close()")


if __name__ == "__main__":
    unittest.main()
