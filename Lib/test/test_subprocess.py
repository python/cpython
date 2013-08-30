import unittest
from test import test_support
import subprocess
import sys
import signal
import os
import errno
import tempfile
import time
import re
import sysconfig

try:
    import resource
except ImportError:
    resource = None
try:
    import threading
except ImportError:
    threading = None

mswindows = (sys.platform == "win32")

#
# Depends on the following external programs: Python
#

if mswindows:
    SETBINARY = ('import msvcrt; msvcrt.setmode(sys.stdout.fileno(), '
                                                'os.O_BINARY);')
else:
    SETBINARY = ''


try:
    mkstemp = tempfile.mkstemp
except AttributeError:
    # tempfile.mkstemp is not available
    def mkstemp():
        """Replacement for mkstemp, calling mktemp."""
        fname = tempfile.mktemp()
        return os.open(fname, os.O_RDWR|os.O_CREAT), fname


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        test_support.reap_children()

    def tearDown(self):
        for inst in subprocess._active:
            inst.wait()
        subprocess._cleanup()
        self.assertFalse(subprocess._active, "subprocess._active not empty")

    def assertStderrEqual(self, stderr, expected, msg=None):
        # In a debug build, stuff like "[6580 refs]" is printed to stderr at
        # shutdown time.  That frustrates tests trying to check stderr produced
        # from a spawned Python process.
        actual = re.sub(r"\[\d+ refs\]\r?\n?$", "", stderr)
        self.assertEqual(actual, expected, msg)


class PopenTestException(Exception):
    pass


class PopenExecuteChildRaises(subprocess.Popen):
    """Popen subclass for testing cleanup of subprocess.PIPE filehandles when
    _execute_child fails.
    """
    def _execute_child(self, *args, **kwargs):
        raise PopenTestException("Forced Exception for Test")


