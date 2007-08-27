import hmac
from hashlib import sha1
import unittest
from test import test_support

class TestVectorsTestCase(unittest.TestCase):

    def test_md5_vectors(self):
        # Test the HMAC module against test vectors from the RFC.

        def md5test(key, data, digest):
            h = hmac.HMAC(key, data)
            self.assertEqual(h.hexdigest().upper(), digest.upper())

        md5test(b"\x0b" * 16,
                b"Hi There",
                "9294727A3638BB1C13F48EF8158BFC9D")

        md5test(b"Jefe",
                b"what do ya want for nothing?",
                "750c783e6ab0b503eaa86e310a5db738")

        md5test(b"\xaa" * 16,
                b"\xdd" * 50,
                "56be34521d144c88dbb8c733f0e8b3f6")

        md5test(bytes(range(1, 26)),
                b"\xcd" * 50,
                "697eaf0aca3a3aea3a75164746ffaa79")

        md5test(b"\x0C" * 16,
                b"Test With Truncation",
                "56461ef2342edc00f9bab995690efd4c")

        md5test(b"\xaa" * 80,
                b"Test Using Larger Than Block-Size Key - Hash Key First",
                "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd")

        md5test(b"\xaa" * 80,
                (b"Test Using Larger Than Block-Size Key "
                 b"and Larger Than One Block-Size Data"),
                "6f630fad67cda0ee1fb1f562db3aa53e")

    def test_sha_vectors(self):
        def shatest(key, data, digest):
            h = hmac.HMAC(key, data, digestmod=sha1)
            self.assertEqual(h.hexdigest().upper(), digest.upper())

        shatest(b"\x0b" * 20,
                b"Hi There",
                "b617318655057264e28bc0b6fb378c8ef146be00")

        shatest(b"Jefe",
                b"what do ya want for nothing?",
                "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79")

        shatest(b"\xAA" * 20,
                b"\xDD" * 50,
                "125d7342b9ac11cd91a39af48aa17b4f63f175d3")

        shatest(bytes(range(1, 26)),
                b"\xCD" * 50,
                "4c9007f4026250c6bc8414f9bf50c86c2d7235da")

        shatest(b"\x0C" * 20,
                b"Test With Truncation",
                "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04")

        shatest(b"\xAA" * 80,
                b"Test Using Larger Than Block-Size Key - Hash Key First",
                "aa4ae5e15272d00e95705637ce8a3b55ed402112")

        shatest(b"\xAA" * 80,
                (b"Test Using Larger Than Block-Size Key "
                 b"and Larger Than One Block-Size Data"),
                "e8e99d0f45237d786d6bbaa7965c7808bbff1a91")


class ConstructorTestCase(unittest.TestCase):

    def test_normal(self):
        # Standard constructor call.
        failed = 0
        try:
            h = hmac.HMAC(b"key")
        except:
            self.fail("Standard constructor call raised exception.")

    def test_withtext(self):
        # Constructor call with text.
        try:
            h = hmac.HMAC(b"key", b"hash this!")
        except:
            self.fail("Constructor call with text argument raised exception.")

    def test_withmodule(self):
        # Constructor call with text and digest module.
        from hashlib import sha1
        try:
            h = hmac.HMAC(b"key", b"", sha1)
        except:
            self.fail("Constructor call with hashlib.sha1 raised exception.")

class SanityTestCase(unittest.TestCase):

    def test_default_is_md5(self):
        # Testing if HMAC defaults to MD5 algorithm.
        # NOTE: this whitebox test depends on the hmac class internals
        import hashlib
        h = hmac.HMAC(b"key")
        self.assertEqual(h.digest_cons, hashlib.md5)

    def test_exercise_all_methods(self):
        # Exercising all methods once.
        # This must not raise any exceptions
        try:
            h = hmac.HMAC(b"my secret key")
            h.update(b"compute the hash of this text!")
            dig = h.digest()
            dig = h.hexdigest()
            h2 = h.copy()
        except:
            self.fail("Exception raised during normal usage of HMAC class.")

class CopyTestCase(unittest.TestCase):

    def test_attributes(self):
        # Testing if attributes are of same type.
        h1 = hmac.HMAC(b"key")
        h2 = h1.copy()
        self.failUnless(h1.digest_cons == h2.digest_cons,
            "digest constructors don't match.")
        self.assertEqual(type(h1.inner), type(h2.inner),
            "Types of inner don't match.")
        self.assertEqual(type(h1.outer), type(h2.outer),
            "Types of outer don't match.")

    def test_realcopy(self):
        # Testing if the copy method created a real copy.
        h1 = hmac.HMAC(b"key")
        h2 = h1.copy()
        # Using id() in case somebody has overridden __cmp__.
        self.failUnless(id(h1) != id(h2), "No real copy of the HMAC instance.")
        self.failUnless(id(h1.inner) != id(h2.inner),
            "No real copy of the attribute 'inner'.")
        self.failUnless(id(h1.outer) != id(h2.outer),
            "No real copy of the attribute 'outer'.")

    def test_equality(self):
        # Testing if the copy has the same digests.
        h1 = hmac.HMAC(b"key")
        h1.update(b"some random text")
        h2 = h1.copy()
        self.assertEqual(h1.digest(), h2.digest(),
            "Digest of copy doesn't match original digest.")
        self.assertEqual(h1.hexdigest(), h2.hexdigest(),
            "Hexdigest of copy doesn't match original hexdigest.")

def test_main():
    test_support.run_unittest(
        TestVectorsTestCase,
        ConstructorTestCase,
        SanityTestCase,
        CopyTestCase
    )

if __name__ == "__main__":
    test_main()
