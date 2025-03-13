import binascii
import functools
import hmac
import hashlib
import random
import test.support.hashlib_helper as hashlib_helper
import unittest
import unittest.mock
import warnings
from _operator import _compare_digest as operator_compare_digest
from test.support import check_disallow_instantiation
from test.support.import_helper import import_fresh_module

try:
    import _hashlib
    from _hashlib import compare_digest as openssl_compare_digest
except ImportError:
    _hashlib = None
    openssl_compare_digest = None

try:
    import _sha2 as sha2
except ImportError:
    sha2 = None


def requires_builtin_sha2():
    return unittest.skipIf(sha2 is None, "requires _sha2")


class ModuleMixin:
    """Mixin with a HMAC module implementation."""

    hmac = None


class PyModuleMixin(ModuleMixin):
    """Pure Python implementation of HMAC.

    The underlying hash functions may be OpenSSL-based or HACL* based,
    depending on whether OpenSSL is present or not.
    """

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.hmac = import_fresh_module('hmac', blocked=['_hashlib', '_hmac'])


@unittest.skip("no builtin implementation for HMAC for now")
class BuiltinModuleMixin(ModuleMixin):
    """Built-in HACL* implementation of HMAC."""

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.hmac = import_fresh_module('_hmac')


class CreatorMixin:
    """Mixin exposing a method creating a HMAC object."""

    def hmac_new(self, key, msg=None, digestmod=None):
        """Create a new HMAC object."""
        raise NotImplementedError

    def bind_hmac_new(self, digestmod):
        """Return a specialization of hmac_new() with a bound digestmod."""
        return functools.partial(self.hmac_new, digestmod=digestmod)


class DigestMixin:
    """Mixin exposing a method computing a HMAC digest."""

    def hmac_digest(self, key, msg=None, digestmod=None):
        """Compute a HMAC digest."""
        raise NotImplementedError

    def bind_hmac_digest(self, digestmod):
        """Return a specialization of hmac_digest() with a bound digestmod."""
        return functools.partial(self.hmac_digest, digestmod=digestmod)


class ThroughObjectMixin(ModuleMixin, CreatorMixin, DigestMixin):
    """Mixin delegating to <module>.HMAC() and <module>.HMAC(...).digest().

    Both the C implementation and the Python implementation of HMAC should
    expose a HMAC class with the same functionalities.
    """

    def hmac_new(self, key, msg=None, digestmod=None):
        """Create a HMAC object via a module-level class constructor."""
        return self.hmac.HMAC(key, msg, digestmod=digestmod)

    def hmac_digest(self, key, msg=None, digestmod=None):
        """Call the digest() method on a HMAC object obtained by hmac_new()."""
        return self.hmac_new(key, msg, digestmod).digest()


class ThroughModuleAPIMixin(ModuleMixin, CreatorMixin, DigestMixin):
    """Mixin delegating to <module>.new() and <module>.digest()."""

    def hmac_new(self, key, msg=None, digestmod=None):
        """Create a HMAC object via a module-level function."""
        return self.hmac.new(key, msg, digestmod=digestmod)

    def hmac_digest(self, key, msg=None, digestmod=None):
        """One-shot HMAC digest computation."""
        return self.hmac.digest(key, msg, digest=digestmod)


@hashlib_helper.requires_hashlib()
class ThroughOpenSSLAPIMixin(CreatorMixin, DigestMixin):
    """Mixin delegating to _hashlib.hmac_new() and _hashlib.hmac_digest()."""

    def hmac_new(self, key, msg=None, digestmod=None):
        return _hashlib.hmac_new(key, msg, digestmod=digestmod)

    def hmac_digest(self, key, msg=None, digestmod=None):
        return _hashlib.hmac_digest(key, msg, digest=digestmod)


class CheckerMixin:
    """Mixin for checking HMAC objects (pure Python, OpenSSL or built-in)."""

    def check_object(self, h, hexdigest, hashname, digest_size, block_size):
        """Check a HMAC object 'h' against the given values."""
        self.check_internals(h, hashname, digest_size, block_size)
        self.check_hexdigest(h, hexdigest, digest_size)

    def check_internals(self, h, hashname, digest_size, block_size):
        """Check the constant attributes of a HMAC object."""
        self.assertEqual(h.name, f"hmac-{hashname}")
        self.assertEqual(h.digest_size, digest_size)
        self.assertEqual(h.block_size, block_size)

    def check_hexdigest(self, h, hexdigest, digest_size):
        """Check the HMAC digest of 'h' and its size."""
        self.assertEqual(len(h.digest()), digest_size)
        self.assertEqual(h.digest(), binascii.unhexlify(hexdigest))
        self.assertEqual(h.hexdigest().upper(), hexdigest.upper())


