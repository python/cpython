import sys
from test import support
import unittest

crypt = support.import_module('crypt')

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
        self.assertTrue(len(crypt.methods) >= 1)
        if sys.platform.startswith('openbsd'):
            self.assertEqual(crypt.methods, [crypt.METHOD_BLF])
        else:
            self.assertEqual(crypt.methods[-1], crypt.METHOD_CRYPT)

    @unittest.skipUnless(crypt.METHOD_BLF in crypt.methods,
                        'requires support of Blowfish')
    def test_log_rounds(self):
        self.assertEqual(len(crypt._saltchars), 64)
        for log_rounds in range(4, 13):
            salt = crypt.mksalt(crypt.METHOD_BLF, log_rounds=log_rounds)
            self.assertTrue(salt)
            self.assertIn('$%02d$' % log_rounds, salt)
            self.assertIn(len(salt) - crypt.METHOD_BLF.salt_chars, {6, 7})


if __name__ == "__main__":
    unittest.main()
