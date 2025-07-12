# Tests invocation of the interpreter with various command line arguments
# Most tests are executed with environment variables ignored
# See test_cmd_line_script.py for testing of script execution

import os
import subprocess
import sys
import sysconfig
import tempfile
import textwrap
import unittest
import warnings
from test import support
from test.support import os_helper
from test.support import force_not_colorized
from test.support import threading_helper
from test.support.script_helper import (
    spawn_python, kill_python, assert_python_ok, assert_python_failure,
    interpreter_requires_environment
)
from textwrap import dedent


if not support.has_subprocess_support:
    raise unittest.SkipTest("test module requires subprocess")


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
        rc, out, err = assert_python_ok(cmd_line)
        if out != b'':
            self.assertEndsWith(out, b'\n')
        self.assertNotIn(b'Traceback', out)
        self.assertNotIn(b'Traceback', err)
        return out

    @support.cpython_only
    def test_help(self):
        self.verify_valid_flag('-h')
        self.verify_valid_flag('-?')
        out = self.verify_valid_flag('--help')
        lines = out.splitlines()
        self.assertIn(b'usage', lines[0])
        self.assertNotIn(b'PYTHONHOME', out)
        self.assertNotIn(b'-X dev', out)
        self.assertLess(len(lines), 50)

    @support.cpython_only
    def test_help_env(self):
        out = self.verify_valid_flag('--help-env')
        self.assertIn(b'PYTHONHOME', out)

    @support.cpython_only
    def test_help_xoptions(self):
        out = self.verify_valid_flag('--help-xoptions')
        self.assertIn(b'-X dev', out)

    @support.cpython_only
    def test_help_all(self):
        out = self.verify_valid_flag('--help-all')
        lines = out.splitlines()
        self.assertIn(b'usage', lines[0])
        self.assertIn(b'PYTHONHOME', out)
        self.assertIn(b'-X dev', out)

        # The first line contains the program name,
        # but the rest should be ASCII-only
        b''.join(lines[1:]).decode('ascii')

    def test_optimize(self):
        self.verify_valid_flag('-O')
        self.verify_valid_flag('-OO')

    def test_site_flag(self):
        self.verify_valid_flag('-S')

    @support.cpython_only
    def test_version(self):
        version = ('Python %d.%d' % sys.version_info[:2]).encode("ascii")
        for switch in '-V', '--version', '-VV':
            rc, out, err = assert_python_ok(switch)
            self.assertNotStartsWith(err, version)
            self.assertStartsWith(out, version)

    def test_verbose(self):
        # -v causes imports to write to stderr.  If the write to
        # stderr itself causes an import to happen (for the output
        # codec), a recursion loop can occur.
        rc, out, err = assert_python_ok('-v')
        self.assertNotIn(b'stack overflow', err)
        rc, out, err = assert_python_ok('-vv')
        self.assertNotIn(b'stack overflow', err)

    @unittest.skipIf(interpreter_requires_environment(),
                     'Cannot run -E tests when PYTHON env vars are required.')
    def test_xoptions(self):
        def get_xoptions(*args):
            # use subprocess module directly because test.support.script_helper adds
            # "-X faulthandler" to the command line
            args = (sys.executable, '-E') + args
            args += ('-c', 'import sys; print(sys._xoptions)')
            out = subprocess.check_output(args)
            opts = eval(out.splitlines()[0])
            return opts

        opts = get_xoptions()
        self.assertEqual(opts, {})

        opts = get_xoptions('-Xa', '-Xb=c,d=e')
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
        rc, out, err = run_python('-I', '-X', 'showrefcount', '-c', code)
        self.assertEqual(out.rstrip(), b"{'showrefcount': True}")
        if support.Py_DEBUG:
            # bpo-46417: Tolerate negative reference count which can occur
            # because of bugs in C extensions. This test is only about checking
            # the showrefcount feature.
            self.assertRegex(err, br'^\[-?\d+ refs, \d+ blocks\]')
        else:
            self.assertEqual(err, b'')

    @support.cpython_only
    def test_xoption_frozen_modules(self):
        tests = {
            ('=on', 'FrozenImporter'),
            ('=off', 'SourceFileLoader'),
            ('=', 'FrozenImporter'),
            ('', 'FrozenImporter'),
        }
        for raw, expected in tests:
            cmd = ['-X', f'frozen_modules{raw}',
                   '-c', 'import os; print(os.__spec__.loader, end="")']
            with self.subTest(raw):
                res = assert_python_ok(*cmd)
                self.assertRegex(res.out.decode('utf-8'), expected)

    @support.cpython_only
    def test_env_var_frozen_modules(self):
        tests = {
            ('on', 'FrozenImporter'),
            ('off', 'SourceFileLoader'),
        }
        for raw, expected in tests:
            cmd = ['-c', 'import os; print(os.__spec__.loader, end="")']
            with self.subTest(raw):
                res = assert_python_ok(*cmd, PYTHON_FROZEN_MODULES=raw)
                self.assertRegex(res.out.decode('utf-8'), expected)

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

    def test_relativedir_bug46421(self):
        # Test `python -m unittest` with a relative directory beginning with ./
        # Note: We have to switch to the project's top module's directory, as per
        # the python unittest wiki. We will switch back when we are done.
        projectlibpath = os.path.dirname(__file__).removesuffix("test")
        with os_helper.change_cwd(projectlibpath):
            # Testing with and without ./
            assert_python_ok('-m', 'unittest', "test/test_longexp.py")
            assert_python_ok('-m', 'unittest', "./test/test_longexp.py")

    def test_run_code(self):
        # Test expected operation of the '-c' switch
        # Switch needs an argument
        assert_python_failure('-c')
        # Check we get an error for an uncaught exception
        assert_python_failure('-c', 'raise Exception')
        # All good if execution is successful
        assert_python_ok('-c', 'pass')

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_non_ascii(self):
        # Test handling of non-ascii data
        command = ("assert(ord(%r) == %s)"
                   % (os_helper.FS_NONASCII, ord(os_helper.FS_NONASCII)))
        assert_python_ok('-c', command)

    @unittest.skipUnless(os_helper.FS_NONASCII, 'need os_helper.FS_NONASCII')
    def test_coding(self):
        # bpo-32381: the -c command ignores the coding cookie
        ch = os_helper.FS_NONASCII
        cmd = f"# coding: latin1\nprint(ascii('{ch}'))"
        res = assert_python_ok('-c', cmd)
        self.assertEqual(res.out.rstrip(), ascii(ch).encode('ascii'))

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
        env['PYTHONCOERCECLOCALE'] = '0'
        code = (
            b'import locale; '
            b'print(ascii("' + undecodable + b'"), '
                b'locale.getencoding())')
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

    @unittest.skipIf(sys.platform == 'win32',
                     'Windows has a native unicode API')
    def test_invalid_utf8_arg(self):
        # bpo-35883: Py_DecodeLocale() must escape b'\xfd\xbf\xbf\xbb\xba\xba'
        # byte sequence with surrogateescape rather than decoding it as the
        # U+7fffbeba character which is outside the [U+0000; U+10ffff] range of
        # Python Unicode characters.
        #
        # Test with default config, in the C locale, in the Python UTF-8 Mode.
        code = 'import sys, os; s=os.fsencode(sys.argv[1]); print(ascii(s))'

        def run_default(arg):
            cmd = [sys.executable, '-c', code, arg]
            return subprocess.run(cmd, stdout=subprocess.PIPE, text=True)

        def run_c_locale(arg):
            cmd = [sys.executable, '-c', code, arg]
            env = dict(os.environ)
            env['LC_ALL'] = 'C'
            return subprocess.run(cmd, stdout=subprocess.PIPE,
                                  text=True, env=env)

        def run_utf8_mode(arg):
            cmd = [sys.executable, '-X', 'utf8', '-c', code, arg]
            return subprocess.run(cmd, stdout=subprocess.PIPE, text=True)

        valid_utf8 = 'e:\xe9, euro:\u20ac, non-bmp:\U0010ffff'.encode('utf-8')
        # invalid UTF-8 byte sequences with a valid UTF-8 sequence
        # in the middle.
        invalid_utf8 = (
            b'\xff'                      # invalid byte
            b'\xc3\xff'                  # invalid byte sequence
            b'\xc3\xa9'                  # valid utf-8: U+00E9 character
            b'\xed\xa0\x80'              # lone surrogate character (invalid)
            b'\xfd\xbf\xbf\xbb\xba\xba'  # character outside [U+0000; U+10ffff]
        )
        test_args = [valid_utf8, invalid_utf8]

        for run_cmd in (run_default, run_c_locale, run_utf8_mode):
            with self.subTest(run_cmd=run_cmd):
                for arg in test_args:
                    proc = run_cmd(arg)
                    self.assertEqual(proc.stdout.rstrip(), ascii(arg))

    @unittest.skipUnless((sys.platform == 'darwin' or
                support.is_android), 'test specific to Mac OS X and Android')
    def test_osx_android_utf8(self):
        text = 'e:\xe9, euro:\u20ac, non-bmp:\U0010ffff'.encode('utf-8')
        code = "import sys; print(ascii(sys.argv[1]))"

        decoded = text.decode('utf-8', 'surrogateescape')
        expected = ascii(decoded).encode('ascii') + b'\n'

        env = os.environ.copy()
        # C locale gives ASCII locale encoding, but Python uses UTF-8
        # to parse the command line arguments on Mac OS X and Android.
        env['LC_ALL'] = 'C'

        p = subprocess.Popen(
            (sys.executable, "-c", code, text),
            stdout=subprocess.PIPE,
            env=env)
        stdout, stderr = p.communicate()
        self.assertEqual(stdout, expected)
        self.assertEqual(p.returncode, 0)

    @unittest.skipIf(os.environ.get("PYTHONUNBUFFERED", "0") != "0",
                     "Python stdio buffering is disabled.")
    def test_non_interactive_output_buffering(self):
        code = textwrap.dedent("""
            import sys
            out = sys.stdout
            print(out.isatty(), out.write_through, out.line_buffering)
            err = sys.stderr
            print(err.isatty(), err.write_through, err.line_buffering)
        """)
        args = [sys.executable, '-c', code]
        proc = subprocess.run(args, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, text=True, check=True)
        self.assertEqual(proc.stdout,
                         'False False False\n'
                         'False False True\n')

    def test_unbuffered_output(self):
        # Test expected operation of the '-u' switch
        for stream in ('stdout', 'stderr'):
            # Binary is unbuffered
            code = ("import os, sys; sys.%s.buffer.write(b'x'); os._exit(0)"
                % stream)
            rc, out, err = assert_python_ok('-u', '-c', code)
            data = err if stream == 'stderr' else out
            self.assertEqual(data, b'x', "binary %s not unbuffered" % stream)
            # Text is unbuffered
            code = ("import os, sys; sys.%s.write('x'); os._exit(0)"
                % stream)
            rc, out, err = assert_python_ok('-u', '-c', code)
            data = err if stream == 'stderr' else out
            self.assertEqual(data, b'x', "text %s not unbuffered" % stream)

    def test_unbuffered_input(self):
        # sys.stdin still works with '-u'
        code = ("import sys; sys.stdout.write(sys.stdin.read(1))")
        p = spawn_python('-u', '-c', code)
        p.stdin.write(b'x')
        p.stdin.flush()
        data, rc = _kill_python_and_exit_code(p)
        self.assertEqual(rc, 0)
        self.assertStartsWith(data, b'x')

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

    @unittest.skipIf(sys.flags.safe_path,
                     'PYTHONSAFEPATH changes default sys.path')
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
        rc2, out2, err2 = assert_python_ok('-c', code, __isolated=False)
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
            self.assertEqual(b'3\r\n4\r\n', err)
        else:
            self.assertEqual(b'1\n2\n', out)
            self.assertEqual(b'3\n4\n', err)

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
            import os, sys, test.support
            test.support.SuppressCrashReport().__enter__()
            sys.stdout.write('x')
            os.close(sys.stdout.fileno())"""
        rc, out, err = assert_python_failure('-c', code)
        self.assertEqual(b'', out)
        self.assertEqual(120, rc)
        self.assertIn(b'Exception ignored while flushing sys.stdout:\n'
                      b'OSError: '.replace(b'\n', os.linesep.encode()),
                      err)

    def test_closed_stdout(self):
        # Issue #13444: if stdout has been explicitly closed, we should
        # not attempt to flush it at shutdown.
        code = "import sys; sys.stdout.close()"
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(b'', err)

    # Issue #7111: Python should work without standard streams

    @unittest.skipIf(os.name != 'posix', "test needs POSIX semantics")
    @unittest.skipIf(sys.platform == "vxworks",
                         "test needs preexec support in subprocess.Popen")
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
        self.assertEqual(err, b'')
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
        if os.environ.get('PYTHONHASHSEED', 'random') != 'random':
            env = dict(os.environ)  # copy
            # We need to test that it is enabled by default without
            # the environment variable enabling it for us.
            del env['PYTHONHASHSEED']
            env['__cleanenv'] = '1'  # consumed by assert_python_ok()
        else:
            env = {}
        for i in range(3):
            code = 'print(hash("spam"))'
            rc, out, err = assert_python_ok('-c', code, **env)
            self.assertEqual(rc, 0)
            hashes.append(out)
        hashes = sorted(set(hashes))  # uniq
        # Rare chance of failure due to 3 random seeds honestly being equal.
        self.assertGreater(len(hashes), 1,
                           msg='3 runs produced an identical random hash '
                               ' for "spam": {}'.format(hashes))

        # Verify that sys.flags contains hash_randomization
        code = 'import sys; print("random is", sys.flags.hash_randomization)'
        rc, out, err = assert_python_ok('-c', code, PYTHONHASHSEED='')
        self.assertIn(b'random is 1', out)

        rc, out, err = assert_python_ok('-c', code, PYTHONHASHSEED='random')
        self.assertIn(b'random is 1', out)

        rc, out, err = assert_python_ok('-c', code, PYTHONHASHSEED='0')
        self.assertIn(b'random is 0', out)

        rc, out, err = assert_python_ok('-R', '-c', code, PYTHONHASHSEED='0')
        self.assertIn(b'random is 1', out)

    def test_del___main__(self):
        # Issue #15001: PyRun_SimpleFileExFlags() did crash because it kept a
        # borrowed reference to the dict of __main__ module and later modify
        # the dict whereas the module was destroyed
        filename = os_helper.TESTFN
        self.addCleanup(os_helper.unlink, filename)
        with open(filename, "w", encoding="utf-8") as script:
            print("import sys", file=script)
            print("del sys.modules['__main__']", file=script)
        assert_python_ok(filename)

    @support.cpython_only
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

    @unittest.skipIf(interpreter_requires_environment(),
                     'Cannot run -I tests when PYTHON env vars are required.')
    def test_isolatedmode(self):
        self.verify_valid_flag('-I')
        self.verify_valid_flag('-IEPs')
        rc, out, err = assert_python_ok('-I', '-c',
            'from sys import flags as f; '
            'print(f.no_user_site, f.ignore_environment, f.isolated, f.safe_path)',
            # dummyvar to prevent extraneous -E
            dummyvar="")
        self.assertEqual(out.strip(), b'1 1 1 True')
        with os_helper.temp_cwd() as tmpdir:
            fake = os.path.join(tmpdir, "uuid.py")
            main = os.path.join(tmpdir, "main.py")
            with open(fake, "w", encoding="utf-8") as f:
                f.write("raise RuntimeError('isolated mode test')\n")
            with open(main, "w", encoding="utf-8") as f:
                f.write("import uuid\n")
                f.write("print('ok')\n")
            # Use -E to ignore PYTHONSAFEPATH env var
            self.assertRaises(subprocess.CalledProcessError,
                              subprocess.check_output,
                              [sys.executable, '-E', main], cwd=tmpdir,
                              stderr=subprocess.DEVNULL)
            out = subprocess.check_output([sys.executable, "-I", main],
                                          cwd=tmpdir)
            self.assertEqual(out.strip(), b"ok")

    def test_sys_flags_set(self):
        # Issue 31845: a startup refactoring broke reading flags from env vars
        for value, expected in (("", 0), ("1", 1), ("text", 1), ("2", 2)):
            env_vars = dict(
                PYTHONDEBUG=value,
                PYTHONOPTIMIZE=value,
                PYTHONDONTWRITEBYTECODE=value,
                PYTHONVERBOSE=value,
            )
            expected_bool = int(bool(value))
            code = (
                "import sys; "
                "sys.stderr.write(str(sys.flags)); "
                f"""sys.exit(not (
                    sys.flags.optimize == sys.flags.verbose == {expected}
                    and sys.flags.debug == sys.flags.dont_write_bytecode == {expected_bool}
                ))"""
            )
            with self.subTest(envar_value=value):
                assert_python_ok('-c', code, **env_vars)

    def test_set_pycache_prefix(self):
        # sys.pycache_prefix can be set from either -X pycache_prefix or
        # PYTHONPYCACHEPREFIX env var, with the former taking precedence.
        NO_VALUE = object()  # `-X pycache_prefix` with no `=PATH`
        cases = [
            # (PYTHONPYCACHEPREFIX, -X pycache_prefix, sys.pycache_prefix)
            (None, None, None),
            ('foo', None, 'foo'),
            (None, 'bar', 'bar'),
            ('foo', 'bar', 'bar'),
            ('foo', '', None),
            ('foo', NO_VALUE, None),
        ]
        for envval, opt, expected in cases:
            exp_clause = "is None" if expected is None else f'== "{expected}"'
            code = f"import sys; sys.exit(not sys.pycache_prefix {exp_clause})"
            args = ['-c', code]
            env = {} if envval is None else {'PYTHONPYCACHEPREFIX': envval}
            if opt is NO_VALUE:
                args[:0] = ['-X', 'pycache_prefix']
            elif opt is not None:
                args[:0] = ['-X', f'pycache_prefix={opt}']
            with self.subTest(envval=envval, opt=opt):
                with os_helper.temp_cwd():
                    assert_python_ok(*args, **env)

    def run_xdev(self, *args, check_exitcode=True, xdev=True):
        env = dict(os.environ)
        env.pop('PYTHONWARNINGS', None)
        env.pop('PYTHONDEVMODE', None)
        env.pop('PYTHONMALLOC', None)

        if xdev:
            args = (sys.executable, '-X', 'dev', *args)
        else:
            args = (sys.executable, *args)
        proc = subprocess.run(args,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              universal_newlines=True,
                              env=env)
        if check_exitcode:
            self.assertEqual(proc.returncode, 0, proc)
        return proc.stdout.rstrip()

    @support.cpython_only
    def test_xdev(self):
        # sys.flags.dev_mode
        code = "import sys; print(sys.flags.dev_mode)"
        out = self.run_xdev("-c", code, xdev=False)
        self.assertEqual(out, "False")
        out = self.run_xdev("-c", code)
        self.assertEqual(out, "True")

        # Warnings
        code = ("import warnings; "
                "print(' '.join('%s::%s' % (f[0], f[2].__name__) "
                                "for f in warnings.filters))")
        if support.Py_DEBUG:
            expected_filters = "default::Warning"
        else:
            expected_filters = ("default::Warning "
                                "default::DeprecationWarning "
                                "ignore::DeprecationWarning "
                                "ignore::PendingDeprecationWarning "
                                "ignore::ImportWarning "
                                "ignore::ResourceWarning")

        out = self.run_xdev("-c", code)
        self.assertEqual(out, expected_filters)

        out = self.run_xdev("-b", "-c", code)
        self.assertEqual(out, f"default::BytesWarning {expected_filters}")

        out = self.run_xdev("-bb", "-c", code)
        self.assertEqual(out, f"error::BytesWarning {expected_filters}")

        out = self.run_xdev("-Werror", "-c", code)
        self.assertEqual(out, f"error::Warning {expected_filters}")

        # Memory allocator debug hooks
        try:
            import _testinternalcapi  # noqa: F401
        except ImportError:
            pass
        else:
            code = "import _testinternalcapi; print(_testinternalcapi.pymem_getallocatorsname())"
            with support.SuppressCrashReport():
                out = self.run_xdev("-c", code, check_exitcode=False)
            if support.with_pymalloc():
                alloc_name = "pymalloc_debug"
            elif support.Py_GIL_DISABLED:
                alloc_name = "mimalloc_debug"
            else:
                alloc_name = "malloc_debug"
            self.assertEqual(out, alloc_name)

        # Faulthandler
        try:
            import faulthandler  # noqa: F401
        except ImportError:
            pass
        else:
            code = "import faulthandler; print(faulthandler.is_enabled())"
            out = self.run_xdev("-c", code)
            self.assertEqual(out, "True")

    def check_warnings_filters(self, cmdline_option, envvar, use_pywarning=False):
        if use_pywarning:
            code = ("import sys; from test.support.import_helper import "
                    "import_fresh_module; "
                    "warnings = import_fresh_module('warnings', blocked=['_warnings']); ")
        else:
            code = "import sys, warnings; "
        code += ("print(' '.join('%s::%s' % (f[0], f[2].__name__) "
                                "for f in warnings.filters))")
        args = (sys.executable, '-W', cmdline_option, '-bb', '-c', code)
        env = dict(os.environ)
        env.pop('PYTHONDEVMODE', None)
        env["PYTHONWARNINGS"] = envvar
        proc = subprocess.run(args,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              universal_newlines=True,
                              env=env)
        self.assertEqual(proc.returncode, 0, proc)
        return proc.stdout.rstrip()

    def test_warnings_filter_precedence(self):
        expected_filters = ("error::BytesWarning "
                            "once::UserWarning "
                            "always::UserWarning")
        if not support.Py_DEBUG:
            expected_filters += (" "
                                 "default::DeprecationWarning "
                                 "ignore::DeprecationWarning "
                                 "ignore::PendingDeprecationWarning "
                                 "ignore::ImportWarning "
                                 "ignore::ResourceWarning")

        out = self.check_warnings_filters("once::UserWarning",
                                          "always::UserWarning")
        self.assertEqual(out, expected_filters)

        out = self.check_warnings_filters("once::UserWarning",
                                          "always::UserWarning",
                                          use_pywarning=True)
        self.assertEqual(out, expected_filters)

    def check_pythonmalloc(self, env_var, name):
        code = 'import _testinternalcapi; print(_testinternalcapi.pymem_getallocatorsname())'
        env = dict(os.environ)
        env.pop('PYTHONDEVMODE', None)
        if env_var is not None:
            env['PYTHONMALLOC'] = env_var
        else:
            env.pop('PYTHONMALLOC', None)
        args = (sys.executable, '-c', code)
        proc = subprocess.run(args,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT,
                              universal_newlines=True,
                              env=env)
        self.assertEqual(proc.stdout.rstrip(), name)
        self.assertEqual(proc.returncode, 0)

    @support.cpython_only
    def test_pythonmalloc(self):
        # Test the PYTHONMALLOC environment variable
        malloc = not support.Py_GIL_DISABLED
        pymalloc = support.with_pymalloc()
        mimalloc = support.with_mimalloc()
        if support.Py_GIL_DISABLED:
            default_name = 'mimalloc_debug' if support.Py_DEBUG else 'mimalloc'
            default_name_debug = 'mimalloc_debug'
        elif pymalloc:
            default_name = 'pymalloc_debug' if support.Py_DEBUG else 'pymalloc'
            default_name_debug = 'pymalloc_debug'
        else:
            default_name = 'malloc_debug' if support.Py_DEBUG else 'malloc'
            default_name_debug = 'malloc_debug'

        tests = [
            (None, default_name),
            ('debug', default_name_debug),
        ]
        if malloc:
            tests.extend([
                ('malloc', 'malloc'),
                ('malloc_debug', 'malloc_debug'),
            ])
        if pymalloc:
            tests.extend((
                ('pymalloc', 'pymalloc'),
                ('pymalloc_debug', 'pymalloc_debug'),
            ))
        if mimalloc:
            tests.extend((
                ('mimalloc', 'mimalloc'),
                ('mimalloc_debug', 'mimalloc_debug'),
            ))

        for env_var, name in tests:
            with self.subTest(env_var=env_var, name=name):
                self.check_pythonmalloc(env_var, name)

    def test_pythondevmode_env(self):
        # Test the PYTHONDEVMODE environment variable
        code = "import sys; print(sys.flags.dev_mode)"
        env = dict(os.environ)
        env.pop('PYTHONDEVMODE', None)
        args = (sys.executable, '-c', code)

        proc = subprocess.run(args, stdout=subprocess.PIPE,
                              universal_newlines=True, env=env)
        self.assertEqual(proc.stdout.rstrip(), 'False')
        self.assertEqual(proc.returncode, 0, proc)

        env['PYTHONDEVMODE'] = '1'
        proc = subprocess.run(args, stdout=subprocess.PIPE,
                              universal_newlines=True, env=env)
        self.assertEqual(proc.stdout.rstrip(), 'True')
        self.assertEqual(proc.returncode, 0, proc)

    def test_python_gil(self):
        cases = [
            # (env, opt, expected, msg)
            ('1', None, '1', "PYTHON_GIL=1"),
            (None, '1', '1', "-X gil=1"),
        ]

        if support.Py_GIL_DISABLED:
            cases.extend(
                [
                    (None, None, 'None', "no options set"),
                    ('0', None, '0', "PYTHON_GIL=0"),
                    ('1', '0', '0', "-X gil=0 overrides PYTHON_GIL=1"),
                    (None, '0', '0', "-X gil=0"),
                ]
            )
        else:
            cases.extend(
                [
                    (None, None, '1', '-X gil=0 (unsupported by this build)'),
                    ('1', None, '1', 'PYTHON_GIL=0 (unsupported by this build)'),
                ]
            )
        code = "import sys; print(sys.flags.gil)"
        environ = dict(os.environ)

        for env, opt, expected, msg in cases:
            with self.subTest(msg, env=env, opt=opt):
                environ.pop('PYTHON_GIL', None)
                if env is not None:
                    environ['PYTHON_GIL'] = env
                extra_args = []
                if opt is not None:
                    extra_args = ['-X', f'gil={opt}']

                proc = subprocess.run([sys.executable, *extra_args, '-c', code],
                                      stdout=subprocess.PIPE,
                                      stderr=subprocess.PIPE,
                                      text=True, env=environ)
                self.assertEqual(proc.returncode, 0, proc)
                self.assertEqual(proc.stdout.rstrip(), expected)
                self.assertEqual(proc.stderr, '')

    def test_python_asyncio_debug(self):
        code = "import asyncio; print(asyncio.new_event_loop().get_debug())"
        rc, out, err = assert_python_ok('-c', code, PYTHONASYNCIODEBUG='1')
        self.assertIn(b'True', out)

    @unittest.skipUnless(sysconfig.get_config_var('Py_TRACE_REFS'), "Requires --with-trace-refs build option")
    def test_python_dump_refs(self):
        code = 'import sys; sys._clear_type_cache()'
        # TODO: Remove warnings context manager once sys._clear_type_cache is removed
        with warnings.catch_warnings():
            warnings.simplefilter("ignore", DeprecationWarning)
            rc, out, err = assert_python_ok('-c', code, PYTHONDUMPREFS='1')
        self.assertEqual(rc, 0)

    @unittest.skipUnless(sysconfig.get_config_var('Py_TRACE_REFS'), "Requires --with-trace-refs build option")
    def test_python_dump_refs_file(self):
        with tempfile.NamedTemporaryFile() as dump_file:
            code = 'import sys; sys._clear_type_cache()'
            # TODO: Remove warnings context manager once sys._clear_type_cache is removed
            with warnings.catch_warnings():
                warnings.simplefilter("ignore", DeprecationWarning)
                rc, out, err = assert_python_ok('-c', code, PYTHONDUMPREFSFILE=dump_file.name)
            self.assertEqual(rc, 0)
            with open(dump_file.name, 'r') as file:
                contents = file.read()
                self.assertIn('Remaining objects', contents)

    @unittest.skipUnless(sys.platform == 'darwin', 'PYTHONEXECUTABLE only works on macOS')
    def test_python_executable(self):
        code = 'import sys; print(sys.executable)'
        expected = "/busr/bbin/bpython"
        rc, out, err = assert_python_ok('-c', code, PYTHONEXECUTABLE=expected)
        self.assertIn(expected.encode(), out)

    @unittest.skipUnless(support.MS_WINDOWS, 'Test only applicable on Windows')
    def test_python_legacy_windows_fs_encoding(self):
        code = "import sys; print(sys.getfilesystemencoding())"
        expected = 'mbcs'
        rc, out, err = assert_python_ok('-c', code, PYTHONLEGACYWINDOWSFSENCODING='1')
        self.assertIn(expected.encode(), out)

    @unittest.skipUnless(support.MS_WINDOWS, 'Test only applicable on Windows')
    def test_python_legacy_windows_stdio(self):
        # Test that _WindowsConsoleIO is used when PYTHONLEGACYWINDOWSSTDIO
        # is not set.
        # We cannot use PIPE becase it prevents creating new console.
        # So we use exit code.
        code = "import sys; sys.exit(type(sys.stdout.buffer.raw).__name__ != '_WindowsConsoleIO')"
        env = os.environ.copy()
        env["PYTHONLEGACYWINDOWSSTDIO"] = ""
        p = subprocess.run([sys.executable, "-c", code],
                           creationflags=subprocess.CREATE_NEW_CONSOLE,
                           env=env)
        self.assertEqual(p.returncode, 0)

        # Then test that FIleIO is used when PYTHONLEGACYWINDOWSSTDIO is set.
        code = "import sys; sys.exit(type(sys.stdout.buffer.raw).__name__ != 'FileIO')"
        env["PYTHONLEGACYWINDOWSSTDIO"] = "1"
        p = subprocess.run([sys.executable, "-c", code],
                           creationflags=subprocess.CREATE_NEW_CONSOLE,
                           env=env)
        self.assertEqual(p.returncode, 0)

    @unittest.skipIf("-fsanitize" in sysconfig.get_config_vars().get('PY_CFLAGS', ()),
                     "PYTHONMALLOCSTATS doesn't work with ASAN")
    def test_python_malloc_stats(self):
        code = "pass"
        rc, out, err = assert_python_ok('-c', code, PYTHONMALLOCSTATS='1')
        self.assertIn(b'Small block threshold', err)

    def test_python_user_base(self):
        code = "import site; print(site.USER_BASE)"
        expected = "/custom/userbase"
        rc, out, err = assert_python_ok('-c', code, PYTHONUSERBASE=expected)
        self.assertIn(expected.encode(), out)

    def test_python_basic_repl(self):
        # Currently this only tests that the env var is set. See test_pyrepl.test_python_basic_repl.
        code = "import os; print('PYTHON_BASIC_REPL' in os.environ)"
        expected = "True"
        rc, out, err = assert_python_ok('-c', code, PYTHON_BASIC_REPL='1')
        self.assertIn(expected.encode(), out)

    @unittest.skipUnless(sysconfig.get_config_var('HAVE_PERF_TRAMPOLINE'), "Requires HAVE_PERF_TRAMPOLINE support")
    def test_python_perf_jit_support(self):
        code = "import sys; print(sys.is_stack_trampoline_active())"
        expected = "True"
        rc, out, err = assert_python_ok('-c', code, PYTHON_PERF_JIT_SUPPORT='1')
        self.assertIn(expected.encode(), out)

    @unittest.skipUnless(sys.platform == 'win32',
                         'bpo-32457 only applies on Windows')
    def test_argv0_normalization(self):
        args = sys.executable, '-c', 'print(0)'
        prefix, exe = os.path.split(sys.executable)
        executable = prefix + '\\.\\.\\.\\' + exe

        proc = subprocess.run(args, stdout=subprocess.PIPE,
                              executable=executable)
        self.assertEqual(proc.returncode, 0, proc)
        self.assertEqual(proc.stdout.strip(), b'0')

    @support.cpython_only
    def test_parsing_error(self):
        args = [sys.executable, '-I', '--unknown-option']
        proc = subprocess.run(args,
                              stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE,
                              text=True)
        err_msg = "Unknown option: --unknown-option\nusage: "
        self.assertStartsWith(proc.stderr, err_msg)
        self.assertNotEqual(proc.returncode, 0)

    def test_int_max_str_digits(self):
        code = "import sys; print(sys.flags.int_max_str_digits, sys.get_int_max_str_digits())"

        assert_python_failure('-X', 'int_max_str_digits', '-c', code)
        assert_python_failure('-X', 'int_max_str_digits=foo', '-c', code)
        assert_python_failure('-X', 'int_max_str_digits=100', '-c', code)
        assert_python_failure('-X', 'int_max_str_digits', '-c', code,
                              PYTHONINTMAXSTRDIGITS='4000')

        assert_python_failure('-c', code, PYTHONINTMAXSTRDIGITS='foo')
        assert_python_failure('-c', code, PYTHONINTMAXSTRDIGITS='100')

        res = assert_python_ok('-c', code)
        res2int = self.res2int
        current_max = sys.get_int_max_str_digits()
        self.assertEqual(res2int(res), (current_max, current_max))
        res = assert_python_ok('-X', 'int_max_str_digits=0', '-c', code)
        self.assertEqual(res2int(res), (0, 0))
        res = assert_python_ok('-X', 'int_max_str_digits=4000', '-c', code)
        self.assertEqual(res2int(res), (4000, 4000))
        res = assert_python_ok('-X', 'int_max_str_digits=100000', '-c', code)
        self.assertEqual(res2int(res), (100000, 100000))

        res = assert_python_ok('-c', code, PYTHONINTMAXSTRDIGITS='0')
        self.assertEqual(res2int(res), (0, 0))
        res = assert_python_ok('-c', code, PYTHONINTMAXSTRDIGITS='4000')
        self.assertEqual(res2int(res), (4000, 4000))
        res = assert_python_ok(
            '-X', 'int_max_str_digits=6000', '-c', code,
            PYTHONINTMAXSTRDIGITS='4000'
        )
        self.assertEqual(res2int(res), (6000, 6000))

    def test_cmd_dedent(self):
        # test that -c auto-dedents its arguments
        test_cases = [
            (
                """
                    print('space-auto-dedent')
                """,
                "space-auto-dedent",
            ),
            (
                dedent(
                    """
                ^^^print('tab-auto-dedent')
                """
                ).replace("^", "\t"),
                "tab-auto-dedent",
            ),
            (
                dedent(
                    """
                ^^if 1:
                ^^^^print('mixed-auto-dedent-1')
                ^^print('mixed-auto-dedent-2')
                """
                ).replace("^", "\t \t"),
                "mixed-auto-dedent-1\nmixed-auto-dedent-2",
            ),
            (
                '''
                    data = """$

                    this data has an empty newline above and a newline with spaces below $
                                            $
                    """$
                    if 1:         $
                        print(repr(data))$
                '''.replace(
                    "$", ""
                ),
                # Note: entirely blank lines are normalized to \n, even if they
                # are part of a data string. This is consistent with
                # textwrap.dedent behavior, but might not be intuitive.
                "'\\n\\nthis data has an empty newline above and a newline with spaces below \\n\\n'",
            ),
            (
                '',
                '',
            ),
            (
                '  \t\n\t\n \t\t\t  \t\t \t\n\t\t \n\n\n\t\t\t   ',
                '',
            ),
        ]
        for code, expected in test_cases:
            # Run the auto-dedent case
            args1 = sys.executable, '-c', code
            proc1 = subprocess.run(args1, stdout=subprocess.PIPE)
            self.assertEqual(proc1.returncode, 0, proc1)
            output1 = proc1.stdout.strip().decode(encoding='utf-8')

            # Manually dedent beforehand, check the result is the same.
            args2 = sys.executable, '-c', dedent(code)
            proc2 = subprocess.run(args2, stdout=subprocess.PIPE)
            self.assertEqual(proc2.returncode, 0, proc2)
            output2 = proc2.stdout.strip().decode(encoding='utf-8')

            self.assertEqual(output1, output2)
            self.assertEqual(output1.replace('\r\n', '\n'), expected)

    def test_cmd_dedent_failcase(self):
        # Mixing tabs and spaces is not allowed
        from textwrap import dedent
        template = dedent(
            '''
            -+if 1:
            +-++ print('will fail')
            ''')
        code = template.replace('-', ' ').replace('+', '\t')
        assert_python_failure('-c', code)
        code = template.replace('-', '\t').replace('+', ' ')
        assert_python_failure('-c', code)

    def test_cpu_count(self):
        code = "import os; print(os.cpu_count(), os.process_cpu_count())"
        res = assert_python_ok('-X', 'cpu_count=4321', '-c', code)
        self.assertEqual(self.res2int(res), (4321, 4321))
        res = assert_python_ok('-c', code, PYTHON_CPU_COUNT='1234')
        self.assertEqual(self.res2int(res), (1234, 1234))

    def test_cpu_count_default(self):
        code = "import os; print(os.cpu_count(), os.process_cpu_count())"
        res = assert_python_ok('-X', 'cpu_count=default', '-c', code)
        self.assertEqual(self.res2int(res), (os.cpu_count(), os.process_cpu_count()))
        res = assert_python_ok('-X', 'cpu_count=default', '-c', code, PYTHON_CPU_COUNT='1234')
        self.assertEqual(self.res2int(res), (os.cpu_count(), os.process_cpu_count()))
        res = assert_python_ok('-c', code, PYTHON_CPU_COUNT='default')
        self.assertEqual(self.res2int(res), (os.cpu_count(), os.process_cpu_count()))

    def test_import_time(self):
        # os is not imported at startup
        code = 'import os; import os'

        for case in 'importtime', 'importtime=1', 'importtime=true':
            res = assert_python_ok('-X', case, '-c', code)
            res_err = res.err.decode('utf-8')
            self.assertRegex(res_err, r'import time: \s*\d+ \| \s*\d+ \| \s*os')
            self.assertNotRegex(res_err, r'import time: cached\s* \| cached\s* \| os')

        res = assert_python_ok('-X', 'importtime=2', '-c', code)
        res_err = res.err.decode('utf-8')
        self.assertRegex(res_err, r'import time: \s*\d+ \| \s*\d+ \| \s*os')
        self.assertRegex(res_err, r'import time: cached\s* \| cached\s* \| os')

        assert_python_failure('-X', 'importtime=-1', '-c', code)
        assert_python_failure('-X', 'importtime=3', '-c', code)

    def res2int(self, res):
        out = res.out.strip().decode("utf-8")
        return tuple(int(i) for i in out.split())

    @unittest.skipUnless(support.Py_GIL_DISABLED,
                         "PYTHON_TLBC and -X tlbc"
                         " only supported in Py_GIL_DISABLED builds")
    @threading_helper.requires_working_threading()
    def test_disable_thread_local_bytecode(self):
        code = """if 1:
            import threading
            def test(x, y):
                return x + y
            t = threading.Thread(target=test, args=(1,2))
            t.start()
            t.join()"""
        assert_python_ok("-W", "always", "-X", "tlbc=0", "-c", code)
        assert_python_ok("-W", "always", "-c", code, PYTHON_TLBC="0")

    @unittest.skipUnless(support.Py_GIL_DISABLED,
                         "PYTHON_TLBC and -X tlbc"
                         " only supported in Py_GIL_DISABLED builds")
    @threading_helper.requires_working_threading()
    def test_enable_thread_local_bytecode(self):
        code = """if 1:
            import threading
            def test(x, y):
                return x + y
            t = threading.Thread(target=test, args=(1,2))
            t.start()
            t.join()"""
        # The functionality of thread-local bytecode is tested more extensively
        # in test_thread_local_bytecode
        assert_python_ok("-W", "always", "-X", "tlbc=1", "-c", code)
        assert_python_ok("-W", "always", "-c", code, PYTHON_TLBC="1")

    @unittest.skipUnless(support.Py_GIL_DISABLED,
                         "PYTHON_TLBC and -X tlbc"
                         " only supported in Py_GIL_DISABLED builds")
    def test_invalid_thread_local_bytecode(self):
        rc, out, err = assert_python_failure("-X", "tlbc")
        self.assertIn(b"tlbc=n: n is missing or invalid", err)
        rc, out, err = assert_python_failure("-X", "tlbc=foo")
        self.assertIn(b"tlbc=n: n is missing or invalid", err)
        rc, out, err = assert_python_failure("-X", "tlbc=-1")
        self.assertIn(b"tlbc=n: n is missing or invalid", err)
        rc, out, err = assert_python_failure("-X", "tlbc=2")
        self.assertIn(b"tlbc=n: n is missing or invalid", err)
        rc, out, err = assert_python_failure(PYTHON_TLBC="foo")
        self.assertIn(b"PYTHON_TLBC=N: N is missing or invalid", err)
        rc, out, err = assert_python_failure(PYTHON_TLBC="-1")
        self.assertIn(b"PYTHON_TLBC=N: N is missing or invalid", err)
        rc, out, err = assert_python_failure(PYTHON_TLBC="2")
        self.assertIn(b"PYTHON_TLBC=N: N is missing or invalid", err)


@unittest.skipIf(interpreter_requires_environment(),
                 'Cannot run -I tests when PYTHON env vars are required.')
class IgnoreEnvironmentTest(unittest.TestCase):

    def run_ignoring_vars(self, predicate, **env_vars):
        # Runs a subprocess with -E set, even though we're passing
        # specific environment variables
        # Logical inversion to match predicate check to a zero return
        # code indicating success
        code = "import sys; sys.stderr.write(str(sys.flags)); sys.exit(not ({}))".format(predicate)
        return assert_python_ok('-E', '-c', code, **env_vars)

    def test_ignore_PYTHONPATH(self):
        path = "should_be_ignored"
        self.run_ignoring_vars("'{}' not in sys.path".format(path),
                               PYTHONPATH=path)

    def test_ignore_PYTHONHASHSEED(self):
        self.run_ignoring_vars("sys.flags.hash_randomization == 1",
                               PYTHONHASHSEED="0")

    def test_sys_flags_not_set(self):
        # Issue 31845: a startup refactoring broke reading flags from env vars
        expected_outcome = """
            (sys.flags.debug == sys.flags.optimize ==
             sys.flags.dont_write_bytecode ==
             sys.flags.verbose == sys.flags.safe_path == 0)
        """
        self.run_ignoring_vars(
            expected_outcome,
            PYTHONDEBUG="1",
            PYTHONOPTIMIZE="1",
            PYTHONDONTWRITEBYTECODE="1",
            PYTHONVERBOSE="1",
            PYTHONSAFEPATH="1",
        )


class SyntaxErrorTests(unittest.TestCase):
    @force_not_colorized
    def check_string(self, code):
        proc = subprocess.run([sys.executable, "-"], input=code,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        self.assertNotEqual(proc.returncode, 0)
        self.assertNotEqual(proc.stderr, None)
        self.assertIn(b"\nSyntaxError", proc.stderr)

    def test_tokenizer_error_with_stdin(self):
        self.check_string(b"(1+2+3")

    def test_decoding_error_at_the_end_of_the_line(self):
        self.check_string(br"'\u1f'")


def tearDownModule():
    support.reap_children()


if __name__ == "__main__":
    unittest.main()
