# Test hashlib module
#
# $Id$
#
#  Copyright (C) 2005-2010   Gregory P. Smith (greg@krypto.org)
#  Licensed to PSF under a Contributor Agreement.
#

import array
import hashlib
import itertools
import os
import sys
try:
    import threading
except ImportError:
    threading = None
import unittest
import warnings
from test import support
from test.support import _4G, bigmemtest, import_fresh_module

# Were we compiled --with-pydebug or with #define Py_DEBUG?
COMPILED_WITH_PYDEBUG = hasattr(sys, 'gettotalrefcount')

c_hashlib = import_fresh_module('hashlib', fresh=['_hashlib'])
py_hashlib = import_fresh_module('hashlib', blocked=['_hashlib'])

def hexstr(s):
    assert isinstance(s, bytes), repr(s)
    h = "0123456789abcdef"
    r = ''
    for i in s:
        r += h[(i >> 4) & 0xF] + h[i & 0xF]
    return r


class HashLibTestCase(unittest.TestCase):
    supported_hash_names = ( 'md5', 'MD5', 'sha1', 'SHA1',
                             'sha224', 'SHA224', 'sha256', 'SHA256',
                             'sha384', 'SHA384', 'sha512', 'SHA512')

    # Issue #14693: fallback modules are always compiled under POSIX
    _warn_on_extension_import = os.name == 'posix' or COMPILED_WITH_PYDEBUG

    def _conditional_import_module(self, module_name):
        """Import a module and return a reference to it or None on failure."""
        try:
            exec('import '+module_name)
        except ImportError as error:
            if self._warn_on_extension_import:
                warnings.warn('Did a C extension fail to compile? %s' % error)
        return locals().get(module_name)

    def __init__(self, *args, **kwargs):
        algorithms = set()
        for algorithm in self.supported_hash_names:
            algorithms.add(algorithm.lower())
        self.constructors_to_test = {}
        for algorithm in algorithms:
            self.constructors_to_test[algorithm] = set()

        # For each algorithm, test the direct constructor and the use
        # of hashlib.new given the algorithm name.
        for algorithm, constructors in self.constructors_to_test.items():
            constructors.add(getattr(hashlib, algorithm))
            def _test_algorithm_via_hashlib_new(data=None, _alg=algorithm):
                if data is None:
                    return hashlib.new(_alg)
                return hashlib.new(_alg, data)
            constructors.add(_test_algorithm_via_hashlib_new)

        _hashlib = self._conditional_import_module('_hashlib')
        if _hashlib:
            # These two algorithms should always be present when this module
            # is compiled.  If not, something was compiled wrong.
            self.assertTrue(hasattr(_hashlib, 'openssl_md5'))
            self.assertTrue(hasattr(_hashlib, 'openssl_sha1'))
            for algorithm, constructors in self.constructors_to_test.items():
                constructor = getattr(_hashlib, 'openssl_'+algorithm, None)
                if constructor:
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
        _sha256 = self._conditional_import_module('_sha256')
        if _sha256:
            add_builtin_constructor('sha224')
            add_builtin_constructor('sha256')
        _sha512 = self._conditional_import_module('_sha512')
        if _sha512:
            add_builtin_constructor('sha384')
            add_builtin_constructor('sha512')

        super(HashLibTestCase, self).__init__(*args, **kwargs)

    @property
    def hash_constructors(self):
        constructors = self.constructors_to_test.values()
        return itertools.chain.from_iterable(constructors)

    def test_hash_array(self):
        a = array.array("b", range(10))
        for cons in self.hash_constructors:
            c = cons(a)
            c.hexdigest()

    def test_algorithms_guaranteed(self):
        self.assertEqual(hashlib.algorithms_guaranteed,
            set(_algo for _algo in self.supported_hash_names
                  if _algo.islower()))

    def test_algorithms_available(self):
        self.assertTrue(set(hashlib.algorithms_guaranteed).
                            issubset(hashlib.algorithms_available))

    def test_unknown_hash(self):
        self.assertRaises(ValueError, hashlib.new, 'spam spam spam spam spam')
        self.assertRaises(TypeError, hashlib.new, 1)

    def test_get_builtin_constructor(self):
        get_builtin_constructor = getattr(hashlib,
                                          '__get_builtin_constructor')
        builtin_constructor_cache = getattr(hashlib,
                                            '__builtin_constructor_cache')
        self.assertRaises(ValueError, get_builtin_constructor, 'test')
        try:
            import _md5
        except ImportError:
            pass
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

    def test_hexdigest(self):
        for cons in self.hash_constructors:
            h = cons()
            self.assertIsInstance(h.digest(), bytes)
            self.assertEqual(hexstr(h.digest()), h.hexdigest())

    def test_name_attribute(self):
        for cons in self.hash_constructors:
            h = cons()
            self.assertIsInstance(h.name, str)
            self.assertIn(h.name, self.supported_hash_names)
            self.assertEqual(h.name, hashlib.new(h.name).name)

    def test_large_update(self):
        aas = b'a' * 128
        bees = b'b' * 127
        cees = b'c' * 126
        dees = b'd' * 2048 #  HASHLIB_GIL_MINSIZE

        for cons in self.hash_constructors:
            m1 = cons()
            m1.update(aas)
            m1.update(bees)
            m1.update(cees)
            m1.update(dees)

            m2 = cons()
            m2.update(aas + bees + cees + dees)
            self.assertEqual(m1.digest(), m2.digest())

            m3 = cons(aas + bees + cees + dees)
            self.assertEqual(m1.digest(), m3.digest())

            # verify copy() doesn't touch original
            m4 = cons(aas + bees + cees)
            m4_digest = m4.digest()
            m4_copy = m4.copy()
            m4_copy.update(dees)
            self.assertEqual(m1.digest(), m4_copy.digest())
            self.assertEqual(m4.digest(), m4_digest)

    def check(self, name, data, hexdigest):
        hexdigest = hexdigest.lower()
        constructors = self.constructors_to_test[name]
        # 2 is for hashlib.name(...) and hashlib.new(name, ...)
        self.assertGreaterEqual(len(constructors), 2)
        for hash_object_constructor in constructors:
            m = hash_object_constructor(data)
            computed = m.hexdigest()
            self.assertEqual(
                    computed, hexdigest,
                    "Hash algorithm %s constructed using %s returned hexdigest"
                    " %r for %d byte input data that should have hashed to %r."
                    % (name, hash_object_constructor,
                       computed, len(data), hexdigest))
            computed = m.digest()
            digest = bytes.fromhex(hexdigest)
            self.assertEqual(computed, digest)
            self.assertEqual(len(digest), m.digest_size)

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

    def check_blocksize_name(self, name, block_size=0, digest_size=0):
        constructors = self.constructors_to_test[name]
        for hash_object_constructor in constructors:
            m = hash_object_constructor()
            self.assertEqual(m.block_size, block_size)
            self.assertEqual(m.digest_size, digest_size)
            self.assertEqual(len(m.digest()), digest_size)
            self.assertEqual(m.name, name)
            # split for sha3_512 / _sha3.sha3 object
            self.assertIn(name.split("_")[0], repr(m))

    def test_blocksize_name(self):
        self.check_blocksize_name('md5', 64, 16)
        self.check_blocksize_name('sha1', 64, 20)
        self.check_blocksize_name('sha224', 64, 28)
        self.check_blocksize_name('sha256', 64, 32)
        self.check_blocksize_name('sha384', 128, 48)
        self.check_blocksize_name('sha512', 128, 64)

    def test_case_md5_0(self):
        self.check('md5', b'', 'd41d8cd98f00b204e9800998ecf8427e')

    def test_case_md5_1(self):
        self.check('md5', b'abc', '900150983cd24fb0d6963f7d28e17f72')

    def test_case_md5_2(self):
        self.check('md5',
                   b'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
                   'd174ab98d277d9f5a5611c2c9f419d9f')

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

    @unittest.skipIf(sys.maxsize < _4G + 5, 'test cannot run on 32-bit systems')
    @bigmemtest(size=_4G + 5, memuse=1, dry_run=False)
    def test_case_sha3_224_huge(self, size):
        self.check('sha3_224', b'A'*size,
                   '58ef60057c9dddb6a87477e9ace5a26f0d9db01881cf9b10a9f8c224')

    def test_gil(self):
        # Check things work fine with an input larger than the size required
        # for multithreaded operation (which is hardwired to 2048).
        gil_minsize = 2048

        for cons in self.hash_constructors:
            m = cons()
            m.update(b'1')
            m.update(b'#' * gil_minsize)
            m.update(b'1')

            m = cons(b'x' * gil_minsize)
            m.update(b'1')

        m = hashlib.md5()
        m.update(b'1')
        m.update(b'#' * gil_minsize)
        m.update(b'1')
        self.assertEqual(m.hexdigest(), 'cb1e1a2cbc80be75e19935d621fb9b21')

        m = hashlib.md5(b'x' * gil_minsize)
        self.assertEqual(m.hexdigest(), 'cfb767f225d58469c5de3632a8803958')

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @support.reap_threads
    def test_threaded_hashing(self):
        # Updating the same hash object from several threads at once
        # using data chunk sizes containing the same byte sequences.
        #
        # If the internal locks are working to prevent multiple
        # updates on the same object from running at once, the resulting
        # hash will be the same as doing it single threaded upfront.
        hasher = hashlib.sha1()
        num_threads = 5
        smallest_data = b'swineflu'
        data = smallest_data*200000
        expected_hash = hashlib.sha1(data*num_threads).hexdigest()

        def hash_in_chunks(chunk_size, event):
            index = 0
            while index < len(data):
                hasher.update(data[index:index+chunk_size])
                index += chunk_size
            event.set()

        events = []
        for threadnum in range(num_threads):
            chunk_size = len(data) // (10**threadnum)
            self.assertGreater(chunk_size, 0)
            self.assertEqual(chunk_size % len(smallest_data), 0)
            event = threading.Event()
            events.append(event)
            threading.Thread(target=hash_in_chunks,
                             args=(chunk_size, event)).start()

        for event in events:
            event.wait()

        self.assertEqual(expected_hash, hasher.hexdigest())


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
            # offical test vectors from RFC 6070
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

    def _test_pbkdf2_hmac(self, pbkdf2):
        for digest_name, results in self.pbkdf2_results.items():
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
                out = pbkdf2(digest_name, bytearray(password),
                             bytearray(salt), rounds, dklen)
                self.assertEqual(out, expected)
                if dklen is None:
                    out = pbkdf2(digest_name, password, salt, rounds)
                    self.assertEqual(out, expected,
                                     (digest_name, password, salt, rounds))

        self.assertRaises(TypeError, pbkdf2, b'sha1', b'pass', b'salt', 1)
        self.assertRaises(TypeError, pbkdf2, 'sha1', 'pass', 'salt', 1)
        self.assertRaises(ValueError, pbkdf2, 'sha1', b'pass', b'salt', 0)
        self.assertRaises(ValueError, pbkdf2, 'sha1', b'pass', b'salt', -1)
        self.assertRaises(ValueError, pbkdf2, 'sha1', b'pass', b'salt', 1, 0)
        self.assertRaises(ValueError, pbkdf2, 'sha1', b'pass', b'salt', 1, -1)
        with self.assertRaisesRegex(ValueError, 'unsupported hash type'):
            pbkdf2('unknown', b'pass', b'salt', 1)

    def test_pbkdf2_hmac_py(self):
        self._test_pbkdf2_hmac(py_hashlib.pbkdf2_hmac)

    @unittest.skipUnless(hasattr(c_hashlib, 'pbkdf2_hmac'),
                     '   test requires OpenSSL > 1.0')
    def test_pbkdf2_hmac_c(self):
        self._test_pbkdf2_hmac(c_hashlib.pbkdf2_hmac)


if __name__ == "__main__":
    unittest.main()
