#
# Test suite to check compliance with PEP 247, the standard API for
# hashing algorithms.
#

import warnings
warnings.filterwarnings("ignore", "the md5 module is deprecated.*",
                        DeprecationWarning)
warnings.filterwarnings("ignore", "the sha module is deprecated.*",
                        DeprecationWarning)

import md5, sha, hmac
from test.test_support import verbose


def check_hash_module(module, key=None):
    assert hasattr(module, 'digest_size'), "Must have digest_size"
    assert (module.digest_size is None or
            module.digest_size > 0), "digest_size must be None or positive"

    if key is not None:
        obj1 = module.new(key)
        obj2 = module.new(key, "string")

        h1 = module.new(key, "string").digest()
        obj3 = module.new(key) ; obj3.update("string") ; h2 = obj3.digest()
        assert h1 == h2, "Hashes must match"

    else:
        obj1 = module.new()
        obj2 = module.new("string")

        h1 = module.new("string").digest()
        obj3 = module.new() ; obj3.update("string") ; h2 = obj3.digest()
        assert h1 == h2, "Hashes must match"

    assert hasattr(obj1, 'digest_size'), "Objects must have digest_size attr"
    if module.digest_size is not None:
        assert obj1.digest_size == module.digest_size, "digest_size must match"
    assert obj1.digest_size == len(h1), "digest_size must match actual size"
    obj1.update("string")
    obj_copy = obj1.copy()
    assert obj1.digest() == obj_copy.digest(), "Copied objects must match"
    assert obj1.hexdigest() == obj_copy.hexdigest(), \
           "Copied objects must match"
    digest, hexdigest = obj1.digest(), obj1.hexdigest()
    hd2 = ""
    for byte in digest:
        hd2 += "%02x" % ord(byte)
    assert hd2 == hexdigest, "hexdigest doesn't appear correct"

    if verbose:
        print 'Module', module.__name__, 'seems to comply with PEP 247'


def test_main():
    check_hash_module(md5)
    check_hash_module(sha)
    check_hash_module(hmac, key='abc')


if __name__ == '__main__':
    test_main()
