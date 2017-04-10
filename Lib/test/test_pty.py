from test.support import verbose, import_module, reap_children

# Skip these tests if termios is not available
termios = import_module('termios')

import errno
import pty
import os
import sys
import select
import signal
import socket
import textwrap
import unittest

TEST_STRING_1 = b"I wish to buy a fish license.\n"
TEST_STRING_2 = b"For my pet fish, Eric.\n"

if verbose:
    def debug(msg):
        # Print debug information in a way we can call it from a forked
        # child which uses the same STDOUT as the parent.  Flush, so
        # that we can debug deadlocks and blocking of the test suite.
        print(msg, flush=True)
else:
    def debug(msg):
        pass


def normalize_output(data):
    # Some operating systems do conversions on newline.  We could possibly
    # fix that by doing the appropriate termios.tcsetattr()s.  I couldn't
    # figure out the right combo on Tru64 and I don't have an IRIX box.
    # So just normalize the output and doc the problem O/Ses by allowing
    # certain combinations for some platforms, but avoid allowing other
    # differences (like extra whitespace, trailing garbage, etc.)

    # This is about the best we can do without getting some feedback
    # from someone more knowledgable.

    # OSF/1 (Tru64) apparently turns \n into \r\r\n.
    if data.endswith(b'\r\r\n'):
        return data.replace(b'\r\r\n', b'\n')

    # IRIX apparently turns \n into \r\n.
    if data.endswith(b'\r\n'):
        return data.replace(b'\r\n', b'\n')

    return data

def _os_timeout_read(fd, n):
    """Raw wrapper around os.read which raises a TimeoutError if no data
    arrived within 10 seconds."""
    rd, _, _ = select.select([fd], [], [], 10)
    if not rd:
        raise TimeoutError
    return os.read(fd, n)

# Note that os.read() is nondeterministic so we need to be very careful
# to make the test suite deterministic.  A normal call to os.read() may
# give us less than expected.  Three wrappers with different focus
# around os.read() follow.
#
# Beware, on my Linux system, if I put 'foo\n' into a terminal fd, I get
# back 'foo\r\n' at the other end.  The behavior depends on the termios
# setting.  The newline translation may be OS-specific.  To make the
# test suite deterministic and OS-independent, _os_readline and
# normalize_output can be used.
#
# In order to avoid newline translation and normalize_output completely,
# some test cases never emit newline characters and flush the fd
# manually.  For example, using print('foo', end='', flush=True) in a
# forked child allows to read exactly len('foo') in the parent.  For
# this, _os_read_exactly and _os_read_exhaust_exactly can be used.

def _os_readline(fd):
    """Use os.read() to read byte by byte until a newline is
    encountered.  May block forever if no newline is read."""
    buf = []
    while True:
        r = os.read(fd, 1)
        if not r:
            raise EOFError
        buf.append(r)
        if r == b'\n':
            break
    return b''.join(buf)

def _os_read_exactly(fd, numbytes):
    """Read exactly numbytes out of fd.  Blocks until we have enough or
    raises TimeoutError.  Does not touch the channel beyond numbytes."""
    ret = []
    numread = 0

    while numread < numbytes:
        if numread > 0:
            # Possible non-determinism caught and prevented
            debug("[_os_read_exactly] More than one os.read() call")
        r = _os_timeout_read(fd, numbytes - numread)
        if not r:
            raise EOFError
        ret.append(r)
        numread += len(r)
    assert numread == numbytes
    return b''.join(ret)

def _os_read_exhaust_exactly(fd, numbytes):
    """Read exactly numbytes out of fd.  Blocks until we have enough or
    raises TimeoutError.  Raises ValueError if more data is in fd."""
    assert numbytes > 0
    first = _os_read_exactly(fd, numbytes - 1)
    final = _os_timeout_read(fd, 1024) #expect to read exactly 1 byte
    ret = first + final

    # The protocol used for the test suite expects exactly the specified
    # amount of data in fd.  If there is more data, there is an error.
    if len(ret) != numbytes:
        raise ValueError("Read more data than expected. Fix your protocol. "
                         "Read: {:s} ({:d} bytes), expected to read only "
                         "{:d} bytes".format(repr(ret), len(ret), numbytes))
    return ret


