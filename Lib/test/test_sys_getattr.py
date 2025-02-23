from pathlib import Path
import tempfile
import test.support
from test.support.script_helper import assert_python_failure, assert_python_ok
import textwrap
import unittest




@test.support.cpython_only
class PySysGetAttrTest(unittest.TestCase):

    common_faulthandler_code = textwrap.dedent('''
        import sys
        from faulthandler import dump_traceback, enable, dump_traceback_later

        class FakeIO:
            def __init__(self, what):
                self.what = what
            def write(self, str):
                pass
            def flush(self):
                pass
            def fileno(self):
                self.restore_std('stderr')
                return 0

            @staticmethod
            def restore_std(what):
                stdfile = getattr(sys, what)
                setattr(sys, what, getattr(sys, "__" + what + "__"))
                del stdfile

            @staticmethod
            def set_std(what):
                setattr(sys, what, FakeIO(what))

        def main():
            enable(None, True)
            FakeIO.set_std('stderr')
            {0}

        if __name__ == "__main__":
            main()
    ''')

    common_warnings_code = textwrap.dedent('''
        from io import StringIO
        import sys
        import warnings

        class Foo:
            def __init__(self):
                self.x = sys.stderr
                setattr(sys, "stderr", StringIO())

            def __repr__(self):
                x = sys.stderr
                setattr(sys, "stderr", self.x)
                del x
                return "Foo"

        def main():
            del warnings._showwarnmsg
            {0}

        if __name__ == "__main__":
            main()
            exit(0)
    ''')

    common_input_code = textwrap.dedent('''
        import sys

        class FakeIO:
            def write(self, str):
                pass
            def flush(self):
                pass
            def fileno(self):
                return 0

        class CrashStdin:
            def __init__(self):
                self.stdin = sys.stdin
                setattr(sys, "stdin", FakeIO())

            def __repr__(self):
                stdin = sys.stdin
                setattr(sys, "stdin", self.stdin)
                del stdin
                return "CrashStdin"

        class CrashStdout:
            def __init__(self):
                self.stdout = sys.stdout
                setattr(sys, "stdout", FakeIO())

            def __repr__(self):
                stdout = sys.stdout
                setattr(sys, "stdout", self.stdout)
                del stdout
                return "CrashStdout"

        class CrashStderr:
            def __init__(self):
                self.stderr = sys.stderr
                setattr(sys, "stderr", FakeIO())

            def __repr__(self):
                stderr = sys.stderr
                setattr(sys, "stderr", self.stderr)
                del stderr
                return "CrashStderr"

        def audit(event, args):
            if event == 'builtins.input':
                repr(args)

        def main():
            {0}
            input({1})

        if __name__ == "__main__":
            main()

    ''')

    unraisable_hook_code = textwrap.dedent('''
        import sys

        class UnraisableHookInitiator:
            def __del__(self):
                raise Exception('1')

        class UnraisableHook:
            def __call__(self, *args, **kwds):
                print('X', *args)

            def __repr__(self):
                h = sys.unraisablehook
                setattr(sys, 'unraisablehook', sys.__unraisablehook__)
                del h
                return 'UnraisableHook'

        def audit(event, args):
            repr(args)

        def main():
            sys.addaudithook(audit)
            setattr(sys, 'unraisablehook', UnraisableHook())
            x = UnraisableHookInitiator()
            del x

        if __name__ == "__main__":
            main()

    ''')

    flush_std_files_common_code = textwrap.dedent('''
        import sys

        class FakeIO:
            def __init__(self, what):
                self.what = what
            def write(self, str):
                pass
            def flush(self):
                pass
            def fileno(self):
                return 0

            @property
            def closed(self):
                stdfile = getattr(sys, self.what)
                setattr(sys, self.what, getattr(sys, "__" + self.what + "__"))
                del stdfile
                return False

        def main():
            setattr(sys, '{0}', FakeIO('{0}'))

        if __name__ == "__main__":
            main()
    ''')

    pyerr_printex_code = textwrap.dedent('''
        import sys

        class Hook:
            def __call__(self, *args, **kwds):
                pass

            def __repr__(self):
                h = sys.excepthook
                setattr(sys, 'excepthook', sys.__excepthook__)
                del h
                return 'Hook'

        def audit(event, args):
            repr(args)

        def main():
            sys.addaudithook(audit)
            setattr(sys, 'excepthook', Hook())
            raise

        if __name__ == "__main__":
            main()
    ''')

    print_code = textwrap.dedent('''
        from io import StringIO
        import sys

        class Bar:
            def __init__(self):
                self.x = sys.stdout
                setattr(sys, "stdout", StringIO())

            def __repr__(self):
                x = sys.stdout
                setattr(sys, "stdout", self.x)
                del x
                return "Bar"

        def main():
            print(Bar())

        if __name__ == "__main__":
            main()
            exit(0)
    ''')

    def _check(self, code):
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(rc, 0)
        self.assertNotIn(b"Segmentation fault", err)
        self.assertNotIn(b"access violation", err)

    def test_print_deleted_stdout(self):
        # print should use strong reference to the stdout.
        self._check(self.print_code)

    def test_faulthandler_enable_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        code = self.common_faulthandler_code.format(
            "enable(None, True)"
        )
        self._check(code)

    def test_faulthandler_dump_traceback_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        code = self.common_faulthandler_code.format(
            "dump_traceback(None, False)"
        )
        self._check(code)

    def test_faulthandler_dump_traceback_later_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        code = self.common_faulthandler_code.format(
            "dump_traceback_later(0.1, True, None, False)"
        )
        self._check(code)

    def test_warnings_warn(self):
        code = self.common_warnings_code.format(
            "warnings.warn(Foo())"
        )
        self._check(code)

    def test_warnings_warn_explicit(self):
        code = self.common_warnings_code.format(
            "warnings.warn_explicit(Foo(), UserWarning, 'filename', 0)"
        )
        self._check(code)

    def test_input_stdin(self):
        code = self.common_input_code.format(
            "",
            "CrashStdin()"
        )
        self._check(code)

    def test_input_stdout(self):
        code = self.common_input_code.format(
            "",
            "CrashStdout()"
        )
        self._check(code)

    def test_input_stderr(self):
        code = self.common_input_code.format(
            "sys.addaudithook(audit)",
            "CrashStderr()"
        )
        self._check(code)

    def test_errors_unraisablehook(self):
        self._check(self.unraisable_hook_code)

    def test_py_finalize_flush_std_files_stdout(self):
        code = self.flush_std_files_common_code.format("stdout")
        self._check(code)

    def test_py_finalize_flush_std_files_stderr(self):
        code = self.flush_std_files_common_code.format("stderr")
        self._check(code)

    def test_pyerr_printex_excepthook(self):
        self._check(self.pyerr_printex_code)

if __name__ == "__main__":
    unittest.main()
