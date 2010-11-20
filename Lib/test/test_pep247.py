"""
Test suite to check compilance with PEP 247, the standard API
for hashing algorithms
"""

import hmac
import unittest
from hashlib import md5, sha1, sha224, sha256, sha384, sha512
from test import support

class Pep247Test(unittest.TestCase):

    def check_module(self, module, key=None):
        self.assertTrue(hasattr(module, 'digest_size'))
        self.assertTrue(module.digest_size is None or module.digest_size > 0)
        self.check_object(module.new, module.digest_size, key)

    def check_object(self, cls, digest_size, key):
        if key is not None:
            obj1 = cls(key)
            obj2 = cls(key, b'string')
            h1 = cls(key, b'string').digest()
            obj3 = cls(key)
            obj3.update(b'string')
            h2 = obj3.digest()
        else:
            obj1 = cls()
            obj2 = cls(b'string')
            h1 = cls(b'string').digest()
            obj3 = cls()
            obj3.update(b'string')
            h2 = obj3.digest()
        self.assertEqual(h1, h2)
        self.assertTrue(hasattr(obj1, 'digest_size'))

        if digest_size is not None:
            self.assertEqual(obj1.digest_size, digest_size)

        self.assertEqual(obj1.digest_size, len(h1))
        obj1.update(b'string')
        obj_copy = obj1.copy()
        self.assertEqual(obj1.digest(), obj_copy.digest())
        self.assertEqual(obj1.hexdigest(), obj_copy.hexdigest())

        digest, hexdigest = obj1.digest(), obj1.hexdigest()
        hd2 = ""
        for byte in digest:
            hd2 += '%02x' % byte
        self.assertEqual(hd2, hexdigest)

    def test_md5(self):
        self.check_object(md5, None, None)

    def test_sha(self):
        self.check_object(sha1, None, None)
        self.check_object(sha224, None, None)
        self.check_object(sha256, None, None)
        self.check_object(sha384, None, None)
        self.check_object(sha512, None, None)

    def test_hmac(self):
        self.check_module(hmac, key=b'abc')

def test_main():
    support.run_unittest(Pep247Test)

if __name__ == '__main__':
    test_main()