# We will access internal functions for mocking.
#pylint: disable=protected-access

# Marginal testing of pty suite. Cannot do extensive 'do or fail' testing
# because pty code is not too portable.
# XXX(nnorwitz):  these tests leak fds when there is an error.
class PtyBasicTest(unittest.TestCase):
    def setUp(self):
        # isatty() and close() can hang on some platforms.  Set an alarm
        # before running the test to make sure we don't hang forever.
        self.old_alarm = signal.signal(signal.SIGALRM, self.handle_sig)
        signal.alarm(10)

    def tearDown(self):
        # remove alarm, restore old alarm handler
        signal.alarm(0)
        signal.signal(signal.SIGALRM, self.old_alarm)

    def handle_sig(self, sig, frame):
        #pylint: disable=unused-argument
        self.fail("isatty hung")

    def test_basic(self):
        try:
            debug("Calling master_open()")
            # XXX deprecated function
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
        s1 = _os_readline(master_fd)
        self.assertEqual(b'I wish to buy a fish license.\n',
                         normalize_output(s1))

        debug("Writing chunked output")
        os.write(slave_fd, TEST_STRING_2[:5])
        os.write(slave_fd, TEST_STRING_2[5:])
        s2 = _os_readline(master_fd)
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

class PtyPosixIntegrationTest(unittest.TestCase):
    """Test black-box functionality.  May actually fork() and exec() a
    fresh python interpreter.  Should not be intrusive for your local
    machine.
    """
    # Tests go amok if you pipe data to STDIN.  This is expected and
    # should never happen, unless the way the test suite is called is
    # completely broken.

    def _spawn_py_get_retcode(self, python_src):
        """Helper function to do pty.spawn() on the supplied python code
        and return the return code, assuming successful termination."""
        cmd = [sys.executable, "-c", python_src]
        debug("executing: {:s}".format(' '.join(cmd)))
        ret = pty.spawn(cmd)

        # behavior of waitpid in module posix
        self.assertLess(ret, 2**16)
        killsig = ret & 0xff
        self.assertEqual(killsig, 0)

        retcode = (ret & 0xff00) >> 8
        return retcode

    def test_spawn_exitsuccess(self):
        """Spawn the python-equivalent of /bin/true."""
        retcode = self._spawn_py_get_retcode('import sys; sys.exit()')
        self.assertEqual(retcode, 0)

    def test_spawn_exitfailure(self):
        """Spawn the python-equivalent of /bin/false."""
        retcode = self._spawn_py_get_retcode('import sys; sys.exit(1)')
        self.assertEqual(retcode, 1)

    def test_spawn_uncommon_exit_code(self):
        """Test an uncommon exit code, which is less likely to be caused
        by a Python exception or other failure."""
        retcode = self._spawn_py_get_retcode('import sys; sys.exit(81)')
        self.assertEqual(retcode, 81)


