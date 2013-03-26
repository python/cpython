# Tests invocation of the interpreter with various command line arguments
# Most tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import test.support, unittest
import os
import sys
import subprocess
import tempfile
from test.script_helper import (spawn_python, kill_python, assert_python_ok,
    assert_python_failure)


# XXX (ncoghlan): Move to script_helper and make consistent with run_python
def _kill_python_and_exit_code(p):
    data = kill_python(p)
    returncode = p.wait()
    return data, returncode

class CmdLineTest(unittest.TestCase):
    def test_directories(self):
        assert_python_failure('.')
        assert_python_failure('< .')

    def verify_valid_flag(self, cmd_line):
        rc, out, err = assert_python_ok(*cmd_line)
        self.assertTrue(out == b'' or out.endswith(b'\n'))
        self.assertNotIn(b'Traceback', out)
        self.assertNotIn(b'Traceback', err)

    def test_optimize(self):
        self.verify_valid_flag('-O')
        self.verify_valid_flag('-OO')

    def test_site_flag(self):
        self.verify_valid_flag('-S')

    def test_usage(self):
        rc, out, err = assert_python_ok('-h')
        self.assertIn(b'usage', out)

    def test_version(self):
        version = ('Python %d.%d' % sys.version_info[:2]).encode("ascii")
        rc, out, err = assert_python_ok('-V')
        self.assertTrue(err.startswith(version))

    def test_verbose(self):
        # -v causes imports to write to stderr.  If the write to
        # stderr itself causes an import to happen (for the output
        # codec), a recursion loop can occur.
        rc, out, err = assert_python_ok('-v')
        self.assertNotIn(b'stack overflow', err)
        rc, out, err = assert_python_ok('-vv')
        self.assertNotIn(b'stack overflow', err)

    def test_xoptions(self):
        rc, out, err = assert_python_ok('-c', 'import sys; print(sys._xoptions)')
        opts = eval(out.splitlines()[0])
        self.assertEqual(opts, {})
        rc, out, err = assert_python_ok(
            '-Xa', '-Xb=c,d=e', '-c', 'import sys; print(sys._xoptions)')
        opts = eval(out.splitlines()[0])
        self.assertEqual(opts, {'a': True, 'b': 'c,d=e'})

    def test_showrefcount(self):
        def run_python(*args):
            # this is similar to assert_python_ok but doesn't strip
            # the refcount from stderr.  It can be replaced once
            # assert_python_ok stops doing that.
            cmd = [sys.executable]
            cmd.extend(args)
            PIPE = subprocess.PIPE
            p = subprocess.Popen(cmd, stdout=PIPE, stderr=PIPE)
            out, err = p.communicate()
            p.stdout.close()
            p.stderr.close()
            rc = p.returncode
            self.assertEqual(rc, 0)
            return rc, out, err
        code = 'import sys; print(sys._xoptions)'
        # normally the refcount is hidden
        rc, out, err = run_python('-c', code)
        self.assertEqual(out.rstrip(), b'{}')
        self.assertEqual(err, b'')
        # "-X showrefcount" shows the refcount, but only in debug builds
        rc, out, err = run_python('-X', 'showrefcount', '-c', code)
        self.assertEqual(out.rstrip(), b"{'showrefcount': True}")
        if hasattr(sys, 'gettotalrefcount'):  # debug build
            self.assertRegex(err, br'^\[\d+ refs, \d+ blocks\]')
        else:
            self.assertEqual(err, b'')

    def test_run_module(self):
        # Test expected operation of the '-m' switch
        # Switch needs an argument
        assert_python_failure('-m')
        # Check we get an error for a nonexistent module
        assert_python_failure('-m', 'fnord43520xyz')
        # Check the runpy module also gives an error for
        # a nonexistent module
        assert_python_failure('-m', 'runpy', 'fnord43520xyz')
        # All good if module is located and run successfully
        assert_python_ok('-m', 'timeit', '-n', '1')

    def test_run_module_bug1764407(self):
        # -m and -i need to play well together
        # Runs the timeit module and checks the __main__
        # namespace has been populated appropriately
        p = spawn_python('-i', '-m', 'timeit', '-n', '1')
        p.stdin.write(b'Timer\n')
        p.stdin.write(b'exit()\n')
        data = kill_python(p)
        self.assertTrue(data.find(b'1 loop') != -1)
        self.assertTrue(data.find(b'__main__.Timer') != -1)

    def test_run_code(self):
        # Test expected operation of the '-c' switch
        # Switch needs an argument
        assert_python_failure('-c')
        # Check we get an error for an uncaught exception
        assert_python_failure('-c', 'raise Exception')
        # All good if execution is successful
        assert_python_ok('-c', 'pass')

    @unittest.skipUnless(test.support.FS_NONASCII, 'need support.FS_NONASCII')
    def test_non_ascii(self):
        # Test handling of non-ascii data
        command = ("assert(ord(%r) == %s)"
                   % (test.support.FS_NONASCII, ord(test.support.FS_NONASCII)))
        assert_python_ok('-c', command)

    # On Windows, pass bytes to subprocess doesn't test how Python decodes the
    # command line, but how subprocess does decode bytes to unicode. Python
    # doesn't decode the command line because Windows provides directly the
    # arguments as unicode (using wmain() instead of main()).
    @unittest.skipIf(sys.platform == 'win32',
                     'Windows has a native unicode API')
    def test_undecodable_code(self):
        undecodable = b"\xff"
        env = os.environ.copy()
        # Use C locale to get ascii for the locale encoding
        env['LC_ALL'] = 'C'
        code = (
            b'import locale; '
            b'print(ascii("' + undecodable + b'"), '
                b'locale.getpreferredencoding())')
        p = subprocess.Popen(
            [sys.executable, "-c", code],
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            env=env)
        stdout, stderr = p.communicate()
        if p.returncode == 1:
            # _Py_char2wchar() decoded b'\xff' as '\udcff' (b'\xff' is not
            # decodable from ASCII) and run_command() failed on
            # PyUnicode_AsUTF8String(). This is the expected behaviour on
            # Linux.
            pattern = b"Unable to decode the command from the command line:"
        elif p.returncode == 0:
            # _Py_char2wchar() decoded b'\xff' as '\xff' even if the locale is
            # C and the locale encoding is ASCII. It occurs on FreeBSD, Solaris
            # and Mac OS X.
            pattern = b"'\\xff' "
            # The output is followed by the encoding name, an alias to ASCII.
            # Examples: "US-ASCII" or "646" (ISO 646, on Solaris).
        else:
            raise AssertionError("Unknown exit code: %s, output=%a" % (p.returncode, stdout))
        if not stdout.startswith(pattern):
            raise AssertionError("%a doesn't start with %a" % (stdout, pattern))

    @unittest.skipUnless(sys.platform == 'darwin', 'test specific to Mac OS X')
    def test_osx_utf8(self):
        def check_output(text):
            decoded = text.decode('utf-8', 'surrogateescape')
            expected = ascii(decoded).encode('ascii') + b'\n'

            env = os.environ.copy()
            # C locale gives ASCII locale encoding, but Python uses UTF-8
            # to parse the command line arguments on Mac OS X
            env['LC_ALL'] = 'C'

            p = subprocess.Popen(
                (sys.executable, "-c", "import sys; print(ascii(sys.argv[1]))", text),
                stdout=subprocess.PIPE,
                env=env)
            stdout, stderr = p.communicate()
            self.assertEqual(stdout, expected)
            self.assertEqual(p.returncode, 0)

        # test valid utf-8
        text = 'e:\xe9, euro:\u20ac, non-bmp:\U0010ffff'.encode('utf-8')
        check_output(text)

        # test invalid utf-8
        text = (
            b'\xff'         # invalid byte
            b'\xc3\xa9'     # valid utf-8 character
            b'\xc3\xff'     # invalid byte sequence
            b'\xed\xa0\x80' # lone surrogate character (invalid)
        )
        check_output(text)

    def test_unbuffered_output(self):
        # Test expected operation of the '-u' switch
        for stream in ('stdout', 'stderr'):
            # Binary is unbuffered
            code = ("import os, sys; sys.%s.buffer.write(b'x'); os._exit(0)"
                % stream)
            rc, out, err = assert_python_ok('-u', '-c', code)
            data = err if stream == 'stderr' else out
            self.assertEqual(data, b'x', "binary %s not unbuffered" % stream)
            # Text is line-buffered
            code = ("import os, sys; sys.%s.write('x\\n'); os._exit(0)"
                % stream)
            rc, out, err = assert_python_ok('-u', '-c', code)
            data = err if stream == 'stderr' else out
            self.assertEqual(data.strip(), b'x',
                "text %s not line-buffered" % stream)

    def test_unbuffered_input(self):
        # sys.stdin still works with '-u'
        code = ("import sys; sys.stdout.write(sys.stdin.read(1))")
        p = spawn_python('-u', '-c', code)
        p.stdin.write(b'x')
        p.stdin.flush()
        data, rc = _kill_python_and_exit_code(p)
        self.assertEqual(rc, 0)
        self.assertTrue(data.startswith(b'x'), data)

    def test_large_PYTHONPATH(self):
        path1 = "ABCDE" * 100
        path2 = "FGHIJ" * 100
        path = path1 + os.pathsep + path2

        code = """if 1:
            import sys
            path = ":".join(sys.path)
            path = path.encode("ascii", "backslashreplace")
            sys.stdout.buffer.write(path)"""
        rc, out, err = assert_python_ok('-S', '-c', code,
                                        PYTHONPATH=path)
        self.assertIn(path1.encode('ascii'), out)
        self.assertIn(path2.encode('ascii'), out)

    def test_empty_PYTHONPATH_issue16309(self):
        # On Posix, it is documented that setting PATH to the
        # empty string is equivalent to not setting PATH at all,
        # which is an exception to the rule that in a string like
        # "/bin::/usr/bin" the empty string in the middle gets
        # interpreted as '.'
        code = """if 1:
            import sys
            path = ":".join(sys.path)
            path = path.encode("ascii", "backslashreplace")
            sys.stdout.buffer.write(path)"""
        rc1, out1, err1 = assert_python_ok('-c', code, PYTHONPATH="")
        rc2, out2, err2 = assert_python_ok('-c', code)
        # regarding to Posix specification, outputs should be equal
        # for empty and unset PYTHONPATH
        self.assertEqual(out1, out2)

    def test_displayhook_unencodable(self):
        for encoding in ('ascii', 'latin-1', 'utf-8'):
            env = os.environ.copy()
            env['PYTHONIOENCODING'] = encoding
            p = subprocess.Popen(
                [sys.executable, '-i'],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                env=env)
            # non-ascii, surrogate, non-BMP printable, non-BMP unprintable
            text = "a=\xe9 b=\uDC80 c=\U00010000 d=\U0010FFFF"
            p.stdin.write(ascii(text).encode('ascii') + b"\n")
            p.stdin.write(b'exit()\n')
            data = kill_python(p)
            escaped = repr(text).encode(encoding, 'backslashreplace')
            self.assertIn(escaped, data)

    def check_input(self, code, expected):
        with tempfile.NamedTemporaryFile("wb+") as stdin:
            sep = os.linesep.encode('ASCII')
            stdin.write(sep.join((b'abc', b'def')))
            stdin.flush()
            stdin.seek(0)
            with subprocess.Popen(
                (sys.executable, "-c", code),
                stdin=stdin, stdout=subprocess.PIPE) as proc:
                stdout, stderr = proc.communicate()
        self.assertEqual(stdout.rstrip(), expected)

    def test_stdin_readline(self):
        # Issue #11272: check that sys.stdin.readline() replaces '\r\n' by '\n'
        # on Windows (sys.stdin is opened in binary mode)
        self.check_input(
            "import sys; print(repr(sys.stdin.readline()))",
            b"'abc\\n'")

    def test_builtin_input(self):
        # Issue #11272: check that input() strips newlines ('\n' or '\r\n')
        self.check_input(
            "print(repr(input()))",
            b"'abc'")

    def test_output_newline(self):
        # Issue 13119 Newline for print() should be \r\n on Windows.
        code = """if 1:
            import sys
            print(1)
            print(2)
            print(3, file=sys.stderr)
            print(4, file=sys.stderr)"""
        rc, out, err = assert_python_ok('-c', code)

        if sys.platform == 'win32':
            self.assertEqual(b'1\r\n2\r\n', out)
            self.assertEqual(b'3\r\n4', err)
        else:
            self.assertEqual(b'1\n2\n', out)
            self.assertEqual(b'3\n4', err)

    def test_unmached_quote(self):
        # Issue #10206: python program starting with unmatched quote
        # spewed spaces to stdout
        rc, out, err = assert_python_failure('-c', "'")
        self.assertRegex(err.decode('ascii', 'ignore'), 'SyntaxError')
        self.assertEqual(b'', out)

    def test_stdout_flush_at_shutdown(self):
        # Issue #5319: if stdout.flush() fails at shutdown, an error should
        # be printed out.
        code = """if 1:
            import os, sys
            sys.stdout.write('x')
            os.close(sys.stdout.fileno())"""
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(b'', out)
        self.assertRegex(err.decode('ascii', 'ignore'),
                         'Exception ignored in.*\nOSError: .*')

    def test_closed_stdout(self):
        # Issue #13444: if stdout has been explicitly closed, we should
        # not attempt to flush it at shutdown.
        code = "import sys; sys.stdout.close()"
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(b'', err)

    # Issue #7111: Python should work without standard streams

    @unittest.skipIf(os.name != 'posix', "test needs POSIX semantics")
    def _test_no_stdio(self, streams):
        code = """if 1:
            import os, sys
            for i, s in enumerate({streams}):
                if getattr(sys, s) is not None:
                    os._exit(i + 1)
            os._exit(42)""".format(streams=streams)
        def preexec():
            if 'stdin' in streams:
                os.close(0)
            if 'stdout' in streams:
                os.close(1)
            if 'stderr' in streams:
                os.close(2)
        p = subprocess.Popen(
            [sys.executable, "-E", "-c", code],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            preexec_fn=preexec)
        out, err = p.communicate()
        self.assertEqual(test.support.strip_python_stderr(err), b'')
        self.assertEqual(p.returncode, 42)

    def test_no_stdin(self):
        self._test_no_stdio(['stdin'])

    def test_no_stdout(self):
        self._test_no_stdio(['stdout'])

    def test_no_stderr(self):
        self._test_no_stdio(['stderr'])

    def test_no_std_streams(self):
        self._test_no_stdio(['stdin', 'stdout', 'stderr'])

    def test_hash_randomization(self):
        # Verify that -R enables hash randomization:
        self.verify_valid_flag('-R')
        hashes = []
        for i in range(2):
            code = 'print(hash("spam"))'
            rc, out, err = assert_python_ok('-c', code)
            self.assertEqual(rc, 0)
            hashes.append(out)
        self.assertNotEqual(hashes[0], hashes[1])

        # Verify that sys.flags contains hash_randomization
        code = 'import sys; print("random is", sys.flags.hash_randomization)'
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(rc, 0)
        self.assertIn(b'random is 1', out)

    def test_del___main__(self):
        # Issue #15001: PyRun_SimpleFileExFlags() did crash because it kept a
        # borrowed reference to the dict of __main__ module and later modify
        # the dict whereas the module was destroyed
        filename = test.support.TESTFN
        self.addCleanup(test.support.unlink, filename)
        with open(filename, "w") as script:
            print("import sys", file=script)
            print("del sys.modules['__main__']", file=script)
        assert_python_ok(filename)

    def test_unknown_options(self):
        rc, out, err = assert_python_failure('-E', '-z')
        self.assertIn(b'Unknown option: -z', err)
        self.assertEqual(err.splitlines().count(b'Unknown option: -z'), 1)
        self.assertEqual(b'', out)
        # Add "without='-E'" to prevent _assert_python to append -E
        # to env_vars and change the output of stderr
        rc, out, err = assert_python_failure('-z', without='-E')
        self.assertIn(b'Unknown option: -z', err)
        self.assertEqual(err.splitlines().count(b'Unknown option: -z'), 1)
        self.assertEqual(b'', out)
        rc, out, err = assert_python_failure('-a', '-z', without='-E')
        self.assertIn(b'Unknown option: -a', err)
        # only the first unknown option is reported
        self.assertNotIn(b'Unknown option: -z', err)
        self.assertEqual(err.splitlines().count(b'Unknown option: -a'), 1)
        self.assertEqual(b'', out)


def test_main():
    test.support.run_unittest(CmdLineTest)
    test.support.reap_children()

if __name__ == "__main__":
    test_main()
