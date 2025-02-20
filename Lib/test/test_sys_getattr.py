from pathlib import Path
import tempfile
import test.support
from test.support.script_helper import assert_python_failure, assert_python_ok
import textwrap
import unittest




@test.support.cpython_only
class PySysGetAttrTest(unittest.TestCase):

    common_faulthandler_code = textwrap.dedent('''
        from contextlib import redirect_stderr
        from faulthandler import dump_traceback, enable, dump_traceback_later
        from threading import Thread
        import time

        class FakeFile:
            def __init__(self):
                self.f = open("{0}", "w")
            def write(self, s):
                self.f.write(s)
            def flush(self):
                time.sleep(0.2)
            def fileno(self):
                time.sleep(0.2)
                return self.f.fileno()

        def thread1():
            text = FakeFile()
            with redirect_stderr(text):
                time.sleep(0.2)

        def main():
            enable(None, True)
            t1 = Thread(target=thread1, args=())
            t1.start()
            time.sleep(0.1)
            {1}

        if __name__ == "__main__":
            main()
            exit(0)
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


    def test_print_deleted_stdout(self):
        # print should use strong reference to the stdout.
        code = textwrap.dedent('''
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
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(rc, 0)
        self.assertNotIn(b"Segmentation fault", err)
        self.assertNotIn(b"access violation", err)

    def test_faulthandler_enable_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_enable")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "enable(None, True)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(rc, 0)
            self.assertNotIn(b"Segmentation fault", err)
            self.assertNotIn(b"access violation", err)

    def test_faulthandler_dump_traceback_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_dump_traceback")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "dump_traceback(None, False)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(rc, 0)
            self.assertNotIn(b"Segmentation fault", err)
            self.assertNotIn(b"access violation", err)

    def test_faulthandler_dump_traceback_later_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_dump_traceback_later")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "dump_traceback_later(0.1, True, None, False)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(rc, 0)
            self.assertNotIn(b"Segmentation fault", err)
            self.assertNotIn(b"access violation", err)

    def test_warnings_warn(self):
        test_code = self.common_warnings_code.format(
            "warnings.warn(Foo())"
        )
        rc, _, err = assert_python_ok('-c', test_code)
        self.assertEqual(rc, 0)
        self.assertNotIn(b"Segmentation fault", err)
        self.assertNotIn(b"access violation", err)

    def test_warnings_warn_explicit(self):
        test_code = self.common_warnings_code.format(
            "warnings.warn_explicit(Foo(), UserWarning, 'filename', 0)"
        )
        rc, _, err = assert_python_ok('-c', test_code)
        self.assertEqual(rc, 0)
        self.assertNotIn(b"Segmentation fault", err)
        self.assertNotIn(b"access violation", err)

if __name__ == "__main__":
    unittest.main()
