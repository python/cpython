import unittest
from unittest import mock
from test import support
from test.support import import_helper
from test.support import os_helper
from test.support import warnings_helper
import subprocess
import sys
import signal
import io
import itertools
import os
import errno
import tempfile
import time
import traceback
import types
import selectors
import sysconfig
import select
import shutil
import threading
import gc
import textwrap
import json
import pathlib
from test.support.os_helper import FakePath

try:
    import _testcapi
except ImportError:
    _testcapi = None

try:
    import pwd
except ImportError:
    pwd = None
try:
    import grp
except ImportError:
    grp = None

try:
    import fcntl
except:
    fcntl = None

if support.PGO:
    raise unittest.SkipTest("test is not helpful for PGO")

if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")

mswindows = (sys.platform == "win32")

#
# Depends on the following external programs: Python
#

if mswindows:
    SETBINARY = ('import msvcrt; msvcrt.setmode(sys.stdout.fileno(), '
                                                'os.O_BINARY);')
else:
    SETBINARY = ''

NONEXISTING_CMD = ('nonexisting_i_hope',)
# Ignore errors that indicate the command was not found
NONEXISTING_ERRORS = (FileNotFoundError, NotADirectoryError, PermissionError)

ZERO_RETURN_CMD = (sys.executable, '-c', 'pass')


def setUpModule():
    shell_true = shutil.which('true')
    if shell_true is None:
        return
    if (os.access(shell_true, os.X_OK) and
        subprocess.run([shell_true]).returncode == 0):
        global ZERO_RETURN_CMD
        ZERO_RETURN_CMD = (shell_true,)  # Faster than Python startup.


