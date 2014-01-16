"""Test script for popen2.py"""

import warnings
warnings.filterwarnings("ignore", ".*popen2 module is deprecated.*",
                        DeprecationWarning)
warnings.filterwarnings("ignore", "os\.popen. is deprecated.*",
                        DeprecationWarning)

import os
import sys
import unittest
import popen2

from test.test_support import run_unittest, reap_children

if sys.platform[:4] == 'beos' or sys.platform[:6] == 'atheos':
    #  Locks get messed up or something.  Generally we're supposed
    #  to avoid mixing "posix" fork & exec with native threads, and
    #  they may be right about that after all.
    raise unittest.SkipTest("popen2() doesn't work on " + sys.platform)

# if we don't have os.popen, check that
# we have os.fork.  if not, skip the test
# (by raising an ImportError)
try:
    from os import popen
    del popen
except ImportError:
    from os import fork
    del fork

class Popen2Test(unittest.TestCase):
    cmd = "cat"
    if os.name == "nt":
        cmd = "more"
    teststr = "ab cd\n"
    # "more" doesn't act the same way across Windows flavors,
    # sometimes adding an extra newline at the start or the
    # end.  So we strip whitespace off both ends for comparison.
    expected = teststr.strip()

    def setUp(self):
        popen2._cleanup()
        # When the test runs, there shouldn't be any open pipes
        self.assertFalse(popen2._active, "Active pipes when test starts" +
            repr([c.cmd for c in popen2._active]))

    def tearDown(self):
        for inst in popen2._active:
            inst.wait()
        popen2._cleanup()
        self.assertFalse(popen2._active, "popen2._active not empty")
        # The os.popen*() API delegates to the subprocess module (on Unix)
        import subprocess
        for inst in subprocess._active:
            inst.wait()
        subprocess._cleanup()
        self.assertFalse(subprocess._active, "subprocess._active not empty")
        reap_children()

    def validate_output(self, teststr, expected_out, r, w, e=None):
        w.write(teststr)
        w.close()
        got = r.read()
        self.assertEqual(expected_out, got.strip(), "wrote %r read %r" %
                         (teststr, got))

        if e is not None:
            got = e.read()
            self.assertFalse(got, "unexpected %r on stderr" % got)

    def test_popen2(self):
        r, w = popen2.popen2(self.cmd)
        self.validate_output(self.teststr, self.expected, r, w)

    def test_popen3(self):
        if os.name == 'posix':
            r, w, e = popen2.popen3([self.cmd])
            self.validate_output(self.teststr, self.expected, r, w, e)

        r, w, e = popen2.popen3(self.cmd)
        self.validate_output(self.teststr, self.expected, r, w, e)

    def test_os_popen2(self):
        # same test as test_popen2(), but using the os.popen*() API
        if os.name == 'posix':
            w, r = os.popen2([self.cmd])
            self.validate_output(self.teststr, self.expected, r, w)

            w, r = os.popen2(["echo", self.teststr])
            got = r.read()
            self.assertEqual(got, self.teststr + "\n")

        w, r = os.popen2(self.cmd)
        self.validate_output(self.teststr, self.expected, r, w)

    def test_os_popen3(self):
        # same test as test_popen3(), but using the os.popen*() API
        if os.name == 'posix':
            w, r, e = os.popen3([self.cmd])
            self.validate_output(self.teststr, self.expected, r, w, e)

            w, r, e = os.popen3(["echo", self.teststr])
            got = r.read()
            self.assertEqual(got, self.teststr + "\n")
            got = e.read()
            self.assertFalse(got, "unexpected %r on stderr" % got)

        w, r, e = os.popen3(self.cmd)
        self.validate_output(self.teststr, self.expected, r, w, e)

    def test_os_popen4(self):
        if os.name == 'posix':
            w, r = os.popen4([self.cmd])
            self.validate_output(self.teststr, self.expected, r, w)

            w, r = os.popen4(["echo", self.teststr])
            got = r.read()
            self.assertEqual(got, self.teststr + "\n")

        w, r = os.popen4(self.cmd)
        self.validate_output(self.teststr, self.expected, r, w)


def test_main():
    run_unittest(Popen2Test)

if __name__ == "__main__":
    test_main()
