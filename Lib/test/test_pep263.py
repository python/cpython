#! -*- coding: koi8-r -*-
# This file is marked as binary in the CVS, to prevent MacCVS from recoding it.

import unittest
from test import test_support

class PEP263Test(unittest.TestCase):

    def test_pep263(self):
        self.assertEqual(
            u"Питон".encode("utf-8"),
            '\xd0\x9f\xd0\xb8\xd1\x82\xd0\xbe\xd0\xbd'
        )
        self.assertEqual(
            u"\П".encode("utf-8"),
            '\\\xd0\x9f'
        )

def test_main():
    test_support.run_unittest(PEP263Test)

if __name__=="__main__":
    test_main()
