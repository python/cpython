from ctypes import *
import sys, unittest
import os, StringIO

class LoaderTest(unittest.TestCase):

    unknowndll = "xxrandomnamexx"

    def test_load(self):
        if os.name == "nt":
            name = "msvcrt"
        elif os.name == "ce":
            name = "coredll"
        elif sys.platform == "darwin":
            name = "libc.dylib"
        elif sys.platform.startswith("freebsd"):
            name = "libc.so"
        elif sys.platform in ("sunos5", "osf1V5"):
            name = "libc.so"
        elif sys.platform.startswith("netbsd") or sys.platform.startswith("openbsd"):
            name = "libc.so"
        else:
            name = "libc.so.6"
##        print (sys.platform, os.name)
        try:
            cdll.load(name)
        except Exception, details:
            self.fail((str(details), name, (os.name, sys.platform)))
        self.assertRaises(OSError, cdll.load, self.unknowndll)

    def test_load_version(self):
        version = "6"
        name = "c"
        if sys.platform == "linux2":
            cdll.load_version(name, version)
            # linux uses version, libc 9 should not exist
            self.assertRaises(OSError, cdll.load_version, name, "9")
        self.assertRaises(OSError, cdll.load_version, self.unknowndll, "")

    if os.name == "posix" and sys.platform != "sunos5":
        def test_find(self):
            name = "c"
            cdll.find(name)
            self.assertRaises(OSError, cdll.find, self.unknowndll)

    def test_load_library(self):
        if os.name == "nt":
            windll.load_library("kernel32").GetModuleHandleW
            windll.LoadLibrary("kernel32").GetModuleHandleW
            WinDLL("kernel32").GetModuleHandleW
        elif os.name == "ce":
            windll.load_library("coredll").GetModuleHandleW
            windll.LoadLibrary("coredll").GetModuleHandleW
            WinDLL("coredll").GetModuleHandleW

    def test_load_ordinal_functions(self):
        if os.name in ("nt", "ce"):
            import _ctypes_test
            dll = WinDLL(_ctypes_test.__file__)
            # We load the same function both via ordinal and name
            func_ord = dll[2]
            func_name = dll.GetString
            # addressof gets the address where the function pointer is stored
            a_ord = addressof(func_ord)
            a_name = addressof(func_name)
            f_ord_addr = c_void_p.from_address(a_ord).value
            f_name_addr = c_void_p.from_address(a_name).value
            self.failUnlessEqual(hex(f_ord_addr), hex(f_name_addr))

            self.failUnlessRaises(AttributeError, dll.__getitem__, 1234)

if __name__ == "__main__":
    unittest.main()