class TestVectorsMixin(CreatorMixin, DigestMixin, CheckerMixin):
    """Mixin class for all test vectors test cases."""

    def hmac_new_by_name(self, key, msg=None, hashname=None):
        """Alternative implementation of hmac_new().

        This is typically useful when one needs to test against an HMAC
        implementation which only recognizes underlying hash functions
        by their name (all HMAC implementations must at least recognize
        hash functions by their names but some may use aliases such as
        `hashlib.sha1` instead of "sha1").
        """
        self.assertIsInstance(hashname, str | None)
        return self.hmac_new(key, msg, digestmod=hashname)

    def hmac_digest_by_name(self, key, msg=None, hashname=None):
        """Alternative implementation of hmac_digest()."""
        self.assertIsInstance(hashname, str | None)
        return self.hmac_digest(key, msg, digestmod=hashname)

    def assert_hmac(
        self, key, msg, hexdigest, hashfunc, hashname, digest_size, block_size
    ):
        """Check that HMAC(key, msg) == digest.

        The 'hashfunc' and 'hashname' are used as 'digestmod' values,
        thereby allowing to test the underlying dispatching mechanism.

        Note that 'hashfunc' may be a string, a callable, or a PEP-257
        module. Note that not all HMAC implementations may recognize the
        same set of types for 'hashfunc', but they should always accept
        a hash function by its name.
        """
        if hashfunc == hashname:
            choices = [hashname]
        else:
            choices = [hashfunc, hashname]

        for digestmod in choices:
            with self.subTest(digestmod=digestmod):
                self.assert_hmac_new(
                    key, msg, hexdigest, digestmod,
                    hashname, digest_size, block_size
                )
                self.assert_hmac_hexdigest(
                    key, msg, hexdigest, digestmod, digest_size
                )
                self.assert_hmac_extra_cases(
                    key, msg, hexdigest, digestmod,
                    hashname, digest_size, block_size
                )

        self.assert_hmac_new_by_name(
            key, msg, hexdigest, hashname, digest_size, block_size
        )
        self.assert_hmac_hexdigest_by_new(
            key, msg, hexdigest, hashname, digest_size
        )

    def assert_hmac_new(
        self, key, msg, hexdigest, digestmod, hashname, digest_size, block_size
    ):
        """Check that HMAC(key, msg) == digest.

        This test uses the `hmac_new()` method to create HMAC objects.
        """
        self._check_hmac_new(
            key, msg, hexdigest, hashname, digest_size, block_size,
            hmac_new_func=self.hmac_new,
            hmac_new_kwds={'digestmod': digestmod},
        )

    def assert_hmac_new_by_name(
        self, key, msg, hexdigest, hashname, digest_size, block_size
    ):
        """Check that HMAC(key, msg) == digest.

        This test uses the `hmac_new_by_name()` method to create HMAC objects.
        """
        self._check_hmac_new(
            key, msg, hexdigest, hashname, digest_size, block_size,
            hmac_new_func=self.hmac_new_by_name,
            hmac_new_kwds={'hashname': hashname},
        )

    def _check_hmac_new(
        self, key, msg, hexdigest, hashname, digest_size, block_size,
        hmac_new_func, hmac_new_kwds,
    ):
        """Check that HMAC(key, msg) == digest.

        This also tests that using an empty/None initial message and
        then calling `h.update(msg)` produces the same result, namely
        that HMAC(key, msg) is equivalent to HMAC(key).update(msg).
        """
        h = hmac_new_func(key, msg, **hmac_new_kwds)
        self.check_object(h, hexdigest, hashname, digest_size, block_size)

        def hmac_new_feed(*args):
            h = hmac_new_func(key, *args, **hmac_new_kwds)
            h.update(msg)
            self.check_hexdigest(h, hexdigest, digest_size)

        with self.subTest('no initial message'):
            hmac_new_feed()
        with self.subTest('initial message is empty'):
            hmac_new_feed(b'')
        with self.subTest('initial message is None'):
            hmac_new_feed(None)

    def assert_hmac_hexdigest(
        self, key, msg, hexdigest, digestmod, digest_size,
    ):
        """Check a HMAC digest computed by hmac_digest()."""
        d = self.hmac_digest(key, msg, digestmod=digestmod)
        self.assertEqual(len(d), digest_size)
        self.assertEqual(d, binascii.unhexlify(hexdigest))

    def assert_hmac_hexdigest_by_new(
        self, key, msg, hexdigest, hashname, digest_size
    ):
        """Check a HMAC digest computed by hmac_digest_by_name()."""
        self.assertIsInstance(hashname, str | None)
        d = self.hmac_digest_by_name(key, msg, hashname=hashname)
        self.assertEqual(len(d), digest_size)
        self.assertEqual(d, binascii.unhexlify(hexdigest))

    def assert_hmac_extra_cases(
        self, key, msg, hexdigest, digestmod, hashname, digest_size, block_size
    ):
        """Extra tests that can be added in subclasses."""
        h1 = self.hmac_new_by_name(key, hashname=hashname)
        h2 = h1.copy()
        h2.update(b"test update should not affect original")
        h1.update(msg)
        self.check_object(h1, hexdigest, hashname, digest_size, block_size)


