import sys
from test import test_support
import unittest

crypt = test_support.import_module('crypt')

if sys.platform.startswith('openbsd'):
    raise unittest.SkipTest('The only supported method on OpenBSD is Blowfish')

class CryptTestCase(unittest.TestCase):

    def test_crypt(self):
        cr = crypt.crypt('mypassword', 'ab')
        if cr is not None:
            cr2 = crypt.crypt('mypassword', cr)
            self.assertEqual(cr2, cr)


def test_main():
    test_support.run_unittest(CryptTestCase)

if __name__ == "__main__":
    test_main()
