"""
Test the terminal: os.get_terminal_size(), os.openpty(), etc.
"""

import errno
import os
import subprocess
import sys
import textwrap
import unittest
from test import support
from test.support import os_helper
from test.support import warnings_helper


@unittest.skipUnless(hasattr(os, 'get_terminal_size'), "requires os.get_terminal_size")
class TermsizeTests(unittest.TestCase):
    def test_does_not_crash(self):
        """Check if get_terminal_size() returns a meaningful value.

        There's no easy portable way to actually check the size of the
        terminal, so let's check if it returns something sensible instead.
        """
        try:
            size = os.get_terminal_size()
        except OSError as e:
            known_errnos = [errno.EINVAL, errno.ENOTTY]
            if sys.platform == "android":
                # The Android testbed redirects the native stdout to a pipe,
                # which returns a different error code.
                known_errnos.append(errno.EACCES)
            if sys.platform == "win32" or e.errno in known_errnos:
                # Under win32 a generic OSError can be thrown if the
                # handle cannot be retrieved
                self.skipTest("failed to query terminal size")
            raise

        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)

    @support.requires_subprocess()
    def test_stty_match(self):
        """Check if stty returns the same results

        stty actually tests stdin, so get_terminal_size is invoked on
        stdin explicitly. If stty succeeded, then get_terminal_size()
        should work too.
        """
        try:
            size = (
                subprocess.check_output(
                    ["stty", "size"], stderr=subprocess.DEVNULL, text=True
                ).split()
            )
        except (FileNotFoundError, subprocess.CalledProcessError,
                PermissionError):
            self.skipTest("stty invocation failed")
        expected = (int(size[1]), int(size[0])) # reversed order

        try:
            actual = os.get_terminal_size(sys.__stdin__.fileno())
        except OSError as e:
            if sys.platform == "win32" or e.errno in (errno.EINVAL, errno.ENOTTY):
                # Under win32 a generic OSError can be thrown if the
                # handle cannot be retrieved
                self.skipTest("failed to query terminal size")
            raise
        self.assertEqual(expected, actual)

    @unittest.skipUnless(sys.platform == 'win32', 'Windows specific test')
    def test_windows_fd(self):
        """Check if get_terminal_size() returns a meaningful value in Windows"""
        try:
            conout = open('conout$', 'w')
        except OSError:
            self.skipTest('failed to open conout$')
        with conout:
            size = os.get_terminal_size(conout.fileno())

        self.assertGreaterEqual(size.columns, 0)
        self.assertGreaterEqual(size.lines, 0)


@unittest.skipUnless(hasattr(os, 'openpty'), "need os.openpty()")
class PseudoterminalTests(unittest.TestCase):
    def open_pty(self):
        """Open a pty fd-pair, and schedule cleanup for it"""
        main_fd, second_fd = os.openpty()
        self.addCleanup(os.close, main_fd)
        self.addCleanup(os.close, second_fd)
        return main_fd, second_fd

    def test_openpty(self):
        main_fd, second_fd = self.open_pty()
        self.assertEqual(os.get_inheritable(main_fd), False)
        self.assertEqual(os.get_inheritable(second_fd), False)

    @unittest.skipUnless(hasattr(os, 'ptsname'), "need os.ptsname()")
    @unittest.skipUnless(hasattr(os, 'O_RDWR'), "need os.O_RDWR")
    @unittest.skipUnless(hasattr(os, 'O_NOCTTY'), "need os.O_NOCTTY")
    def test_open_via_ptsname(self):
        main_fd, second_fd = self.open_pty()
        second_path = os.ptsname(main_fd)
        reopened_second_fd = os.open(second_path, os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, reopened_second_fd)
        os.write(reopened_second_fd, b'foo')
        self.assertEqual(os.read(main_fd, 3), b'foo')

    @unittest.skipUnless(hasattr(os, 'posix_openpt'), "need os.posix_openpt()")
    @unittest.skipUnless(hasattr(os, 'grantpt'), "need os.grantpt()")
    @unittest.skipUnless(hasattr(os, 'unlockpt'), "need os.unlockpt()")
    @unittest.skipUnless(hasattr(os, 'ptsname'), "need os.ptsname()")
    @unittest.skipUnless(hasattr(os, 'O_RDWR'), "need os.O_RDWR")
    @unittest.skipUnless(hasattr(os, 'O_NOCTTY'), "need os.O_NOCTTY")
    def test_posix_pty_functions(self):
        mother_fd = os.posix_openpt(os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, mother_fd)
        os.grantpt(mother_fd)
        os.unlockpt(mother_fd)
        son_path = os.ptsname(mother_fd)
        son_fd = os.open(son_path, os.O_RDWR|os.O_NOCTTY)
        self.addCleanup(os.close, son_fd)
        self.assertEqual(os.ptsname(mother_fd), os.ttyname(son_fd))

    @warnings_helper.ignore_fork_in_thread_deprecation_warnings()
    @unittest.skipUnless(hasattr(os, 'spawnl'), "need os.spawnl()")
    @support.requires_subprocess()
    def test_pipe_spawnl(self):
        # gh-77046: On Windows, os.pipe() file descriptors must be created with
        # _O_NOINHERIT to make them non-inheritable. UCRT has no public API to
        # get (_osfile(fd) & _O_NOINHERIT), so use a functional test.
        #
        # Make sure that fd is not inherited by a child process created by
        # os.spawnl(): get_osfhandle() and dup() must fail with EBADF.

        fd, fd2 = os.pipe()
        self.addCleanup(os.close, fd)
        self.addCleanup(os.close, fd2)

        code = textwrap.dedent(f"""
            import errno
            import os
            import test.support
            try:
                import msvcrt
            except ImportError:
                msvcrt = None

            fd = {fd}

            with test.support.SuppressCrashReport():
                if msvcrt is not None:
                    try:
                        handle = msvcrt.get_osfhandle(fd)
                    except OSError as exc:
                        if exc.errno != errno.EBADF:
                            raise
                        # get_osfhandle(fd) failed with EBADF as expected
                    else:
                        raise Exception("get_osfhandle() must fail")

                try:
                    fd3 = os.dup(fd)
                except OSError as exc:
                    if exc.errno != errno.EBADF:
                        raise
                    # os.dup(fd) failed with EBADF as expected
                else:
                    os.close(fd3)
                    raise Exception("dup must fail")
        """)

        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, os_helper.TESTFN)
        with open(filename, "w") as fp:
            print(code, file=fp, end="")

        executable = sys.executable
        cmd = [executable, filename]
        if os.name == "nt" and " " in cmd[0]:
            cmd[0] = f'"{cmd[0]}"'
        exitcode = os.spawnl(os.P_WAIT, executable, *cmd)
        self.assertEqual(exitcode, 0)


if __name__ == "__main__":
    unittest.main()