class PyTestVectorsMixin(PyModuleMixin, TestVectorsMixin):

    def assert_hmac_extra_cases(
        self, key, msg, hexdigest, digestmod, hashname, digest_size, block_size
    ):
        super().assert_hmac_extra_cases(
            key, msg, hexdigest, digestmod, hashname, digest_size, block_size
        )

        h = self.hmac.HMAC.__new__(self.hmac.HMAC)
        h._init_old(key, msg, digestmod=digestmod)
        self.check_object(h, hexdigest, hashname, digest_size, block_size)


class OpenSSLTestVectorsMixin(TestVectorsMixin):

    def hmac_new(self, key, msg=None, digestmod=None):
        return _hashlib.hmac_new(key, msg, digestmod=digestmod)

    def hmac_digest(self, key, msg=None, digestmod=None):
        return _hashlib.hmac_digest(key, msg, digest=digestmod)

    def hmac_new_by_name(self, key, msg=None, hashname=None):
        # ignore 'digestmod' and use the exact openssl function
        openssl_func = getattr(_hashlib, f"openssl_{hashname}")
        return self.hmac_new(key, msg, digestmod=openssl_func)

    def hmac_digest_by_name(self, key, msg=None, hashname=None):
        openssl_func = getattr(_hashlib, f"openssl_{hashname}")
        return self.hmac_digest(key, msg, digestmod=openssl_func)


