import ctypes
import errno
import os
import threading
import unittest
from ctypes import CDLL, c_int, c_char_p, c_wchar_p, get_errno, set_errno
from ctypes.util import find_library


class Test(unittest.TestCase):
    def test_open(self):
        libc_name = find_library("c")
        if libc_name is None:
            self.skipTest("Unable to find C library")

        libc = CDLL(libc_name, use_errno=True)
        if os.name == "nt":
            libc_open = libc._open
        else:
            libc_open = libc.open

        libc_open.argtypes = c_char_p, c_int

        self.assertEqual(libc_open(b"", 0), -1)
        self.assertEqual(get_errno(), errno.ENOENT)

        self.assertEqual(set_errno(32), errno.ENOENT)
        self.assertEqual(get_errno(), 32)

        def _worker():
            set_errno(0)

            libc = CDLL(libc_name, use_errno=False)
            if os.name == "nt":
                libc_open = libc._open
            else:
                libc_open = libc.open
            libc_open.argtypes = c_char_p, c_int
            self.assertEqual(libc_open(b"", 0), -1)
            self.assertEqual(get_errno(), 0)

        t = threading.Thread(target=_worker)
        t.start()
        t.join()

        self.assertEqual(get_errno(), 32)
        set_errno(0)

    @unittest.skipUnless(os.name == "nt", 'Test specific to Windows')
    def test_GetLastError(self):
        dll = ctypes.WinDLL("kernel32", use_last_error=True)
        GetModuleHandle = dll.GetModuleHandleA
        GetModuleHandle.argtypes = [c_wchar_p]

        self.assertEqual(0, GetModuleHandle("foo"))
        self.assertEqual(ctypes.get_last_error(), 126)

        self.assertEqual(ctypes.set_last_error(32), 126)
        self.assertEqual(ctypes.get_last_error(), 32)

        def _worker():
            ctypes.set_last_error(0)

            dll = ctypes.WinDLL("kernel32", use_last_error=False)
            GetModuleHandle = dll.GetModuleHandleW
            GetModuleHandle.argtypes = [c_wchar_p]
            GetModuleHandle("bar")

            self.assertEqual(ctypes.get_last_error(), 0)

        t = threading.Thread(target=_worker)
        t.start()
        t.join()

        self.assertEqual(ctypes.get_last_error(), 32)

        ctypes.set_last_error(0)


if __name__ == "__main__":
    unittest.main()
