# Test the hashlib module.
#
#  Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
#  Licensed to PSF under a Contributor Agreement.
#

import array
from binascii import unhexlify
import hashlib
import importlib
import io
import itertools
import logging
import os
import re
import sys
import sysconfig
import tempfile
import threading
import unittest
from test import support
from test.support import _4G, bigmemtest
from test.support import hashlib_helper
from test.support.import_helper import import_fresh_module
from test.support import requires_resource
from test.support import threading_helper
from http.client import HTTPException


default_builtin_hashes = {'md5', 'sha1', 'sha256', 'sha512', 'sha3', 'blake2'}
# --with-builtin-hashlib-hashes override
builtin_hashes = sysconfig.get_config_var("PY_BUILTIN_HASHLIB_HASHES")
if builtin_hashes is None:
    builtin_hashes = default_builtin_hashes
else:
    builtin_hashes = {
        m.strip() for m in builtin_hashes.strip('"').lower().split(",")
    }

# hashlib with and without OpenSSL backend for PBKDF2
# only import builtin_hashlib when all builtin hashes are available.
# Otherwise import prints noise on stderr
openssl_hashlib = import_fresh_module('hashlib', fresh=['_hashlib'])
if builtin_hashes == default_builtin_hashes:
    builtin_hashlib = import_fresh_module('hashlib', blocked=['_hashlib'])
else:
    builtin_hashlib = None

try:
    from _hashlib import HASH, HASHXOF, openssl_md_meth_names, get_fips_mode
except ImportError:
    HASH = None
    HASHXOF = None
    openssl_md_meth_names = frozenset()

    def get_fips_mode():
        return 0

try:
    import _blake2
except ImportError:
    _blake2 = None

requires_blake2 = unittest.skipUnless(_blake2, 'requires _blake2')

try:
    import _sha3
except ImportError:
    _sha3 = None

requires_sha3 = unittest.skipUnless(_sha3, 'requires _sha3')


def hexstr(s):
    assert isinstance(s, bytes), repr(s)
    h = "0123456789abcdef"
    r = ''
    for i in s:
        r += h[(i >> 4) & 0xF] + h[i & 0xF]
    return r


URL = "http://www.pythontest.net/hashlib/{}.txt"

def read_vectors(hash_name):
    url = URL.format(hash_name)
    try:
        testdata = support.open_urlresource(url, encoding="utf-8")
    except (OSError, HTTPException):
        raise unittest.SkipTest("Could not retrieve {}".format(url))
    with testdata:
        for line in testdata:
            line = line.strip()
            if line.startswith('#') or not line:
                continue
            parts = line.split(',')
            parts[0] = bytes.fromhex(parts[0])
            yield parts


DEPRECATED_STRING_PARAMETER = re.escape(
    "the 'string' keyword parameter is deprecated since "
    "Python 3.15 and slated for removal in Python 3.19; "
    "use the 'data' keyword parameter or pass the data "
    "to hash as a positional argument instead"
)


