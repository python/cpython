import re
import unittest
import warnings

from test import test_support

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
                self.fail("SyntaxError did not contain '%r'" % (errtext,))
        else:
            self.fail("compile() did not raise SyntaxError")

    def test_assign_call(self):
        self._check_error("f() = 1", "assign")

    def test_assign_del(self):
        self._check_error("del f()", "delete")

    def test_global_err_then_warn(self):
        # Bug tickler:  The SyntaxError raised for one global statement
        # shouldn't be clobbered by a SyntaxWarning issued for a later one.
        source = re.sub('(?m)^ *:', '', """\
            :def error(a):
            :    global a  # SyntaxError
            :def warning():
            :    b = 1
            :    global b  # SyntaxWarning
            :""")
        warnings.filterwarnings(action='ignore', category=SyntaxWarning)
        self._check_error(source, "global")
        warnings.filters.pop(0)

def test_main():
    test_support.run_unittest(SyntaxTestCase)

if __name__ == "__main__":
    test_main()
