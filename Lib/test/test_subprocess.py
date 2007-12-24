import unittest
from test import test_support
import subprocess
import sys
import signal
import os
import tempfile
import time
import re

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

class ProcessTestCase(unittest.TestCase):
    def setUp(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        if hasattr(test_support, "reap_children"):
            test_support.reap_children()

    def tearDown(self):
        # Try to minimize the number of children we have so this test
        # doesn't crash on some buildbots (Alphas in particular).
        if hasattr(test_support, "reap_children"):
            test_support.reap_children()

    def mkstemp(self):
        """wrapper for mkstemp, calling mktemp if mkstemp is not available"""
        if hasattr(tempfile, "mkstemp"):
            return tempfile.mkstemp()
        else:
            fname = tempfile.mktemp()
            return os.open(fname, os.O_RDWR|os.O_CREAT), fname

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
        p = subprocess.Popen(["somethingyoudonthave",
                              "-c", "import sys; sys.exit(47)"],
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
        cmd = r"import sys, os; sys.exit(os.write(sys.stdout.fileno(), '.\n'))"
        rc = subprocess.call([sys.executable, "-c", cmd], stdout=1)
        self.assertEquals(rc, 2)

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
        self.assert_(stderr.startswith(b"pineapple"))

    def test_communicate(self):
        p = subprocess.Popen([sys.executable, "-c",
                              'import sys,os;'
                              'sys.stderr.write("pineapple");'
                              'sys.stdout.write(sys.stdin.read())'],
                             stdin=subprocess.PIPE,
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (stdout, stderr) = p.communicate("banana")
        self.assertEqual(stdout, b"banana")
        self.assertEqual(remove_stderr_debug_decorations(stderr),
                         b"pineapple")

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
        if (not hasattr(test_support, "is_resource_enabled") or
            test_support.is_resource_enabled("subprocess") and not mswindows):
            max_handles = 1026 # too much for most UNIX systems
        else:
            max_handles = 65
        for i in range(max_handles):
            p = subprocess.Popen([sys.executable, "-c",
                                  "import sys;"
                                  "sys.stdout.write(sys.stdin.read())"],
                                 stdin=subprocess.PIPE,
                                 stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            data = p.communicate("lime")[0]
            self.assertEqual(data, b"lime")


    def test_list2cmdline(self):
        self.assertEqual(subprocess.list2cmdline(['a b c', 'd', 'e']),
                         '"a b c" d e')
        self.assertEqual(subprocess.list2cmdline(['ab"c', '\\', 'd']),
                         'ab\\"c \\ d')
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
        self.assert_(count >= 2)
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
            f, fname = self.mkstemp()
            os.write(f, "#!/bin/sh\n")
            os.write(f, "exec %s -c 'import sys; sys.exit(47)'\n" %
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
            f, fname = self.mkstemp()
            os.write(f, "#!/bin/sh\n")
            os.write(f, "exec %s -c 'import sys; sys.exit(47)'\n" %
                        sys.executable)
            os.close(f)
            os.chmod(fname, 0o700)
            rc = subprocess.call(fname)
            os.remove(fname)
            self.assertEqual(rc, 47)


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


def test_main():
    test_support.run_unittest(ProcessTestCase)
    if hasattr(test_support, "reap_children"):
        test_support.reap_children()

if __name__ == "__main__":
    unittest.main()  # XXX test_main()
