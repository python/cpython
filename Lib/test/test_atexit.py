import sys
import unittest
import StringIO
import atexit
from imp import reload
from test import test_support


def exit():
    raise SystemExit


class TestCase(unittest.TestCase):
    def setUp(self):
        self.save_stdout = sys.stdout
        self.save_stderr = sys.stderr
        self.stream = StringIO.StringIO()
        sys.stdout = sys.stderr = self.subst_io = self.stream
        self.save_handlers = atexit._exithandlers
        atexit._exithandlers = []

    def tearDown(self):
        sys.stdout = self.save_stdout
        sys.stderr = self.save_stderr
        atexit._exithandlers = self.save_handlers

    def test_args(self):
        atexit.register(self.h1)
        atexit.register(self.h4)
        atexit.register(self.h4, 4, kw="abc")
        atexit._run_exitfuncs()
        self.assertEqual(self.subst_io.getvalue(),
                         "h4 (4,) {'kw': 'abc'}\nh4 () {}\nh1\n")

    def test_badargs(self):
        atexit.register(lambda: 1, 0, 0, (x for x in (1,2)), 0, 0)
        self.assertRaises(TypeError, atexit._run_exitfuncs)

    def test_order(self):
        atexit.register(self.h1)
        atexit.register(self.h2)
        atexit.register(self.h3)
        atexit._run_exitfuncs()
        self.assertEqual(self.subst_io.getvalue(), "h3\nh2\nh1\n")

    def test_sys_override(self):
        # be sure a preset sys.exitfunc is handled properly
        exfunc = sys.exitfunc
        sys.exitfunc = self.h1
        reload(atexit)
        try:
            atexit.register(self.h2)
            atexit._run_exitfuncs()
        finally:
            sys.exitfunc = exfunc
        self.assertEqual(self.subst_io.getvalue(), "h2\nh1\n")

    def test_raise(self):
        atexit.register(self.raise1)
        atexit.register(self.raise2)
        self.assertRaises(TypeError, atexit._run_exitfuncs)

    def test_exit(self):
        # be sure a SystemExit is handled properly
        atexit.register(exit)

        self.assertRaises(SystemExit, atexit._run_exitfuncs)
        self.assertEqual(self.stream.getvalue(), '')

    ### helpers
    def h1(self):
        print "h1"

    def h2(self):
        print "h2"

    def h3(self):
        print "h3"

    def h4(self, *args, **kwargs):
        print "h4", args, kwargs

    def raise1(self):
        raise TypeError

    def raise2(self):
        raise SystemError

def test_main():
    test_support.run_unittest(TestCase)


if __name__ == "__main__":
    test_main()
