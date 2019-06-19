"""
Test the implementation of the PEP 540: the UTF-8 Mode.
"""

import locale
import os
import sys
import textwrap
import unittest
from test import support
from test.support.script_helper import assert_python_ok, assert_python_failure


MS_WINDOWS = (sys.platform == 'win32')
POSIX_LOCALES = ('C', 'POSIX')


class UTF8ModeTests(unittest.TestCase):
    DEFAULT_ENV = {
        'PYTHONUTF8': '',
        'PYTHONLEGACYWINDOWSFSENCODING': '',
        'PYTHONCOERCECLOCALE': '0',
    }

    def posix_locale(self):
        loc = locale.setlocale(locale.LC_CTYPE, None)
        return (loc in POSIX_LOCALES)

    def get_output(self, *args, failure=False, **kw):
        kw = dict(self.DEFAULT_ENV, **kw)
        if failure:
            out = assert_python_failure(*args, **kw)
            out = out[2]
        else:
            out = assert_python_ok(*args, **kw)
            out = out[1]
        return out.decode().rstrip("\n\r")

    @unittest.skipIf(MS_WINDOWS, 'Windows has no POSIX locale')
    def test_posix_locale(self):
        code = 'import sys; print(sys.flags.utf8_mode)'

        for loc in POSIX_LOCALES:
            with self.subTest(LC_ALL=loc):
                out = self.get_output('-c', code, LC_ALL=loc)
                self.assertEqual(out, '1')

    def test_xoption(self):
        code = 'import sys; print(sys.flags.utf8_mode)'

        out = self.get_output('-X', 'utf8', '-c', code)
        self.assertEqual(out, '1')

        # undocumented but accepted syntax: -X utf8=1
        out = self.get_output('-X', 'utf8=1', '-c', code)
        self.assertEqual(out, '1')

        out = self.get_output('-X', 'utf8=0', '-c', code)
        self.assertEqual(out, '0')

        if MS_WINDOWS:
            # PYTHONLEGACYWINDOWSFSENCODING disables the UTF-8 Mode
            # and has the priority over -X utf8
            out = self.get_output('-X', 'utf8', '-c', code,
                                  PYTHONLEGACYWINDOWSFSENCODING='1')
            self.assertEqual(out, '0')

    def test_env_var(self):
        code = 'import sys; print(sys.flags.utf8_mode)'

        out = self.get_output('-c', code, PYTHONUTF8='1')
        self.assertEqual(out, '1')

        out = self.get_output('-c', code, PYTHONUTF8='0')
        self.assertEqual(out, '0')

        # -X utf8 has the priority over PYTHONUTF8
        out = self.get_output('-X', 'utf8=0', '-c', code, PYTHONUTF8='1')
        self.assertEqual(out, '0')

        if MS_WINDOWS:
            # PYTHONLEGACYWINDOWSFSENCODING disables the UTF-8 mode
            # and has the priority over PYTHONUTF8
            out = self.get_output('-X', 'utf8', '-c', code, PYTHONUTF8='1',
                                  PYTHONLEGACYWINDOWSFSENCODING='1')
            self.assertEqual(out, '0')

        # Cannot test with the POSIX locale, since the POSIX locale enables
        # the UTF-8 mode
        if not self.posix_locale():
            # PYTHONUTF8 should be ignored if -E is used
            out = self.get_output('-E', '-c', code, PYTHONUTF8='1')
            self.assertEqual(out, '0')

        # invalid mode
        out = self.get_output('-c', code, PYTHONUTF8='xxx', failure=True)
        self.assertIn('invalid PYTHONUTF8 environment variable value',
                      out.rstrip())

    def test_filesystemencoding(self):
        code = textwrap.dedent('''
            import sys
            print("{}/{}".format(sys.getfilesystemencoding(),
                                 sys.getfilesystemencodeerrors()))
        ''')

        if MS_WINDOWS:
            expected = 'utf-8/surrogatepass'
        else:
            expected = 'utf-8/surrogateescape'

        out = self.get_output('-X', 'utf8', '-c', code)
        self.assertEqual(out, expected)

        if MS_WINDOWS:
            # PYTHONLEGACYWINDOWSFSENCODING disables the UTF-8 mode
            # and has the priority over -X utf8 and PYTHONUTF8
            out = self.get_output('-X', 'utf8', '-c', code,
                                  PYTHONUTF8='strict',
                                  PYTHONLEGACYWINDOWSFSENCODING='1')
            self.assertEqual(out, 'mbcs/replace')

    def test_stdio(self):
        code = textwrap.dedent('''
            import sys
            print(f"stdin: {sys.stdin.encoding}/{sys.stdin.errors}")
            print(f"stdout: {sys.stdout.encoding}/{sys.stdout.errors}")
            print(f"stderr: {sys.stderr.encoding}/{sys.stderr.errors}")
        ''')

        out = self.get_output('-X', 'utf8', '-c', code,
                              PYTHONIOENCODING='')
        self.assertEqual(out.splitlines(),
                         ['stdin: utf-8/surrogateescape',
                          'stdout: utf-8/surrogateescape',
                          'stderr: utf-8/backslashreplace'])

        # PYTHONIOENCODING has the priority over PYTHONUTF8
        out = self.get_output('-X', 'utf8', '-c', code,
                              PYTHONIOENCODING="latin1")
        self.assertEqual(out.splitlines(),
                         ['stdin: latin1/strict',
                          'stdout: latin1/strict',
                          'stderr: latin1/backslashreplace'])

        out = self.get_output('-X', 'utf8', '-c', code,
                              PYTHONIOENCODING=":namereplace")
        self.assertEqual(out.splitlines(),
                         ['stdin: utf-8/namereplace',
                          'stdout: utf-8/namereplace',
                          'stderr: utf-8/backslashreplace'])

    def test_io(self):
        code = textwrap.dedent('''
            import sys
            filename = sys.argv[1]
            with open(filename) as fp:
                print(f"{fp.encoding}/{fp.errors}")
        ''')
        filename = __file__

        out = self.get_output('-c', code, filename, PYTHONUTF8='1')
        self.assertEqual(out, 'UTF-8/strict')

    def _check_io_encoding(self, module, encoding=None, errors=None):
        filename = __file__

        # Encoding explicitly set
        args = []
        if encoding:
            args.append(f'encoding={encoding!r}')
        if errors:
            args.append(f'errors={errors!r}')
        code = textwrap.dedent('''
            import sys
            from %s import open
            filename = sys.argv[1]
            with open(filename, %s) as fp:
                print(f"{fp.encoding}/{fp.errors}")
        ''') % (module, ', '.join(args))
        out = self.get_output('-c', code, filename,
                              PYTHONUTF8='1')

        if not encoding:
            encoding = 'UTF-8'
        if not errors:
            errors = 'strict'
        self.assertEqual(out, f'{encoding}/{errors}')

    def check_io_encoding(self, module):
        self._check_io_encoding(module, encoding="latin1")
        self._check_io_encoding(module, errors="namereplace")
        self._check_io_encoding(module,
                                encoding="latin1", errors="namereplace")

    def test_io_encoding(self):
        self.check_io_encoding('io')

    def test_io_encoding(self):
        self.check_io_encoding('_pyio')

    def test_locale_getpreferredencoding(self):
        code = 'import locale; print(locale.getpreferredencoding(False), locale.getpreferredencoding(True))'
        out = self.get_output('-X', 'utf8', '-c', code)
        self.assertEqual(out, 'UTF-8 UTF-8')

        for loc in POSIX_LOCALES:
            with self.subTest(LC_ALL=loc):
                out = self.get_output('-X', 'utf8', '-c', code, LC_ALL=loc)
                self.assertEqual(out, 'UTF-8 UTF-8')

    @unittest.skipIf(MS_WINDOWS, 'test specific to Unix')
    def test_cmd_line(self):
        arg = 'h\xe9\u20ac'.encode('utf-8')
        arg_utf8 = arg.decode('utf-8')
        arg_ascii = arg.decode('ascii', 'surrogateescape')
        code = 'import locale, sys; print("%s:%s" % (locale.getpreferredencoding(), ascii(sys.argv[1:])))'

        def check(utf8_opt, expected, **kw):
            out = self.get_output('-X', utf8_opt, '-c', code, arg, **kw)
            args = out.partition(':')[2].rstrip()
            self.assertEqual(args, ascii(expected), out)

        check('utf8', [arg_utf8])
        for loc in POSIX_LOCALES:
            with self.subTest(LC_ALL=loc):
                check('utf8', [arg_utf8], LC_ALL=loc)

        if sys.platform == 'darwin' or support.is_android:
            c_arg = arg_utf8
        elif sys.platform.startswith("aix"):
            c_arg = arg.decode('iso-8859-1')
        else:
            c_arg = arg_ascii
        for loc in POSIX_LOCALES:
            with self.subTest(LC_ALL=loc):
                check('utf8=0', [c_arg], LC_ALL=loc)

    def test_optim_level(self):
        # CPython: check that Py_Main() doesn't increment Py_OptimizeFlag
        # twice when -X utf8 requires to parse the configuration twice (when
        # the encoding changes after reading the configuration, the
        # configuration is read again with the new encoding).
        code = 'import sys; print(sys.flags.optimize)'
        out = self.get_output('-X', 'utf8', '-O', '-c', code)
        self.assertEqual(out, '1')
        out = self.get_output('-X', 'utf8', '-OO', '-c', code)
        self.assertEqual(out, '2')

        code = 'import sys; print(sys.flags.ignore_environment)'
        out = self.get_output('-X', 'utf8', '-E', '-c', code)
        self.assertEqual(out, '1')


if __name__ == "__main__":
    unittest.main()
