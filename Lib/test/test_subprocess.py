import unittest
from test import support
import subprocess
import sys
import signal
import os
import errno
import tempfile
import time
import re
import shutil

mswindows = (sys.platform == "win32")

#
# Depends on the following external programs: Python
#

if mswindows:
    SETBINARY = ('import msvcrt; msvcrt.setmode(sys.stdout.fileno(), '
                                                'os.O_BINARY);')
else:
    SETBINARY = ''

# In a debug build, stuff like "[6580 refs]" is printed to stderr at
# shutdown time.  That frustrates tests trying to check stderr produced
# from a spawned Python process.
def remove_stderr_debug_decorations(stderr):
    return re.sub("\[\d+ refs\]\r?\n?$", "", stderr.decode()).encode()
    #return re.sub(r"\[\d+ refs\]\r?\n?$", "", stderr)

class BaseTestCase(unittest.TestCase):
    def setUp(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        if hasattr(support, "reap_children"):
            support.reap_children()

    def tearDown(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        if hasattr(support, "reap_children"):
            support.reap_children()

    def mkstemp(self, *args, **kwargs):
        """wrapper for mkstemp, calling mktemp if mkstemp is not available"""
        if hasattr(tempfile, "mkstemp"):
            return tempfile.mkstemp(*args, **kwargs)
        else:
            fname = tempfile.mktemp(*args, **kwargs)
            return os.open(fname, os.O_RDWR|os.O_CREAT), fname

class ProcessTestCase(BaseTestCase):
    #
    # Generic tests
    #
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
        try:
            subprocess.check_call([sys.executable, "-c",
                                   "import sys; sys.exit(47)"])
        except subprocess.CalledProcessError as e:
            self.assertEqual(e.returncode, 47)
        else:
            self.fail("Expected CalledProcessError")

    def test_check_output(self):
        # check_output() function with zero return code
        output = subprocess.check_output(
                [sys.executable, "-c", "print('BDFL')"])
        self.assertTrue(b'BDFL' in output)

    def test_check_output_nonzero(self):
        # check_call() function with non-zero return code
        try:
            subprocess.check_output(
                    [sys.executable, "-c", "import sys; sys.exit(5)"])
        except subprocess.CalledProcessError as e:
            self.assertEqual(e.returncode, 5)
        else:
            self.fail("Expected CalledProcessError")

    def test_check_output_stderr(self):
        # check_output() function stderr redirected to stdout
        output = subprocess.check_output(
                [sys.executable, "-c", "import sys; sys.stderr.write('BDFL')"],
                stderr=subprocess.STDOUT)
        self.assertTrue(b'BDFL' in output)

    def test_check_output_stdout_arg(self):
        # check_output() function stderr redirected to stdout
        try:
            output = subprocess.check_output(
                    [sys.executable, "-c", "print('will not be run')"],
                    stdout=sys.stdout)
        except ValueError as e:
            self.assertTrue('stdout' in e.args[0])
        else:
            self.fail("Expected ValueError when stdout arg supplied.")

    def test_call_kwargs(self):
        # call() function with keyword args
        newenv = os.environ.copy()
        newenv["FRUIT"] = "banana"
        rc = subprocess.call([sys.executable, "-c",
                              'import sys, os;'
                              'sys.exit(os.getenv("FRUIT")=="banana")'],
                             env=newenv)
        self.assertEqual(rc, 1)

    def test_stdin_none(self):
        # .stdin is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print("banana")'],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        p.wait()
        self.assertEqual(p.stdin, None)

    def test_stdout_none(self):
        # .stdout is None when not redirected
        p = subprocess.Popen([sys.executable, "-c",
                             'print("    this bit of output is from a '
                             'test of stdout in a different '
                             'process ...")'],
                             stdin=subprocess.PIPE, stderr=subprocess.PIPE)
        p.wait()
        self.assertEqual(p.stdout, None)

    def test_stderr_none(self):
        # .stderr is None when not redirected
        p = subprocess.Popen([sys.executable, "-c", 'print("banana")'],
                         stdin=subprocess.PIPE, stdout=subprocess.PIPE)
        p.wait()
        self.assertEqual(p.stderr, None)

    def test_executable(self):
        arg0 = os.path.join(os.path.dirname(sys.executable),
                            "somethingyoudonthave")
        p = subprocess.Popen([arg0, "-c", "import sys; sys.exit(47)"],
                             executable=sys.executable)
        p.wait()
        self.assertEqual(p.returncode, 47)

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
        self.assertEqual(p.stdout.read(), b"orange")

    def test_stdout_filedes(self):
        # stdout is set to open file descriptor
        tf = tempfile.TemporaryFile()
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
        self.assertEqual(remove_stderr_debug_decorations(p.stderr.read()),
                         b"strawberry")

    def test_stderr_filedes(self):
        # stderr is set to open file descriptor
        tf = tempfile.TemporaryFile()
        d = tf.fileno()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=d)
        p.wait()
        os.lseek(d, 0, 0)
        self.assertEqual(remove_stderr_debug_decorations(os.read(d, 1024)),
                         b"strawberry")

    def test_stderr_fileobj(self):
        # stderr is set to open file object
        tf = tempfile.TemporaryFile()
        p = subprocess.Popen([sys.executable, "-c",
                          'import sys; sys.stderr.write("strawberry")'],
                         stderr=tf)
        p.wait()
        tf.seek(0)
        self.assertEqual(remove_stderr_debug_decorations(tf.read()),
                         b"strawberry")

    def test_stdout_stderr_pipe(self):
        # capture stdout and stderr to the same pipe
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys;'
                              'sys.stdout.write("apple");'
                              'sys.stdout.flush();'
                              'sys.stderr.write("orange")'],
                             stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        output = p.stdout.read()
        stripped = remove_stderr_debug_decorations(output)
        self.assertEqual(stripped, b"appleorange")

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
        output = tf.read()
        stripped = remove_stderr_debug_decorations(output)
        self.assertEqual(stripped, b"appleorange")

    def test_stdout_filedes_of_stdout(self):
        # stdout is set to 1 (#1531862).
        cmd = r"import sys, os; sys.exit(os.write(sys.stdout.fileno(), b'.\n'))"
        rc = subprocess.call([sys.executable, "-c", cmd], stdout=1)
        self.assertEqual(rc, 2)

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
        normcase = os.path.normcase
        self.assertEqual(normcase(p.stdout.read().decode("utf-8")),
                         normcase(tmpdir))

    def test_env(self):
        newenv = os.environ.copy()
        newenv["FRUIT"] = "orange"
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stdout.write(os.getenv("FRUIT"))'],
                             stdout=subprocess.PIPE,
                             env=newenv)
        self.assertEqual(p.stdout.read(), b"orange")

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
        # When running with a pydebug build, the # of references is outputted
        # to stderr, so just check if stderr at least started with "pinapple"
        self.assertEqual(remove_stderr_debug_decorations(stderr), b"pineapple")

    def test_communicate(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stderr.write("pineapple");'
                              'sys.stdout.write(sys.stdin.read())'],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate(b"banana")
        self.assertEqual(stdout, b"banana")
        self.assertEqual(remove_stderr_debug_decorations(stderr),
                         b"pineapple")

    # This test is Linux specific for simplicity to at least have
    # some coverage.  It is not a platform specific bug.
    if os.path.isdir('/proc/%d/fd' % os.getpid()):
        # Test for the fd leak reported in http://bugs.python.org/issue2791.
        def test_communicate_pipe_fd_leak(self):
            fd_directory = '/proc/%d/fd' % os.getpid()
            num_fds_before_popen = len(os.listdir(fd_directory))
            p = subprocess.Popen([sys.executable, '-c', 'print()'],
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
        p.stdin.write(b"banana")
        (stdout, stderr) = p.communicate(b"split")
        self.assertEqual(stdout, b"bananasplit")
        self.assertEqual(remove_stderr_debug_decorations(stderr), b"")

    def test_universal_newlines(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY +
                              'sys.stdout.write("line1\\n");'
                              'sys.stdout.flush();'
                              'sys.stdout.write("line2\\n");'
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
        stdout = p.stdout.read()
        self.assertEqual(stdout, "line1\nline2\nline3\nline4\nline5\nline6")

    def test_universal_newlines_communicate(self):
        # universal newlines through communicate()
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;' + SETBINARY +
                              'sys.stdout.write("line1\\n");'
                              'sys.stdout.flush();'
                              'sys.stdout.write("line2\\n");'
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
        (stdout, stderr) = p.communicate()
        self.assertEqual(stdout, "line1\nline2\nline3\nline4\nline5\nline6")

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
        self.assertTrue(count >= 2)
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
        try:
            subprocess.Popen([sys.executable, "-c", "pass"], "orange")
        except TypeError:
            pass
        else:
            self.fail("Expected TypeError")

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
            try:
                subprocess.Popen(['nonexisting_i_hope'],
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            # Windows raises IOError
            except (IOError, OSError) as err:
                # ignore errors that indicate the command was not found
                if err.errno not in (errno.ENOENT, errno.EACCES):
                    raise

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
        ifhandle, ifname = self.mkstemp()
        ofhandle, ofname = self.mkstemp()
        efhandle, efname = self.mkstemp()
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

    #
    # POSIX tests
    #
    if not mswindows:
        def test_exceptions(self):
            # caught & re-raised exceptions
            try:
                p = subprocess.Popen([sys.executable, "-c", ""],
                                     cwd="/this/path/does/not/exist")
            except OSError as e:
                # The attribute child_traceback should contain "os.chdir"
                # somewhere.
                self.assertNotEqual(e.child_traceback.find("os.chdir"), -1)
            else:
                self.fail("Expected OSError")

        def _suppress_core_files(self):
            """Try to prevent core files from being created.
            Returns previous ulimit if successful, else None.
            """
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

            try:
                import resource
                old_limit = resource.getrlimit(resource.RLIMIT_CORE)
                resource.setrlimit(resource.RLIMIT_CORE, (0,0))
                return old_limit
            except (ImportError, ValueError, resource.error):
                return None



        def _unsuppress_core_files(self, old_limit):
            """Return core file behavior to default."""
            if old_limit is None:
                return
            try:
                import resource
                resource.setrlimit(resource.RLIMIT_CORE, old_limit)
            except (ImportError, ValueError, resource.error):
                return

        def test_run_abort(self):
            # returncode handles signal termination
            old_limit = self._suppress_core_files()
            try:
                p = subprocess.Popen([sys.executable,
                                      "-c", "import os; os.abort()"])
            finally:
                self._unsuppress_core_files(old_limit)
            p.wait()
            self.assertEqual(-p.returncode, signal.SIGABRT)

        def test_preexec(self):
            # preexec function
            p = subprocess.Popen([sys.executable, "-c",
                                  'import sys,os;'
                                  'sys.stdout.write(os.getenv("FRUIT"))'],
                                 stdout=subprocess.PIPE,
                                 preexec_fn=lambda: os.putenv("FRUIT",
                                                              "apple"))
            self.assertEqual(p.stdout.read(), b"apple")

        def test_args_string(self):
            # args is a string
            fd, fname = self.mkstemp()
            # reopen in text mode
            with open(fd, "w") as fobj:
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
                              [sys.executable,
                               "-c", "import sys; sys.exit(47)"],
                              startupinfo=47)
            self.assertRaises(ValueError, subprocess.call,
                              [sys.executable,
                               "-c", "import sys; sys.exit(47)"],
                              creationflags=47)

        def test_shell_sequence(self):
            # Run command through the shell (sequence)
            newenv = os.environ.copy()
            newenv["FRUIT"] = "apple"
            p = subprocess.Popen(["echo $FRUIT"], shell=1,
                                 stdout=subprocess.PIPE,
                                 env=newenv)
            self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

        def test_shell_string(self):
            # Run command through the shell (string)
            newenv = os.environ.copy()
            newenv["FRUIT"] = "apple"
            p = subprocess.Popen("echo $FRUIT", shell=1,
                                 stdout=subprocess.PIPE,
                                 env=newenv)
            self.assertEqual(p.stdout.read().strip(b" \t\r\n\f"), b"apple")

        def test_call_string(self):
            # call() function with string argument on UNIX
            fd, fname = self.mkstemp()
            # reopen in text mode
            with open(fd, "w") as fobj:
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
                self.assertEqual(p.stdout.read().strip(), bytes(sh, 'ascii'))

        def DISABLED_test_send_signal(self):
            p = subprocess.Popen([sys.executable,
                              "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.send_signal(signal.SIGINT)
            self.assertNotEqual(p.wait(), 0)

        def DISABLED_test_kill(self):
            p = subprocess.Popen([sys.executable,
                            "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.kill()
            self.assertEqual(p.wait(), -signal.SIGKILL)

        def DISABLED_test_terminate(self):
            p = subprocess.Popen([sys.executable,
                            "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.terminate()
            self.assertEqual(p.wait(), -signal.SIGTERM)

        def test_undecodable_env(self):
            for key, value in (('test', 'abc\uDCFF'), ('test\uDCFF', '42')):
                value_repr = ascii(value).encode("ascii")

                # test str with surrogates
                script = "import os; print(ascii(os.getenv(%s)))" % repr(key)
                env = os.environ.copy()
                env[key] = value
                # Force surrogate-escaping of \xFF in the child process;
                # otherwise it can be decoded as-is if the default locale
                # is latin-1.
                env['PYTHONFSENCODING'] = 'ascii'
                stdout = subprocess.check_output(
                    [sys.executable, "-c", script],
                    env=env)
                stdout = stdout.rstrip(b'\n\r')
                self.assertEqual(stdout, value_repr)

                # test bytes
                key = key.encode("ascii", "surrogateescape")
                value = value.encode("ascii", "surrogateescape")
                script = "import os; print(ascii(os.getenv(%s)))" % repr(key)
                env = os.environ.copy()
                env[key] = value
                stdout = subprocess.check_output(
                    [sys.executable, "-c", script],
                    env=env)
                stdout = stdout.rstrip(b'\n\r')
                self.assertEqual(stdout, value_repr)

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

        def test_surrogates_error_message(self):
            def prepare():
                raise ValueError("surrogate:\uDCff")

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
            try:
                self.assertEqual(f.read(4), b"appl")
                self.assertIn(f, select.select([f], [], [], 0.0)[0])
            finally:
                p.wait()

    #
    # Windows tests
    #
    if mswindows:
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
                              [sys.executable,
                               "-c", "import sys; sys.exit(47)"],
                              preexec_fn=lambda: 1)
            self.assertRaises(ValueError, subprocess.call,
                              [sys.executable,
                               "-c", "import sys; sys.exit(47)"],
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
            self.assertNotEqual(p.stdout.read().find(b"physalis"), -1)

        def test_shell_string(self):
            # Run command through the shell (string)
            newenv = os.environ.copy()
            newenv["FRUIT"] = "physalis"
            p = subprocess.Popen("set", shell=1,
                                 stdout=subprocess.PIPE,
                                 env=newenv)
            self.assertNotEqual(p.stdout.read().find(b"physalis"), -1)

        def test_call_string(self):
            # call() function with string argument on Windows
            rc = subprocess.call(sys.executable +
                                 ' -c "import sys; sys.exit(47)"')
            self.assertEqual(rc, 47)

        def DISABLED_test_send_signal(self):
            p = subprocess.Popen([sys.executable,
                              "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.send_signal(signal.SIGTERM)
            self.assertNotEqual(p.wait(), 0)

        def DISABLED_test_kill(self):
            p = subprocess.Popen([sys.executable,
                            "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.kill()
            self.assertNotEqual(p.wait(), 0)

        def DISABLED_test_terminate(self):
            p = subprocess.Popen([sys.executable,
                            "-c", "input()"])

            self.assertTrue(p.poll() is None, p.poll())
            p.terminate()
            self.assertNotEqual(p.wait(), 0)


class CommandTests(unittest.TestCase):
# The module says:
#   "NB This only works (and is only relevant) for UNIX."
#
# Actually, getoutput should work on any platform with an os.popen, but
# I'll take the comment as given, and skip this suite.
    if os.name == 'posix':

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


unit_tests = [ProcessTestCase, CommandTests]

if mswindows:
    class CommandsWithSpaces (BaseTestCase):

        def setUp(self):
            super().setUp()
            f, fname = self.mkstemp(".py", "te st")
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

    unit_tests.append(CommandsWithSpaces)


if getattr(subprocess, '_has_poll', False):
    class ProcessTestCaseNoPoll(ProcessTestCase):
        def setUp(self):
            subprocess._has_poll = False
            ProcessTestCase.setUp(self)

        def tearDown(self):
            subprocess._has_poll = True
            ProcessTestCase.tearDown(self)

    unit_tests.append(ProcessTestCaseNoPoll)


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

unit_tests.append(HelperFunctionTests)

def test_main():
    support.run_unittest(*unit_tests)
    support.reap_children()

if __name__ == "__main__":
    test_main()
