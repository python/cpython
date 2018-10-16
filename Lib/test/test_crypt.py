import sys
from test import support
import unittest

crypt = support.import_module('crypt')

if sys.platform.startswith('openbsd'):
    raise unittest.SkipTest('The only supported method on OpenBSD is Blowfish')

class CryptTestCase(unittest.TestCase):

    def test_crypt(self):
        cr = crypt.crypt('mypassword')
        cr2 = crypt.crypt('mypassword', cr)
        self.assertEqual(cr2, cr)
        cr = crypt.crypt('mypassword', 'ab')
        if cr is not None:
            cr2 = crypt.crypt('mypassword', cr)
            self.assertEqual(cr2, cr)

    def test_salt(self):
        self.assertEqual(len(crypt._saltchars), 64)
        for method in crypt.methods:
            salt = crypt.mksalt(method)
            self.assertIn(len(salt) - method.salt_chars, {0, 1, 3, 4, 6, 7})
            if method.ident:
                self.assertIn(method.ident, salt[:len(salt)-method.salt_chars])

    def test_saltedcrypt(self):
        for method in crypt.methods:
            cr = crypt.crypt('assword', method)
            self.assertEqual(len(cr), method.total_size)
            cr2 = crypt.crypt('assword', cr)
            self.assertEqual(cr2, cr)
            cr = crypt.crypt('assword', crypt.mksalt(method))
            self.assertEqual(len(cr), method.total_size)

    def test_methods(self):
        # Guarantee that METHOD_CRYPT is the last method in crypt.methods.
        self.assertTrue(len(crypt.methods) >= 1)
        self.assertEqual(crypt.METHOD_CRYPT, crypt.methods[-1])


if __name__ == "__main__":
    unittest.main()
