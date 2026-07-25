import os
import subprocess
import sys
import unittest
from textwrap import dedent

from test import support
from test.support import os_helper, requires_resource
from test.support.os_helper import TESTFN, TESTFN_ASCII

if sys.platform != "win32":
    raise unittest.SkipTest("windows related tests")

import _winapi
import msvcrt


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


def has_console():
    # A process created without a console (for example by pythonw.exe, or with
    # the DETACHED_PROCESS creation flag) cannot write to the console.
    try:
        with open('CONOUT$', 'w'):
            return True
    except OSError:
        return False


requires_console = unittest.skipUnless(has_console(), 'requires a console')


class TestConsoleIO(unittest.TestCase):
    # CREATE_NEW_CONSOLE creates a "popup" window.
    @requires_resource('gui')
    def run_in_separated_process(self, code):
        # Run test in a separated process to avoid stdin conflicts.
        # See: gh-110147
        cmd = [sys.executable, '-c', code]
        try:
            subprocess.run(cmd, check=True, capture_output=True,
                           creationflags=subprocess.CREATE_NEW_CONSOLE)
        except subprocess.CalledProcessError as exc:
            support.skip_on_low_desktop_heap_memory_subprocess(exc.returncode)
            raise

    def test_kbhit(self):
        code = dedent('''
            import msvcrt
            assert msvcrt.kbhit() == 0
        ''')
        self.run_in_separated_process(code)

    def test_getch(self):
        msvcrt.ungetch(b'c')
        self.assertEqual(msvcrt.getch(), b'c')

    def check_getwch(self, funcname):
        code = dedent(f'''
            import msvcrt
            from _testconsole import write_input
            with open("CONIN$", "rb", buffering=0) as stdin:
                write_input(stdin, {ascii(c_encoded)})
                assert msvcrt.{funcname}() == "{c}"
        ''')
        self.run_in_separated_process(code)

    def test_getwch(self):
        self.check_getwch('getwch')

    def test_getche(self):
        msvcrt.ungetch(b'c')
        self.assertEqual(msvcrt.getche(), b'c')

    def test_getwche(self):
        self.check_getwch('getwche')

    @requires_console
    def test_putch(self):
        msvcrt.putch(b'c')

    @requires_console
    def test_putwch(self):
        msvcrt.putwch(c)

    def test_putch_without_console(self):
        # gh-69573: putch() and putwch() must report the error instead of
        # silently ignoring it when the process has no console attached.
        code = dedent('''
            import msvcrt
            import sys

            for name, arg in (('putch', b'c'), ('putwch', 'c')):
                func = getattr(msvcrt, name)
                try:
                    func(arg)
                except OSError:
                    pass
                else:
                    sys.exit(f'msvcrt.{name}() did not raise OSError')
        ''')
        # DETACHED_PROCESS: the child process is created without a console.
        proc = subprocess.run([sys.executable, '-c', code],
                              creationflags=subprocess.DETACHED_PROCESS,
                              capture_output=True, text=True)
        self.assertEqual(proc.returncode, 0, proc.stderr)


class TestOther(unittest.TestCase):
    def test_heap_min(self):
        try:
            msvcrt.heapmin()
        except OSError:
            pass


if __name__ == "__main__":
    unittest.main()
