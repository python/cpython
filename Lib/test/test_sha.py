# Testing sha module (NIST's Secure Hash Algorithm)

# use the three examples from Federal Information Processing Standards
# Publication 180-1, Secure Hash Standard,  1995 April 17
# http://www.itl.nist.gov/div897/pubs/fip180-1.htm

import warnings
warnings.filterwarnings("ignore", "the sha module is deprecated.*",
                        DeprecationWarning)

import sha
import unittest
from test import test_support


class SHATestCase(unittest.TestCase):
    def check(self, data, digest):
        # Check digest matches the expected value
        obj = sha.new(data)
        computed = obj.hexdigest()
        self.assert_(computed == digest)

        # Verify that the value doesn't change between two consecutive
        # digest operations.
        computed_again = obj.hexdigest()
        self.assert_(computed == computed_again)

        # Check hexdigest() output matches digest()'s output
        digest = obj.digest()
        hexd = ""
        for c in digest:
            hexd += '%02x' % ord(c)
        self.assert_(computed == hexd)

    def test_case_1(self):
        self.check("abc",
                   "a9993e364706816aba3e25717850c26c9cd0d89d")

    def test_case_2(self):
        self.check("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                   "84983e441c3bd26ebaae4aa1f95129e5e54670f1")

    def test_case_3(self):
        self.check("a" * 1000000,
                   "34aa973cd4c4daa4f61eeb2bdbad27316534016f")

    def test_case_4(self):
        self.check(chr(0xAA) * 80,
                   '4ca0ef38f1794b28a8f8ee110ee79d48ce13be25')

def test_main():
    test_support.run_unittest(SHATestCase)


if __name__ == "__main__":
    test_main()
