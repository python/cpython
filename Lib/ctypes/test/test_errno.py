import unittest, os, errno
from ctypes import *
from ctypes.util import find_library
import threading

class Test(unittest.TestCase):
    def test_open(self):
        libc_name = find_library("c")
        if libc_name is not None:
            libc = CDLL(libc_name, use_errno=True)
            if os.name == "nt":
                libc_open = libc._open
            else:
                libc_open = libc.open

            libc_open.argtypes = c_char_p, c_int

            self.failUnlessEqual(libc_open("", 0), -1)
            self.failUnlessEqual(get_errno(), errno.ENOENT)

            self.failUnlessEqual(set_errno(32), errno.ENOENT)
            self.failUnlessEqual(get_errno(), 32)


            def _worker():
                set_errno(0)

                libc = CDLL(libc_name, use_errno=False)
                if os.name == "nt":
                    libc_open = libc._open
                else:
                    libc_open = libc.open
                libc_open.argtypes = c_char_p, c_int
                self.failUnlessEqual(libc_open("", 0), -1)
                self.failUnlessEqual(get_errno(), 0)

            t = threading.Thread(target=_worker)
            t.start()
            t.join()

            self.failUnlessEqual(get_errno(), 32)
            set_errno(0)

    if os.name == "nt":

        def test_GetLastError(self):
            dll = WinDLL("kernel32", use_last_error=True)
            GetModuleHandle = dll.GetModuleHandleA
            GetModuleHandle.argtypes = [c_wchar_p]

            self.failUnlessEqual(0, GetModuleHandle("foo"))
            self.failUnlessEqual(get_last_error(), 126)

            self.failUnlessEqual(set_last_error(32), 126)
            self.failUnlessEqual(get_last_error(), 32)

            def _worker():
                set_last_error(0)

                dll = WinDLL("kernel32", use_last_error=False)
                GetModuleHandle = dll.GetModuleHandleW
                GetModuleHandle.argtypes = [c_wchar_p]
                GetModuleHandle("bar")

                self.failUnlessEqual(get_last_error(), 0)

            t = threading.Thread(target=_worker)
            t.start()
            t.join()

            self.failUnlessEqual(get_last_error(), 32)

            set_last_error(0)

if __name__ == "__main__":
    unittest.main()