class RFCTestCasesMixin(TestVectorsMixin):
    """Test HMAC implementations against test vectors from the RFC.

    Subclasses must override the 'md5' and other 'sha*' attributes
    to test the implementations. Their value can be a string, a callable,
    or a PEP-257 module.
    """

    ALGORITHMS = [
        'md5', 'sha1',
        'sha224', 'sha256', 'sha384', 'sha512',
    ]

    # Those will be automatically set to non-None on subclasses
    # as they are set by __init_subclass()__.
    md5 = sha1 = sha224 = sha256 = sha384 = sha512 = None

    def __init_subclass__(cls, *args, **kwargs):
        super().__init_subclass__(*args, **kwargs)
        for name in cls.ALGORITHMS:
            setattr(cls, name, name)

    def test_md5(self):
        def md5test(key, msg, hexdigest):
            self.assert_hmac(key, msg, hexdigest, self.md5, "md5", 16, 64)

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

    def test_sha1(self):
        def shatest(key, msg, hexdigest):
            self.assert_hmac(key, msg, hexdigest, self.sha1, "sha1", 20, 64)

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

    def test_sha2_224_rfc4231(self):
        self._test_sha2_rfc4231(self.sha224, 'sha224', 28, 64)

    def test_sha2_256_rfc4231(self):
        self._test_sha2_rfc4231(self.sha256, 'sha256', 32, 64)

    def test_sha2_384_rfc4231(self):
        self._test_sha2_rfc4231(self.sha384, 'sha384', 48, 128)

    def test_sha2_512_rfc4231(self):
        self._test_sha2_rfc4231(self.sha512, 'sha512', 64, 128)

    def _test_sha2_rfc4231(self, hashfunc, hashname, digest_size, block_size):

        def hmactest(key, data, hexdigests):
            hexdigest = hexdigests[hashname]

            self.assert_hmac(
                key, data, hexdigest,
                hashfunc=hashfunc,
                hashname=hashname,
                digest_size=digest_size,
                block_size=block_size
            )

        # 4.2.  Test Case 1
        hmactest(key=b'\x0b' * 20,
                 data=b'Hi There',
                 hexdigests={
                     'sha224': '896fb1128abbdf196832107cd49df33f'
                               '47b4b1169912ba4f53684b22',
                     'sha256': 'b0344c61d8db38535ca8afceaf0bf12b'
                               '881dc200c9833da726e9376c2e32cff7',
                     'sha384': 'afd03944d84895626b0825f4ab46907f'
                               '15f9dadbe4101ec682aa034c7cebc59c'
                               'faea9ea9076ede7f4af152e8b2fa9cb6',
                     'sha512': '87aa7cdea5ef619d4ff0b4241a1d6cb0'
                               '2379f4e2ce4ec2787ad0b30545e17cde'
                               'daa833b7d6b8a702038b274eaea3f4e4'
                               'be9d914eeb61f1702e696c203a126854',
                 })

        # 4.3.  Test Case 2
        hmactest(key=b'Jefe',
                 data=b'what do ya want for nothing?',
                 hexdigests={
                     'sha224': 'a30e01098bc6dbbf45690f3a7e9e6d0f'
                               '8bbea2a39e6148008fd05e44',
                     'sha256': '5bdcc146bf60754e6a042426089575c7'
                               '5a003f089d2739839dec58b964ec3843',
                     'sha384': 'af45d2e376484031617f78d2b58a6b1b'
                               '9c7ef464f5a01b47e42ec3736322445e'
                               '8e2240ca5e69e2c78b3239ecfab21649',
                     'sha512': '164b7a7bfcf819e2e395fbe73b56e0a3'
                               '87bd64222e831fd610270cd7ea250554'
                               '9758bf75c05a994a6d034f65f8f0e6fd'
                               'caeab1a34d4a6b4b636e070a38bce737',
                 })

        # 4.4.  Test Case 3
        hmactest(key=b'\xaa' * 20,
                 data=b'\xdd' * 50,
                 hexdigests={
                     'sha224': '7fb3cb3588c6c1f6ffa9694d7d6ad264'
                               '9365b0c1f65d69d1ec8333ea',
                     'sha256': '773ea91e36800e46854db8ebd09181a7'
                               '2959098b3ef8c122d9635514ced565fe',
                     'sha384': '88062608d3e6ad8a0aa2ace014c8a86f'
                               '0aa635d947ac9febe83ef4e55966144b'
                               '2a5ab39dc13814b94e3ab6e101a34f27',
                     'sha512': 'fa73b0089d56a284efb0f0756c890be9'
                               'b1b5dbdd8ee81a3655f83e33b2279d39'
                               'bf3e848279a722c806b485a47e67c807'
                               'b946a337bee8942674278859e13292fb',
                 })

        # 4.5.  Test Case 4
        hmactest(key=bytes(x for x in range(0x01, 0x19 + 1)),
                 data=b'\xcd' * 50,
                 hexdigests={
                     'sha224': '6c11506874013cac6a2abc1bb382627c'
                               'ec6a90d86efc012de7afec5a',
                     'sha256': '82558a389a443c0ea4cc819899f2083a'
                               '85f0faa3e578f8077a2e3ff46729665b',
                     'sha384': '3e8a69b7783c25851933ab6290af6ca7'
                               '7a9981480850009cc5577c6e1f573b4e'
                               '6801dd23c4a7d679ccf8a386c674cffb',
                     'sha512': 'b0ba465637458c6990e5a8c5f61d4af7'
                               'e576d97ff94b872de76f8050361ee3db'
                               'a91ca5c11aa25eb4d679275cc5788063'
                               'a5f19741120c4f2de2adebeb10a298dd',
                 })

        # 4.7.  Test Case 6
        hmactest(key=b'\xaa' * 131,
                 data=b'Test Using Larger Than Block-Siz'
                      b'e Key - Hash Key First',
                 hexdigests={
                     'sha224': '95e9a0db962095adaebe9b2d6f0dbce2'
                               'd499f112f2d2b7273fa6870e',
                     'sha256': '60e431591ee0b67f0d8a26aacbf5b77f'
                               '8e0bc6213728c5140546040f0ee37f54',
                     'sha384': '4ece084485813e9088d2c63a041bc5b4'
                               '4f9ef1012a2b588f3cd11f05033ac4c6'
                               '0c2ef6ab4030fe8296248df163f44952',
                     'sha512': '80b24263c7c1a3ebb71493c1dd7be8b4'
                               '9b46d1f41b4aeec1121b013783f8f352'
                               '6b56d037e05f2598bd0fd2215d6a1e52'
                               '95e64f73f63f0aec8b915a985d786598',
                 })

        # 4.8.  Test Case 7
        hmactest(key=b'\xaa' * 131,
                 data=b'This is a test using a larger th'
                      b'an block-size key and a larger t'
                      b'han block-size data. The key nee'
                      b'ds to be hashed before being use'
                      b'd by the HMAC algorithm.',
                 hexdigests={
                     'sha224': '3a854166ac5d9f023f54d517d0b39dbd'
                               '946770db9c2b95c9f6f565d1',
                     'sha256': '9b09ffa71b942fcb27635fbcd5b0e944'
                               'bfdc63644f0713938a7f51535c3a35e2',
                     'sha384': '6617178e941f020d351e2f254e8fd32c'
                               '602420feb0b8fb9adccebb82461e99c5'
                               'a678cc31e799176d3860e6110c46523e',
                     'sha512': 'e37b6a775dc87dbaa4dfa9f96e5e3ffd'
                               'debd71f8867289865df5a32d20cdc944'
                               'b6022cac3c4982b10d5eeb55c3e4de15'
                               '134676fb6de0446065c97440fa8c6a58',
                 })

    @hashlib_helper.requires_hashdigest('sha256')
    def test_legacy_block_size_warnings(self):
        class MockCrazyHash(object):
            """Ain't no block_size attribute here."""
            def __init__(self, *args):
                self._x = hashlib.sha256(*args)
                self.digest_size = self._x.digest_size
            def update(self, v):
                self._x.update(v)
            def digest(self):
                return self._x.digest()

        with warnings.catch_warnings():
            warnings.simplefilter('error', RuntimeWarning)
            with self.assertRaises(RuntimeWarning):
                hmac.HMAC(b'a', b'b', digestmod=MockCrazyHash)
                self.fail('Expected warning about missing block_size')

            MockCrazyHash.block_size = 1
            with self.assertRaises(RuntimeWarning):
                hmac.HMAC(b'a', b'b', digestmod=MockCrazyHash)
                self.fail('Expected warning about small block_size')

    def test_with_fallback(self):
        cache = getattr(hashlib, '__builtin_constructor_cache')
        try:
            cache['foo'] = hashlib.sha256
            hexdigest = hmac.digest(b'key', b'message', 'foo').hex()
            expected = ('6e9ef29b75fffc5b7abae527d58fdadb'
                        '2fe42e7219011976917343065f58ed4a')
            self.assertEqual(hexdigest, expected)
        finally:
            cache.pop('foo')


