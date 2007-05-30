# Testing md5 module
import warnings
warnings.filterwarnings("ignore", "the md5 module is deprecated.*",
                        DeprecationWarning)

import unittest
from md5 import md5
from test import test_support

def hexstr(s):
    import string
    h = string.hexdigits
    r = ''
    for c in s:
        i = ord(c)
        r = r + h[(i >> 4) & 0xF] + h[i & 0xF]
    return r

class MD5_Test(unittest.TestCase):

    def md5test(self, s, expected):
        self.assertEqual(hexstr(md5(s).digest()), expected)
        self.assertEqual(md5(s).hexdigest(), expected)

    def test_basics(self):
        eq = self.md5test
        eq('', 'd41d8cd98f00b204e9800998ecf8427e')
        eq('a', '0cc175b9c0f1b6a831c399e269772661')
        eq('abc', '900150983cd24fb0d6963f7d28e17f72')
        eq('message digest', 'f96b697d7cb7938d525a2f31aaf161d0')
        eq('abcdefghijklmnopqrstuvwxyz', 'c3fcd3d76192e4007dfb496cca67e13b')
        eq('ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
           'd174ab98d277d9f5a5611c2c9f419d9f')
        eq('12345678901234567890123456789012345678901234567890123456789012345678901234567890',
           '57edf4a22be3c955ac49da2e2107b67a')

    def test_hexdigest(self):
        # hexdigest is new with Python 2.0
        m = md5('testing the hexdigest method')
        h = m.hexdigest()
        self.assertEqual(hexstr(m.digest()), h)

    def test_large_update(self):
        aas = 'a' * 64
        bees = 'b' * 64
        cees = 'c' * 64

        m1 = md5()
        m1.update(aas)
        m1.update(bees)
        m1.update(cees)

        m2 = md5()
        m2.update(aas + bees + cees)
        self.assertEqual(m1.digest(), m2.digest())

def test_main():
    test_support.run_unittest(MD5_Test)

if __name__ == '__main__':
    test_main()