class BaseTestCase(unittest.TestCase):
    def setUp(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        support.reap_children()

    def tearDown(self):
        if not mswindows:
            # subprocess._active is not used on Windows and is set to None.
            for inst in subprocess._active:
                inst.wait()
            subprocess._cleanup()
            self.assertFalse(
                subprocess._active, "subprocess._active not empty"
            )
        self.doCleanups()
        support.reap_children()


class PopenTestException(Exception):
    pass


class PopenExecuteChildRaises(subprocess.Popen):
    """Popen subclass for testing cleanup of subprocess.PIPE filehandles when
    _execute_child fails.
    """
    def _execute_child(self, *args, **kwargs):
        raise PopenTestException("Forced Exception for Test")


class ProcessTestCase(BaseTestCase):

    def test_io_buffered_by_default(self):
        p = subprocess.Popen(ZERO_RETURN_CMD,
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        try:
            self.assertIsInstance(p.stdin, io.BufferedIOBase)
            self.assertIsInstance(p.stdout, io.BufferedIOBase)
            self.assertIsInstance(p.stderr, io.BufferedIOBase)
        finally:
            p.stdin.close()
            p.stdout.close()
            p.stderr.close()
            p.wait()

    def test_io_unbuffered_works(self):
        p = subprocess.Popen(ZERO_RETURN_CMD,
                             stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE, bufsize=0)
        try:
            self.assertIsInstance(p.stdin, io.RawIOBase)
            self.assertIsInstance(p.stdout, io.RawIOBase)
            self.assertIsInstance(p.stderr, io.RawIOBase)
        finally:
            p.stdin.close()
            p.stdout.close()
            p.stderr.close()
            p.wait()

    def test_call_seq(self):
        # call() function with sequence argument
        rc = subprocess.call([sys.executable, "-c",
                              "import sys; sys.exit(47)"])
        self.assertEqual(rc, 47)

    def test_call_timeout(self):
        # call() function with timeout argument; we want to test that the child
        # process gets killed when the timeout expires.  If the child isn't
        # killed, this call will deadlock since subprocess.call waits for the
        # child.
        self.assertRaises(subprocess.TimeoutExpired, subprocess.call,
                          [sys.executable, "-c", "while True: pass"],
                          timeout=0.1)

    def test_check_call_zero(self):
        # check_call() function with zero return code
        rc = subprocess.check_call(ZERO_RETURN_CMD)
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
                [sys.executable, "-c", "print('BDFL')"])
        self.assertIn(b'BDFL', output)

        with self.assertRaisesRegex(ValueError,
                "stdout argument not allowed, it will be overridden"):
            subprocess.check_output([], stdout=None)

        with self.assertRaisesRegex(ValueError,
                "check argument not allowed, it will be overridden"):
            subprocess.check_output([], check=False)

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
        self.assertIn(b'BDFL', output)

    def test_check_output_stdin_arg(self):
        # check_output() can be called with stdin set to a file
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        tf.write(b'pear')
        tf.seek(0)
        output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; sys.stdout.write(sys.stdin.read().upper())"],
                stdin=tf)
        self.assertIn(b'PEAR', output)

    def test_check_output_input_arg(self):
        # check_output() can be called with input set to a string
        output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; sys.stdout.write(sys.stdin.read().upper())"],
                input=b'pear')
        self.assertIn(b'PEAR', output)

    def test_check_output_input_none(self):
        """input=None has a legacy meaning of input='' on check_output."""
        output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; print('XX' if sys.stdin.read() else '')"],
                input=None)
        self.assertNotIn(b'XX', output)

    def test_check_output_input_none_text(self):
        output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; print('XX' if sys.stdin.read() else '')"],
                input=None, text=True)
        self.assertNotIn('XX', output)

    def test_check_output_input_none_universal_newlines(self):
        output = subprocess.check_output(
                [sys.executable, "-c",
                 "import sys; print('XX' if sys.stdin.read() else '')"],
                input=None, universal_newlines=True)
        self.assertNotIn('XX', output)

    def test_check_output_input_none_encoding_errors(self):
        output = subprocess.check_output(
                [sys.executable, "-c", "print('foo')"],
                input=None, encoding='utf-8', errors='ignore')
        self.assertIn('foo', output)

    def test_check_output_stdout_arg(self):
        # check_output() refuses to accept 'stdout' argument
        with self.assertRaises(ValueError) as c:
            output = subprocess.check_output(
                    [sys.executable, "-c", "print('will not be run')"],
                    stdout=sys.stdout)
            self.fail("Expected ValueError when stdout arg supplied.")
        self.assertIn('stdout', c.exception.args[0])

    def test_check_output_stdin_with_input_arg(self):
        # check_output() refuses to accept 'stdin' with 'input'
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        tf.write(b'pear')
        tf.seek(0)
        with self.assertRaises(ValueError) as c:
            output = subprocess.check_output(
                    [sys.executable, "-c", "print('will not be run')"],
                    stdin=tf, input=b'hare')
            self.fail("Expected ValueError when stdin and input args supplied.")
        self.assertIn('stdin', c.exception.args[0])
        self.assertIn('input', c.exception.args[0])

    def test_check_output_timeout(self):
        # check_output() function with timeout arg
        with self.assertRaises(subprocess.TimeoutExpired) as c:
            output = subprocess.check_output(
                    [sys.executable, "-c",
                     "import sys, time\n"
                     "sys.stdout.write('BDFL')\n"
                     "sys.stdout.flush()\n"
                     "time.sleep(3600)"],
                    # Some heavily loaded buildbots (sparc Debian 3.x) require
                    # this much time to start and print.
                    timeout=3)
            self.fail("Expected TimeoutExpired.")
        self.assertEqual(c.exception.output, b'BDFL')

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
        with support.captured_stderr() as s:
            self.assertRaises(TypeError, subprocess.Popen, invalid_arg_name=1)
            argcount = subprocess.Popen.__init__.__code__.co_argcount
            too_many_args = [0] * (argcount + 1)
            self.assertRaises(TypeError, subprocess.Popen, *too_many_args)
        self.assertEqual(s.getvalue(), '')

    def test_stdin_none(self):
        # .stdin is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print("banana")'],
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
                'p = Popen([sys.executable, "-c", "print(\'test_stdout_none\')"],'
                '          stdin=PIPE, stderr=PIPE);'
                'p.wait(); assert p.stdout is None;')
        p = subprocess.Popen([sys.executable, "-c", code],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        out, err = p.communicate()
        self.assertEqual(p.returncode, 0, err)
        self.assertEqual(out.rstrip(), b'test_stdout_none')

    def test_stderr_none(self):
        # .stderr is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print("banana")'],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stdin.close)
        p.wait()
        self.assertEqual(p.stderr, None)

    def _assert_python(self, pre_args, **kwargs):
        # We include sys.exit() to prevent the test runner from hanging
        # whenever python is found.
        args = pre_args + ["import sys; sys.exit(47)"]
        p = subprocess.Popen(args, **kwargs)
        p.wait()
        self.assertEqual(47, p.returncode)

    def test_executable(self):
        # Check that the executable argument works.
        #
        # On Unix (non-Mac and non-Windows), Python looks at args[0] to
        # determine where its standard library is, so we need the directory
        # of args[0] to be valid for the Popen() call to Python to succeed.
        # See also issue #16170 and issue #7774.
        doesnotexist = os.path.join(os.path.dirname(sys.executable),
                                    "doesnotexist")
        self._assert_python([doesnotexist, "-c"], executable=sys.executable)

    def test_bytes_executable(self):
        doesnotexist = os.path.join(os.path.dirname(sys.executable),
                                    "doesnotexist")
        self._assert_python([doesnotexist, "-c"],
                            executable=os.fsencode(sys.executable))

    def test_pathlike_executable(self):
        doesnotexist = os.path.join(os.path.dirname(sys.executable),
                                    "doesnotexist")
        self._assert_python([doesnotexist, "-c"],
                            executable=FakePath(sys.executable))

    def test_executable_takes_precedence(self):
        # Check that the executable argument takes precedence over args[0].
        #
        # Verify first that the call succeeds without the executable arg.
        pre_args = [sys.executable, "-c"]
        self._assert_python(pre_args)
        self.assertRaises(NONEXISTING_ERRORS,
                          self._assert_python, pre_args,
                          executable=NONEXISTING_CMD[0])

    @unittest.skipIf(mswindows, "executable argument replaces shell")
    def test_executable_replaces_shell(self):
        # Check that the executable argument replaces the default shell
        # when shell=True.
        self._assert_python([], executable=sys.executable, shell=True)

    @unittest.skipIf(mswindows, "executable argument replaces shell")
    def test_bytes_executable_replaces_shell(self):
        self._assert_python([], executable=os.fsencode(sys.executable),
                            shell=True)

    @unittest.skipIf(mswindows, "executable argument replaces shell")
    def test_pathlike_executable_replaces_shell(self):
        self._assert_python([], executable=FakePath(sys.executable),
                            shell=True)

    # For use in the test_cwd* tests below.
    def _normalize_cwd(self, cwd):
        # Normalize an expected cwd (for Tru64 support).
        # We can't use os.path.realpath since it doesn't expand Tru64 {memb}
        # strings.  See bug #1063571.
        with os_helper.change_cwd(cwd):
            return os.getcwd()

    # For use in the test_cwd* tests below.
    def _split_python_path(self):
        # Return normalized (python_dir, python_base).
        python_path = os.path.realpath(sys.executable)
        return os.path.split(python_path)

    # For use in the test_cwd* tests below.
    def _assert_cwd(self, expected_cwd, python_arg, **kwargs):
        # Invoke Python via Popen, and assert that (1) the call succeeds,
        # and that (2) the current working directory of the child process
        # matches *expected_cwd*.
        p = subprocess.Popen([python_arg, "-c",
                              "import os, sys; "
                              "buf = sys.stdout.buffer; "
                              "buf.write(os.getcwd().encode()); "
                              "buf.flush(); "
                              "sys.exit(47)"],
                              stdout=subprocess.PIPE,
                              **kwargs)
        self.addCleanup(p.stdout.close)
        p.wait()
        self.assertEqual(47, p.returncode)
        normcase = os.path.normcase
        self.assertEqual(normcase(expected_cwd),
                         normcase(p.stdout.read().decode()))

    def test_cwd(self):
        # Check that cwd changes the cwd for the child process.
        temp_dir = tempfile.gettempdir()
        temp_dir = self._normalize_cwd(temp_dir)
        self._assert_cwd(temp_dir, sys.executable, cwd=temp_dir)

    def test_cwd_with_bytes(self):
        temp_dir = tempfile.gettempdir()
        temp_dir = self._normalize_cwd(temp_dir)
        self._assert_cwd(temp_dir, sys.executable, cwd=os.fsencode(temp_dir))

    def test_cwd_with_pathlike(self):
        temp_dir = tempfile.gettempdir()
        temp_dir = self._normalize_cwd(temp_dir)
        self._assert_cwd(temp_dir, sys.executable, cwd=FakePath(temp_dir))

    @unittest.skipIf(mswindows, "pending resolution of issue #15533")
    def test_cwd_with_relative_arg(self):
        # Check that Popen looks for args[0] relative to cwd if args[0]
        # is relative.
        python_dir, python_base = self._split_python_path()
        rel_python = os.path.join(os.curdir, python_base)
        with os_helper.temp_cwd() as wrong_dir:
            # Before calling with the correct cwd, confirm that the call fails
            # without cwd and with the wrong cwd.
            self.assertRaises(FileNotFoundError, subprocess.Popen,
                              [rel_python])
            self.assertRaises(FileNotFoundError, subprocess.Popen,
                              [rel_python], cwd=wrong_dir)
            python_dir = self._normalize_cwd(python_dir)
            self._assert_cwd(python_dir, rel_python, cwd=python_dir)

    @unittest.skipIf(mswindows, "pending resolution of issue #15533")
    def test_cwd_with_relative_executable(self):
        # Check that Popen looks for executable relative to cwd if executable
        # is relative (and that executable takes precedence over args[0]).
        python_dir, python_base = self._split_python_path()
        rel_python = os.path.join(os.curdir, python_base)
        doesntexist = "somethingyoudonthave"
        with os_helper.temp_cwd() as wrong_dir:
            # Before calling with the correct cwd, confirm that the call fails
            # without cwd and with the wrong cwd.
            self.assertRaises(FileNotFoundError, subprocess.Popen,
                              [doesntexist], executable=rel_python)
            self.assertRaises(FileNotFoundError, subprocess.Popen,
                              [doesntexist], executable=rel_python,
                              cwd=wrong_dir)
            python_dir = self._normalize_cwd(python_dir)
            self._assert_cwd(python_dir, doesntexist, executable=rel_python,
                             cwd=python_dir)

    def test_cwd_with_absolute_arg(self):
        # Check that Popen can find the executable when the cwd is wrong
        # if args[0] is an absolute path.
        python_dir, python_base = self._split_python_path()
        abs_python = os.path.join(python_dir, python_base)
        rel_python = os.path.join(os.curdir, python_base)
        with os_helper.temp_dir() as wrong_dir:
            # Before calling with an absolute path, confirm that using a
            # relative path fails.
            self.assertRaises(FileNotFoundError, subprocess.Popen,
                              [rel_python], cwd=wrong_dir)
            wrong_dir = self._normalize_cwd(wrong_dir)
            self._assert_cwd(wrong_dir, abs_python, cwd=wrong_dir)

    @unittest.skipIf(sys.base_prefix != sys.prefix,
                     'Test is not venv-compatible')
    def test_executable_with_cwd(self):
        python_dir, python_base = self._split_python_path()
        python_dir = self._normalize_cwd(python_dir)
        self._assert_cwd(python_dir, "somethingyoudonthave",
                         executable=sys.executable, cwd=python_dir)

    @unittest.skipIf(sys.base_prefix != sys.prefix,
                     'Test is not venv-compatible')
    @unittest.skipIf(sysconfig.is_python_build(),
                     "need an installed Python. See #7774")
    def test_executable_without_cwd(self):
        # For a normal installation, it should work without 'cwd'
        # argument.  For test runs in the build directory, see #7774.
        self._assert_cwd(os.getcwd(), "somethingyoudonthave",
                         executable=sys.executable)

    def test_stdin_pipe(self):
        # stdin redirection
        p = subprocess.Popen([sys.executable, "-c",
                         'import sys; sys.exit(sys.stdin.read() == "pear")'],
                        stdin=subprocess.PIPE)
        p.stdin.write(b"pear")
        p.stdin.close()
        p.wait()
        self.assertEqual(p.returncode, 1)

    def test_stdin_filedes(self):
        # stdin is set to open file descriptor
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        d = tf.fileno()
        os.write(d, b"pear")
        os.lseek(d, 0, 0)
        p = subprocess.Popen([sys.executable, "-c",
                         'import sys; sys.exit(sys.stdin.read() == "pear")'],
                         stdin=d)
        p.wait()
        self.assertEqual(p.returncode, 1)

    def test_stdin_fileobj(self):
        # stdin is set to open file object
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        tf.write(b"pear")
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
        with p:
            self.assertEqual(p.stdout.read(), b"orange")

    def test_stdout_filedes(self):
        # stdout is set to open file descriptor
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        d = tf.fileno()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stdout.write("orange")'],
                         stdout=d)
        p.wait()
        os.lseek(d, 0, 0)
        self.assertEqual(os.read(d, 1024), b"orange")

    def test_stdout_fileobj(self):
        # stdout is set to open file object
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stdout.write("orange")'],
                         stdout=tf)
        p.wait()
        tf.seek(0)
        self.assertEqual(tf.read(), b"orange")

    def test_stderr_pipe(self):
        # stderr redirection
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=subprocess.PIPE)
        with p:
            self.assertEqual(p.stderr.read(), b"strawberry")

    def test_stderr_filedes(self):
        # stderr is set to open file descriptor
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        d = tf.fileno()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=d)
        p.wait()
        os.lseek(d, 0, 0)
        self.assertEqual(os.read(d, 1024), b"strawberry")

    def test_stderr_fileobj(self):
        # stderr is set to open file object
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertEqual(tf.read(), b"strawberry")

    def test_stderr_redirect_with_no_stdout_redirect(self):
        # test stderr=STDOUT while stdout=None (not set)

        # - grandchild prints to stderr
        # - child redirects grandchild's stderr to its stdout
        # - the parent should get grandchild's stderr in child's stdout
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys, subprocess;'
                              'rc = subprocess.call([sys.executable, "-c",'
                              '    "import sys;"'
                              '    "sys.stderr.write(\'42\')"],'
                              '    stderr=subprocess.STDOUT);'
                              'sys.exit(rc)'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        #NOTE: stdout should get stderr from grandchild
        self.assertEqual(stdout, b'42')
        self.assertEqual(stderr, b'') # should be empty
        self.assertEqual(p.returncode, 0)

    def test_stdout_stderr_pipe(self):
        # capture stdout and stderr to the same pipe
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdout.write("apple");'
                              'sys.stdout.flush();'
                              'sys.stderr.write("orange")'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        with p:
            self.assertEqual(p.stdout.read(), b"appleorange")

    def test_stdout_stderr_file(self):
        # capture stdout and stderr to the same open file
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdout.write("apple");'
                              'sys.stdout.flush();'
                              'sys.stderr.write("orange")'],
                             stdout=tf,
                             stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertEqual(tf.read(), b"appleorange")

    def test_stdout_filedes_of_stdout(self):
        # stdout is set to 1 (#1531862).
        # To avoid printing the text on stdout, we do something similar to
        # test_stdout_none (see above).  The parent subprocess calls the child
        # subprocess passing stdout=1, and this test uses stdout=PIPE in
        # order to capture and check the output of the parent. See #11963.
        code = ('import sys, subprocess; '
                'rc = subprocess.call([sys.executable, "-c", '
                '    "import os, sys; sys.exit(os.write(sys.stdout.fileno(), '
                     'b\'test with stdout=1\'))"], stdout=1); '
                'assert rc == 18')
        p = subprocess.Popen([sys.executable, "-c", code],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        out, err = p.communicate()
        self.assertEqual(p.returncode, 0, err)
        self.assertEqual(out.rstrip(), b'test with stdout=1')

    def test_stdout_devnull(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'for i in range(10240):'
                              'print("x" * 1024)'],
                              stdout=subprocess.DEVNULL)
        p.wait()
        self.assertEqual(p.stdout, None)

    def test_stderr_devnull(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys\n'
                              'for i in range(10240):'
                              'sys.stderr.write("x" * 1024)'],
                              stderr=subprocess.DEVNULL)
        p.wait()
        self.assertEqual(p.stderr, None)

    def test_stdin_devnull(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdin.read(1)'],
                              stdin=subprocess.DEVNULL)
        p.wait()
        self.assertEqual(p.stdin, None)

    @unittest.skipUnless(fcntl and hasattr(fcntl, 'F_GETPIPE_SZ'),
                         'fcntl.F_GETPIPE_SZ required for test.')
    def test_pipesizes(self):
        test_pipe_r, test_pipe_w = os.pipe()
        try:
            # Get the default pipesize with F_GETPIPE_SZ
            pipesize_default = fcntl.fcntl(test_pipe_w, fcntl.F_GETPIPE_SZ)
        finally:
            os.close(test_pipe_r)
            os.close(test_pipe_w)
        pipesize = pipesize_default // 2
        if pipesize < 512:  # the POSIX minimum
            raise unittest.SkitTest(
                'default pipesize too small to perform test.')
        p = subprocess.Popen(
            [sys.executable, "-c",
             'import sys; sys.stdin.read(); sys.stdout.write("out"); '
             'sys.stderr.write("error!")'],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, pipesize=pipesize)
        try:
            for fifo in [p.stdin, p.stdout, p.stderr]:
                self.assertEqual(
                    fcntl.fcntl(fifo.fileno(), fcntl.F_GETPIPE_SZ),
                    pipesize)
            # Windows pipe size can be acquired via GetNamedPipeInfoFunction
            # https://docs.microsoft.com/en-us/windows/win32/api/namedpipeapi/nf-namedpipeapi-getnamedpipeinfo
            # However, this function is not yet in _winapi.
            p.stdin.write(b"pear")
            p.stdin.close()
            p.stdout.close()
            p.stderr.close()
        finally:
            p.kill()
            p.wait()

    @unittest.skipUnless(fcntl and hasattr(fcntl, 'F_GETPIPE_SZ'),
                         'fcntl.F_GETPIPE_SZ required for test.')
    def test_pipesize_default(self):
        p = subprocess.Popen(
            [sys.executable, "-c",
             'import sys; sys.stdin.read(); sys.stdout.write("out"); '
             'sys.stderr.write("error!")'],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, pipesize=-1)
        try:
            fp_r, fp_w = os.pipe()
            try:
                default_pipesize = fcntl.fcntl(fp_w, fcntl.F_GETPIPE_SZ)
                for fifo in [p.stdin, p.stdout, p.stderr]:
                    self.assertEqual(
                        fcntl.fcntl(fifo.fileno(), fcntl.F_GETPIPE_SZ),
                        default_pipesize)
            finally:
                os.close(fp_r)
                os.close(fp_w)
            # On other platforms we cannot test the pipe size (yet). But above
            # code using pipesize=-1 should not crash.
            p.stdin.close()
            p.stdout.close()
            p.stderr.close()
        finally:
            p.kill()
            p.wait()

    def test_env(self):
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange"
        with subprocess.Popen([sys.executable, "-c",
                               'import sys,os;'
                               'sys.stdout.write(os.getenv("FRUIT"))'],
                              stdout=subprocess.PIPE,
                              env=newenv) as p:
            stdout, stderr = p.communicate()
            self.assertEqual(stdout, b"orange")

    # Windows requires at least the SYSTEMROOT environment variable to start
    # Python
    @unittest.skipIf(sys.platform == 'win32',
                     'cannot test an empty env on Windows')
    @unittest.skipIf(sysconfig.get_config_var('Py_ENABLE_SHARED') == 1,
                     'The Python shared library cannot be loaded '
                     'with an empty environment.')
    def test_empty_env(self):
        """Verify that env={} is as empty as possible."""

        def is_env_var_to_ignore(n):
            """Determine if an environment variable is under our control."""
            # This excludes some __CF_* and VERSIONER_* keys MacOS insists
            # on adding even when the environment in exec is empty.
            # Gentoo sandboxes also force LD_PRELOAD and SANDBOX_* to exist.
            return ('VERSIONER' in n or '__CF' in n or  # MacOS
                    n == 'LD_PRELOAD' or n.startswith('SANDBOX') or # Gentoo
                    n == 'LC_CTYPE') # Locale coercion triggered

        with subprocess.Popen([sys.executable, "-c",
                               'import os; print(list(os.environ.keys()))'],
                              stdout=subprocess.PIPE, env={}) as p:
            stdout, stderr = p.communicate()
            child_env_names = eval(stdout.strip())
            self.assertIsInstance(child_env_names, list)
            child_env_names = [k for k in child_env_names
                               if not is_env_var_to_ignore(k)]
            self.assertEqual(child_env_names, [])

    def test_invalid_cmd(self):
        # null character in the command name
        cmd = sys.executable + '\0'
        with self.assertRaises(ValueError):
            subprocess.Popen([cmd, "-c", "pass"])

        # null character in the command argument
        with self.assertRaises(ValueError):
            subprocess.Popen([sys.executable, "-c", "pass#\0"])

    def test_invalid_env(self):
        # null character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT\0VEGETABLE"] = "cabbage"
        with self.assertRaises(ValueError):
            subprocess.Popen(ZERO_RETURN_CMD, env=newenv)

        # null character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange\0VEGETABLE=cabbage"
        with self.assertRaises(ValueError):
            subprocess.Popen(ZERO_RETURN_CMD, env=newenv)

        # equal character in the environment variable name
        newenv = os.environ.copy()
        newenv["FRUIT=ORANGE"] = "lemon"
        with self.assertRaises(ValueError):
            subprocess.Popen(ZERO_RETURN_CMD, env=newenv)

        # equal character in the environment variable value
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange=lemon"
        with subprocess.Popen([sys.executable, "-c",
                               'import sys, os;'
                               'sys.stdout.write(os.getenv("FRUIT"))'],
                              stdout=subprocess.PIPE,
                              env=newenv) as p:
            stdout, stderr = p.communicate()
            self.assertEqual(stdout, b"orange=lemon")

    def test_communicate_stdin(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.exit(sys.stdin.read() == "pear")'],
                             stdin=subprocess.PIPE)
        p.communicate(b"pear")
        self.assertEqual(p.returncode, 1)

    def test_communicate_stdout(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys; sys.stdout.write("pineapple")'],
                             stdout=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, b"pineapple")
        self.assertEqual(stderr, None)

    def test_communicate_stderr(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys; sys.stderr.write("pineapple")'],
                             stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, None)
        self.assertEqual(stderr, b"pineapple")

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
        (stdout, stderr) = p.communicate(b"banana")
        self.assertEqual(stdout, b"banana")
        self.assertEqual(stderr, b"pineapple")

    def test_communicate_timeout(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os,time;'
                              'sys.stderr.write("pineapple\\n");'
                              'time.sleep(1);'
                              'sys.stderr.write("pear\\n");'
                              'sys.stdout.write(sys.stdin.read())'],
                             universal_newlines=True,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.assertRaises(subprocess.TimeoutExpired, p.communicate, "banana",
                          timeout=0.3)
        # Make sure we can keep waiting for it, and that we get the whole output
        # after it completes.
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, "banana")
        self.assertEqual(stderr.encode(), b"pineapple\npear\n")

    def test_communicate_timeout_large_output(self):
        # Test an expiring timeout while the child is outputting lots of data.
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os,time;'
                              'sys.stdout.write("a" * (64 * 1024));'
                              'time.sleep(0.2);'
                              'sys.stdout.write("a" * (64 * 1024));'
                              'time.sleep(0.2);'
                              'sys.stdout.write("a" * (64 * 1024));'
                              'time.sleep(0.2);'
                              'sys.stdout.write("a" * (64 * 1024));'],
                             stdout=subprocess.PIPE)
        self.assertRaises(subprocess.TimeoutExpired, p.communicate, timeout=0.4)
        (stdout, _) = p.communicate()
        self.assertEqual(len(stdout), 4 * 64 * 1024)

    # Test for the fd leak reported in http://bugs.python.org/issue2791.
    def test_communicate_pipe_fd_leak(self):
        for stdin_pipe in (False, True):
            for stdout_pipe in (False, True):
                for stderr_pipe in (False, True):
                    options = {}
                    if stdin_pipe:
                        options['stdin'] = subprocess.PIPE
                    if stdout_pipe:
                        options['stdout'] = subprocess.PIPE
                    if stderr_pipe:
                        options['stderr'] = subprocess.PIPE
                    if not options:
                        continue
                    p = subprocess.Popen(ZERO_RETURN_CMD, **options)
                    p.communicate()
                    if p.stdin is not None:
                        self.assertTrue(p.stdin.closed)
                    if p.stdout is not None:
                        self.assertTrue(p.stdout.closed)
                    if p.stderr is not None:
                        self.assertTrue(p.stderr.closed)

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
        os.close(x)
        os.close(y)
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stdout.write(sys.stdin.read(47));'
                              'sys.stderr.write("x" * %d);'
                              'sys.stdout.write(sys.stdin.read())' %
                              support.PIPE_MAX_SIZE],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        string_to_write = b"a" * support.PIPE_MAX_SIZE
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
        p.stdin.write(b"banana")
        (stdout, stderr) = p.communicate(b"split")
        self.assertEqual(stdout, b"bananasplit")
        self.assertEqual(stderr, b"")

    def test_universal_newlines_and_text(self):
        args = [
            sys.executable, "-c",
            'import sys,os;' + SETBINARY +
            'buf = sys.stdout.buffer;'
            'buf.write(sys.stdin.readline().encode());'
            'buf.flush();'
            'buf.write(b"line2\\n");'
            'buf.flush();'
            'buf.write(sys.stdin.read().encode());'
            'buf.flush();'
            'buf.write(b"line4\\n");'
            'buf.flush();'
            'buf.write(b"line5\\r\\n");'
            'buf.flush();'
            'buf.write(b"line6\\r");'
            'buf.flush();'
            'buf.write(b"\\nline7");'
            'buf.flush();'
            'buf.write(b"\\nline8");']

        for extra_kwarg in ('universal_newlines', 'text'):
            p = subprocess.Popen(args, **{'stdin': subprocess.PIPE,
                                          'stdout': subprocess.PIPE,
                                          extra_kwarg: True})
            with p:
                p.stdin.write("line1\n")
                p.stdin.flush()
                self.assertEqual(p.stdout.readline(), "line1\n")
                p.stdin.write("line3\n")
                p.stdin.close()
                self.addCleanup(p.stdout.close)
                self.assertEqual(p.stdout.readline(),
                                 "line2\n")
                self.assertEqual(p.stdout.read(6),
                                 "line3\n")
                self.assertEqual(p.stdout.read(),
                                 "line4\nline5\nline6\nline7\nline8")

    def test_universal_newlines_communicate(self):
        # universal newlines through communicate()
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY +
                              'buf = sys.stdout.buffer;'
                              'buf.write(b"line2\\n");'
                              'buf.flush();'
                              'buf.write(b"line4\\n");'
                              'buf.flush();'
                              'buf.write(b"line5\\r\\n");'
                              'buf.flush();'
                              'buf.write(b"line6\\r");'
                              'buf.flush();'
                              'buf.write(b"\\nline7");'
                              'buf.flush();'
                              'buf.write(b"\\nline8");'],
                             stderr=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             universal_newlines=1)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout,
                         "line2\nline4\nline5\nline6\nline7\nline8")

    def test_universal_newlines_communicate_stdin(self):
        # universal newlines through communicate(), with only stdin
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY + textwrap.dedent('''
                               s = sys.stdin.readline()
                               assert s == "line1\\n", repr(s)
                               s = sys.stdin.read()
                               assert s == "line3\\n", repr(s)
                              ''')],
                             stdin=subprocess.PIPE,
                             universal_newlines=1)
        (stdout, stderr) = p.communicate("line1\nline3\n")
        self.assertEqual(p.returncode, 0)

    def test_universal_newlines_communicate_input_none(self):
        # Test communicate(input=None) with universal newlines.
        #
        # We set stdout to PIPE because, as of this writing, a different
        # code path is tested when the number of pipes is zero or one.
        p = subprocess.Popen(ZERO_RETURN_CMD,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             universal_newlines=True)
        p.communicate()
        self.assertEqual(p.returncode, 0)

    def test_universal_newlines_communicate_stdin_stdout_stderr(self):
        # universal newlines through communicate(), with stdin, stdout, stderr
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY + textwrap.dedent('''
                               s = sys.stdin.buffer.readline()
                               sys.stdout.buffer.write(s)
                               sys.stdout.buffer.write(b"line2\\r")
                               sys.stderr.buffer.write(b"eline2\\n")
                               s = sys.stdin.buffer.read()
                               sys.stdout.buffer.write(s)
                               sys.stdout.buffer.write(b"line4\\n")
                               sys.stdout.buffer.write(b"line5\\r\\n")
                               sys.stderr.buffer.write(b"eline6\\r")
                               sys.stderr.buffer.write(b"eline7\\r\\nz")
                              ''')],
                             stdin=subprocess.PIPE,
                             stderr=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             universal_newlines=True)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        (stdout, stderr) = p.communicate("line1\nline3\n")
        self.assertEqual(p.returncode, 0)
        self.assertEqual("line1\nline2\nline3\nline4\nline5\n", stdout)
        # Python debug build push something like "[42442 refs]\n"
        # to stderr at exit of subprocess.
        self.assertTrue(stderr.startswith("eline2\neline6\neline7\n"))

    def test_universal_newlines_communicate_encodings(self):
        # Check that universal newlines mode works for various encodings,
        # in particular for encodings in the UTF-16 and UTF-32 families.
        # See issue #15595.
        #
        # UTF-16 and UTF-32-BE are sufficient to check both with BOM and
        # without, and UTF-16 and UTF-32.
        for encoding in ['utf-16', 'utf-32-be']:
            code = ("import sys; "
                    r"sys.stdout.buffer.write('1\r\n2\r3\n4'.encode('%s'))" %
                    encoding)
            args = [sys.executable, '-c', code]
            # We set stdin to be non-None because, as of this writing,
            # a different code path is used when the number of pipes is
            # zero or one.
            popen = subprocess.Popen(args,
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     encoding=encoding)
            stdout, stderr = popen.communicate(input='')
            self.assertEqual(stdout, '1\n2\n3\n4')

    def test_communicate_errors(self):
        for errors, expected in [
            ('ignore', ''),
            ('replace', '\ufffd\ufffd'),
            ('surrogateescape', '\udc80\udc80'),
            ('backslashreplace', '\\x80\\x80'),
        ]:
            code = ("import sys; "
                    r"sys.stdout.buffer.write(b'[\x80\x80]')")
            args = [sys.executable, '-c', code]
            # We set stdin to be non-None because, as of this writing,
            # a different code path is used when the number of pipes is
            # zero or one.
            popen = subprocess.Popen(args,
                                     stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE,
                                     encoding='utf-8',
                                     errors=errors)
            stdout, stderr = popen.communicate(input='')
            self.assertEqual(stdout, '[{}]'.format(expected))

    def test_no_leaking(self):
        # Make sure we leak no resources
        if not mswindows:
            max_handles = 1026 # too much for most UNIX systems
        else:
            max_handles = 2050 # too much for (at least some) Windows setups
        handles = []
        tmpdir = tempfile.mkdtemp()
        try:
            for i in range(max_handles):
                try:
                    tmpfile = os.path.join(tmpdir, os_helper.TESTFN)
                    handles.append(os.open(tmpfile, os.O_WRONLY|os.O_CREAT))
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
            shutil.rmtree(tmpdir)

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
        p = subprocess.Popen([sys.executable, "-c",
                              "import os; os.read(0, 1)"],
                             stdin=subprocess.PIPE)
        self.addCleanup(p.stdin.close)
        self.assertIsNone(p.poll())
        os.write(p.stdin.fileno(), b'A')
        p.wait()
        # Subsequent invocations should just return the returncode
        self.assertEqual(p.poll(), 0)

    def test_wait(self):
        p = subprocess.Popen(ZERO_RETURN_CMD)
        self.assertEqual(p.wait(), 0)
        # Subsequent invocations should just return the returncode
        self.assertEqual(p.wait(), 0)

    def test_wait_timeout(self):
        p = subprocess.Popen([sys.executable,
                              "-c", "import time; time.sleep(0.3)"])
        with self.assertRaises(subprocess.TimeoutExpired) as c:
            p.wait(timeout=0.0001)
        self.assertIn("0.0001", str(c.exception))  # For coverage of __str__.
        self.assertEqual(p.wait(timeout=support.SHORT_TIMEOUT), 0)

    def test_invalid_bufsize(self):
        # an invalid type of the bufsize argument should raise
        # TypeError.
        with self.assertRaises(TypeError):
            subprocess.Popen(ZERO_RETURN_CMD, "orange")

    def test_bufsize_is_none(self):
        # bufsize=None should be the same as bufsize=0.
        p = subprocess.Popen(ZERO_RETURN_CMD, None)
        self.assertEqual(p.wait(), 0)
        # Again with keyword arg
        p = subprocess.Popen(ZERO_RETURN_CMD, bufsize=None)
        self.assertEqual(p.wait(), 0)

    def _test_bufsize_equal_one(self, line, expected, universal_newlines):
        # subprocess may deadlock with bufsize=1, see issue #21332
        with subprocess.Popen([sys.executable, "-c", "import sys;"
                               "sys.stdout.write(sys.stdin.readline());"
                               "sys.stdout.flush()"],
                              stdin=subprocess.PIPE,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.DEVNULL,
                              bufsize=1,
                              universal_newlines=universal_newlines) as p:
            p.stdin.write(line) # expect that it flushes the line in text mode
            os.close(p.stdin.fileno()) # close it without flushing the buffer
            read_line = p.stdout.readline()
            with support.SuppressCrashReport():
                try:
                    p.stdin.close()
                except OSError:
                    pass
            p.stdin = None
        self.assertEqual(p.returncode, 0)
        self.assertEqual(read_line, expected)

    def test_bufsize_equal_one_text_mode(self):
        # line is flushed in text mode with bufsize=1.
        # we should get the full line in return
        line = "line\n"
        self._test_bufsize_equal_one(line, line, universal_newlines=True)

    def test_bufsize_equal_one_binary_mode(self):
        # line is not flushed in binary mode with bufsize=1.
        # we should get empty response
        line = b'line' + os.linesep.encode() # assume ascii-based locale
        with self.assertWarnsRegex(RuntimeWarning, 'line buffering'):
            self._test_bufsize_equal_one(line, b'', universal_newlines=False)

    def test_leaking_fds_on_error(self):
        # see bug #5179: Popen leaks file descriptors to PIPEs if
        # the child fails to execute; this will eventually exhaust
        # the maximum number of open fds. 1024 seems a very common
        # value for that limit, but Windows has 2048, so we loop
        # 1024 times (each call leaked two fds).
        for i in range(1024):
            with self.assertRaises(NONEXISTING_ERRORS):
                subprocess.Popen(NONEXISTING_CMD,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)

    def test_nonexisting_with_pipes(self):
        # bpo-30121: Popen with pipes must close properly pipes on error.
        # Previously, os.close() was called with a Windows handle which is not
        # a valid file descriptor.
        #
        # Run the test in a subprocess to control how the CRT reports errors
        # and to get stderr content.
        try:
            import msvcrt
            msvcrt.CrtSetReportMode
        except (AttributeError, ImportError):
            self.skipTest("need msvcrt.CrtSetReportMode")

        code = textwrap.dedent(f"""
            import msvcrt
            import subprocess

            cmd = {NONEXISTING_CMD!r}

            for report_type in [msvcrt.CRT_WARN,
                                msvcrt.CRT_ERROR,
                                msvcrt.CRT_ASSERT]:
                msvcrt.CrtSetReportMode(report_type, msvcrt.CRTDBG_MODE_FILE)
                msvcrt.CrtSetReportFile(report_type, msvcrt.CRTDBG_FILE_STDERR)

            try:
                subprocess.Popen(cmd,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            except OSError:
                pass
        """)
        cmd = [sys.executable, "-c", code]
        proc = subprocess.Popen(cmd,
                                stderr=subprocess.PIPE,
                                universal_newlines=True)
        with proc:
            stderr = proc.communicate()[1]
        self.assertEqual(stderr, "")
        self.assertEqual(proc.returncode, 0)

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
                subprocess.Popen(NONEXISTING_CMD,
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

    def test_threadsafe_wait(self):
        """Issue21291: Popen.wait() needs to be threadsafe for returncode."""
        proc = subprocess.Popen([sys.executable, '-c',
                                 'import time; time.sleep(12)'])
        self.assertEqual(proc.returncode, None)
        results = []

        def kill_proc_timer_thread():
            results.append(('thread-start-poll-result', proc.poll()))
            # terminate it from the thread and wait for the result.
            proc.kill()
            proc.wait()
            results.append(('thread-after-kill-and-wait', proc.returncode))
            # this wait should be a no-op given the above.
            proc.wait()
            results.append(('thread-after-second-wait', proc.returncode))

        # This is a timing sensitive test, the failure mode is
        # triggered when both the main thread and this thread are in
        # the wait() call at once.  The delay here is to allow the
        # main thread to most likely be blocked in its wait() call.
        t = threading.Timer(0.2, kill_proc_timer_thread)
        t.start()

        if mswindows:
            expected_errorcode = 1
        else:
            # Should be -9 because of the proc.kill() from the thread.
            expected_errorcode = -9

        # Wait for the process to finish; the thread should kill it
        # long before it finishes on its own.  Supplying a timeout
        # triggers a different code path for better coverage.
        proc.wait(timeout=support.SHORT_TIMEOUT)
        self.assertEqual(proc.returncode, expected_errorcode,
                         msg="unexpected result in wait from main thread")

        # This should be a no-op with no change in returncode.
        proc.wait()
        self.assertEqual(proc.returncode, expected_errorcode,
                         msg="unexpected result in second main wait.")

        t.join()
        # Ensure that all of the thread results are as expected.
        # When a race condition occurs in wait(), the returncode could
        # be set by the wrong thread that doesn't actually have it
        # leading to an incorrect value.
        self.assertEqual([('thread-start-poll-result', None),
                          ('thread-after-kill-and-wait', expected_errorcode),
                          ('thread-after-second-wait', expected_errorcode)],
                         results)

    def test_issue8780(self):
        # Ensure that stdout is inherited from the parent
        # if stdout=PIPE is not used
        code = ';'.join((
            'import subprocess, sys',
            'retcode = subprocess.call('
                "[sys.executable, '-c', 'print(\"Hello World!\")'])",
            'assert retcode == 0'))
        output = subprocess.check_output([sys.executable, '-c', code])
        self.assertTrue(output.startswith(b'Hello World!'), ascii(output))

    def test_handles_closed_on_exception(self):
        # If CreateProcess exits with an error, ensure the
        # duplicate output handles are released
        ifhandle, ifname = tempfile.mkstemp()
        ofhandle, ofname = tempfile.mkstemp()
        efhandle, efname = tempfile.mkstemp()
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
        p = subprocess.Popen(ZERO_RETURN_CMD,
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stderr.close)
        self.addCleanup(p.stdin.close)
        p.communicate(b"x" * 2**20)

    def test_repr(self):
        path_cmd = pathlib.Path("my-tool.py")
        pathlib_cls = path_cmd.__class__.__name__

        cases = [
            ("ls", True, 123, "<Popen: returncode: 123 args: 'ls'>"),
            ('a' * 100, True, 0,
             "<Popen: returncode: 0 args: 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa...>"),
            (["ls"], False, None, "<Popen: returncode: None args: ['ls']>"),
            (["ls", '--my-opts', 'a' * 100], False, None,
             "<Popen: returncode: None args: ['ls', '--my-opts', 'aaaaaaaaaaaaaaaaaaaaaaaa...>"),
            (path_cmd, False, 7, f"<Popen: returncode: 7 args: {pathlib_cls}('my-tool.py')>")
        ]
        with unittest.mock.patch.object(subprocess.Popen, '_execute_child'):
            for cmd, shell, code, sx in cases:
                p = subprocess.Popen(cmd, shell=shell)
                p.returncode = code
                self.assertEqual(repr(p), sx)

    def test_communicate_epipe_only_stdin(self):
        # Issue 10963: communicate() should hide EPIPE
        p = subprocess.Popen(ZERO_RETURN_CMD,
                             stdin=subprocess.PIPE)
        self.addCleanup(p.stdin.close)
        p.wait()
        p.communicate(b"x" * 2**20)

    @unittest.skipUnless(hasattr(signal, 'SIGUSR1'),
                         "Requires signal.SIGUSR1")
    @unittest.skipUnless(hasattr(os, 'kill'),
                         "Requires os.kill")
    @unittest.skipUnless(hasattr(os, 'getppid'),
                         "Requires os.getppid")
    def test_communicate_eintr(self):
        # Issue #12493: communicate() should handle EINTR
        def handler(signum, frame):
            pass
        old_handler = signal.signal(signal.SIGUSR1, handler)
        self.addCleanup(signal.signal, signal.SIGUSR1, old_handler)

        args = [sys.executable, "-c",
                'import os, signal;'
                'os.kill(os.getppid(), signal.SIGUSR1)']
        for stream in ('stdout', 'stderr'):
            kw = {stream: subprocess.PIPE}
            with subprocess.Popen(args, **kw) as process:
                # communicate() will be interrupted by SIGUSR1
                process.communicate()


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
                    ZERO_RETURN_CMD, stdin=subprocess.PIPE,
                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)

        # NOTE: This test doesn't verify that the real _execute_child
        # does not close the file descriptors itself on the way out
        # during an exception.  Code inspection has confirmed that.

        fds_after_exception = os.listdir(fd_directory)
        self.assertEqual(fds_before_popen, fds_after_exception)

    @unittest.skipIf(mswindows, "behavior currently not supported on Windows")
    def test_file_not_found_includes_filename(self):
        with self.assertRaises(FileNotFoundError) as c:
            subprocess.call(['/opt/nonexistent_binary', 'with', 'some', 'args'])
        self.assertEqual(c.exception.filename, '/opt/nonexistent_binary')

    @unittest.skipIf(mswindows, "behavior currently not supported on Windows")
    def test_file_not_found_with_bad_cwd(self):
        with self.assertRaises(FileNotFoundError) as c:
            subprocess.Popen(['exit', '0'], cwd='/some/nonexistent/directory')
        self.assertEqual(c.exception.filename, '/some/nonexistent/directory')

    def test_class_getitems(self):
        self.assertIsInstance(subprocess.Popen[bytes], types.GenericAlias)
        self.assertIsInstance(subprocess.CompletedProcess[str], types.GenericAlias)

    @unittest.skipIf(not sysconfig.get_config_var("HAVE_VFORK"),
                     "vfork() not enabled by configure.")
    @mock.patch("subprocess._fork_exec")
    def test__use_vfork(self, mock_fork_exec):
        self.assertTrue(subprocess._USE_VFORK)  # The default value regardless.
        mock_fork_exec.side_effect = RuntimeError("just testing args")
        with self.assertRaises(RuntimeError):
            subprocess.run([sys.executable, "-c", "pass"])
        mock_fork_exec.assert_called_once()
        self.assertTrue(mock_fork_exec.call_args.args[-1])
        with mock.patch.object(subprocess, '_USE_VFORK', False):
            with self.assertRaises(RuntimeError):
                subprocess.run([sys.executable, "-c", "pass"])
            self.assertFalse(mock_fork_exec.call_args_list[-1].args[-1])


