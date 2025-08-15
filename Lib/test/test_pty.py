import unittest
from test.support import (
    is_android, is_apple_mobile, is_wasm32, reap_children, verbose
)
from test.support.import_helper import import_module
from test.support.os_helper import TESTFN, unlink

# Skip these tests if termios is not available
import_module('termios')

if is_android or is_apple_mobile or is_wasm32:
    raise unittest.SkipTest("pty is not available on this platform")

import errno
import os
import pty
import tty
import sys
import select
import signal
import socket
import io # readline

TEST_STRING_1 = b"I wish to buy a fish license.\n"
TEST_STRING_2 = b"For my pet fish, Eric.\n"

_HAVE_WINSZ = hasattr(tty, "TIOCGWINSZ") and hasattr(tty, "TIOCSWINSZ")

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
    # avoid isatty()
    try:
        tty.tcgetattr(pty.STDIN_FILENO)
        return unittest.expectedFailure(fun)
    except tty.error:
        pass
    return fun


def write_all(fd, data):
    written = os.write(fd, data)
    if written != len(data):
        # gh-73256, gh-110673: It should never happen, but check just in case
        raise Exception(f"short write: os.write({fd}, {len(data)} bytes) "
                        f"wrote {written} bytes")


# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.
class PtyTest(unittest.TestCase):
    def setUp(self):
        old_sighup = signal.signal(signal.SIGHUP, self.handle_sighup)
        self.addCleanup(signal.signal, signal.SIGHUP, old_sighup)

        # Save original stdin window size.
        self.stdin_dim = None
        if _HAVE_WINSZ:
            try:
                self.stdin_dim = tty.tcgetwinsize(pty.STDIN_FILENO)
                self.addCleanup(tty.tcsetwinsize, pty.STDIN_FILENO,
                                self.stdin_dim)
            except tty.error:
                pass

    @staticmethod
    def handle_sighup(signum, frame):
        pass

    @expectedFailureIfStdinIsTTY
    def test_openpty(self):
        try:
            mode = tty.tcgetattr(pty.STDIN_FILENO)
        except tty.error:
            # Not a tty or bad/closed fd.
            debug("tty.tcgetattr(pty.STDIN_FILENO) failed")
            mode = None

        new_dim = None
        if self.stdin_dim:
            try:
                # Modify pty.STDIN_FILENO window size; we need to
                # check if pty.openpty() is able to set pty slave
                # window size accordingly.
                debug("Setting pty.STDIN_FILENO window size.")
                debug(f"original size: (row, col) = {self.stdin_dim}")
                target_dim = (self.stdin_dim[0] + 1, self.stdin_dim[1] + 1)
                debug(f"target size: (row, col) = {target_dim}")
                tty.tcsetwinsize(pty.STDIN_FILENO, target_dim)

                # Were we able to set the window size
                # of pty.STDIN_FILENO successfully?
                new_dim = tty.tcgetwinsize(pty.STDIN_FILENO)
                self.assertEqual(new_dim, target_dim,
                                 "pty.STDIN_FILENO window size unchanged")
            except OSError as e:
                logging.getLogger(__name__).warning(
                    "Failed to set pty.STDIN_FILENO window size.", exc_info=e,
                )
                pass

        try:
            debug("Calling pty.openpty()")
            try:
                master_fd, slave_fd, slave_name = pty.openpty(mode, new_dim,
                                                              True)
            except TypeError:
                master_fd, slave_fd = pty.openpty()
                slave_name = None
            debug(f"Got {master_fd=}, {slave_fd=}, {slave_name=}")
        except OSError:
            # " An optional feature could not be imported " ... ?
            raise unittest.SkipTest("Pseudo-terminals (seemingly) not functional.")

        # closing master_fd can raise a SIGHUP if the process is
        # the session leader: we installed a SIGHUP signal handler
        # to ignore this signal.
        self.addCleanup(os.close, master_fd)
        self.addCleanup(os.close, slave_fd)

        self.assertTrue(os.isatty(slave_fd), "slave_fd is not a tty")

        if mode:
            self.assertEqual(tty.tcgetattr(slave_fd), mode,
                             "openpty() failed to set slave termios")
        if new_dim:
            self.assertEqual(tty.tcgetwinsize(slave_fd), new_dim,
                             "openpty() failed to set slave window size")

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
        write_all(slave_fd, TEST_STRING_1)
        s1 = _readline(master_fd)
        self.assertEqual(b'I wish to buy a fish license.\n',
                         normalize_output(s1))

        debug("Writing chunked output")
        write_all(slave_fd, TEST_STRING_2[:5])
        write_all(slave_fd, TEST_STRING_2[5:])
        s2 = _readline(master_fd)
        self.assertEqual(b'For my pet fish, Eric.\n', normalize_output(s2))

    def test_fork(self):
        debug("calling pty.fork()")
        pid, master_fd = pty.fork()
        self.addCleanup(os.close, master_fd)
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

    def test_master_read(self):
        # XXX(nnorwitz):  this test leaks fds when there is an error.
        debug("Calling pty.openpty()")
        master_fd, slave_fd = pty.openpty()
        debug(f"Got master_fd '{master_fd}', slave_fd '{slave_fd}'")

        self.addCleanup(os.close, master_fd)

        debug("Closing slave_fd")
        os.close(slave_fd)

        debug("Reading from master_fd")
        try:
            data = os.read(master_fd, 1)
        except OSError: # Linux
            data = b""

        self.assertEqual(data, b"")

    def test_spawn_doesnt_hang(self):
        self.addCleanup(unlink, TESTFN)
        with open(TESTFN, 'wb') as f:
            STDOUT_FILENO = 1
            dup_stdout = os.dup(STDOUT_FILENO)
            os.dup2(f.fileno(), STDOUT_FILENO)
            buf = b''
            def master_read(fd):
                nonlocal buf
                data = os.read(fd, 1024)
                buf += data
                return data
            try:
                pty.spawn([sys.executable, '-c', 'print("hi there")'],
                          master_read)
            finally:
                os.dup2(dup_stdout, STDOUT_FILENO)
                os.close(dup_stdout)
        self.assertEqual(buf, b'hi there\r\n')
        with open(TESTFN, 'rb') as f:
            self.assertEqual(f.read(), b'hi there\r\n')

