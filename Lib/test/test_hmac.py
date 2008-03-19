import hmac
import hashlib
import unittest
import warnings
from test import test_support

class TestVectorsTestCase(unittest.TestCase):

    def test_md5_vectors(self):
        # Test the HMAC module against test vectors from the RFC.

        def md5test(key, data, digest):
            h = hmac.HMAC(key, data)
            self.assertEqual(h.hexdigest().upper(), digest.upper())

        md5test(chr(0x0b) * 16,
                "Hi There",
                "9294727A3638BB1C13F48EF8158BFC9D")

        md5test("Jefe",
                "what do ya want for nothing?",
                "750c783e6ab0b503eaa86e310a5db738")

        md5test(chr(0xAA)*16,
                chr(0xDD)*50,
                "56be34521d144c88dbb8c733f0e8b3f6")

        md5test("".join([chr(i) for i in range(1, 26)]),
                chr(0xCD) * 50,
                "697eaf0aca3a3aea3a75164746ffaa79")

        md5test(chr(0x0C) * 16,
                "Test With Truncation",
                "56461ef2342edc00f9bab995690efd4c")

        md5test(chr(0xAA) * 80,
                "Test Using Larger Than Block-Size Key - Hash Key First",
                "6b1ab7fe4bd7bf8f0b62e6ce61b9d0cd")

        md5test(chr(0xAA) * 80,
                ("Test Using Larger Than Block-Size Key "
                 "and Larger Than One Block-Size Data"),
                "6f630fad67cda0ee1fb1f562db3aa53e")

    def test_sha_vectors(self):
        def shatest(key, data, digest):
            h = hmac.HMAC(key, data, digestmod=hashlib.sha1)
            self.assertEqual(h.hexdigest().upper(), digest.upper())

        shatest(chr(0x0b) * 20,
                "Hi There",
                "b617318655057264e28bc0b6fb378c8ef146be00")

        shatest("Jefe",
                "what do ya want for nothing?",
                "effcdf6ae5eb2fa2d27416d5f184df9c259a7c79")

        shatest(chr(0xAA)*20,
                chr(0xDD)*50,
                "125d7342b9ac11cd91a39af48aa17b4f63f175d3")

        shatest("".join([chr(i) for i in range(1, 26)]),
                chr(0xCD) * 50,
                "4c9007f4026250c6bc8414f9bf50c86c2d7235da")

        shatest(chr(0x0C) * 20,
                "Test With Truncation",
                "4c1a03424b55e07fe7f27be1d58bb9324a9a5a04")

        shatest(chr(0xAA) * 80,
                "Test Using Larger Than Block-Size Key - Hash Key First",
                "aa4ae5e15272d00e95705637ce8a3b55ed402112")

        shatest(chr(0xAA) * 80,
                ("Test Using Larger Than Block-Size Key "
                 "and Larger Than One Block-Size Data"),
                "e8e99d0f45237d786d6bbaa7965c7808bbff1a91")

    def _rfc4231_test_cases(self, hashfunc):
        def hmactest(key, data, hexdigests):
            h = hmac.HMAC(key, data, digestmod=hashfunc)
            self.assertEqual(h.hexdigest().lower(), hexdigests[hashfunc])

        # 4.2.  Test Case 1
        hmactest(key = '\x0b'*20,
                 data = 'Hi There',
                 hexdigests = {
                   hashlib.sha224: '896fb1128abbdf196832107cd49df33f'
                                   '47b4b1169912ba4f53684b22',
                   hashlib.sha256: 'b0344c61d8db38535ca8afceaf0bf12b'
                                   '881dc200c9833da726e9376c2e32cff7',
                   hashlib.sha384: 'afd03944d84895626b0825f4ab46907f'
                                   '15f9dadbe4101ec682aa034c7cebc59c'
                                   'faea9ea9076ede7f4af152e8b2fa9cb6',
                   hashlib.sha512: '87aa7cdea5ef619d4ff0b4241a1d6cb0'
                                   '2379f4e2ce4ec2787ad0b30545e17cde'
                                   'daa833b7d6b8a702038b274eaea3f4e4'
                                   'be9d914eeb61f1702e696c203a126854',
                 })

        # 4.3.  Test Case 2
        hmactest(key = 'Jefe',
                 data = 'what do ya want for nothing?',
                 hexdigests = {
                   hashlib.sha224: 'a30e01098bc6dbbf45690f3a7e9e6d0f'
                                   '8bbea2a39e6148008fd05e44',
                   hashlib.sha256: '5bdcc146bf60754e6a042426089575c7'
                                   '5a003f089d2739839dec58b964ec3843',
                   hashlib.sha384: 'af45d2e376484031617f78d2b58a6b1b'
                                   '9c7ef464f5a01b47e42ec3736322445e'
                                   '8e2240ca5e69e2c78b3239ecfab21649',
                   hashlib.sha512: '164b7a7bfcf819e2e395fbe73b56e0a3'
                                   '87bd64222e831fd610270cd7ea250554'
                                   '9758bf75c05a994a6d034f65f8f0e6fd'
                                   'caeab1a34d4a6b4b636e070a38bce737',
                 })

        # 4.4.  Test Case 3
        hmactest(key = '\xaa'*20,
                 data = '\xdd'*50,
                 hexdigests = {
                   hashlib.sha224: '7fb3cb3588c6c1f6ffa9694d7d6ad264'
                                   '9365b0c1f65d69d1ec8333ea',
                   hashlib.sha256: '773ea91e36800e46854db8ebd09181a7'
                                   '2959098b3ef8c122d9635514ced565fe',
                   hashlib.sha384: '88062608d3e6ad8a0aa2ace014c8a86f'
                                   '0aa635d947ac9febe83ef4e55966144b'
                                   '2a5ab39dc13814b94e3ab6e101a34f27',
                   hashlib.sha512: 'fa73b0089d56a284efb0f0756c890be9'
                                   'b1b5dbdd8ee81a3655f83e33b2279d39'
                                   'bf3e848279a722c806b485a47e67c807'
                                   'b946a337bee8942674278859e13292fb',
                 })

        # 4.5.  Test Case 4
        hmactest(key = ''.join([chr(x) for x in xrange(0x01, 0x19+1)]),
                 data = '\xcd'*50,
                 hexdigests = {
                   hashlib.sha224: '6c11506874013cac6a2abc1bb382627c'
                                   'ec6a90d86efc012de7afec5a',
                   hashlib.sha256: '82558a389a443c0ea4cc819899f2083a'
                                   '85f0faa3e578f8077a2e3ff46729665b',
                   hashlib.sha384: '3e8a69b7783c25851933ab6290af6ca7'
                                   '7a9981480850009cc5577c6e1f573b4e'
                                   '6801dd23c4a7d679ccf8a386c674cffb',
                   hashlib.sha512: 'b0ba465637458c6990e5a8c5f61d4af7'
                                   'e576d97ff94b872de76f8050361ee3db'
                                   'a91ca5c11aa25eb4d679275cc5788063'
                                   'a5f19741120c4f2de2adebeb10a298dd',
                 })

        # 4.7.  Test Case 6
        hmactest(key = '\xaa'*131,
                 data = 'Test Using Larger Than Block-Siz'
                        'e Key - Hash Key First',
                 hexdigests = {
                   hashlib.sha224: '95e9a0db962095adaebe9b2d6f0dbce2'
                                   'd499f112f2d2b7273fa6870e',
                   hashlib.sha256: '60e431591ee0b67f0d8a26aacbf5b77f'
                                   '8e0bc6213728c5140546040f0ee37f54',
                   hashlib.sha384: '4ece084485813e9088d2c63a041bc5b4'
                                   '4f9ef1012a2b588f3cd11f05033ac4c6'
                                   '0c2ef6ab4030fe8296248df163f44952',
                   hashlib.sha512: '80b24263c7c1a3ebb71493c1dd7be8b4'
                                   '9b46d1f41b4aeec1121b013783f8f352'
                                   '6b56d037e05f2598bd0fd2215d6a1e52'
                                   '95e64f73f63f0aec8b915a985d786598',
                 })

        # 4.8.  Test Case 7
        hmactest(key = '\xaa'*131,
                 data = 'This is a test using a larger th'
                        'an block-size key and a larger t'
                        'han block-size data. The key nee'
                        'ds to be hashed before being use'
                        'd by the HMAC algorithm.',
                 hexdigests = {
                   hashlib.sha224: '3a854166ac5d9f023f54d517d0b39dbd'
                                   '946770db9c2b95c9f6f565d1',
                   hashlib.sha256: '9b09ffa71b942fcb27635fbcd5b0e944'
                                   'bfdc63644f0713938a7f51535c3a35e2',
                   hashlib.sha384: '6617178e941f020d351e2f254e8fd32c'
                                   '602420feb0b8fb9adccebb82461e99c5'
                                   'a678cc31e799176d3860e6110c46523e',
                   hashlib.sha512: 'e37b6a775dc87dbaa4dfa9f96e5e3ffd'
                                   'debd71f8867289865df5a32d20cdc944'
                                   'b6022cac3c4982b10d5eeb55c3e4de15'
                                   '134676fb6de0446065c97440fa8c6a58',
                 })

    def test_sha224_rfc4231(self):
        self._rfc4231_test_cases(hashlib.sha224)

    def test_sha256_rfc4231(self):
        self._rfc4231_test_cases(hashlib.sha256)

    def test_sha384_rfc4231(self):
        self._rfc4231_test_cases(hashlib.sha384)

    def test_sha512_rfc4231(self):
        self._rfc4231_test_cases(hashlib.sha512)

    def test_legacy_block_size_warnings(self):
        class MockCrazyHash(object):
            """Ain't no block_size attribute here."""
            def __init__(self, *args):
                self._x = hashlib.sha1(*args)
                self.digest_size = self._x.digest_size
            def update(self, v):
                self._x.update(v)
            def digest(self):
                return self._x.digest()

        with test_support.catch_warning():
            warnings.simplefilter('error', RuntimeWarning)
            try:
                hmac.HMAC('a', 'b', digestmod=MockCrazyHash)
            except RuntimeWarning:
                pass
            else:
                self.fail('Expected warning about missing block_size')

            MockCrazyHash.block_size = 1
            try:
                hmac.HMAC('a', 'b', digestmod=MockCrazyHash)
            except RuntimeWarning:
                pass
            else:
                self.fail('Expected warning about small block_size')