class HashLibTestCase(unittest.TestCase):
    supported_hash_names = ( 'md5', 'MD5', 'sha1', 'SHA1',
                             'sha224', 'SHA224', 'sha256', 'SHA256',
                             'sha384', 'SHA384', 'sha512', 'SHA512',
                             'blake2b', 'blake2s',
                             'sha3_224', 'sha3_256', 'sha3_384', 'sha3_512',
                             'shake_128', 'shake_256')

    shakes = {'shake_128', 'shake_256'}

    # gh-58898: Fallback modules are always compiled under POSIX.
    _warn_on_extension_import = (os.name == 'posix' or support.Py_DEBUG)

    def _conditional_import_module(self, module_name):
        """Import a module and return a reference to it or None on failure."""
        try:
            return importlib.import_module(module_name)
        except ModuleNotFoundError as error:
            if self._warn_on_extension_import and module_name in builtin_hashes:
                logging.getLogger(__name__).warning(
                    'Did a C extension fail to compile? %s',
                    error,
                    exc_info=error,
                )
        return None

    def __init__(self, *args, **kwargs):
        algorithms = set()
        for algorithm in self.supported_hash_names:
            algorithms.add(algorithm.lower())

        _blake2 = self._conditional_import_module('_blake2')
        if _blake2:
            algorithms.update({'blake2b', 'blake2s'})

        self.constructors_to_test = {}
        for algorithm in algorithms:
            self.constructors_to_test[algorithm] = set()

        # For each algorithm, test the direct constructor and the use
        # of hashlib.new given the algorithm name.
        for algorithm, constructors in self.constructors_to_test.items():
            constructors.add(getattr(hashlib, algorithm))
            def c(*args, __algorithm_name=algorithm, **kwargs):
                return hashlib.new(__algorithm_name, *args, **kwargs)
            c.__name__ = f'do_test_algorithm_via_hashlib_new_{algorithm}'
            constructors.add(c)

        _hashlib = self._conditional_import_module('_hashlib')
        self._hashlib = _hashlib
        if _hashlib:
            # These algorithms should always be present when this module
            # is compiled.  If not, something was compiled wrong.
            self.assertHasAttr(_hashlib, 'openssl_md5')
            self.assertHasAttr(_hashlib, 'openssl_sha1')
            for algorithm, constructors in self.constructors_to_test.items():
                constructor = getattr(_hashlib, 'openssl_'+algorithm, None)
                if constructor:
                    try:
                        constructor()
                    except ValueError:
                        # default constructor blocked by crypto policy
                        pass
                    else:
                        constructors.add(constructor)

        def add_builtin_constructor(name):
            constructor = getattr(hashlib, "__get_builtin_constructor")(name)
            self.constructors_to_test[name].add(constructor)

        _md5 = self._conditional_import_module('_md5')
        if _md5:
            add_builtin_constructor('md5')
        _sha1 = self._conditional_import_module('_sha1')
        if _sha1:
            add_builtin_constructor('sha1')
        _sha2 = self._conditional_import_module('_sha2')
        if _sha2:
            add_builtin_constructor('sha224')
            add_builtin_constructor('sha256')
            add_builtin_constructor('sha384')
            add_builtin_constructor('sha512')
        _sha3 = self._conditional_import_module('_sha3')
        if _sha3:
            add_builtin_constructor('sha3_224')
            add_builtin_constructor('sha3_256')
            add_builtin_constructor('sha3_384')
            add_builtin_constructor('sha3_512')
            add_builtin_constructor('shake_128')
            add_builtin_constructor('shake_256')
        if _blake2:
            add_builtin_constructor('blake2s')
            add_builtin_constructor('blake2b')

        super(HashLibTestCase, self).__init__(*args, **kwargs)

    @property
    def hash_constructors(self):
        constructors = self.constructors_to_test.values()
        return itertools.chain.from_iterable(constructors)

    @property
    def shake_constructors(self):
        for shake_name in self.shakes:
            yield from self.constructors_to_test.get(shake_name, ())

    @property
    def is_fips_mode(self):
        return get_fips_mode()

    def test_hash_array(self):
        a = array.array("b", range(10))
        for cons in self.hash_constructors:
            c = cons(a, usedforsecurity=False)
            if c.name in self.shakes:
                c.hexdigest(16)
            else:
                c.hexdigest()

    def test_algorithms_guaranteed(self):
        self.assertEqual(hashlib.algorithms_guaranteed,
            set(_algo for _algo in self.supported_hash_names
                  if _algo.islower()))

    def test_algorithms_available(self):
        self.assertTrue(set(hashlib.algorithms_guaranteed).
                            issubset(hashlib.algorithms_available))
        # all available algorithms must be loadable, bpo-47101
        self.assertNotIn("undefined", hashlib.algorithms_available)
        for name in hashlib.algorithms_available:
            digest = hashlib.new(name, usedforsecurity=False)

    def test_usedforsecurity_true(self):
        hashlib.new("sha256", usedforsecurity=True)
        if self.is_fips_mode:
            self.skipTest("skip in FIPS mode")
        for cons in self.hash_constructors:
            cons(usedforsecurity=True)
            cons(b'', usedforsecurity=True)
        hashlib.new("md5", usedforsecurity=True)
        hashlib.md5(usedforsecurity=True)
        if self._hashlib is not None:
            self._hashlib.new("md5", usedforsecurity=True)
            self._hashlib.openssl_md5(usedforsecurity=True)

    def test_usedforsecurity_false(self):
        hashlib.new("sha256", usedforsecurity=False)
        for cons in self.hash_constructors:
            cons(usedforsecurity=False)
            cons(b'', usedforsecurity=False)
        hashlib.new("md5", usedforsecurity=False)
        hashlib.md5(usedforsecurity=False)
        if self._hashlib is not None:
            self._hashlib.new("md5", usedforsecurity=False)
            self._hashlib.openssl_md5(usedforsecurity=False)

    @unittest.skipIf(get_fips_mode(), "skip in FIPS mode")
    def test_clinic_signature(self):
        for constructor in self.hash_constructors:
            with self.subTest(constructor.__name__):
                constructor(b'')
                constructor(data=b'')
                with self.assertWarnsRegex(DeprecationWarning,
                                           DEPRECATED_STRING_PARAMETER):
                    constructor(string=b'')

            digest_name = constructor(b'').name
            with self.subTest(digest_name):
                hashlib.new(digest_name, b'')
                hashlib.new(digest_name, data=b'')
                with self.assertWarnsRegex(DeprecationWarning,
                                           DEPRECATED_STRING_PARAMETER):
                    hashlib.new(digest_name, string=b'')
                # Make sure that _hashlib contains the constructor
                # to test when using a combination of libcrypto and
                # interned hash implementations.
                if self._hashlib and digest_name in self._hashlib._constructors:
                    self._hashlib.new(digest_name, b'')
                    self._hashlib.new(digest_name, data=b'')
                    with self.assertWarnsRegex(DeprecationWarning,
                                               DEPRECATED_STRING_PARAMETER):
                        self._hashlib.new(digest_name, string=b'')

    @unittest.skipIf(get_fips_mode(), "skip in FIPS mode")
    def test_clinic_signature_errors(self):
        nomsg = b''
        mymsg = b'msg'
        conflicting_call = re.escape(
            "'data' and 'string' are mutually exclusive "
            "and support for 'string' keyword parameter "
            "is slated for removal in a future version."
        )
        duplicated_param = re.escape("given by name ('data') and position")
        unexpected_param = re.escape("got an unexpected keyword argument '_'")
        for args, kwds, errmsg in [
            # Reject duplicated arguments before unknown keyword arguments.
            ((nomsg,), dict(data=nomsg, _=nomsg), duplicated_param),
            ((mymsg,), dict(data=nomsg, _=nomsg), duplicated_param),
            # Reject duplicated arguments before conflicting ones.
            *itertools.product(
                [[nomsg], [mymsg]],
                [dict(data=nomsg), dict(data=nomsg, string=nomsg)],
                [duplicated_param]
            ),
            # Reject unknown keyword arguments before conflicting ones.
            *itertools.product(
                [()],
                [
                    dict(_=None),
                    dict(data=nomsg, _=None),
                    dict(string=nomsg, _=None),
                    dict(string=nomsg, data=nomsg, _=None),
                ],
                [unexpected_param]
            ),
            ((nomsg,), dict(_=None), unexpected_param),
            ((mymsg,), dict(_=None), unexpected_param),
            # Reject conflicting arguments.
            [(nomsg,), dict(string=nomsg), conflicting_call],
            [(mymsg,), dict(string=nomsg), conflicting_call],
            [(), dict(data=nomsg, string=nomsg), conflicting_call],
        ]:
            for constructor in self.hash_constructors:
                digest_name = constructor(b'').name
                with self.subTest(constructor.__name__, args=args, kwds=kwds):
                    with self.assertRaisesRegex(TypeError, errmsg):
                        constructor(*args, **kwds)
                with self.subTest(digest_name, args=args, kwds=kwds):
                    with self.assertRaisesRegex(TypeError, errmsg):
                        hashlib.new(digest_name, *args, **kwds)
                    if (self._hashlib and
                            digest_name in self._hashlib._constructors):
                        with self.assertRaisesRegex(TypeError, errmsg):
                            self._hashlib.new(digest_name, *args, **kwds)

    def test_unknown_hash(self):
        self.assertRaises(ValueError, hashlib.new, 'spam spam spam spam spam')
        self.assertRaises(TypeError, hashlib.new, 1)

    def test_new_upper_to_lower(self):
        self.assertEqual(hashlib.new("SHA256").name, "sha256")

    @support.thread_unsafe("modifies sys.modules")
    def test_get_builtin_constructor(self):
        get_builtin_constructor = getattr(hashlib,
                                          '__get_builtin_constructor')
        builtin_constructor_cache = getattr(hashlib,
                                            '__builtin_constructor_cache')
        self.assertRaises(ValueError, get_builtin_constructor, 'test')
        try:
            import _md5
        except ImportError:
            self.skipTest("_md5 module not available")
        # This forces an ImportError for "import _md5" statements
        sys.modules['_md5'] = None
        # clear the cache
        builtin_constructor_cache.clear()
        try:
            self.assertRaises(ValueError, get_builtin_constructor, 'md5')
        finally:
            if '_md5' in locals():
                sys.modules['_md5'] = _md5
            else:
                del sys.modules['_md5']
        self.assertRaises(TypeError, get_builtin_constructor, 3)
        constructor = get_builtin_constructor('md5')
        self.assertIs(constructor, _md5.md5)
        self.assertEqual(sorted(builtin_constructor_cache), ['MD5', 'md5'])

    def test_copy(self):
        for cons in self.hash_constructors:
            h1 = cons(os.urandom(16), usedforsecurity=False)
            h2 = h1.copy()
            self.assertIs(type(h1), type(h2))
            self.assertEqual(h1.name, h2.name)
            size = (16,) if h1.name in self.shakes else ()
            self.assertEqual(h1.digest(*size), h2.digest(*size))
            self.assertEqual(h1.hexdigest(*size), h2.hexdigest(*size))

    def test_hexdigest(self):
        for cons in self.hash_constructors:
            h = cons(usedforsecurity=False)
            if h.name in self.shakes:
                self.assertIsInstance(h.digest(16), bytes)
                self.assertEqual(hexstr(h.digest(16)), h.hexdigest(16))
            else:
                self.assertIsInstance(h.digest(), bytes)
                self.assertEqual(hexstr(h.digest()), h.hexdigest())

    def test_shakes_zero_digest_length(self):
        for constructor in self.shake_constructors:
            with self.subTest(constructor=constructor):
                h = constructor(b'abcdef', usedforsecurity=False)
                self.assertEqual(h.digest(0), b'')
                self.assertEqual(h.hexdigest(0), '')

    def test_shakes_invalid_digest_length(self):
        # See https://github.com/python/cpython/issues/79103.
        for constructor in self.shake_constructors:
            with self.subTest(constructor=constructor):
                h = constructor(usedforsecurity=False)
                # Note: digest() and hexdigest() take a signed input and
                # raise if it is negative; the rationale is that we use
                # internally PyBytes_FromStringAndSize() and _Py_strhex()
                # which both take a Py_ssize_t.
                for negative_size in (-1, -10, -(1 << 31), -sys.maxsize):
                    self.assertRaises(ValueError, h.digest, negative_size)
                    self.assertRaises(ValueError, h.hexdigest, negative_size)

    def test_shakes_overflow_digest_length(self):
        # See https://github.com/python/cpython/issues/135759.

        exc_types = (OverflowError, ValueError)
        # HACL* accepts an 'uint32_t' while OpenSSL accepts a 'size_t'.
        openssl_overflown_sizes = (sys.maxsize + 1, 2 * sys.maxsize)
        # https://github.com/python/cpython/issues/79103 restricts
        # the accepted built-in lengths to 2 ** 29, even if OpenSSL
        # accepts such lengths.
        builtin_overflown_sizes = openssl_overflown_sizes + (
            2 ** 29, 2 ** 32 - 10, 2 ** 32, 2 ** 32 + 10,
            2 ** 61, 2 ** 64 - 10, 2 ** 64, 2 ** 64 + 10,
        )

        for constructor in self.shake_constructors:
            with self.subTest(constructor=constructor):
                h = constructor(usedforsecurity=False)
                if HASH is not None and isinstance(h, HASH):
                    overflown_sizes = openssl_overflown_sizes
                else:
                    overflown_sizes = builtin_overflown_sizes
                for invalid_size in overflown_sizes:
                    self.assertRaises(exc_types, h.digest, invalid_size)
                    self.assertRaises(exc_types, h.hexdigest, invalid_size)

    def test_name_attribute(self):
        for cons in self.hash_constructors:
            h = cons(usedforsecurity=False)
            self.assertIsInstance(h.name, str)
            if h.name in self.supported_hash_names:
                self.assertIn(h.name, self.supported_hash_names)
            else:
                self.assertNotIn(h.name, self.supported_hash_names)
            self.assertEqual(
                h.name,
                hashlib.new(h.name, usedforsecurity=False).name
            )

    def test_large_update(self):
        aas = b'a' * 128
        bees = b'b' * 127
        cees = b'c' * 126
        dees = b'd' * 2048 #  HASHLIB_GIL_MINSIZE

        for cons in self.hash_constructors:
            m1 = cons(usedforsecurity=False)
            m1.update(aas)
            m1.update(bees)
            m1.update(cees)
            m1.update(dees)
            if m1.name in self.shakes:
                args = (16,)
            else:
                args = ()

            m2 = cons(usedforsecurity=False)
            m2.update(aas + bees + cees + dees)
            self.assertEqual(m1.digest(*args), m2.digest(*args))

            m3 = cons(aas + bees + cees + dees, usedforsecurity=False)
            self.assertEqual(m1.digest(*args), m3.digest(*args))

            # verify copy() doesn't touch original
            m4 = cons(aas + bees + cees, usedforsecurity=False)
            m4_digest = m4.digest(*args)
            m4_copy = m4.copy()
            m4_copy.update(dees)
            self.assertEqual(m1.digest(*args), m4_copy.digest(*args))
            self.assertEqual(m4.digest(*args), m4_digest)

    @requires_resource('cpu')
    def test_sha256_update_over_4gb(self):
        zero_1mb = b"\0" * 1024 * 1024
        h = hashlib.sha256()
        for i in range(0, 4096):
            h.update(zero_1mb)
        h.update(b"hello world")
        self.assertEqual(h.hexdigest(), "a5364f7a52ebe2e25f1838a4ca715a893b6fd7a23f2a0d9e9762120da8b1bf53")

    @requires_resource('cpu')
    def test_sha3_256_update_over_4gb(self):
        zero_1mb = b"\0" * 1024 * 1024
        h = hashlib.sha3_256()
        for i in range(0, 4096):
            h.update(zero_1mb)
        h.update(b"hello world")
        self.assertEqual(h.hexdigest(), "e2d4535e3b613135c14f2fe4e026d7ad8d569db44901740beffa30d430acb038")

    @requires_resource('cpu')
    def test_blake2_update_over_4gb(self):
        # blake2s or blake2b doesn't matter based on how our C code is structured, this tests the
        # common loop macro logic.
        zero_1mb = b"\0" * 1024 * 1024
        h = hashlib.blake2s()
        for i in range(0, 4096):
            h.update(zero_1mb)
        h.update(b"hello world")
        self.assertEqual(h.hexdigest(), "8a268e83dd30528bc0907fa2008c91de8f090a0b6e0e60a5ff0d999d8485526f")

    def check(self, name, data, hexdigest, shake=False, **kwargs):
        length = len(hexdigest)//2
        hexdigest = hexdigest.lower()
        constructors = self.constructors_to_test[name]
        # 2 is for hashlib.name(...) and hashlib.new(name, ...)
        self.assertGreaterEqual(len(constructors), 2)
        for hash_object_constructor in constructors:
            m = hash_object_constructor(data, **kwargs)
            computed = m.hexdigest() if not shake else m.hexdigest(length)
            self.assertEqual(
                    computed, hexdigest,
                    "Hash algorithm %s constructed using %s returned hexdigest"
                    " %r for %d byte input data that should have hashed to %r."
                    % (name, hash_object_constructor,
                       computed, len(data), hexdigest))
            computed = m.digest() if not shake else m.digest(length)
            digest = bytes.fromhex(hexdigest)
            self.assertEqual(computed, digest)
            if not shake:
                self.assertEqual(len(digest), m.digest_size)

        if not shake and kwargs.get("key") is None:
            # skip shake and blake2 extended parameter tests
            self.check_file_digest(name, data, hexdigest)

    def check_file_digest(self, name, data, hexdigest):
        hexdigest = hexdigest.lower()
        try:
            hashlib.new(name)
        except ValueError:
            # skip, algorithm is blocked by security policy.
            return
        digests = [name]
        digests.extend(self.constructors_to_test[name])

        with tempfile.TemporaryFile() as f:
            f.write(data)

            for digest in digests:
                buf = io.BytesIO(data)
                buf.seek(0)
                self.assertEqual(
                    hashlib.file_digest(buf, digest).hexdigest(), hexdigest
                )
                f.seek(0)
                digestobj = hashlib.file_digest(f, digest)
                self.assertEqual(digestobj.hexdigest(), hexdigest)

    def check_no_unicode(self, algorithm_name):
        # Unicode objects are not allowed as input.
        constructors = self.constructors_to_test[algorithm_name]
        for hash_object_constructor in constructors:
            self.assertRaises(TypeError, hash_object_constructor, 'spam')

    def test_no_unicode(self):
        self.check_no_unicode('md5')
        self.check_no_unicode('sha1')
        self.check_no_unicode('sha224')
        self.check_no_unicode('sha256')
        self.check_no_unicode('sha384')
        self.check_no_unicode('sha512')

    @requires_blake2
    def test_no_unicode_blake2(self):
        self.check_no_unicode('blake2b')
        self.check_no_unicode('blake2s')

    @requires_sha3
    def test_no_unicode_sha3(self):
        self.check_no_unicode('sha3_224')
        self.check_no_unicode('sha3_256')
        self.check_no_unicode('sha3_384')
        self.check_no_unicode('sha3_512')
        self.check_no_unicode('shake_128')
        self.check_no_unicode('shake_256')

    def check_blocksize_name(self, name, block_size=0, digest_size=0,
                             digest_length=None):
        constructors = self.constructors_to_test[name]
        for hash_object_constructor in constructors:
            m = hash_object_constructor(usedforsecurity=False)
            self.assertEqual(m.block_size, block_size)
            self.assertEqual(m.digest_size, digest_size)
            if digest_length:
                self.assertEqual(len(m.digest(digest_length)),
                                 digest_length)
                self.assertEqual(len(m.hexdigest(digest_length)),
                                 2*digest_length)
            else:
                self.assertEqual(len(m.digest()), digest_size)
                self.assertEqual(len(m.hexdigest()), 2*digest_size)
            self.assertEqual(m.name, name)
            # split for sha3_512 / _sha3.sha3 object
            self.assertIn(name.split("_")[0], repr(m).lower())

    def test_blocksize_and_name(self):
        self.check_blocksize_name('md5', 64, 16)
        self.check_blocksize_name('sha1', 64, 20)
        self.check_blocksize_name('sha224', 64, 28)
        self.check_blocksize_name('sha256', 64, 32)
        self.check_blocksize_name('sha384', 128, 48)
        self.check_blocksize_name('sha512', 128, 64)

    @requires_sha3
    def test_blocksize_name_sha3(self):
        self.check_blocksize_name('sha3_224', 144, 28)
        self.check_blocksize_name('sha3_256', 136, 32)
        self.check_blocksize_name('sha3_384', 104, 48)
        self.check_blocksize_name('sha3_512', 72, 64)
        self.check_blocksize_name('shake_128', 168, 0, 32)
        self.check_blocksize_name('shake_256', 136, 0, 64)

    def check_sha3(self, name, capacity, rate, suffix):
        constructors = self.constructors_to_test[name]
        for hash_object_constructor in constructors:
            m = hash_object_constructor()
            if HASH is not None and isinstance(m, HASH):
                # _hashopenssl's variant does not have extra SHA3 attributes
                continue
            self.assertEqual(capacity + rate, 1600)
            self.assertEqual(m._capacity_bits, capacity)
            self.assertEqual(m._rate_bits, rate)
            self.assertEqual(m._suffix, suffix)

    @requires_sha3
    def test_extra_sha3(self):
        self.check_sha3('sha3_224', 448, 1152, b'\x06')
        self.check_sha3('sha3_256', 512, 1088, b'\x06')
        self.check_sha3('sha3_384', 768, 832, b'\x06')
        self.check_sha3('sha3_512', 1024, 576, b'\x06')
        self.check_sha3('shake_128', 256, 1344, b'\x1f')
        self.check_sha3('shake_256', 512, 1088, b'\x1f')

    @requires_blake2
    def test_blocksize_name_blake2(self):
        self.check_blocksize_name('blake2b', 128, 64)
        self.check_blocksize_name('blake2s', 64, 32)

    def test_case_md5_0(self):
        self.check(
            'md5', b'', 'd41d8cd98f00b204e9800998ecf8427e',
            usedforsecurity=False
        )

    def test_case_md5_1(self):
        self.check(
            'md5', b'abc', '900150983cd24fb0d6963f7d28e17f72',
            usedforsecurity=False
        )

    def test_case_md5_2(self):
        self.check(
            'md5',
            b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
            'd174ab98d277d9f5a5611c2c9f419d9f',
            usedforsecurity=False
        )

    @unittest.skipIf(sys.maxsize < _4G + 5, 'test cannot run on 32-bit systems')
    @bigmemtest(size=_4G + 5, memuse=1, dry_run=False)
    def test_case_md5_huge(self, size):
        self.check('md5', b'A'*size, 'c9af2dff37468ce5dfee8f2cfc0a9c6d')

    @unittest.skipIf(sys.maxsize < _4G - 1, 'test cannot run on 32-bit systems')
    @bigmemtest(size=_4G - 1, memuse=1, dry_run=False)
    def test_case_md5_uintmax(self, size):
        self.check('md5', b'A'*size, '28138d306ff1b8281f1a9067e1a1a2b3')

    # use the three examples from Federal Information Processing Standards
    # Publication 180-1, Secure Hash Standard,  1995 April 17
    # http://www.itl.nist.gov/div897/pubs/fip180-1.htm

    def test_case_sha1_0(self):
        self.check('sha1', b"",
                   "da39a3ee5e6b4b0d3255bfef95601890afd80709")

    def test_case_sha1_1(self):
        self.check('sha1', b"abc",
                   "a9993e364706816aba3e25717850c26c9cd0d89d")

    def test_case_sha1_2(self):
        self.check('sha1',
                   b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                   "84983e441c3bd26ebaae4aa1f95129e5e54670f1")

    def test_case_sha1_3(self):
        self.check('sha1', b"a" * 1000000,
                   "34aa973cd4c4daa4f61eeb2bdbad27316534016f")


    # use the examples from Federal Information Processing Standards
    # Publication 180-2, Secure Hash Standard,  2002 August 1
    # http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf

    def test_case_sha224_0(self):
        self.check('sha224', b"",
          "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f")

    def test_case_sha224_1(self):
        self.check('sha224', b"abc",
          "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7")

    def test_case_sha224_2(self):
        self.check('sha224',
          b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525")

    def test_case_sha224_3(self):
        self.check('sha224', b"a" * 1000000,
          "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67")


    def test_case_sha256_0(self):
        self.check('sha256', b"",
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")

    def test_case_sha256_1(self):
        self.check('sha256', b"abc",
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")

    def test_case_sha256_2(self):
        self.check('sha256',
          b"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1")

    def test_case_sha256_3(self):
        self.check('sha256', b"a" * 1000000,
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0")


    def test_case_sha384_0(self):
        self.check('sha384', b"",
          "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da"+
          "274edebfe76f65fbd51ad2f14898b95b")

    def test_case_sha384_1(self):
        self.check('sha384', b"abc",
          "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed"+
          "8086072ba1e7cc2358baeca134c825a7")

    def test_case_sha384_2(self):
        self.check('sha384',
                   b"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"+
                   b"hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
          "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712"+
          "fcc7c71a557e2db966c3e9fa91746039")

    def test_case_sha384_3(self):
        self.check('sha384', b"a" * 1000000,
          "9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b"+
          "07b8b3dc38ecc4ebae97ddd87f3d8985")


    def test_case_sha512_0(self):
        self.check('sha512', b"",
          "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"+
          "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e")

    def test_case_sha512_1(self):
        self.check('sha512', b"abc",
          "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"+
          "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f")

    def test_case_sha512_2(self):
        self.check('sha512',
                   b"abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"+
                   b"hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
          "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"+
          "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909")

    def test_case_sha512_3(self):
        self.check('sha512', b"a" * 1000000,
          "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973eb"+
          "de0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b")

    def check_blake2(self, constructor, salt_size, person_size, key_size,
                     digest_size, max_offset):
        self.assertEqual(constructor.SALT_SIZE, salt_size)
        for i in range(salt_size + 1):
            constructor(salt=b'a' * i)
        salt = b'a' * (salt_size + 1)
        self.assertRaises(ValueError, constructor, salt=salt)

        self.assertEqual(constructor.PERSON_SIZE, person_size)
        for i in range(person_size+1):
            constructor(person=b'a' * i)
        person = b'a' * (person_size + 1)
        self.assertRaises(ValueError, constructor, person=person)

        self.assertEqual(constructor.MAX_DIGEST_SIZE, digest_size)
        for i in range(1, digest_size + 1):
            constructor(digest_size=i)
        self.assertRaises(ValueError, constructor, digest_size=-1)
        self.assertRaises(ValueError, constructor, digest_size=0)
        self.assertRaises(ValueError, constructor, digest_size=digest_size+1)

        self.assertEqual(constructor.MAX_KEY_SIZE, key_size)
        for i in range(key_size+1):
            constructor(key=b'a' * i)
        key = b'a' * (key_size + 1)
        self.assertRaises(ValueError, constructor, key=key)
        self.assertEqual(constructor().hexdigest(),
                         constructor(key=b'').hexdigest())

        for i in range(0, 256):
            constructor(fanout=i)
        self.assertRaises(ValueError, constructor, fanout=-1)
        self.assertRaises(ValueError, constructor, fanout=256)

        for i in range(1, 256):
            constructor(depth=i)
        self.assertRaises(ValueError, constructor, depth=-1)
        self.assertRaises(ValueError, constructor, depth=0)
        self.assertRaises(ValueError, constructor, depth=256)

        for i in range(0, 256):
            constructor(node_depth=i)
        self.assertRaises(ValueError, constructor, node_depth=-1)
        self.assertRaises(ValueError, constructor, node_depth=256)

        for i in range(0, digest_size + 1):
            constructor(inner_size=i)
        self.assertRaises(ValueError, constructor, inner_size=-1)
        self.assertRaises(ValueError, constructor, inner_size=digest_size+1)

        constructor(leaf_size=0)
        constructor(leaf_size=(1<<32)-1)
        self.assertRaises(ValueError, constructor, leaf_size=-1)
        self.assertRaises(OverflowError, constructor, leaf_size=1<<32)

        constructor(node_offset=0)
        constructor(node_offset=max_offset)
        self.assertRaises(ValueError, constructor, node_offset=-1)
        self.assertRaises(OverflowError, constructor, node_offset=max_offset+1)

        self.assertRaises(TypeError, constructor, '')

        constructor(
            b'',
            key=b'',
            salt=b'',
            person=b'',
            digest_size=17,
            fanout=1,
            depth=1,
            leaf_size=256,
            node_offset=512,
            node_depth=1,
            inner_size=7,
            last_node=True
        )

    def blake2_rfc7693(self, constructor, md_len, in_len):
        def selftest_seq(length, seed):
            mask = (1<<32)-1
            a = (0xDEAD4BAD * seed) & mask
            b = 1
            out = bytearray(length)
            for i in range(length):
                t = (a + b) & mask
                a, b = b, t
                out[i] = (t >> 24) & 0xFF
            return out
        outer = constructor(digest_size=32)
        for outlen in md_len:
            for inlen in in_len:
                indata = selftest_seq(inlen, inlen)
                key = selftest_seq(outlen, outlen)
                unkeyed = constructor(indata, digest_size=outlen)
                outer.update(unkeyed.digest())
                keyed = constructor(indata, key=key, digest_size=outlen)
                outer.update(keyed.digest())
        return outer.hexdigest()

    @requires_blake2
    def test_blake2b(self):
        self.check_blake2(hashlib.blake2b, 16, 16, 64, 64, (1<<64)-1)
        b2b_md_len = [20, 32, 48, 64]
        b2b_in_len = [0, 3, 128, 129, 255, 1024]
        self.assertEqual(
            self.blake2_rfc7693(hashlib.blake2b, b2b_md_len, b2b_in_len),
            "c23a7800d98123bd10f506c61e29da5603d763b8bbad2e737f5e765a7bccd475")

    @requires_blake2
    def test_case_blake2b_0(self):
        self.check('blake2b', b"",
          "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419"+
          "d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce")

    @requires_blake2
    def test_case_blake2b_1(self):
        self.check('blake2b', b"abc",
          "ba80a53f981c4d0d6a2797b69f12f6e94c212f14685ac4b74b12bb6fdbffa2d1"+
          "7d87c5392aab792dc252d5de4533cc9518d38aa8dbf1925ab92386edd4009923")

    @requires_blake2
    def test_case_blake2b_all_parameters(self):
        # This checks that all the parameters work in general, and also that
        # parameter byte order doesn't get confused on big endian platforms.
        self.check('blake2b', b"foo",
          "920568b0c5873b2f0ab67bedb6cf1b2b",
          digest_size=16,
          key=b"bar",
          salt=b"baz",
          person=b"bing",
          fanout=2,
          depth=3,
          leaf_size=4,
          node_offset=5,
          node_depth=6,
          inner_size=7,
          last_node=True)

    @requires_blake2
    def test_blake2b_vectors(self):
        for msg, key, md in read_vectors('blake2b'):
            key = bytes.fromhex(key)
            self.check('blake2b', msg, md, key=key)

    @requires_blake2
    def test_blake2s(self):
        self.check_blake2(hashlib.blake2s, 8, 8, 32, 32, (1<<48)-1)
        b2s_md_len = [16, 20, 28, 32]
        b2s_in_len = [0, 3, 64, 65, 255, 1024]
        self.assertEqual(
            self.blake2_rfc7693(hashlib.blake2s, b2s_md_len, b2s_in_len),
            "6a411f08ce25adcdfb02aba641451cec53c598b24f4fc787fbdc88797f4c1dfe")

    @requires_blake2
    def test_case_blake2s_0(self):
        self.check('blake2s', b"",
          "69217a3079908094e11121d042354a7c1f55b6482ca1a51e1b250dfd1ed0eef9")

    @requires_blake2
    def test_case_blake2s_1(self):
        self.check('blake2s', b"abc",
          "508c5e8c327c14e2e1a72ba34eeb452f37458b209ed63a294d999b4c86675982")

    @requires_blake2
    def test_case_blake2s_all_parameters(self):
        # This checks that all the parameters work in general, and also that
        # parameter byte order doesn't get confused on big endian platforms.
        self.check('blake2s', b"foo",
          "bf2a8f7fe3c555012a6f8046e646bc75",
          digest_size=16,
          key=b"bar",
          salt=b"baz",
          person=b"bing",
          fanout=2,
          depth=3,
          leaf_size=4,
          node_offset=5,
          node_depth=6,
          inner_size=7,
          last_node=True)

    @requires_blake2
    def test_blake2s_vectors(self):
        for msg, key, md in read_vectors('blake2s'):
            key = bytes.fromhex(key)
            self.check('blake2s', msg, md, key=key)

    @requires_sha3
    def test_case_sha3_224_0(self):
        self.check('sha3_224', b"",
          "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7")

    @requires_sha3
    def test_case_sha3_224_vector(self):
        for msg, md in read_vectors('sha3_224'):
            self.check('sha3_224', msg, md)

    @requires_sha3
    def test_case_sha3_256_0(self):
        self.check('sha3_256', b"",
          "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a")

    @requires_sha3
    def test_case_sha3_256_vector(self):
        for msg, md in read_vectors('sha3_256'):
            self.check('sha3_256', msg, md)

    @requires_sha3
    def test_case_sha3_384_0(self):
        self.check('sha3_384', b"",
          "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2a"+
          "c3713831264adb47fb6bd1e058d5f004")

    @requires_sha3
    def test_case_sha3_384_vector(self):
        for msg, md in read_vectors('sha3_384'):
            self.check('sha3_384', msg, md)

    @requires_sha3
    def test_case_sha3_512_0(self):
        self.check('sha3_512', b"",
          "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a6"+
          "15b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26")

    @requires_sha3
    def test_case_sha3_512_vector(self):
        for msg, md in read_vectors('sha3_512'):
            self.check('sha3_512', msg, md)

    def test_case_shake_128_0(self):
        self.check('shake_128', b"",
          "7f9c2ba4e88f827d616045507605853ed73b8093f6efbc88eb1a6eacfa66ef26",
          True)
        self.check('shake_128', b"", "7f9c", True)

    def test_case_shake128_vector(self):
        for msg, md in read_vectors('shake_128'):
            self.check('shake_128', msg, md, True)

    def test_case_shake_256_0(self):
        self.check('shake_256', b"",
          "46b9dd2b0ba88d13233b3feb743eeb243fcd52ea62b81b82b50c27646ed5762f",
          True)
        self.check('shake_256', b"", "46b9", True)

    def test_case_shake256_vector(self):
        for msg, md in read_vectors('shake_256'):
            self.check('shake_256', msg, md, True)

    def test_gil(self):
        # Check things work fine with an input larger than the size required
        # for multithreaded operation. Currently, all cryptographic modules
        # have the same constant value (2048) but in the future it might not
        # be the case.
        mods = ['_md5', '_sha1', '_sha2', '_sha3', '_blake2', '_hashlib']
        gil_minsize = hashlib_helper.find_gil_minsize(mods)
        for cons in self.hash_constructors:
            # constructors belong to one of the above modules
            m = cons(usedforsecurity=False)
            m.update(b'1')
            m.update(b'#' * gil_minsize)
            m.update(b'1')

            m = cons(b'x' * gil_minsize, usedforsecurity=False)
            m.update(b'1')

    def test_sha256_gil(self):
        gil_minsize = hashlib_helper.find_gil_minsize(['_sha2', '_hashlib'])
        data = b'1' + b'#' * gil_minsize + b'1'
        expected = hashlib.sha256(data).hexdigest()

        m = hashlib.sha256()
        m.update(b'1')
        m.update(b'#' * gil_minsize)
        m.update(b'1')
        self.assertEqual(m.hexdigest(), expected)

    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_threaded_hashing_fast(self):
        # Same as test_threaded_hashing_slow() but only tests some functions
        # since otherwise test_hashlib.py becomes too slow during development.
        for name in ['md5', 'sha1', 'sha256', 'sha3_256', 'blake2s']:
            if constructor := getattr(hashlib, name, None):
                with self.subTest(name):
                    self.do_test_threaded_hashing(constructor, is_shake=False)
        if shake_128 := getattr(hashlib, 'shake_128', None):
            self.do_test_threaded_hashing(shake_128, is_shake=True)

    @requires_resource('cpu')
    @threading_helper.reap_threads
    @threading_helper.requires_working_threading()
    def test_threaded_hashing_slow(self):
        for algorithm, constructors in self.constructors_to_test.items():
            is_shake = algorithm in self.shakes
            for constructor in constructors:
                with self.subTest(constructor.__name__, is_shake=is_shake):
                    self.do_test_threaded_hashing(constructor, is_shake)

    def do_test_threaded_hashing(self, constructor, is_shake):
        # Updating the same hash object from several threads at once
        # using data chunk sizes containing the same byte sequences.
        #
        # If the internal locks are working to prevent multiple
        # updates on the same object from running at once, the resulting
        # hash will be the same as doing it single threaded upfront.

        # The data to hash has length s|M|q^N and the chunk size for the i-th
        # thread is s|M|q^(N-i), where N is the number of threads, M is a fixed
        # message of small length, and s >= 1 and q >= 2 are small integers.
        smallest_size, num_threads, s, q = 8, 5, 2, 10

        smallest_data = os.urandom(smallest_size)
        data = s * smallest_data * (q ** num_threads)

        h1 = constructor(usedforsecurity=False)
        h2 = constructor(data * num_threads, usedforsecurity=False)

        def update(chunk_size):
            for index in range(0, len(data), chunk_size):
                h1.update(data[index:index + chunk_size])

        threads = []
        for thread_num in range(num_threads):
            # chunk_size = len(data) // (q ** thread_num)
            chunk_size = s * smallest_size * q ** (num_threads - thread_num)
            self.assertGreater(chunk_size, 0)
            self.assertEqual(chunk_size % smallest_size, 0)
            thread = threading.Thread(target=update, args=(chunk_size,))
            threads.append(thread)

        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

        if is_shake:
            self.assertEqual(h1.hexdigest(16), h2.hexdigest(16))
        else:
            self.assertEqual(h1.hexdigest(), h2.hexdigest())

    def test_get_fips_mode(self):
        fips_mode = self.is_fips_mode
        if fips_mode is not None:
            self.assertIsInstance(fips_mode, int)

    @support.cpython_only
    def test_disallow_instantiation(self):
        for algorithm, constructors in self.constructors_to_test.items():
            if algorithm.startswith(("sha3_", "shake", "blake")):
                # _sha3 and _blake types can be instantiated
                continue
            # all other types have DISALLOW_INSTANTIATION
            for constructor in constructors:
                # In FIPS mode some algorithms are not available raising ValueError
                try:
                    h = constructor()
                except ValueError:
                    continue
                with self.subTest(constructor=constructor):
                    support.check_disallow_instantiation(self, type(h))

    @unittest.skipUnless(HASH is not None, 'need _hashlib')
    def test_hash_disallow_instantiation(self):
        # internal types like _hashlib.HASH are not constructable
        support.check_disallow_instantiation(self, HASH)
        support.check_disallow_instantiation(self, HASHXOF)

    def test_readonly_types(self):
        for algorithm, constructors in self.constructors_to_test.items():
            # all other types have DISALLOW_INSTANTIATION
            for constructor in constructors:
                # In FIPS mode some algorithms are not available raising ValueError
                try:
                    hash_type = type(constructor())
                except ValueError:
                    continue
                with self.subTest(hash_type=hash_type):
                    with self.assertRaisesRegex(TypeError, "immutable type"):
                        hash_type.value = False


class KDFTests(unittest.TestCase):

    pbkdf2_test_vectors = [
        (b'password', b'salt', 1, None),
        (b'password', b'salt', 2, None),
        (b'password', b'salt', 4096, None),
        # too slow, it takes over a minute on a fast CPU.
        #(b'password', b'salt', 16777216, None),
        (b'passwordPASSWORDpassword', b'saltSALTsaltSALTsaltSALTsaltSALTsalt',
         4096, -1),
        (b'pass\0word', b'sa\0lt', 4096, 16),
    ]

    pbkdf2_results = {
        "sha1": [
            # official test vectors from RFC 6070
            (bytes.fromhex('0c60c80f961f0e71f3a9b524af6012062fe037a6'), None),
            (bytes.fromhex('ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957'), None),
            (bytes.fromhex('4b007901b765489abead49d926f721d065a429c1'), None),
            #(bytes.fromhex('eefe3d61cd4da4e4e9945b3d6ba2158c2634e984'), None),
            (bytes.fromhex('3d2eec4fe41c849b80c8d83662c0e44a8b291a964c'
                           'f2f07038'), 25),
            (bytes.fromhex('56fa6aa75548099dcc37d7f03425e0c3'), None),],
        "sha256": [
            (bytes.fromhex('120fb6cffcf8b32c43e7225256c4f837'
                           'a86548c92ccc35480805987cb70be17b'), None),
            (bytes.fromhex('ae4d0c95af6b46d32d0adff928f06dd0'
                           '2a303f8ef3c251dfd6e2d85a95474c43'), None),
            (bytes.fromhex('c5e478d59288c841aa530db6845c4c8d'
                           '962893a001ce4e11a4963873aa98134a'), None),
            #(bytes.fromhex('cf81c66fe8cfc04d1f31ecb65dab4089'
            #               'f7f179e89b3b0bcb17ad10e3ac6eba46'), None),
            (bytes.fromhex('348c89dbcbd32b2f32d814b8116e84cf2b17'
                           '347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9'), 40),
            (bytes.fromhex('89b69d0516f829893c696226650a8687'), None),],
        "sha512": [
            (bytes.fromhex('867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5'
                           'd513554e1c8cf252c02d470a285a0501bad999bfe943c08f'
                           '050235d7d68b1da55e63f73b60a57fce'), None),
            (bytes.fromhex('e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f004071'
                           '3f18aefdb866d53cf76cab2868a39b9f7840edce4fef5a82'
                           'be67335c77a6068e04112754f27ccf4e'), None),
            (bytes.fromhex('d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f8'
                           '7f6902e072f457b5143f30602641b3d55cd335988cb36b84'
                           '376060ecd532e039b742a239434af2d5'), None),
            (bytes.fromhex('8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b8'
                           '68c005174dc4ee71115b59f9e60cd9532fa33e0f75aefe30'
                           '225c583a186cd82bd4daea9724a3d3b8'), 64),
            (bytes.fromhex('9d9e9c4cd21fe4be24d5b8244c759665'), None),],
    }

    def _test_pbkdf2_hmac(self, pbkdf2, supported):
        for digest_name, results in self.pbkdf2_results.items():
            if digest_name not in supported:
                continue
            for i, vector in enumerate(self.pbkdf2_test_vectors):
                password, salt, rounds, dklen = vector
                expected, overwrite_dklen = results[i]
                if overwrite_dklen:
                    dklen = overwrite_dklen
                out = pbkdf2(digest_name, password, salt, rounds, dklen)
                self.assertEqual(out, expected,
                                 (digest_name, password, salt, rounds, dklen))
                out = pbkdf2(digest_name, memoryview(password),
                             memoryview(salt), rounds, dklen)
                self.assertEqual(out, expected)
                out = pbkdf2(digest_name, bytearray(password),
                             bytearray(salt), rounds, dklen)
                self.assertEqual(out, expected)
                if dklen is None:
                    out = pbkdf2(digest_name, password, salt, rounds)
                    self.assertEqual(out, expected,
                                     (digest_name, password, salt, rounds))

        with self.assertRaisesRegex(ValueError, '.*unsupported.*'):
            pbkdf2('unknown', b'pass', b'salt', 1)

        if 'sha1' in supported:
            self.assertRaises(
                TypeError, pbkdf2, b'sha1', b'pass', b'salt', 1
            )
            self.assertRaises(
                TypeError, pbkdf2, 'sha1', 'pass', 'salt', 1
            )
            self.assertRaises(
                ValueError, pbkdf2, 'sha1', b'pass', b'salt', 0
            )
            self.assertRaises(
                ValueError, pbkdf2, 'sha1', b'pass', b'salt', -1
            )
            self.assertRaises(
                ValueError, pbkdf2, 'sha1', b'pass', b'salt', 1, 0
            )
            self.assertRaises(
                ValueError, pbkdf2, 'sha1', b'pass', b'salt', 1, -1
            )
            out = pbkdf2(hash_name='sha1', password=b'password', salt=b'salt',
                iterations=1, dklen=None)
            self.assertEqual(out, self.pbkdf2_results['sha1'][0][0])

    @unittest.skipIf(openssl_hashlib is None, "requires OpenSSL bindings")
    def test_pbkdf2_hmac_c(self):
        self._test_pbkdf2_hmac(openssl_hashlib.pbkdf2_hmac, openssl_md_meth_names)

    def test_normalized_name(self):
        self.assertNotIn("blake2b512", hashlib.algorithms_available)
        self.assertNotIn("sha3-512", hashlib.algorithms_available)

    def test_file_digest(self):
        data = b'a' * 65536
        d1 = hashlib.sha256()
        with tempfile.NamedTemporaryFile(delete_on_close=False) as fp:
            for _ in range(10):
                d1.update(data)
                fp.write(data)
            fp.close()

            with open(fp.name, "rb") as f:
                d2 = hashlib.file_digest(f, hashlib.sha256)

            self.assertEqual(d1.hexdigest(), d2.hexdigest())
            self.assertEqual(d1.name, d2.name)
            self.assertIs(type(d1), type(d2))

            with self.assertRaises(ValueError):
                with open(fp.name, "r") as f:
                    hashlib.file_digest(f, "sha256")

            with self.assertRaises(ValueError):
                with open(fp.name, "wb") as f:
                    hashlib.file_digest(f, "sha256")

        with self.assertRaises(ValueError):
            hashlib.file_digest(None, "sha256")

        class NonBlocking:
            def readinto(self, buf):
                return None
            def readable(self):
                return True

        with self.assertRaises(BlockingIOError):
            hashlib.file_digest(NonBlocking(), hashlib.sha256)


@unittest.skipUnless(hasattr(hashlib, 'scrypt'), 'requires OpenSSL 1.1+')
@unittest.skipIf(get_fips_mode(), reason="scrypt is blocked in FIPS mode")
class TestScrypt(unittest.TestCase):

    scrypt_test_vectors = [
        (b'', b'', 16, 1, 1, unhexlify('77d6576238657b203b19ca42c18a0497f16b4844e3074ae8dfdffa3fede21442fcd0069ded0948f8326a753a0fc81f17e8d3e0fb2e0d3628cf35e20c38d18906')),
        (b'password', b'NaCl', 1024, 8, 16, unhexlify('fdbabe1c9d3472007856e7190d01e9fe7c6ad7cbc8237830e77376634b3731622eaf30d92e22a3886ff109279d9830dac727afb94a83ee6d8360cbdfa2cc0640')),
        (b'pleaseletmein', b'SodiumChloride', 16384, 8, 1, unhexlify('7023bdcb3afd7348461c06cd81fd38ebfda8fbba904f8e3ea9b543f6545da1f2d5432955613f0fcf62d49705242a9af9e61e85dc0d651e40dfcf017b45575887')),
   ]

    def test_scrypt(self):
        for password, salt, n, r, p, expected in self.scrypt_test_vectors:
            result = hashlib.scrypt(password, salt=salt, n=n, r=r, p=p)
            self.assertEqual(result, expected)

        # these parameters must be valid
        hashlib.scrypt(b'password', salt=b'salt', n=2, r=8, p=1)
        hashlib.scrypt(b'password', salt=b'salt', n=2, r=8, p=1, maxmem=0)
        hashlib.scrypt(b'password', salt=b'salt', n=2, r=8, p=1, dklen=1)

    def test_scrypt_types(self):
        # password and salt must be bytes-like
        with self.assertRaises(TypeError):
            hashlib.scrypt('password', salt=b'salt', n=2, r=8, p=1)
        with self.assertRaises(TypeError):
            hashlib.scrypt(b'password', salt='salt', n=2, r=8, p=1)
        # require keyword args
        with self.assertRaises(TypeError):
            hashlib.scrypt(b'password')
        with self.assertRaises(TypeError):
            hashlib.scrypt(b'password', b'salt')
        with self.assertRaises(TypeError):
            hashlib.scrypt(b'password', 2, 8, 1, salt=b'salt')

    def test_scrypt_validate(self):
        def scrypt(password=b"password", /, **kwargs):
            # overwrite well-defined parameters with bad ones
            kwargs = dict(salt=b'salt', n=2, r=8, p=1) | kwargs
            return hashlib.scrypt(password, **kwargs)

        for param_name in ('n', 'r', 'p', 'maxmem', 'dklen'):
            param = {param_name: None}
            with self.subTest(**param):
                self.assertRaises(TypeError, scrypt, **param)

        self.assertRaises(ValueError, scrypt, n=0)
        self.assertRaises(ValueError, scrypt, n=-1)
        self.assertRaises(ValueError, scrypt, n=1)

        self.assertRaises(ValueError, scrypt, r=0)
        self.assertRaises(ValueError, scrypt, r=-1)

        self.assertRaises(ValueError, scrypt, p=-1)
        self.assertRaises(ValueError, scrypt, p=0)

        self.assertRaises(ValueError, scrypt, maxmem=-1)
        # OpenSSL hard limit for 'maxmem' is an 'uint64_t' but for now,
        # we do not use the 'uint64' Clinic converter but the 'long' one.
        self.assertRaises(OverflowError, scrypt, maxmem=(1 << 64))
        # Historically, Python allowed 'maxmem' to be at most INT_MAX,
        # which is at most 2**32-1 (on Windows, sizeof(long) == 4, so
        # an OverflowError will be raised instead of a ValueError).
        numeric_exc_types = (OverflowError, ValueError)
        self.assertRaises(numeric_exc_types, scrypt, maxmem=(1 << 32))

        self.assertRaises(ValueError, scrypt, dklen=-1)
        self.assertRaises(ValueError, scrypt, dklen=0)
        MAX_DKLEN = ((1 << 32) - 1) * 32  # see RFC 7914
        self.assertRaises(numeric_exc_types, scrypt, dklen=MAX_DKLEN + 1)


if __name__ == "__main__":
    unittest.main()