class ProcessTestCase(BaseTestCase):

    def test_call_seq(self):
        # call() function with sequence argument
        rc = subprocess.call([sys.executable, "-c",
                              "import sys; sys.exit(47)"])
        self.assertEqual(rc, 47)

    def test_check_call_zero(self):
        # check_call() function with zero return code
        rc = subprocess.check_call([sys.executable, "-c",
                                    "import sys; sys.exit(0)"])
        self.assertEqual(rc, 0)

    def test_check_call_nonzero(self):
        # check_call() function with non-zero return code
        with self.assertRaises(subprocess.CalledProcessError) as c:
            subprocess.check_call([sys.executable, "-c",
                                   "import sys; sys.exit(47)"])
        self.assertEqual(c.exception.returncode, 47)

    def test_check_output(self):
        # check_output() function with zero return code
        output = subprocess.check_output(
                [sys.executable, "-c", "print 'BDFL'"])
        self.assertIn('BDFL', output)

    def test_check_output_nonzero(self):
        # check_call() function with non-zero return code
        with self.assertRaises(subprocess.CalledProcessError) as c:
            subprocess.check_output(
                    [sys.executable, "-c", "import sys; sys.exit(5)"])
        self.assertEqual(c.exception.returncode, 5)

    def test_check_output_stderr(self):
        # check_output() function stderr redirected to stdout
        output = subprocess.check_output(
                [sys.executable, "-c", "import sys; sys.stderr.write('BDFL')"],
                stderr=subprocess.STDOUT)
        self.assertIn('BDFL', output)

    def test_check_output_stdout_arg(self):
        # check_output() function stderr redirected to stdout
        with self.assertRaises(ValueError) as c:
            output = subprocess.check_output(
                    [sys.executable, "-c", "print 'will not be run'"],
                    stdout=sys.stdout)
            self.fail("Expected ValueError when stdout arg supplied.")
        self.assertIn('stdout', c.exception.args[0])

    def test_call_kwargs(self):
        # call() function with keyword args
        newenv = os.environ.copy()
        newenv["FRUIT"] = "banana"
        rc = subprocess.call([sys.executable, "-c",
                              'import sys, os;'
                              'sys.exit(os.getenv("FRUIT")=="banana")'],
                             env=newenv)
        self.assertEqual(rc, 1)

    def test_invalid_args(self):
        # Popen() called with invalid arguments should raise TypeError
        # but Popen.__del__ should not complain (issue #12085)
        with test_support.captured_stderr() as s:
            self.assertRaises(TypeError, subprocess.Popen, invalid_arg_name=1)
            argcount = subprocess.Popen.__init__.__code__.co_argcount
            too_many_args = [0] * (argcount + 1)
            self.assertRaises(TypeError, subprocess.Popen, *too_many_args)
        self.assertEqual(s.getvalue(), '')

    def test_stdin_none(self):
        # .stdin is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print "banana"'],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        p.wait()
        self.assertEqual(p.stdin, None)

    def test_stdout_none(self):
        # .stdout is None when not redirected, and the child's stdout will
        # be inherited from the parent.  In order to test this we run a
        # subprocess in a subprocess:
        # this_test
        #   \-- subprocess created by this test (parent)
        #          \-- subprocess created by the parent subprocess (child)
        # The parent doesn't specify stdout, so the child will use the
        # parent's stdout.  This test checks that the message printed by the
        # child goes to the parent stdout.  The parent also checks that the
        # child's stdout is None.  See #11963.
        code = ('import sys; from subprocess import Popen, PIPE;'
                'p = Popen([sys.executable, "-c", "print \'test_stdout_none\'"],'
                '          stdin=PIPE, stderr=PIPE);'
                'p.wait(); assert p.stdout is None;')
        p = subprocess.Popen([sys.executable, "-c", code],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        out, err = p.communicate()
        self.assertEqual(p.returncode, 0, err)
        self.assertEqual(out.rstrip(), 'test_stdout_none')

    def test_stderr_none(self):
        # .stderr is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print "banana"'],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stdin.close)
        p.wait()
        self.assertEqual(p.stderr, None)

    def test_executable_with_cwd(self):
        python_dir = os.path.dirname(os.path.realpath(sys.executable))
        p = subprocess.Popen(["somethingyoudonthave", "-c",
                              "import sys; sys.exit(47)"],
                             executable=sys.executable, cwd=python_dir)
        p.wait()
        self.assertEqual(p.returncode, 47)

    @unittest.skipIf(sysconfig.is_python_build(),
                     "need an installed Python. See #7774")
    def test_executable_without_cwd(self):
        # For a normal installation, it should work without 'cwd'
        # argument.  For test runs in the build directory, see #7774.
        p = subprocess.Popen(["somethingyoudonthave", "-c",
                              "import sys; sys.exit(47)"],
                             executable=sys.executable)
        p.wait()
        self.assertEqual(p.returncode, 47)

    def test_stdin_pipe(self):
        # stdin redirection
        p = subprocess.Popen([sys.executable, "-c",
                         'import sys; sys.exit(sys.stdin.read() == "pear")'],
                        stdin=subprocess.PIPE)
        p.stdin.write("pear")
        p.stdin.close()
        p.wait()
        self.assertEqual(p.returncode, 1)

    def test_stdin_filedes(self):
        # stdin is set to open file descriptor
        tf = tempfile.TemporaryFile()
        d = tf.fileno()
        os.write(d, "pear")
        os.lseek(d, 0, 0)
        p = subprocess.Popen([sys.executable, "-c",
                         'import sys; sys.exit(sys.stdin.read() == "pear")'],
                         stdin=d)
        p.wait()
        self.assertEqual(p.returncode, 1)

    def test_stdin_fileobj(self):
        # stdin is set to open file object
        tf = tempfile.TemporaryFile()
        tf.write("pear")
        tf.seek(0)
        p = subprocess.Popen([sys.executable, "-c",
                         'import sys; sys.exit(sys.stdin.read() == "pear")'],
                         stdin=tf)
        p.wait()
        self.assertEqual(p.returncode, 1)

    def test_stdout_pipe(self):
        # stdout redirection
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stdout.write("orange")'],
                         stdout=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read(), "orange")

    def test_stdout_filedes(self):
        # stdout is set to open file descriptor
        tf = tempfile.TemporaryFile()
        d = tf.fileno()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stdout.write("orange")'],
                         stdout=d)
        p.wait()
        os.lseek(d, 0, 0)
        self.assertEqual(os.read(d, 1024), "orange")

    def test_stdout_fileobj(self):
        # stdout is set to open file object
        tf = tempfile.TemporaryFile()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stdout.write("orange")'],
                         stdout=tf)
        p.wait()
        tf.seek(0)
        self.assertEqual(tf.read(), "orange")

    def test_stderr_pipe(self):
        # stderr redirection
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=subprocess.PIPE)
        self.addCleanup(p.stderr.close)
        self.assertStderrEqual(p.stderr.read(), "strawberry")

    def test_stderr_filedes(self):
        # stderr is set to open file descriptor
        tf = tempfile.TemporaryFile()
        d = tf.fileno()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=d)
        p.wait()
        os.lseek(d, 0, 0)
        self.assertStderrEqual(os.read(d, 1024), "strawberry")

    def test_stderr_fileobj(self):
        # stderr is set to open file object
        tf = tempfile.TemporaryFile()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertStderrEqual(tf.read(), "strawberry")

    def test_stdout_stderr_pipe(self):
        # capture stdout and stderr to the same pipe
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys;'
                          'sys.stdout.write("apple");'
                          'sys.stdout.flush();'
                          'sys.stderr.write("orange")'],
                         stdout=subprocess.PIPE,
                         stderr=subprocess.STDOUT)
        self.addCleanup(p.stdout.close)
        self.assertStderrEqual(p.stdout.read(), "appleorange")

    def test_stdout_stderr_file(self):
        # capture stdout and stderr to the same open file
        tf = tempfile.TemporaryFile()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys;'
                          'sys.stdout.write("apple");'
                          'sys.stdout.flush();'
                          'sys.stderr.write("orange")'],
                         stdout=tf,
                         stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertStderrEqual(tf.read(), "appleorange")

    def test_stdout_filedes_of_stdout(self):
        # stdout is set to 1 (#1531862).
        # To avoid printing the text on stdout, we do something similar to
        # test_stdout_none (see above).  The parent subprocess calls the child
        # subprocess passing stdout=1, and this test uses stdout=PIPE in
        # order to capture and check the output of the parent. See #11963.
        code = ('import sys, subprocess; '
                'rc = subprocess.call([sys.executable, "-c", '
                '    "import os, sys; sys.exit(os.write(sys.stdout.fileno(), '
                     '\'test with stdout=1\'))"], stdout=1); '
                'assert rc == 18')
        p = subprocess.Popen([sys.executable, "-c", code],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        out, err = p.communicate()
        self.assertEqual(p.returncode, 0, err)
        self.assertEqual(out.rstrip(), 'test with stdout=1')

    def test_cwd(self):
        tmpdir = tempfile.gettempdir()
        # We cannot use os.path.realpath to canonicalize the path,
        # since it doesn't expand Tru64 {memb} strings. See bug 1063571.
        cwd = os.getcwd()
        os.chdir(tmpdir)
        tmpdir = os.getcwd()
        os.chdir(cwd)
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;'
                          'sys.stdout.write(os.getcwd())'],
                         stdout=subprocess.PIPE,
                         cwd=tmpdir)
        self.addCleanup(p.stdout.close)
        normcase = os.path.normcase
        self.assertEqual(normcase(p.stdout.read()), normcase(tmpdir))

    def test_env(self):
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange"
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;'
                          'sys.stdout.write(os.getenv("FRUIT"))'],
                         stdout=subprocess.PIPE,
                         env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read(), "orange")

    def test_communicate_stdin(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.exit(sys.stdin.read() == "pear")'],
                             stdin=subprocess.PIPE)
        p.communicate("pear")
        self.assertEqual(p.returncode, 1)

    def test_communicate_stdout(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys; sys.stdout.write("pineapple")'],
                             stdout=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, "pineapple")
        self.assertEqual(stderr, None)

    def test_communicate_stderr(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys; sys.stderr.write("pineapple")'],
                             stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, None)
        self.assertStderrEqual(stderr, "pineapple")

    def test_communicate(self):
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;'
                          'sys.stderr.write("pineapple");'
                          'sys.stdout.write(sys.stdin.read())'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        (stdout, stderr) = p.communicate("banana")
        self.assertEqual(stdout, "banana")
        self.assertStderrEqual(stderr, "pineapple")

    # This test is Linux specific for simplicity to at least have
    # some coverage.  It is not a platform specific bug.
    @unittest.skipUnless(os.path.isdir('/proc/%d/fd' % os.getpid()),
                         "Linux specific")
    # Test for the fd leak reported in http://bugs.python.org/issue2791.
    def test_communicate_pipe_fd_leak(self):
        fd_directory = '/proc/%d/fd' % os.getpid()
        num_fds_before_popen = len(os.listdir(fd_directory))
        p = subprocess.Popen([sys.executable, "-c", "print()"],
                             stdout=subprocess.PIPE)
        p.communicate()
        num_fds_after_communicate = len(os.listdir(fd_directory))
        del p
        num_fds_after_destruction = len(os.listdir(fd_directory))
        self.assertEqual(num_fds_before_popen, num_fds_after_destruction)
        self.assertEqual(num_fds_before_popen, num_fds_after_communicate)

    def test_communicate_returns(self):
        # communicate() should return None if no redirection is active
        p = subprocess.Popen([sys.executable, "-c",
                              "import sys; sys.exit(47)"])
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, None)
        self.assertEqual(stderr, None)

    def test_communicate_pipe_buf(self):
        # communicate() with writes larger than pipe_buf
        # This test will probably deadlock rather than fail, if
        # communicate() does not work properly.
        x, y = os.pipe()
        if mswindows:
            pipe_buf = 512
        else:
            pipe_buf = os.fpathconf(x, "PC_PIPE_BUF")
        os.close(x)
        os.close(y)
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;'
                          'sys.stdout.write(sys.stdin.read(47));'
                          'sys.stderr.write("xyz"*%d);'
                          'sys.stdout.write(sys.stdin.read())' % pipe_buf],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        string_to_write = "abc"*pipe_buf
        (stdout, stderr) = p.communicate(string_to_write)
        self.assertEqual(stdout, string_to_write)

    def test_writes_before_communicate(self):
        # stdin.write before communicate()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;'
                          'sys.stdout.write(sys.stdin.read())'],
                         stdin=subprocess.PIPE,
                         stdout=subprocess.PIPE,
                         stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        p.stdin.write("banana")
        (stdout, stderr) = p.communicate("split")
        self.assertEqual(stdout, "bananasplit")
        self.assertStderrEqual(stderr, "")

    def test_universal_newlines(self):
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;' + SETBINARY +
                          'sys.stdout.write("line1\\n");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line2\\r");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line3\\r\\n");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line4\\r");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("\\nline5");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("\\nline6");'],
                         stdout=subprocess.PIPE,
                         universal_newlines=1)
        self.addCleanup(p.stdout.close)
        stdout = p.stdout.read()
        if hasattr(file, 'newlines'):
            # Interpreter with universal newline support
            self.assertEqual(stdout,
                             "line1\nline2\nline3\nline4\nline5\nline6")
        else:
            # Interpreter without universal newline support
            self.assertEqual(stdout,
                             "line1\nline2\rline3\r\nline4\r\nline5\nline6")

    def test_universal_newlines_communicate(self):
        # universal newlines through communicate()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys,os;' + SETBINARY +
                          'sys.stdout.write("line1\\n");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line2\\r");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line3\\r\\n");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("line4\\r");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("\\nline5");'
                          'sys.stdout.flush();'
                          'sys.stdout.write("\\nline6");'],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                         universal_newlines=1)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        (stdout, stderr) = p.communicate()
        if hasattr(file, 'newlines'):
            # Interpreter with universal newline support
            self.assertEqual(stdout,
                             "line1\nline2\nline3\nline4\nline5\nline6")
        else:
            # Interpreter without universal newline support
            self.assertEqual(stdout,
                             "line1\nline2\rline3\r\nline4\r\nline5\nline6")

    def test_no_leaking(self):
        # Make sure we leak no resources
        if not mswindows:
            max_handles = 1026 # too much for most UNIX systems
        else:
            max_handles = 2050 # too much for (at least some) Windows setups
        handles = []
        try:
            for i in range(max_handles):
                try:
                    handles.append(os.open(test_support.TESTFN,
                                           os.O_WRONLY | os.O_CREAT))
                except OSError as e:
                    if e.errno != errno.EMFILE:
                        raise
                    break
            else:
                self.skipTest("failed to reach the file descriptor limit "
                    "(tried %d)" % max_handles)
            # Close a couple of them (should be enough for a subprocess)
            for i in range(10):
                os.close(handles.pop())
            # Loop creating some subprocesses. If one of them leaks some fds,
            # the next loop iteration will fail by reaching the max fd limit.
            for i in range(15):
                p = subprocess.Popen([sys.executable, "-c",
                                      "import sys;"
                                      "sys.stdout.write(sys.stdin.read())"],
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     stderr=subprocess.PIPE)
                data = p.communicate(b"lime")[0]
                self.assertEqual(data, b"lime")
        finally:
            for h in handles:
                os.close(h)
            test_support.unlink(test_support.TESTFN)

    def test_list2cmdline(self):
        self.assertEqual(subprocess.list2cmdline(['a b c', 'd', 'e']),
                         '"a b c" d e')
        self.assertEqual(subprocess.list2cmdline(['ab"c', '\\', 'd']),
                         'ab\\"c \\ d')
        self.assertEqual(subprocess.list2cmdline(['ab"c', ' \\', 'd']),
                         'ab\\"c " \\\\" d')
        self.assertEqual(subprocess.list2cmdline(['a\\\\\\b', 'de fg', 'h']),
                         'a\\\\\\b "de fg" h')
        self.assertEqual(subprocess.list2cmdline(['a\\"b', 'c', 'd']),
                         'a\\\\\\"b c d')
        self.assertEqual(subprocess.list2cmdline(['a\\\\b c', 'd', 'e']),
                         '"a\\\\b c" d e')
        self.assertEqual(subprocess.list2cmdline(['a\\\\b\\ c', 'd', 'e']),
                         '"a\\\\b\\ c" d e')
        self.assertEqual(subprocess.list2cmdline(['ab', '']),
                         'ab ""')


    def test_poll(self):
        p = subprocess.Popen([sys.executable,
                          "-c", "import time; time.sleep(1)"])
        count = 0
        while p.poll() is None:
            time.sleep(0.1)
            count += 1
        # We expect that the poll loop probably went around about 10 times,
        # but, based on system scheduling we can't control, it's possible
        # poll() never returned None.  It "should be" very rare that it
        # didn't go around at least twice.
        self.assertGreaterEqual(count, 2)
        # Subsequent invocations should just return the returncode
        self.assertEqual(p.poll(), 0)


    def test_wait(self):
        p = subprocess.Popen([sys.executable,
                          "-c", "import time; time.sleep(2)"])
        self.assertEqual(p.wait(), 0)
        # Subsequent invocations should just return the returncode
        self.assertEqual(p.wait(), 0)


    def test_invalid_bufsize(self):
        # an invalid type of the bufsize argument should raise
        # TypeError.
        with self.assertRaises(TypeError):
            subprocess.Popen([sys.executable, "-c", "pass"], "orange")

    def test_leaking_fds_on_error(self):
        # see bug #5179: Popen leaks file descriptors to PIPEs if
        # the child fails to execute; this will eventually exhaust
        # the maximum number of open fds. 1024 seems a very common
        # value for that limit, but Windows has 2048, so we loop
        # 1024 times (each call leaked two fds).
        for i in range(1024):
            # Windows raises IOError.  Others raise OSError.
            with self.assertRaises(EnvironmentError) as c:
                subprocess.Popen(['nonexisting_i_hope'],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            # ignore errors that indicate the command was not found
            if c.exception.errno not in (errno.ENOENT, errno.EACCES):
                raise c.exception

    @unittest.skipIf(threading is None, "threading required")
    def test_double_close_on_error(self):
        # Issue #18851
        fds = []
        def open_fds():
            for i in range(20):
                fds.extend(os.pipe())
                time.sleep(0.001)
        t = threading.Thread(target=open_fds)
        t.start()
        try:
            with self.assertRaises(EnvironmentError):
                subprocess.Popen(['nonexisting_i_hope'],
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
        finally:
            t.join()
            exc = None
            for fd in fds:
                # If a double close occurred, some of those fds will
                # already have been closed by mistake, and os.close()
                # here will raise.
                try:
                    os.close(fd)
                except OSError as e:
                    exc = e
            if exc is not None:
                raise exc

    def test_handles_closed_on_exception(self):
        # If CreateProcess exits with an error, ensure the
        # duplicate output handles are released
        ifhandle, ifname = mkstemp()
        ofhandle, ofname = mkstemp()
        efhandle, efname = mkstemp()
        try:
            subprocess.Popen (["*"], stdin=ifhandle, stdout=ofhandle,
              stderr=efhandle)
        except OSError:
            os.close(ifhandle)
            os.remove(ifname)
            os.close(ofhandle)
            os.remove(ofname)
            os.close(efhandle)
            os.remove(efname)
        self.assertFalse(os.path.exists(ifname))
        self.assertFalse(os.path.exists(ofname))
        self.assertFalse(os.path.exists(efname))

    def test_communicate_epipe(self):
        # Issue 10963: communicate() should hide EPIPE
        p = subprocess.Popen([sys.executable, "-c", 'pass'],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        p.communicate("x" * 2**20)

    def test_communicate_epipe_only_stdin(self):
        # Issue 10963: communicate() should hide EPIPE
        p = subprocess.Popen([sys.executable, "-c", 'pass'],
                             stdin=subprocess.PIPE)
        self.addCleanup(p.stdin.close)
        time.sleep(2)
        p.communicate("x" * 2**20)

    # This test is Linux-ish specific for simplicity to at least have
    # some coverage.  It is not a platform specific bug.
    @unittest.skipUnless(os.path.isdir('/proc/%d/fd' % os.getpid()),
                         "Linux specific")
    def test_failed_child_execute_fd_leak(self):
        """Test for the fork() failure fd leak reported in issue16327."""
        fd_directory = '/proc/%d/fd' % os.getpid()
        fds_before_popen = os.listdir(fd_directory)
        with self.assertRaises(PopenTestException):
            PopenExecuteChildRaises(
                    [sys.executable, '-c', 'pass'], stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # NOTE: This test doesn't verify that the real _execute_child
        # does not close the file descriptors itself on the way out
        # during an exception.  Code inspection has confirmed that.

        fds_after_exception = os.listdir(fd_directory)
        self.assertEqual(fds_before_popen, fds_after_exception)


# context manager
class _SuppressCoreFiles(object):
    """Try to prevent core files from being created."""
    old_limit = None

    def __enter__(self):
        """Try to save previous ulimit, then set it to (0, 0)."""
        if resource is not None:
            try:
                self.old_limit = resource.getrlimit(resource.RLIMIT_CORE)
                resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
            except (ValueError, resource.error):
                pass

        if sys.platform == 'darwin':
            # Check if the 'Crash Reporter' on OSX was configured
            # in 'Developer' mode and warn that it will get triggered
            # when it is.
            #
            # This assumes that this context manager is used in tests
            # that might trigger the next manager.
            value = subprocess.Popen(['/usr/bin/defaults', 'read',
                    'com.apple.CrashReporter', 'DialogType'],
                    stdout=subprocess.PIPE).communicate()[0]
            if value.strip() == b'developer':
                print "this tests triggers the Crash Reporter, that is intentional"
                sys.stdout.flush()

    def __exit__(self, *args):
        """Return core file behavior to default."""
        if self.old_limit is None:
            return
        if resource is not None:
            try:
                resource.setrlimit(resource.RLIMIT_CORE, self.old_limit)
            except (ValueError, resource.error):
                pass

    @unittest.skipUnless(hasattr(signal, 'SIGALRM'),
                         "Requires signal.SIGALRM")
    def test_communicate_eintr(self):
        # Issue #12493: communicate() should handle EINTR
        def handler(signum, frame):
            pass
        old_handler = signal.signal(signal.SIGALRM, handler)
        self.addCleanup(signal.signal, signal.SIGALRM, old_handler)

        # the process is running for 2 seconds
        args = [sys.executable, "-c", 'import time; time.sleep(2)']
        for stream in ('stdout', 'stderr'):
            kw = {stream: subprocess.PIPE}
            with subprocess.Popen(args, **kw) as process:
                signal.alarm(1)
                # communicate() will be interrupted by SIGALRM
                process.communicate()


@unittest.skipIf(mswindows, "POSIX specific tests")
class POSIXProcessTestCase(BaseTestCase):

    def test_exceptions(self):
        # caught & re-raised exceptions
        with self.assertRaises(OSError) as c:
            p = subprocess.Popen([sys.executable, "-c", ""],
                                 cwd="/this/path/does/not/exist")
        # The attribute child_traceback should contain "os.chdir" somewhere.
        self.assertIn("os.chdir", c.exception.child_traceback)

    def test_run_abort(self):
        # returncode handles signal termination
        with _SuppressCoreFiles():
            p = subprocess.Popen([sys.executable, "-c",
                                  "import os; os.abort()"])
            p.wait()
        self.assertEqual(-p.returncode, signal.SIGABRT)

    def test_preexec(self):
        # preexec function
        p = subprocess.Popen([sys.executable, "-c",
                              "import sys, os;"
                              "sys.stdout.write(os.getenv('FRUIT'))"],
                             stdout=subprocess.PIPE,
                             preexec_fn=lambda: os.putenv("FRUIT", "apple"))
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read(), "apple")

    class _TestExecuteChildPopen(subprocess.Popen):
        """Used to test behavior at the end of _execute_child."""
        def __init__(self, testcase, *args, **kwargs):
            self._testcase = testcase
            subprocess.Popen.__init__(self, *args, **kwargs)

        def _execute_child(
                self, args, executable, preexec_fn, close_fds, cwd, env,
                universal_newlines, startupinfo, creationflags, shell, to_close,
                p2cread, p2cwrite,
                c2pread, c2pwrite,
                errread, errwrite):
            try:
                subprocess.Popen._execute_child(
                        self, args, executable, preexec_fn, close_fds,
                        cwd, env, universal_newlines,
                        startupinfo, creationflags, shell, to_close,
                        p2cread, p2cwrite,
                        c2pread, c2pwrite,
                        errread, errwrite)
            finally:
                # Open a bunch of file descriptors and verify that
                # none of them are the same as the ones the Popen
                # instance is using for stdin/stdout/stderr.
                devzero_fds = [os.open("/dev/zero", os.O_RDONLY)
                               for _ in range(8)]
                try:
                    for fd in devzero_fds:
                        self._testcase.assertNotIn(
                                fd, (p2cwrite, c2pread, errread))
                finally:
                    for fd in devzero_fds:
                        os.close(fd)

    @unittest.skipIf(not os.path.exists("/dev/zero"), "/dev/zero required.")
    def test_preexec_errpipe_does_not_double_close_pipes(self):
        """Issue16140: Don't double close pipes on preexec error."""

        def raise_it():
            raise RuntimeError("force the _execute_child() errpipe_data path.")

        with self.assertRaises(RuntimeError):
            self._TestExecuteChildPopen(
                    self, [sys.executable, "-c", "pass"],
                    stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE, preexec_fn=raise_it)

    def test_args_string(self):
        # args is a string
        f, fname = mkstemp()
        os.write(f, "#!/bin/sh\n")
        os.write(f, "exec '%s' -c 'import sys; sys.exit(47)'\n" %
                    sys.executable)
        os.close(f)
        os.chmod(fname, 0o700)
        p = subprocess.Popen(fname)
        p.wait()
        os.remove(fname)
        self.assertEqual(p.returncode, 47)

    def test_invalid_args(self):
        # invalid arguments should raise ValueError
        self.assertRaises(ValueError, subprocess.call,
                          [sys.executable, "-c",
                           "import sys; sys.exit(47)"],
                          startupinfo=47)
        self.assertRaises(ValueError, subprocess.call,
                          [sys.executable, "-c",
                           "import sys; sys.exit(47)"],
                          creationflags=47)

    def test_shell_sequence(self):
        # Run command through the shell (sequence)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "apple"
        p = subprocess.Popen(["echo $FRUIT"], shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read().strip(), "apple")

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "apple"
        p = subprocess.Popen("echo $FRUIT", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read().strip(), "apple")

    def test_call_string(self):
        # call() function with string argument on UNIX
        f, fname = mkstemp()
        os.write(f, "#!/bin/sh\n")
        os.write(f, "exec '%s' -c 'import sys; sys.exit(47)'\n" %
                    sys.executable)
        os.close(f)
        os.chmod(fname, 0700)
        rc = subprocess.call(fname)
        os.remove(fname)
        self.assertEqual(rc, 47)

    def test_specific_shell(self):
        # Issue #9265: Incorrect name passed as arg[0].
        shells = []
        for prefix in ['/bin', '/usr/bin/', '/usr/local/bin']:
            for name in ['bash', 'ksh']:
                sh = os.path.join(prefix, name)
                if os.path.isfile(sh):
                    shells.append(sh)
        if not shells: # Will probably work for any shell but csh.
            self.skipTest("bash or ksh required for this test")
        sh = '/bin/sh'
        if os.path.isfile(sh) and not os.path.islink(sh):
            # Test will fail if /bin/sh is a symlink to csh.
            shells.append(sh)
        for sh in shells:
            p = subprocess.Popen("echo $0", executable=sh, shell=True,
                                 stdout=subprocess.PIPE)
            self.addCleanup(p.stdout.close)
            self.assertEqual(p.stdout.read().strip(), sh)

    def _kill_process(self, method, *args):
        # Do not inherit file handles from the parent.
        # It should fix failures on some platforms.
        p = subprocess.Popen([sys.executable, "-c", """if 1:
                             import sys, time
                             sys.stdout.write('x\\n')
                             sys.stdout.flush()
                             time.sleep(30)
                             """],
                             close_fds=True,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        # Wait for the interpreter to be completely initialized before
        # sending any signal.
        p.stdout.read(1)
        getattr(p, method)(*args)
        return p

    @unittest.skipIf(sys.platform.startswith(('netbsd', 'openbsd')),
                     "Due to known OS bug (issue #16762)")
    def _kill_dead_process(self, method, *args):
        # Do not inherit file handles from the parent.
        # It should fix failures on some platforms.
        p = subprocess.Popen([sys.executable, "-c", """if 1:
                             import sys, time
                             sys.stdout.write('x\\n')
                             sys.stdout.flush()
                             """],
                             close_fds=True,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        # Wait for the interpreter to be completely initialized before
        # sending any signal.
        p.stdout.read(1)
        # The process should end after this
        time.sleep(1)
        # This shouldn't raise even though the child is now dead
        getattr(p, method)(*args)
        p.communicate()

    def test_send_signal(self):
        p = self._kill_process('send_signal', signal.SIGINT)
        _, stderr = p.communicate()
        self.assertIn('KeyboardInterrupt', stderr)
        self.assertNotEqual(p.wait(), 0)

    def test_kill(self):
        p = self._kill_process('kill')
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, '')
        self.assertEqual(p.wait(), -signal.SIGKILL)

    def test_terminate(self):
        p = self._kill_process('terminate')
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, '')
        self.assertEqual(p.wait(), -signal.SIGTERM)

    def test_send_signal_dead(self):
        # Sending a signal to a dead process
        self._kill_dead_process('send_signal', signal.SIGINT)

    def test_kill_dead(self):
        # Killing a dead process
        self._kill_dead_process('kill')

    def test_terminate_dead(self):
        # Terminating a dead process
        self._kill_dead_process('terminate')

    def check_close_std_fds(self, fds):
        # Issue #9905: test that subprocess pipes still work properly with
        # some standard fds closed
        stdin = 0
        newfds = []
        for a in fds:
            b = os.dup(a)
            newfds.append(b)
            if a == 0:
                stdin = b
        try:
            for fd in fds:
                os.close(fd)
            out, err = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdout.write("apple");'
                              'sys.stdout.flush();'
                              'sys.stderr.write("orange")'],
                       stdin=stdin,
                       stdout=subprocess.PIPE,
                       stderr=subprocess.PIPE).communicate()
            err = test_support.strip_python_stderr(err)
            self.assertEqual((out, err), (b'apple', b'orange'))
        finally:
            for b, a in zip(newfds, fds):
                os.dup2(b, a)
            for b in newfds:
                os.close(b)

    def test_close_fd_0(self):
        self.check_close_std_fds([0])

    def test_close_fd_1(self):
        self.check_close_std_fds([1])

    def test_close_fd_2(self):
        self.check_close_std_fds([2])

    def test_close_fds_0_1(self):
        self.check_close_std_fds([0, 1])

    def test_close_fds_0_2(self):
        self.check_close_std_fds([0, 2])

    def test_close_fds_1_2(self):
        self.check_close_std_fds([1, 2])

    def test_close_fds_0_1_2(self):
        # Issue #10806: test that subprocess pipes still work properly with
        # all standard fds closed.
        self.check_close_std_fds([0, 1, 2])

    def check_swap_fds(self, stdin_no, stdout_no, stderr_no):
        # open up some temporary files
        temps = [mkstemp() for i in range(3)]
        temp_fds = [fd for fd, fname in temps]
        try:
            # unlink the files -- we won't need to reopen them
            for fd, fname in temps:
                os.unlink(fname)

            # save a copy of the standard file descriptors
            saved_fds = [os.dup(fd) for fd in range(3)]
            try:
                # duplicate the temp files over the standard fd's 0, 1, 2
                for fd, temp_fd in enumerate(temp_fds):
                    os.dup2(temp_fd, fd)

                # write some data to what will become stdin, and rewind
                os.write(stdin_no, b"STDIN")
                os.lseek(stdin_no, 0, 0)

                # now use those files in the given order, so that subprocess
                # has to rearrange them in the child
                p = subprocess.Popen([sys.executable, "-c",
                    'import sys; got = sys.stdin.read();'
                    'sys.stdout.write("got %s"%got); sys.stderr.write("err")'],
                    stdin=stdin_no,
                    stdout=stdout_no,
                    stderr=stderr_no)
                p.wait()

                for fd in temp_fds:
                    os.lseek(fd, 0, 0)

                out = os.read(stdout_no, 1024)
                err = test_support.strip_python_stderr(os.read(stderr_no, 1024))
            finally:
                for std, saved in enumerate(saved_fds):
                    os.dup2(saved, std)
                    os.close(saved)

            self.assertEqual(out, b"got STDIN")
            self.assertEqual(err, b"err")

        finally:
            for fd in temp_fds:
                os.close(fd)

    # When duping fds, if there arises a situation where one of the fds is
    # either 0, 1 or 2, it is possible that it is overwritten (#12607).
    # This tests all combinations of this.
    def test_swap_fds(self):
        self.check_swap_fds(0, 1, 2)
        self.check_swap_fds(0, 2, 1)
        self.check_swap_fds(1, 0, 2)
        self.check_swap_fds(1, 2, 0)
        self.check_swap_fds(2, 0, 1)
        self.check_swap_fds(2, 1, 0)

    def test_wait_when_sigchild_ignored(self):
        # NOTE: sigchild_ignore.py may not be an effective test on all OSes.
        sigchild_ignore = test_support.findfile("sigchild_ignore.py",
                                                subdir="subprocessdata")
        p = subprocess.Popen([sys.executable, sigchild_ignore],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        self.assertEqual(0, p.returncode, "sigchild_ignore.py exited"
                         " non-zero with this error:\n%s" % stderr)

    def test_zombie_fast_process_del(self):
        # Issue #12650: on Unix, if Popen.__del__() was called before the
        # process exited, it wouldn't be added to subprocess._active, and would
        # remain a zombie.
        # spawn a Popen, and delete its reference before it exits
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys, time;'
                              'time.sleep(0.2)'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        ident = id(p)
        pid = p.pid
        del p
        # check that p is in the active processes list
        self.assertIn(ident, [id(o) for o in subprocess._active])

    def test_leak_fast_process_del_killed(self):
        # Issue #12650: on Unix, if Popen.__del__() was called before the
        # process exited, and the process got killed by a signal, it would never
        # be removed from subprocess._active, which triggered a FD and memory
        # leak.
        # spawn a Popen, delete its reference and kill it
        p = subprocess.Popen([sys.executable, "-c",
                              'import time;'
                              'time.sleep(3)'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        ident = id(p)
        pid = p.pid
        del p
        os.kill(pid, signal.SIGKILL)
        # check that p is in the active processes list
        self.assertIn(ident, [id(o) for o in subprocess._active])

        # let some time for the process to exit, and create a new Popen: this
        # should trigger the wait() of p
        time.sleep(0.2)
        with self.assertRaises(EnvironmentError) as c:
            with subprocess.Popen(['nonexisting_i_hope'],
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE) as proc:
                pass
        # p should have been wait()ed on, and removed from the _active list
        self.assertRaises(OSError, os.waitpid, pid, 0)
        self.assertNotIn(ident, [id(o) for o in subprocess._active])

    def test_pipe_cloexec(self):
        # Issue 12786: check that the communication pipes' FDs are set CLOEXEC,
        # and are not inherited by another child process.
        p1 = subprocess.Popen([sys.executable, "-c",
                               'import os;'
                               'os.read(0, 1)'
                              ],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE)

        p2 = subprocess.Popen([sys.executable, "-c", """if True:
                               import os, errno, sys
                               for fd in %r:
                                   try:
                                       os.close(fd)
                                   except OSError as e:
                                       if e.errno != errno.EBADF:
                                           raise
                                   else:
                                       sys.exit(1)
                               sys.exit(0)
                               """ % [f.fileno() for f in (p1.stdin, p1.stdout,
                                                           p1.stderr)]
                              ],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, close_fds=False)
        p1.communicate('foo')
        _, stderr = p2.communicate()

        self.assertEqual(p2.returncode, 0, "Unexpected error: " + repr(stderr))


@unittest.skipUnless(mswindows, "Windows specific tests")
class Win32ProcessTestCase(BaseTestCase):

    def test_startupinfo(self):
        # startupinfo argument
        # We uses hardcoded constants, because we do not want to
        # depend on win32all.
        STARTF_USESHOWWINDOW = 1
        SW_MAXIMIZE = 3
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags = STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = SW_MAXIMIZE
        # Since Python is a console process, it won't be affected
        # by wShowWindow, but the argument should be silently
        # ignored
        subprocess.call([sys.executable, "-c", "import sys; sys.exit(0)"],
                        startupinfo=startupinfo)

    def test_creationflags(self):
        # creationflags argument
        CREATE_NEW_CONSOLE = 16
        sys.stderr.write("    a DOS box should flash briefly ...\n")
        subprocess.call(sys.executable +
                        ' -c "import time; time.sleep(0.25)"',
                        creationflags=CREATE_NEW_CONSOLE)

    def test_invalid_args(self):
        # invalid arguments should raise ValueError
        self.assertRaises(ValueError, subprocess.call,
                          [sys.executable, "-c",
                           "import sys; sys.exit(47)"],
                          preexec_fn=lambda: 1)
        self.assertRaises(ValueError, subprocess.call,
                          [sys.executable, "-c",
                           "import sys; sys.exit(47)"],
                          stdout=subprocess.PIPE,
                          close_fds=True)

    def test_close_fds(self):
        # close file descriptors
        rc = subprocess.call([sys.executable, "-c",
                              "import sys; sys.exit(47)"],
                              close_fds=True)
        self.assertEqual(rc, 47)

    def test_shell_sequence(self):
        # Run command through the shell (sequence)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "physalis"
        p = subprocess.Popen(["set"], shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertIn("physalis", p.stdout.read())

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "physalis"
        p = subprocess.Popen("set", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertIn("physalis", p.stdout.read())

    def test_call_string(self):
        # call() function with string argument on Windows
        rc = subprocess.call(sys.executable +
                             ' -c "import sys; sys.exit(47)"')
        self.assertEqual(rc, 47)

    def _kill_process(self, method, *args):
        # Some win32 buildbot raises EOFError if stdin is inherited
        p = subprocess.Popen([sys.executable, "-c", """if 1:
                             import sys, time
                             sys.stdout.write('x\\n')
                             sys.stdout.flush()
                             time.sleep(30)
                             """],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        # Wait for the interpreter to be completely initialized before
        # sending any signal.
        p.stdout.read(1)
        getattr(p, method)(*args)
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, '')
        returncode = p.wait()
        self.assertNotEqual(returncode, 0)

    def _kill_dead_process(self, method, *args):
        p = subprocess.Popen([sys.executable, "-c", """if 1:
                             import sys, time
                             sys.stdout.write('x\\n')
                             sys.stdout.flush()
                             sys.exit(42)
                             """],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        # Wait for the interpreter to be completely initialized before
        # sending any signal.
        p.stdout.read(1)
        # The process should end after this
        time.sleep(1)
        # This shouldn't raise even though the child is now dead
        getattr(p, method)(*args)
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, b'')
        rc = p.wait()
        self.assertEqual(rc, 42)

    def test_send_signal(self):
        self._kill_process('send_signal', signal.SIGTERM)

    def test_kill(self):
        self._kill_process('kill')

    def test_terminate(self):
        self._kill_process('terminate')

    def test_send_signal_dead(self):
        self._kill_dead_process('send_signal', signal.SIGTERM)

    def test_kill_dead(self):
        self._kill_dead_process('kill')

    def test_terminate_dead(self):
        self._kill_dead_process('terminate')


@unittest.skipUnless(getattr(subprocess, '_has_poll', False),
                     "poll system call not supported")
class ProcessTestCaseNoPoll(ProcessTestCase):
    def setUp(self):
        subprocess._has_poll = False
        ProcessTestCase.setUp(self)

    def tearDown(self):
        subprocess._has_poll = True
        ProcessTestCase.tearDown(self)


class HelperFunctionTests(unittest.TestCase):
    @unittest.skipIf(mswindows, "errno and EINTR make no sense on windows")
    def test_eintr_retry_call(self):
        record_calls = []
        def fake_os_func(*args):
            record_calls.append(args)
            if len(record_calls) == 2:
                raise OSError(errno.EINTR, "fake interrupted system call")
            return tuple(reversed(args))

        self.assertEqual((999, 256),
                         subprocess._eintr_retry_call(fake_os_func, 256, 999))
        self.assertEqual([(256, 999)], record_calls)
        # This time there will be an EINTR so it will loop once.
        self.assertEqual((666,),
                         subprocess._eintr_retry_call(fake_os_func, 666))
        self.assertEqual([(256, 999), (666,), (666,)], record_calls)

@unittest.skipUnless(mswindows, "mswindows only")
class CommandsWithSpaces (BaseTestCase):

    def setUp(self):
        super(CommandsWithSpaces, self).setUp()
        f, fname = mkstemp(".py", "te st")
        self.fname = fname.lower ()
        os.write(f, b"import sys;"
                    b"sys.stdout.write('%d %s' % (len(sys.argv), [a.lower () for a in sys.argv]))"
        )
        os.close(f)

    def tearDown(self):
        os.remove(self.fname)
        super(CommandsWithSpaces, self).tearDown()

    def with_spaces(self, *args, **kwargs):
        kwargs['stdout'] = subprocess.PIPE
        p = subprocess.Popen(*args, **kwargs)
        self.addCleanup(p.stdout.close)
        self.assertEqual(
          p.stdout.read ().decode("mbcs"),
          "2 [%r, 'ab cd']" % self.fname
        )

    def test_shell_string_with_spaces(self):
        # call() function with string argument with spaces on Windows
        self.with_spaces('"%s" "%s" "%s"' % (sys.executable, self.fname,
                                             "ab cd"), shell=1)

    def test_shell_sequence_with_spaces(self):
        # call() function with sequence argument with spaces on Windows
        self.with_spaces([sys.executable, self.fname, "ab cd"], shell=1)

    def test_noshell_string_with_spaces(self):
        # call() function with string argument with spaces on Windows
        self.with_spaces('"%s" "%s" "%s"' % (sys.executable, self.fname,
                             "ab cd"))

    def test_noshell_sequence_with_spaces(self):
        # call() function with sequence argument with spaces on Windows
        self.with_spaces([sys.executable, self.fname, "ab cd"])

def test_main():
    unit_tests = (ProcessTestCase,
                  POSIXProcessTestCase,
                  Win32ProcessTestCase,
                  ProcessTestCaseNoPoll,
                  HelperFunctionTests,
                  CommandsWithSpaces)

    test_support.run_unittest(*unit_tests)
    test_support.reap_children()

if __name__ == "__main__":
    test_main()
