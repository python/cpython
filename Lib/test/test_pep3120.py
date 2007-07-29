# This file is marked as binary in the CVS, to prevent MacCVS from recoding it.

import unittest
from test import test_support

class PEP3120Test(unittest.TestCase):

    def test_pep3120(self):
        self.assertEqual(
            "Питон".encode("utf-8"),
            b'\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
        )
        self.assertEqual(
            "\П".encode("utf-8"),
            b'\\\xd0\x9f'
        )

    def test_badsyntax(self):
        try:
            import test.badsyntax_pep3120
        except SyntaxError as msg:
            self.assert_(str(msg).find("Non-UTF-8 code starting with") >= 0)
        else:
            self.fail("expected exception didn't occur")

def test_main():
    test_support.run_unittest(PEP3120Test)

if __name__=="__main__":
    test_main()
