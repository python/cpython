import sys
import unittest
import io
import atexit
from test import support

### helpers
def h1():
    print("h1")

def h2():
    print("h2")

def h3():
    print("h3")

def h4(*args, **kwargs):
    print("h4", args, kwargs)

def raise1():
    raise TypeError

def raise2():
    raise SystemError

def exit():
    raise SystemExit


class GeneralTest(unittest.TestCase):

    def setUp(self):
        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr
        self.stream = io.StringIO()
        sys.stdout = sys.stderr = self.stream
        atexit._clear()

    def tearDown(self):
        sys.stdout = self.save_stdout
        sys.stderr = self.save_stderr
        atexit._clear()

    def test_args(self):
        # be sure args are handled properly
        atexit.register(h1)
        atexit.register(h4)
        atexit.register(h4, 4, kw="abc")
        atexit._run_exitfuncs()

        self.assertEqual(self.stream.getvalue(),
                            "h4 (4,) {'kw': 'abc'}\nh4 () {}\nh1\n")

    def test_badargs(self):
        atexit.register(lambda: 1, 0, 0, (x for x in (1,2)), 0, 0)
        self.assertRaises(TypeError, atexit._run_exitfuncs)

    def test_order(self):
        # be sure handlers are executed in reverse order
        atexit.register(h1)
        atexit.register(h2)
        atexit.register(h3)
        atexit._run_exitfuncs()

        self.assertEqual(self.stream.getvalue(), "h3\nh2\nh1\n")

    def test_raise(self):
        # be sure raises are handled properly
        atexit.register(raise1)
        atexit.register(raise2)

        self.assertRaises(TypeError, atexit._run_exitfuncs)

    def test_raise_unnormalized(self):
        # Issue #10756: Make sure that an unnormalized exception is
        # handled properly
        atexit.register(lambda: 1 / 0)

        self.assertRaises(ZeroDivisionError, atexit._run_exitfuncs)
        self.assertIn("ZeroDivisionError", self.stream.getvalue())

    def test_exit(self):
        # be sure a SystemExit is handled properly
        atexit.register(exit)

        self.assertRaises(SystemExit, atexit._run_exitfuncs)
        self.assertEqual(self.stream.getvalue(), '')

    def test_print_tracebacks(self):
        # Issue #18776: the tracebacks should be printed when errors occur.
        def f():
            1/0  # one
        def g():
            1/0  # two
        def h():
            1/0  # three
        atexit.register(f)
        atexit.register(g)
        atexit.register(h)

        self.assertRaises(ZeroDivisionError, atexit._run_exitfuncs)
        stderr = self.stream.getvalue()
        self.assertEqual(stderr.count("ZeroDivisionError"), 3)
        self.assertIn("# one", stderr)
        self.assertIn("# two", stderr)
        self.assertIn("# three", stderr)

    def test_stress(self):
        a = [0]
        def inc():
            a[0] += 1

        for i in range(128):
            atexit.register(inc)
        atexit._run_exitfuncs()

        self.assertEqual(a[0], 128)

    def test_clear(self):
        a = [0]
        def inc():
            a[0] += 1

        atexit.register(inc)
        atexit._clear()
        atexit._run_exitfuncs()

        self.assertEqual(a[0], 0)

    def test_unregister(self):
        a = [0]
        def inc():
            a[0] += 1
        def dec():
            a[0] -= 1

        for i in range(4):
            atexit.register(inc)
        atexit.register(dec)
        atexit.unregister(inc)
        atexit._run_exitfuncs()

        self.assertEqual(a[0], -1)

    def test_bound_methods(self):
        l = []
        atexit.register(l.append, 5)
        atexit._run_exitfuncs()
        self.assertEqual(l, [5])

        atexit.unregister(l.append)
        atexit._run_exitfuncs()
        self.assertEqual(l, [5])


@support.cpython_only
class SubinterpreterTest(unittest.TestCase):

    def test_callbacks_leak(self):
        # This test shows a leak in refleak mode if atexit doesn't
        # take care to free callbacks in its per-subinterpreter module
        # state.
        n = atexit._ncallbacks()
        code = r"""if 1:
            import atexit
            def f():
                pass
            atexit.register(f)
            del atexit
            """
        ret = support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        self.assertEqual(atexit._ncallbacks(), n)

    def test_callbacks_leak_refcycle(self):
        # Similar to the above, but with a refcycle through the atexit
        # module.
        n = atexit._ncallbacks()
        code = r"""if 1:
            import atexit
            def f():
                pass
            atexit.register(f)
            atexit.__atexit = atexit
            """
        ret = support.run_in_subinterp(code)
        self.assertEqual(ret, 0)
        self.assertEqual(atexit._ncallbacks(), n)


if __name__ == "__main__":
    unittest.main()