class RunFuncTestCase(BaseTestCase):
    def run_python(self, code, **kwargs):
        """Run Python code in a subprocess using subprocess.run"""
        argv = [sys.executable, "-c", code]
        return subprocess.run(argv, **kwargs)

    def test_returncode(self):
        # call() function with sequence argument
        cp = self.run_python("import sys; sys.exit(47)")
        self.assertEqual(cp.returncode, 47)
        with self.assertRaises(subprocess.CalledProcessError):
            cp.check_returncode()

    def test_check(self):
        with self.assertRaises(subprocess.CalledProcessError) as c:
            self.run_python("import sys; sys.exit(47)", check=True)
        self.assertEqual(c.exception.returncode, 47)

    def test_check_zero(self):
        # check_returncode shouldn't raise when returncode is zero
        cp = subprocess.run(ZERO_RETURN_CMD, check=True)
        self.assertEqual(cp.returncode, 0)

    def test_timeout(self):
        # run() function with timeout argument; we want to test that the child
        # process gets killed when the timeout expires.  If the child isn't
        # killed, this call will deadlock since subprocess.run waits for the
        # child.
        with self.assertRaises(subprocess.TimeoutExpired):
            self.run_python("while True: pass", timeout=0.0001)

    def test_capture_stdout(self):
        # capture stdout with zero return code
        cp = self.run_python("print('BDFL')", stdout=subprocess.PIPE)
        self.assertIn(b'BDFL', cp.stdout)

    def test_capture_stderr(self):
        cp = self.run_python("import sys; sys.stderr.write('BDFL')",
                             stderr=subprocess.PIPE)
        self.assertIn(b'BDFL', cp.stderr)

    def test_check_output_stdin_arg(self):
        # run() can be called with stdin set to a file
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        tf.write(b'pear')
        tf.seek(0)
        cp = self.run_python(
                 "import sys; sys.stdout.write(sys.stdin.read().upper())",
                stdin=tf, stdout=subprocess.PIPE)
        self.assertIn(b'PEAR', cp.stdout)

    def test_check_output_input_arg(self):
        # check_output() can be called with input set to a string
        cp = self.run_python(
                "import sys; sys.stdout.write(sys.stdin.read().upper())",
                input=b'pear', stdout=subprocess.PIPE)
        self.assertIn(b'PEAR', cp.stdout)

    def test_check_output_stdin_with_input_arg(self):
        # run() refuses to accept 'stdin' with 'input'
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        tf.write(b'pear')
        tf.seek(0)
        with self.assertRaises(ValueError,
              msg="Expected ValueError when stdin and input args supplied.") as c:
            output = self.run_python("print('will not be run')",
                                     stdin=tf, input=b'hare')
        self.assertIn('stdin', c.exception.args[0])
        self.assertIn('input', c.exception.args[0])

    def test_check_output_timeout(self):
        with self.assertRaises(subprocess.TimeoutExpired) as c:
            cp = self.run_python((
                     "import sys, time\n"
                     "sys.stdout.write('BDFL')\n"
                     "sys.stdout.flush()\n"
                     "time.sleep(3600)"),
                    # Some heavily loaded buildbots (sparc Debian 3.x) require
                    # this much time to start and print.
                    timeout=3, stdout=subprocess.PIPE)
        self.assertEqual(c.exception.output, b'BDFL')
        # output is aliased to stdout
        self.assertEqual(c.exception.stdout, b'BDFL')

    def test_run_kwargs(self):
        newenv = os.environ.copy()
        newenv["FRUIT"] = "banana"
        cp = self.run_python(('import sys, os;'
                      'sys.exit(33 if os.getenv("FRUIT")=="banana" else 31)'),
                             env=newenv)
        self.assertEqual(cp.returncode, 33)

    def test_run_with_pathlike_path(self):
        # bpo-31961: test run(pathlike_object)
        # the name of a command that can be run without
        # any arguments that exit fast
        prog = 'tree.com' if mswindows else 'ls'
        path = shutil.which(prog)
        if path is None:
            self.skipTest(f'{prog} required for this test')
        path = FakePath(path)
        res = subprocess.run(path, stdout=subprocess.DEVNULL)
        self.assertEqual(res.returncode, 0)
        with self.assertRaises(TypeError):
            subprocess.run(path, stdout=subprocess.DEVNULL, shell=True)

    def test_run_with_bytes_path_and_arguments(self):
        # bpo-31961: test run([bytes_object, b'additional arguments'])
        path = os.fsencode(sys.executable)
        args = [path, '-c', b'import sys; sys.exit(57)']
        res = subprocess.run(args)
        self.assertEqual(res.returncode, 57)

    def test_run_with_pathlike_path_and_arguments(self):
        # bpo-31961: test run([pathlike_object, 'additional arguments'])
        path = FakePath(sys.executable)
        args = [path, '-c', 'import sys; sys.exit(57)']
        res = subprocess.run(args)
        self.assertEqual(res.returncode, 57)

    def test_capture_output(self):
        cp = self.run_python(("import sys;"
                              "sys.stdout.write('BDFL'); "
                              "sys.stderr.write('FLUFL')"),
                             capture_output=True)
        self.assertIn(b'BDFL', cp.stdout)
        self.assertIn(b'FLUFL', cp.stderr)

    def test_stdout_with_capture_output_arg(self):
        # run() refuses to accept 'stdout' with 'capture_output'
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        with self.assertRaises(ValueError,
            msg=("Expected ValueError when stdout and capture_output "
                 "args supplied.")) as c:
            output = self.run_python("print('will not be run')",
                                      capture_output=True, stdout=tf)
        self.assertIn('stdout', c.exception.args[0])
        self.assertIn('capture_output', c.exception.args[0])

    def test_stderr_with_capture_output_arg(self):
        # run() refuses to accept 'stderr' with 'capture_output'
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        with self.assertRaises(ValueError,
            msg=("Expected ValueError when stderr and capture_output "
                 "args supplied.")) as c:
            output = self.run_python("print('will not be run')",
                                      capture_output=True, stderr=tf)
        self.assertIn('stderr', c.exception.args[0])
        self.assertIn('capture_output', c.exception.args[0])

    # This test _might_ wind up a bit fragile on loaded build+test machines
    # as it depends on the timing with wide enough margins for normal situations
    # but does assert that it happened "soon enough" to believe the right thing
    # happened.
    @unittest.skipIf(mswindows, "requires posix like 'sleep' shell command")
    def test_run_with_shell_timeout_and_capture_output(self):
        """Output capturing after a timeout mustn't hang forever on open filehandles."""
        before_secs = time.monotonic()
        try:
            subprocess.run('sleep 3', shell=True, timeout=0.1,
                           capture_output=True)  # New session unspecified.
        except subprocess.TimeoutExpired as exc:
            after_secs = time.monotonic()
            stacks = traceback.format_exc()  # assertRaises doesn't give this.
        else:
            self.fail("TimeoutExpired not raised.")
        self.assertLess(after_secs - before_secs, 1.5,
                        msg="TimeoutExpired was delayed! Bad traceback:\n```\n"
                        f"{stacks}```")

    def test_encoding_warning(self):
        code = textwrap.dedent("""\
            from subprocess import *
            run("echo hello", shell=True, text=True)
            check_output("echo hello", shell=True, text=True)
            """)
        cp = subprocess.run([sys.executable, "-Xwarn_default_encoding", "-c", code],
                            capture_output=True)
        lines = cp.stderr.splitlines()
        self.assertEqual(len(lines), 2, lines)
        self.assertTrue(lines[0].startswith(b"<string>:2: EncodingWarning: "))
        self.assertTrue(lines[1].startswith(b"<string>:3: EncodingWarning: "))


