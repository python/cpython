import hmac
import unittest
import test_support

class TestVectorsTestCase(unittest.TestCase):
    def test_vectors(self):
        """Test the HMAC module against test vectors from the RFC."""

        def md5test(key, data, digest):
            h = hmac.HMAC(key, data)
            self.failUnless(h.hexdigest().upper() == digest.upper())

        md5test(chr(0x0b) * 16,
                "Hi There",
                "9294727A3638BB1C13F48EF8158BFC9D")

        md5test("Jefe",
                "what do ya want for nothing?",
                "750c783e6ab0b503eaa86e310a5db738")

        md5test(chr(0xAA)*16,
                chr(0xDD)*50,
                "56be34521d144c88dbb8c733f0e8b3f6")

class ConstructorTestCase(unittest.TestCase):
    def test_normal(self):
        """Standard constructor call."""
        failed = 0
        try:
            h = hmac.HMAC("key")
        except:
            self.fail("Standard constructor call raised exception.")

    def test_withtext(self):
        """Constructor call with text."""
        try:
            h = hmac.HMAC("key", "hash this!")
        except:
            self.fail("Constructor call with text argument raised exception.")

    def test_withmodule(self):
        """Constructor call with text and digest module."""
        import sha
        try:
            h = hmac.HMAC("key", "", sha)
        except:
            self.fail("Constructor call with sha module raised exception.")

class SanityTestCase(unittest.TestCase):
    def test_default_is_md5(self):
        """Testing if HMAC defaults to MD5 algorithm."""
        import md5
        h = hmac.HMAC("key")
        self.failUnless(h.digestmod == md5)

    def test_exercise_all_methods(self):
        """Exercising all methods once."""
        # This must not raise any exceptions
        try:
            h = hmac.HMAC("my secret key")
            h.update("compute the hash of this text!")
            dig = h.digest()
            dig = h.hexdigest()
            h2 = h.copy()
        except:
            fail("Exception raised during normal usage of HMAC class.")

class CopyTestCase(unittest.TestCase):
    def test_attributes(self):
        """Testing if attributes are of same type."""
        h1 = hmac.HMAC("key")
        h2 = h1.copy()
        self.failUnless(h1.digestmod == h2.digestmod,
            "Modules don't match.")
        self.failUnless(type(h1.inner) == type(h2.inner),
            "Types of inner don't match.")
        self.failUnless(type(h1.outer) == type(h2.outer),
            "Types of outer don't match.")

    def test_realcopy(self):
        """Testing if the copy method created a real copy."""
        h1 = hmac.HMAC("key")
        h2 = h1.copy()
        # Using id() in case somebody has overridden __cmp__.
        self.failUnless(id(h1) != id(h2), "No real copy of the HMAC instance.")
        self.failUnless(id(h1.inner) != id(h2.inner),
            "No real copy of the attribute 'inner'.")
        self.failUnless(id(h1.outer) != id(h2.outer),
            "No real copy of the attribute 'outer'.")

    def test_equality(self):
        """Testing if the copy has the same digests."""
        h1 = hmac.HMAC("key")
        h1.update("some random text")
        h2 = h1.copy()
        self.failUnless(h1.digest() == h2.digest(),
            "Digest of copy doesn't match original digest.")
        self.failUnless(h1.hexdigest() == h2.hexdigest(),
            "Hexdigest of copy doesn't match original hexdigest.")

def test_main():
    test_support.run_unittest(TestVectorsTestCase)
    test_support.run_unittest(ConstructorTestCase)
    test_support.run_unittest(SanityTestCase)
    test_support.run_unittest(CopyTestCase)

if __name__ == "__main__":
    test_main()
