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

class TestCase(unittest.TestCase):
    def setUp(self):
        self.stream = io.StringIO()
        self.save_stdout, self.save_stderr = sys.stderr, sys.stdout
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


def test_main():
    support.run_unittest(TestCase)

if __name__ == "__main__":
    test_main()
