"""
Test suite to check compilance with PEP 247, the standard API
for hashing algorithms
"""

import warnings
warnings.filterwarnings('ignore', 'the md5 module is deprecated.*',
                        DeprecationWarning)
warnings.filterwarnings('ignore', 'the sha module is deprecated.*',
                        DeprecationWarning)
import hmac
import md5
import sha
import unittest
from test import test_support

class Pep247Test(unittest.TestCase):

    def check_module(self, module, key=None):
        self.assert_(hasattr(module, 'digest_size'))
        self.assert_(module.digest_size is None or module.digest_size > 0)
        if not key is None:
            obj1 = module.new(key)
            obj2 = module.new(key, 'string')
            h1 = module.new(key, 'string').digest()
            obj3 = module.new(key)
            obj3.update('string')
            h2 = obj3.digest()
        else:
            obj1 = module.new()
            obj2 = module.new('string')
            h1 = module.new('string').digest()
            obj3 = module.new()
            obj3.update('string')
            h2 = obj3.digest()
        self.assertEquals(h1, h2)
        self.assert_(hasattr(obj1, 'digest_size'))

        if not module.digest_size is None:
            self.assertEquals(obj1.digest_size, module.digest_size)

        self.assertEquals(obj1.digest_size, len(h1))
        obj1.update('string')
        obj_copy = obj1.copy()
        self.assertEquals(obj1.digest(), obj_copy.digest())
        self.assertEquals(obj1.hexdigest(), obj_copy.hexdigest())

        digest, hexdigest = obj1.digest(), obj1.hexdigest()
        hd2 = ""
        for byte in digest:
            hd2 += '%02x' % ord(byte)
        self.assertEquals(hd2, hexdigest)

    def test_md5(self):
        self.check_module(md5)

    def test_sha(self):
        self.check_module(sha)

    def test_hmac(self):
        self.check_module(hmac, key='abc')

def test_main():
    test_support.run_unittest(Pep247Test)

if __name__ == '__main__':
    test_main()
