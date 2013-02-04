import unittest
from test import script_helper
from test import support
import subprocess
import sys
import signal
import io
import locale
import os
import errno
import tempfile
import time
import re
import sysconfig
import warnings
import select
import shutil
import gc

try:
    import resource
except ImportError:
    resource = None

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
        support.reap_children()

    def tearDown(self):
        for inst in subprocess._active:
            inst.wait()
        subprocess._cleanup()
        self.assertFalse(subprocess._active, "subprocess._active not empty")

    def assertStderrEqual(self, stderr, expected, msg=None):
        # In a debug build, stuff like "[6580 refs]" is printed to stderr at
        # shutdown time.  That frustrates tests trying to check stderr produced
        # from a spawned Python process.
        actual = support.strip_python_stderr(stderr)
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
                [sys.executable, "-c", "print('BDFL')"])
        self.assertIn(b'BDFL', output)

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

    def test_check_output_stdout_arg(self):
        # check_output() function stderr redirected to stdout
        with self.assertRaises(ValueError) as c:
            output = subprocess.check_output(
                    [sys.executable, "-c", "print('will not be run')"],
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
        # .stdout is None when not redirected
        p = subprocess.Popen([sys.executable, "-c",
                             'print("    this bit of output is from a '
                             'test of stdout in a different '
                             'process ...")'],
                             stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        self.addCleanup(p.stdin.close)
        self.addCleanup(p.stderr.close)
        p.wait()
        self.assertEqual(p.stdout, None)

    def test_stderr_none(self):
        # .stderr is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print("banana")'],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        self.addCleanup(p.stdout.close)
        self.addCleanup(p.stdin.close)
        p.wait()
        self.assertEqual(p.stderr, None)

    # For use in the test_cwd* tests below.
    def _normalize_cwd(self, cwd):
        # Normalize an expected cwd (for Tru64 support).
        # We can't use os.path.realpath since it doesn't expand Tru64 {memb}
        # strings.  See bug #1063571.
        original_cwd = os.getcwd()
        os.chdir(cwd)
        cwd = os.getcwd()
        os.chdir(original_cwd)
        return cwd

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
                              "sys.stdout.write(os.getcwd()); "
                              "sys.exit(47)"],
                              stdout=subprocess.PIPE,
                              **kwargs)
        self.addCleanup(p.stdout.close)
        p.wait()
        self.assertEqual(47, p.returncode)
        normcase = os.path.normcase
        self.assertEqual(normcase(expected_cwd),
                         normcase(p.stdout.read().decode("utf-8")))

    def test_cwd(self):
        # Check that cwd changes the cwd for the child process.
        temp_dir = tempfile.gettempdir()
        temp_dir = self._normalize_cwd(temp_dir)
        self._assert_cwd(temp_dir, sys.executable, cwd=temp_dir)

    @unittest.skipIf(mswindows, "pending resolution of issue #15533")
    def test_cwd_with_relative_arg(self):
        # Check that Popen looks for args[0] relative to cwd if args[0]
        # is relative.
        python_dir, python_base = self._split_python_path()
        rel_python = os.path.join(os.curdir, python_base)
        with support.temp_cwd() as wrong_dir:
            # Before calling with the correct cwd, confirm that the call fails
            # without cwd and with the wrong cwd.
            self.assertRaises(OSError, subprocess.Popen,
                              [rel_python])
            self.assertRaises(OSError, subprocess.Popen,
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
        with support.temp_cwd() as wrong_dir:
            # Before calling with the correct cwd, confirm that the call fails
            # without cwd and with the wrong cwd.
            self.assertRaises(OSError, subprocess.Popen,
                              [doesntexist], executable=rel_python)
            self.assertRaises(OSError, subprocess.Popen,
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
        with script_helper.temp_dir() as wrong_dir:
            # Before calling with an absolute path, confirm that using a
            # relative path fails.
            self.assertRaises(OSError, subprocess.Popen,
                              [rel_python], cwd=wrong_dir)
            wrong_dir = self._normalize_cwd(wrong_dir)
            self._assert_cwd(wrong_dir, abs_python, cwd=wrong_dir)

    def test_executable_with_cwd(self):
        python_dir, python_base = self._split_python_path()
        python_dir = self._normalize_cwd(python_dir)
        self._assert_cwd(python_dir, "somethingyoudonthave",
                         executable=sys.executable, cwd=python_dir)

    @unittest.skipIf(sysconfig.is_python_build(),
                     "need an installed Python. See #7774")
    def test_executable_without_cwd(self):
        # For a normal installation, it should work without 'cwd'
        # argument.  For test runs in the build directory, see #7774.
        self._assert_cwd('', "somethingyoudonthave", executable=sys.executable)

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
        self.addCleanup(p.stdout.close)
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
        self.addCleanup(p.stderr.close)
        self.assertStderrEqual(p.stderr.read(), b"strawberry")

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
        self.assertStderrEqual(os.read(d, 1024), b"strawberry")

    def test_stderr_fileobj(self):
        # stderr is set to open file object
        tf = tempfile.TemporaryFile()
        self.addCleanup(tf.close)
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertStderrEqual(tf.read(), b"strawberry")

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
        self.assertStderrEqual(p.stdout.read(), b"appleorange")

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
        self.assertStderrEqual(tf.read(), b"appleorange")

    def test_stdout_filedes_of_stdout(self):
        # stdout is set to 1 (#1531862).
        cmd = r"import sys, os; sys.exit(os.write(sys.stdout.fileno(), b'.\n'))"
        rc = subprocess.call([sys.executable, "-c", cmd], stdout=1)
        self.assertEqual(rc, 2)

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
    @unittest.skipIf(sysconfig.get_config_var('Py_ENABLE_SHARED') is not None,
                     'the python library cannot be loaded '
                     'with an empty environment')
    def test_empty_env(self):
        with subprocess.Popen([sys.executable, "-c",
                               'import os; '
                               'print(list(os.environ.keys()))'],
                              stdout=subprocess.PIPE,
                              env={}) as p:
            stdout, stderr = p.communicate()
            self.assertIn(stdout.strip(),
                (b"[]",
                 # Mac OS X adds __CF_USER_TEXT_ENCODING variable to an empty
                 # environment
                 b"['__CF_USER_TEXT_ENCODING']"))

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
        self.assertStderrEqual(stderr, b"pineapple")

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
        self.assertStderrEqual(stderr, b"pineapple")

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
                    p = subprocess.Popen((sys.executable, "-c", "pass"), **options)
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
        string_to_write = b"abc"*pipe_buf
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
        self.assertStderrEqual(stderr, b"")

    def test_universal_newlines(self):
        p = subprocess.Popen([sys.executable, "-c",
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
                              'buf.write(b"\\nline8");'],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             universal_newlines=1)
        p.stdin.write("line1\n")
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
                              'import sys,os;' + SETBINARY + '''\nif True:
                                  s = sys.stdin.readline()
                                  assert s == "line1\\n", repr(s)
                                  s = sys.stdin.read()
                                  assert s == "line3\\n", repr(s)
                              '''],
                             stdin=subprocess.PIPE,
                             universal_newlines=1)
        (stdout, stderr) = p.communicate("line1\nline3\n")
        self.assertEqual(p.returncode, 0)

    def test_universal_newlines_communicate_input_none(self):
        # Test communicate(input=None) with universal newlines.
        #
        # We set stdout to PIPE because, as of this writing, a different
        # code path is tested when the number of pipes is zero or one.
        p = subprocess.Popen([sys.executable, "-c", "pass"],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             universal_newlines=True)
        p.communicate()
        self.assertEqual(p.returncode, 0)

    def test_universal_newlines_communicate_stdin_stdout_stderr(self):
        # universal newlines through communicate(), with stdin, stdout, stderr
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY + '''\nif True:
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
                              '''],
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
        # Don't use assertStderrEqual because it strips CR and LF from output.
        self.assertTrue(stderr.startswith("eline2\neline6\neline7\n"))

    def test_universal_newlines_communicate_encodings(self):
        # Check that universal newlines mode works for various encodings,
        # in particular for encodings in the UTF-16 and UTF-32 families.
        # See issue #15595.
        #
        # UTF-16 and UTF-32-BE are sufficient to check both with BOM and
        # without, and UTF-16 and UTF-32.
        for encoding in ['utf-16', 'utf-32-be']:
            old_getpreferredencoding = locale.getpreferredencoding
            # Indirectly via io.TextIOWrapper, Popen() defaults to
            # locale.getpreferredencoding(False) and earlier in Python 3.2 to
            # locale.getpreferredencoding().
            def getpreferredencoding(do_setlocale=True):
                return encoding
            code = ("import sys; "
                    r"sys.stdout.buffer.write('1\r\n2\r3\n4'.encode('%s'))" %
                    encoding)
            args = [sys.executable, '-c', code]
            try:
                locale.getpreferredencoding = getpreferredencoding
                # We set stdin to be non-None because, as of this writing,
                # a different code path is used when the number of pipes is
                # zero or one.
                popen = subprocess.Popen(args, universal_newlines=True,
                                         stdin=subprocess.PIPE,
                                         stdout=subprocess.PIPE)
                stdout, stderr = popen.communicate(input='')
            finally:
                locale.getpreferredencoding = old_getpreferredencoding

            self.assertEqual(stdout, '1\n2\n3\n4')

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
                    tmpfile = os.path.join(tmpdir, support.TESTFN)
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

    def test_bufsize_is_none(self):
        # bufsize=None should be the same as bufsize=0.
        p = subprocess.Popen([sys.executable, "-c", "pass"], None)
        self.assertEqual(p.wait(), 0)
        # Again with keyword arg
        p = subprocess.Popen([sys.executable, "-c", "pass"], bufsize=None)
        self.assertEqual(p.wait(), 0)

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
        p.communicate(b"x" * 2**20)

    def test_communicate_epipe_only_stdin(self):
        # Issue 10963: communicate() should hide EPIPE
        p = subprocess.Popen([sys.executable, "-c", 'pass'],
                             stdin=subprocess.PIPE)
        self.addCleanup(p.stdin.close)
        time.sleep(2)
        p.communicate(b"x" * 2**20)

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
                print("this tests triggers the Crash Reporter, "
                      "that is intentional", end='')
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
            desired_exception.strerror += ': ' + repr(self._nonexistent_dir)
        else:
            self.fail("chdir to nonexistant directory %s succeeded." %
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
        else:
            self.fail("Expected OSError: %s" % desired_exception)

    def test_restore_signals(self):
        # Code coverage for both values of restore_signals to make sure it
        # at least does not blow up.
        # A test for behavior would be complex.  Contributions welcome.
        subprocess.call([sys.executable, "-c", ""], restore_signals=True)
        subprocess.call([sys.executable, "-c", ""], restore_signals=False)

    def test_start_new_session(self):
        # For code coverage of calling setsid().  We don't care if we get an
        # EPERM error from it depending on the test execution environment, that
        # still indicates that it was called.
        try:
            output = subprocess.check_output(
                    [sys.executable, "-c",
                     "import os; print(os.getpgid(os.getpid()))"],
                    start_new_session=True)
        except OSError as e:
            if e.errno != errno.EPERM:
                raise
        else:
            parent_pgid = os.getpgid(os.getpid())
            child_pgid = int(output)
            self.assertNotEqual(parent_pgid, child_pgid)

    def test_run_abort(self):
        # returncode handles signal termination
        with _SuppressCoreFiles():
            p = subprocess.Popen([sys.executable, "-c",
                                  'import os; os.abort()'])
            p.wait()
        self.assertEqual(-p.returncode, signal.SIGABRT)

    def test_preexec(self):
        # DISCLAIMER: Setting environment variables is *not* a good use
        # of a preexec_fn.  This is merely a test.
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stdout.write(os.getenv("FRUIT"))'],
                             stdout=subprocess.PIPE,
                             preexec_fn=lambda: os.putenv("FRUIT", "apple"))
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read(), b"apple")

    def test_preexec_exception(self):
        def raise_it():
            raise ValueError("What if two swallows carried a coconut?")
        try:
            p = subprocess.Popen([sys.executable, "-c", ""],
                                 preexec_fn=raise_it)
        except RuntimeError as e:
            self.assertTrue(
                    subprocess._posixsubprocess,
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
                    map(os.close, devzero_fds)

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

    def test_preexec_gc_module_failure(self):
        # This tests the code that disables garbage collection if the child
        # process will execute any Python.
        def raise_runtime_error():
            raise RuntimeError("this shouldn't escape")
        enabled = gc.isenabled()
        orig_gc_disable = gc.disable
        orig_gc_isenabled = gc.isenabled
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

            gc.disable = raise_runtime_error
            self.assertRaises(RuntimeError, subprocess.Popen,
                              [sys.executable, '-c', ''],
                              preexec_fn=lambda: None)

            del gc.isenabled  # force an AttributeError
            self.assertRaises(AttributeError, subprocess.Popen,
                              [sys.executable, '-c', ''],
                              preexec_fn=lambda: None)
        finally:
            gc.disable = orig_gc_disable
            gc.isenabled = orig_gc_isenabled
            if not enabled:
                gc.disable()

    def test_args_string(self):
        # args is a string
        fd, fname = mkstemp()
        # reopen in text mode
        with open(fd, "w", errors="surrogateescape") as fobj:
            fobj.write("#!/bin/sh\n")
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
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "apple"
        p = subprocess.Popen("echo $FRUIT", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

    def test_call_string(self):
        # call() function with string argument on UNIX
        fd, fname = mkstemp()
        # reopen in text mode
        with open(fd, "w", errors="surrogateescape") as fobj:
            fobj.write("#!/bin/sh\n")
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
            self.addCleanup(p.stdout.close)
            self.assertEqual(p.stdout.read().strip(), bytes(sh, 'ascii'))

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
        self.assertIn(b'KeyboardInterrupt', stderr)
        self.assertNotEqual(p.wait(), 0)

    def test_kill(self):
        p = self._kill_process('kill')
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, b'')
        self.assertEqual(p.wait(), -signal.SIGKILL)

    def test_terminate(self):
        p = self._kill_process('terminate')
        _, stderr = p.communicate()
        self.assertStderrEqual(stderr, b'')
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
            err = support.strip_python_stderr(err)
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

    def test_remapping_std_fds(self):
        # open up some temporary files
        temps = [mkstemp() for i in range(3)]
        try:
            temp_fds = [fd for fd, fname in temps]

            # unlink the files -- we won't need to reopen them
            for fd, fname in temps:
                os.unlink(fname)

            # write some data to what will become stdin, and rewind
            os.write(temp_fds[1], b"STDIN")
            os.lseek(temp_fds[1], 0, 0)

            # move the standard file descriptors out of the way
            saved_fds = [os.dup(fd) for fd in range(3)]
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
                # restore the original fd's underneath sys.stdin, etc.
                for std, saved in enumerate(saved_fds):
                    os.dup2(saved, std)
                    os.close(saved)

            for fd in temp_fds:
                os.lseek(fd, 0, 0)

            out = os.read(temp_fds[2], 1024)
            err = support.strip_python_stderr(os.read(temp_fds[0], 1024))
            self.assertEqual(out, b"got STDIN")
            self.assertEqual(err, b"err")

        finally:
            for fd in temp_fds:
                os.close(fd)

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
                err = support.strip_python_stderr(os.read(stderr_no, 1024))
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

    def test_surrogates_error_message(self):
        def prepare():
            raise ValueError("surrogate:\uDCff")

        try:
            subprocess.call(
                [sys.executable, "-c", "pass"],
                preexec_fn=prepare)
        except ValueError as err:
            # Pure Python implementations keeps the message
            self.assertIsNone(subprocess._posixsubprocess)
            self.assertEqual(str(err), "surrogate:\uDCff")
        except RuntimeError as err:
            # _posixsubprocess uses a default message
            self.assertIsNotNone(subprocess._posixsubprocess)
            self.assertEqual(str(err), "Exception occurred in preexec_fn.")
        else:
            self.fail("Expected ValueError or RuntimeError")

    def test_undecodable_env(self):
        for key, value in (('test', 'abc\uDCFF'), ('test\uDCFF', '42')):
            # test str with surrogates
            script = "import os; print(ascii(os.getenv(%s)))" % repr(key)
            env = os.environ.copy()
            env[key] = value
            # Use C locale to get ascii for the locale encoding to force
            # surrogate-escaping of \xFF in the child process; otherwise it can
            # be decoded as-is if the default locale is latin-1.
            env['LC_ALL'] = 'C'
            stdout = subprocess.check_output(
                [sys.executable, "-c", script],
                env=env)
            stdout = stdout.rstrip(b'\n\r')
            self.assertEqual(stdout.decode('ascii'), ascii(value))

            # test bytes
            key = key.encode("ascii", "surrogateescape")
            value = value.encode("ascii", "surrogateescape")
            script = "import os; print(ascii(os.getenvb(%s)))" % repr(key)
            env = os.environ.copy()
            env[key] = value
            stdout = subprocess.check_output(
                [sys.executable, "-c", script],
                env=env)
            stdout = stdout.rstrip(b'\n\r')
            self.assertEqual(stdout.decode('ascii'), ascii(value))

    def test_bytes_program(self):
        abs_program = os.fsencode(sys.executable)
        path, program = os.path.split(sys.executable)
        program = os.fsencode(program)

        # absolute bytes path
        exitcode = subprocess.call([abs_program, "-c", "pass"])
        self.assertEqual(exitcode, 0)

        # bytes program, unicode PATH
        env = os.environ.copy()
        env["PATH"] = path
        exitcode = subprocess.call([program, "-c", "pass"], env=env)
        self.assertEqual(exitcode, 0)

        # bytes program, bytes PATH
        envb = os.environb.copy()
        envb[b"PATH"] = os.fsencode(path)
        exitcode = subprocess.call([program, "-c", "pass"], env=envb)
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
            fd = os.open("/dev/null", os.O_RDONLY)
            self.addCleanup(os.close, fd)
            open_fds.add(fd)

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
                             pass_fds=())
        output, ignored = p.communicate()
        remaining_fds = set(map(int, output.split(b',')))

        self.assertFalse(remaining_fds & fds_to_keep & open_fds,
                         "Some fds not in pass_fds were left open")
        self.assertIn(1, remaining_fds, "Subprocess failed")

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
                        [sys.executable, "-c", "import sys; sys.exit(0)"],
                        close_fds=False, pass_fds=(fd, )))
            self.assertIn('overriding close_fds', str(context.warning))

    def test_stdout_stdin_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen([sys.executable, "-c", "import sys; sys.exit(0)"],
                                 stdout=inout, stdin=inout)
            p.wait()

    def test_stdout_stderr_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen([sys.executable, "-c", "import sys; sys.exit(0)"],
                                 stdout=inout, stderr=inout)
            p.wait()

    def test_stderr_stdin_are_single_inout_fd(self):
        with io.open(os.devnull, "r+") as inout:
            p = subprocess.Popen([sys.executable, "-c", "import sys; sys.exit(0)"],
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
                         stderr.decode('utf8'))

    def test_select_unbuffered(self):
        # Issue #11459: bufsize=0 should really set the pipes as
        # unbuffered (and therefore let select() work properly).
        select = support.import_module("select")
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
        self.assertIn(b"physalis", p.stdout.read())

    def test_shell_string(self):
        # Run command through the shell (string)
        newenv = os.environ.copy()
        newenv["FRUIT"] = "physalis"
        p = subprocess.Popen("set", shell=1,
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.addCleanup(p.stdout.close)
        self.assertIn(b"physalis", p.stdout.read())

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
        self.assertStderrEqual(stderr, b'')
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


# The module says:
#   "NB This only works (and is only relevant) for UNIX."
#
# Actually, getoutput should work on any platform with an os.popen, but
# I'll take the comment as given, and skip this suite.
@unittest.skipUnless(os.name == 'posix', "only relevant for UNIX")
class CommandTests(unittest.TestCase):
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

            status, output = subprocess.getstatusoutput('cat ' + name)
            self.assertNotEqual(status, 0)
        finally:
            if dir is not None:
                os.rmdir(dir)


@unittest.skipUnless(getattr(subprocess, '_has_poll', False),
                     "poll system call not supported")
class ProcessTestCaseNoPoll(ProcessTestCase):
    def setUp(self):
        subprocess._has_poll = False
        ProcessTestCase.setUp(self)

    def tearDown(self):
        subprocess._has_poll = True
        ProcessTestCase.tearDown(self)


@unittest.skipUnless(getattr(subprocess, '_posixsubprocess', False),
                     "_posixsubprocess extension module not found.")
class ProcessTestCasePOSIXPurePython(ProcessTestCase, POSIXProcessTestCase):
    @classmethod
    def setUpClass(cls):
        global subprocess
        assert subprocess._posixsubprocess
        # Reimport subprocess while forcing _posixsubprocess to not exist.
        with support.check_warnings(('.*_posixsubprocess .* not being used.*',
                                     RuntimeWarning)):
            subprocess = support.import_fresh_module(
                    'subprocess', blocked=['_posixsubprocess'])
        assert not subprocess._posixsubprocess

    @classmethod
    def tearDownClass(cls):
        global subprocess
        # Reimport subprocess as it should be, restoring order to the universe.
        subprocess = support.import_fresh_module('subprocess')
        assert subprocess._posixsubprocess


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


@unittest.skipUnless(mswindows, "Windows-specific tests")
class CommandsWithSpaces (BaseTestCase):

    def setUp(self):
        super().setUp()
        f, fname = mkstemp(".py", "te st")
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


class ContextManagerTests(BaseTestCase):

    def test_pipe(self):
        with subprocess.Popen([sys.executable, "-c",
                               "import sys;"
                               "sys.stdout.write('stdout');"
                               "sys.stderr.write('stderr');"],
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE) as proc:
            self.assertEqual(proc.stdout.read(), b"stdout")
            self.assertStderrEqual(proc.stderr.read(), b"stderr")

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
        with self.assertRaises(EnvironmentError) as c:
            with subprocess.Popen(['nonexisting_i_hope'],
                                  stdout=subprocess.PIPE,
                                  stderr=subprocess.PIPE) as proc:
                pass

        self.assertEqual(c.exception.errno, errno.ENOENT)


def test_main():
    unit_tests = (ProcessTestCase,
                  POSIXProcessTestCase,
                  Win32ProcessTestCase,
                  ProcessTestCasePOSIXPurePython,
                  CommandTests,
                  ProcessTestCaseNoPoll,
                  HelperFunctionTests,
                  CommandsWithSpaces,
                  ContextManagerTests,
                  )

    support.run_unittest(*unit_tests)
    support.reap_children()

if __name__ == "__main__":
    unittest.main()