class RFCWithOpenSSLHashFunctionTestCasesMixin(RFCTestCasesMixin):

    def __init_subclass__(cls, *args, **kwargs):
        super().__init_subclass__(*args, **kwargs)

        for name in cls.ALGORITHMS:
            @property
            @hashlib_helper.requires_hashlib()
            @hashlib_helper.requires_hashdigest(name, openssl=True)
            def func(self, *, __name=name):  # __name needed to bind 'name'
                return getattr(_hashlib, f'openssl_{__name}')
            setattr(cls, name, func)


class PyRFCTestCase(PyTestVectorsMixin, ThroughObjectMixin,
                    RFCWithOpenSSLHashFunctionTestCasesMixin,
                    unittest.TestCase):
    """Python implementation of HMAC using hmac.HMAC().

    The underlying hash functions are OpenSSL-based.
    """


class PyDotNewRFCTestCase(PyTestVectorsMixin, ThroughModuleAPIMixin,
                          RFCWithOpenSSLHashFunctionTestCasesMixin,
                          unittest.TestCase):
    """Python implementation of HMAC using hmac.new().

    The underlying hash functions are OpenSSL-based.
    """


class OpenSSLRFCTestCase(OpenSSLTestVectorsMixin,
                         RFCWithOpenSSLHashFunctionTestCasesMixin,
                         unittest.TestCase):
    """OpenSSL implementation of HMAC.

    The underlying hash functions are also OpenSSL-based."""


# TODO(picnixz): once we have a HACL* HMAC, we should also test the Python
# implementation of HMAC with a HACL*-based hash function. For now, we only
# test it partially via the '_sha2' module, but for completeness we could
# also test the RFC test vectors against all possible implementations.


class DigestModTestCaseMixin(CreatorMixin, DigestMixin):
    """Tests for the 'digestmod' parameter."""

    def assert_raises_missing_digestmod(self):
        """A context manager catching errors when a digestmod is missing."""
        return self.assertRaisesRegex(TypeError, "Missing required.*digestmod")

    def assert_raises_unknown_digestmod(self):
        """A context manager catching errors when a digestmod is unknown."""
        return self.assertRaisesRegex(ValueError, "[Uu]nsupported.*")

    def test_constructor_missing_digestmod(self):
        catcher = self.assert_raises_missing_digestmod
        self.do_test_constructor_missing_digestmod(catcher)

    def test_constructor_unknown_digestmod(self):
        catcher = self.assert_raises_unknown_digestmod
        self.do_test_constructor_unknown_digestmod(catcher)

    def do_test_constructor_missing_digestmod(self, catcher):
        for func, args, kwds in self.cases_missing_digestmod_in_constructor():
            with self.subTest(args=args, kwds=kwds), catcher():
                func(*args, **kwds)

    def do_test_constructor_unknown_digestmod(self, catcher):
        for func, args, kwds in self.cases_unknown_digestmod_in_constructor():
            with self.subTest(args=args, kwds=kwds), catcher():
                func(*args, **kwds)

    def cases_missing_digestmod_in_constructor(self):
        raise NotImplementedError

    def make_missing_digestmod_cases(self, func, choices):
        """Generate cases for missing digestmod tests."""
        key, msg = b'unused key', b'unused msg'
        cases = self._invalid_digestmod_cases(func, key, msg, choices)
        return [(func, (key,), {}), (func, (key, msg), {})] + cases

    def cases_unknown_digestmod_in_constructor(self):
        raise NotImplementedError

    def make_unknown_digestmod_cases(self, func, choices):
        """Generate cases for unknown digestmod tests."""
        key, msg = b'unused key', b'unused msg'
        return self._invalid_digestmod_cases(func, key, msg, choices)

    def _invalid_digestmod_cases(self, func, key, msg, choices):
        cases = []
        for digestmod in choices:
            kwargs = {'digestmod': digestmod}
            cases.append((func, (key,), kwargs))
            cases.append((func, (key, msg), kwargs))
            cases.append((func, (key,), kwargs | {'msg': msg}))
        return cases