class PtyMockingTestBase(unittest.TestCase):
    """Base class for tests which replace STDIN and STDOUT of the pty
    module with their own pipes."""

    def setUp(self):
        save_and_restore = ['pty.STDIN_FILENO',
                            'pty.STDOUT_FILENO',
                            'pty.select',
                            'pty.fork',
                            'os.forkpty']
        self.saved = dict()
        for k in save_and_restore:
            module, attr = k.split('.')
            module = globals()[module]
            self.saved[k] = getattr(module, attr)

        self.fds = []  # A list of file descriptors to close.
        self.files = []
        self.select_rfds_lengths = []
        self.select_rfds_results = []

    def tearDown(self):
        for k, v in self.saved.items():
            module, attr = k.split('.')
            module = globals()[module]
            setattr(module, attr, v)

        for file in self.files:
            try:
                file.close()
            except OSError:
                debug("close error {}".format(file))
        for fd in self.fds:
            try:
                os.close(fd)
            except OSError:
                debug("os.close error {}".format(fd))

    def _pipe(self):
        pipe_fds = os.pipe()
        self.fds.extend(pipe_fds)
        return pipe_fds

    def _mock_stdin_stdout(self):
        """Mock STDIN and STDOUT with two fresh pipes.  Replaces
        pty.STDIN_FILENO/pty.STDOUT_FILENO by one end of the pipe.
        Returns the other end of the pipe."""
        read_from_stdout_fd, mock_stdout_fd = self._pipe()
        pty.STDOUT_FILENO = mock_stdout_fd
        mock_stdin_fd, write_to_stdin_fd = self._pipe()
        pty.STDIN_FILENO = mock_stdin_fd

        # STDIN and STDOUT fileno of the pty module are replaced by our
        # mocks.  This is required for the pty._copy loop.  In contrast,
        # when pty.fork is called, the child's input/output must be set
        # up properly, i.e. STDIN=0, STDOUT=1; not our mock fds.  If
        # pty.fork can delegate its work to os.forkpty, STDIN and STDOUT
        # are correctly set.  If os.forkpty is not available, the backup
        # path of pty.fork would break in the presence of our mocks.  We
        # wrap pty.fork to temporarily restore the STDIN/STDOUT file
        # descriptors for forking and reintroduce our mocks immediately
        # afterwards.
        def forkwrap():
            # debug("Calling wrapped pty.fork ({}), "
            #       "delegating to {}".format(pty.fork,
            #                                 self.saved['pty.fork']))
            pty.STDIN_FILENO = 0
            pty.STDOUT_FILENO = 1
            self.assertEqual(pty.STDERR_FILENO, 2, "pty.fork correct STDERR")
            ret = self.saved['pty.fork']()
            pty.STDIN_FILENO = mock_stdin_fd
            pty.STDOUT_FILENO = mock_stdout_fd
            return ret
        pty.fork = forkwrap

        return (write_to_stdin_fd, read_from_stdout_fd)

    def _disable_os_forkpty(self):
        """os.forkpty is only available on some flavours of UNIX.
        Replace it by a function which always fails.  Used to trigger
        both code paths in pty.fork."""
        os.forkpty = self._mock_disabled_osforkpty

    @staticmethod
    def _mock_disabled_osforkpty():
        """Simulate function failure or unavailability."""
        debug("os.forkpty disabled by mock function.")
        raise OSError

