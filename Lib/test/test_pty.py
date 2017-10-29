from test.support import verbose, import_module, reap_children

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

TEST_STRING_1 = b"I wish to buy a fish license.\n"
TEST_STRING_2 = b"For my pet fish, Eric.\n"

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



# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.
# XXX(nnorwitz):  these tests leak fds when there is an error.
class PtyTest(unittest.TestCase):
    def setUp(self):
        # isatty() and close() can hang on some platforms.  Set an alarm
        # before running the test to make sure we don't hang forever.
        old_alarm = signal.signal(signal.SIGALRM, self.handle_sig)
        self.addCleanup(signal.signal, signal.SIGALRM, old_alarm)
        self.addCleanup(signal.alarm, 0)
        signal.alarm(10)

    def handle_sig(self, sig, frame):
        self.fail("isatty hung")

    def test_basic(self):
        try:
            debug("Calling master_open()")
            master_fd, slave_name = pty.master_open()
            debug("Got master_fd '%d', slave_name '%s'" %
                  (master_fd, slave_name))
            debug("Calling slave_open(%r)" % (slave_name,))
            slave_fd = pty.slave_open(slave_name)
            debug("Got slave_fd '%d'" % slave_fd)
        except OSError:
            # " An optional feature could not be imported " ... ?
            raise unittest.SkipTest("Pseudo-terminals (seemingly) not functional.")

        self.assertTrue(os.isatty(slave_fd), 'slave_fd is not a tty')

        # Solaris requires reading the fd before anything is returned.
        # My guess is that since we open and close the slave fd
        # in master_open(), we need to read the EOF.

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
            res = status >> 8
            debug("Child (%d) exited with status %d (%d)." % (pid, res, status))
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

        # pty.fork() passed.


class _MockSelectEternalWait(Exception):
    """Used both as exception and placeholder value.  Models that no
    more select activity is expected and that a test can be
    terminated."""
    pass

