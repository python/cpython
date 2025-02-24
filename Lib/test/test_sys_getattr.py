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

        # The faulthandler_get_fileno calls stderr functions twice - the first
        # call is a fileno and the second one is a flush. So, if the stderr
        # changed while fileno is called, then we get a segfault when we
        # call flush after that.
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

        # First of all, we have to delete _showwarnmsg to get into show_warning
        # function. The show_warning do series of calls of PyFile_WriteObject
        # and PyFile_WriteString functions. And one of the call is for __repr__
        # of warning's message. So if we change stderr while calling __repr__
        # (or concurently) we can get segfault from one of the consequence call
        # to write functions.
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

        # The input function gets borrowed refs for stdin, stdout and stderr.
        # As we use FakeIO without fileno the input functions thinks that we
        # are not tty and following happens:
        # audit is called, stderr is flushed, prompt's __repr__ is printed to
        # stdout and line is read from stdin. For stdin and stdout we can just
        # replace stdin and stdout from prompt's __repr__ and get segfault. But
        # for stderr we should do this from audit function.
        class CrashWhat:
            def __init__(self, what):
                self.what = what
                self.std = getattr(sys, what)
                setattr(sys, what, FakeIO())

            def __repr__(self):
                std = getattr(sys, self.what)
                setattr(sys, self.what, self.std)
                del std
                return "Crash"

        def audit(event, args):
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

        # To get into unraisablehook we need to raise an exception from __del__
        # function. So, format_unraisable_v gets hook, calls audit
        # function and calls hook. If we revert back unraisablehook from audit
        # function we will get segfault when calling hook.
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

        # The flush_std_files function gets stdout and stderr. And then checks
        # if both of them are closed. And if so calls flush for them.
        # If we restore stdfile from FakeIO.closed property we can get segfault.
        class FakeIO:
            def __init__(self, what):
                self.what = what
            def write(self, str):
                pass
            def flush(self):
                pass

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

        # To get into excepthook we should run invalid statement.
        # Then _PyErr_PrintEx gets excepthook, calls audit function and tries
        # to call hook. If we replace hook from audit (or concurently) we get
        # segfault.
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

        # The print function gets stdout and does a series of calls write
        # functions. One of the function calls __repr__ and if we replace
        # stdout from __repr__ (or concurently) we get segfault.
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
            "CrashWhat('stdin')"
        )
        self._check(code)

    def test_input_stdout(self):
        code = self.common_input_code.format(
            "",
            "CrashWhat('stdout')"
        )
        self._check(code)

    def test_input_stderr(self):
        code = self.common_input_code.format(
            "sys.addaudithook(audit)",
            "CrashWhat('stderr')"
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