class PtySpawnTestBase(PtyMockingTestBase):
    """A base class for the following integration test setup: A child
    process is spawned with pty.spawn().  The child runs a fresh python
    interpreter; its python code is passed via command line argument as
    string.  A background process is forked, reusing the current
    instance of the python interpreter.  These processes are connected
    over STDIN/STDOUT pipes.  These tests run fork(), select(),
    execlp(), and other calls on your system.

    Starting from the parent (the main thread of this test suite), two
    additional processes are forked in this test setup.  We call the
    spawn()-ed child the 'slave' because it is connected to the slave
    side of the pty.  The background process is connected to the master
    side of the pty.

    parent
      |
    create mock
    STDIN/STDOUT
    pipes
      |
      |
      ..os.fork().> background
      |                |
      |
      .......................pty.spawn(*)..>slave
     pty._copy
     and wait          | <-- STDIN/STDOUT --> |
     wait slave        |         pipes        |
                       |                      |
                       |                      |
                      exit                    |
                                              |
                                             exit
     wait for
     background
      |


    *) python -c "slave child python code here"


    The code for the spawned slave is python code in a string.  This
    makes the test suite very portable.
    """
    # We introduce this generic base class for the test setup to
    # encapsulate multiple different types of tests in individual
    # classes.

    # Helper building blocks for the spawned (slave) python shell
    _EXEC_IMPORTS = textwrap.dedent("""\
        import sys
        import time
        import signal
        import tty, termios
        """)

    @staticmethod
    def _fork_background_process(master_fun, io_fds):
        pid = os.fork()
        assert pid >= 0, "fork failure must raise OSError"
        if pid > 0:
            debug("forked child ({:d}) from parent ({:d})".format(pid, os.getpid()))
            return pid

        # Forked.  Run master_fun and pass return code back to parent,
        # wrapped to catch all exceptions.
        try:
            debug("[background] started ({:d})".format(os.getpid()))
            rc = master_fun(*io_fds)
            if not isinstance(rc, int):
                raise Exception("master_fun must return an int")
        except:
            debug("[background] Abort due to exception")
            sys.excepthook(*sys.exc_info())
            rc = 1
        finally:
            if rc != 0:
                debug("[background] Abnormal termination ({:d})".format(os.getpid()))
            # Destroy forked background process.
            # Do not use sys.exit(), it is hooked by the test suite.
            sys.stdout.flush()
            sys.stderr.flush()
            os._exit(rc)

    def _spawn_master_and_slave(self, master_fun, slave_src, close_stdin=False):
        """Spawn a slave and fork a master background process.
        master_fun must be a python function.  slave_src must be python
        code as string.  This function forks them and connects their
        STDIN/STDOUT over a pipe and checks that they cleanly exit.
        Control never returns from master_fun."""
        io_fds = self._mock_stdin_stdout()
        # io_fds[0]: write to slave's STDIN
        # io_fds[1]: read from slave's STDOUT

        if close_stdin:
            debug("Closing stdin")
            os.close(io_fds[0])
            self.fds.remove(io_fds[0])

        sys.stdout.flush()
        sys.stderr.flush()
        # Without this flush, we interfere with the debug output from
        # the child that will be spawned and execute master_fun. Don't
        # confuse it with the child spawned by spawn().  It will work
        # fine without the flush, unless you enable debug and have your
        # STDOUT piped!  The forked child will print to the same fd as
        # we (the parent) print our debug information to.

        background_pid = self._fork_background_process(master_fun, io_fds)

        # spawn the slave in a new python interpreter, passing the code
        # with the -c option
        retcode_slave = pty.spawn([sys.executable, '-c', slave_src])
        if retcode_slave != 0:
            debug("Slave failed.")
            errmsg = ["Spawned slave returned but failed.",
                      "Exit code {:d}".format(retcode_slave)]
            debug("killing background process ({:d})".format(background_pid))
            os.kill(background_pid, 9)
            if verbose:
                read_from_stdout_fd = io_fds[1]
                errmsg.extend(["The failed child code was:",
                               "--- BEGIN child code ---",
                               slave_src,
                               "--- END child code ---"])
                rd, _, _ = select.select([read_from_stdout_fd], [], [], 0)
                if rd:
                    errmsg.append("Dumping what the slave wrote last:")
                    rawoutput = os.read(read_from_stdout_fd, 1024*1024)
                    errmsg.append(repr(rawoutput))
                else:
                    errmsg.append("No output from child.")
            self.fail('\n'.join(errmsg))
        self.assertEqual(retcode_slave, 0)

        retcode_background = os.waitpid(background_pid, 0)[1]
        self.assertEqual(retcode_background, 0)
        debug("background and slave are done")

        # We require that b'slave exits now' is left in the slave's
        # STDOUT to confirm clean exit.
        expecting = b'slave exits now'
        slave_wrote = _os_read_exhaust_exactly(io_fds[1], len(expecting))
        self.assertEqual(slave_wrote, expecting)


class PtyPingTest(PtySpawnTestBase):
    """Master and Slave count to 1000 by turns."""

    _EXEC_CHILD = textwrap.dedent("""
        # Set terminal to a well-defined state.  Disable echoing by
        # setting it to raw mode.
        tty.setraw(sys.stdin.fileno())

        # Ping-Pong count to 1000 with master. We start.
        for i in range(1000):
            print("Ping {:d}".format(i), end='', flush=True)
            pong, num = input().split()
            if pong != "Pong" or int(num) != i:
                sys.exit("Did not get Pong")

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)
        sys.exit() #success
        """)

    @staticmethod
    def _background_process(to_stdin, from_stdout):
        debug("Staring Ping Pong")
        # Ping-Pong count to 1000 with slave.
        # Read Ping from slave, reply with pong.
        # If something is wrong, this may block.
        for i in range(1000):
            expected = "Ping {:d}".format(i)
            received = _os_read_exhaust_exactly(from_stdout,
                                                len(expected)).decode('ascii')
            if expected != received:
                raise RuntimeError("Expected {:s}, received "
                                   "{:s}".format(expected, received))
            answer = "Pong {:d}\n".format(i).encode('ascii')
            pty._writen(to_stdin, answer)

        return 0 # success

    def test_alternate_ping(self):
        """Spawn a slave and fork a master background process.  Let them
        Ping-Pong count to 1000 by turns."""
        child_code = self._EXEC_IMPORTS + self._EXEC_CHILD
        self._spawn_master_and_slave(self._background_process, child_code)

    def test_alternate_ping_disable_osforkpty(self):
        """Spawn a slave and fork a master background process.  Let them
        Ping-Pong count to 1000 by turns. Disable os.forkpty(), trigger
        pty.fork() backup code."""
        self._disable_os_forkpty()
        child_code = self._EXEC_IMPORTS + self._EXEC_CHILD
        self._spawn_master_and_slave(self._background_process, child_code)

