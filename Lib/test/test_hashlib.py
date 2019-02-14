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
import sys
try:
    import threading
except ImportError:
    threading = None
import unittest
import warnings
from binascii import unhexlify

from test import test_support
from test.test_support import _4G, precisionbigmemtest

# Were we compiled --with-pydebug or with #define Py_DEBUG?
COMPILED_WITH_PYDEBUG = hasattr(sys, 'gettotalrefcount')


def hexstr(s):
    import string
    h = string.hexdigits
    r = ''
    for c in s:
        i = ord(c)
        r = r + h[(i >> 4) & 0xF] + h[i & 0xF]
    return r


class HashLibTestCase(unittest.TestCase):
    supported_hash_names = ( 'md5', 'MD5', 'sha1', 'SHA1',
                             'sha224', 'SHA224', 'sha256', 'SHA256',
                             'sha384', 'SHA384', 'sha512', 'SHA512' )

    _warn_on_extension_import = COMPILED_WITH_PYDEBUG

    def _conditional_import_module(self, module_name):
        """Import a module and return a reference to it or None on failure."""
        try:
            exec('import '+module_name)
        except ImportError, error:
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
            assert hasattr(_hashlib, 'openssl_md5')
            assert hasattr(_hashlib, 'openssl_sha1')
            for algorithm, constructors in self.constructors_to_test.items():
                constructor = getattr(_hashlib, 'openssl_'+algorithm, None)
                if constructor:
                    constructors.add(constructor)

        _md5 = self._conditional_import_module('_md5')
        if _md5:
            self.constructors_to_test['md5'].add(_md5.new)
        _sha = self._conditional_import_module('_sha')
        if _sha:
            self.constructors_to_test['sha1'].add(_sha.new)
        _sha256 = self._conditional_import_module('_sha256')
        if _sha256:
            self.constructors_to_test['sha224'].add(_sha256.sha224)
            self.constructors_to_test['sha256'].add(_sha256.sha256)
        _sha512 = self._conditional_import_module('_sha512')
        if _sha512:
            self.constructors_to_test['sha384'].add(_sha512.sha384)
            self.constructors_to_test['sha512'].add(_sha512.sha512)

        super(HashLibTestCase, self).__init__(*args, **kwargs)

    def test_hash_array(self):
        a = array.array("b", range(10))
        constructors = self.constructors_to_test.itervalues()
        for cons in itertools.chain.from_iterable(constructors):
            c = cons(a)
            c.hexdigest()

    def test_algorithms_attribute(self):
        self.assertEqual(hashlib.algorithms,
            tuple([_algo for _algo in self.supported_hash_names if
                                                _algo.islower()]))

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
        get_builtin_constructor = hashlib.__dict__[
                '__get_builtin_constructor']
        self.assertRaises(ValueError, get_builtin_constructor, 'test')
        try:
            import _md5
        except ImportError:
            pass
        # This forces an ImportError for "import _md5" statements
        sys.modules['_md5'] = None
        try:
            self.assertRaises(ValueError, get_builtin_constructor, 'md5')
        finally:
            if '_md5' in locals():
                sys.modules['_md5'] = _md5
            else:
                del sys.modules['_md5']
        self.assertRaises(TypeError, get_builtin_constructor, 3)

    def test_hexdigest(self):
        for name in self.supported_hash_names:
            h = hashlib.new(name)
            self.assertTrue(hexstr(h.digest()) == h.hexdigest())

    def test_large_update(self):
        aas = 'a' * 128
        bees = 'b' * 127
        cees = 'c' * 126
        abcs = aas + bees + cees

        for name in self.supported_hash_names:
            m1 = hashlib.new(name)
            m1.update(aas)
            m1.update(bees)
            m1.update(cees)

            m2 = hashlib.new(name)
            m2.update(abcs)
            self.assertEqual(m1.digest(), m2.digest(), name+' update problem.')

            m3 = hashlib.new(name, abcs)
            self.assertEqual(m1.digest(), m3.digest(), name+' new problem.')

    def check(self, name, data, digest):
        constructors = self.constructors_to_test[name]
        # 2 is for hashlib.name(...) and hashlib.new(name, ...)
        self.assertGreaterEqual(len(constructors), 2)
        for hash_object_constructor in constructors:
            computed = hash_object_constructor(data).hexdigest()
            self.assertEqual(
                    computed, digest,
                    "Hash algorithm %s constructed using %s returned hexdigest"
                    " %r for %d byte input data that should have hashed to %r."
                    % (name, hash_object_constructor,
                       computed, len(data), digest))

    def check_update(self, name, data, digest):
        constructors = self.constructors_to_test[name]
        # 2 is for hashlib.name(...) and hashlib.new(name, ...)
        self.assertGreaterEqual(len(constructors), 2)
        for hash_object_constructor in constructors:
            h = hash_object_constructor()
            h.update(data)
            computed = h.hexdigest()
            self.assertEqual(
                    computed, digest,
                    "Hash algorithm %s using %s when updated returned hexdigest"
                    " %r for %d byte input data that should have hashed to %r."
                    % (name, hash_object_constructor,
                       computed, len(data), digest))

    def check_unicode(self, algorithm_name):
        # Unicode objects are not allowed as input.
        expected = hashlib.new(algorithm_name, str(u'spam')).hexdigest()
        self.check(algorithm_name, u'spam', expected)

    def test_unicode(self):
        # In python 2.x unicode is auto-encoded to the system default encoding
        # when passed to hashlib functions.
        self.check_unicode('md5')
        self.check_unicode('sha1')
        self.check_unicode('sha224')
        self.check_unicode('sha256')
        self.check_unicode('sha384')
        self.check_unicode('sha512')

    def test_case_md5_0(self):
        self.check('md5', '', 'd41d8cd98f00b204e9800998ecf8427e')

    def test_case_md5_1(self):
        self.check('md5', 'abc', '900150983cd24fb0d6963f7d28e17f72')

    def test_case_md5_2(self):
        self.check('md5', 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789',
                   'd174ab98d277d9f5a5611c2c9f419d9f')

    @unittest.skipIf(sys.maxsize < _4G + 5, 'test cannot run on 32-bit systems')
    @precisionbigmemtest(size=_4G + 5, memuse=1, dry_run=False)
    def test_case_md5_huge(self, size):
        self.check('md5', 'A'*size, 'c9af2dff37468ce5dfee8f2cfc0a9c6d')

    @unittest.skipIf(sys.maxsize < _4G + 5, 'test cannot run on 32-bit systems')
    @precisionbigmemtest(size=_4G + 5, memuse=1, dry_run=False)
    def test_case_md5_huge_update(self, size):
        self.check_update('md5', 'A'*size, 'c9af2dff37468ce5dfee8f2cfc0a9c6d')

    @unittest.skipIf(sys.maxsize < _4G - 1, 'test cannot run on 32-bit systems')
    @precisionbigmemtest(size=_4G - 1, memuse=1, dry_run=False)
    def test_case_md5_uintmax(self, size):
        self.check('md5', 'A'*size, '28138d306ff1b8281f1a9067e1a1a2b3')

    # use the three examples from Federal Information Processing Standards
    # Publication 180-1, Secure Hash Standard,  1995 April 17
    # http://www.itl.nist.gov/div897/pubs/fip180-1.htm

    def test_case_sha1_0(self):
        self.check('sha1', "",
                   "da39a3ee5e6b4b0d3255bfef95601890afd80709")

    def test_case_sha1_1(self):
        self.check('sha1', "abc",
                   "a9993e364706816aba3e25717850c26c9cd0d89d")

    def test_case_sha1_2(self):
        self.check('sha1', "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
                   "84983e441c3bd26ebaae4aa1f95129e5e54670f1")

    def test_case_sha1_3(self):
        self.check('sha1', "a" * 1000000,
                   "34aa973cd4c4daa4f61eeb2bdbad27316534016f")

    @precisionbigmemtest(size=_4G + 5, memuse=1)
    def test_case_sha1_huge(self, size):
        if size == _4G + 5:
            try:
                self.check('sha1', 'A'*size,
                        '87d745c50e6b2879ffa0fb2c930e9fbfe0dc9a5b')
            except OverflowError:
                pass # 32-bit arch

    @precisionbigmemtest(size=_4G + 5, memuse=1)
    def test_case_sha1_huge_update(self, size):
        if size == _4G + 5:
            try:
                self.check_update('sha1', 'A'*size,
                        '87d745c50e6b2879ffa0fb2c930e9fbfe0dc9a5b')
            except OverflowError:
                pass # 32-bit arch

    # use the examples from Federal Information Processing Standards
    # Publication 180-2, Secure Hash Standard,  2002 August 1
    # http://csrc.nist.gov/publications/fips/fips180-2/fips180-2.pdf

    def test_case_sha224_0(self):
        self.check('sha224', "",
          "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f")

    def test_case_sha224_1(self):
        self.check('sha224', "abc",
          "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7")

    def test_case_sha224_2(self):
        self.check('sha224',
          "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "75388b16512776cc5dba5da1fd890150b0c6455cb4f58b1952522525")

    def test_case_sha224_3(self):
        self.check('sha224', "a" * 1000000,
          "20794655980c91d8bbb4c1ea97618a4bf03f42581948b2ee4ee7ad67")


    def test_case_sha256_0(self):
        self.check('sha256', "",
          "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")

    def test_case_sha256_1(self):
        self.check('sha256', "abc",
          "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")

    def test_case_sha256_2(self):
        self.check('sha256',
          "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
          "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1")

    def test_case_sha256_3(self):
        self.check('sha256', "a" * 1000000,
          "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0")


    def test_case_sha384_0(self):
        self.check('sha384', "",
          "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da"+
          "274edebfe76f65fbd51ad2f14898b95b")

    def test_case_sha384_1(self):
        self.check('sha384', "abc",
          "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed"+
          "8086072ba1e7cc2358baeca134c825a7")

    def test_case_sha384_2(self):
        self.check('sha384',
                   "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"+
                   "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
          "09330c33f71147e83d192fc782cd1b4753111b173b3b05d22fa08086e3b0f712"+
          "fcc7c71a557e2db966c3e9fa91746039")

    def test_case_sha384_3(self):
        self.check('sha384', "a" * 1000000,
          "9d0e1809716474cb086e834e310a4a1ced149e9c00f248527972cec5704c2a5b"+
          "07b8b3dc38ecc4ebae97ddd87f3d8985")


    def test_case_sha512_0(self):
        self.check('sha512', "",
          "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce"+
          "47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e")

    def test_case_sha512_1(self):
        self.check('sha512', "abc",
          "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"+
          "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f")

    def test_case_sha512_2(self):
        self.check('sha512',
                   "abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"+
                   "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu",
          "8e959b75dae313da8cf4f72814fc143f8f7779c6eb9f7fa17299aeadb6889018"+
          "501d289e4900f7e4331b99dec4b5433ac7d329eeb6dd26545e96e55b874be909")

    def test_case_sha512_3(self):
        self.check('sha512', "a" * 1000000,
          "e718483d0ce769644e2e42c7bc15b4638e1f98b13b2044285632a803afa973eb"+
          "de0ff244877ea60a4cb0432ce577c31beb009c5c2c49aa2e4eadb217ad8cc09b")

    @unittest.skipUnless(threading, 'Threading required for this test.')
    @test_support.reap_threads
    def test_threaded_hashing(self):
        # Updating the same hash object from several threads at once
        # using data chunk sizes containing the same byte sequences.
        #
        # If the internal locks are working to prevent multiple
        # updates on the same object from running at once, the resulting
        # hash will be the same as doing it single threaded upfront.
        hasher = hashlib.sha1()
        num_threads = 5
        smallest_data = 'swineflu'
        data = smallest_data*200000
        expected_hash = hashlib.sha1(data*num_threads).hexdigest()

        def hash_in_chunks(chunk_size):
            index = 0
            while index < len(data):
                hasher.update(data[index:index+chunk_size])
                index += chunk_size

        threads = []
        for threadnum in xrange(num_threads):
            chunk_size = len(data) // (10**threadnum)
            assert chunk_size > 0
            assert chunk_size % len(smallest_data) == 0
            thread = threading.Thread(target=hash_in_chunks,
                                      args=(chunk_size,))
            threads.append(thread)

        for thread in threads:
            thread.start()
        for thread in threads:
            thread.join()

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
            # official test vectors from RFC 6070
            (unhexlify('0c60c80f961f0e71f3a9b524af6012062fe037a6'), None),
            (unhexlify('ea6c014dc72d6f8ccd1ed92ace1d41f0d8de8957'), None),
            (unhexlify('4b007901b765489abead49d926f721d065a429c1'), None),
            #(unhexlify('eefe3d61cd4da4e4e9945b3d6ba2158c2634e984'), None),
            (unhexlify('3d2eec4fe41c849b80c8d83662c0e44a8b291a964c'
                           'f2f07038'), 25),
            (unhexlify('56fa6aa75548099dcc37d7f03425e0c3'), None),],
        "sha256": [
            (unhexlify('120fb6cffcf8b32c43e7225256c4f837'
                           'a86548c92ccc35480805987cb70be17b'), None),
            (unhexlify('ae4d0c95af6b46d32d0adff928f06dd0'
                           '2a303f8ef3c251dfd6e2d85a95474c43'), None),
            (unhexlify('c5e478d59288c841aa530db6845c4c8d'
                           '962893a001ce4e11a4963873aa98134a'), None),
            #(unhexlify('cf81c66fe8cfc04d1f31ecb65dab4089'
            #               'f7f179e89b3b0bcb17ad10e3ac6eba46'), None),
            (unhexlify('348c89dbcbd32b2f32d814b8116e84cf2b17'
                           '347ebc1800181c4e2a1fb8dd53e1c635518c7dac47e9'), 40),
            (unhexlify('89b69d0516f829893c696226650a8687'), None),],
        "sha512": [
            (unhexlify('867f70cf1ade02cff3752599a3a53dc4af34c7a669815ae5'
                           'd513554e1c8cf252c02d470a285a0501bad999bfe943c08f'
                           '050235d7d68b1da55e63f73b60a57fce'), None),
            (unhexlify('e1d9c16aa681708a45f5c7c4e215ceb66e011a2e9f004071'
                           '3f18aefdb866d53cf76cab2868a39b9f7840edce4fef5a82'
                           'be67335c77a6068e04112754f27ccf4e'), None),
            (unhexlify('d197b1b33db0143e018b12f3d1d1479e6cdebdcc97c5c0f8'
                           '7f6902e072f457b5143f30602641b3d55cd335988cb36b84'
                           '376060ecd532e039b742a239434af2d5'), None),
            (unhexlify('8c0511f4c6e597c6ac6315d8f0362e225f3c501495ba23b8'
                           '68c005174dc4ee71115b59f9e60cd9532fa33e0f75aefe30'
                           '225c583a186cd82bd4daea9724a3d3b8'), 64),
            (unhexlify('9d9e9c4cd21fe4be24d5b8244c759665'), None),],
    }

    def test_pbkdf2_hmac(self):
        for digest_name, results in self.pbkdf2_results.items():
            for i, vector in enumerate(self.pbkdf2_test_vectors):
                password, salt, rounds, dklen = vector
                expected, overwrite_dklen = results[i]
                if overwrite_dklen:
                    dklen = overwrite_dklen
                out = hashlib.pbkdf2_hmac(
                    digest_name, password, salt, rounds, dklen)
                self.assertEqual(out, expected,
                                 (digest_name, password, salt, rounds, dklen))


def test_main():
    test_support.run_unittest(HashLibTestCase, KDFTests)

if __name__ == "__main__":
    test_main()
