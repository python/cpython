import os
import sys
import unittest

from test.support import os_helper
from test.support.os_helper import TESTFN, TESTFN_ASCII

if sys.platform != "win32":
    raise unittest.SkipTest("windows related tests")

import _winapi
import msvcrt;

from _testconsole import write_input, flush_console_input_buffer


class TestFileOperations(unittest.TestCase):
    def test_locking(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            msvcrt.locking(f.fileno(), msvcrt.LK_LOCK, 1)
            self.assertRaises(OSError, msvcrt.locking, f.fileno(), msvcrt.LK_NBLCK, 1)

    def test_unlockfile(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            msvcrt.locking(f.fileno(), msvcrt.LK_LOCK, 1)
            msvcrt.locking(f.fileno(), msvcrt.LK_UNLCK, 1)
            msvcrt.locking(f.fileno(), msvcrt.LK_LOCK, 1)

    def test_setmode(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            msvcrt.setmode(f.fileno(), os.O_BINARY)
            msvcrt.setmode(f.fileno(), os.O_TEXT)

    def test_open_osfhandle(self):
        h = _winapi.CreateFile(TESTFN_ASCII, _winapi.GENERIC_WRITE, 0, 0, 1, 128, 0)
        self.addCleanup(os_helper.unlink, TESTFN_ASCII)

        try:
            fd = msvcrt.open_osfhandle(h, os.O_RDONLY)
            h = None
            os.close(fd)
        finally:
            if h:
                _winapi.CloseHandle(h)

    def test_get_osfhandle(self):
        with open(TESTFN, "w") as f:
            self.addCleanup(os_helper.unlink, TESTFN)

            msvcrt.get_osfhandle(f.fileno())


c = '\u5b57'  # unicode CJK char (meaning 'character') for 'wide-char' tests
c_encoded = b'\x57\x5b' # utf-16-le (which windows internally used) encoded char for this CJK char


class TestConsoleIO(unittest.TestCase):
    def test_kbhit(self):
        h = msvcrt.get_osfhandle(sys.stdin.fileno())
        flush_console_input_buffer(h)
        self.assertEqual(msvcrt.kbhit(), 0)

    def test_getch(self):
        msvcrt.ungetch(b'c')
        self.assertEqual(msvcrt.getch(), b'c')

    def test_getwch(self):
        with open('CONIN$', 'rb', buffering=0) as stdin:
            h = msvcrt.get_osfhandle(stdin.fileno())
            flush_console_input_buffer(h)

            write_input(stdin, c_encoded)
            self.assertEqual(msvcrt.getwch(), c)

    def test_getche(self):
        msvcrt.ungetch(b'c')
        self.assertEqual(msvcrt.getche(), b'c')

    def test_getwche(self):
        with open('CONIN$', 'rb', buffering=0) as stdin:
            h = msvcrt.get_osfhandle(stdin.fileno())
            flush_console_input_buffer(h)

            write_input(stdin, c_encoded)
            self.assertEqual(msvcrt.getwche(), c)

    def test_putch(self):
        msvcrt.putch(b'c')

    def test_putwch(self):
        msvcrt.putwch(c)


class TestOther(unittest.TestCase):
    def test_heap_min(self):
        try:
            msvcrt.heapmin()
        except OSError:
            pass


if __name__ == "__main__":
    unittest.main()
