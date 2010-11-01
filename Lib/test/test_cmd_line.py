# Tests invocation of the interpreter with various command line arguments
# Most tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import test.support, unittest
import os
import sys
from test.script_helper import spawn_python, kill_python, assert_python_ok, assert_python_failure

# spawn_python normally enforces use of -E to avoid environmental effects
# but one test checks PYTHONPATH behaviour explicitly
# XXX (ncoghlan): Give script_helper.spawn_python an option to switch
# off the -E flag that is normally inserted automatically
import subprocess
def _spawn_python_with_env(*args):
    cmd_line = [sys.executable]
    cmd_line.extend(args)
    return subprocess.Popen(cmd_line, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.STDOUT)


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

    def test_q(self):
        self.verify_valid_flag('-Qold')
        self.verify_valid_flag('-Qnew')
        self.verify_valid_flag('-Qwarn')
        self.verify_valid_flag('-Qwarnall')

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

    def test_run_module(self):
        # Test expected operation of the '-m' switch
        # Switch needs an argument
        assert_python_failure('-m')
        # Check we get an error for a nonexistent module
        assert_python_failure('-m', 'fnord43520xyz')
        # Check the runpy module also gives an error for
        # a nonexistent module
        assert_python_failure('-m', 'runpy', 'fnord43520xyz'),
        # All good if module is located and run successfully
        assert_python_ok('-m', 'timeit', '-n', '1'),

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

    @unittest.skipIf(sys.getfilesystemencoding() == 'ascii',
                     'need a filesystem encoding different than ASCII')
    def test_non_ascii(self):
        # Test handling of non-ascii data
        if test.support.verbose:
            import locale
            print('locale encoding = %s, filesystem encoding = %s'
                  % (locale.getpreferredencoding(), sys.getfilesystemencoding()))
        command = "assert(ord('\xe9') == 0xe9)"
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
            decoded = text.decode('utf8', 'surrogateescape')
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
        with test.support.EnvironmentVarGuard() as env:
            path1 = "ABCDE" * 100
            path2 = "FGHIJ" * 100
            env['PYTHONPATH'] = path1 + os.pathsep + path2

            code = """
import sys
path = ":".join(sys.path)
path = path.encode("ascii", "backslashreplace")
sys.stdout.buffer.write(path)"""
            code = code.strip().splitlines()
            code = '; '.join(code)
            p = _spawn_python_with_env('-S', '-c', code)
            stdout, _ = p.communicate()
            p.stdout.close()
            self.assertIn(path1.encode('ascii'), stdout)
            self.assertIn(path2.encode('ascii'), stdout)


def test_main():
    test.support.run_unittest(CmdLineTest)
    test.support.reap_children()

if __name__ == "__main__":
    test_main()