class PtyReadAllTest(PtySpawnTestBase):
    """Read from (slow) pty.spawn()ed child, make sure we get
    everything.  Slow tests."""

    @staticmethod
    def _background_process(to_stdin, from_stdout):
        debug("[background] starting to read")

        bytes_transferred = 0
        for i in range(500):
            expected = "long cat is long "*10 + "ID {:d}".format(i)
            expected = expected.encode('ascii')
            received = _os_read_exactly(from_stdout, len(expected))
            if expected != received:
                raise RuntimeError("Expected {!r} but got {!r}".format(expected, received))
            bytes_transferred += len(received)

        debug("[background] received {} bytes from the slave.".format(bytes_transferred))
        return 0 # success

    # a dynamic sleep time needs to be formatted.
    _EXEC_CHILD_FMT = textwrap.dedent("""
        tty.setraw(sys.stdin.fileno())

        for i in range(500):
            print("long cat is long "*10 + "ID {{:d}}".format(i), end='', flush=True)
            if {sleeptime:f} and i % 400 == 0:
                time.sleep({sleeptime:f}) # make slow, once.

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)
        sys.exit()
        """)

    def test_read(self):
        """Spawn a slave and fork a master background process.  Receive
        several kBytes from the slave."""
        child_code = self._EXEC_IMPORTS + \
                     self._EXEC_CHILD_FMT.format(sleeptime=0.05)
        debug("Test may take up to 1 second ...")
        self._spawn_master_and_slave(self._background_process, child_code)

    def test_read_close_stdin(self):
        """Spawn a slave and fork a master background process.  Close
        STDIN and receive several kBytes from the slave."""
        # only sleep in one test to speed this up
        child_code = self._EXEC_IMPORTS + \
                     self._EXEC_CHILD_FMT.format(sleeptime=0)
        self._spawn_master_and_slave(self._background_process, child_code,
                                     close_stdin=True)

