from test import test_support
import unittest
import crypt

class CryptTestCase(unittest.TestCase):

    def test_crypt(self):
        c = crypt.crypt('mypassword', 'ab')
        if test_support.verbose:
            print('Test encryption: ', c)

def test_main():
    test_support.run_unittest(CryptTestCase)

if __name__ == "__main__":
    test_main()
