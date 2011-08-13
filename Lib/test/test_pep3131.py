import unittest
from test import support

class PEP3131Test(unittest.TestCase):

    def test_valid(self):
        class T:
            ä = 1
            µ = 2 # this is a compatibility character
            蟒 = 3
            𝔘𝔫𝔦𝔠𝔬𝔡𝔢  = 4
        self.assertEqual(getattr(T, "\xe4"), 1)
        self.assertEqual(getattr(T, "\u03bc"), 2)
        self.assertEqual(getattr(T, '\u87d2'), 3)
        v = getattr(T, "\U0001d518\U0001d52b\U0001d526\U0001d520\U0001d52c\U0001d521\U0001d522")
        self.assertEqual(v, 4)

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
