'''
   Tests for fpformat module
   Nick Mathewson
'''
from test.test_support import run_unittest, import_module
import unittest
fpformat = import_module('fpformat', deprecated=True)
fix, sci, NotANumber = fpformat.fix, fpformat.sci, fpformat.NotANumber

StringType = type('')

# Test the old and obsolescent fpformat module.
#
# (It's obsolescent because fix(n,d) == "%.*f"%(d,n) and
#                           sci(n,d) == "%.*e"%(d,n)
#  for all reasonable numeric n and d, except that sci gives 3 exponent
#  digits instead of 2.
#
# Differences only occur for unreasonable n and d.    <.2 wink>)

class FpformatTest(unittest.TestCase):

    def checkFix(self, n, digits):
        result = fix(n, digits)
        if isinstance(n, StringType):
            n = repr(n)
        expected = "%.*f" % (digits, float(n))

        self.assertEqual(result, expected)

    def checkSci(self, n, digits):
        result = sci(n, digits)
        if isinstance(n, StringType):
            n = repr(n)
        expected = "%.*e" % (digits, float(n))
        # add the extra 0 if needed
        num, exp = expected.split("e")
        if len(exp) < 4:
            exp = exp[0] + "0" + exp[1:]
        expected = "%se%s" % (num, exp)

        self.assertEqual(result, expected)

    def test_basic_cases(self):
        self.assertEqual(fix(100.0/3, 3), '33.333')
        self.assertEqual(sci(100.0/3, 3), '3.333e+001')

    def test_reasonable_values(self):
        for d in range(7):
            for val in (1000.0/3, 1000, 1000.0, .002, 1.0/3, 1e10):
                for realVal in (val, 1.0/val, -val, -1.0/val):
                    self.checkFix(realVal, d)
                    self.checkSci(realVal, d)

    def test_failing_values(self):
        # Now for 'unreasonable n and d'
        self.assertEqual(fix(1.0, 1000), '1.'+('0'*1000))
        self.assertEqual(sci("1"+('0'*1000), 0), '1e+1000')

        # This behavior is inconsistent.  sci raises an exception; fix doesn't.
        yacht = "Throatwobbler Mangrove"
        self.assertEqual(fix(yacht, 10), yacht)
        try:
            sci(yacht, 10)
        except NotANumber:
            pass
        else:
            self.fail("No exception on non-numeric sci")

    def test_REDOS(self):
        # This attack string will hang on the old decoder pattern.
        attack = '+0' + ('0' * 1000000) + '++'
        digs = 5 # irrelevant

        # fix returns input if it does not decode
        self.assertEqual(fpformat.fix(attack, digs), attack)
        # sci raises NotANumber
        with self.assertRaises(NotANumber):
            fpformat.sci(attack, digs)

def test_main():
    run_unittest(FpformatTest)


if __name__ == "__main__":
    test_main()
