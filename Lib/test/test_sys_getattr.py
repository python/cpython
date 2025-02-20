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
        from threading import Thread
        import time
        from faulthandler import dump_traceback, enable, dump_traceback_later

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
    ''')


    def test_print_deleted_stdout(self):
        # print should use strong reference to the stdout.
        code = textwrap.dedent('''
            # from https://gist.github.com/colesbury/c48f50e95d5d68e24814a56e2664e587
            from contextlib import redirect_stdout
            from io import StringIO
            from threading import Thread
            import time

            class Foo:
                def __repr__(self):
                    time.sleep(0.2)
                    return "Foo"

            def thread1():
                text = StringIO()
                with redirect_stdout(text):
                    time.sleep(0.2)

            def main():
                t1 = Thread(target=thread1, args=())
                t1.start()
                time.sleep(0.1)
                print(Foo())


            if __name__ == "__main__":
                main()
        ''')
        rc, out, err = assert_python_ok('-c', code)
        self.assertEqual(out, b"")
        self.assertEqual(err, b"")

    def test_faulthandler_enable_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_enable")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "enable(None, True)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(out, b"")
            self.assertEqual(err, b"")

    def test_faulthandler_dump_traceback_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_dump_traceback")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "dump_traceback(None, False)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(out, b"")
            self.assertEqual(err, b"")

    def test_faulthandler_dump_traceback_later_deleted_stderr(self):
        # faulthandler should use strong reference to the stderr
        with tempfile.TemporaryDirectory() as tmpdir:
            path = Path(tmpdir, "test_faulthandler_dump_traceback_later")
            test_code = self.common_faulthandler_code.format(
                path.as_posix(),
                "dump_traceback_later(0.1, True, None, False)"
            )
            rc, out, err = assert_python_ok('-c', test_code)
            self.assertEqual(out, b"")
            self.assertEqual(err, b"")

if __name__ == "__main__":
    unittest.main()