class ConstructorTestCaseMixin(CreatorMixin, DigestMixin, CheckerMixin):
    """HMAC constructor tests based on HMAC-SHA-2/256."""

    key = b"key"
    msg = b"hash this!"
    res = "6c845b47f52b3b47f6590c502db7825aad757bf4fadc8fa972f7cd2e76a5bdeb"

    def do_test_constructor(self, hmac_on_key_and_msg):
        self.do_test_constructor_invalid_types(hmac_on_key_and_msg)
        self.do_test_constructor_supported_types(hmac_on_key_and_msg)

    def do_test_constructor_invalid_types(self, hmac_on_key_and_msg):
        self.assertRaises(TypeError, hmac_on_key_and_msg, 1)
        self.assertRaises(TypeError, hmac_on_key_and_msg, "key")

        self.assertRaises(TypeError, hmac_on_key_and_msg, b"key", 1)
        self.assertRaises(TypeError, hmac_on_key_and_msg, b"key", "msg")

    def do_test_constructor_supported_types(self, hmac_on_key_and_msg):
        for tp_key in [bytes, bytearray]:
            for tp_msg in [bytes, bytearray, memoryview]:
                with self.subTest(tp_key=tp_key, tp_msg=tp_msg):
                    h = hmac_on_key_and_msg(tp_key(self.key), tp_msg(self.msg))
                    self.assertEqual(h.name, "hmac-sha256")
                    self.assertEqual(h.hexdigest(), self.res)

    @hashlib_helper.requires_hashdigest("sha256")
    def test_constructor(self):
        self.do_test_constructor(self.bind_hmac_new("sha256"))

    @hashlib_helper.requires_hashdigest("sha256")
    def test_digest(self):
        digest = self.hmac_digest(self.key, self.msg, "sha256")
        self.assertEqual(digest, binascii.unhexlify(self.res))


class PyConstructorBaseMixin(PyModuleMixin,
                             DigestModTestCaseMixin,
                             ConstructorTestCaseMixin):

    def cases_missing_digestmod_in_constructor(self):
        func, choices = self.hmac_new, ['', None, False]
        return self.make_missing_digestmod_cases(func, choices)

    def cases_unknown_digestmod_in_constructor(self):
        func, choices = self.hmac_new, ['unknown']
        return self.make_unknown_digestmod_cases(func, choices)

    @requires_builtin_sha2()
    def test_constructor_with_module(self):
        self.do_test_constructor(self.bind_hmac_new(sha2.sha256))

    @requires_builtin_sha2()
    def test_digest_with_module(self):
        digest = self.hmac_digest(self.key, self.msg, sha2.sha256)
        self.assertEqual(digest, binascii.unhexlify(self.res))


class PyConstructorTestCase(ThroughObjectMixin, PyConstructorBaseMixin,
                            unittest.TestCase):
    """Test the hmac.HMAC() pure Python constructor."""


class PyModuleConstructorTestCase(ThroughModuleAPIMixin, PyConstructorBaseMixin,
                                  unittest.TestCase):
    """Test the hmac.new() and hmac.digest() functions."""

    def test_hmac_digest_digestmod_parameter(self):
        func = self.hmac_digest

        def raiser():
            raise RuntimeError("custom exception")

        with self.assertRaisesRegex(RuntimeError, "custom exception"):
            func(b'key', b'msg', raiser)

        with self.assertRaisesRegex(ValueError, 'hash type'):
            func(b'key', b'msg', 'unknown')

        with self.assertRaisesRegex(AttributeError, 'new'):
            func(b'key', b'msg', 1234)
        with self.assertRaisesRegex(AttributeError, 'new'):
            func(b'key', b'msg', None)


class ExtensionConstructorTestCaseMixin(DigestModTestCaseMixin,
                                        ConstructorTestCaseMixin):

    # The underlying C class.
    obj_type = None

    # The exact exception class raised when a 'digestmod' parameter is invalid.
    exc_type = None

    def test_internal_types(self):
        # internal C types are immutable and cannot be instantiated
        check_disallow_instantiation(self, self.obj_type)
        with self.assertRaisesRegex(TypeError, "immutable type"):
            self.obj_type.value = None

    def assert_digestmod_error(self):
        self.assertIsSubclass(self.exc_type, ValueError)
        return self.assertRaises(self.exc_type)

    def test_constructor_missing_digestmod(self):
        self.do_test_constructor_missing_digestmod(self.assert_digestmod_error)

    def test_constructor_unknown_digestmod(self):
        self.do_test_constructor_unknown_digestmod(self.assert_digestmod_error)

    def cases_missing_digestmod_in_constructor(self):
        func, choices = self.hmac_new, ['', None, False]
        return self.make_missing_digestmod_cases(func, choices)

    def cases_unknown_digestmod_in_constructor(self):
        func, choices = self.hmac_new, ['unknown', 1234]
        return self.make_unknown_digestmod_cases(func, choices)


class OpenSSLConstructorTestCase(ThroughOpenSSLAPIMixin,
                                 ExtensionConstructorTestCaseMixin,
                                 unittest.TestCase):

    @property
    def obj_type(self):
        return _hashlib.HMAC

    @property
    def exc_type(self):
        return _hashlib.UnsupportedDigestmodError

    def test_hmac_digest_digestmod_parameter(self):
        # TODO(picnixz): remove default arguments in _hashlib.hmac_digest()
        # since the return value is not a HMAC object but a bytes object.
        for value in [object, 'unknown', 1234, None]:
            with self.subTest(value=value), self.assert_digestmod_error():
                self.hmac_digest(b'key', b'msg', value)


