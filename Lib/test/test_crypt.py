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
            self.assertEqual(crypt.methods, [crypt.METHOD_BLOWFISH])
        else:
            self.assertEqual(crypt.methods[-1], crypt.METHOD_CRYPT)

    @unittest.skipUnless(crypt.METHOD_SHA256 in crypt.methods or
                         crypt.METHOD_SHA512 in crypt.methods,
                        'requires support of SHA-2')
    def test_sha2_rounds(self):
        for method in (crypt.METHOD_SHA256, crypt.METHOD_SHA512):
            for rounds in 1000, 10_000, 100_000:
                salt = crypt.mksalt(method, rounds=rounds)
                self.assertIn('$rounds=%d$' % rounds, salt)
                self.assertEqual(len(salt) - method.salt_chars,
                                 11 + len(str(rounds)))
                cr = crypt.crypt('mypassword', salt)
                self.assertTrue(cr)
                cr2 = crypt.crypt('mypassword', cr)
                self.assertEqual(cr2, cr)

    @unittest.skipUnless(crypt.METHOD_BLOWFISH in crypt.methods,
                        'requires support of Blowfish')
    def test_blowfish_rounds(self):
        for log_rounds in range(4, 11):
            salt = crypt.mksalt(crypt.METHOD_BLOWFISH, rounds=1 << log_rounds)
            self.assertIn('$%02d$' % log_rounds, salt)
            self.assertIn(len(salt) - crypt.METHOD_BLOWFISH.salt_chars, {6, 7})
            cr = crypt.crypt('mypassword', salt)
            self.assertTrue(cr)
            cr2 = crypt.crypt('mypassword', cr)
            self.assertEqual(cr2, cr)

    def test_invalid_rounds(self):
        for method in (crypt.METHOD_SHA256, crypt.METHOD_SHA512,
                       crypt.METHOD_BLOWFISH):
            with self.assertRaises(TypeError):
                crypt.mksalt(method, rounds='4096')
            with self.assertRaises(TypeError):
                crypt.mksalt(method, rounds=4096.0)
            for rounds in (0, 1, -1, 1<<999):
                with self.assertRaises(ValueError):
                    crypt.mksalt(method, rounds=rounds)
        with self.assertRaises(ValueError):
            crypt.mksalt(crypt.METHOD_BLOWFISH, rounds=1000)
        for method in (crypt.METHOD_CRYPT, crypt.METHOD_MD5):
            with self.assertRaisesRegex(ValueError, 'support'):
                crypt.mksalt(method, rounds=4096)


if __name__ == "__main__":
    unittest.main()