class SmallPtyTests(unittest.TestCase):
    """These tests don't spawn children or hang."""

    def setUp(self):
        self.orig_stdin_fileno = pty.STDIN_FILENO
        self.orig_stdout_fileno = pty.STDOUT_FILENO
        self.orig_pty_close = pty.close
        self.orig_pty__copy = pty._copy
        self.orig_pty_fork = pty.fork
        self.orig_pty_select = pty.select
        self.orig_pty_setraw = pty.setraw
        self.orig_pty_tcgetattr = pty.tcgetattr
        self.orig_pty_tcsetattr = pty.tcsetattr
        self.orig_pty_waitpid = pty.waitpid
        self.fds = []  # A list of file descriptors to close.
        self.files = []
        self.select_input = []
        self.select_output = []
        self.tcsetattr_mode_setting = None

    def tearDown(self):
        pty.STDIN_FILENO = self.orig_stdin_fileno
        pty.STDOUT_FILENO = self.orig_stdout_fileno
        pty.close = self.orig_pty_close
        pty._copy = self.orig_pty__copy
        pty.fork = self.orig_pty_fork
        pty.select = self.orig_pty_select
        pty.setraw = self.orig_pty_setraw
        pty.tcgetattr = self.orig_pty_tcgetattr
        pty.tcsetattr = self.orig_pty_tcsetattr
        pty.waitpid = self.orig_pty_waitpid
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

    def _mock_select(self, rfds, wfds, xfds):
        # This will raise IndexError when no more expected calls exist.
        self.assertEqual((rfds, wfds, xfds), self.select_input.pop(0))
        return self.select_output.pop(0)

    def _make_mock_fork(self, pid):
        def mock_fork():
            return (pid, 12)
        return mock_fork

    def _mock_tcsetattr(self, fileno, opt, mode):
        self.tcsetattr_mode_setting = mode

    def test__copy_to_each(self):
        """Test the normal data case on both master_fd and stdin."""
        read_from_stdout_fd, mock_stdout_fd = self._pipe()
        pty.STDOUT_FILENO = mock_stdout_fd
        mock_stdin_fd, write_to_stdin_fd = self._pipe()
        pty.STDIN_FILENO = mock_stdin_fd
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # Feed data.  Smaller than PIPEBUF.  These writes will not block.
        write_all(masters[1], b'from master')
        write_all(write_to_stdin_fd, b'from stdin')

        # Expect three select calls, the last one will cause IndexError
        pty.select = self._mock_select
        self.select_input.append(([mock_stdin_fd, masters[0]], [], []))
        self.select_output.append(([mock_stdin_fd, masters[0]], [], []))
        self.select_input.append(([mock_stdin_fd, masters[0]], [mock_stdout_fd, masters[0]], []))
        self.select_output.append(([], [mock_stdout_fd, masters[0]], []))
        self.select_input.append(([mock_stdin_fd, masters[0]], [], []))

        with self.assertRaises(IndexError):
            pty._copy(masters[0])

        # Test that the right data went to the right places.
        rfds = select.select([read_from_stdout_fd, masters[1]], [], [], 0)[0]
        self.assertEqual([read_from_stdout_fd, masters[1]], rfds)
        self.assertEqual(os.read(read_from_stdout_fd, 20), b'from master')
        self.assertEqual(os.read(masters[1], 20), b'from stdin')

    def test__restore_tty_mode_normal_return(self):
        """Test that spawn resets the tty mode no when _copy returns normally."""

        # PID 1 is returned from mocked fork to run the parent branch
        # of code
        pty.fork = self._make_mock_fork(1)

        status_sentinel = object()
        pty.waitpid = lambda _1, _2: [None, status_sentinel]
        pty.close = lambda _: None

        pty._copy = lambda _1, _2, _3: None

        mode_sentinel = object()
        pty.tcgetattr = lambda fd: mode_sentinel
        pty.tcsetattr = self._mock_tcsetattr
        pty.setraw = lambda _: None

        self.assertEqual(pty.spawn([]), status_sentinel, "pty.waitpid process status not returned by pty.spawn")
        self.assertEqual(self.tcsetattr_mode_setting, mode_sentinel, "pty.tcsetattr not called with original mode value")


def tearDownModule():
    reap_children()


if __name__ == "__main__":
    unittest.main()