class SmallPtyTests(unittest.TestCase):
    """Whitebox mocking tests which don't spawn children or hang.  Test
    the _copy loop to transfer data between parent and child."""

    def setUp(self):
        self.orig_stdin_fileno = pty.STDIN_FILENO
        self.orig_stdout_fileno = pty.STDOUT_FILENO
        self.orig_pty_select = pty.select
        self.fds = []  # A list of file descriptors to close.
        self.files = []
        self.select_rfds_lengths = []
        self.select_rfds_results = []

        # monkey-patch and replace with mock
        pty.select = self._mock_select
        self._mock_stdin_stdout()

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

    def _mock_stdin_stdout(self):
        """Mock STDIN and STDOUT with two fresh pipes.  Replaces
        pty.STDIN_FILENO/pty.STDOUT_FILENO by one end of the pipe.
        Makes the other end of the pipe available via self."""
        self.read_from_stdout_fd, pty.STDOUT_FILENO = self._pipe()
        pty.STDIN_FILENO, self.write_to_stdin_fd = self._pipe()

    def _mock_select(self, rfds, wfds, xfds):
        """Simulates the behavior of select.select.  Only implemented
        for reader waiting list (first parameter)."""
        assert wfds == [] and xfds == []
        # This will raise IndexError when no more expected calls exist.
        self.assertEqual(self.select_rfds_lengths.pop(0), len(rfds))
        if len(rfds) == 0:
            # called with three empty lists as file descriptors to wait
            # on.  Behavior of real select is platform-dependent and
            # likely infinite blocking on Linux.
            raise self.fail("mock select on no waitables")
        rfds_result = self.select_rfds_results.pop(0)

        if rfds_result is _MockSelectEternalWait:
            raise _MockSelectEternalWait
        return rfds_result, [], []

    def test__mock_stdin_stdout(self):
        self.assertGreater(pty.STDIN_FILENO, 2, "replaced by our mock")

    def test__mock_select(self):
        # Test the select proxy of this test class. Meta testing.
        self.select_rfds_lengths.append(0)
        with self.assertRaises(AssertionError):
            self._mock_select([], [], [])

        # Prepare two select calls. Second one will block forever.
        self.select_rfds_lengths.append(3)
        self.select_rfds_results.append("foo")
        self.select_rfds_lengths.append(3)
        self.select_rfds_results.append(_MockSelectEternalWait)

        # Call one
        self.assertEqual(self._mock_select([1, 2, 3], [], []),
                         ("foo", [], []))

        # Call two
        with self.assertRaises(_MockSelectEternalWait):
            self._mock_select([1, 2, 3], [], [])

        # lists are cleaned
        self.assertEqual(self.select_rfds_lengths, [])
        self.assertEqual(self.select_rfds_results, [])

    def _socketpair(self):
        socketpair = socket.socketpair()
        self.files.extend(socketpair)
        return socketpair

    def test__copy_to_each(self):
        # Test the normal data case on both master_fd and stdin.
        masters = [s.fileno() for s in self._socketpair()]

        # Feed data.  Smaller than PIPEBUF.  These writes will not block.
        os.write(masters[1], b'from master')
        os.write(self.write_to_stdin_fd, b'from stdin')

        # Expect two select calls, the last one will simulate eternal waiting
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([pty.STDIN_FILENO, masters[0]])
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append(_MockSelectEternalWait)

        with self.assertRaises(_MockSelectEternalWait):
            pty._copy(masters[0])

        # Test that the right data went to the right places.
        rfds = select.select([self.read_from_stdout_fd, masters[1]], [], [], 0)[0]
        self.assertEqual([self.read_from_stdout_fd, masters[1]], rfds)
        self.assertEqual(os.read(self.read_from_stdout_fd, 20), b'from master')
        self.assertEqual(os.read(masters[1], 20), b'from stdin')

    def _copy_eof_close_slave_helper(self, close_stdin):
        """Helper to test the empty read EOF case on master_fd and/or
        stdin."""
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # This side of the channel would usually be the slave_fd of the
        # child.  We simulate that the child has exited and its side of
        # the channel is destroyed.
        socketpair[1].close()
        self.files.remove(socketpair[1])

        # Optionally close fd or fill with dummy data in order to
        # prevent blocking on one read call
        if close_stdin:
            os.close(self.write_to_stdin_fd)
            self.fds.remove(self.write_to_stdin_fd)
        else:
            os.write(self.write_to_stdin_fd, b'from stdin')

        # Expect exactly one select() call.  This call returns master_fd
        # and STDIN.  Since the slave side of masters is closed, we
        # expect the _copy loop to exit immediately.
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([pty.STDIN_FILENO, masters[0]])

        # Run the _copy test, which returns nothing and cleanly exits
        self.assertIsNone(pty._copy(masters[0]))

        # We expect that everything is consumed
        self.assertEqual(self.select_rfds_results, [])
        self.assertEqual(self.select_rfds_lengths, [])

        # Test that STDIN was not touched.  This test simulated the
        # scenario where the child process immediately closed its end of
        # the pipe.  This means, nothing should be copied.
        rfds = select.select([self.read_from_stdout_fd, pty.STDIN_FILENO], [], [], 0)[0]
        # data or EOF is still sitting unconsumed in STDIN
        self.assertEqual(rfds, [pty.STDIN_FILENO])
        unconsumed = os.read(pty.STDIN_FILENO, 20)
        if close_stdin:
            self.assertFalse(unconsumed) #EOF
        else:
            self.assertEqual(unconsumed, b'from stdin')

    def test__copy_eof_on_all(self):
        # Test the empty read EOF case on both master_fd and stdin.
        self._copy_eof_close_slave_helper(close_stdin=True)

    def test__copy_eof_on_master(self):
        # Test the empty read EOF case on only master_fd.
        self._copy_eof_close_slave_helper(close_stdin=False)

    def test__copy_eof_on_stdin(self):
        # Test the empty read EOF case on stdin.
        masters = [s.fileno() for s in self._socketpair()]

        # Fill with dummy data
        os.write(masters[1], b'from master')

        os.close(self.write_to_stdin_fd)
        self.fds.remove(self.write_to_stdin_fd)

        # Expect two select() calls.  The first call returns master_fd
        # and STDIN.
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([pty.STDIN_FILENO, masters[0]])
        # The second call causes _MockSelectEternalWait.  We expect that
        # STDIN is removed from the waiters as it reached EOF.
        self.select_rfds_lengths.append(1)
        self.select_rfds_results.append(_MockSelectEternalWait)

        with self.assertRaises(_MockSelectEternalWait):
            pty._copy(masters[0])

        # We expect that everything is consumed
        self.assertEqual(self.select_rfds_results, [])
        self.assertEqual(self.select_rfds_lengths, [])


def tearDownModule():
    reap_children()

if __name__ == "__main__":
    unittest.main()