class ConstructorTestCase(unittest.TestCase):

    def test_normal(self):
        # Standard constructor call.
        failed = 0
        try:
            h = hmac.HMAC("key")
        except:
            self.fail("Standard constructor call raised exception.")

    def test_withtext(self):
        # Constructor call with text.
        try:
            h = hmac.HMAC("key", "hash this!")
        except:
            self.fail("Constructor call with text argument raised exception.")

    def test_withmodule(self):
        # Constructor call with text and digest module.
        try:
            h = hmac.HMAC("key", "", hashlib.sha1)
        except:
            self.fail("Constructor call with hashlib.sha1 raised exception.")

class SanityTestCase(unittest.TestCase):

    def test_default_is_md5(self):
        # Testing if HMAC defaults to MD5 algorithm.
        # NOTE: this whitebox test depends on the hmac class internals
        h = hmac.HMAC("key")
        self.failUnless(h.digest_cons == hashlib.md5)

    def test_exercise_all_methods(self):
        # Exercising all methods once.
        # This must not raise any exceptions
        try:
            h = hmac.HMAC("my secret key")
            h.update("compute the hash of this text!")
            dig = h.digest()
            dig = h.hexdigest()
            h2 = h.copy()
        except:
            self.fail("Exception raised during normal usage of HMAC class.")

