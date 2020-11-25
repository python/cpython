from test.support import verbose, reap_children
from test.support.import_helper import import_module

# Skip these tests if termios is not available
import_module('termios')

import errno
import pty
import os
import sys
import select
import signal
import socket
import io # readline
import unittest

import struct
import tty
import fcntl
import platform
import warnings

TEST_STRING_1 = b"I wish to buy a fish license.\n"
TEST_STRING_2 = b"For my pet fish, Eric.\n"

try:
    _TIOCGWINSZ = tty.TIOCGWINSZ
    _TIOCSWINSZ = tty.TIOCSWINSZ
    _HAVE_WINSZ = True
except AttributeError:
    _HAVE_WINSZ = False

if verbose:
    def debug(msg):
        print(msg)
else:
    def debug(msg):
        pass


# Note that os.read() is nondeterministic so we need to be very careful
# to make the test suite deterministic.  A normal call to os.read() may
# give us less than expected.
#
# Beware, on my Linux system, if I put 'foo\n' into a terminal fd, I get
# back 'foo\r\n' at the other end.  The behavior depends on the termios
# setting.  The newline translation may be OS-specific.  To make the
# test suite deterministic and OS-independent, the functions _readline
# and normalize_output can be used.

def normalize_output(data):
    # Some operating systems do conversions on newline.  We could possibly fix
    # that by doing the appropriate termios.tcsetattr()s.  I couldn't figure out
    # the right combo on Tru64.  So, just normalize the output and doc the
    # problem O/Ses by allowing certain combinations for some platforms, but
    # avoid allowing other differences (like extra whitespace, trailing garbage,
    # etc.)

    # This is about the best we can do without getting some feedback
    # from someone more knowledgable.

    # OSF/1 (Tru64) apparently turns \n into \r\r\n.
    if data.endswith(b'\r\r\n'):
        return data.replace(b'\r\r\n', b'\n')

    if data.endswith(b'\r\n'):
        return data.replace(b'\r\n', b'\n')

    return data

def _readline(fd):
    """Read one line.  May block forever if no newline is read."""
    reader = io.FileIO(fd, mode='rb', closefd=False)
    return reader.readline()

def expectedFailureIfStdinIsTTY(fun):
    # avoid isatty() for now
    try:
        tty.tcgetattr(pty.STDIN_FILENO)
        return unittest.expectedFailure(fun)
    except tty.error:
        pass
    return fun

def expectedFailureOnBSD(fun):
    PLATFORM = platform.system()
    if PLATFORM.endswith("BSD") or PLATFORM == "Darwin":
        return unittest.expectedFailure(fun)
    return fun

def _get_term_winsz(fd):
    s = struct.pack("HHHH", 0, 0, 0, 0)
    return fcntl.ioctl(fd, _TIOCGWINSZ, s)

def _set_term_winsz(fd, winsz):
    fcntl.ioctl(fd, _TIOCSWINSZ, winsz)


# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.
# XXX(nnorwitz):  these tests leak fds when there is an error.
class PtyTest(unittest.TestCase):
    def setUp(self):
        old_alarm = signal.signal(signal.SIGALRM, self.handle_sig)
        self.addCleanup(signal.signal, signal.SIGALRM, old_alarm)

        old_sighup = signal.signal(signal.SIGHUP, self.handle_sighup)
        self.addCleanup(signal.signal, signal.SIGHUP, old_sighup)

        # isatty() and close() can hang on some platforms. Set an alarm
        # before running the test to make sure we don't hang forever.
        self.addCleanup(signal.alarm, 0)
        signal.alarm(10)

        # Save original stdin window size
        self.stdin_rows = None
        self.stdin_cols = None
        if _HAVE_WINSZ:
            try:
                stdin_dim = os.get_terminal_size(pty.STDIN_FILENO)
                self.stdin_rows = stdin_dim.lines
                self.stdin_cols = stdin_dim.columns
                old_stdin_winsz = struct.pack("HHHH", self.stdin_rows,
                                              self.stdin_cols, 0, 0)
                self.addCleanup(_set_term_winsz, pty.STDIN_FILENO, old_stdin_winsz)
            except OSError:
                pass

    def handle_sig(self, sig, frame):
        self.fail("isatty hung")

    @staticmethod
    def handle_sighup(signum, frame):
        # bpo-38547: if the process is the session leader, os.close(master_fd)
        # of "master_fd, slave_name = pty.master_open()" raises SIGHUP
        # signal: just ignore the signal.
        #
        # NOTE: the above comment is from an older version of the test;
        # master_open() is not being used anymore.
        pass

    @expectedFailureIfStdinIsTTY
    def test_openpty(self):
        try:
            mode = tty.tcgetattr(pty.STDIN_FILENO)
        except tty.error:
            # not a tty or bad/closed fd
            debug("tty.tcgetattr(pty.STDIN_FILENO) failed")
            mode = None

        new_stdin_winsz = None
        if self.stdin_rows != None and self.stdin_cols != None:
            try:
                debug("Setting pty.STDIN_FILENO window size")
                # Set number of columns and rows to be the
                # floors of 1/5 of respective original values
                target_stdin_winsz = struct.pack("HHHH", self.stdin_rows//5,
                                                 self.stdin_cols//5, 0, 0)
                _set_term_winsz(pty.STDIN_FILENO, target_stdin_winsz)

                # Were we able to set the window size
                # of pty.STDIN_FILENO successfully?
                new_stdin_winsz = _get_term_winsz(pty.STDIN_FILENO)
                self.assertEqual(new_stdin_winsz, target_stdin_winsz,
                                 "pty.STDIN_FILENO window size unchanged")
            except OSError:
                warnings.warn("Failed to set pty.STDIN_FILENO window size")
                pass

        try:
            debug("Calling pty.openpty()")
            try:
                master_fd, slave_fd = pty.openpty(mode, new_stdin_winsz)
            except TypeError:
                master_fd, slave_fd = pty.openpty()
            debug(f"Got master_fd '{master_fd}', slave_fd '{slave_fd}'")
        except OSError:
            # " An optional feature could not be imported " ... ?
            raise unittest.SkipTest("Pseudo-terminals (seemingly) not functional.")

        self.assertTrue(os.isatty(slave_fd), "slave_fd is not a tty")

        if mode:
            self.assertEqual(tty.tcgetattr(slave_fd), mode,
                             "openpty() failed to set slave termios")
        if new_stdin_winsz:
            self.assertEqual(_get_term_winsz(slave_fd), new_stdin_winsz,
                             "openpty() failed to set slave window size")

        # Solaris requires reading the fd before anything is returned.
        # My guess is that since we open and close the slave fd
        # in master_open(), we need to read the EOF.
        #
        # NOTE: the above comment is from an older version of the test;
        # master_open() is not being used anymore.

        # Ensure the fd is non-blocking in case there's nothing to read.
        blocking = os.get_blocking(master_fd)
        try:
            os.set_blocking(master_fd, False)
            try:
                s1 = os.read(master_fd, 1024)
                self.assertEqual(b'', s1)
            except OSError as e:
                if e.errno != errno.EAGAIN:
                    raise
        finally:
            # Restore the original flags.
            os.set_blocking(master_fd, blocking)

        debug("Writing to slave_fd")
        os.write(slave_fd, TEST_STRING_1)
        s1 = _readline(master_fd)
        self.assertEqual(b'I wish to buy a fish license.\n',
                         normalize_output(s1))

        debug("Writing chunked output")
        os.write(slave_fd, TEST_STRING_2[:5])
        os.write(slave_fd, TEST_STRING_2[5:])
        s2 = _readline(master_fd)
        self.assertEqual(b'For my pet fish, Eric.\n', normalize_output(s2))

        os.close(slave_fd)
        # closing master_fd can raise a SIGHUP if the process is
        # the session leader: we installed a SIGHUP signal handler
        # to ignore this signal.
        os.close(master_fd)

    def test_fork(self):
        debug("calling pty.fork()")
        pid, master_fd = pty.fork()
        if pid == pty.CHILD:
            # stdout should be connected to a tty.
            if not os.isatty(1):
                debug("Child's fd 1 is not a tty?!")
                os._exit(3)

            # After pty.fork(), the child should already be a session leader.
            # (on those systems that have that concept.)
            debug("In child, calling os.setsid()")
            try:
                os.setsid()
            except OSError:
                # Good, we already were session leader
                debug("Good: OSError was raised.")
                pass
            except AttributeError:
                # Have pty, but not setsid()?
                debug("No setsid() available?")
                pass
            except:
                # We don't want this error to propagate, escaping the call to
                # os._exit() and causing very peculiar behavior in the calling
                # regrtest.py !
                # Note: could add traceback printing here.
                debug("An unexpected error was raised.")
                os._exit(1)
            else:
                debug("os.setsid() succeeded! (bad!)")
                os._exit(2)
            os._exit(4)
        else:
            debug("Waiting for child (%d) to finish." % pid)
            # In verbose mode, we have to consume the debug output from the
            # child or the child will block, causing this test to hang in the
            # parent's waitpid() call.  The child blocks after a
            # platform-dependent amount of data is written to its fd.  On
            # Linux 2.6, it's 4000 bytes and the child won't block, but on OS
            # X even the small writes in the child above will block it.  Also
            # on Linux, the read() will raise an OSError (input/output error)
            # when it tries to read past the end of the buffer but the child's
            # already exited, so catch and discard those exceptions.  It's not
            # worth checking for EIO.
            while True:
                try:
                    data = os.read(master_fd, 80)
                except OSError:
                    break
                if not data:
                    break
                sys.stdout.write(str(data.replace(b'\r\n', b'\n'),
                                     encoding='ascii'))

            ##line = os.read(master_fd, 80)
            ##lines = line.replace('\r\n', '\n').split('\n')
            ##if False and lines != ['In child, calling os.setsid()',
            ##             'Good: OSError was raised.', '']:
            ##    raise TestFailed("Unexpected output from child: %r" % line)

            (pid, status) = os.waitpid(pid, 0)
            res = os.waitstatus_to_exitcode(status)
            debug("Child (%d) exited with code %d (status %d)." % (pid, res, status))
            if res == 1:
                self.fail("Child raised an unexpected exception in os.setsid()")
            elif res == 2:
                self.fail("pty.fork() failed to make child a session leader.")
            elif res == 3:
                self.fail("Child spawned by pty.fork() did not have a tty as stdout")
            elif res != 4:
                self.fail("pty.fork() failed for unknown reasons.")

            ##debug("Reading from master_fd now that the child has exited")
            ##try:
            ##    s1 = os.read(master_fd, 1024)
            ##except OSError:
            ##    pass
            ##else:
            ##    raise TestFailed("Read from master_fd did not raise exception")

        os.close(master_fd)

    @expectedFailureOnBSD
    def test_master_read(self):
        debug("Calling pty.openpty()")
        master_fd, slave_fd = pty.openpty()
        debug(f"Got master_fd '{master_fd}', slave_fd '{slave_fd}'")

        debug("Closing slave_fd")
        os.close(slave_fd)

        debug("Reading from master_fd")
        with self.assertRaises(OSError):
            os.read(master_fd, 1)

        os.close(master_fd)

class SmallPtyTests(unittest.TestCase):
    """These tests don't spawn children or hang."""

    def setUp(self):
        self.orig_stdin_fileno = pty.STDIN_FILENO
        self.orig_stdout_fileno = pty.STDOUT_FILENO
        self.orig_pty_select = pty.select
        self.fds = []  # A list of file descriptors to close.
        self.files = []
        self.select_rfds_lengths = []
        self.select_rfds_results = []

    def tearDown(self):
        pty.STDIN_FILENO = self.orig_stdin_fileno
        pty.STDOUT_FILENO = self.orig_stdout_fileno
        pty.select = self.orig_pty_select
        for file in self.files:
            try:
                file.close()
            except OSError:
                pass
        for fd in self.fds:
            try:
                os.close(fd)
            except OSError:
                pass

    def _pipe(self):
        pipe_fds = os.pipe()
        self.fds.extend(pipe_fds)
        return pipe_fds

    def _socketpair(self):
        socketpair = socket.socketpair()
        self.files.extend(socketpair)
        return socketpair

    def _mock_select(self, rfds, wfds, xfds, timeout=0):
        # This will raise IndexError when no more expected calls exist.
        # This ignores the timeout
        self.assertEqual(self.select_rfds_lengths.pop(0), len(rfds))
        return self.select_rfds_results.pop(0), [], []

    def test__copy_to_each(self):
        """Test the normal data case on both master_fd and stdin."""
        read_from_stdout_fd, mock_stdout_fd = self._pipe()
        pty.STDOUT_FILENO = mock_stdout_fd
        mock_stdin_fd, write_to_stdin_fd = self._pipe()
        pty.STDIN_FILENO = mock_stdin_fd
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # Feed data.  Smaller than PIPEBUF.  These writes will not block.
        os.write(masters[1], b'from master')
        os.write(write_to_stdin_fd, b'from stdin')

        # Expect two select calls, the last one will cause IndexError
        pty.select = self._mock_select
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([mock_stdin_fd, masters[0]])
        self.select_rfds_lengths.append(2)

        with self.assertRaises(IndexError):
            pty._copy(masters[0])

        # Test that the right data went to the right places.
        rfds = select.select([read_from_stdout_fd, masters[1]], [], [], 0)[0]
        self.assertEqual([read_from_stdout_fd, masters[1]], rfds)
        self.assertEqual(os.read(read_from_stdout_fd, 20), b'from master')
        self.assertEqual(os.read(masters[1], 20), b'from stdin')

    def test__copy_eof_on_all(self):
        """Test the empty read EOF case on both master_fd and stdin."""
        read_from_stdout_fd, mock_stdout_fd = self._pipe()
        pty.STDOUT_FILENO = mock_stdout_fd
        mock_stdin_fd, write_to_stdin_fd = self._pipe()
        pty.STDIN_FILENO = mock_stdin_fd
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        socketpair[1].close()
        os.close(write_to_stdin_fd)

        # Expect two select calls, the last one will cause IndexError
        pty.select = self._mock_select
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([mock_stdin_fd, masters[0]])
        # We expect that both fds were removed from the fds list as they
        # both encountered an EOF before the second select call.
        self.select_rfds_lengths.append(0)

        with self.assertRaises(IndexError):
            pty._copy(masters[0])


def tearDownModule():
    reap_children()

if __name__ == "__main__":
    unittest.main()