class PtyTermiosIntegrationTest(PtySpawnTestBase):
    """Terminals are not just pipes.  This integration testsuite asserts
    that specific terminal functionality is operational.  It tests ISIG,
    which transforms sending 0x03 at the master side (usually triggered
    by humans by pressing ctrl+c) to sending an INTR signal to the
    child.  In addition, on Linux, it tests pretty printing of control
    characters, for example ^G, which is not defined in the POSIX.1-2008
    Standard but implemented.

    This class contains larger integration tests which verify the subtle
    interplay of several modules.  It depends on termios, pty, signal,
    and os.  It uses the Linux-only control character pretty printing
    feature because this is one of the simple features of terminals
    which are easy to test without digging into full-fledged os-level
    integration tests.
    """

    @staticmethod
    def _wait_for_slave(to_stdin, from_stdout):
        # Expected to be called by _background_process. Wait for the
        # slave to become ready and initialized.
        debug("[background] waiting for slave process.")
        read = _os_read_exhaust_exactly(from_stdout, len(b'slave ready!'))
        if read != b'slave ready!':
            raise ValueError('handshake with slave failed')
        debug("[background] slave ready.")

    def _enable_echoctl(self):
        if sys.platform == 'linux':
            self.echoctl = True
        else:
            raise unittest.SkipTest('Test only available on Linux.')

    _EXEC_BASE_TERMINAL_SETUP_FMT = textwrap.dedent(r"""
        def _base_terminal_setup(additional_lflag=0):
            "Set up terminal to sane defaults (with regard to my Linux"
            "system). See POSIX.1-2008, Chapter 11, General Terminal"
            "Interface."

            # Warning: ECHOCTL is not defined in POSIX.  Works on
            # Linux 4.4 with Ubuntu GLIBC 2.23. Did not work on Mac.
            if {echoctl}:
                echoctl = termios.ECHOCTL
            else:
                echoctl = 0

            terminal_fd = sys.stdin.fileno()
            old = termios.tcgetattr(terminal_fd)
            # don't need iflag
            old[0] = 0

            # oflag: output processing: replace \n by \r\n
            old[1] = termios.ONLCR | termios.OPOST

            # don't need cflag
            old[2] = 0

            # lflag: canonical mode (line-buffer),
            #        normal echoing,
            #        echoing of control chars in caret notation (for example ^C)
            old[3] = termios.ICANON | termios.ECHO | echoctl | additional_lflag

            termios.tcsetattr(terminal_fd, termios.TCSADRAIN, old)
        """)

    @staticmethod
    def _background_process_echo(to_stdin, from_stdout):
        PtyTermiosIntegrationTest._wait_for_slave(to_stdin, from_stdout)

        answer = b"Hello, I'm background process!\n"
        pty._writen(to_stdin, answer)

        # Slave terminal echoes back everything, rewriting line endings.
        answer = answer[:-1] + b'\r\n'
        read = _os_read_exactly(from_stdout, len(answer))
        if read != answer:
            debug("Unexpected answer: {!r}".format(read))
            raise ValueError('Getting echoed data failed')
        return 0

    _EXEC_CHILD_ECHO = textwrap.dedent(r"""
        _base_terminal_setup()
        print("slave ready!", end='', flush=True)

        inp = input()
        if inp != "Hello, I'm background process!":
            sys.exit("failure getting answer, got `{}'".format(inp))

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)
        sys.exit()
        """)

    def test_echo(self):
        """Terminals: Echoing of all characters written to the master
        side and newline output translation."""
        child_code = self._EXEC_IMPORTS + \
            self._EXEC_BASE_TERMINAL_SETUP_FMT.format(echoctl=False) + \
            self._EXEC_CHILD_ECHO
        self._spawn_master_and_slave(self._background_process_echo, child_code)

    @staticmethod
    def _background_process_bell(to_stdin, from_stdout):
        PtyTermiosIntegrationTest._wait_for_slave(to_stdin, from_stdout)

        debug("[background] sending bell escape sequence to slave")
        BELL = b'\a'
        to_slave = b'Bell here -> '+BELL+b' <-Hello slave!\n'
        pty._writen(to_stdin, to_slave)

        # Bell character gets `pretty-printed' when echoed by terminal
        expected = b'Bell here -> ^G <-Hello slave!\r\n'
        received = _os_read_exactly(from_stdout, len(expected))
        if received != expected:
            raise RuntimeError("Expecting {!r} but got {!r}".format(expected, received))

        debug("[background] got it back")
        return 0

    _EXEC_CHILD_BELL = textwrap.dedent(r"""
        _base_terminal_setup()
        print("slave ready!", end='', flush=True)

        command = input()
        # note how background process gets ^G and slave gets \a
        if command != 'Bell here -> \a <-Hello slave!':
            sys.exit("failure getting bell")
        # terminal has automatically echoed the command, we can ignore it

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)
        sys.exit()
        """)

    def test_bell_echoctl(self):
        """Terminals: Pretty printing of the bell character in caret
        notation."""
        self._enable_echoctl()
        child_code = self._EXEC_IMPORTS + \
            self._EXEC_BASE_TERMINAL_SETUP_FMT.format(echoctl=self.echoctl) + \
            self._EXEC_CHILD_BELL
        self._spawn_master_and_slave(self._background_process_bell, child_code)

    @staticmethod
    def _background_process_eof(to_stdin, from_stdout):
        PtyTermiosIntegrationTest._wait_for_slave(to_stdin, from_stdout)

        debug("[background] sending slave an EOF")
        EOF = b'\x04'
        pty._writen(to_stdin, EOF)

        # On OS X, we found that this test leaves an EOF character in
        # STDOUT.  Tested on OS X 10.6.8 and 10.11.2.  Wipe EOF
        # character which may remain here.
        c = os.read(from_stdout, 1)
        if c == b'\x04':
            c = os.read(from_stdout, 1)
        if c != b'!':
            raise RuntimeError("Did not receive marker.")

        return 0

    _EXEC_CHILD_EOF = textwrap.dedent("""
        _base_terminal_setup()
        print("slave ready!", end='', flush=True)

        try:
            input()
            # unreachable if we got our EOF:
            sys.exit("failure, no EOF received")
        except EOFError:
            # we expect an EOF here, this is good
            pass

        # OS X leaves an EOF character in the channel which we want to
        # remove. We set an exclamation mark as marker and in the
        # background process, we read everything until we reach this
        # marker.
        print("!", end='', flush=True)

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)
        sys.exit()
        """)

    def test_eof(self):
        """Terminals: Processing of the special EOF character."""
        self.echoctl = False
        child_code = self._EXEC_IMPORTS + \
            self._EXEC_BASE_TERMINAL_SETUP_FMT.format(echoctl=self.echoctl) + \
            self._EXEC_CHILD_EOF
        self._spawn_master_and_slave(self._background_process_eof, child_code)

    def test_eof_echoctl(self):
        """Terminals: Processing of the special EOF character with
        ECHOCTL enabled."""
        # ^D is usually not pretty printed
        self._enable_echoctl()
        child_code = self._EXEC_IMPORTS + \
            self._EXEC_BASE_TERMINAL_SETUP_FMT.format(echoctl=self.echoctl) + \
            self._EXEC_CHILD_EOF
        self._spawn_master_and_slave(self._background_process_eof, child_code)

    @staticmethod
    def _background_process_intr(to_stdin, from_stdout):
        """Try to send SIGINT to child. Careful: Testsuite also watches
        for SIGINT.  We only set our signal handler in the forked
        slave."""
        PtyTermiosIntegrationTest._wait_for_slave(to_stdin, from_stdout)

        debug("[background] sending interrupt escape sequence to slave.")
        INTR = b'\x03'
        to_slave = b'This buffered stuff will be ignored'+INTR+b' Ohai slave!\n'
        pty._writen(to_stdin, to_slave)

        expected = INTR+b' Ohai slave!\r\n'
        received = _os_read_exactly(from_stdout, len(expected))
        if received != expected:
            raise RuntimeError("Expecting {!r} but got {!r}".format(expected, received))

        debug("[background] got it back")
        return 0

    _EXEC_CHILD_INTR = textwrap.dedent("""
        _sigint_received = False

        def _SIGINT_handler(a, b):
            global _sigint_received
            _sigint_received = True

        signal.signal(signal.SIGINT, _SIGINT_handler)

        # tell our controlling terminal to send signals on special characters
        _base_terminal_setup(termios.ISIG)
        print("slave ready!", end='', flush=True)

        command = input()
        # Yes, only this arrives at STDIN here!
        if command != ' Ohai slave!':
            print(command)
            sys.exit("failure getting interrupted input")
        # terminal has automatically echoed the command and ^C, we can ignore it

        # Send final confirmation that all went well to master.
        print("slave exits now", end='', flush=True)

        if _sigint_received:
            sys.exit()
        else:
            sys.exit("failure, did not receive SIGINT")
        """)

    def test_intr(self):
        """Terminals: Writing a x03 char to the master side is
        translated to sending an INTR signal to the slave.  Simulates
        pressing ctrl+c in master."""
        self.echoctl = False
        child_code = self._EXEC_IMPORTS + \
            self._EXEC_BASE_TERMINAL_SETUP_FMT.format(echoctl=self.echoctl) + \
            self._EXEC_CHILD_INTR
        self._spawn_master_and_slave(self._background_process_intr, child_code)


