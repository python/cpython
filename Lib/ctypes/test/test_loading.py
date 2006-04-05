from ctypes import *
import sys, unittest
import os, StringIO

libc_name = None
if os.name == "nt":
    libc_name = "msvcrt"
elif os.name == "ce":
    libc_name = "coredll"
elif sys.platform == "darwin":
    libc_name = "libc.dylib"
elif sys.platform == "cygwin":
    libc_name = "cygwin1.dll"
else:
    for line in os.popen("ldd %s" % sys.executable):
        if "libc.so" in line:
            if sys.platform == "openbsd3":
                libc_name = line.split()[4]
            else:
                libc_name = line.split()[2]
##            print "libc_name is", libc_name
            break

class LoaderTest(unittest.TestCase):

    unknowndll = "xxrandomnamexx"

    if libc_name is not None:
        def test_load(self):
            cdll.load(libc_name)
            cdll.load(os.path.basename(libc_name))
            self.assertRaises(OSError, cdll.load, self.unknowndll)

    if libc_name is not None and "libc.so.6" in libc_name:
        def test_load_version(self):
            cdll.load_version("c", "6")
            # linux uses version, libc 9 should not exist
            self.assertRaises(OSError, cdll.load_version, "c", "9")
            self.assertRaises(OSError, cdll.load_version, self.unknowndll, "")

        def test_find(self):
            name = "c"
            cdll.find(name)
            self.assertRaises(OSError, cdll.find, self.unknowndll)

    if os.name in ("nt", "ce"):
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
