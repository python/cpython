import unittest
import sys
from test import support

class PEP3131Test(unittest.TestCase):

    def test_valid(self):
        class T:
            ä = 1
            µ = 2 # this is a compatibility character
            蟒 = 3
            x󠄀 = 4
        self.assertEqual(getattr(T, "\xe4"), 1)
        self.assertEqual(getattr(T, "\u03bc"), 2)
        self.assertEqual(getattr(T, '\u87d2'), 3)
        self.assertEqual(getattr(T, 'x\U000E0100'), 4)

    def test_non_bmp_normalized(self):
        𝔘𝔫𝔦𝔠𝔬𝔡𝔢 = 1
        # On wide builds, this is normalized, but on narrow ones it is not. See
        # #12746.
        try:
            self.assertIn("𝔘𝔫𝔦𝔠𝔬𝔡𝔢", dir())
        except AssertionError:
            raise unittest.case._ExpectedFailure(sys.exc_info())

    def test_invalid(self):
        try:
            from test import badsyntax_3131
        except SyntaxError as s:
            self.assertEqual(str(s),
              "invalid character in identifier (badsyntax_3131.py, line 2)")
        else:
            self.fail("expected exception didn't occur")

def test_main():
    support.run_unittest(PEP3131Test)

if __name__=="__main__":
    test_main()