class SanityTestCaseMixin(CreatorMixin):
    """Sanity checks for HMAC objects and their object interface.

    The tests here use a common digestname and do not check all supported
    hash functions.
    """

    # The underlying HMAC class to test. May be in C or in Python.
    hmac_class: type
    # The underlying hash function name (should be accepted by the HMAC class).
    digestname: str

    def test_methods(self):
        h = self.hmac_new(b"my secret key", digestmod=self.digestname)
        self.assertIsInstance(h, self.hmac_class)
        self.assertIsNone(h.update(b"compute the hash of this text!"))
        self.assertIsInstance(h.digest(), bytes)
        self.assertIsInstance(h.hexdigest(), str)
        self.assertIsInstance(h.copy(), self.hmac_class)

    def test_repr(self):
        # HMAC object representation may differ across implementations
        raise NotImplementedError


@hashlib_helper.requires_hashdigest('sha256')
class PySanityTestCase(ThroughObjectMixin, PyModuleMixin, SanityTestCaseMixin,
                       unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.hmac_class = cls.hmac.HMAC
        cls.digestname = 'sha256'

    def test_repr(self):
        h = self.hmac_new(b"my secret key", digestmod=self.digestname)
        self.assertStartsWith(repr(h), "<hmac.HMAC object at")


@hashlib_helper.requires_hashdigest('sha256', openssl=True)
class OpenSSLSanityTestCase(ThroughOpenSSLAPIMixin, SanityTestCaseMixin,
                            unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.hmac_class = _hashlib.HMAC
        cls.digestname = 'sha256'

    def test_repr(self):
        h = self.hmac_new(b"my secret key", digestmod=self.digestname)
        self.assertStartsWith(repr(h), f"<{self.digestname} HMAC object @")


class UpdateTestCaseMixin:
    """Tests for the update() method (streaming HMAC)."""

    def HMAC(self, key, msg=None):
        """Create a HMAC object."""
        raise NotImplementedError

    def test_update(self):
        key, msg = random.randbytes(16), random.randbytes(16)
        with self.subTest(key=key, msg=msg):
            h1 = self.HMAC(key, msg)

            h2 = self.HMAC(key)
            h2.update(msg)

            self.assertEqual(h1.digest(), h2.digest())
            self.assertEqual(h1.hexdigest(), h2.hexdigest())

    def test_update_exceptions(self):
        h = self.HMAC(b"key")
        for msg in ['invalid msg', 123, (), []]:
            with self.subTest(msg=msg):
                self.assertRaises(TypeError, h.update, msg)


@hashlib_helper.requires_hashdigest('sha256')
class PyUpdateTestCase(UpdateTestCaseMixin, unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        super().setUpClass()
        cls.hmac = import_fresh_module('hmac', blocked=['_hashlib'])

    def HMAC(self, key, msg=None):
        return self.hmac.HMAC(key, msg, digestmod='sha256')


@hashlib_helper.requires_hashlib()
@hashlib_helper.requires_hashdigest('sha256', openssl=True)
class OpenSSLUpdateTestCase(UpdateTestCaseMixin, unittest.TestCase):

    def HMAC(self, key, msg=None):
        return hmac.new(key, msg, digestmod='sha256')


@hashlib_helper.requires_hashdigest('sha256')
class CopyTestCase(unittest.TestCase):

    def test_attributes_old(self):
        # Testing if attributes are of same type.
        h1 = hmac.HMAC.__new__(hmac.HMAC)
        h1._init_old(b"key", b"msg", digestmod="sha256")
        h2 = h1.copy()
        self.assertEqual(type(h1._inner), type(h2._inner))
        self.assertEqual(type(h1._outer), type(h2._outer))

    def test_realcopy_old(self):
        # Testing if the copy method created a real copy.
        h1 = hmac.HMAC.__new__(hmac.HMAC)
        h1._init_old(b"key", b"msg", digestmod="sha256")
        self.assertIsNone(h1._hmac)

        h2 = h1.copy()
        self.assertIsNone(h2._hmac)
        # Using id() in case somebody has overridden __eq__/__ne__.
        self.assertNotEqual(id(h1), id(h2))
        self.assertNotEqual(id(h1._inner), id(h2._inner))
        self.assertNotEqual(id(h1._outer), id(h2._outer))

    @hashlib_helper.requires_hashlib()
    def test_realcopy_hmac(self):
        h1 = hmac.HMAC.__new__(hmac.HMAC)
        h1._init_hmac(b"key", b"msg", digestmod="sha256")
        h2 = h1.copy()
        self.assertNotEqual(id(h1._hmac), id(h2._hmac))

    def test_equality(self):
        # Testing if the copy has the same digests.
        h1 = hmac.HMAC(b"key", digestmod="sha256")
        h1.update(b"some random text")
        h2 = h1.copy()
        self.assertEqual(h1.digest(), h2.digest())
        self.assertEqual(h1.hexdigest(), h2.hexdigest())

    def test_equality_new(self):
        # Testing if the copy has the same digests with hmac.new().
        h1 = hmac.new(b"key", digestmod="sha256")
        h1.update(b"some random text")
        h2 = h1.copy()
        self.assertNotEqual(id(h1), id(h2))
        self.assertEqual(h1.digest(), h2.digest())
        self.assertEqual(h1.hexdigest(), h2.hexdigest())


class CompareDigestMixin:

    @staticmethod
    def compare_digest(a, b):
        """Implementation of 'a == b' to test."""
        raise NotImplementedError

    def assert_digest_equal(self, a, b):
        with self.subTest(a=a, b=b):
            self.assertTrue(self.compare_digest(a, b))
        with self.subTest(a=b, b=a):
            self.assertTrue(self.compare_digest(b, a))

    def assert_digest_not_equal(self, a, b):
        with self.subTest(a=a, b=b):
            self.assertFalse(self.compare_digest(a, b))
        with self.subTest(a=b, b=a):
            self.assertFalse(self.compare_digest(b, a))

    def test_exceptions(self):
        for a, b in [
            # Testing input type exception handling
            (100, 200), (100, b"foobar"), ("foobar", b"foobar"),
            # non-ASCII strings
            ("fooä", "fooä")
        ]:
            self.assertRaises(TypeError, self.compare_digest, a, b)
            self.assertRaises(TypeError, self.compare_digest, b, a)

    def test_bytes(self):
        # Testing bytes of different lengths
        a, b = b"foobar", b"foo"
        self.assert_digest_not_equal(a, b)
        a, b = b"\xde\xad\xbe\xef", b"\xde\xad"
        self.assert_digest_not_equal(a, b)

        # Testing bytes of same lengths, different values
        a, b = b"foobar", b"foobaz"
        self.assert_digest_not_equal(a, b)
        a, b = b"\xde\xad\xbe\xef", b"\xab\xad\x1d\xea"
        self.assert_digest_not_equal(a, b)

        # Testing bytes of same lengths, same values
        a, b = b"foobar", b"foobar"
        self.assert_digest_equal(a, b)
        a, b = b"\xde\xad\xbe\xef", b"\xde\xad\xbe\xef"
        self.assert_digest_equal(a, b)

    def test_bytearray(self):
        # Testing bytearrays of same lengths, same values
        a, b = bytearray(b"foobar"), bytearray(b"foobar")
        self.assert_digest_equal(a, b)

        # Testing bytearrays of different lengths
        a, b = bytearray(b"foobar"), bytearray(b"foo")
        self.assert_digest_not_equal(a, b)

        # Testing bytearrays of same lengths, different values
        a, b = bytearray(b"foobar"), bytearray(b"foobaz")
        self.assert_digest_not_equal(a, b)

    def test_mixed_types(self):
        # Testing byte and bytearray of same lengths, same values
        a, b = bytearray(b"foobar"), b"foobar"
        self.assert_digest_equal(a, b)

        # Testing byte bytearray of different lengths
        a, b = bytearray(b"foobar"), b"foo"
        self.assert_digest_not_equal(a, b)

        # Testing byte and bytearray of same lengths, different values
        a, b = bytearray(b"foobar"), b"foobaz"
        self.assert_digest_not_equal(a, b)

    def test_string(self):
        # Testing str of same lengths
        a, b = "foobar", "foobar"
        self.assert_digest_equal(a, b)

        # Testing str of different lengths
        a, b = "foo", "foobar"
        self.assert_digest_not_equal(a, b)

        # Testing str of same lengths, different values
        a, b = "foobar", "foobaz"
        self.assert_digest_not_equal(a, b)

    def test_string_subclass(self):
        class S(str):
            def __eq__(self, other):
                raise ValueError("should not be called")

        a, b = S("foobar"), S("foobar")
        self.assert_digest_equal(a, b)
        a, b = S("foobar"), "foobar"
        self.assert_digest_equal(a, b)
        a, b = S("foobar"), S("foobaz")
        self.assert_digest_not_equal(a, b)

    def test_bytes_subclass(self):
        class B(bytes):
            def __eq__(self, other):
                raise ValueError("should not be called")

        a, b = B(b"foobar"), B(b"foobar")
        self.assert_digest_equal(a, b)
        a, b = B(b"foobar"), b"foobar"
        self.assert_digest_equal(a, b)
        a, b = B(b"foobar"), B(b"foobaz")
        self.assert_digest_not_equal(a, b)


class HMACCompareDigestTestCase(CompareDigestMixin, unittest.TestCase):
    compare_digest = hmac.compare_digest

    def test_compare_digest_func(self):
        if openssl_compare_digest is not None:
            self.assertIs(self.compare_digest, openssl_compare_digest)
        else:
            self.assertIs(self.compare_digest, operator_compare_digest)


@hashlib_helper.requires_hashlib()
class OpenSSLCompareDigestTestCase(CompareDigestMixin, unittest.TestCase):
    compare_digest = openssl_compare_digest


class OperatorCompareDigestTestCase(CompareDigestMixin, unittest.TestCase):
    compare_digest = operator_compare_digest


if __name__ == "__main__":
    unittest.main()