class _MockSelectEternalWait(Exception):
    """Used both as exception and placeholder value.  Models that no
    more select activity is expected and that a test can be
    terminated."""
    pass

class PtyCopyTests(PtyMockingTestBase):
    """Whitebox mocking tests which don't spawn children or hang.  Test
    the _copy loop to transfer data between parent and child."""

    def _socketpair(self):
        socketpair = socket.socketpair()
        self.files.extend(socketpair)
        return socketpair

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

    def test__mock_select(self):
        """Test the select proxy of the test class.  Such meta testing.
        """
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

    def test__copy_to_each(self):
        """Test the normal data case on both master_fd and stdin."""
        write_to_stdin_fd, read_from_stdout_fd = self._mock_stdin_stdout()
        mock_stdin_fd = pty.STDIN_FILENO
        self.assertGreater(mock_stdin_fd, 2, "replaced by our mock")
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # Feed data.  Smaller than PIPEBUF.  These writes will not block.
        os.write(masters[1], b'from master')
        os.write(write_to_stdin_fd, b'from stdin')

        # monkey-patch pty.select with our mock
        pty.select = self._mock_select

        # Expect two select calls, the last one will simulate eternal waiting
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([mock_stdin_fd, masters[0]])
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append(_MockSelectEternalWait)

        with self.assertRaises(_MockSelectEternalWait):
            pty._copy(masters[0])

        # Test that the right data went to the right places.
        rfds = select.select([read_from_stdout_fd, masters[1]], [], [], 0)[0]
        self.assertEqual([read_from_stdout_fd, masters[1]], rfds)
        self.assertEqual(os.read(read_from_stdout_fd, 20), b'from master')
        self.assertEqual(os.read(masters[1], 20), b'from stdin')

    def _copy_eof_close_slave_helper(self, close_stdin):
        """Helper to test the empty read EOF case on master_fd and/or
        stdin."""
        write_to_stdin_fd, read_from_stdout_fd = self._mock_stdin_stdout()
        mock_stdin_fd = pty.STDIN_FILENO
        self.assertGreater(mock_stdin_fd, 2, "replaced by our mock")
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # This side of the channel would usually be the slave_fd of the
        # child.  We simulate that the child has exited and its side of
        # the channel is destroyed.
        socketpair[1].close()
        self.files.remove(socketpair[1])

        # optionally close fd or fill with dummy data in order to
        # prevent blocking on one read call
        if close_stdin:
            os.close(write_to_stdin_fd)
            self.fds.remove(write_to_stdin_fd)
        else:
            os.write(write_to_stdin_fd, b'from stdin')

        # monkey-patch pty.select with our mock
        pty.select = self._mock_select

        # Expect exactly one select() call.  This call returns master_fd
        # and STDIN.  Since the slave side of masters is closed, we
        # expect the _copy loop to exit immediately.
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([mock_stdin_fd, masters[0]])

        # run the _copy test, which returns nothing and cleanly exits
        self.assertIsNone(pty._copy(masters[0]))

        # We expect that everything is consumed
        self.assertEqual(self.select_rfds_results, [])
        self.assertEqual(self.select_rfds_lengths, [])

        # Test that STDIN was not touched.  This test simulated the
        # scenario where the child process immediately closed its end of
        # the pipe.  This means, nothing should be copied.
        rfds = select.select([read_from_stdout_fd, mock_stdin_fd], [], [], 0)[0]
        # data or EOF is still sitting unconsumed in mock_stdin_fd
        self.assertEqual(rfds, [mock_stdin_fd])
        unconsumed = os.read(mock_stdin_fd, 20)
        if close_stdin:
            self.assertFalse(unconsumed) #EOF
        else:
            self.assertEqual(unconsumed, b'from stdin')

    def test__copy_eof_on_all(self):
        """Test the empty read EOF case on both master_fd and stdin."""
        self._copy_eof_close_slave_helper(close_stdin=True)

    def test__copy_eof_on_master(self):
        """Test the empty read EOF case on only master_fd."""
        self._copy_eof_close_slave_helper(close_stdin=False)

    def test__copy_eof_on_stdin(self):
        """Test the empty read EOF case on stdin."""
        write_to_stdin_fd, read_from_stdout_fd = self._mock_stdin_stdout()
        mock_stdin_fd = pty.STDIN_FILENO
        self.assertGreater(mock_stdin_fd, 2, "replaced by our mock")
        socketpair = self._socketpair()
        masters = [s.fileno() for s in socketpair]

        # Fill with dummy data
        os.write(masters[1], b'from master')

        os.close(write_to_stdin_fd)
        self.fds.remove(write_to_stdin_fd)

        # monkey-patch pty.select with our mock
        pty.select = self._mock_select

        # Expect two select() calls.  The first call returns master_fd
        # and STDIN.
        self.select_rfds_lengths.append(2)
        self.select_rfds_results.append([mock_stdin_fd, masters[0]])
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
