from test import support
import unittest

crypt = support.import_module('crypt')

class CryptTestCase(unittest.TestCase):

    def test_crypt(self):
        c = crypt.crypt('mypassword', 'ab')
        if support.verbose:
            print('Test encryption: ', c)

def test_main():
    support.run_unittest(CryptTestCase)

if __name__ == "__main__":
    test_main()
