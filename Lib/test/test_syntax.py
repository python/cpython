import re
import unittest

import test_support

class SyntaxTestCase(unittest.TestCase):

    def _check_error(self, code, errtext,
                     filename="<testcase>", mode="exec"):
        """Check that compiling code raises SyntaxError with errtext.

        errtest is a regular expression that must be present in the
        test of the exception raised.
        """
        try:
            compile(code, filename, mode)
        except SyntaxError, err:
            mo = re.search(errtext, str(err))
            if mo is None:
                self.fail("SyntaxError did not contain '%s'" % `errtext`)
        else:
            self.fail("compile() did not raise SyntaxError")

    def test_assign_call(self):
        self._check_error("f() = 1", "assign")

    def test_assign_del(self):
        self._check_error("del f()", "delete")

def test_main():
    test_support.run_unittest(SyntaxTestCase)

if __name__ == "__main__":
    test_main()