def _get_test_grp_name():
    for name_group in ('staff', 'nogroup', 'grp', 'nobody', 'nfsnobody'):
        if grp:
            try:
                grp.getgrnam(name_group)
            except KeyError:
                continue
            return name_group
    else:
        raise unittest.SkipTest('No identified group name to use for this test on this platform.')


@unittest.skipIf(mswindows, "POSIX specific tests")
class POSIXProcessTestCase(BaseTestCase):

    def setUp(self):
        super().setUp()
        self._nonexistent_dir = "/_this/pa.th/does/not/exist"

    def _get_chdir_exception(self):
        try:
            os.chdir(self._nonexistent_dir)
        except OSError as e:
            # This avoids hard coding the errno value or the OS perror()
            # string and instead capture the exception that we want to see
            # below for comparison.
            desired_exception = e
        else:
            self.fail("chdir to nonexistent directory %s succeeded." %
                      self._nonexistent_dir)
        return desired_exception

    def test_exception_cwd(self):
        """Test error in the child raised in the parent for a bad cwd."""
        desired_exception = self._get_chdir_exception()
        try:
            p = subprocess.Popen([sys.executable, "-c", ""],
                                 cwd=self._nonexistent_dir)
        except OSError as e:
            # Test that the child process chdir failure actually makes
            # it up to the parent process as the correct exception.
            self.assertEqual(desired_exception.errno, e.errno)
            self.assertEqual(desired_exception.strerror, e.strerror)
            self.assertEqual(desired_exception.filename, e.filename)
        else:
            self.fail("Expected OSError: %s" % desired_exception)

    def test_exception_bad_executable(self):
        """Test error in the child raised in the parent for a bad executable."""
        desired_exception = self._get_chdir_exception()
        try:
            p = subprocess.Popen([sys.executable, "-c", ""],
                                 executable=self._nonexistent_dir)
        except OSError as e:
            # Test that the child process exec failure actually makes
            # it up to the parent process as the correct exception.
            self.assertEqual(desired_exception.errno, e.errno)
            self.assertEqual(desired_exception.strerror, e.strerror)
            self.assertEqual(desired_exception.filename, e.filename)
        else:
            self.fail("Expected OSError: %s" % desired_exception)

    def test_exception_bad_args_0(self):
        """Test error in the child raised in the parent for a bad args[0]."""
        desired_exception = self._get_chdir_exception()
        try:
            p = subprocess.Popen([self._nonexistent_dir, "-c", ""])
        except OSError as e:
            # Test that the child process exec failure actually makes
            # it up to the parent process as the correct exception.
            self.assertEqual(desired_exception.errno, e.errno)
            self.assertEqual(desired_exception.strerror, e.strerror)
            self.assertEqual(desired_exception.filename, e.filename)
        else:
            self.fail("Expected OSError: %s" % desired_exception)

    # We mock the __del__ method for Popen in the next two tests
    # because it does cleanup based on the pid returned by fork_exec
    # along with issuing a resource warning if it still exists. Since
    # we don't actually spawn a process in these tests we can forego
    # the destructor. An alternative would be to set _child_created to
    # False before the destructor is called but there is no easy way
    # to do that
    class PopenNoDestructor(subprocess.Popen):
        def __del__(self):
            pass

    @mock.patch("subprocess._fork_exec")
    def test_exception_errpipe_normal(self, fork_exec):
        """Test error passing done through errpipe_write in the good case"""
        def proper_error(*args):
            errpipe_write = args[13]
            # Write the hex for the error code EISDIR: 'is a directory'
            err_code = '{:x}'.format(errno.EISDIR).encode()
            os.write(errpipe_write, b"OSError:" + err_code + b":")
            return 0

        fork_exec.side_effect = proper_error

        with mock.patch("subprocess.os.waitpid",
                        side_effect=ChildProcessError):
            with self.assertRaises(IsADirectoryError):
                self.PopenNoDestructor(["non_existent_command"])

    @mock.patch("subprocess._fork_exec")
    def test_exception_errpipe_bad_data(self, fork_exec):
        """Test error passing done through errpipe_write where its not
        in the expected format"""
        error_data = b"\xFF\x00\xDE\xAD"
        def bad_error(*args):
            errpipe_write = args[13]
            # Anything can be in the pipe, no assumptions should
            # be made about its encoding, so we'll write some
            # arbitrary hex bytes to test it out
            os.write(errpipe_write, error_data)
            return 0

        fork_exec.side_effect = bad_error

        with mock.patch("subprocess.os.waitpid",
                        side_effect=ChildProcessError):
            with self.assertRaises(subprocess.SubprocessError) as e:
                self.PopenNoDestructor(["non_existent_command"])

        self.assertIn(repr(error_data), str(e.exception))

    @unittest.skipIf(not os.path.exists('/proc/self/status'),
                     "need /proc/self/status")
    def test_restore_signals(self):
        # Blindly assume that cat exists on systems with /proc/self/status...
        default_proc_status = subprocess.check_output(
                ['cat', '/proc/self/status'],
                restore_signals=False)
        for line in default_proc_status.splitlines():
            if line.startswith(b'SigIgn'):
                default_sig_ign_mask = line
                break
        else:
            self.skipTest("SigIgn not found in /proc/self/status.")
        restored_proc_status = subprocess.check_output(
                ['cat', '/proc/self/status'],
                restore_signals=True)
        for line in restored_proc_status.splitlines():
            if line.startswith(b'SigIgn'):
                restored_sig_ign_mask = line
                break
        self.assertNotEqual(default_sig_ign_mask, restored_sig_ign_mask,
                            msg="restore_signals=True should've unblocked "
                            "SIGPIPE and friends.")

    def test_start_new_session(self):
        # For code coverage of calling setsid().  We don't care if we get an
        # EPERM error from it depending on the test execution environment, that
        # still indicates that it was called.
        try:
            output = subprocess.check_output(
                    [sys.executable, "-c", "import os; print(os.getsid(0))"],
                    start_new_session=True)
        except PermissionError as e:
            if e.errno != errno.EPERM:
                raise  # EACCES?
        else:
            parent_sid = os.getsid(0)
            child_sid = int(output)
            self.assertNotEqual(parent_sid, child_sid)

    @unittest.skipUnless(hasattr(os, 'setpgid') and hasattr(os, 'getpgid'),
                         'no setpgid or getpgid on platform')
    def test_process_group_0(self):
        # For code coverage of calling setpgid().  We don't care if we get an
        # EPERM error from it depending on the test execution environment, that
        # still indicates that it was called.
        try:
            output = subprocess.check_output(
                    [sys.executable, "-c", "import os; print(os.getpgid(0))"],
                    process_group=0)
        except PermissionError as e:
            if e.errno != errno.EPERM:
                raise  # EACCES?
        else:
            parent_pgid = os.getpgid(0)
            child_pgid = int(output)
            self.assertNotEqual(parent_pgid, child_pgid)

    @unittest.skipUnless(hasattr(os, 'setreuid'), 'no setreuid on platform')
    def test_user(self):
        # For code coverage of the user parameter.  We don't care if we get an
        # EPERM error from it depending on the test execution environment, that
        # still indicates that it was called.

        uid = os.geteuid()
        test_users = [65534 if uid != 65534 else 65533, uid]
        name_uid = "nobody" if sys.platform != 'darwin' else "unknown"

        if pwd is not None:
            try:
                pwd.getpwnam(name_uid)
                test_users.append(name_uid)
            except KeyError:
                # unknown user name
                name_uid = None

        for user in test_users:
            # posix_spawn() may be used with close_fds=False
            for close_fds in (False, True):
                with self.subTest(user=user, close_fds=close_fds):
                    try:
                        output = subprocess.check_output(
                                [sys.executable, "-c",
                                 "import os; print(os.getuid())"],
                                user=user,
                                close_fds=close_fds)
                    except PermissionError:  # (EACCES, EPERM)
                        pass
                    except OSError as e:
                        if e.errno not in (errno.EACCES, errno.EPERM):
                            raise
                    else:
                        if isinstance(user, str):
                            user_uid = pwd.getpwnam(user).pw_uid
                        else:
                            user_uid = user
                        child_user = int(output)
                        self.assertEqual(child_user, user_uid)

        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, user=-1)

        with self.assertRaises(OverflowError):
            subprocess.check_call(ZERO_RETURN_CMD,
                                  cwd=os.curdir, env=os.environ, user=2**64)

        if pwd is None and name_uid is not None:
            with self.assertRaises(ValueError):
                subprocess.check_call(ZERO_RETURN_CMD, user=name_uid)

    @unittest.skipIf(hasattr(os, 'setreuid'), 'setreuid() available on platform')
    def test_user_error(self):
        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, user=65535)

    @unittest.skipUnless(hasattr(os, 'setregid'), 'no setregid() on platform')
    def test_group(self):
        gid = os.getegid()
        group_list = [65534 if gid != 65534 else 65533]
        name_group = _get_test_grp_name()

        if grp is not None:
            group_list.append(name_group)

        for group in group_list + [gid]:
            # posix_spawn() may be used with close_fds=False
            for close_fds in (False, True):
                with self.subTest(group=group, close_fds=close_fds):
                    try:
                        output = subprocess.check_output(
                                [sys.executable, "-c",
                                 "import os; print(os.getgid())"],
                                group=group,
                                close_fds=close_fds)
                    except PermissionError:  # (EACCES, EPERM)
                        pass
                    else:
                        if isinstance(group, str):
                            group_gid = grp.getgrnam(group).gr_gid
                        else:
                            group_gid = group

                        child_group = int(output)
                        self.assertEqual(child_group, group_gid)

        # make sure we bomb on negative values
        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, group=-1)

        with self.assertRaises(OverflowError):
            subprocess.check_call(ZERO_RETURN_CMD,
                                  cwd=os.curdir, env=os.environ, group=2**64)

        if grp is None:
            with self.assertRaises(ValueError):
                subprocess.check_call(ZERO_RETURN_CMD, group=name_group)

    @unittest.skipIf(hasattr(os, 'setregid'), 'setregid() available on platform')
    def test_group_error(self):
        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, group=65535)

    @unittest.skipUnless(hasattr(os, 'setgroups'), 'no setgroups() on platform')
    def test_extra_groups(self):
        gid = os.getegid()
        group_list = [65534 if gid != 65534 else 65533]
        name_group = _get_test_grp_name()
        perm_error = False

        if grp is not None:
            group_list.append(name_group)

        try:
            output = subprocess.check_output(
                    [sys.executable, "-c",
                     "import os, sys, json; json.dump(os.getgroups(), sys.stdout)"],
                    extra_groups=group_list)
        except OSError as ex:
            if ex.errno != errno.EPERM:
                raise
            perm_error = True

        else:
            parent_groups = os.getgroups()
            child_groups = json.loads(output)

            if grp is not None:
                desired_gids = [grp.getgrnam(g).gr_gid if isinstance(g, str) else g
                                for g in group_list]
            else:
                desired_gids = group_list

            if perm_error:
                self.assertEqual(set(child_groups), set(parent_groups))
            else:
                self.assertEqual(set(desired_gids), set(child_groups))

        # make sure we bomb on negative values
        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, extra_groups=[-1])

        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD,
                                  cwd=os.curdir, env=os.environ,
                                  extra_groups=[2**64])

        if grp is None:
            with self.assertRaises(ValueError):
                subprocess.check_call(ZERO_RETURN_CMD,
                                      extra_groups=[name_group])

    @unittest.skipIf(hasattr(os, 'setgroups'), 'setgroups() available on platform')
    def test_extra_groups_error(self):
        with self.assertRaises(ValueError):
            subprocess.check_call(ZERO_RETURN_CMD, extra_groups=[])

    @unittest.skipIf(mswindows or not hasattr(os, 'umask'),
                     'POSIX umask() is not available.')
    def test_umask(self):
        tmpdir = None
        try:
            tmpdir = tempfile.mkdtemp()
            name = os.path.join(tmpdir, "beans")
            # We set an unusual umask in the child so as a unique mode
            # for us to test the child's touched file for.
            subprocess.check_call(
                    [sys.executable, "-c", f"open({name!r}, 'w').close()"],
                    umask=0o053)
            # Ignore execute permissions entirely in our test,
            # filesystems could be mounted to ignore or force that.
            st_mode = os.stat(name).st_mode & 0o666
            expected_mode = 0o624
            self.assertEqual(expected_mode, st_mode,
                             msg=f'{oct(expected_mode)} != {oct(st_mode)}')
        finally:
            if tmpdir is not None:
                shutil.rmtree(tmpdir)

    def test_run_abort(self):
        # returncode handles signal termination
        with support.SuppressCrashReport():
            p = subprocess.Popen([sys.executable, "-c",
                                  'import os; os.abort()'])
            p.wait()
        self.assertEqual(-p.returncode, signal.SIGABRT)

    def test_CalledProcessError_str_signal(self):
        err = subprocess.CalledProcessError(-int(signal.SIGABRT), "fake cmd")
        error_string = str(err)
        # We're relying on the repr() of the signal.Signals intenum to provide
        # the word signal, the signal name and the numeric value.
        self.assertIn("signal", error_string.lower())
        # We're not being specific about the signal name as some signals have
        # multiple names and which name is revealed can vary.
        self.assertIn("SIG", error_string)
        self.assertIn(str(signal.SIGABRT), error_string)

    def test_CalledProcessError_str_unknown_signal(self):
        err = subprocess.CalledProcessError(-9876543, "fake cmd")
        error_string = str(err)
        self.assertIn("unknown signal 9876543.", error_string)

    def test_CalledProcessError_str_non_zero(self):
        err = subprocess.CalledProcessError(2, "fake cmd")
        error_string = str(err)
        self.assertIn("non-zero exit status 2.", error_string)

    def test_preexec(self):
        # DISCLAIMER: Setting environment variables is *not* a good use
        # of a preexec_fn.  This is merely a test.
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stdout.write(os.getenv("FRUIT"))'],
                             stdout=subprocess.PIPE,
                             preexec_fn=lambda: os.putenv("FRUIT", "apple"))
        with p:
            self.assertEqual(p.stdout.read(), b"apple")

    def test_preexec_exception(self):
        def raise_it():
            raise ValueError("What if two swallows carried a coconut?")
        try:
            p = subprocess.Popen([sys.executable, "-c", ""],
                                 preexec_fn=raise_it)
        except subprocess.SubprocessError as e:
            self.assertTrue(
                    subprocess._fork_exec,
                    "Expected a ValueError from the preexec_fn")
        except ValueError as e:
            self.assertIn("coconut", e.args[0])
        else:
            self.fail("Exception raised by preexec_fn did not make it "
                      "to the parent process.")

    class _TestExecuteChildPopen(subprocess.Popen):
        """Used to test behavior at the end of _execute_child."""
        def __init__(self, testcase, *args, **kwargs):
            self._testcase = testcase
            subprocess.Popen.__init__(self, *args, **kwargs)

        def _execute_child(self, *args, **kwargs):
            try:
                subprocess.Popen._execute_child(self, *args, **kwargs)
            finally:
                # Open a bunch of file descriptors and verify that
                # none of them are the same as the ones the Popen
                # instance is using for stdin/stdout/stderr.
                devzero_fds = [os.open("/dev/zero", os.O_RDONLY)
                               for _ in range(8)]
                try:
                    for fd in devzero_fds:
                        self._testcase.assertNotIn(
                                fd, (self.stdin.fileno(), self.stdout.fileno(),
                                     self.stderr.fileno()),
                                msg="At least one fd was closed early.")
                finally:
                    for fd in devzero_fds:
                        os.close(fd)

    @unittest.skipIf(not os.path.exists("/dev/zero"), "/dev/zero required.")
    def test_preexec_errpipe_does_not_double_close_pipes(self):
        """Issue16140: Don't double close pipes on preexec error."""

        def raise_it():
            raise subprocess.SubprocessError(
                    "force the _execute_child() errpipe_data path.")

        with self.assertRaises(subprocess.SubprocessError):
            self._TestExecuteChildPopen(
                        self, ZERO_RETURN_CMD,
                        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                        stderr=subprocess.PIPE, preexec_fn=raise_it)

    def test_preexec_gc_module_failure(self):
        # This tests the code that disables garbage collection if the child
        # process will execute any Python.
        enabled = gc.isenabled()
        try:
            gc.disable()
            self.assertFalse(gc.isenabled())
            subprocess.call([sys.executable, '-c', ''],
                            preexec_fn=lambda: None)
            self.assertFalse(gc.isenabled(),
                             "Popen enabled gc when it shouldn't.")

            gc.enable()
            self.assertTrue(gc.isenabled())
            subprocess.call([sys.executable, '-c', ''],
                            preexec_fn=lambda: None)
            self.assertTrue(gc.isenabled(), "Popen left gc disabled.")
        finally:
            if not enabled:
                gc.disable()

    @unittest.skipIf(
        sys.platform == 'darwin', 'setrlimit() seems to fail on OS X')
    def test_preexec_fork_failure(self):
        # The internal code did not preserve the previous exception when
        # re-enabling garbage collection
        try:
            from resource import getrlimit, setrlimit, RLIMIT_NPROC
        except ImportError as err:
            self.skipTest(err)  # RLIMIT_NPROC is specific to Linux and BSD
        limits = getrlimit(RLIMIT_NPROC)
        [_, hard] = limits
        setrlimit(RLIMIT_NPROC, (0, hard))
        self.addCleanup(setrlimit, RLIMIT_NPROC, limits)
        try:
            subprocess.call([sys.executable, '-c', ''],
                            preexec_fn=lambda: None)
        except BlockingIOError:
            # Forking should raise EAGAIN, translated to BlockingIOError
            pass
        else:
            self.skipTest('RLIMIT_NPROC had no effect; probably superuser')

    def test_args_string(self):
        # args is a string
        fd, fname = tempfile.mkstemp()
        # reopen in text mode
        with open(fd, "w", errors="surrogateescape") as fobj:
            fobj.write("#!%s\n" % support.unix_shell)
            fobj.write("exec '%s' -c 'import sys; sys.exit(47)'\n" %
                       sys.executable)
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
        with p:
            self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "apple"
        p = subprocess.Popen("echo $FRUIT", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        with p:
            self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

    def test_call_string(self):
        # call() function with string argument on UNIX
        fd, fname = tempfile.mkstemp()
        # reopen in text mode
        with open(fd, "w", errors="surrogateescape") as fobj:
            fobj.write("#!%s\n" % support.unix_shell)
            fobj.write("exec '%s' -c 'import sys; sys.exit(47)'\n" %
                       sys.executable)
        os.chmod(fname, 0o700)
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
            with p:
                self.assertEqual(p.stdout.read().strip(), bytes(sh, 'ascii'))

    def _kill_process(self, method, *args):
        # Do not inherit file handles from the parent.
        # It should fix failures on some platforms.
        # Also set the SIGINT handler to the default to make sure it's not
        # being ignored (some tests rely on that.)
        old_handler = signal.signal(signal.SIGINT, signal.default_int_handler)
        try:
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
        finally:
            signal.signal(signal.SIGINT, old_handler)
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
        self.assertIn(b'KeyboardInterrupt', stderr)
        self.assertNotEqual(p.wait(), 0)

    def test_kill(self):
        p = self._kill_process('kill')
        _, stderr = p.communicate()
        self.assertEqual(stderr, b'')
        self.assertEqual(p.wait(), -signal.SIGKILL)

    def test_terminate(self):
        p = self._kill_process('terminate')
        _, stderr = p.communicate()
        self.assertEqual(stderr, b'')
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

    def _save_fds(self, save_fds):
        fds = []
        for fd in save_fds:
            inheritable = os.get_inheritable(fd)
            saved = os.dup(fd)
            fds.append((fd, saved, inheritable))
        return fds

    def _restore_fds(self, fds):
        for fd, saved, inheritable in fds:
            os.dup2(saved, fd, inheritable=inheritable)
            os.close(saved)

    def check_close_std_fds(self, fds):
        # Issue #9905: test that subprocess pipes still work properly with
        # some standard fds closed
        stdin = 0
        saved_fds = self._save_fds(fds)
        for fd, saved, inheritable in saved_fds:
            if fd == 0:
                stdin = saved
                break
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
            self.assertEqual(out, b'apple')
            self.assertEqual(err, b'orange')
        finally:
            self._restore_fds(saved_fds)

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

    def test_small_errpipe_write_fd(self):
        """Issue #15798: Popen should work when stdio fds are available."""
        new_stdin = os.dup(0)
        new_stdout = os.dup(1)
        try:
            os.close(0)
            os.close(1)

            # Side test: if errpipe_write fails to have its CLOEXEC
            # flag set this should cause the parent to think the exec
            # failed.  Extremely unlikely: everyone supports CLOEXEC.
            subprocess.Popen([
                    sys.executable, "-c",
                    "print('AssertionError:0:CLOEXEC failure.')"]).wait()
        finally:
            # Restore original stdin and stdout
            os.dup2(new_stdin, 0)
            os.dup2(new_stdout, 1)
            os.close(new_stdin)
            os.close(new_stdout)

    def test_remapping_std_fds(self):
        # open up some temporary files
        temps = [tempfile.mkstemp() for i in range(3)]
        try:
            temp_fds = [fd for fd, fname in temps]

            # unlink the files -- we won't need to reopen them
            for fd, fname in temps:
                os.unlink(fname)

            # write some data to what will become stdin, and rewind
            os.write(temp_fds[1], b"STDIN")
            os.lseek(temp_fds[1], 0, 0)

            # move the standard file descriptors out of the way
            saved_fds = self._save_fds(range(3))
            try:
                # duplicate the file objects over the standard fd's
                for fd, temp_fd in enumerate(temp_fds):
                    os.dup2(temp_fd, fd)

                # now use those files in the "wrong" order, so that subprocess
                # has to rearrange them in the child
                p = subprocess.Popen([sys.executable, "-c",
                    'import sys; got = sys.stdin.read();'
                    'sys.stdout.write("got %s"%got); sys.stderr.write("err")'],
                    stdin=temp_fds[1],
                    stdout=temp_fds[2],
                    stderr=temp_fds[0])
                p.wait()
            finally:
                self._restore_fds(saved_fds)

            for fd in temp_fds:
                os.lseek(fd, 0, 0)

            out = os.read(temp_fds[2], 1024)
            err = os.read(temp_fds[0], 1024).strip()
            self.assertEqual(out, b"got STDIN")
            self.assertEqual(err, b"err")

        finally:
            for fd in temp_fds:
                os.close(fd)

    def check_swap_fds(self, stdin_no, stdout_no, stderr_no):
        # open up some temporary files
        temps = [tempfile.mkstemp() for i in range(3)]
        temp_fds = [fd for fd, fname in temps]
        try:
            # unlink the files -- we won't need to reopen them
            for fd, fname in temps:
                os.unlink(fname)

            # save a copy of the standard file descriptors
            saved_fds = self._save_fds(range(3))
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
                err = os.read(stderr_no, 1024).strip()
            finally:
                self._restore_fds(saved_fds)

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

    def _check_swap_std_fds_with_one_closed(self, from_fds, to_fds):
        saved_fds = self._save_fds(range(3))
        try:
            for from_fd in from_fds:
                with tempfile.TemporaryFile() as f:
                    os.dup2(f.fileno(), from_fd)

            fd_to_close = (set(range(3)) - set(from_fds)).pop()
            os.close(fd_to_close)

            arg_names = ['stdin', 'stdout', 'stderr']
            kwargs = {}
            for from_fd, to_fd in zip(from_fds, to_fds):
                kwargs[arg_names[to_fd]] = from_fd

            code = textwrap.dedent(r'''
                import os, sys
                skipped_fd = int(sys.argv[1])
                for fd in range(3):
                    if fd != skipped_fd:
                        os.write(fd, str(fd).encode('ascii'))
            ''')

            skipped_fd = (set(range(3)) - set(to_fds)).pop()

            rc = subprocess.call([sys.executable, '-c', code, str(skipped_fd)],
                                 **kwargs)
            self.assertEqual(rc, 0)

            for from_fd, to_fd in zip(from_fds, to_fds):
                os.lseek(from_fd, 0, os.SEEK_SET)
                read_bytes = os.read(from_fd, 1024)
                read_fds = list(map(int, read_bytes.decode('ascii')))
                msg = textwrap.dedent(f"""
                    When testing {from_fds} to {to_fds} redirection,
                    parent descriptor {from_fd} got redirected
                    to descriptor(s) {read_fds} instead of descriptor {to_fd}.
                """)
                self.assertEqual([to_fd], read_fds, msg)
        finally:
            self._restore_fds(saved_fds)

    # Check that subprocess can remap std fds correctly even
    # if one of them is closed (#32844).
    def test_swap_std_fds_with_one_closed(self):
        for from_fds in itertools.combinations(range(3), 2):
            for to_fds in itertools.permutations(range(3), 2):
                self._check_swap_std_fds_with_one_closed(from_fds, to_fds)

    def test_surrogates_error_message(self):
        def prepare():
            raise ValueError("surrogate:\uDCff")

        try:
            subprocess.call(
                ZERO_RETURN_CMD,
                preexec_fn=prepare)
        except ValueError as err:
            # Pure Python implementations keeps the message
            self.assertIsNone(subprocess._fork_exec)
            self.assertEqual(str(err), "surrogate:\uDCff")
        except subprocess.SubprocessError as err:
            # _posixsubprocess uses a default message
            self.assertIsNotNone(subprocess._fork_exec)
            self.assertEqual(str(err), "Exception occurred in preexec_fn.")
        else:
            self.fail("Expected ValueError or subprocess.SubprocessError")

    def test_undecodable_env(self):
        for key, value in (('test', 'abc\uDCFF'), ('test\uDCFF', '42')):
            encoded_value = value.encode("ascii", "surrogateescape")

            # test str with surrogates
            script = "import os; print(ascii(os.getenv(%s)))" % repr(key)
            env = os.environ.copy()
            env[key] = value
            # Use C locale to get ASCII for the locale encoding to force
            # surrogate-escaping of \xFF in the child process
            env['LC_ALL'] = 'C'
            decoded_value = value
            stdout = subprocess.check_output(
                [sys.executable, "-c", script],
                env=env)
            stdout = stdout.rstrip(b'\n\r')
            self.assertEqual(stdout.decode('ascii'), ascii(decoded_value))

            # test bytes
            key = key.encode("ascii", "surrogateescape")
            script = "import os; print(ascii(os.getenvb(%s)))" % repr(key)
            env = os.environ.copy()
            env[key] = encoded_value
            stdout = subprocess.check_output(
                [sys.executable, "-c", script],
                env=env)
            stdout = stdout.rstrip(b'\n\r')
            self.assertEqual(stdout.decode('ascii'), ascii(encoded_value))

    def test_bytes_program(self):
        abs_program = os.fsencode(ZERO_RETURN_CMD[0])
        args = list(ZERO_RETURN_CMD[1:])
        path, program = os.path.split(ZERO_RETURN_CMD[0])
        program = os.fsencode(program)

        # absolute bytes path
        exitcode = subprocess.call([abs_program]+args)
        self.assertEqual(exitcode, 0)

        # absolute bytes path as a string
        cmd = b"'%s' %s" % (abs_program, " ".join(args).encode("utf-8"))
        exitcode = subprocess.call(cmd, shell=True)
        self.assertEqual(exitcode, 0)

        # bytes program, unicode PATH
        env = os.environ.copy()
        env["PATH"] = path
        exitcode = subprocess.call([program]+args, env=env)
        self.assertEqual(exitcode, 0)

        # bytes program, bytes PATH
        envb = os.environb.copy()
        envb[b"PATH"] = os.fsencode(path)
        exitcode = subprocess.call([program]+args, env=envb)
        self.assertEqual(exitcode, 0)

    def test_pipe_cloexec(self):
        sleeper = support.findfile("input_reader.py", subdir="subprocessdata")
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")

        p1 = subprocess.Popen([sys.executable, sleeper],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, close_fds=False)

        self.addCleanup(p1.communicate, b'')

        p2 = subprocess.Popen([sys.executable, fd_status],
                              stdout=subprocess.PIPE, close_fds=False)

        output, error = p2.communicate()
        result_fds = set(map(int, output.split(b',')))
        unwanted_fds = set([p1.stdin.fileno(), p1.stdout.fileno(),
                            p1.stderr.fileno()])

        self.assertFalse(result_fds & unwanted_fds,
                         "Expected no fds from %r to be open in child, "
                         "found %r" %
                              (unwanted_fds, result_fds & unwanted_fds))

    def test_pipe_cloexec_real_tools(self):
        qcat = support.findfile("qcat.py", subdir="subprocessdata")
        qgrep = support.findfile("qgrep.py", subdir="subprocessdata")

        subdata = b'zxcvbn'
        data = subdata * 4 + b'\n'

        p1 = subprocess.Popen([sys.executable, qcat],
                              stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                              close_fds=False)

        p2 = subprocess.Popen([sys.executable, qgrep, subdata],
                              stdin=p1.stdout, stdout=subprocess.PIPE,
                              close_fds=False)

        self.addCleanup(p1.wait)
        self.addCleanup(p2.wait)
        def kill_p1():
            try:
                p1.terminate()
            except ProcessLookupError:
                pass
        def kill_p2():
            try:
                p2.terminate()
            except ProcessLookupError:
                pass
        self.addCleanup(kill_p1)
        self.addCleanup(kill_p2)

        p1.stdin.write(data)
        p1.stdin.close()

        readfiles, ignored1, ignored2 = select.select([p2.stdout], [], [], 10)

        self.assertTrue(readfiles, "The child hung")
        self.assertEqual(p2.stdout.read(), data)

        p1.stdout.close()
        p2.stdout.close()

    def test_close_fds(self):
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")

        fds = os.pipe()
        self.addCleanup(os.close, fds[0])
        self.addCleanup(os.close, fds[1])

        open_fds = set(fds)
        # add a bunch more fds
        for _ in range(9):
            fd = os.open(os.devnull, os.O_RDONLY)
            self.addCleanup(os.close, fd)
            open_fds.add(fd)

        for fd in open_fds:
            os.set_inheritable(fd, True)

        p = subprocess.Popen([sys.executable, fd_status],
                             stdout=subprocess.PIPE, close_fds=False)
        output, ignored = p.communicate()
        remaining_fds = set(map(int, output.split(b',')))

        self.assertEqual(remaining_fds & open_fds, open_fds,
                         "Some fds were closed")

        p = subprocess.Popen([sys.executable, fd_status],
                             stdout=subprocess.PIPE, close_fds=True)
        output, ignored = p.communicate()
        remaining_fds = set(map(int, output.split(b',')))

        self.assertFalse(remaining_fds & open_fds,
                         "Some fds were left open")
        self.assertIn(1, remaining_fds, "Subprocess failed")

        # Keep some of the fd's we opened open in the subprocess.
        # This tests _posixsubprocess.c's proper handling of fds_to_keep.
        fds_to_keep = set(open_fds.pop() for _ in range(8))
        p = subprocess.Popen([sys.executable, fd_status],
                             stdout=subprocess.PIPE, close_fds=True,
                             pass_fds=fds_to_keep)
        output, ignored = p.communicate()
        remaining_fds = set(map(int, output.split(b',')))

        self.assertFalse((remaining_fds - fds_to_keep) & open_fds,
                         "Some fds not in pass_fds were left open")
        self.assertIn(1, remaining_fds, "Subprocess failed")


    @unittest.skipIf(sys.platform.startswith("freebsd") and
                     os.stat("/dev").st_dev == os.stat("/dev/fd").st_dev,
                     "Requires fdescfs mounted on /dev/fd on FreeBSD")
    def test_close_fds_when_max_fd_is_lowered(self):
        """Confirm that issue21618 is fixed (may fail under valgrind)."""
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")

        # This launches the meat of the test in a child process to
        # avoid messing with the larger unittest processes maximum
        # number of file descriptors.
        #  This process launches:
        #  +--> Process that lowers its RLIMIT_NOFILE aftr setting up
        #    a bunch of high open fds above the new lower rlimit.
        #    Those are reported via stdout before launching a new
        #    process with close_fds=False to run the actual test:
        #    +--> The TEST: This one launches a fd_status.py
        #      subprocess with close_fds=True so we can find out if
        #      any of the fds above the lowered rlimit are still open.
        p = subprocess.Popen([sys.executable, '-c', textwrap.dedent(
        '''
        import os, resource, subprocess, sys, textwrap
        open_fds = set()
        # Add a bunch more fds to pass down.
        for _ in range(40):
            fd = os.open(os.devnull, os.O_RDONLY)
            open_fds.add(fd)

        # Leave a two pairs of low ones available for use by the
        # internal child error pipe and the stdout pipe.
        # We also leave 10 more open as some Python buildbots run into
        # "too many open files" errors during the test if we do not.
        for fd in sorted(open_fds)[:14]:
            os.close(fd)
            open_fds.remove(fd)

        for fd in open_fds:
            #self.addCleanup(os.close, fd)
            os.set_inheritable(fd, True)

        max_fd_open = max(open_fds)

        # Communicate the open_fds to the parent unittest.TestCase process.
        print(','.join(map(str, sorted(open_fds))))
        sys.stdout.flush()

        rlim_cur, rlim_max = resource.getrlimit(resource.RLIMIT_NOFILE)
        try:
            # 29 is lower than the highest fds we are leaving open.
            resource.setrlimit(resource.RLIMIT_NOFILE, (29, rlim_max))
            # Launch a new Python interpreter with our low fd rlim_cur that
            # inherits open fds above that limit.  It then uses subprocess
            # with close_fds=True to get a report of open fds in the child.
            # An explicit list of fds to check is passed to fd_status.py as
            # letting fd_status rely on its default logic would miss the
            # fds above rlim_cur as it normally only checks up to that limit.
            subprocess.Popen(
                [sys.executable, '-c',
                 textwrap.dedent("""
                     import subprocess, sys
                     subprocess.Popen([sys.executable, %r] +
                                      [str(x) for x in range({max_fd})],
                                      close_fds=True).wait()
                     """.format(max_fd=max_fd_open+1))],
                close_fds=False).wait()
        finally:
            resource.setrlimit(resource.RLIMIT_NOFILE, (rlim_cur, rlim_max))
        ''' % fd_status)], stdout=subprocess.PIPE)

        output, unused_stderr = p.communicate()
        output_lines = output.splitlines()
        self.assertEqual(len(output_lines), 2,
                         msg="expected exactly two lines of output:\n%r" % output)
        opened_fds = set(map(int, output_lines[0].strip().split(b',')))
        remaining_fds = set(map(int, output_lines[1].strip().split(b',')))

        self.assertFalse(remaining_fds & opened_fds,
                         msg="Some fds were left open.")


    # Mac OS X Tiger (10.4) has a kernel bug: sometimes, the file
    # descriptor of a pipe closed in the parent process is valid in the
    # child process according to fstat(), but the mode of the file
    # descriptor is invalid, and read or write raise an error.
    @support.requires_mac_ver(10, 5)
    def test_pass_fds(self):
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")

        open_fds = set()

        for x in range(5):
            fds = os.pipe()
            self.addCleanup(os.close, fds[0])
            self.addCleanup(os.close, fds[1])
            os.set_inheritable(fds[0], True)
            os.set_inheritable(fds[1], True)
            open_fds.update(fds)

        for fd in open_fds:
            p = subprocess.Popen([sys.executable, fd_status],
                                 stdout=subprocess.PIPE, close_fds=True,
                                 pass_fds=(fd, ))
            output, ignored = p.communicate()

            remaining_fds = set(map(int, output.split(b',')))
            to_be_closed = open_fds - {fd}

            self.assertIn(fd, remaining_fds, "fd to be passed not passed")
            self.assertFalse(remaining_fds & to_be_closed,
                             "fd to be closed passed")

            # pass_fds overrides close_fds with a warning.
            with self.assertWarns(RuntimeWarning) as context:
                self.assertFalse(subprocess.call(
                        ZERO_RETURN_CMD,
                        close_fds=False, pass_fds=(fd, )))
            self.assertIn('overriding close_fds', str(context.warning))

    def test_pass_fds_inheritable(self):
        script = support.findfile("fd_status.py", subdir="subprocessdata")

        inheritable, non_inheritable = os.pipe()
        self.addCleanup(os.close, inheritable)
        self.addCleanup(os.close, non_inheritable)
        os.set_inheritable(inheritable, True)
        os.set_inheritable(non_inheritable, False)
        pass_fds = (inheritable, non_inheritable)
        args = [sys.executable, script]
        args += list(map(str, pass_fds))

        p = subprocess.Popen(args,
                             stdout=subprocess.PIPE, close_fds=True,
                             pass_fds=pass_fds)
        output, ignored = p.communicate()
        fds = set(map(int, output.split(b',')))

        # the inheritable file descriptor must be inherited, so its inheritable
        # flag must be set in the child process after fork() and before exec()
        self.assertEqual(fds, set(pass_fds), "output=%a" % output)

        # inheritable flag must not be changed in the parent process
        self.assertEqual(os.get_inheritable(inheritable), True)
        self.assertEqual(os.get_inheritable(non_inheritable), False)


    # bpo-32270: Ensure that descriptors specified in pass_fds
    # are inherited even if they are used in redirections.
    # Contributed by @izbyshev.
    def test_pass_fds_redirected(self):
        """Regression test for https://bugs.python.org/issue32270."""
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")
        pass_fds = []
        for _ in range(2):
            fd = os.open(os.devnull, os.O_RDWR)
            self.addCleanup(os.close, fd)
            pass_fds.append(fd)

        stdout_r, stdout_w = os.pipe()
        self.addCleanup(os.close, stdout_r)
        self.addCleanup(os.close, stdout_w)
        pass_fds.insert(1, stdout_w)

        with subprocess.Popen([sys.executable, fd_status],
                              stdin=pass_fds[0],
                              stdout=pass_fds[1],
                              stderr=pass_fds[2],
                              close_fds=True,
                              pass_fds=pass_fds):
            output = os.read(stdout_r, 1024)
        fds = {int(num) for num in output.split(b',')}

        self.assertEqual(fds, {0, 1, 2} | frozenset(pass_fds), f"output={output!a}")


    def test_stdout_stdin_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen(ZERO_RETURN_CMD,
                                 stdout=inout, stdin=inout)
            p.wait()

    def test_stdout_stderr_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen(ZERO_RETURN_CMD,
                                 stdout=inout, stderr=inout)
            p.wait()

    def test_stderr_stdin_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen(ZERO_RETURN_CMD,
                                 stderr=inout, stdin=inout)
            p.wait()

    def test_wait_when_sigchild_ignored(self):
        # NOTE: sigchild_ignore.py may not be an effective test on all OSes.
        sigchild_ignore = support.findfile("sigchild_ignore.py",
                                           subdir="subprocessdata")
        p = subprocess.Popen([sys.executable, sigchild_ignore],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        self.assertEqual(0, p.returncode, "sigchild_ignore.py exited"
                         " non-zero with this error:\n%s" %
                         stderr.decode('utf-8'))

    def test_select_unbuffered(self):
        # Issue #11459: bufsize=0 should really set the pipes as
        # unbuffered (and therefore let select() work properly).
        select = import_helper.import_module("select")
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdout.write("apple")'],
                             stdout=subprocess.PIPE,
                             bufsize=0)
        f = p.stdout
        self.addCleanup(f.close)
        try:
            self.assertEqual(f.read(4), b"appl")
            self.assertIn(f, select.select([f], [], [], 0.0)[0])
        finally:
            p.wait()

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
        with warnings_helper.check_warnings(('', ResourceWarning)):
            p = None

        if mswindows:
            # subprocess._active is not used on Windows and is set to None.
            self.assertIsNone(subprocess._active)
        else:
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
        with warnings_helper.check_warnings(('', ResourceWarning)):
            p = None
            support.gc_collect()  # For PyPy or other GCs.

        os.kill(pid, signal.SIGKILL)
        if mswindows:
            # subprocess._active is not used on Windows and is set to None.
            self.assertIsNone(subprocess._active)
        else:
            # check that p is in the active processes list
            self.assertIn(ident, [id(o) for o in subprocess._active])

        # let some time for the process to exit, and create a new Popen: this
        # should trigger the wait() of p
        time.sleep(0.2)
        with self.assertRaises(OSError):
            with subprocess.Popen(NONEXISTING_CMD,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE) as proc:
                pass
        # p should have been wait()ed on, and removed from the _active list
        self.assertRaises(OSError, os.waitpid, pid, 0)
        if mswindows:
            # subprocess._active is not used on Windows and is set to None.
            self.assertIsNone(subprocess._active)
        else:
            self.assertNotIn(ident, [id(o) for o in subprocess._active])

    def test_close_fds_after_preexec(self):
        fd_status = support.findfile("fd_status.py", subdir="subprocessdata")

        # this FD is used as dup2() target by preexec_fn, and should be closed
        # in the child process
        fd = os.dup(1)
        self.addCleanup(os.close, fd)

        p = subprocess.Popen([sys.executable, fd_status],
                             stdout=subprocess.PIPE, close_fds=True,
                             preexec_fn=lambda: os.dup2(1, fd))
        output, ignored = p.communicate()

        remaining_fds = set(map(int, output.split(b',')))

        self.assertNotIn(fd, remaining_fds)

    @support.cpython_only
    def test_fork_exec(self):
        # Issue #22290: fork_exec() must not crash on memory allocation failure
        # or other errors
        import _posixsubprocess
        gc_enabled = gc.isenabled()
        try:
            # Use a preexec function and enable the garbage collector
            # to force fork_exec() to re-enable the garbage collector
            # on error.
            func = lambda: None
            gc.enable()

            for args, exe_list, cwd, env_list in (
                (123,      [b"exe"], None, [b"env"]),
                ([b"arg"], 123,      None, [b"env"]),
                ([b"arg"], [b"exe"], 123,  [b"env"]),
                ([b"arg"], [b"exe"], None, 123),
            ):
                with self.assertRaises(TypeError) as err:
                    _posixsubprocess.fork_exec(
                        args, exe_list,
                        True, (), cwd, env_list,
                        -1, -1, -1, -1,
                        1, 2, 3, 4,
                        True, True, 0,
                        False, [], 0, -1,
                        func, False)
                # Attempt to prevent
                # "TypeError: fork_exec() takes exactly N arguments (M given)"
                # from passing the test.  More refactoring to have us start
                # with a valid *args list, confirm a good call with that works
                # before mutating it in various ways to ensure that bad calls
                # with individual arg type errors raise a typeerror would be
                # ideal.  Saving that for a future PR...
                self.assertNotIn('takes exactly', str(err.exception))
        finally:
            if not gc_enabled:
                gc.disable()

    @support.cpython_only
    def test_fork_exec_sorted_fd_sanity_check(self):
        # Issue #23564: sanity check the fork_exec() fds_to_keep sanity check.
        import _posixsubprocess
        class BadInt:
            first = True
            def __init__(self, value):
                self.value = value
            def __int__(self):
                if self.first:
                    self.first = False
                    return self.value
                raise ValueError

        gc_enabled = gc.isenabled()
        try:
            gc.enable()

            for fds_to_keep in (
                (-1, 2, 3, 4, 5),  # Negative number.
                ('str', 4),  # Not an int.
                (18, 23, 42, 2**63),  # Out of range.
                (5, 4),  # Not sorted.
                (6, 7, 7, 8),  # Duplicate.
                (BadInt(1), BadInt(2)),
            ):
                with self.assertRaises(
                        ValueError,
                        msg='fds_to_keep={}'.format(fds_to_keep)) as c:
                    _posixsubprocess.fork_exec(
                        [b"false"], [b"false"],
                        True, fds_to_keep, None, [b"env"],
                        -1, -1, -1, -1,
                        1, 2, 3, 4,
                        True, True, 0,
                        None, None, None, -1,
                        None, True)
                self.assertIn('fds_to_keep', str(c.exception))
        finally:
            if not gc_enabled:
                gc.disable()

    def test_communicate_BrokenPipeError_stdin_close(self):
        # By not setting stdout or stderr or a timeout we force the fast path
        # that just calls _stdin_write() internally due to our mock.
        proc = subprocess.Popen(ZERO_RETURN_CMD)
        with proc, mock.patch.object(proc, 'stdin') as mock_proc_stdin:
            mock_proc_stdin.close.side_effect = BrokenPipeError
            proc.communicate()  # Should swallow BrokenPipeError from close.
            mock_proc_stdin.close.assert_called_with()

    def test_communicate_BrokenPipeError_stdin_write(self):
        # By not setting stdout or stderr or a timeout we force the fast path
        # that just calls _stdin_write() internally due to our mock.
        proc = subprocess.Popen(ZERO_RETURN_CMD)
        with proc, mock.patch.object(proc, 'stdin') as mock_proc_stdin:
            mock_proc_stdin.write.side_effect = BrokenPipeError
            proc.communicate(b'stuff')  # Should swallow the BrokenPipeError.
            mock_proc_stdin.write.assert_called_once_with(b'stuff')
            mock_proc_stdin.close.assert_called_once_with()

    def test_communicate_BrokenPipeError_stdin_flush(self):
        # Setting stdin and stdout forces the ._communicate() code path.
        # python -h exits faster than python -c pass (but spams stdout).
        proc = subprocess.Popen([sys.executable, '-h'],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
        with proc, mock.patch.object(proc, 'stdin') as mock_proc_stdin, \
                open(os.devnull, 'wb') as dev_null:
            mock_proc_stdin.flush.side_effect = BrokenPipeError
            # because _communicate registers a selector using proc.stdin...
            mock_proc_stdin.fileno.return_value = dev_null.fileno()
            # _communicate() should swallow BrokenPipeError from flush.
            proc.communicate(b'stuff')
            mock_proc_stdin.flush.assert_called_once_with()

    def test_communicate_BrokenPipeError_stdin_close_with_timeout(self):
        # Setting stdin and stdout forces the ._communicate() code path.
        # python -h exits faster than python -c pass (but spams stdout).
        proc = subprocess.Popen([sys.executable, '-h'],
                                stdin=subprocess.PIPE,
                                stdout=subprocess.PIPE)
        with proc, mock.patch.object(proc, 'stdin') as mock_proc_stdin:
            mock_proc_stdin.close.side_effect = BrokenPipeError
            # _communicate() should swallow BrokenPipeError from close.
            proc.communicate(timeout=999)
            mock_proc_stdin.close.assert_called_once_with()

    @unittest.skipUnless(_testcapi is not None
                         and hasattr(_testcapi, 'W_STOPCODE'),
                         'need _testcapi.W_STOPCODE')
    def test_stopped(self):
        """Test wait() behavior when waitpid returns WIFSTOPPED; issue29335."""
        args = ZERO_RETURN_CMD
        proc = subprocess.Popen(args)

        # Wait until the real process completes to avoid zombie process
        support.wait_process(proc.pid, exitcode=0)

        status = _testcapi.W_STOPCODE(3)
        with mock.patch('subprocess.os.waitpid', return_value=(proc.pid, status)):
            returncode = proc.wait()

        self.assertEqual(returncode, -3)

    def test_send_signal_race(self):
        # bpo-38630: send_signal() must poll the process exit status to reduce
        # the risk of sending the signal to the wrong process.
        proc = subprocess.Popen(ZERO_RETURN_CMD)

        # wait until the process completes without using the Popen APIs.
        support.wait_process(proc.pid, exitcode=0)

        # returncode is still None but the process completed.
        self.assertIsNone(proc.returncode)

        with mock.patch("os.kill") as mock_kill:
            proc.send_signal(signal.SIGTERM)

        # send_signal() didn't call os.kill() since the process already
        # completed.
        mock_kill.assert_not_called()

        # Don't check the returncode value: the test reads the exit status,
        # so Popen failed to read it and uses a default returncode instead.
        self.assertIsNotNone(proc.returncode)

    def test_send_signal_race2(self):
        # bpo-40550: the process might exist between the returncode check and
        # the kill operation
        p = subprocess.Popen([sys.executable, '-c', 'exit(1)'])

        # wait for process to exit
        while not p.returncode:
            p.poll()

        with mock.patch.object(p, 'poll', new=lambda: None):
            p.returncode = None
            p.send_signal(signal.SIGTERM)
        p.kill()

    def test_communicate_repeated_call_after_stdout_close(self):
        proc = subprocess.Popen([sys.executable, '-c',
                                 'import os, time; os.close(1), time.sleep(2)'],
                                stdout=subprocess.PIPE)
        while True:
            try:
                proc.communicate(timeout=0.1)
                return
            except subprocess.TimeoutExpired:
                pass


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
        subprocess.call(ZERO_RETURN_CMD,
                        startupinfo=startupinfo)

    def test_startupinfo_keywords(self):
        # startupinfo argument
        # We use hardcoded constants, because we do not want to
        # depend on win32all.
        STARTF_USERSHOWWINDOW = 1
        SW_MAXIMIZE = 3
        startupinfo = subprocess.STARTUPINFO(
            dwFlags=STARTF_USERSHOWWINDOW,
            wShowWindow=SW_MAXIMIZE
        )
        # Since Python is a console process, it won't be affected
        # by wShowWindow, but the argument should be silently
        # ignored
        subprocess.call(ZERO_RETURN_CMD,
                        startupinfo=startupinfo)

    def test_startupinfo_copy(self):
        # bpo-34044: Popen must not modify input STARTUPINFO structure
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags = subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE

        # Call Popen() twice with the same startupinfo object to make sure
        # that it's not modified
        for _ in range(2):
            cmd = ZERO_RETURN_CMD
            with open(os.devnull, 'w') as null:
                proc = subprocess.Popen(cmd,
                                        stdout=null,
                                        stderr=subprocess.STDOUT,
                                        startupinfo=startupinfo)
                with proc:
                    proc.communicate()
                self.assertEqual(proc.returncode, 0)

            self.assertEqual(startupinfo.dwFlags,
                             subprocess.STARTF_USESHOWWINDOW)
            self.assertIsNone(startupinfo.hStdInput)
            self.assertIsNone(startupinfo.hStdOutput)
            self.assertIsNone(startupinfo.hStdError)
            self.assertEqual(startupinfo.wShowWindow, subprocess.SW_HIDE)
            self.assertEqual(startupinfo.lpAttributeList, {"handle_list": []})

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

    @support.cpython_only
    def test_issue31471(self):
        # There shouldn't be an assertion failure in Popen() in case the env
        # argument has a bad keys() method.
        class BadEnv(dict):
            keys = None
        with self.assertRaises(TypeError):
            subprocess.Popen(ZERO_RETURN_CMD, env=BadEnv())

    def test_close_fds(self):
        # close file descriptors
        rc = subprocess.call([sys.executable, "-c",
                              "import sys; sys.exit(47)"],
                              close_fds=True)
        self.assertEqual(rc, 47)

    def test_close_fds_with_stdio(self):
        import msvcrt

        fds = os.pipe()
        self.addCleanup(os.close, fds[0])
        self.addCleanup(os.close, fds[1])

        handles = []
        for fd in fds:
            os.set_inheritable(fd, True)
            handles.append(msvcrt.get_osfhandle(fd))

        p = subprocess.Popen([sys.executable, "-c",
                              "import msvcrt; print(msvcrt.open_osfhandle({}, 0))".format(handles[0])],
                             stdout=subprocess.PIPE, close_fds=False)
        stdout, stderr = p.communicate()
        self.assertEqual(p.returncode, 0)
        int(stdout.strip())  # Check that stdout is an integer

        p = subprocess.Popen([sys.executable, "-c",
                              "import msvcrt; print(msvcrt.open_osfhandle({}, 0))".format(handles[0])],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE, close_fds=True)
        stdout, stderr = p.communicate()
        self.assertEqual(p.returncode, 1)
        self.assertIn(b"OSError", stderr)

        # The same as the previous call, but with an empty handle_list
        handle_list = []
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.lpAttributeList = {"handle_list": handle_list}
        p = subprocess.Popen([sys.executable, "-c",
                              "import msvcrt; print(msvcrt.open_osfhandle({}, 0))".format(handles[0])],
                             stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                             startupinfo=startupinfo, close_fds=True)
        stdout, stderr = p.communicate()
        self.assertEqual(p.returncode, 1)
        self.assertIn(b"OSError", stderr)

        # Check for a warning due to using handle_list and close_fds=False
        with warnings_helper.check_warnings((".*overriding close_fds",
                                             RuntimeWarning)):
            startupinfo = subprocess.STARTUPINFO()
            startupinfo.lpAttributeList = {"handle_list": handles[:]}
            p = subprocess.Popen([sys.executable, "-c",
                                  "import msvcrt; print(msvcrt.open_osfhandle({}, 0))".format(handles[0])],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 startupinfo=startupinfo, close_fds=False)
            stdout, stderr = p.communicate()
            self.assertEqual(p.returncode, 0)

    def test_empty_attribute_list(self):
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.lpAttributeList = {}
        subprocess.call(ZERO_RETURN_CMD,
                        startupinfo=startupinfo)

    def test_empty_handle_list(self):
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.lpAttributeList = {"handle_list": []}
        subprocess.call(ZERO_RETURN_CMD,
                        startupinfo=startupinfo)

    def test_shell_sequence(self):
        # Run command through the shell (sequence)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "physalis"
        p = subprocess.Popen(["set"], shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        with p:
            self.assertIn(b"physalis", p.stdout.read())

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "physalis"
        p = subprocess.Popen("set", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        with p:
            self.assertIn(b"physalis", p.stdout.read())

    def test_shell_encodings(self):
        # Run command through the shell (string)
        for enc in ['ansi', 'oem']:
            newenv = os.environ.copy()
            newenv["FRUIT"] = "physalis"
            p = subprocess.Popen("set", shell=1,
                                 stdout=subprocess.PIPE,
                                 env=newenv,
                                 encoding=enc)
            with p:
                self.assertIn("physalis", p.stdout.read(), enc)

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
        with p:
            # Wait for the interpreter to be completely initialized before
            # sending any signal.
            p.stdout.read(1)
            getattr(p, method)(*args)
            _, stderr = p.communicate()
            self.assertEqual(stderr, b'')
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
        with p:
            # Wait for the interpreter to be completely initialized before
            # sending any signal.
            p.stdout.read(1)
            # The process should end after this
            time.sleep(1)
            # This shouldn't raise even though the child is now dead
            getattr(p, method)(*args)
            _, stderr = p.communicate()
            self.assertEqual(stderr, b'')
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

class MiscTests(unittest.TestCase):

    class RecordingPopen(subprocess.Popen):
        """A Popen that saves a reference to each instance for testing."""
        instances_created = []

        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            self.instances_created.append(self)

    @mock.patch.object(subprocess.Popen, "_communicate")
    def _test_keyboardinterrupt_no_kill(self, popener, mock__communicate,
                                        **kwargs):
        """Fake a SIGINT happening during Popen._communicate() and ._wait().

        This avoids the need to actually try and get test environments to send
        and receive signals reliably across platforms.  The net effect of a ^C
        happening during a blocking subprocess execution which we want to clean
        up from is a KeyboardInterrupt coming out of communicate() or wait().
        """

        mock__communicate.side_effect = KeyboardInterrupt
        try:
            with mock.patch.object(subprocess.Popen, "_wait") as mock__wait:
                # We patch out _wait() as no signal was involved so the
                # child process isn't actually going to exit rapidly.
                mock__wait.side_effect = KeyboardInterrupt
                with mock.patch.object(subprocess, "Popen",
                                       self.RecordingPopen):
                    with self.assertRaises(KeyboardInterrupt):
                        popener([sys.executable, "-c",
                                 "import time\ntime.sleep(9)\nimport sys\n"
                                 "sys.stderr.write('\\n!runaway child!\\n')"],
                                stdout=subprocess.DEVNULL, **kwargs)
                for call in mock__wait.call_args_list[1:]:
                    self.assertNotEqual(
                            call, mock.call(timeout=None),
                            "no open-ended wait() after the first allowed: "
                            f"{mock__wait.call_args_list}")
                sigint_calls = []
                for call in mock__wait.call_args_list:
                    if call == mock.call(timeout=0.25):  # from Popen.__init__
                        sigint_calls.append(call)
                self.assertLessEqual(mock__wait.call_count, 2,
                                     msg=mock__wait.call_args_list)
                self.assertEqual(len(sigint_calls), 1,
                                 msg=mock__wait.call_args_list)
        finally:
            # cleanup the forgotten (due to our mocks) child process
            process = self.RecordingPopen.instances_created.pop()
            process.kill()
            process.wait()
            self.assertEqual([], self.RecordingPopen.instances_created)

    def test_call_keyboardinterrupt_no_kill(self):
        self._test_keyboardinterrupt_no_kill(subprocess.call, timeout=6.282)

    def test_run_keyboardinterrupt_no_kill(self):
        self._test_keyboardinterrupt_no_kill(subprocess.run, timeout=6.282)

    def test_context_manager_keyboardinterrupt_no_kill(self):
        def popen_via_context_manager(*args, **kwargs):
            with subprocess.Popen(*args, **kwargs) as unused_process:
                raise KeyboardInterrupt  # Test how __exit__ handles ^C.
        self._test_keyboardinterrupt_no_kill(popen_via_context_manager)

    def test_getoutput(self):
        self.assertEqual(subprocess.getoutput('echo xyzzy'), 'xyzzy')
        self.assertEqual(subprocess.getstatusoutput('echo xyzzy'),
                         (0, 'xyzzy'))

        # we use mkdtemp in the next line to create an empty directory
        # under our exclusive control; from that, we can invent a pathname
        # that we _know_ won't exist.  This is guaranteed to fail.
        dir = None
        try:
            dir = tempfile.mkdtemp()
            name = os.path.join(dir, "foo")
            status, output = subprocess.getstatusoutput(
                ("type " if mswindows else "cat ") + name)
            self.assertNotEqual(status, 0)
        finally:
            if dir is not None:
                os.rmdir(dir)

    def test__all__(self):
        """Ensure that __all__ is populated properly."""
        intentionally_excluded = {"list2cmdline", "Handle", "pwd", "grp", "fcntl"}
        exported = set(subprocess.__all__)
        possible_exports = set()
        import types
        for name, value in subprocess.__dict__.items():
            if name.startswith('_'):
                continue
            if isinstance(value, (types.ModuleType,)):
                continue
            possible_exports.add(name)
        self.assertEqual(exported, possible_exports - intentionally_excluded)


@unittest.skipUnless(hasattr(selectors, 'PollSelector'),
                     "Test needs selectors.PollSelector")
class ProcessTestCaseNoPoll(ProcessTestCase):
    def setUp(self):
        self.orig_selector = subprocess._PopenSelector
        subprocess._PopenSelector = selectors.SelectSelector
        ProcessTestCase.setUp(self)

    def tearDown(self):
        subprocess._PopenSelector = self.orig_selector
        ProcessTestCase.tearDown(self)


@unittest.skipUnless(mswindows, "Windows-specific tests")
class CommandsWithSpaces (BaseTestCase):

    def setUp(self):
        super().setUp()
        f, fname = tempfile.mkstemp(".py", "te st")
        self.fname = fname.lower ()
        os.write(f, b"import sys;"
                    b"sys.stdout.write('%d %s' % (len(sys.argv), [a.lower () for a in sys.argv]))"
        )
        os.close(f)

    def tearDown(self):
        os.remove(self.fname)
        super().tearDown()

    def with_spaces(self, *args, **kwargs):
        kwargs['stdout'] = subprocess.PIPE
        p = subprocess.Popen(*args, **kwargs)
        with p:
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


class ContextManagerTests(BaseTestCase):

    def test_pipe(self):
        with subprocess.Popen([sys.executable, "-c",
                               "import sys;"
                               "sys.stdout.write('stdout');"
                               "sys.stderr.write('stderr');"],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as proc:
            self.assertEqual(proc.stdout.read(), b"stdout")
            self.assertEqual(proc.stderr.read(), b"stderr")

        self.assertTrue(proc.stdout.closed)
        self.assertTrue(proc.stderr.closed)

    def test_returncode(self):
        with subprocess.Popen([sys.executable, "-c",
                               "import sys; sys.exit(100)"]) as proc:
            pass
        # __exit__ calls wait(), so the returncode should be set
        self.assertEqual(proc.returncode, 100)

    def test_communicate_stdin(self):
        with subprocess.Popen([sys.executable, "-c",
                              "import sys;"
                              "sys.exit(sys.stdin.read() == 'context')"],
                             stdin=subprocess.PIPE) as proc:
            proc.communicate(b"context")
            self.assertEqual(proc.returncode, 1)

    def test_invalid_args(self):
        with self.assertRaises(NONEXISTING_ERRORS):
            with subprocess.Popen(NONEXISTING_CMD,
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE) as proc:
                pass

    def test_broken_pipe_cleanup(self):
        """Broken pipe error should not prevent wait() (Issue 21619)"""
        proc = subprocess.Popen(ZERO_RETURN_CMD,
                                stdin=subprocess.PIPE,
                                bufsize=support.PIPE_MAX_SIZE*2)
        proc = proc.__enter__()
        # Prepare to send enough data to overflow any OS pipe buffering and
        # guarantee a broken pipe error. Data is held in BufferedWriter
        # buffer until closed.
        proc.stdin.write(b'x' * support.PIPE_MAX_SIZE)
        self.assertIsNone(proc.returncode)
        # EPIPE expected under POSIX; EINVAL under Windows
        self.assertRaises(OSError, proc.__exit__, None, None, None)
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(proc.stdin.closed)


if __name__ == "__main__":
    unittest.main()