class CopyTestCase(unittest.TestCase):

    def test_attributes(self):
        # Testing if attributes are of same type.
        h1 = hmac.HMAC("key")
        h2 = h1.copy()
        self.failUnless(h1.digest_cons == h2.digest_cons,
            "digest constructors don't match.")
        self.failUnless(type(h1.inner) == type(h2.inner),
            "Types of inner don't match.")
        self.failUnless(type(h1.outer) == type(h2.outer),
            "Types of outer don't match.")

    def test_realcopy(self):
        # Testing if the copy method created a real copy.
        h1 = hmac.HMAC("key")
        h2 = h1.copy()
        # Using id() in case somebody has overridden __cmp__.
        self.failUnless(id(h1) != id(h2), "No real copy of the HMAC instance.")
        self.failUnless(id(h1.inner) != id(h2.inner),
            "No real copy of the attribute 'inner'.")
        self.failUnless(id(h1.outer) != id(h2.outer),
            "No real copy of the attribute 'outer'.")

    def test_equality(self):
        # Testing if the copy has the same digests.
        h1 = hmac.HMAC("key")
        h1.update("some random text")
        h2 = h1.copy()
        self.failUnless(h1.digest() == h2.digest(),
            "Digest of copy doesn't match original digest.")
        self.failUnless(h1.hexdigest() == h2.hexdigest(),
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
