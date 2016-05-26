from test import support
import unittest

crypt = support.import_module('crypt')

class CryptTestCase(unittest.TestCase):

    def test_crypt(self):
        c = crypt.crypt('mypassword', 'ab')
        if support.verbose:
            print('Test encryption: ', c)

    def test_salt(self):
        self.assertEqual(len(crypt._saltchars), 64)
        for method in crypt.methods:
            salt = crypt.mksalt(method)
            self.assertEqual(len(salt),
                    method.salt_chars + (3 if method.ident else 0))

    def test_saltedcrypt(self):
        for method in crypt.methods:
            pw = crypt.crypt('assword', method)
            self.assertEqual(len(pw), method.total_size)
            pw = crypt.crypt('assword', crypt.mksalt(method))
            self.assertEqual(len(pw), method.total_size)

    def test_methods(self):
        # Guarantee that METHOD_CRYPT is the last method in crypt.methods.
        self.assertTrue(len(crypt.methods) >= 1)
        self.assertEqual(crypt.METHOD_CRYPT, crypt.methods[-1])

if __name__ == "__main__":
    unittest.main()
