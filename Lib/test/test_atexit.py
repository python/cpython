import sys
import unittest
import StringIO
import atexit
from test import test_support

class TestCase(unittest.TestCase):
    def test_args(self):
        # be sure args are handled properly
        s = StringIO.StringIO()
        sys.stdout = sys.stderr = s
        save_handlers = atexit._exithandlers
        atexit._exithandlers = []
        try:
            atexit.register(self.h1)
            atexit.register(self.h4)
            atexit.register(self.h4, 4, kw="abc")
            atexit._run_exitfuncs()
        finally:
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
            atexit._exithandlers = save_handlers
        self.assertEqual(s.getvalue(), "h4 (4,) {'kw': 'abc'}\nh4 () {}\nh1\n")

    def test_order(self):
        # be sure handlers are executed in reverse order
        s = StringIO.StringIO()
        sys.stdout = sys.stderr = s
        save_handlers = atexit._exithandlers
        atexit._exithandlers = []
        try:
            atexit.register(self.h1)
            atexit.register(self.h2)
            atexit.register(self.h3)
            atexit._run_exitfuncs()
        finally:
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
            atexit._exithandlers = save_handlers
        self.assertEqual(s.getvalue(), "h3\nh2\nh1\n")

    def test_sys_override(self):
        # be sure a preset sys.exitfunc is handled properly
        s = StringIO.StringIO()
        sys.stdout = sys.stderr = s
        save_handlers = atexit._exithandlers
        atexit._exithandlers = []
        exfunc = sys.exitfunc
        sys.exitfunc = self.h1
        reload(atexit)
        try:
            atexit.register(self.h2)
            atexit._run_exitfuncs()
        finally:
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
            atexit._exithandlers = save_handlers
            sys.exitfunc = exfunc
        self.assertEqual(s.getvalue(), "h2\nh1\n")

    def test_raise(self):
        # be sure raises are handled properly
        s = StringIO.StringIO()
        sys.stdout = sys.stderr = s
        save_handlers = atexit._exithandlers
        atexit._exithandlers = []
        try:
            atexit.register(self.raise1)
            atexit.register(self.raise2)
            self.assertRaises(TypeError, atexit._run_exitfuncs)
        finally:
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
            atexit._exithandlers = save_handlers

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
