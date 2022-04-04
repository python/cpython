import atexit
import os
import sys
import textwrap
import unittest
from test import support
from test.support import script_helper


class GeneralTest(unittest.TestCase):
    def test_general(self):
        # Run _test_atexit.py in a subprocess since it calls atexit._clear()
        script = support.findfile("_test_atexit.py")
        script_helper.run_test_script(script)

class FunctionalTest(unittest.TestCase):
    def test_shutdown(self):
        # Actually test the shutdown mechanism in a subprocess
        code = textwrap.dedent("""
            import atexit

            def f(msg):
                print(msg)

            atexit.register(f, "one")
            atexit.register(f, "two")
        """)
        res = script_helper.assert_python_ok("-c", code)
        self.assertEqual(res.out.decode().splitlines(), ["two", "one"])
        self.assertFalse(res.err)

    def test_atexit_instances(self):
        # bpo-42639: It is safe to have more than one atexit instance.
        code = textwrap.dedent("""
            import sys
            import atexit as atexit1
            del sys.modules['atexit']
            import atexit as atexit2
            del sys.modules['atexit']

            assert atexit2 is not atexit1

            atexit1.register(print, "atexit1")
            atexit2.register(print, "atexit2")
        """)
        res = script_helper.assert_python_ok("-c", code)
        self.assertEqual(res.out.decode().splitlines(), ["atexit2", "atexit1"])
        self.assertFalse(res.err)


@support.cpython_only
class SubinterpreterTest(unittest.TestCase):

    def test_callbacks_leak(self):
        # This test shows a leak in refleak mode if atexit doesn't
        # take care to free callbacks in its per-subinterpreter module
        # state.
        n = atexit._ncallbacks()
        code = textwrap.dedent(r"""
            import atexit
            def f():
                pass
            atexit.register(f)
            del atexit
        """)
        ret = support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        self.assertEqual(atexit._ncallbacks(), n)

    def test_callbacks_leak_refcycle(self):
        # Similar to the above, but with a refcycle through the atexit
        # module.
        n = atexit._ncallbacks()
        code = textwrap.dedent(r"""
            import atexit
            def f():
                pass
            atexit.register(f)
            atexit.__atexit = atexit
        """)
        ret = support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        self.assertEqual(atexit._ncallbacks(), n)

    def test_callback_on_subinterpreter_teardown(self):
        # This tests if a callback is called on
        # subinterpreter teardown.
        expected = b"The test has passed!"
        r, w = os.pipe()

        code = textwrap.dedent(r"""
            import os
            import atexit
            def callback():
                os.write({:d}, b"The test has passed!")
            atexit.register(callback)
        """.format(w))
        ret = support.run_in_subinterp(code)
        os.close(w)
        self.assertEqual(os.read(r, len(expected)), expected)
        os.close(r)


if __name__ == "__main__":
    unittest.main()
