import test.support
from test.support.script_helper import assert_python_failure, assert_python_ok
import textwrap
import unittest




@test.support.cpython_only
class PySysGetAttrTest(unittest.TestCase):


    def test_changing_stdout_from_thread(self):
        # print should use strong reference to the stdout.
        code = textwrap.dedent('''
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

if __name__ == "__main__":
    unittest.main()
